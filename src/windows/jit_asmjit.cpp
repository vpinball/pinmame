// ---------------------------------------------------------------------------
//  asmjit-based ARM7 JIT backend
//
//  Compiled into the build only when PINMAME_JIT_ASMJIT is defined (see
//  cmake/asmjit.cmake and the vcproj VS2017+ conversion). Emulator core (arm7core.c/arm7exec.c/at91.c) uses it
//
//  Contents:
//   - ARM7 -> host translator: decodes a subset of ARM7 and emits host code via
//     the asmjit Assembler, using the block ABI -- a C-callable
//     function `uint32_t block(void* ctx)` that mutates the register file in
//     place and returns the next emulated PC. ctx is &ARM7.sArmRegister[0]
//     (reg i at i*4, CPSR at 16). Two emitter backends at full parity: x86/x64
//     and AArch64 (AJ_HOST_*); decode, routing, controller, and tests are shared.
//   - JIT controller (ArmAsmjitCtl + arm7_aj_*): address->block map + JitRuntime;
//     translate-on-demand, cache, SMC-invalidate.
//   - Self-tests: an optional code-generation regression suite, run via the single entry
//     point jit_asmjit_run_selftests() (headless, for CI) or
//     jit_asmjit_selftest_report() (its Win32 MessageBox wrapper). The
//     individual tests are static and registered in the k_selftests[] table
// ---------------------------------------------------------------------------

#ifdef PINMAME_JIT_ASMJIT

//#define EXPOSE_JIT_SELFTEST

#include <asmjit/host.h> // core + the HOST backend (x86.h or a64.h)
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <vector>

#include "jit_asmjit.h" // C-callable controller API (arm7_aj_*, arm7_block_fn, ArmAsmjitCtl)

#if defined(EXPOSE_JIT_SELFTEST) && defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
#if _MSC_VER >= 1800
 // Windows 2000 _WIN32_WINNT_WIN2K
#define _WIN32_WINNT 0x0500
#elif _MSC_VER < 1600
#define _WIN32_WINNT 0x0400
#else
#define _WIN32_WINNT 0x0403
#endif
#define WINVER _WIN32_WINNT
#endif
#include <windows.h>
#endif

using namespace asmjit;

// Host-architecture selection. The controller (block cache, slots, coverage/SMC tracking, deferred release, dispatcher protocol, C API),
// the emit_arm_insn router, translate_block, and all self-tests are ISA-neutral and shared (via the host-backend glue below);
// only the per-instruction emitters and the dispatcher emission are per-arch, behind AJ_HOST_X86 (x86/x64) and AJ_HOST_A64 (AArch64).
// Both backends are at full feature and self-test parity
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#define AJ_HOST_X86 1
#elif defined(__aarch64__) || defined(_M_ARM64)
#define AJ_HOST_A64 1
#else
#error "jit_asmjit: unsupported architecture (x86/x64/AArch64 only)"
#endif

namespace {

// Layout chosen to match the real ARM7 active register file (ARM7.sArmRegister):
// R0..R15 at indices 0..15, CPSR at index 16 (eCPSR). The translated block's
// context pointer is &cpu (self-test) or &ARM7.sArmRegister[0] (real build), so
// reg_off()/cpsr_off() are identical in both. 'cycles' and 'mem' are self-test
// extras (the real build uses ARM7_ICOUNT and the read/write callbacks)
struct ArmCpuSelfTest {
    uint32_t r[16];    // R0..R15                    (indices 0..15)
    uint32_t cpsr;     // CPSR / NZCV at bit 31..28  (index 16 == eCPSR)
    int32_t  cycles;   // self-test only
    uint8_t  mem[256]; // self-test only
};

// The generated code reaches the register file as ctx-relative offsets
// (reg_off(i) = i*4, CPSR at 16*4). The real build passes &ARM7.sArmRegister[0],
// so these offsets MUST match the ARM7 layout (R0..R15 at 0..15, eCPSR at 16).
// These asserts pin the self-test context to that layout; if the ARM7 register
// enum (eCPSR) ever moves, update reg_off()/cpsr_off() to match
static_assert(offsetof(ArmCpuSelfTest, r) == 0,
              "register file must start the context so reg_off(i) == i*4");
static_assert(offsetof(ArmCpuSelfTest, cpsr) == 16 * sizeof(uint32_t),
              "CPSR must be at index 16 to match ARM7 sArmRegister / eCPSR");

typedef uint32_t (*SelfTestBlockFn)(ArmCpuSelfTest *cpu);

} // namespace

namespace {
// ---- host-backend glue ------------------------------------------------------
// The minimal per-arch vocabulary that lets the SHARED code (the emit_arm_insn router, translate_block, and the block-building self-test harnesses)
// drive either emitter backend; everything bigger stays per-arch behind AJ_HOST_*
#if AJ_HOST_X86
typedef x86::Assembler HostAssembler;
typedef x86::Gp        HostGp;
inline HostGp host_ctx_reg(const HostAssembler& a) { return a.zbx(); } // callee-saved ctx pointer
inline void host_add_block_regs(FuncFrame& f, const HostAssembler& a, const HostGp& ctx)
{ f.add_dirty_regs(ctx, a.zax(), a.zsi()); } // + esi: callee-saved shifter-carry stash
inline void emit_return_imm(HostAssembler& a, uint32_t v) { a.mov(x86::eax, v); }
inline void emit_jump(HostAssembler& a, const Label& l) { a.jmp(l); }
inline void emit_ctx_cycles_sub(HostAssembler& a, const HostGp& ctx, int n) // self-test ctx 'cycles' field
{ a.sub(x86::dword_ptr(ctx, (int)offsetof(ArmCpuSelfTest, cycles)), n); }
#else
typedef a64::Assembler HostAssembler;
typedef a64::Gp        HostGp;
inline HostGp host_ctx_reg(HostAssembler&) { return a64::x19; } // callee-saved (AAPCS64)
inline void host_add_block_regs(FuncFrame& f, HostAssembler&, const HostGp& ctx)
{ f.add_dirty_regs(ctx, a64::x30); } // w0-w3/w16/w17 scratch are caller-saved; x30 (LR) must
                                     // survive the blr thunk calls, so mark it saved always
inline void emit_return_imm(HostAssembler& a, uint32_t v) { a.mov(a64::w0, v); }
inline void emit_jump(HostAssembler& a, const Label& l) { a.b(l); }
inline void emit_ctx_cycles_sub(HostAssembler& a, const HostGp& ctx, int n)
{
    a.ldr(a64::w16, a64::ptr(ctx, (int)offsetof(ArmCpuSelfTest, cycles)));
    a.sub(a64::w16, a64::w16, n);
    a.str(a64::w16, a64::ptr(ctx, (int)offsetof(ArmCpuSelfTest, cycles)));
}
#endif
} // namespace

// Builds and runs a tiny translated block to validate the asmjit toolchain and
// the block ABI on the host architecture. Returns 1 on success, 0 on
// failure. Safe to call from C (e.g. a smoke test)
static int jit_asmjit_selftest(void)
{
    JitRuntime rt;

    CodeHolder code;
    code.init(rt.environment());

    const int OFS_R0       = (int)(offsetof(ArmCpuSelfTest, r) + 0 * sizeof(uint32_t));
    const int OFS_R1       = (int)(offsetof(ArmCpuSelfTest, r) + 1 * sizeof(uint32_t));
    const int OFS_CYCLES   = (int)offsetof(ArmCpuSelfTest, cycles);
    const uint32_t NEXT_PC = 0x0000100Cu;

    HostAssembler a(&code);

    FuncDetail func;
    func.init(FuncSignature::build<uint32_t, ArmCpuSelfTest *>(), code.environment());

    FuncFrame frame;
    frame.init(func);

    HostGp ctx = host_ctx_reg(a);
    host_add_block_regs(frame, a, ctx);

    FuncArgsAssignment argsAsg(&func);
    argsAsg.assign_all(ctx);
    argsAsg.update_func_frame(frame);
    frame.finalize();

    a.emit_prolog(frame);
    a.emit_args_assignment(frame, argsAsg);

#if AJ_HOST_X86
    a.mov(x86::dword_ptr(ctx, OFS_R0), 10);       // r0 = 10
    a.mov(x86::eax, x86::dword_ptr(ctx, OFS_R0)); // eax = r0
    a.add(x86::eax, x86::eax);                    // eax += eax
    a.mov(x86::dword_ptr(ctx, OFS_R1), x86::eax); // r1 = 20
    a.sub(x86::dword_ptr(ctx, OFS_CYCLES), 3);    // cycles -= 3
    a.mov(x86::eax, NEXT_PC);                     // return next PC

    a.emit_epilog(frame);
#else // AJ_HOST_A64: same block, emitted with the AArch64 assembler
    a.mov(a64::w0, 10);                        // r0 = 10
    a.str(a64::w0, a64::ptr(ctx, OFS_R0));
    a.ldr(a64::w0, a64::ptr(ctx, OFS_R0));     // w0 = r0
    a.add(a64::w0, a64::w0, a64::w0);          // w0 += w0
    a.str(a64::w0, a64::ptr(ctx, OFS_R1));     // r1 = 20
    a.ldr(a64::w1, a64::ptr(ctx, OFS_CYCLES));
    a.sub(a64::w1, a64::w1, 3);
    a.str(a64::w1, a64::ptr(ctx, OFS_CYCLES)); // cycles -= 3
    a.mov(a64::w0, NEXT_PC);                   // return next PC

    a.emit_epilog(frame);
#endif

    SelfTestBlockFn fn = nullptr;
    if (rt.add(&fn, &code) != kErrorOk)
        return 0;

    ArmCpuSelfTest cpu{};
    cpu.cycles = 100;
    uint32_t pc = fn(&cpu);
    rt.release(fn);

    return (cpu.r[0] == 10 && cpu.r[1] == 20 && cpu.cycles == 97 && pc == NEXT_PC) ? 1 : 0;
}

// ---------------------------------------------------------------------------
//  ARM7 -> asmjit translator
//
//  Decodes the ARM7 subset below and emits host code via asmjit using the
//  block ABI. The emulated register file is reached as offsets from the
//  context pointer (ctx), never via embedded absolute addresses; the decode/
//  routing here is shared, and per-instruction emitters exist for both host
//  backends (AJ_HOST_X86 / AJ_HOST_A64), so blocks run on x86, x64 and ARM64.
//
//  Coverage: all data-processing ops -- operand 2 an immediate or a register
//  with immediate/register-specified shift amounts (LSL/LSR/ASR/ROR incl.
//  ARM's amount>=32 edge cases and the LSR/ASR #0 (=#32) / ROR #0 (=RRX)
//  special forms); all condition codes; full S=1 flags for arithmetic ops
//  (N/Z/C/V captured from the HOST flags of the add/sub/adc/sbb itself, see
//  the flag helpers) and for logical ops (C = shifter carry-out, computed for
//  rotated immediates and immediate shift amounts). R15 source operands read
//  the pipelined PC (+8, or +12 with a register-specified shift amount) as
//  translate-time constants; DP writes to PC end the block, with S=1 (MOVS PC
//  / SUBS PC,LR = exception return) going through the arm7_aj_exc_return
//  helper (SPSR->CPSR + bank switch + IRQ check; Callback mode only).
//  Single data transfer LDR/STR/LDRB/STRB (immediate or shifted-register
//  offset, pre/post-index, writeback; PC base reads instr+8, STR of PC stores
//  instr+12; LDR into PC and PC-base writeback end the block). Halfword/
//  signed transfers LDRH/STRH/LDRSB/LDRSH. Block transfer LDM/STM (all 4
//  modes, writeback, STM of PC, LDM with PC = return). Branches B/BL. BX
//  (unmasked PC + T bit on odd, interpreter parity). SWP/SWPB (Rd aliasing
//  Rn/Rm deferred). Multiplies MUL/MLA and UMULL/SMULL/UMLAL/SMLAL (S=1: N/Z
//  from the result, C/V preserved -- interpreter parity; R15 operands
//  deferred). PSR transfers MRS/MSR via the emulator's HandlePSRTransfer
//  (exact interpreter semantics incl. SPSR banking; Callback mode only).
//
//  Still deferred to the interpreter: S=1 logical with a register-specified
//  shift AMOUNT (run-time shifter carry), Rs==15, S=1 LDM/STM (user-bank/
//  SPSR restore), the deprecated TSTP/TEQP/CMPP/CMNP forms, coprocessor/SWI.
//  Anything unsupported returns "unsupported" -> interpreter fallback
// ---------------------------------------------------------------------------

namespace {

// Byte offset of emulated register 'idx' within the CPU context
inline int reg_off(int idx)
{
    return (int)offsetof(ArmCpuSelfTest, r) + idx * (int)sizeof(uint32_t);
}

// ARM CPSR flag masks (ARM bit positions)
constexpr uint32_t kN = 1u << 31, kZ = 1u << 30, kC = 1u << 29, kV = 1u << 28;

// Translate-time memory mode (single-threaded translation):
//   Mock     memory transfers hit the ctx mock buffer ([ctx + addr + memOff]).
//            Used ONLY by the self-tests (the buffer lives in ArmCpuSelfTest).
//   Callback memory transfers emit a host call to the emulator's real
//            read/write functions (s_memcb). The real-build hot path.
//   Defer    memory transfers report "unsupported" so the block ends and the
//            interpreter performs the access (real build with no callbacks set;
//            also the fallback for transfer forms the translator does not emit)
enum class MemMode { Mock, Callback, Defer };
MemMode s_memMode = MemMode::Mock;

// Real-build memory callbacks (addresses of arm7_cpu_read*/write* thunks), set
// per controller and copied here for the duration of a translate_block() call.
// Reads:  uint32_t fn(uint32_t addr)  (byte/half results zero/sign-handled
//         by the thunk; LDRB uses r8 which returns the 0..255 byte value)
// Writes: void fn(uint32_t addr, uint32_t data).
struct MemCallbacks {
    const void *r8, *r16, *r32, *w8, *w16, *w32;
    // gen_test_abort / gen_test_irq parity with the legacy JIT (arm7jit.c): a load
    // or store can touch a peripheral that raises a data abort or an IRQ/FIQ, and the
    // exception must vector before the rest of the block runs. pAbtD/pIrq/pFiq are the
    // addresses of the pending-flag bytes; after a memory access the block tests them
    // and, if any is set, exits to the emulator (whose ARM7_CHECKIRQ then vectors).
    // check_irq is the emulator's arm7_check_irq_state -- currently informational
    // (the exit-to-emulator path needs no in-JIT call), reserved for a future in-JIT
    // fast-path that would avoid breaking the block when an IRQ is pending-but-masked.
    // pIrqFlag is a byte the block sets on exit to request that the exec loop run
    // ARM7_CHECKIRQ (so the loop vectors only at the interpreter's check points, not
    // after every block -- see arm7_aj_irq_flag / arm7exec.c)
    const void *check_irq, *pAbtD, *pIrq, *pFiq, *pIrqFlag;
    // The emulator's HandlePSRTransfer(insn) -- void fn(uint32_t). With it wired,
    // translated blocks handle MRS/MSR by calling it (exact interpreter semantics,
    // incl. SPSR banking and mode switches); without it those end the block for the
    // interpreter. Wired via arm7_aj_set_psr_transfer
    const void *psr_transfer;
    // The emulator's exception-return helper -- uint32_t fn(uint32_t newpc)
    // (arm7_aj_exc_return in arm7core.c). Implements the interpreter's HandleALU
    // S=1/Rd=R15 sequence: SPSR->CPSR, SwitchMode, PC mask, ARM7_CHECKIRQ; returns
    // the final PC. With it wired, blocks handle MOVS PC / SUBS PC,LR,... (IRQ
    // exit); without it those defer. Wired via arm7_aj_set_exc_return
    const void *exc_return;
};
MemCallbacks s_memcb = {};

// Translate-time copy of the controller's ARM7_ICOUNT pointer (arm7_aj_set_icount).
// When set, blocks charge their own cycles AT EACH EXIT with the cost of the path
// actually executed (see emit_charge_cycles) -- so an early abort/IRQ exit charges
// only the executed prefix (no double count when the tail re-runs as its own
// block), and a condition-false instruction refunds down to ARM's 1 cycle.
// Null (self-tests / unwired): no charge code is emitted; such blocks are never
// run by arm7_aj_run anyway (it requires the wired pointer).
int *s_picount = nullptr;

inline int cpsr_off() { return (int)offsetof(ArmCpuSelfTest, cpsr); }

//!! TODO (perf): inline RAM fast path for LDR/STR:
// Every memory access currently pays the full C thunk -> PinMAME memory-map
// dispatch (~50-150 host insns); loads/stores might be roughly a third of ARM code.
// Rough outline:
//  - New wiring, e.g. arm7_aj_set_fast_ram(ctl, hostBase, guestLo, guestHi),
//    fed from the driver's RAM window (see at91_set_ram_pointers /
//    at91_init_jit; only valid POST-remap, which is exactly when the JIT is
//    enabled). Resets the controller; translate-time copy like s_memcb.
//  - In emit_arm_ldrstr (word/byte first; halfword & LDM/STM later), after the
//    effective address is in a register: inline
//        if ((addr - guestLo) < (guestHi - guestLo) && !(addr & 3))  // word case
//            direct MOV via [hostBase + (addr - guestLo)]
//        else -> existing thunk call
//    The alignment test matters: arm7_cpu_read32 ROTATES unaligned word reads
//    (see READ32), so unaligned must take the slow path. LDRB needs no
//    alignment test; LDRH needs (addr & 1) == 0.
//  - STORES must keep SMC detection: also test cover[(addr-minAddr)>>2] != 0
//    inline (one byte load; the array address can be embedded since 'this' is
//    stable but the ARRAY is reallocated by set_range -> read the c->cover cell
//    like the dispatcher reads c->slot) and fall into the thunk when covered,
//    which performs the untranslate. Fast path must be dropped (controller
//    reset) if the RAM window changes.
//  - The abort/IRQ post-check stays AFTER either path (a RAM access can't
//    raise, but keeping one code shape is simpler and the check is cheap).
//  - Not applicable to the peripheral region (>= 0xFFC00000) or CS callbacks by
//    construction -- those fall outside [guestLo, guestHi) and keep the thunk.

#if AJ_HOST_X86 // ================== x86/x64 emitter backend ==================

// Emit an ABI-correct call to a C memory thunk. Precondition: the effective
// address is in ECX and, for writes, the store value is in EDX. For reads the
// result is left in EAX. The host calling convention is selected at compile time
// because the JIT only ever generates code for the host it was compiled for; the
// block's FuncFrame must have reserved call stack space (set_call_stack_size, see
// translate_block). Standardizing on addr=ECX/val=EDX makes the dominant Win64
// target a zero-move shuffle (arg0=RCX, arg1=RDX)
inline void emit_mem_call(x86::Assembler& a, const void* fn, bool isWrite)
{
#if defined(_WIN64)
    // Win64: arg0=RCX (=addr), arg1=RDX (=val) already in place; 32B shadow space is part of the reserved call stack. Result in EAX
    (void)isWrite;
    a.mov(x86::rax, (uint64_t)(uintptr_t)fn);
    a.call(x86::rax);
#elif defined(__x86_64__)
    // System V AMD64 (Linux/macOS x64): arg0=RDI, arg1=RSI. Result in EAX.
    a.mov(x86::edi, x86::ecx);
    if (isWrite) a.mov(x86::esi, x86::edx);
    a.mov(x86::rax, (uint64_t)(uintptr_t)fn);
    a.call(x86::rax);
#elif defined(_WIN32) || defined(__i386__)
    // cdecl (x86): args pushed right-to-left; we write them into the reserved
    // outgoing-arg area at [ESP+0/+4] (no per-call cleanup -- the area is part of the fixed frame). Result in EAX
    if (isWrite) a.mov(x86::dword_ptr(x86::esp, 4), x86::edx);
    a.mov(x86::dword_ptr(x86::esp, 0), x86::ecx);
    a.mov(x86::eax, (uint32_t)(uintptr_t)fn);
    a.call(x86::eax);
#else
#  error "asmjit JIT memory callbacks: unsupported host ABI"
#endif
}

// Load a host pointer constant into 'r' (arch-width: 64-bit on x64, 32-bit on x86)
inline void emit_mov_ptr(x86::Assembler& a, const x86::Gp& r, const void* p)
{
#if defined(_WIN64) || defined(__x86_64__)
    a.mov(r, (uint64_t)(uintptr_t)p);
#else
    a.mov(r, (uint32_t)(uintptr_t)p);
#endif
}

// Emit "ARM7_ICOUNT -= cyc" (or += for a negative refund) using 'scratch' for
// the pointer. No-op in uncharged mode (s_picount null) or when cyc == 0
inline void emit_charge_cycles(x86::Assembler& a, int cyc, const x86::Gp& scratch)
{
    if (!s_picount || cyc == 0) return;
    emit_mov_ptr(a, scratch, s_picount);
    if (cyc > 0) a.sub(x86::dword_ptr(scratch), cyc);
    else         a.add(x86::dword_ptr(scratch), -cyc);
}

// Abort/IRQ check emitted right after a SINGLE data transfer (LDR/STR) in Callback
// mode, and ONLY there -- matching the interpreter, which runs ARM7_CHECKIRQ after a
// single data transfer (arm7exec.c case 4-7) but not after LDM/STM or the DP/halfword
// class. A load/store can touch a peripheral that raises a data abort or an IRQ/FIQ;
// this yields the same observable result as the legacy JIT WITHOUT calling into the
// emulator from generated code (ABI/stack-fragile): the block exits at the next-
// instruction PC and the exec loop's ARM7_CHECKIRQ vectors it. Exit ONLY when the
// interrupt would truly vector -- pending AND unmasked (CPSR I/F) -- mirroring
// arm7_check_irq_state; a pending-but-masked interrupt must not break the block (that
// caused spurious exits + cycle over-count). Cheap guard (byte/flag tests, no call).
// NB: an early exit charges the block's full cycle total (minor over-count); the abort
// test runs after this op's side effects (unobservable on the AT91: no MMU, no data
// abort). No-op unless the hooks are wired (self-test/Mock paths leave them null)
inline void emit_mem_post_checks(x86::Assembler& a, const x86::Gp& ctx, uint32_t pc, const FuncFrame& frame, int cyc)
{
    if (!s_memcb.pIrq || !s_memcb.pIrqFlag) return; // hooks not wired -> no codegen
    using CC = x86::CondCode;
    const x86::Mem cpsr = x86::dword_ptr(ctx, cpsr_off());
    Label do_exit = a.new_label();
    Label l_irq   = a.new_label();
    Label cont    = a.new_label();
    // Exit the block (so the exec loop's ARM7_CHECKIRQ vectors) ONLY when an
    // exception would ACTUALLY be taken -- mirroring the legacy JIT, which diverts
    // only if arm7_check_irq_state changed R15. A pending-but-MASKED interrupt must
    // NOT break the block: doing so caused frequent spurious exits, which threw off
    // MAME timing. Pending bytes via movzx+test; mask bits via
    // test mem,imm against the CPSR -- both proven patterns above.
    // 'cyc' is the cost of the instructions executed UP TO AND INCLUDING this one:
    // the exit charges exactly that, so the unexecuted tail is neither charged
    // here nor double-charged when it later runs as its own block from the resume PC.
    // Data abort: always taken (AT91 never raises one, so this arm is effectively dead).
    emit_mov_ptr(a, a.zax(), s_memcb.pAbtD); a.movzx(x86::eax, x86::byte_ptr(a.zax())); a.test(x86::eax, x86::eax); a.j(CC::kNotZero, do_exit);
    // FIQ: taken if pending AND F (FIQ-disable, bit 6) clear.
    emit_mov_ptr(a, a.zax(), s_memcb.pFiq);  a.movzx(x86::eax, x86::byte_ptr(a.zax())); a.test(x86::eax, x86::eax); a.j(CC::kZero, l_irq);
    a.test(cpsr, 0x40u); a.j(CC::kZero, do_exit);     // (cpsr & F_MASK)==0 -> unmasked -> vector
    a.bind(l_irq);
    // IRQ: taken if pending AND I (IRQ-disable, bit 7) clear.
    emit_mov_ptr(a, a.zax(), s_memcb.pIrq);  a.movzx(x86::eax, x86::byte_ptr(a.zax())); a.test(x86::eax, x86::eax); a.j(CC::kZero, cont);
    a.test(cpsr, 0x80u); a.j(CC::kNotZero, cont);     // (cpsr & I_MASK)!=0 -> masked -> continue in-block
    a.bind(do_exit);
    // Request the IRQ check in the exec loop (byte write via a register; mov m8,r8 is
    // a proven encoding here). The exec loop runs ARM7_CHECKIRQ only when this is set,
    // so it vectors at the interpreter's check points (single data transfers) only.
    emit_mov_ptr(a, a.zax(), s_memcb.pIrqFlag);
    a.mov(x86::ecx, 1);
    a.mov(x86::byte_ptr(a.zax()), x86::cl);
    emit_charge_cycles(a, cyc, a.zdx()); // executed prefix only (see above)
    a.mov(x86::eax, pc + 4u); // resume at the next instruction; the exec loop then vectors
    a.emit_epilog(frame);
    a.bind(cont);
}

// Leaves (N ^ V) in bit 31 of EAX (clobbers EAX, EDX). Caller then does
// `test eax, kN` -> ZF set means N==V, ZF clear means N!=V
void emit_nv_into_eax(x86::Assembler& a, const x86::Gp& ctx)
{
    a.mov(x86::eax, x86::dword_ptr(ctx, cpsr_off()));
    a.mov(x86::edx, x86::eax);
    a.shl(x86::edx, 3);         // V (bit 28) -> bit 31
    a.xor_(x86::eax, x86::edx); // bit 31 = N ^ V
}

// Portable ARM condition evaluation (no x86 EFLAGS round-trip): emit code that
// jumps to 'skip' when condition 'cond' (insn bits 31-28) is FALSE, so the
// instruction body is bypassed. Emits nothing for AL. Returns false for NV
bool emit_cond_skip(x86::Assembler& a, const x86::Gp& ctx, uint32_t cond, const Label& skip)
{
    const x86::Mem cpsr = x86::dword_ptr(ctx, cpsr_off());
    using CC = x86::CondCode;
    switch (cond) {
    case 0x0: a.test(cpsr, kZ); a.j(CC::kZero,    skip); return true; // EQ:  exec if Z
    case 0x1: a.test(cpsr, kZ); a.j(CC::kNotZero, skip); return true; // NE:  exec if !Z
    case 0x2: a.test(cpsr, kC); a.j(CC::kZero,    skip); return true; // CS:  exec if C
    case 0x3: a.test(cpsr, kC); a.j(CC::kNotZero, skip); return true; // CC:  exec if !C
    case 0x4: a.test(cpsr, kN); a.j(CC::kZero,    skip); return true; // MI:  exec if N
    case 0x5: a.test(cpsr, kN); a.j(CC::kNotZero, skip); return true; // PL:  exec if !N
    case 0x6: a.test(cpsr, kV); a.j(CC::kZero,    skip); return true; // VS:  exec if V
    case 0x7: a.test(cpsr, kV); a.j(CC::kNotZero, skip); return true; // VC:  exec if !V
    case 0x8: // HI: C && !Z   -> skip if !C, or if Z
        a.test(cpsr, kC); a.j(CC::kZero,    skip);
        a.test(cpsr, kZ); a.j(CC::kNotZero, skip);
        return true;
    case 0x9: { // LS: !C || Z -> skip if C && !Z
        Label exec = a.new_label();
        a.test(cpsr, kC); a.j(CC::kZero, exec); // C clear -> execute
        a.test(cpsr, kZ); a.j(CC::kZero, skip); // C set & Z clear -> skip
        a.bind(exec);
        return true;
    }
    case 0xA: // GE: N==V -> skip if N!=V
        emit_nv_into_eax(a, ctx); a.test(x86::eax, kN); a.j(CC::kNotZero, skip);
        return true;
    case 0xB: // LT: N!=V -> skip if N==V
        emit_nv_into_eax(a, ctx); a.test(x86::eax, kN); a.j(CC::kZero, skip);
        return true;
    case 0xC: // GT: !Z && N==V -> skip if Z, or if N!=V
        a.test(cpsr, kZ); a.j(CC::kNotZero, skip);
        emit_nv_into_eax(a, ctx); a.test(x86::eax, kN); a.j(CC::kNotZero, skip);
        return true;
    case 0xD: { // LE: Z || N!=V -> skip if !Z && N==V
        Label exec = a.new_label();
        a.test(cpsr, kZ); a.j(CC::kNotZero, exec);                            // Z set -> execute
        emit_nv_into_eax(a, ctx); a.test(x86::eax, kN); a.j(CC::kZero, skip); // Z clear & N==V -> skip
        a.bind(exec);
        return true;
    }
    case 0xE: return true;  // AL: always execute (no code)
    default:  return false; // 0xF NV: unsupported
    }
}

// Set N and Z in CPSR from the result in EAX, preserving V (and everything else).
// For logical ops C is preserved unless the operand-2 shifter defined a carry;
// if 'setC' is true, C is overwritten with 'cVal'. Clobbers ECX/EDX; EAX preserved
void emit_set_flags_logical(x86::Assembler& a, const x86::Gp& ctx, bool setC, bool cVal)
{
    const x86::Mem cpsr = x86::dword_ptr(ctx, cpsr_off());
    const uint32_t clearMask = kN | kZ | (setC ? kC : 0u);
    a.mov(x86::edx, cpsr);
    a.and_(x86::edx, ~clearMask); // clear N,Z (and C if we will set it)
    a.mov(x86::ecx, x86::eax);
    a.and_(x86::ecx, kN);         // N = result bit 31
    a.or_(x86::edx, x86::ecx);
    a.cmp(x86::eax, 1);           // Z, branchless: CF = (result == 0)
    a.sbb(x86::ecx, x86::ecx);    // all-ones when zero
    a.and_(x86::ecx, kZ);
    a.or_(x86::edx, x86::ecx);
    if (setC && cVal)
        a.or_(x86::edx, kC);
    a.mov(cpsr, x86::edx);
}

// Set N/Z in CPSR from the result in EAX and C from ESI bit 0 (the operand-2
// shifter's carry-out, computed at materialization time); V and everything else
// preserved. Used by flag-setting logical ops whose register operand 2 has an
// immediate-amount shift. Clobbers ECX/EDX/ESI; EAX preserved
void emit_set_flags_logical_carry_esi(x86::Assembler& a, const x86::Gp& ctx)
{
    const x86::Mem cpsr = x86::dword_ptr(ctx, cpsr_off());
    a.mov(x86::edx, cpsr);
    a.and_(x86::edx, ~(kN | kZ | kC));
    a.and_(x86::esi, 1u);
    a.shl(x86::esi, 29);        // carry bit 0 -> kC (bit 29)
    a.or_(x86::edx, x86::esi);
    a.mov(x86::ecx, x86::eax);
    a.and_(x86::ecx, kN);       // N = result bit 31
    a.or_(x86::edx, x86::ecx);
    a.cmp(x86::eax, 1);         // Z, branchless: CF = (result == 0)
    a.sbb(x86::ecx, x86::ecx);  // all-ones when zero
    a.and_(x86::ecx, kZ);
    a.or_(x86::edx, x86::ecx);
    a.mov(cpsr, x86::edx);
}

void emit_set_flags_after_carry(x86::Assembler& a, const x86::Gp& ctx, bool cIsBorrow); // below

// Set N/Z/C/V for an add/sub result by capturing the HOST flags of the add/sub
// that produced it. Precondition: the host add/sub was the last flag-writing
// instruction emitted (only flag-transparent MOVs in between, e.g. the Rd
// write-back). This replaced a fully recomputed portable version (bitwise
// overflow formulas + branches, ~20 insns) with the branchless setcc capture.
// isSub: ARM C = !borrow for subtraction-style ops. Clobbers EAX/ECX/EDX
void emit_set_flags_arith(x86::Assembler& a, const x86::Gp& ctx, bool isSub)
{
    emit_set_flags_after_carry(a, ctx, isSub); // identical capture (see there)
}

// Set N/Z/C/V after a host add/sub/adc/sbb. MUST be emitted while the host
// flags of that op are still valid (only flag-transparent MOVs in between): all
// four are captured branchlessly with single-bit setcc. For sub-style ops ARM
// C = !borrow, so pass cIsBorrow=true to capture C with kNotCarry (the a64
// backend needs no inversion -- see its twin). Uses only high/low byte
// regs of ECX/EDX (no REX conflict). Clobbers EAX (result no longer needed by
// any caller), ECX, EDX
void emit_set_flags_after_carry(x86::Assembler& a, const x86::Gp& ctx, bool cIsBorrow)
{
    using CC = x86::CondCode;
    a.set(CC::kOverflow, x86::dl);                          // V (0/1)
    a.set(CC::kSign,     x86::dh);                          // N
    a.set(cIsBorrow ? CC::kNotCarry : CC::kCarry, x86::cl); // C
    a.set(CC::kZero,     x86::ch);                          // Z
    a.movzx(x86::eax, x86::dl); a.shl(x86::eax, 28);                            // V -> bit 28
    a.movzx(x86::edx, x86::dh); a.shl(x86::edx, 31); a.or_(x86::eax, x86::edx); // N -> bit 31
    a.movzx(x86::edx, x86::cl); a.shl(x86::edx, 29); a.or_(x86::eax, x86::edx); // C -> bit 29
    a.movzx(x86::edx, x86::ch); a.shl(x86::edx, 30); a.or_(x86::eax, x86::edx); // Z -> bit 30
    a.mov(x86::edx, x86::dword_ptr(ctx, cpsr_off()));
    a.and_(x86::edx, ~(kN | kZ | kC | kV));                 // preserve mode bits etc.
    a.or_(x86::eax, x86::edx);
    a.mov(x86::dword_ptr(ctx, cpsr_off()), x86::eax);
}

// Emit the operation for one data-processing instruction. The condition is
// already handled by emit_arm_insn, so the cond bits are ignored here. Returns
// false (emitting nothing) if the instruction is outside the supported subset
bool emit_arm_dp(x86::Assembler& a, const x86::Gp& ctx, uint32_t insn, uint32_t pc)
{
    if (((insn >> 26) & 0x3u) != 0u) return false; // data-processing class (bits 27:26 == 00)

    const bool     useReg = !((insn >> 25) & 1u);
    const uint32_t opcode = (insn >> 21) & 0xFu;
    const bool     Sraw   = (insn >> 20) & 1u;
    const uint32_t Rn     = (insn >> 16) & 0xFu;
    const uint32_t Rd     = (insn >> 12) & 0xFu;
    const bool     compareOp = (opcode >= 0x8u && opcode <= 0xBu);
    if (Rd == 15u && Sraw) {
        // Deprecated 26-bit TSTP/TEQP/CMPP/CMNP forms (compare, Rd=PC, S=1): the
        // interpreter writes R15 = result and runs ARM7_CHECKIRQ -- nothing like a
        // plain compare -- so defer. (These were previously mis-emitted as plain
        // compares; no ARMv4 compiler emits them, but defer is the correct call.)
        if (compareOp) return false;
        // MOVS PC / SUBS PC,LR,... (exception return): needs the wired helper --
        // SPSR->CPSR restore, bank switch, ARM7_CHECKIRQ. emit_arm_insn emits the
        // call after this function stores the (flag-less) result to the R15 slot
        if (s_memMode != MemMode::Callback || !s_memcb.exc_return) return false;
    }
    // For a PC destination the operation NEVER sets flags: S=1 there means
    // "restore CPSR from SPSR" instead (the exception-return call above), so the
    // computation below always uses the S=0 form when Rd == 15
    const bool S = Sraw && Rd != 15u;
    if (Rn == 15u) {
        // Rn == R15 reads the pipelined PC, a translate-time constant (interpreter
        // HandleALU: + (I ? 8 : bit4 ? 12 : 8)). Park it in the R15 context slot --
        // stale/unused while a block runs, and overwritten by any Rd==15 result
        // below -- so every existing 'rn' memory-operand path just works
        const uint32_t addpc = ((insn >> 25) & 1u) ? 8u : ((insn & 0x10u) ? 12u : 8u);
        a.mov(x86::dword_ptr(ctx, reg_off(15)), pc + addpc);
    }
    // Rd == 15 (any S): the result is stored to the R15 slot as usual, then
    // emit_arm_insn turns it into a branch / exception return (block exit)

    // Materialize operand 2: either an immediate value (op2reg=false) or the
    // (optionally shifted) register value in ECX (op2reg=true)
    const bool op2reg = useReg;
    uint32_t   imm    = 0;
    bool       immCarryDefined = false; // operand-2 shifter carry is defined (rotated imm)...
    bool       immCarry        = false; // ...and this is its value (bit 31 of the rotated imm)
    // Flag-setting LOGICAL ops take their C flag from the operand-2 shifter's
    // carry-out (arithmetic ops ignore it). For an immediate shift AMOUNT the
    // carry bit index is a translate-time constant, computed below into ESI
    // (bit 0) from the pre-shift value; register-specified amounts with S=1
    // logical stay deferred (run-time amount -> many-way carry; rare)
    bool shiftCarryInEsi = false;
    if (useReg) {
        const uint32_t Rm  = insn & 0xFu;
        const uint32_t shf = (insn >> 4) & 0xFFu; // shift field (insn bits 11-4)
        const bool logicalS = S && (opcode == 0x0u || opcode == 0x1u || opcode == 0x8u ||
                                    opcode == 0x9u || opcode == 0xCu || opcode == 0xDu ||
                                    opcode == 0xEu || opcode == 0xFu);
        if ((shf & 1u) == 0u) {
            // Immediate shift amount. Operand-2 value goes straight into ECX.
            // Rm == R15 reads the pipelined PC, a translate-time constant
            // (interpreter decodeShift: +8 for an immediate shift amount)
            if (Rm == 15u) a.mov(x86::ecx, pc + 8u);
            else           a.mov(x86::ecx, x86::dword_ptr(ctx, reg_off((int)Rm)));
            if (shf != 0u) {
                const uint32_t stype = (shf >> 1) & 0x3u;  // insn bits 6-5
                const uint32_t sh    = (shf >> 3) & 0x1Fu; // insn bits 11-7
                if (logicalS) {
                    // Shifter carry-out (interpreter decodeShift) -> ESI bit 0,
                    // from the PRE-shift value: LSL #k: bit 32-k; LSR/ASR/ROR #k:
                    // bit k-1; LSR/ASR #0 (= #32): bit 31; ROR #0 (RRX): bit 0
                    a.mov(x86::esi, x86::ecx);
                    const uint32_t cbit = (sh == 0u)
                        ? ((stype == 3u) ? 0u : 31u)
                        : ((stype == 0u) ? (32u - sh) : (sh - 1u));
                    if (cbit) a.shr(x86::esi, cbit);
                    a.and_(x86::esi, 1u);
                    shiftCarryInEsi = true;
                }
                if (sh == 0u) {
                    // Special #0 forms (LSL #0 is encoded as shf==0, handled above):
                    //   LSR #0 = LSR #32 -> 0 ; ASR #0 = ASR #32 -> sign ; ROR #0 = RRX.
                    switch (stype) {
                    case 1:  a.xor_(x86::ecx, x86::ecx); break; // LSR #32 -> 0
                    case 2:  a.sar(x86::ecx, 31);        break; // ASR #32 -> sign fill
                    default: // ROR #0 = RRX: ECX = (C << 31) | (Rm >> 1)
                        a.shr(x86::ecx, 1);
                        a.mov(x86::eax, x86::dword_ptr(ctx, cpsr_off()));
                        a.and_(x86::eax, kC);      // isolate C (bit 29)
                        a.shl(x86::eax, 2);        // bit 29 -> bit 31
                        a.or_(x86::ecx, x86::eax);
                        break;
                    }
                } else {
                    switch (stype) {
                    case 0:  a.shl(x86::ecx, sh); break; // LSL
                    case 1:  a.shr(x86::ecx, sh); break; // LSR
                    case 2:  a.sar(x86::ecx, sh); break; // ASR
                    default: a.ror(x86::ecx, sh); break; // ROR
                    }
                }
            }
        } else {
            // Register-specified shift amount (insn bit 4 = 1, bit 7 = 0). The
            // amount is the bottom byte of Rs. x86 variable shifts need the count
            // in CL, so the value lives in EAX during the shift, then moves to ECX.
            // ARM treats amounts >= 32 specially (x86 would mask the count to 31)
            if (shf & 0x8u) return false;             // bit 7 = 1 -> not data-processing (mul-class)
            if (logicalS)   return false;             // run-time shift amount + logical S (run-time carry) -> defer
            const uint32_t Rs    = (shf >> 4) & 0xFu; // insn bits 11-8
            const uint32_t stype = (shf >> 1) & 0x3u; // insn bits 6-5
            if (Rs == 15u) return false;
            // Rm == R15 reads the pipelined PC: +12 with a register-specified
            // shift amount (interpreter decodeShift)
            if (Rm == 15u) a.mov(x86::eax, pc + 12u);
            else           a.mov(x86::eax, x86::dword_ptr(ctx, reg_off((int)Rm)));
            a.mov(x86::ecx, x86::dword_ptr(ctx, reg_off((int)Rs)));
            a.and_(x86::ecx, 0xFFu);                  // shift amount (CL = low byte)
            if (stype == 3u) {
                a.ror(x86::eax, x86::cl);             // ROR: x86 CL-masking matches ARM
            } else {
                Label lt32 = a.new_label();
                Label done = a.new_label();
                a.cmp(x86::ecx, 32);
                a.j(x86::CondCode::kCarry, lt32);            // amount < 32 (unsigned below)
                if (stype == 2u) a.sar(x86::eax, 31);        // ASR by >=32 -> sign fill
                else             a.xor_(x86::eax, x86::eax); // LSL/LSR by >=32 -> 0
                a.jmp(done);
                a.bind(lt32);
                if (stype == 0u)      a.shl(x86::eax, x86::cl); // LSL
                else if (stype == 1u) a.shr(x86::eax, x86::cl); // LSR
                else                  a.sar(x86::eax, x86::cl); // ASR
                a.bind(done);
            }
            a.mov(x86::ecx, x86::eax); // operand-2 value -> ECX
        }
    } else {
        const uint32_t rot = ((insn >> 8) & 0xFu) * 2u;
        const uint32_t v   = insn & 0xFFu;
        imm = rot ? ((v >> rot) | (v << (32u - rot))) : v;
        immCarryDefined = (rot != 0u); // C from operand 2 defined only if rotated
        immCarry        = ((imm >> 31) & 1u) != 0u;
    }

    const x86::Mem rd = x86::dword_ptr(ctx, reg_off((int)Rd));

    // Flag-setting (S=1) with an immediate operand 2: logical ops (N/Z from
    // result, C from the immediate shifter carry, V preserved), the logical
    // compares TST/TEQ (no write-back), the arithmetic ops ADD/SUB/RSB, and the
    // arithmetic compares CMP/CMN (N/Z/C/V via emit_set_flags_arith). ADC/SBC/RSC
    // (carry-in) and flags with a shifted-register operand 2 are later sub-steps
    if (S) {
        const x86::Mem rn = x86::dword_ptr(ctx, reg_off((int)Rn));
        if (op2reg) {
            // Operand 2 (possibly shifted) is already in ECX. Arithmetic ops just
            // feed it to the ALU (the shifter carry is irrelevant). Logical ops
            // need the shifter carry-out for C, which we only handle for the
            // no-shift case (C preserved); shifted-register logical flags -> later
            switch (opcode) {
            case 0xDu: case 0xFu: case 0x0u: case 0x1u:
            case 0x8u: case 0x9u: case 0xCu: case 0xEu:
                // With a shift, C comes from the shifter carry-out computed into
                // ESI at materialization (immediate amounts only; register-
                // specified amounts were already deferred there)
                if (((insn >> 4) & 0xFFu) != 0u && !shiftCarryInEsi) return false;
                switch (opcode) {
                case 0xDu: a.mov(x86::eax, x86::ecx); break;                                  // MOV
                case 0xFu: a.mov(x86::eax, x86::ecx); a.not_(x86::eax); break;                // MVN
                case 0x0u: case 0x8u: a.mov(x86::eax, rn); a.and_(x86::eax, x86::ecx); break; // AND / TST
                case 0x1u: case 0x9u: a.mov(x86::eax, rn); a.xor_(x86::eax, x86::ecx); break; // EOR / TEQ
                case 0xCu: a.mov(x86::eax, rn); a.or_(x86::eax, x86::ecx); break;             // ORR
                default:   a.mov(x86::eax, x86::ecx); a.not_(x86::eax); a.and_(x86::eax, rn); break; // BIC: Rn & ~op2
                }
                if (opcode != 0x8u && opcode != 0x9u) a.mov(rd, x86::eax); // TST/TEQ: no write-back
                if (shiftCarryInEsi) emit_set_flags_logical_carry_esi(a, ctx); // C = shifter carry-out
                else                 emit_set_flags_logical(a, ctx, false, false); // C preserved (no shift)
                return true;
            // arithmetic: op2 (shifted) is in ECX; the flag helpers capture the HOST
            // flags of the add/sub/adc/sbb itself (only the flag-transparent Rd MOV
            // may sit in between), so no operand staging is needed
            case 0x4u: a.mov(x86::eax, rn); a.add(x86::eax, x86::ecx); a.mov(rd, x86::eax); emit_set_flags_arith(a, ctx, false); return true; // ADD
            case 0x2u: a.mov(x86::eax, rn); a.sub(x86::eax, x86::ecx); a.mov(rd, x86::eax); emit_set_flags_arith(a, ctx, true);  return true; // SUB
            case 0xAu: a.mov(x86::eax, rn); a.sub(x86::eax, x86::ecx); emit_set_flags_arith(a, ctx, true);  return true; // CMP
            case 0xBu: a.mov(x86::eax, rn); a.add(x86::eax, x86::ecx); emit_set_flags_arith(a, ctx, false); return true; // CMN
            case 0x3u: a.mov(x86::eax, x86::ecx); a.sub(x86::eax, rn); a.mov(rd, x86::eax); emit_set_flags_arith(a, ctx, true); return true; // RSB: op2 - Rn
            case 0x5u: a.mov(x86::eax, rn); a.mov(x86::edx, x86::dword_ptr(ctx, cpsr_off())); a.bt(x86::edx, 29);          a.adc(x86::eax, x86::ecx); a.mov(rd, x86::eax); emit_set_flags_after_carry(a, ctx, false); return true; // ADC
            case 0x6u: a.mov(x86::eax, rn); a.mov(x86::edx, x86::dword_ptr(ctx, cpsr_off())); a.bt(x86::edx, 29); a.cmc(); a.sbb(x86::eax, x86::ecx); a.mov(rd, x86::eax); emit_set_flags_after_carry(a, ctx, true);  return true; // SBC
            case 0x7u: a.mov(x86::eax, x86::ecx); a.mov(x86::edx, x86::dword_ptr(ctx, cpsr_off())); a.bt(x86::edx, 29); a.cmc(); a.sbb(x86::eax, rn); a.mov(rd, x86::eax); emit_set_flags_after_carry(a, ctx, true);  return true; // RSC: op2 - Rn
            default:   return false;
            }
        }
        switch (opcode) {
        // logical, write-back
        case 0xDu: a.mov(x86::eax, imm);                        a.mov(rd, x86::eax); emit_set_flags_logical(a, ctx, immCarryDefined, immCarry); return true; // MOV
        case 0xFu: a.mov(x86::eax, ~imm);                       a.mov(rd, x86::eax); emit_set_flags_logical(a, ctx, immCarryDefined, immCarry); return true; // MVN
        case 0x0u: a.mov(x86::eax, rn); a.and_(x86::eax, imm);  a.mov(rd, x86::eax); emit_set_flags_logical(a, ctx, immCarryDefined, immCarry); return true; // AND
        case 0x1u: a.mov(x86::eax, rn); a.xor_(x86::eax, imm);  a.mov(rd, x86::eax); emit_set_flags_logical(a, ctx, immCarryDefined, immCarry); return true; // EOR
        case 0xCu: a.mov(x86::eax, rn); a.or_ (x86::eax, imm);  a.mov(rd, x86::eax); emit_set_flags_logical(a, ctx, immCarryDefined, immCarry); return true; // ORR
        case 0xEu: a.mov(x86::eax, rn); a.and_(x86::eax, ~imm); a.mov(rd, x86::eax); emit_set_flags_logical(a, ctx, immCarryDefined, immCarry); return true; // BIC
        // logical compares, no write-back
        case 0x8u: a.mov(x86::eax, rn); a.and_(x86::eax, imm);  emit_set_flags_logical(a, ctx, immCarryDefined, immCarry); return true; // TST
        case 0x9u: a.mov(x86::eax, rn); a.xor_(x86::eax, imm);  emit_set_flags_logical(a, ctx, immCarryDefined, immCarry); return true; // TEQ
        // arithmetic, write-back: the flag helpers capture the HOST flags of the
        // add/sub itself (only the flag-transparent Rd MOV in between)
        case 0x4u: a.mov(x86::eax, rn);  a.add(x86::eax, imm); a.mov(rd, x86::eax); emit_set_flags_arith(a, ctx, false); return true; // ADD
        case 0x2u: a.mov(x86::eax, rn);  a.sub(x86::eax, imm); a.mov(rd, x86::eax); emit_set_flags_arith(a, ctx, true);  return true; // SUB
        case 0x3u: a.mov(x86::eax, imm); a.sub(x86::eax, rn);  a.mov(rd, x86::eax); emit_set_flags_arith(a, ctx, true);  return true; // RSB: imm - Rn
        // arithmetic compares, no write-back
        case 0xAu: a.mov(x86::eax, rn);  a.sub(x86::eax, imm); emit_set_flags_arith(a, ctx, true);  return true; // CMP
        case 0xBu: a.mov(x86::eax, rn);  a.add(x86::eax, imm); emit_set_flags_arith(a, ctx, false); return true; // CMN
        // arithmetic with carry-in, write-back (host CF carries the ARM C flag)
        case 0x5u: // ADC: Rd = Rn + imm + C
            a.mov(x86::eax, rn); a.mov(x86::edx, x86::dword_ptr(ctx, cpsr_off())); a.bt(x86::edx, 29);
            a.adc(x86::eax, imm); a.mov(rd, x86::eax); emit_set_flags_after_carry(a, ctx, false); return true;
        case 0x6u: // SBC: Rd = Rn - imm - !C  (x86 sbb subtracts CF, so set CF = !C)
            a.mov(x86::eax, rn); a.mov(x86::edx, x86::dword_ptr(ctx, cpsr_off())); a.bt(x86::edx, 29); a.cmc();
            a.sbb(x86::eax, imm); a.mov(rd, x86::eax); emit_set_flags_after_carry(a, ctx, true); return true;
        case 0x7u: // RSC: Rd = imm - Rn - !C
            a.mov(x86::eax, imm); a.mov(x86::edx, x86::dword_ptr(ctx, cpsr_off())); a.bt(x86::edx, 29); a.cmc();
            a.sbb(x86::eax, rn); a.mov(rd, x86::eax); emit_set_flags_after_carry(a, ctx, true); return true;
        default:   return false;  // (all DP opcodes with immediate operand 2 now covered)
        }
    }

    switch (opcode) {
    case 0xDu: // MOV  Rd = op2
        if (op2reg) a.mov(rd, x86::ecx); else a.mov(rd, imm);
        return true;
    case 0xFu: // MVN  Rd = ~op2
        if (op2reg) { a.not_(x86::ecx); a.mov(rd, x86::ecx); } else a.mov(rd, ~imm);
        return true;
    case 0x3u: // RSB  Rd = op2 - Rn
        if (op2reg) { a.sub(x86::ecx, x86::dword_ptr(ctx, reg_off((int)Rn))); a.mov(rd, x86::ecx); }
        else { a.mov(x86::eax, imm); a.sub(x86::eax, x86::dword_ptr(ctx, reg_off((int)Rn))); a.mov(rd, x86::eax); }
        return true;
    default: break;
    }

    // Remaining ops compute  Rd = Rn <op> op2
    a.mov(x86::eax, x86::dword_ptr(ctx, reg_off((int)Rn)));
    switch (opcode) {
    case 0x4u: if (op2reg) a.add (x86::eax, x86::ecx); else a.add (x86::eax, imm); break; // ADD
    case 0x2u: if (op2reg) a.sub (x86::eax, x86::ecx); else a.sub (x86::eax, imm); break; // SUB
    case 0x0u: if (op2reg) a.and_(x86::eax, x86::ecx); else a.and_(x86::eax, imm); break; // AND
    case 0xCu: if (op2reg) a.or_ (x86::eax, x86::ecx); else a.or_ (x86::eax, imm); break; // ORR
    case 0x1u: if (op2reg) a.xor_(x86::eax, x86::ecx); else a.xor_(x86::eax, imm); break; // EOR
    case 0xEu: // BIC  Rd = Rn & ~op2
        if (op2reg) { a.not_(x86::ecx); a.and_(x86::eax, x86::ecx); } else a.and_(x86::eax, ~imm);
        break;
    case 0x5u: // ADC  Rd = Rn + op2 + C
        a.mov(x86::edx, x86::dword_ptr(ctx, cpsr_off())); a.bt(x86::edx, 29);
        if (op2reg) a.adc(x86::eax, x86::ecx); else a.adc(x86::eax, imm); break;
    case 0x6u: // SBC  Rd = Rn - op2 - !C  (set host CF = !C, then sbb)
        a.mov(x86::edx, x86::dword_ptr(ctx, cpsr_off())); a.bt(x86::edx, 29); a.cmc();
        if (op2reg) a.sbb(x86::eax, x86::ecx); else a.sbb(x86::eax, imm); break;
    case 0x7u: // RSC  Rd = op2 - Rn - !C
        a.mov(x86::edx, x86::dword_ptr(ctx, cpsr_off())); a.bt(x86::edx, 29); a.cmc();
        if (op2reg) { a.sbb(x86::ecx, x86::eax); a.mov(x86::eax, x86::ecx); }
        else { a.mov(x86::esi, imm); a.sbb(x86::esi, x86::eax); a.mov(x86::eax, x86::esi); }
        break;
    default: return false; // opcodes 8-B with S=0 are MRS/MSR/BX etc., not DP -> fall back
    }
    a.mov(rd, x86::eax);
    return true;
}

// Single data transfer: LDR/STR/LDRB/STRB (insn bits 27-26 == 01). Immediate or
// register (optionally shifted) offset, word/byte, pre/post-index, writeback;
// literal-pool PC base; LDR into PC as control flow. The same address arithmetic
// drives all three memory modes (see s_memMode): Callback emits real read/write
// calls (the live path), Mock hits the ctx 'mem' buffer (self-tests), Defer ends
// the block for the interpreter
bool emit_arm_ldrstr(x86::Assembler& a, const x86::Gp& ctx, uint32_t insn, uint32_t pc, FuncFrame& frame, int cyc)
{
    if (s_memMode == MemMode::Defer) return false;  // no callbacks: end block, interpreter does memory
    const bool     I  = (insn >> 25) & 1u;  // 0 = immediate offset (NOTE: inverted vs DP), 1 = register
    const bool     P  = (insn >> 24) & 1u;  // pre/post index
    const bool     U  = (insn >> 23) & 1u;  // add/subtract offset
    const bool     B  = (insn >> 22) & 1u;  // byte/word
    const bool     W  = (insn >> 21) & 1u;  // writeback (pre-index)
    const bool     L  = (insn >> 20) & 1u;  // load/store
    const uint32_t Rn = (insn >> 16) & 0xFu;
    const uint32_t Rd = (insn >> 12) & 0xFu;
    // PC operands: a PC base reads as (instr_pc + 8); STR of PC stores (instr_pc + 12).
    // A load INTO PC, and a PC-base writeback, both write PC = control flow: those end
    // the block (handled below via 'branchToPc'). Both PC operands at once is too rare
    if (Rd == 15u && Rn == 15u) return false;

    // Offset: an immediate ('off'), or register Rm with an optional immediate
    // shift, materialized into ESI ('offIsReg'). LDR/STR offsets never use a
    // register-specified shift amount, so bit 4 must be 0
    bool     offIsReg = false;
    uint32_t off      = 0;
    if (!I) {
        off = insn & 0xFFFu;                   // imm12
    } else {
        const uint32_t Rm = insn & 0xFu;
        if (Rm == 15u) return false;
        const uint32_t shf = (insn >> 4) & 0xFFu;
        if (shf & 1u) return false;            // register-specified shift not valid here
        a.mov(x86::esi, x86::dword_ptr(ctx, reg_off((int)Rm)));
        if (shf != 0u) {
            const uint32_t stype = (shf >> 1) & 0x3u;
            const uint32_t sh    = (shf >> 3) & 0x1Fu;
            if (sh == 0u) {
                // #0 specials (LSL #0 is shf==0, handled above): LSR/ASR #32, ROR=RRX.
                switch (stype) {
                case 1:  a.xor_(x86::esi, x86::esi); break; // LSR #32 -> 0
                case 2:  a.sar(x86::esi, 31);        break; // ASR #32 -> sign
                default: // ROR #0 = RRX: (C << 31) | (Rm >> 1)  (EAX is free here)
                    a.shr(x86::esi, 1);
                    a.mov(x86::eax, x86::dword_ptr(ctx, cpsr_off()));
                    a.and_(x86::eax, kC); a.shl(x86::eax, 2); a.or_(x86::esi, x86::eax);
                    break;
                }
            } else {
                switch (stype) {
                case 0:  a.shl(x86::esi, sh); break; // LSL
                case 1:  a.shr(x86::esi, sh); break; // LSR
                case 2:  a.sar(x86::esi, sh); break; // ASR
                default: a.ror(x86::esi, sh); break; // ROR
                }
            }
        }
        offIsReg = true;
    }

    // --- Callback mode: emit calls to the emulator's real read/write ---------
    // All values needed across the call (writeback to Rn, the effective address,
    // the store value) are computed/persisted before the call; nothing lives in a
    // volatile host register across it (ARM state stays in the ctx memory; ctx is
    // a callee-saved reg). The running address is re-derived from the base each
    // time, so no temp must survive a call
    if (s_memMode == MemMode::Callback) {
        const bool pcBase = (Rn == 15u);
        if (pcBase && (!P || W)) return false;          // PC-base writeback = control flow -> defer
        if (pcBase) a.mov(x86::eax, pc + 8u);           // EAX = base (PC reads instr+8)
        else        a.mov(x86::eax, x86::dword_ptr(ctx, reg_off((int)Rn)));
        a.mov(x86::edx, x86::eax);                      // EDX = effective address
        if (P) {                                        // pre-index
            if (offIsReg)       { if (U) a.add(x86::edx, x86::esi); else a.sub(x86::edx, x86::esi); }
            else if (off != 0u) { if (U) a.add(x86::edx, off);      else a.sub(x86::edx, off); }
        }
        if (!P) {                                       // post-index writeback: Rn = base +/- off
            a.mov(x86::ecx, x86::eax);
            if (offIsReg) { if (U) a.add(x86::ecx, x86::esi); else a.sub(x86::ecx, x86::esi); }
            else          { if (U) a.add(x86::ecx, off);      else a.sub(x86::ecx, off); }
            a.mov(x86::dword_ptr(ctx, reg_off((int)Rn)), x86::ecx);
        } else if (W) {                                 // pre-index writeback: Rn = effective address
            a.mov(x86::dword_ptr(ctx, reg_off((int)Rn)), x86::edx);
        }
        if (L) {                                        // load: Rd = read(addr)
            a.mov(x86::ecx, x86::edx);                  // arg0 = address
            emit_mem_call(a, B ? s_memcb.r8 : s_memcb.r32, false);  // -> EAX
            if (Rd == 15u) {                            // LDR into PC = return
                a.and_(x86::eax, ~3u);
                emit_charge_cycles(a, cyc, a.zdx());
                a.emit_epilog(frame);
            }
            else           a.mov(x86::dword_ptr(ctx, reg_off((int)Rd)), x86::eax);
        } else {                                        // store: write(addr, value)
            a.mov(x86::ecx, x86::edx);                  // arg0 = address
            if (Rd == 15u) a.mov(x86::edx, pc + 12u);   // STR of PC stores instr+12
            else           a.mov(x86::edx, x86::dword_ptr(ctx, reg_off((int)Rd))); // arg1 = value
            emit_mem_call(a, B ? s_memcb.w8 : s_memcb.w32, true);
        }
        // LDR into PC exited above WITHOUT the post-check or the irq flag, so the
        // exec loop's flag-gated ARM7_CHECKIRQ doesn't run for it either -- a small,
        // deliberate divergence from the interpreter (which checks after EVERY single
        // data transfer): an exception raised synchronously by that load's own access
        // (a PC load from a peripheral register -- no known code) stays pending until
        // the next post-checked LDR/STR. Externally raised IRQs are unaffected
        // (arm7_core_set_irq_line runs its own check). Everything else checks here
        if (!(L && Rd == 15u)) emit_mem_post_checks(a, ctx, pc, frame, cyc);
        return true;
    }

    const int      memOff = (int)offsetof(ArmCpuSelfTest, mem);
    const x86::Mem rnm    = x86::dword_ptr(ctx, reg_off((int)Rn));

    if (Rn == 15u) a.mov(x86::ecx, pc + 8u); // PC base reads as instr_pc + 8
    else           a.mov(x86::ecx, rnm);
    a.mov(x86::edx, x86::ecx);               // EDX = effective access address (zero-extends ZDX)
    if (P) {                                 // pre-index: address = base +/- offset
        if (offIsReg)       { if (U) a.add(x86::edx, x86::esi); else a.sub(x86::edx, x86::esi); }
        else if (off != 0u) { if (U) a.add(x86::edx, off);      else a.sub(x86::edx, off); }
    }
    // mock-memory operand: [ctx + address + memOff]; ZDX is the arch-width index.
    const x86::Mem mw = x86::ptr(ctx, a.zdx(), 0, memOff, 4); // word
    const x86::Mem mb = x86::ptr(ctx, a.zdx(), 0, memOff, 1); // byte
    bool branchToPc = false;
    if (L) {                              // load
        if (B) a.movzx(x86::eax, mb); else a.mov(x86::eax, mw);
        if (Rd == 15u) branchToPc = true; // LDR into PC: EAX = loaded value = new PC
        else           a.mov(x86::dword_ptr(ctx, reg_off((int)Rd)), x86::eax);
    } else {                              // store
        if (Rd == 15u) a.mov(x86::eax, pc + 12u); // STR of PC stores instr_pc + 12
        else           a.mov(x86::eax, x86::dword_ptr(ctx, reg_off((int)Rd)));
        if (B) a.mov(mb, x86::al); else a.mov(mw, x86::eax);
    }
    // writeback
    if (!P) {                          // post-index: Rn = base +/- offset (always)
        if (offIsReg) { if (U) a.add(x86::ecx, x86::esi); else a.sub(x86::ecx, x86::esi); }
        else          { if (U) a.add(x86::ecx, off);      else a.sub(x86::ecx, off); }
        if (Rn == 15u) { a.mov(x86::eax, x86::ecx); branchToPc = true; } // PC-base writeback = branch
        else             a.mov(rnm, x86::ecx);
    } else if (W) {                    // pre-index with writeback: Rn = computed address
        if (Rn == 15u) { a.mov(x86::eax, x86::edx); branchToPc = true; }
        else             a.mov(rnm, x86::edx);
    }
    if (branchToPc) {                  // control flow: EAX = new PC -> return from the block
        a.and_(x86::eax, ~3u);         // ARMv4 LDR-to-PC ignores the low two bits
        emit_charge_cycles(a, cyc, a.zdx());
        a.emit_epilog(frame);
    }
    return true;
}

// Halfword & signed data transfer: LDRH/STRH/LDRSB/LDRSH. Shares the DP class
// (bits 27-26 == 00) but is marked by bit7 = bit4 = 1 with SH (bits 6-5) != 0 and
// bit25 == 0. Immediate (split-nibble) or register offset; pre/post-index;
// writeback; a PC base reads instr+8. Transfer to/from PC and PC-base writeback
// are not valid here -> deferred
bool emit_arm_halfword(x86::Assembler& a, const x86::Gp& ctx, uint32_t insn, uint32_t pc)
{
    if (s_memMode == MemMode::Defer) return false; // no callbacks: end block, interpreter does memory
    const bool     P   = (insn >> 24) & 1u;
    const bool     U   = (insn >> 23) & 1u;
    const bool     Imm = (insn >> 22) & 1u;   // 1 = immediate (split) offset, 0 = register
    const bool     W   = (insn >> 21) & 1u;
    const bool     L   = (insn >> 20) & 1u;
    const uint32_t Rn  = (insn >> 16) & 0xFu;
    const uint32_t Rd  = (insn >> 12) & 0xFu;
    const uint32_t sh  = (insn >> 5)  & 0x3u; // 1=H (unsigned half), 2=SB (signed byte), 3=SH (signed half)
    if (Rd == 15u)              return false; // transfer to/from PC here is undefined on ARM7
    if (!L && sh != 1u)         return false; // only STRH stores; SH=10/11 store = LDRD/STRD (ARMv5E+)
    if (Rn == 15u && (W || !P)) return false; // PC-base writeback = control flow -> defer

    bool     offIsReg = false;
    uint32_t off      = 0;
    if (Imm) {
        off = (((insn >> 8) & 0xFu) << 4) | (insn & 0xFu);      // split 8-bit immediate
    } else {
        const uint32_t Rm = insn & 0xFu;
        if (Rm == 15u) return false;
        a.mov(x86::esi, x86::dword_ptr(ctx, reg_off((int)Rm))); // register offset (no shift)
        offIsReg = true;
    }

    // --- Callback mode: emit calls to the emulator's real read/write ---------
    // Same call-safety discipline as emit_arm_ldrstr (writeback persisted before
    // the call; nothing live in a volatile reg across it). Rd==15 and PC-base
    // writeback were already deferred above, so no control-flow exit here
    if (s_memMode == MemMode::Callback) {
        const bool pcBase = (Rn == 15u);                // here implies P && !W (base read only)
        if (pcBase) a.mov(x86::eax, pc + 8u);           // EAX = base
        else        a.mov(x86::eax, x86::dword_ptr(ctx, reg_off((int)Rn)));
        a.mov(x86::edx, x86::eax);                      // EDX = effective address
        if (P) {                                        // pre-index
            if (offIsReg)       { if (U) a.add(x86::edx, x86::esi); else a.sub(x86::edx, x86::esi); }
            else if (off != 0u) { if (U) a.add(x86::edx, off);      else a.sub(x86::edx, off); }
        }
        if (!P) {                                       // post-index writeback: Rn = base +/- off
            a.mov(x86::ecx, x86::eax);
            if (offIsReg) { if (U) a.add(x86::ecx, x86::esi); else a.sub(x86::ecx, x86::esi); }
            else          { if (U) a.add(x86::ecx, off);      else a.sub(x86::ecx, off); }
            a.mov(x86::dword_ptr(ctx, reg_off((int)Rn)), x86::ecx);
        } else if (W) {                                 // pre-index writeback: Rn = effective address
            a.mov(x86::dword_ptr(ctx, reg_off((int)Rn)), x86::edx);
        }
        if (L) {                                        // load
            a.mov(x86::ecx, x86::edx);                  // arg0 = address
            emit_mem_call(a, (sh == 2u) ? s_memcb.r8 : s_memcb.r16, false); // SB reads a byte, H/SH a half
            if      (sh == 1u) {}                       // LDRH : value already zero-extended in EAX
            else if (sh == 2u) a.movsx(x86::eax, x86::al); // LDRSB: sign-extend byte
            else               a.movsx(x86::eax, x86::ax); // LDRSH: sign-extend half
            a.mov(x86::dword_ptr(ctx, reg_off((int)Rd)), x86::eax);
        } else {                                        // STRH (low 16 bits of Rd)
            a.mov(x86::ecx, x86::edx);                  // arg0 = address
            a.mov(x86::edx, x86::dword_ptr(ctx, reg_off((int)Rd))); // arg1 = value (w16 truncates)
            emit_mem_call(a, s_memcb.w16, true);
        }
        // NB: no abort/IRQ post-check here -- the interpreter checks IRQ only after a
        // SINGLE data transfer (LDR/STR), not after halfword/DP-class ops. See the
        // matching note in emit_arm_blocktransfer
        return true;
    }

    const int      memOff = (int)offsetof(ArmCpuSelfTest, mem);
    const x86::Mem rnm    = x86::dword_ptr(ctx, reg_off((int)Rn));
    if (Rn == 15u) a.mov(x86::ecx, pc + 8u); else a.mov(x86::ecx, rnm);
    a.mov(x86::edx, x86::ecx);
    if (P) {
        if (offIsReg)       { if (U) a.add(x86::edx, x86::esi); else a.sub(x86::edx, x86::esi); }
        else if (off != 0u) { if (U) a.add(x86::edx, off);      else a.sub(x86::edx, off); }
    }
    const x86::Mem mh = x86::ptr(ctx, a.zdx(), 0, memOff, 2); // 16-bit
    const x86::Mem mb = x86::ptr(ctx, a.zdx(), 0, memOff, 1); // 8-bit
    if (L) {                                   // load
        if      (sh == 1u) a.movzx(x86::eax, mh); // LDRH  : zero-extend 16
        else if (sh == 2u) a.movsx(x86::eax, mb); // LDRSB : sign-extend 8
        else               a.movsx(x86::eax, mh); // LDRSH : sign-extend 16
        a.mov(x86::dword_ptr(ctx, reg_off((int)Rd)), x86::eax);
    } else {                                   // STRH (low 16 bits of Rd)
        a.mov(x86::eax, x86::dword_ptr(ctx, reg_off((int)Rd)));
        a.mov(mh, x86::ax);
    }
    if (!P) {                                  // post-index writeback (Rn != 15 here)
        if (offIsReg) { if (U) a.add(x86::ecx, x86::esi); else a.sub(x86::ecx, x86::esi); }
        else          { if (U) a.add(x86::ecx, off);      else a.sub(x86::ecx, off); }
        a.mov(rnm, x86::ecx);
    } else if (W) {
        a.mov(rnm, x86::edx);
    }
    return true;
}

// Block data transfer LDM/STM (insn bits 27-25 == 100). Registers transfer in
// ascending register order with the lowest register at the lowest address; the
// four modes (IA/IB/DA/DB) only change the start address and the writeback value,
// all computable at translate time (the register count is constant). LDM with PC
// in the list ends the block (return). S=1 (user-bank / SPSR restore), a PC base,
// and the base register appearing in the list are deferred. Memory access follows
// s_memMode (Callback = real calls / Mock = ctx buffer / Defer = interpreter)
bool emit_arm_blocktransfer(x86::Assembler& a, const x86::Gp& ctx, uint32_t insn, uint32_t pc, const FuncFrame& frame, int cyc)
{
    if (s_memMode == MemMode::Defer) return false; // no callbacks: end block, interpreter does memory
    const bool     P    = (insn >> 24) & 1u;
    const bool     U    = (insn >> 23) & 1u;
    const bool     S    = (insn >> 22) & 1u;
    const bool     W    = (insn >> 21) & 1u;
    const bool     L    = (insn >> 20) & 1u;
    const uint32_t Rn   = (insn >> 16) & 0xFu;
    const uint32_t list = insn & 0xFFFFu;
    if (S)         return false; // user-mode banks / SPSR restore -> defer
    if (Rn == 15u) return false; // PC base -> defer
    int count = 0;
    for (int i = 0; i < 16; ++i) if (list & (1u << i)) ++count;
    if (count == 0) return false; // empty list is unpredictable -> defer

    const int32_t bytes    = 4 * count;
    const int32_t firstOff = U ? (P ? 4 : 0) : (P ? -bytes : -bytes + 4); // IB/IA : DB/DA
    const int32_t finalOff = U ? bytes : -bytes;

    // --- Callback mode: emit a call per transferred register -----------------
    // Each transfer address is base + a translate-time constant, so we re-read the
    // (unchanged) base from ctx[Rn] each iteration -> no running address need
    // survive a call. Writeback is applied AFTER the loop (so the re-reads see the
    // original base). The base register in the list is the one case that would
    // break that invariant (and is UNPREDICTABLE with writeback on ARM), so defer
    // it to the interpreter
    if (s_memMode == MemMode::Callback) {
        if (list & (1u << Rn)) return false; // base in transfer list -> defer
        bool branchToPc = false;
        int  pos = 0;
        for (int i = 0; i < 16; ++i) {
            if (!(list & (1u << i))) continue;
            const int32_t addrOff = firstOff + 4 * pos;
            a.mov(x86::ecx, x86::dword_ptr(ctx, reg_off((int)Rn))); // arg0 = base (re-read, unchanged)
            if (addrOff) a.add(x86::ecx, addrOff);                  //        + constant offset
            if (L) {
                emit_mem_call(a, s_memcb.r32, false);               // -> EAX
                if (i == 15) { a.mov(x86::edx, x86::eax); branchToPc = true; } // save new PC (PC is last)
                else         a.mov(x86::dword_ptr(ctx, reg_off(i)), x86::eax);
            } else {
                if (i == 15) a.mov(x86::edx, pc + 12u);             // STM stores PC as instr+12
                else         a.mov(x86::edx, x86::dword_ptr(ctx, reg_off(i))); // arg1 = value
                emit_mem_call(a, s_memcb.w32, true);
            }
            ++pos;
        }
        if (W) {                                    // writeback: Rn = base +/- 4*count
            a.mov(x86::eax, x86::dword_ptr(ctx, reg_off((int)Rn)));
            a.add(x86::eax, finalOff);
            a.mov(x86::dword_ptr(ctx, reg_off((int)Rn)), x86::eax);
        }
        if (branchToPc) {                           // LDM ...,PC = return (after writeback)
            a.mov(x86::eax, x86::edx);
            a.and_(x86::eax, ~3u);
            emit_charge_cycles(a, cyc, a.zdx());    // (EDX's PC already copied to EAX)
            a.emit_epilog(frame);
        }
        // NB: no abort/IRQ post-check here. The interpreter checks IRQ only after a
        // SINGLE data transfer (arm7exec.c case 4-7), NOT after block transfer
        // (case 8-9); a check here would vector interrupts earlier than the
        // interpreter does, so match its timing exactly
        return true;
    }

    const int      memOff = (int)offsetof(ArmCpuSelfTest, mem);
    const x86::Mem rnm    = x86::dword_ptr(ctx, reg_off((int)Rn));
    a.mov(x86::ecx, rnm);      // ECX = base (kept for writeback)
    a.mov(x86::edx, x86::ecx); // EDX = running transfer address
    if (firstOff) a.add(x86::edx, firstOff);

    bool branchToPc = false;
    for (int i = 0; i < 16; ++i) {
        if (!(list & (1u << i))) continue;
        const x86::Mem m = x86::ptr(ctx, a.zdx(), 0, memOff, 4);
        if (L) {
            a.mov(x86::eax, m);
            if (i == 15) branchToPc = true;         // LDM ...,PC -> EAX = new PC
            else         a.mov(x86::dword_ptr(ctx, reg_off(i)), x86::eax);
        } else {
            if (i == 15) a.mov(x86::eax, pc + 12u); // STM stores PC as instr+12
            else         a.mov(x86::eax, x86::dword_ptr(ctx, reg_off(i)));
            a.mov(m, x86::eax);
        }
        a.add(x86::edx, 4);
    }
    if (W) { // writeback: Rn = base +/- 4*count
        a.add(x86::ecx, finalOff);
        a.mov(rnm, x86::ecx);
    }
    if (branchToPc) {
        a.and_(x86::eax, ~3u);
        emit_charge_cycles(a, cyc, a.zdx());
        a.emit_epilog(frame);
    }
    return true;
}

// Branch B / BL (insn bits 27-25 == 101). Target is a compile-time constant
// (pc + 8 + signext(imm24) << 2); BL also stores the return address (pc + 4) in
// LR. A branch ends the block: load the target into EAX (the next-PC return
// value) and emit the function epilog (return). Conditional branches are gated
// by the caller's emit_cond_skip, so this body runs only when the branch is taken
bool emit_arm_branch(x86::Assembler& a, const x86::Gp& ctx, uint32_t insn, uint32_t pc, const FuncFrame& frame, int cyc)
{
    const bool     L      = (insn >> 24) & 1u;               // link
    const int32_t  off    = (int32_t)(insn << 8) >> 6;       // sign-extended imm24, scaled *4
    const uint32_t target = pc + 8u + (uint32_t)off;
    if (L) a.mov(x86::dword_ptr(ctx, reg_off(14)), pc + 4u); // LR = address of the next instruction
    emit_charge_cycles(a, cyc, a.zdx());
    a.mov(x86::eax, target);
    a.emit_epilog(frame);                                    // return target as the next PC
    return true;
}

#endif // AJ_HOST_X86 (classifiers below are pure decode: shared)

// ---------------------------------------------------------------------------
// Instruction-class predicates for the 00-class sub-decodes (multiply, PSR
// transfer). These mirror the interpreter's dispatch (arm7exec.c case 0/1)
// EXACTLY, and are shared by emit_arm_insn / arm_insn_cycles /
// arm_insn_ends_block so the three can never disagree about an encoding
inline bool arm_is_bx(uint32_t insn) {      // BX: bits 27-4 = 000100101111111111110001
    return (insn & 0x0FFFFFF0u) == 0x012FFF10u;
}
inline bool arm_is_mul32(uint32_t insn) {   // MUL/MLA: bits 27-23 = 00000, bits 7-4 = 1001
    return (insn & 0x0F8000F0u) == 0x00000090u;
}
inline bool arm_is_mul64(uint32_t insn) {   // UMULL/SMULL/UMLAL/SMLAL: bits 27-23 = 00001, bits 7-4 = 1001
    return (insn & 0x0F8000F0u) == 0x00800090u;
}
inline bool arm_is_swap(uint32_t insn) {    // SWP/SWPB, canonical form only (non-canonical -> interpreter)
    return (insn & 0x0FB00FF0u) == 0x01000090u;
}
inline bool arm_is_psr(uint32_t insn) {     // MRS/MSR (interpreter test: S clear && bits 24-23 == 10)
    if (((insn >> 26) & 0x3u) != 0u) return false;                     // 00-class only
    // BX satisfies the S-clear/bits-24-23 test below; the interpreter only avoids
    // sending BX to HandlePSRTransfer by checking its pattern FIRST -- so must we
    if (arm_is_bx(insn)) return false;
    if (!((insn >> 25) & 1u) && (insn & 0x90u) == 0x90u) return false; // mul/swap/halfword group (I=0, bit7=bit4=1)
    return (insn & 0x01900000u) == 0x01000000u;
}

#if AJ_HOST_X86 // x86/x64 emitter backend, part 2

// Branch and exchange BX Rm: PC = Rm, EXACTLY like the interpreter (arm7exec.c
// case 1): the value is NOT masked, and if bit 0 is set the CPSR T bit is set
// (Thumb is not implemented by this core; this is parity, not a mode switch).
// Ends the block. Rm == 15 (BX PC -- nonsensical) is deferred
bool emit_arm_bx(x86::Assembler& a, const x86::Gp& ctx, uint32_t insn, const FuncFrame& frame, int cyc)
{
    const uint32_t Rm = insn & 0xFu;
    if (Rm == 15u) return false;
    a.mov(x86::eax, x86::dword_ptr(ctx, reg_off((int)Rm)));
    Label even = a.new_label();
    a.test(x86::eax, 1u);
    a.j(x86::CondCode::kZero, even);
    a.or_(x86::dword_ptr(ctx, cpsr_off()), 0x20u); // T bit (bit 5), interpreter parity
    a.bind(even);
    emit_charge_cycles(a, cyc, a.zdx());
    a.emit_epilog(frame);                          // return Rm as the next PC (unmasked)
    return true;
}

// Single data swap SWP/SWPB: tmp = read(Rn); write(Rn, Rm); Rd = tmp -- an LDR
// followed by an STR, per the interpreter (HandleSwap). No abort/IRQ post-check:
// the interpreter runs none after HandleSwap (arm7exec.c case 1). Callback mode
// only (two memory calls). R15 anywhere is deferred; so is Rd aliasing Rn/Rm:
// nothing survives a call in a volatile register, so the loaded value is parked
// in the Rd slot BETWEEN the two calls -- earlier than the interpreter writes it,
// which is only observable through such aliasing (UNPREDICTABLE on hardware)
bool emit_arm_swap(x86::Assembler& a, const x86::Gp& ctx, uint32_t insn)
{
    if (s_memMode != MemMode::Callback) return false;
    const uint32_t Rn = (insn >> 16) & 0xFu;
    const uint32_t Rd = (insn >> 12) & 0xFu;
    const uint32_t Rm =  insn        & 0xFu;
    const bool     B  = (insn >> 22) & 1u;
    if (Rn == 15u || Rd == 15u || Rm == 15u) return false;
    if (Rd == Rn || Rd == Rm) return false;
    a.mov(x86::ecx, x86::dword_ptr(ctx, reg_off((int)Rn))); // arg0 = address
    emit_mem_call(a, B ? s_memcb.r8 : s_memcb.r32, false);  // EAX = old value
    a.mov(x86::dword_ptr(ctx, reg_off((int)Rd)), x86::eax); // Rd = old value (parked)
    a.mov(x86::ecx, x86::dword_ptr(ctx, reg_off((int)Rn))); // arg0 = address (re-read, unchanged)
    a.mov(x86::edx, x86::dword_ptr(ctx, reg_off((int)Rm))); // arg1 = new value
    emit_mem_call(a, B ? s_memcb.w8 : s_memcb.w32, true);
    return true;
}

// Multiplies cost 1S + mI, where m = 1..4 by MULTIPLIER magnitude (byte count
// of |Rs|: <0x100 -> 1 ... else 4; long forms +1I; see arm7core.c HandleMul et
// al). The block's exit constant charges the worst case (m = 4, arm_insn_cycles);
// this emits the run-time correction "ARM7_ICOUNT += 4 - m" from the actual Rs
// value, making the charged total spec/interpreter-exact. 'sign': MUL/MLA and
// SMULL/SMLAL classify |Rs| unsigned (0x80000000 stays put under negation ->
// m = 4, matching HandleMul/HandleSMulLong); UMULL/UMLAL classify Rs raw.
// Emitted BEFORE the multiply's own code so an Rd == Rs overwrite cannot skew it.
// Clobbers ECX/EDX. No-op in uncharged mode (s_picount null)
void emit_mul_cycle_refund(x86::Assembler& a, const x86::Gp& ctx, uint32_t Rs, bool sign)
{
    if (!s_picount) return;
    a.mov(x86::ecx, x86::dword_ptr(ctx, reg_off((int)Rs)));
    if (sign) {
        a.mov(x86::edx, x86::ecx);
        a.sar(x86::edx, 31);
        a.xor_(x86::ecx, x86::edx);
        a.sub(x86::ecx, x86::edx);  // |Rs|
    }
    a.or_(x86::ecx, 1);             // Rs == 0: keep BSR defined -> m = 1
    a.bsr(x86::edx, x86::ecx);      // index of the top set bit
    a.shr(x86::edx, 3);             // byte index = m - 1 (0..3)
    a.xor_(x86::edx, 3);            // refund = 4 - m
    emit_mov_ptr(a, a.zcx(), s_picount);
    a.add(x86::dword_ptr(a.zcx()), x86::edx);
}

// Multiply MUL (Rd = Rm * Rs) / MLA (Rd = Rm * Rs + Rn). R15 in any operand is
// deferred (as in the legacy JIT). S=1 sets N/Z from the result and -- matching
// the INTERPRETER (HandleMul: N|Z cleared only), NOT the legacy JIT (which
// zeroed C) -- preserves C and V. The low 32 bits of the product are
// sign-agnostic, so one imul serves both signednesses. Works in all memory
// modes (no memory access). Cycle cost: worst case in arm_insn_cycles plus the
// data-dependent run-time refund above
bool emit_arm_mul32(x86::Assembler& a, const x86::Gp& ctx, uint32_t insn)
{
    const uint32_t Rd = (insn >> 16) & 0xFu; // NB: multiply encodings put Rd at 19-16
    const uint32_t Rn = (insn >> 12) & 0xFu;
    const uint32_t Rs = (insn >>  8) & 0xFu;
    const uint32_t Rm =  insn        & 0xFu;
    const bool     A  = (insn >> 21) & 1u;
    const bool     S  = (insn >> 20) & 1u;
    if (Rd == 15u || Rm == 15u || Rs == 15u || (A && Rn == 15u)) return false;
    emit_mul_cycle_refund(a, ctx, Rs, true); // MUL/MLA: m from |Rs| (HandleMul)
    a.mov(x86::eax, x86::dword_ptr(ctx, reg_off((int)Rm)));
    a.imul(x86::eax, x86::dword_ptr(ctx, reg_off((int)Rs)));
    if (A) a.add(x86::eax, x86::dword_ptr(ctx, reg_off((int)Rn)));
    a.mov(x86::dword_ptr(ctx, reg_off((int)Rd)), x86::eax);
    if (S) emit_set_flags_logical(a, ctx, false, false); // N/Z from EAX; C,V preserved
    return true;
}

// Long multiply UMULL/SMULL (RdHi:RdLo = Rm * Rs) / UMLAL/SMLAL (+= product).
// One-operand mul/imul leaves the 64-bit product in EDX:EAX. R15 anywhere is
// deferred. S=1: N = bit 63, Z = (64-bit result == 0); C,V preserved
// (interpreter parity, HandleSMulLong/HandleUMulLong). RdHi is stored before
// RdLo, matching the interpreter's write order for the unpredictable
// RdHi == RdLo case. Works in all memory modes (no memory access)
bool emit_arm_mul64(x86::Assembler& a, const x86::Gp& ctx, uint32_t insn)
{
    const uint32_t RdHi = (insn >> 16) & 0xFu;
    const uint32_t RdLo = (insn >> 12) & 0xFu;
    const uint32_t Rs   = (insn >>  8) & 0xFu;
    const uint32_t Rm   =  insn        & 0xFu;
    const bool   signd  = (insn >> 22) & 1u;
    const bool   A      = (insn >> 21) & 1u;
    const bool   S      = (insn >> 20) & 1u;
    if (RdHi == 15u || RdLo == 15u || Rs == 15u || Rm == 15u) return false;
    emit_mul_cycle_refund(a, ctx, Rs, signd); // m from Rs (|Rs| when signed), HandleS/UMulLong
    a.mov(x86::eax, x86::dword_ptr(ctx, reg_off((int)Rm)));
    if (signd) a.imul(x86::dword_ptr(ctx, reg_off((int)Rs))); // EDX:EAX = Rm * Rs (signed)
    else       a.mul (x86::dword_ptr(ctx, reg_off((int)Rs))); //                   (unsigned)
    if (A) {                                                  // UMLAL/SMLAL: RdHi:RdLo += product
        a.add(x86::eax, x86::dword_ptr(ctx, reg_off((int)RdLo)));
        a.adc(x86::edx, x86::dword_ptr(ctx, reg_off((int)RdHi)));
    }
    a.mov(x86::dword_ptr(ctx, reg_off((int)RdHi)), x86::edx); // hi first (see above)
    a.mov(x86::dword_ptr(ctx, reg_off((int)RdLo)), x86::eax);
    if (S) {
        const x86::Mem cpsr = x86::dword_ptr(ctx, cpsr_off());
        a.mov(x86::ecx, cpsr);
        a.and_(x86::ecx, ~(kN | kZ)); // C,V (and mode bits) preserved
        a.mov(x86::esi, x86::edx);
        a.and_(x86::esi, kN);         // N = bit 63 (bit 31 of the high word)
        a.or_(x86::ecx, x86::esi);
        a.or_(x86::eax, x86::edx);    // Z = (lo | hi) == 0 (results already stored)
        a.cmp(x86::eax, 1);           // branchless: CF = (64-bit result == 0)
        a.sbb(x86::eax, x86::eax);
        a.and_(x86::eax, kZ);
        a.or_(x86::ecx, x86::eax);
        a.mov(cpsr, x86::ecx);
    }
    return true;
}

// PSR transfer MRS/MSR: emit a call to the emulator's own HandlePSRTransfer
// (wired via arm7_aj_set_psr_transfer), passing the raw instruction word --
// EXACT interpreter semantics by construction, including SPSR banking, field
// masks, and mode switches (SwitchMode swaps the banked registers through the
// active sArmRegister[0..15] window, so the ctx offsets this JIT uses stay
// valid for the rest of the block). No IRQ check afterwards: the interpreter
// does not run ARM7_CHECKIRQ after a PSR transfer either (arm7exec.c case 0/1;
// an MSR that unmasks a pending IRQ vectors at the next check point, typically
// the LDR/STR post-check or the interpreted exception-return instruction).
// Needs Callback mode: the call requires the block's reserved call stack, and
// the helper address arrives with the other hooks
bool emit_arm_psr(x86::Assembler& a, const x86::Gp& ctx, uint32_t insn)
{
    (void)ctx;
    if (s_memMode != MemMode::Callback || !s_memcb.psr_transfer) return false;
    if (!((insn >> 21) & 1u) && ((insn >> 12) & 0xFu) == 15u)
        return false;  // MRS into PC: would be a control transfer -> defer (invalid anyway)
    a.mov(x86::ecx, insn);                         // arg0 = instruction word
    emit_mem_call(a, s_memcb.psr_transfer, false); // HandlePSRTransfer(insn)
    return true;
}

// Two-arg charge form for the SHARED router (per-arch scratch choice)
inline void emit_charge_cycles(x86::Assembler& a, int cyc) { emit_charge_cycles(a, cyc, a.zdx()); }

// Block exit for a DP instruction that wrote R15 (emit_arm_dp stored the
// flag-less result to the R15 slot). excReturn (S=1): MOVS PC / SUBS PC,LR --
// the wired helper restores CPSR from the SPSR, switches banks, masks bits
// 1:0, runs ARM7_CHECKIRQ, and returns the FINAL PC (the check may have vectored)
inline void emit_dp_pc_exit(x86::Assembler& a, const x86::Gp& ctx, bool excReturn, const FuncFrame& frame, int cycCharge)
{
    if (excReturn) {
        a.mov(x86::ecx, x86::dword_ptr(ctx, reg_off(15)));
        emit_mem_call(a, s_memcb.exc_return, false);
        emit_charge_cycles(a, cycCharge, a.zdx());
        a.emit_epilog(frame); // EAX = final PC from the helper
    } else {
        a.mov(x86::eax, x86::dword_ptr(ctx, reg_off(15)));
        a.and_(x86::eax, ~3u);
        emit_charge_cycles(a, cycCharge, a.zdx());
        a.emit_epilog(frame);
    }
}

#endif // AJ_HOST_X86 (emitter backend, part 2)

#if AJ_HOST_A64 // ================== AArch64 emitter backend ==================
//
// Same contracts as the x86 backend above, emitted for AAPCS64. Register plan
// inside a block: w0 = primary value/result ("eax"), w1 = operand 2 ("ecx"),
// w2 = Rn / shift amount ("edx"), w3 = shifter carry-out stash ("esi"),
// w16/w17 = helper scratch; x19 = ctx (callee-saved). All except x19 are
// caller-saved, so nothing needs spilling around thunk calls.
//
// The guest ISA is this host's direct ancestor, so most mappings are 1:1:
// the condition-code encodings match, `msr nzcv` loads guest flags straight
// into the host flags, and the NZCV semantics are identical INCLUDING the
// two spots that needed extra work on x86 -- C = NOT-borrow on subtraction,
// and SBC computing Rn - op2 - !C natively (no cmc dance)

// ABI call to a C thunk: argument(s) already in w0 (+w1 for writes), result
// in w0 -- native AAPCS64, nothing to shuffle (unlike the x86 ECX/EDX contract)
inline void emit_mem_call(a64::Assembler& a, const void* fn, bool /*isWrite*/)
{
    a.mov(a64::x16, (uint64_t)(uintptr_t)fn); // x16/IP0: intra-call scratch by ABI
    a.blr(a64::x16);
}

// charge '*s_picount -= cyc' at a block exit (clobbers x16/w17)
inline void emit_charge_cycles(a64::Assembler& a, int cyc)
{
    if (s_picount == nullptr || cyc == 0) return;
    a.mov(a64::x16, (uint64_t)(uintptr_t)s_picount);
    a.ldr(a64::w17, a64::ptr(a64::x16));
    if (cyc > 0) a.sub(a64::w17, a64::w17, cyc);
    else         a.add(a64::w17, a64::w17, -cyc);
    a.str(a64::w17, a64::ptr(a64::x16));
}

// Guest condition -> host condition: the 4-bit encodings are identical, but
// asmjit's portable CondCode enum reorders them (kAL=0, kEQ=2, ...): guest
// cond c in 0..13 is CondCode(c + 2), and flipping bit 0 still inverts.
// One msr replaces the x86 backend's whole compound-condition test ladder
inline bool emit_cond_skip(a64::Assembler& a, const a64::Gp& ctx, uint32_t cond, const Label& skip)
{
    if (cond == 0xEu) return true;  // AL: no code
    if (cond == 0xFu) return false; // NV: unsupported
    a.ldr(a64::w16, a64::ptr(ctx, cpsr_off()));
    a.msr(Imm(a64::Predicate::SysReg::kNZCV), a64::x16); // guest N/Z/C/V (bits 31:28) -> host flags
    a.b((arm::CondCode)((cond ^ 1u) + 2u), skip);             // branch away when FALSE
    return true;
}

// host C := guest C for adc/sbc (a full NZCV load; N/Z/V are then overwritten
// by the adcs/sbcs that follows, or ignored). Clobbers w16
inline void emit_carry_in(a64::Assembler& a, const a64::Gp& ctx)
{
    a.ldr(a64::w16, a64::ptr(ctx, cpsr_off()));
    a.msr(Imm(a64::Predicate::SysReg::kNZCV), a64::x16);
}

// Set N/Z in CPSR from the result in w0, preserving V (and everything else);
// C is preserved too unless the operand-2 shifter defined it (setC + cVal,
// a translate-time constant for rotated immediates). Clobbers w16/w17
void emit_set_flags_logical(a64::Assembler& a, const a64::Gp& ctx, bool setC, bool cVal)
{
    a.cmp(a64::w0, 0);                                   // host N/Z from the result
    a.mrs(a64::x17, Imm(a64::Predicate::SysReg::kNZCV));
    a.and_(a64::w17, a64::w17, kN | kZ);
    a.ldr(a64::w16, a64::ptr(ctx, cpsr_off()));
    a.and_(a64::w16, a64::w16, setC ? ~(kN | kZ | kC) : ~(kN | kZ));
    if (setC && cVal) a.orr(a64::w16, a64::w16, kC);
    a.orr(a64::w16, a64::w16, a64::w17);
    a.str(a64::w16, a64::ptr(ctx, cpsr_off()));
}

// Same, with C from the shifter carry-out stashed in w3 bit 0 (the a64 twin
// of emit_set_flags_logical_carry_esi). Clobbers w3/w16/w17
void emit_set_flags_logical_carry_w3(a64::Assembler& a, const a64::Gp& ctx)
{
    a.cmp(a64::w0, 0);
    a.mrs(a64::x17, Imm(a64::Predicate::SysReg::kNZCV));
    a.and_(a64::w17, a64::w17, kN | kZ);
    a.ldr(a64::w16, a64::ptr(ctx, cpsr_off()));
    a.and_(a64::w16, a64::w16, ~(kN | kZ | kC));
    a.orr(a64::w16, a64::w16, a64::w17);
    a.and_(a64::w3, a64::w3, 1u);
    a.lsl(a64::w3, a64::w3, 29);                         // carry bit 0 -> kC (bit 29)
    a.orr(a64::w16, a64::w16, a64::w3);
    a.str(a64::w16, a64::ptr(ctx, cpsr_off()));
}

// Set N/Z/C/V by capturing the host NZCV right after an adds/subs/adcs/sbcs
// (only flag-transparent instructions may sit in between, e.g. the Rd str).
// No carry inversion is ever needed on this host (see the header note);
// cIsBorrow is kept for x86 signature parity. Clobbers w16/w17
void emit_set_flags_after_carry(a64::Assembler& a, const a64::Gp& ctx, bool /*cIsBorrow*/)
{
    a.mrs(a64::x17, Imm(a64::Predicate::SysReg::kNZCV)); // N/Z/C/V at bits 31:28
    a.and_(a64::w17, a64::w17, kN | kZ | kC | kV);
    a.ldr(a64::w16, a64::ptr(ctx, cpsr_off()));
    a.and_(a64::w16, a64::w16, ~(kN | kZ | kC | kV));    // preserve mode bits etc.
    a.orr(a64::w16, a64::w16, a64::w17);
    a.str(a64::w16, a64::ptr(ctx, cpsr_off()));
}
void emit_set_flags_arith(a64::Assembler& a, const a64::Gp& ctx, bool isSub)
{ emit_set_flags_after_carry(a, ctx, isSub); }

// Emit one data-processing instruction (condition already handled by the
// shared router). Mirrors the x86 emitter's coverage and deferrals exactly.
// One structural difference: an immediate operand 2 is always MATERIALIZED
// into w1 (AArch64 logical-immediate encodings can't hold arbitrary rotated
// ARM immediates), which collapses the x86 version's separate immediate and
// register paths into a single opcode switch
bool emit_arm_dp(a64::Assembler& a, const a64::Gp& ctx, uint32_t insn, uint32_t pc)
{
    if (((insn >> 26) & 0x3u) != 0u) return false; // data-processing class only

    const bool     useReg = !((insn >> 25) & 1u);
    const uint32_t opcode = (insn >> 21) & 0xFu;
    const bool     Sraw   = (insn >> 20) & 1u;
    const uint32_t Rn     = (insn >> 16) & 0xFu;
    const uint32_t Rd     = (insn >> 12) & 0xFu;
    const bool     compareOp = (opcode >= 0x8u && opcode <= 0xBu);
    if (compareOp && !Sraw) return false; // TST/TEQ/CMP/CMN encodings need S=1 (S=0 is the MRS/MSR/BX space)
    if (Rd == 15u && Sraw) {
        if (compareOp) return false; // deprecated TSTP/TEQP/CMPP/CMNP forms -> interpreter (see x86 note)
        // MOVS PC / SUBS PC,LR (exception return): needs the wired helper
        if (s_memMode != MemMode::Callback || !s_memcb.exc_return) return false;
    }
    const bool S = Sraw && Rd != 15u; // Rd==PC never sets flags (S=1 = SPSR restore, handled at exit)
    if (Rn == 15u) {
        // pipelined PC as a translate-time constant, parked in the R15 slot (same trick as x86)
        const uint32_t addpc = ((insn >> 25) & 1u) ? 8u : ((insn & 0x10u) ? 12u : 8u);
        a.mov(a64::w16, pc + addpc);
        a.str(a64::w16, a64::ptr(ctx, reg_off(15)));
    }

    const bool logicalOp = (opcode == 0x0u || opcode == 0x1u || opcode == 0x8u ||
                            opcode == 0x9u || opcode == 0xCu || opcode == 0xDu ||
                            opcode == 0xEu || opcode == 0xFu);
    bool shiftCarryInW3  = false; // shifter carry-out (S-logical) parked in w3 bit 0
    bool immCarryDefined = false;
    bool immCarry        = false;

    // ---- operand 2 -> w1 ----------------------------------------------------
    if (useReg) {
        const uint32_t Rm  = insn & 0xFu;
        const uint32_t shf = (insn >> 4) & 0xFFu; // shift field (insn bits 11-4)
        const bool logicalS = S && logicalOp;
        if ((shf & 1u) == 0u) {
            // immediate shift amount (Rm == R15 reads the pipelined PC, +8)
            if (Rm == 15u) a.mov(a64::w1, pc + 8u);
            else           a.ldr(a64::w1, a64::ptr(ctx, reg_off((int)Rm)));
            if (shf != 0u) {
                const uint32_t stype = (shf >> 1) & 0x3u;  // insn bits 6-5
                const uint32_t sh    = (shf >> 3) & 0x1Fu; // insn bits 11-7
                if (logicalS) {
                    // shifter carry-out (see the x86 twin for the bit table)
                    const uint32_t cbit = (sh == 0u)
                        ? ((stype == 3u) ? 0u : 31u)
                        : ((stype == 0u) ? (32u - sh) : (sh - 1u));
                    if (cbit) a.lsr(a64::w3, a64::w1, cbit);
                    else      a.mov(a64::w3, a64::w1);
                    a.and_(a64::w3, a64::w3, 1u);
                    shiftCarryInW3 = true;
                }
                if (sh == 0u) {
                    // special #0 forms: LSR/ASR #0 = #32; ROR #0 = RRX
                    switch (stype) {
                    case 1:  a.mov(a64::w1, 0);           break; // LSR #32 -> 0
                    case 2:  a.asr(a64::w1, a64::w1, 31); break; // ASR #32 -> sign fill
                    default:                                     // RRX: w1 = (C << 31) | (Rm >> 1)
                        a.lsr(a64::w1, a64::w1, 1);
                        a.ldr(a64::w16, a64::ptr(ctx, cpsr_off()));
                        a.and_(a64::w16, a64::w16, kC);          // isolate C (bit 29)
                        a.lsl(a64::w16, a64::w16, 2);            // -> bit 31
                        a.orr(a64::w1, a64::w1, a64::w16);
                        break;
                    }
                } else {
                    switch (stype) {
                    case 0:  a.lsl(a64::w1, a64::w1, sh); break;
                    case 1:  a.lsr(a64::w1, a64::w1, sh); break;
                    case 2:  a.asr(a64::w1, a64::w1, sh); break;
                    default: a.ror(a64::w1, a64::w1, sh); break;
                    }
                }
            }
        } else {
            // register-specified shift amount; ARM amounts >= 32 are special
            // (AArch64 variable shifts mask the count mod 32, like x86's mod-31 issue)
            if (shf & 0x8u) return false; // bit 7 = 1 -> mul-class, not DP
            if (logicalS)   return false; // run-time shift amount + logical S -> defer (like x86)
            const uint32_t Rs    = (shf >> 4) & 0xFu;
            const uint32_t stype = (shf >> 1) & 0x3u;
            if (Rs == 15u) return false;
            if (Rm == 15u) a.mov(a64::w1, pc + 12u); // pipelined PC: +12 with a register amount
            else           a.ldr(a64::w1, a64::ptr(ctx, reg_off((int)Rm)));
            a.ldr(a64::w2, a64::ptr(ctx, reg_off((int)Rs)));
            a.and_(a64::w2, a64::w2, 0xFFu);         // shift amount = bottom byte of Rs
            if (stype == 3u) {
                a.ror(a64::w1, a64::w1, a64::w2);    // RORV's mod-32 count matches ARM's value result
            } else {
                Label lt32 = a.new_label();
                Label done = a.new_label();
                a.cmp(a64::w2, 32);
                a.b(arm::CondCode::kUnsignedLT, lt32);
                if (stype == 2u) a.asr(a64::w1, a64::w1, 31); // ASR >= 32 -> sign fill
                else             a.mov(a64::w1, 0);           // LSL/LSR >= 32 -> 0
                a.b(done);
                a.bind(lt32);
                if (stype == 0u)      a.lsl(a64::w1, a64::w1, a64::w2);
                else if (stype == 1u) a.lsr(a64::w1, a64::w1, a64::w2);
                else                  a.asr(a64::w1, a64::w1, a64::w2);
                a.bind(done);
            }
        }
    } else {
        const uint32_t rot = ((insn >> 8) & 0xFu) * 2u;
        const uint32_t v   = insn & 0xFFu;
        const uint32_t imm = rot ? ((v >> rot) | (v << (32u - rot))) : v;
        immCarryDefined = (rot != 0u); // C from operand 2 defined only if rotated
        immCarry        = ((imm >> 31) & 1u) != 0u;
        a.mov(a64::w1, imm);
    }

    // ---- Rn -> w2, compute into w0 ------------------------------------------
    if (!(opcode == 0xDu || opcode == 0xFu)) // MOV/MVN ignore Rn
        a.ldr(a64::w2, a64::ptr(ctx, reg_off((int)Rn)));

    switch (opcode) {
    // logical (host flags not used: N/Z come from the result in the helpers)
    case 0xDu: a.mov (a64::w0, a64::w1); break;                     // MOV
    case 0xFu: a.mvn_(a64::w0, a64::w1); break;                     // MVN (mvn_: MSVC arm64_neon.h defines a 'mvn' macro)
    case 0x0u: case 0x8u: a.and_(a64::w0, a64::w2, a64::w1); break; // AND / TST
    case 0x1u: case 0x9u: a.eor (a64::w0, a64::w2, a64::w1); break; // EOR / TEQ
    case 0xCu: a.orr (a64::w0, a64::w2, a64::w1); break;            // ORR
    case 0xEu: a.bic (a64::w0, a64::w2, a64::w1); break;            // BIC
    // arithmetic (the S forms set host NZCV, captured right after the store)
    case 0x4u: case 0xBu: S ? a.adds(a64::w0, a64::w2, a64::w1) : a.add(a64::w0, a64::w2, a64::w1); break; // ADD / CMN
    case 0x2u: case 0xAu: S ? a.subs(a64::w0, a64::w2, a64::w1) : a.sub(a64::w0, a64::w2, a64::w1); break; // SUB / CMP
    case 0x3u:            S ? a.subs(a64::w0, a64::w1, a64::w2) : a.sub(a64::w0, a64::w1, a64::w2); break; // RSB: op2 - Rn
    case 0x5u: emit_carry_in(a, ctx); S ? a.adcs(a64::w0, a64::w2, a64::w1) : a.adc(a64::w0, a64::w2, a64::w1); break; // ADC
    case 0x6u: emit_carry_in(a, ctx); S ? a.sbcs(a64::w0, a64::w2, a64::w1) : a.sbc(a64::w0, a64::w2, a64::w1); break; // SBC: Rn - op2 - !C, native
    case 0x7u: emit_carry_in(a, ctx); S ? a.sbcs(a64::w0, a64::w1, a64::w2) : a.sbc(a64::w0, a64::w1, a64::w2); break; // RSC: op2 - Rn - !C
    default: return false;
    }

    if (!compareOp) a.str(a64::w0, a64::ptr(ctx, reg_off((int)Rd))); // TST/TEQ/CMP/CMN: no write-back

    if (S) {
        if (!logicalOp)          emit_set_flags_after_carry(a, ctx, false);    // full NZCV from the host op
        else if (shiftCarryInW3) emit_set_flags_logical_carry_w3(a, ctx);      // C = shifter carry-out
        else if (!useReg)        emit_set_flags_logical(a, ctx, immCarryDefined, immCarry);
        else                     emit_set_flags_logical(a, ctx, false, false); // unshifted reg op2: C preserved
    }
    return true;
}

// B/BL: identical structure to the x86 twin
bool emit_arm_branch(a64::Assembler& a, const a64::Gp& ctx, uint32_t insn, uint32_t pc, FuncFrame& frame, int cyc)
{
    const bool     L      = (insn >> 24) & 1u;         // link
    const int32_t  off    = (int32_t)(insn << 8) >> 6; // sign-extended imm24, scaled *4
    const uint32_t target = pc + 8u + (uint32_t)off;
    if (L) {
        a.mov(a64::w16, pc + 4u);                      // LR = address of the next instruction
        a.str(a64::w16, a64::ptr(ctx, reg_off(14)));
    }
    emit_charge_cycles(a, cyc);
    a.mov(a64::w0, target);
    a.emit_epilog(frame);                              // return target as the next PC
    return true;
}

// Block exit for a DP instruction that wrote R15 (the a64 twin of the x86
// emit_dp_pc_exit; see there for the exception-return contract)
inline void emit_dp_pc_exit(a64::Assembler& a, const a64::Gp& ctx, bool excReturn, FuncFrame& frame, int cycCharge)
{
    a.ldr(a64::w0, a64::ptr(ctx, reg_off(15)));
    if (excReturn) emit_mem_call(a, s_memcb.exc_return, false); // w0 = final PC from the helper
    else           a.and_(a64::w0, a64::w0, 0xFFFFFFFCu);       // PC bits 1:0 ignored in ARM state
    emit_charge_cycles(a, cycCharge);
    a.emit_epilog(frame);
}

// Post-access abort/IRQ checks: the a64 twin of the x86 emit_mem_post_checks
// (see there for the exit policy and cycle notes). Clobbers w16/w17
inline void emit_mem_post_checks(a64::Assembler& a, const a64::Gp& ctx, uint32_t pc, FuncFrame& frame, int cyc)
{
    if (!s_memcb.pIrq || !s_memcb.pIrqFlag) return; // hooks not wired -> no codegen
    Label do_exit = a.new_label();
    Label l_irq   = a.new_label();
    Label cont    = a.new_label();
    // Data abort: always taken
    a.mov(a64::x16, (uint64_t)(uintptr_t)s_memcb.pAbtD);
    a.ldrb(a64::w17, a64::ptr(a64::x16));
    a.cbnz(a64::w17, do_exit);
    // FIQ: taken if pending AND F (bit 6) clear
    a.mov(a64::x16, (uint64_t)(uintptr_t)s_memcb.pFiq);
    a.ldrb(a64::w17, a64::ptr(a64::x16));
    a.cbz(a64::w17, l_irq);
    a.ldr(a64::w16, a64::ptr(ctx, cpsr_off()));
    a.tst(a64::w16, 0x40u);
    a.b(arm::CondCode::kEQ, do_exit);       // unmasked -> vector
    a.bind(l_irq);
    // IRQ: taken if pending AND I (bit 7) clear
    a.mov(a64::x16, (uint64_t)(uintptr_t)s_memcb.pIrq);
    a.ldrb(a64::w17, a64::ptr(a64::x16));
    a.cbz(a64::w17, cont);
    a.ldr(a64::w16, a64::ptr(ctx, cpsr_off()));
    a.tst(a64::w16, 0x80u);
    a.b(arm::CondCode::kNE, cont);          // masked -> continue in-block
    a.bind(do_exit);
    a.mov(a64::x16, (uint64_t)(uintptr_t)s_memcb.pIrqFlag);
    a.mov(a64::w17, 1);
    a.strb(a64::w17, a64::ptr(a64::x16));   // request the IRQ check in the exec loop
    emit_charge_cycles(a, cyc);             // executed prefix only
    a.mov(a64::w0, pc + 4u);                // resume at the next instruction
    a.emit_epilog(frame);
    a.bind(cont);
}

// Single data transfer LDR/STR/LDRB/STRB: mirrors the x86 twin (same decode,
// deferrals, three memory modes and call-safety discipline). Register plan:
// w1 = base, w2 = effective address, w3 = register offset, w0 = value/result,
// x16 = mock host address / thunk target
bool emit_arm_ldrstr(a64::Assembler& a, const a64::Gp& ctx, uint32_t insn, uint32_t pc, FuncFrame& frame, int cyc)
{
    if (s_memMode == MemMode::Defer) return false;
    const bool     I  = (insn >> 25) & 1u;    // 0 = immediate offset (inverted vs DP), 1 = register
    const bool     P  = (insn >> 24) & 1u;
    const bool     U  = (insn >> 23) & 1u;
    const bool     B  = (insn >> 22) & 1u;
    const bool     W  = (insn >> 21) & 1u;
    const bool     L  = (insn >> 20) & 1u;
    const uint32_t Rn = (insn >> 16) & 0xFu;
    const uint32_t Rd = (insn >> 12) & 0xFu;
    if (Rd == 15u && Rn == 15u) return false; // both PC operands at once: too rare

    bool     offIsReg = false;
    uint32_t off      = 0;
    if (!I) {
        off = insn & 0xFFFu;                  // imm12
    } else {
        const uint32_t Rm = insn & 0xFu;
        if (Rm == 15u) return false;
        const uint32_t shf = (insn >> 4) & 0xFFu;
        if (shf & 1u) return false;           // register-specified shift amount not valid here
        a.ldr(a64::w3, a64::ptr(ctx, reg_off((int)Rm)));
        if (shf != 0u) {
            const uint32_t stype = (shf >> 1) & 0x3u;
            const uint32_t sh    = (shf >> 3) & 0x1Fu;
            if (sh == 0u) {
                switch (stype) {
                case 1:  a.mov(a64::w3, 0); break;           // LSR #32
                case 2:  a.asr(a64::w3, a64::w3, 31); break; // ASR #32
                default:                                     // RRX
                    a.lsr(a64::w3, a64::w3, 1);
                    a.ldr(a64::w16, a64::ptr(ctx, cpsr_off()));
                    a.and_(a64::w16, a64::w16, kC);
                    a.lsl(a64::w16, a64::w16, 2);
                    a.orr(a64::w3, a64::w3, a64::w16);
                    break;
                }
            } else {
                switch (stype) {
                case 0:  a.lsl(a64::w3, a64::w3, sh); break;
                case 1:  a.lsr(a64::w3, a64::w3, sh); break;
                case 2:  a.asr(a64::w3, a64::w3, sh); break;
                default: a.ror(a64::w3, a64::w3, sh); break;
                }
            }
        }
        offIsReg = true;
    }

    // shared address arithmetic: w1 = base, w2 = effective address
    const bool pcBase = (Rn == 15u);
    if (s_memMode == MemMode::Callback && pcBase && (!P || W)) return false; // PC-base writeback -> defer
    if (pcBase) a.mov(a64::w1, pc + 8u);
    else        a.ldr(a64::w1, a64::ptr(ctx, reg_off((int)Rn)));
    a.mov(a64::w2, a64::w1);
    if (P) {
        if (offIsReg)       { if (U) a.add(a64::w2, a64::w2, a64::w3); else a.sub(a64::w2, a64::w2, a64::w3); }
        else if (off != 0u) { if (U) a.add(a64::w2, a64::w2, off);     else a.sub(a64::w2, a64::w2, off); }
    }

    if (s_memMode == MemMode::Callback) {
        if (!P) {                             // post-index writeback: Rn = base +/- off
            if (offIsReg) { if (U) a.add(a64::w1, a64::w1, a64::w3); else a.sub(a64::w1, a64::w1, a64::w3); }
            else          { if (U) a.add(a64::w1, a64::w1, off);     else a.sub(a64::w1, a64::w1, off); }
            a.str(a64::w1, a64::ptr(ctx, reg_off((int)Rn)));
        } else if (W) {                       // pre-index writeback: Rn = effective address
            a.str(a64::w2, a64::ptr(ctx, reg_off((int)Rn)));
        }
        if (L) {
            a.mov(a64::w0, a64::w2);          // arg0 = address
            emit_mem_call(a, B ? s_memcb.r8 : s_memcb.r32, false); // -> w0
            if (Rd == 15u) {                  // LDR into PC = return
                a.and_(a64::w0, a64::w0, 0xFFFFFFFCu);
                emit_charge_cycles(a, cyc);
                a.emit_epilog(frame);
            }
            else a.str(a64::w0, a64::ptr(ctx, reg_off((int)Rd)));
        } else {
            if (Rd == 15u) a.mov(a64::w1, pc + 12u); // STR of PC stores instr+12
            else           a.ldr(a64::w1, a64::ptr(ctx, reg_off((int)Rd))); // arg1 = value
            a.mov(a64::w0, a64::w2);          // arg0 = address
            emit_mem_call(a, B ? s_memcb.w8 : s_memcb.w32, true);
        }
        // same LDR-into-PC post-check exemption as the x86 twin (see there)
        if (!(L && Rd == 15u)) emit_mem_post_checks(a, ctx, pc, frame, cyc);
        return true;
    }

    // --- Mock mode: the ctx 'mem' buffer, addressed as ctx + address + memOff
    const int memOff = (int)offsetof(ArmCpuSelfTest, mem);
    a.add(a64::x16, ctx, a64::x2);            // w2 writes zero-extend -> x2 is the address
    bool branchToPc = false;
    if (L) {
        if (B) a.ldrb(a64::w0, a64::ptr(a64::x16, memOff));
        else   a.ldr (a64::w0, a64::ptr(a64::x16, memOff));
        if (Rd == 15u) branchToPc = true;     // loaded value = new PC (stays in w0)
        else           a.str(a64::w0, a64::ptr(ctx, reg_off((int)Rd)));
    } else {
        if (Rd == 15u) a.mov(a64::w0, pc + 12u);
        else           a.ldr(a64::w0, a64::ptr(ctx, reg_off((int)Rd)));
        if (B) a.strb(a64::w0, a64::ptr(a64::x16, memOff));
        else   a.str (a64::w0, a64::ptr(a64::x16, memOff));
    }
    if (!P) {                                 // post-index writeback (always)
        if (offIsReg) { if (U) a.add(a64::w1, a64::w1, a64::w3); else a.sub(a64::w1, a64::w1, a64::w3); }
        else          { if (U) a.add(a64::w1, a64::w1, off);     else a.sub(a64::w1, a64::w1, off); }
        if (Rn == 15u) { a.mov(a64::w0, a64::w1); branchToPc = true; } // PC-base writeback = branch
        else             a.str(a64::w1, a64::ptr(ctx, reg_off((int)Rn)));
    } else if (W) {
        if (Rn == 15u) { a.mov(a64::w0, a64::w2); branchToPc = true; }
        else             a.str(a64::w2, a64::ptr(ctx, reg_off((int)Rn)));
    }
    if (branchToPc) {
        a.and_(a64::w0, a64::w0, 0xFFFFFFFCu);
        emit_charge_cycles(a, cyc);
        a.emit_epilog(frame);
    }
    return true;
}

// Halfword & signed transfer LDRH/STRH/LDRSB/LDRSH: mirrors the x86 twin
// (same decode/deferrals; no post-check -- see the note there)
bool emit_arm_halfword(a64::Assembler& a, const a64::Gp& ctx, uint32_t insn, uint32_t pc)
{
    if (s_memMode == MemMode::Defer) return false;
    const bool     P   = (insn >> 24) & 1u;
    const bool     U   = (insn >> 23) & 1u;
    const bool     Imm = (insn >> 22) & 1u;
    const bool     W   = (insn >> 21) & 1u;
    const bool     L   = (insn >> 20) & 1u;
    const uint32_t Rn  = (insn >> 16) & 0xFu;
    const uint32_t Rd  = (insn >> 12) & 0xFu;
    const uint32_t sh  = (insn >> 5)  & 0x3u; // 1=H, 2=SB, 3=SH
    if (Rd == 15u)              return false;
    if (!L && sh != 1u)         return false; // only STRH stores
    if (Rn == 15u && (W || !P)) return false; // PC-base writeback -> defer

    bool     offIsReg = false;
    uint32_t off      = 0;
    if (Imm) {
        off = (((insn >> 8) & 0xFu) << 4) | (insn & 0xFu);
    } else {
        const uint32_t Rm = insn & 0xFu;
        if (Rm == 15u) return false;
        a.ldr(a64::w3, a64::ptr(ctx, reg_off((int)Rm)));
        offIsReg = true;
    }

    if (Rn == 15u) a.mov(a64::w1, pc + 8u);
    else           a.ldr(a64::w1, a64::ptr(ctx, reg_off((int)Rn)));
    a.mov(a64::w2, a64::w1);
    if (P) {
        if (offIsReg)       { if (U) a.add(a64::w2, a64::w2, a64::w3); else a.sub(a64::w2, a64::w2, a64::w3); }
        else if (off != 0u) { if (U) a.add(a64::w2, a64::w2, off);     else a.sub(a64::w2, a64::w2, off); }
    }

    if (s_memMode == MemMode::Callback) {
        if (!P) {
            if (offIsReg) { if (U) a.add(a64::w1, a64::w1, a64::w3); else a.sub(a64::w1, a64::w1, a64::w3); }
            else          { if (U) a.add(a64::w1, a64::w1, off);     else a.sub(a64::w1, a64::w1, off); }
            a.str(a64::w1, a64::ptr(ctx, reg_off((int)Rn)));
        } else if (W) {
            a.str(a64::w2, a64::ptr(ctx, reg_off((int)Rn)));
        }
        if (L) {
            a.mov(a64::w0, a64::w2);
            emit_mem_call(a, (sh == 2u) ? s_memcb.r8 : s_memcb.r16, false);
            if      (sh == 2u) a.sxtb(a64::w0, a64::w0); // LDRSB
            else if (sh == 3u) a.sxth(a64::w0, a64::w0); // LDRSH (LDRH already zero-extended)
            a.str(a64::w0, a64::ptr(ctx, reg_off((int)Rd)));
        } else {
            a.ldr(a64::w1, a64::ptr(ctx, reg_off((int)Rd))); // arg1 (w16 write truncates)
            a.mov(a64::w0, a64::w2);
            emit_mem_call(a, s_memcb.w16, true);
        }
        return true;
    }

    const int memOff = (int)offsetof(ArmCpuSelfTest, mem);
    a.add(a64::x16, ctx, a64::x2);
    if (L) {
        if      (sh == 1u) a.ldrh (a64::w0, a64::ptr(a64::x16, memOff));
        else if (sh == 2u) a.ldrsb(a64::w0, a64::ptr(a64::x16, memOff));
        else               a.ldrsh(a64::w0, a64::ptr(a64::x16, memOff));
        a.str(a64::w0, a64::ptr(ctx, reg_off((int)Rd)));
    } else {
        a.ldr(a64::w0, a64::ptr(ctx, reg_off((int)Rd)));
        a.strh(a64::w0, a64::ptr(a64::x16, memOff));
    }
    if (!P) {
        if (offIsReg) { if (U) a.add(a64::w1, a64::w1, a64::w3); else a.sub(a64::w1, a64::w1, a64::w3); }
        else          { if (U) a.add(a64::w1, a64::w1, off);     else a.sub(a64::w1, a64::w1, off); }
        a.str(a64::w1, a64::ptr(ctx, reg_off((int)Rn)));
    } else if (W) {
        a.str(a64::w2, a64::ptr(ctx, reg_off((int)Rn)));
    }
    return true;
}

// Block data transfer LDM/STM: mirrors the x86 twin (constant per-register
// offsets, base re-read per call, writeback after the loop, PC last; see there)
bool emit_arm_blocktransfer(a64::Assembler& a, const a64::Gp& ctx, uint32_t insn, uint32_t pc, FuncFrame& frame, int cyc)
{
    if (s_memMode == MemMode::Defer) return false;
    const bool     P    = (insn >> 24) & 1u;
    const bool     U    = (insn >> 23) & 1u;
    const bool     S    = (insn >> 22) & 1u;
    const bool     W    = (insn >> 21) & 1u;
    const bool     L    = (insn >> 20) & 1u;
    const uint32_t Rn   = (insn >> 16) & 0xFu;
    const uint32_t list = insn & 0xFFFFu;
    if (S)         return false;
    if (Rn == 15u) return false;
    int count = 0;
    for (int i = 0; i < 16; ++i) if (list & (1u << i)) ++count;
    if (count == 0) return false;

    const int32_t bytes    = 4 * count;
    const int32_t firstOff = U ? (P ? 4 : 0) : (P ? -bytes : -bytes + 4);
    const int32_t finalOff = U ? bytes : -bytes;

    if (s_memMode == MemMode::Callback) {
        if (list & (1u << Rn)) return false; // base in transfer list -> defer
        bool branchToPc = false;
        int  pos = 0;
        for (int i = 0; i < 16; ++i) {
            if (!(list & (1u << i))) continue;
            const int32_t addrOff = firstOff + 4 * pos;
            a.ldr(a64::w0, a64::ptr(ctx, reg_off((int)Rn))); // arg0 = base (re-read, unchanged)
            if (addrOff > 0)      a.add(a64::w0, a64::w0, addrOff);
            else if (addrOff < 0) a.sub(a64::w0, a64::w0, -addrOff);
            if (L) {
                emit_mem_call(a, s_memcb.r32, false);        // -> w0
                if (i == 15) { a.mov(a64::w3, a64::w0); branchToPc = true; } // PC is last: no call follows
                else         a.str(a64::w0, a64::ptr(ctx, reg_off(i)));
            } else {
                if (i == 15) a.mov(a64::w1, pc + 12u);       // STM stores PC as instr+12
                else         a.ldr(a64::w1, a64::ptr(ctx, reg_off(i)));
                emit_mem_call(a, s_memcb.w32, true);
            }
            ++pos;
        }
        if (W) {
            a.ldr(a64::w0, a64::ptr(ctx, reg_off((int)Rn)));
            if (finalOff > 0) a.add(a64::w0, a64::w0, finalOff);
            else              a.sub(a64::w0, a64::w0, -finalOff);
            a.str(a64::w0, a64::ptr(ctx, reg_off((int)Rn)));
        }
        if (branchToPc) {
            a.and_(a64::w0, a64::w3, 0xFFFFFFFCu);
            emit_charge_cycles(a, cyc);
            a.emit_epilog(frame);
        }
        // no post-check: interpreter parity (see the x86 twin's note)
        return true;
    }

    const int memOff = (int)offsetof(ArmCpuSelfTest, mem);
    a.ldr(a64::w1, a64::ptr(ctx, reg_off((int)Rn)));       // base (kept for writeback)
    a.mov(a64::w2, a64::w1);                               // running transfer address
    if (firstOff > 0)      a.add(a64::w2, a64::w2, firstOff);
    else if (firstOff < 0) a.sub(a64::w2, a64::w2, -firstOff);

    bool branchToPc = false;
    for (int i = 0; i < 16; ++i) {
        if (!(list & (1u << i))) continue;
        a.add(a64::x16, ctx, a64::x2);
        if (L) {
            a.ldr(a64::w0, a64::ptr(a64::x16, memOff));
            if (i == 15) branchToPc = true;                // new PC stays in w0 (PC is last)
            else         a.str(a64::w0, a64::ptr(ctx, reg_off(i)));
        } else {
            if (i == 15) a.mov(a64::w0, pc + 12u);
            else         a.ldr(a64::w0, a64::ptr(ctx, reg_off(i)));
            a.str(a64::w0, a64::ptr(a64::x16, memOff));
        }
        a.add(a64::w2, a64::w2, 4);
    }
    if (W) {
        if (finalOff > 0) a.add(a64::w1, a64::w1,  finalOff);
        else              a.sub(a64::w1, a64::w1, -finalOff);
        a.str(a64::w1, a64::ptr(ctx, reg_off((int)Rn)));
    }
    if (branchToPc) {
        a.and_(a64::w0, a64::w0, 0xFFFFFFFCu);
        emit_charge_cycles(a, cyc);
        a.emit_epilog(frame);
    }
    return true;
}

// BX Rm: PC = Rm UNMASKED + T bit on odd (interpreter parity; see the x86 twin)
bool emit_arm_bx(a64::Assembler& a, const a64::Gp& ctx, uint32_t insn, FuncFrame& frame, int cyc)
{
    const uint32_t Rm = insn & 0xFu;
    if (Rm == 15u) return false;
    a.ldr(a64::w0, a64::ptr(ctx, reg_off((int)Rm)));
    Label even = a.new_label();
    a.tst(a64::w0, 1u);
    a.b(arm::CondCode::kEQ, even);
    a.ldr(a64::w16, a64::ptr(ctx, cpsr_off()));
    a.orr(a64::w16, a64::w16, 0x20u);       // T bit (bit 5), interpreter parity
    a.str(a64::w16, a64::ptr(ctx, cpsr_off()));
    a.bind(even);
    emit_charge_cycles(a, cyc);
    a.emit_epilog(frame);                   // return Rm as the next PC (unmasked)
    return true;
}

// SWP/SWPB: read-then-write through the thunks; same deferrals/parking as x86
bool emit_arm_swap(a64::Assembler& a, const a64::Gp& ctx, uint32_t insn)
{
    if (s_memMode != MemMode::Callback) return false;
    const uint32_t Rn = (insn >> 16) & 0xFu;
    const uint32_t Rd = (insn >> 12) & 0xFu;
    const uint32_t Rm =  insn        & 0xFu;
    const bool     B  = (insn >> 22) & 1u;
    if (Rn == 15u || Rd == 15u || Rm == 15u) return false;
    if (Rd == Rn || Rd == Rm) return false;
    a.ldr(a64::w0, a64::ptr(ctx, reg_off((int)Rn)));       // arg0 = address
    emit_mem_call(a, B ? s_memcb.r8 : s_memcb.r32, false); // w0 = old value
    a.str(a64::w0, a64::ptr(ctx, reg_off((int)Rd)));       // Rd = old value (parked)
    a.ldr(a64::w0, a64::ptr(ctx, reg_off((int)Rn)));       // address (re-read, unchanged)
    a.ldr(a64::w1, a64::ptr(ctx, reg_off((int)Rm)));       // arg1 = new value
    emit_mem_call(a, B ? s_memcb.w8 : s_memcb.w32, true);
    return true;
}

// Run-time multiply cycle refund "ARM7_ICOUNT += 4 - m" (see the x86 twin for
// the model). On this host the whole bsr/shr/xor dance collapses to
// refund = clz(|Rs| | 1) >> 3. Clobbers w16/w17/x1; emitted BEFORE the multiply
void emit_mul_cycle_refund(a64::Assembler& a, const a64::Gp& ctx, uint32_t Rs, bool sign)
{
    if (!s_picount) return;
    a.ldr(a64::w16, a64::ptr(ctx, reg_off((int)Rs)));
    if (sign) {                              // |Rs| (INT32_MIN stays put -> m = 4)
        a.asr(a64::w17, a64::w16, 31);
        a.eor(a64::w16, a64::w16, a64::w17);
        a.sub(a64::w16, a64::w16, a64::w17);
    }
    a.orr(a64::w16, a64::w16, 1u);           // Rs == 0 -> m = 1
    a.clz(a64::w17, a64::w16);
    a.lsr(a64::w17, a64::w17, 3);            // refund = clz >> 3 (= 4 - m)
    a.mov(a64::x1, (uint64_t)(uintptr_t)s_picount);
    a.ldr(a64::w16, a64::ptr(a64::x1));
    a.add(a64::w16, a64::w16, a64::w17);
    a.str(a64::w16, a64::ptr(a64::x1));
}

// MUL/MLA: single 32-bit mul (sign-agnostic low half); flags as on x86
bool emit_arm_mul32(a64::Assembler& a, const a64::Gp& ctx, uint32_t insn)
{
    const uint32_t Rd = (insn >> 16) & 0xFu; // multiply encodings put Rd at 19-16
    const uint32_t Rn = (insn >> 12) & 0xFu;
    const uint32_t Rs = (insn >>  8) & 0xFu;
    const uint32_t Rm =  insn        & 0xFu;
    const bool     A  = (insn >> 21) & 1u;
    const bool     S  = (insn >> 20) & 1u;
    if (Rd == 15u || Rm == 15u || Rs == 15u || (A && Rn == 15u)) return false;
    emit_mul_cycle_refund(a, ctx, Rs, true);
    a.ldr(a64::w0, a64::ptr(ctx, reg_off((int)Rm)));
    a.ldr(a64::w1, a64::ptr(ctx, reg_off((int)Rs)));
    a.mul(a64::w0, a64::w0, a64::w1);
    if (A) {
        a.ldr(a64::w1, a64::ptr(ctx, reg_off((int)Rn)));
        a.add(a64::w0, a64::w0, a64::w1);
    }
    a.str(a64::w0, a64::ptr(ctx, reg_off((int)Rd)));
    if (S) emit_set_flags_logical(a, ctx, false, false); // N/Z from w0; C,V preserved
    return true;
}

// UMULL/SMULL/UMLAL/SMLAL: the native widening multiply gives the 64-bit
// product in one instruction, and S-flags come from a single 64-bit cmp
// (N = bit 63, Z = 64-bit zero) -- both cheaper than the x86 twin
bool emit_arm_mul64(a64::Assembler& a, const a64::Gp& ctx, uint32_t insn)
{
    const uint32_t RdHi = (insn >> 16) & 0xFu;
    const uint32_t RdLo = (insn >> 12) & 0xFu;
    const uint32_t Rs   = (insn >>  8) & 0xFu;
    const uint32_t Rm   =  insn        & 0xFu;
    const bool   signd  = (insn >> 22) & 1u;
    const bool   A      = (insn >> 21) & 1u;
    const bool   S      = (insn >> 20) & 1u;
    if (RdHi == 15u || RdLo == 15u || Rs == 15u || Rm == 15u) return false;
    emit_mul_cycle_refund(a, ctx, Rs, signd);
    a.ldr(a64::w0, a64::ptr(ctx, reg_off((int)Rm)));
    a.ldr(a64::w1, a64::ptr(ctx, reg_off((int)Rs)));
    if (signd) a.smull(a64::x0, a64::w0, a64::w1); // x0 = Rm * Rs
    else       a.umull(a64::x0, a64::w0, a64::w1);
    if (A) {                                       // RdHi:RdLo += product
        a.ldr(a64::w2, a64::ptr(ctx, reg_off((int)RdLo)));
        a.ldr(a64::w3, a64::ptr(ctx, reg_off((int)RdHi)));
        a.lsl(a64::x3, a64::x3, 32);
        a.add(a64::x2, a64::x2, a64::x3);          // 64-bit accumulator
        a.add(a64::x0, a64::x0, a64::x2);
    }
    a.lsr(a64::x16, a64::x0, 32);
    a.str(a64::w16, a64::ptr(ctx, reg_off((int)RdHi))); // hi first (interpreter write order)
    a.str(a64::w0,  a64::ptr(ctx, reg_off((int)RdLo)));
    if (S) {
        a.cmp(a64::x0, 0);                         // 64-bit: N = bit 63, Z = whole == 0
        a.mrs(a64::x17, Imm(a64::Predicate::SysReg::kNZCV));
        a.and_(a64::w17, a64::w17, kN | kZ);
        a.ldr(a64::w16, a64::ptr(ctx, cpsr_off()));
        a.and_(a64::w16, a64::w16, ~(kN | kZ));    // C,V (and mode bits) preserved
        a.orr(a64::w16, a64::w16, a64::w17);
        a.str(a64::w16, a64::ptr(ctx, cpsr_off()));
    }
    return true;
}

// MRS/MSR via the wired HandlePSRTransfer (same contract as the x86 twin)
bool emit_arm_psr(a64::Assembler& a, const a64::Gp& ctx, uint32_t insn)
{
    (void)ctx;
    if (s_memMode != MemMode::Callback || !s_memcb.psr_transfer) return false;
    if (!((insn >> 21) & 1u) && ((insn >> 12) & 0xFu) == 15u)
        return false;  // MRS into PC -> defer (invalid anyway)
    a.mov(a64::w0, insn);                          // arg0 = instruction word
    emit_mem_call(a, s_memcb.psr_transfer, false); // HandlePSRTransfer(insn)
    return true;
}

#endif // AJ_HOST_A64 (emitter backend)

inline int arm_insn_cycles(uint32_t insn); // defined below (shared cost model)

// Translate one ARM instruction: evaluate its condition (skip the body if
// false), emit the operation, then bind the skip target. Branches and
// PC-writing transfers end the block by emitting their own epilog (hence the
// FuncFrame). 'cycCharge' is the cost of the block's instructions up to and
// including this one: block exits emitted here or in the sub-emitters charge
// exactly that (see emit_charge_cycles), and a condition-false instruction
// costing more than ARM's 1-cycle skip price refunds the difference
bool emit_arm_insn(HostAssembler& a, const HostGp& ctx, uint32_t insn, uint32_t pc, FuncFrame& frame, int cycCharge = 0)
{
    const uint32_t cond = insn >> 28;
    if (cond == 0xFu) return false;           // NV: unsupported
    // A skipped instruction really costs 1 cycle; blocks charge full per-path
    // costs at their exits, so conditionals costing more get a refund snippet
    // on the condition-false path. Cost-1 conditionals (plain DP, the common
    // case) are already exact and get no extra code
    const int  cost       = arm_insn_cycles(insn);
    const bool refundSkip = (cond != 0xEu) && cost > 1 && s_picount != nullptr;
    Label skip = a.new_label();
    emit_cond_skip(a, ctx, cond, skip);       // jumps to 'skip' if false (no-op for AL)
    const uint32_t cls = (insn >> 26) & 0x3u; // instruction class (bits 27-26)
    bool ok;
    if (cls == 0u) {                          // data processing & friends
        // BX, multiply, halfword transfer, swap, and PSR transfer all share the DP
        // class; route them (in the interpreter's dispatch order) before data processing
        if (arm_is_bx(insn)) {
            ok = emit_arm_bx(a, ctx, insn, frame, cycCharge);
        } else if (arm_is_mul32(insn)) {
            ok = emit_arm_mul32(a, ctx, insn);
        } else if (arm_is_mul64(insn)) {
            ok = emit_arm_mul64(a, ctx, insn);
        } else if ((insn & 0x02000090u) == 0x00000090u && ((insn >> 5) & 0x3u) != 0u) {
            // Halfword/signed transfer: bit7 = bit4 = 1, bit25 = 0, SH (bits 6-5) != 0
            ok = emit_arm_halfword(a, ctx, insn, pc);
        } else if (arm_is_swap(insn)) {
            ok = emit_arm_swap(a, ctx, insn);
        } else if (arm_is_psr(insn)) {
            ok = emit_arm_psr(a, ctx, insn);
        } else {
            ok = emit_arm_dp(a, ctx, insn, pc);
            // A DP op that writes R15 (and isn't a no-write-back compare TST/TEQ/
            // CMP/CMN) exits the block: emit_arm_dp stored the result to the R15
            // slot (flag-less), so read it back as the new PC
            const uint32_t opc = (insn >> 21) & 0xFu;
            const bool compare = (opc >= 0x8u && opc <= 0xBu);
            if (ok && ((insn >> 12) & 0xFu) == 15u && !compare) {
                // S=1: exception return (MOVS PC / SUBS PC,LR,...) through the
                // wired helper; emit_arm_dp already verified it is wired.
                // S=0: plain PC write, masked. Both exit the block (per-arch)
                emit_dp_pc_exit(a, ctx, ((insn >> 20) & 1u) != 0u, frame, cycCharge);
            }
        }
    }
    else if (cls == 1u)        ok = emit_arm_ldrstr(a, ctx, insn, pc, frame, cycCharge); // LDR/STR
    else if (cls == 2u) {
        if ((insn >> 25) & 1u) ok = emit_arm_branch(a, ctx, insn, pc, frame, cycCharge);        // B/BL (101)
        else                   ok = emit_arm_blocktransfer(a, ctx, insn, pc, frame, cycCharge); // LDM/STM (100)
    }
    else                       ok = false;  // coprocessor / SWI -> later
    if (ok && refundSkip) {
        // executed path hops over the refund (dead code after an always-exiting
        // body like a taken branch -- harmless); the condition-false path lands
        // on it and gives back cost-1 before continuing in the block
        Label end = a.new_label();
        emit_jump(a, end);
        a.bind(skip);
        emit_charge_cycles(a, -(cost - 1));
        a.bind(end);
    } else {
        a.bind(skip);
    }
    return ok;
}

// Cycle cost of one translated ARM instruction: the ARM7TDMI datasheet's S/N/I
// formulas with S = N = I = 1 (the interpreter's model exactly -- see the
// cycle-counting note atop arm7core.c; bus wait states etc. are not modeled).
// Historically expressed as the legacy x86 JIT's (arm7jit.c) "default 3
// cycles with per-op deltas": DP -2 (=1), +1 register-specified shift, +2 PC dest;
// LDR/STR load 3 / store 2 / load-PC 5; halfword load 3 / STRH 2; LDM (n+2, +2 if
// PC); STM (n+1); B/BL 3; MRS/MSR 3-2 (=1); multiplies see below.
// Returns the EXECUTE cost; a conditional instruction that is skipped at run
// time really costs 1, which the emitted code accounts for exactly via a
// refund on the condition-false path (see emit_arm_insn's refundSkip).
//
// Only called for instructions the JIT actually translates. Ops the JIT defers
// (coprocessor, SWI, ...) end the block and are executed AND
// cycle-counted by the interpreter.
//
// Multiplies: the real cost is data-dependent (1S + mI, m = 1..4 by multiplier
// magnitude; +1I accumulate; long forms +1I more -- arm7core.c HandleMul et
// al). The constants below charge the WORST CASE (m = 4); the multiply
// emitters then refund 4-m at run time from the actual multiplier value
// (emit_mul_cycle_refund), so the charged total is spec/interpreter-exact
inline int arm_insn_cycles(uint32_t insn)
{
    const uint32_t cls = (insn >> 26) & 0x3u;
    if (cls == 0u) {
        if (arm_is_bx(insn))    return 3;                                       // BX (legacy default 3)
        if (arm_is_mul32(insn)) return ((insn >> 21) & 1u) ? 6 : 5;             // MUL/MLA: 1S+mI (+1I A), worst-case m=4; run-time refund of 4-m
        if (arm_is_mul64(insn)) return ((insn >> 21) & 1u) ? 7 : 6;             // MULL/MLAL: 1S+(m+1)I (+1I A), worst-case m=4; run-time refund
        if (arm_is_swap(insn))  return 4;                                       // SWP/SWPB (legacy SWAP 3+1; = interpreter's net 4)
        if (arm_is_psr(insn))   return 1;                                       // MRS/MSR (legacy PSRX 3-2; = interpreter's net 1)
        if ((insn & 0x02000090u) == 0x00000090u && ((insn >> 5) & 0x3u) != 0u)  // halfword/signed
            return ((insn >> 20) & 1u) ? 3 : 2;                                 // load 3 (PC-load deferred) / STRH 2
        int c = 1;                                                              // data processing
        if (!((insn >> 25) & 1u) && ((insn >> 4) & 1u)) c += 1;                 // register-specified shift amount
        const uint32_t opc = (insn >> 21) & 0xFu;
        const bool compare = (opc >= 0x8u && opc <= 0xBu);
        if (((insn >> 12) & 0xFu) == 15u && !compare) c += 2;                   // PC destination
        return c;
    }
    if (cls == 1u) {                                                            // LDR/STR
        const bool L = (insn >> 20) & 1u;
        if (L && ((insn >> 12) & 0xFu) == 15u) return 5;                        // LDR into PC
        return L ? 3 : 2;                                                       // load / store
    }
    if (cls == 2u) {
        if ((insn >> 25) & 1u) return 3;                                        // branch B/BL
        const bool L = (insn >> 20) & 1u;                                       // LDM/STM
        const uint32_t list = insn & 0xFFFFu;
        int n = 0; for (int i = 0; i < 16; ++i) if (list & (1u << i)) ++n;
        if (L) return n + 2 + ((list & 0x8000u) ? 2 : 0);                       // LDM (+2 if PC in list)
        return n + 1;                                                           // STM
    }
    return 3;                                                                   // fallback (untranslated -> not reached)
}

// True if 'insn' transfers control / writes PC. translate_block ends the block
// here (a basic-block boundary), whether the instruction is conditional or not:
// a taken branch returns its target, a not-taken one returns the fall-through PC.
// Mirrors the emitters' epilog cases
inline bool arm_insn_ends_block(uint32_t insn)
{
    const uint32_t cls = (insn >> 26) & 0x3u;
    if (cls == 2u) {
        if ((insn >> 25) & 1u) return true;                                     // branch
        return ((insn >> 20) & 1u) && (insn & 0x8000u);                         // LDM with PC
    }
    if (cls == 1u)                                                              // LDR into PC
        return ((insn >> 20) & 1u) && (((insn >> 12) & 0xFu) == 15u);
    if (cls == 0u) {
        if (arm_is_bx(insn)) return true;                                       // BX: PC = Rm
        // Multiply, swap, and PSR transfer never write the PC (R15 forms are deferred
        // by their emitters). Bits 15-12 hold Rn/Rd/SBO for these encodings, so they
        // must not fall through to the DP "writes PC" test below (e.g. MLA with Rn=15).
        if (arm_is_mul32(insn) || arm_is_mul64(insn) || arm_is_swap(insn) || arm_is_psr(insn)) return false;
        if ((insn & 0x02000090u) == 0x00000090u && ((insn >> 5) & 0x3u) != 0u) return false; // halfword
        const uint32_t opc = (insn >> 21) & 0xFu;
        const bool compare = (opc >= 0x8u && opc <= 0xBu);
        return (((insn >> 12) & 0xFu) == 15u) && !compare;                      // DP writes PC
    }
    return false;
}

} // namespace

// Translate a small fixed ARM program into one block, run it, and
// verify the resulting state. Returns 1 on success, 0 on failure. Exercises the
// real decode->emit path (vs. the hand-emitted jit_asmjit_selftest())
static int jit_asmjit_translate_selftest(void)
{
    // MOV r0,#10 ; ADD r1,r0,#5 ; SUB r2,r1,#3   -> r0=10, r1=15, r2=12
    static const uint32_t prog[] = { 0xE3A0000Au, 0xE2801005u, 0xE2412003u };
    const uint32_t base_pc = 0x1000u;
    const int      count   = (int)(sizeof(prog) / sizeof(prog[0]));

    JitRuntime rt;
    CodeHolder code;
    code.init(rt.environment());
    HostAssembler a(&code);

    FuncDetail func;
    func.init(FuncSignature::build<uint32_t, ArmCpuSelfTest *>(), code.environment());
    FuncFrame frame;
    frame.init(func);
    HostGp ctx = host_ctx_reg(a);
    host_add_block_regs(frame, a, ctx); // ctx + per-host callee-saved scratch
    FuncArgsAssignment args(&func);
    args.assign_all(ctx);
    args.update_func_frame(frame);
    frame.finalize();

    a.emit_prolog(frame);
    a.emit_args_assignment(frame, args);

    uint32_t pc = base_pc;
    for (int i = 0; i < count; ++i) {
        if (!emit_arm_dp(a, ctx, prog[i], pc))
            return 0; // unsupported op in the test program -> translator regressed
        emit_ctx_cycles_sub(a, ctx, 1);
        pc += 4;
    }
    emit_return_imm(a, pc); // return next PC
    a.emit_epilog(frame);

    SelfTestBlockFn fn = nullptr;
    if (rt.add(&fn, &code) != kErrorOk)
        return 0;
    ArmCpuSelfTest cpu{};
    cpu.cycles = 100;
    uint32_t rpc = fn(&cpu);
    rt.release(fn);

    return (cpu.r[0] == 10 && cpu.r[1] == 15 && cpu.r[2] == 12 &&
            cpu.cycles == 100 - count && rpc == base_pc + 4u * (uint32_t)count) ? 1 : 0;
}

// Translate a sequence of conditional MOVs against a fixed CPSR (Z set), and
// verify only the true-condition instructions wrote their register. Exercises
// the portable condition evaluator (simple + compound conditions)
static int jit_asmjit_cond_selftest(void)
{
    // CPSR = Z set (Z=1, N=0, C=0, V=0). MOV<cc> Rd,#7.
    static const uint32_t prog[] = {
        0x03A00007u, // MOVEQ r0,#7  -> exec (Z=1)
        0x13A01007u, // MOVNE r1,#7  -> skip
        0x23A02007u, // MOVCS r2,#7  -> skip (C=0)
        0x33A03007u, // MOVCC r3,#7  -> exec (C=0)
        0xA3A04007u, // MOVGE r4,#7  -> exec (N==V)
        0xB3A05007u, // MOVLT r5,#7  -> skip (N==V)
    };
    const int count = (int)(sizeof(prog) / sizeof(prog[0]));

    JitRuntime rt;
    CodeHolder code;
    code.init(rt.environment());
    HostAssembler a(&code);

    FuncDetail func;
    func.init(FuncSignature::build<uint32_t, ArmCpuSelfTest *>(), code.environment());
    FuncFrame frame;
    frame.init(func);
    HostGp ctx = host_ctx_reg(a);
    host_add_block_regs(frame, a, ctx); // ctx + per-host callee-saved scratch
    FuncArgsAssignment args(&func);
    args.assign_all(ctx);
    args.update_func_frame(frame);
    frame.finalize();

    a.emit_prolog(frame);
    a.emit_args_assignment(frame, args);
    for (int i = 0; i < count; ++i)
        if (!emit_arm_insn(a, ctx, prog[i], 4u * (uint32_t)i, frame)) // synthetic PC: 0, 4, 8, ...
            return 0;
    emit_return_imm(a, 0u);
    a.emit_epilog(frame);

    SelfTestBlockFn fn = nullptr;
    if (rt.add(&fn, &code) != kErrorOk)
        return 0;
    ArmCpuSelfTest cpu{};
    cpu.cpsr = kZ;  // Z set
    fn(&cpu);
    rt.release(fn);

    return (cpu.r[0] == 7 && cpu.r[1] == 0 && cpu.r[2] == 0 &&
            cpu.r[3] == 7 && cpu.r[4] == 7 && cpu.r[5] == 0) ? 1 : 0;
}

// Translate data-processing with a shifted-register operand 2 (immediate shift
// amounts) and verify the results. Exercises LSL/LSR/ASR and a shifted ADD
static int jit_asmjit_shift_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00010u, // MOV r0,#0x10            -> r0 = 16
        0xE1A01200u, // MOV r1, r0, LSL #4      -> r1 = 256
        0xE1A02120u, // MOV r2, r0, LSR #2      -> r2 = 4
        0xE1A030C0u, // MOV r3, r0, ASR #1      -> r3 = 8
        0xE0804080u, // ADD r4, r0, r0, LSL #1  -> r4 = 48
    };
    const int count = (int)(sizeof(prog) / sizeof(prog[0]));

    JitRuntime rt;
    CodeHolder code;
    code.init(rt.environment());
    HostAssembler a(&code);
    FuncDetail func;
    func.init(FuncSignature::build<uint32_t, ArmCpuSelfTest *>(), code.environment());
    FuncFrame frame;
    frame.init(func);
    HostGp ctx = host_ctx_reg(a);
    host_add_block_regs(frame, a, ctx); // ctx + per-host callee-saved scratch
    FuncArgsAssignment args(&func);
    args.assign_all(ctx);
    args.update_func_frame(frame);
    frame.finalize();

    a.emit_prolog(frame);
    a.emit_args_assignment(frame, args);
    for (int i = 0; i < count; ++i)
        if (!emit_arm_insn(a, ctx, prog[i], 4u * (uint32_t)i, frame)) // synthetic PC: 0, 4, 8, ...
            return 0;
    emit_return_imm(a, 0u);
    a.emit_epilog(frame);

    SelfTestBlockFn fn = nullptr;
    if (rt.add(&fn, &code) != kErrorOk)
        return 0;
    ArmCpuSelfTest cpu{};
    fn(&cpu);
    rt.release(fn);

    return (cpu.r[0] == 16 && cpu.r[1] == 256 && cpu.r[2] == 4 &&
            cpu.r[3] == 8  && cpu.r[4] == 48) ? 1 : 0;
}

// Translate flag-setting logical ops (S=1, immediate operand 2) and verify both
// the results and the resulting CPSR (N/Z; C preserved here as the immediates are un-rotated)
static int jit_asmjit_flags_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3B00000u, // MOVS r0,#0        -> r0=0,           Z=1, N=0
        0xE3F01000u, // MVNS r1,#0        -> r1=0xFFFFFFFF,  N=1, Z=0
        0xE211200Fu, // ANDS r2,r1,#0x0F  -> r2=0x0F,        N=0, Z=0
        0xE21030FFu, // ANDS r3,r0,#0xFF  -> r3=0,           Z=1, N=0  (final CPSR = Z)
    };
    const int count = (int)(sizeof(prog) / sizeof(prog[0]));

    JitRuntime rt;
    CodeHolder code;
    code.init(rt.environment());
    HostAssembler a(&code);
    FuncDetail func;
    func.init(FuncSignature::build<uint32_t, ArmCpuSelfTest *>(), code.environment());
    FuncFrame frame;
    frame.init(func);
    HostGp ctx = host_ctx_reg(a);
    host_add_block_regs(frame, a, ctx); // ctx + per-host callee-saved scratch
    FuncArgsAssignment args(&func);
    args.assign_all(ctx);
    args.update_func_frame(frame);
    frame.finalize();

    a.emit_prolog(frame);
    a.emit_args_assignment(frame, args);
    for (int i = 0; i < count; ++i)
        if (!emit_arm_insn(a, ctx, prog[i], 4u * (uint32_t)i, frame)) // synthetic PC: 0, 4, 8, ...
            return 0;
    emit_return_imm(a, 0u);
    a.emit_epilog(frame);

    SelfTestBlockFn fn = nullptr;
    if (rt.add(&fn, &code) != kErrorOk)
        return 0;
    ArmCpuSelfTest cpu{}; // CPSR starts 0
    fn(&cpu);
    rt.release(fn);

    return (cpu.r[0] == 0 && cpu.r[1] == 0xFFFFFFFFu && cpu.r[2] == 0x0Fu &&
            cpu.r[3] == 0 && cpu.cpsr == kZ) ? 1 : 0;
}

// Translate arithmetic flag-setting ops + compares and verify results and CPSR,
// including a carry/no-borrow case and a signed-overflow case
static int jit_asmjit_arith_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A0000Au, // MOV  r0,#10         -> r0=10 (no flags)
        0xE250100Au, // SUBS r1,r0,#10      -> r1=0;          Z=1, C=1 (no borrow), N=0, V=0
        0xE3500005u, // CMP  r0,#5          -> (no write)     C=1, N=0, Z=0, V=0
        0xE3E02102u, // MVN  r2,#0x80000000 -> r2=0x7FFFFFFF (no flags)
        0xE2923001u, // ADDS r3,r2,#1       -> r3=0x80000000; N=1, V=1, C=0, Z=0  (final CPSR)
    };
    const int count = (int)(sizeof(prog) / sizeof(prog[0]));

    JitRuntime rt;
    CodeHolder code;
    code.init(rt.environment());
    HostAssembler a(&code);
    FuncDetail func;
    func.init(FuncSignature::build<uint32_t, ArmCpuSelfTest *>(), code.environment());
    FuncFrame frame;
    frame.init(func);
    HostGp ctx = host_ctx_reg(a);
    host_add_block_regs(frame, a, ctx); // ctx + per-host callee-saved scratch
    FuncArgsAssignment args(&func);
    args.assign_all(ctx);
    args.update_func_frame(frame);
    frame.finalize();

    a.emit_prolog(frame);
    a.emit_args_assignment(frame, args);
    for (int i = 0; i < count; ++i)
        if (!emit_arm_insn(a, ctx, prog[i], 4u * (uint32_t)i, frame)) // synthetic PC: 0, 4, 8, ...
            return 0;
    emit_return_imm(a, 0u);
    a.emit_epilog(frame);

    SelfTestBlockFn fn = nullptr;
    if (rt.add(&fn, &code) != kErrorOk)
        return 0;
    ArmCpuSelfTest cpu{}; // CPSR starts 0
    fn(&cpu);
    rt.release(fn);

    return (cpu.r[0] == 10 && cpu.r[1] == 0 && cpu.r[2] == 0x7FFFFFFFu &&
            cpu.r[3] == 0x80000000u && cpu.cpsr == (kN | kV)) ? 1 : 0;
}

// Translate register-specified shift amounts and verify, including the ARM
// "amount >= 32 -> 0" edge case that the host's shift-count masking (x86 mod-32
// semantics, AArch64 variable shifts likewise) would get wrong
static int jit_asmjit_regshift_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00001u, // MOV r0,#1
        0xE3A01004u, // MOV r1,#4
        0xE1A02110u, // MOV r2, r0, LSL r1 -> r2 = 1<<4  = 16
        0xE3A03028u, // MOV r3,#40
        0xE1A04310u, // MOV r4, r0, LSL r3 -> r4 = 0     (amount 40 >= 32)
        0xE3A05002u, // MOV r5,#2
        0xE1A06531u, // MOV r6, r1, LSR r5 -> r6 = 4>>2 = 1
    };
    const int count = (int)(sizeof(prog) / sizeof(prog[0]));

    JitRuntime rt;
    CodeHolder code;
    code.init(rt.environment());
    HostAssembler a(&code);
    FuncDetail func;
    func.init(FuncSignature::build<uint32_t, ArmCpuSelfTest *>(), code.environment());
    FuncFrame frame;
    frame.init(func);
    HostGp ctx = host_ctx_reg(a);
    host_add_block_regs(frame, a, ctx); // ctx + per-host callee-saved scratch
    FuncArgsAssignment args(&func);
    args.assign_all(ctx);
    args.update_func_frame(frame);
    frame.finalize();

    a.emit_prolog(frame);
    a.emit_args_assignment(frame, args);
    for (int i = 0; i < count; ++i)
        if (!emit_arm_insn(a, ctx, prog[i], 4u * (uint32_t)i, frame)) // synthetic PC: 0, 4, 8, ...
            return 0;
    emit_return_imm(a, 0u);
    a.emit_epilog(frame);

    SelfTestBlockFn fn = nullptr;
    if (rt.add(&fn, &code) != kErrorOk)
        return 0;
    ArmCpuSelfTest cpu{};
    fn(&cpu);
    rt.release(fn);

    return (cpu.r[0] == 1 && cpu.r[1] == 4 && cpu.r[2] == 16 && cpu.r[3] == 40 &&
            cpu.r[4] == 0 && cpu.r[5] == 2 && cpu.r[6] == 1) ? 1 : 0;
}

// Translate the immediate #0 shift specials (LSR/ASR #32) and RRX, and verify.
// RRX reads the C flag, so the test pre-sets CPSR.C = 1
static int jit_asmjit_special_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00000u, // MOV r0,#0
        0xE1A01060u, // MOV r1, r0, RRX    -> (C<<31)|(0>>1) = 0x80000000   (C preset)
        0xE3A030FFu, // MOV r3,#0xFF
        0xE1A04023u, // MOV r4, r3, LSR #0 -> LSR #32 -> 0
        0xE1A06041u, // MOV r6, r1, ASR #0 -> ASR #32 of 0x80000000 -> 0xFFFFFFFF
    };
    const int count = (int)(sizeof(prog) / sizeof(prog[0]));

    JitRuntime rt;
    CodeHolder code;
    code.init(rt.environment());
    HostAssembler a(&code);
    FuncDetail func;
    func.init(FuncSignature::build<uint32_t, ArmCpuSelfTest *>(), code.environment());
    FuncFrame frame;
    frame.init(func);
    HostGp ctx = host_ctx_reg(a);
    host_add_block_regs(frame, a, ctx); // ctx + per-host callee-saved scratch
    FuncArgsAssignment args(&func);
    args.assign_all(ctx);
    args.update_func_frame(frame);
    frame.finalize();

    a.emit_prolog(frame);
    a.emit_args_assignment(frame, args);
    for (int i = 0; i < count; ++i)
        if (!emit_arm_insn(a, ctx, prog[i], 4u * (uint32_t)i, frame)) // synthetic PC: 0, 4, 8, ...
            return 0;
    emit_return_imm(a, 0u);
    a.emit_epilog(frame);

    SelfTestBlockFn fn = nullptr;
    if (rt.add(&fn, &code) != kErrorOk)
        return 0;
    ArmCpuSelfTest cpu{};
    cpu.cpsr = kC; // C set, for RRX
    fn(&cpu);
    rt.release(fn);

    return (cpu.r[0] == 0 && cpu.r[1] == 0x80000000u && cpu.r[3] == 0xFFu &&
            cpu.r[4] == 0 && cpu.r[6] == 0xFFFFFFFFu) ? 1 : 0;
}

// Translate ADC/SBC with carry-in (and an ADCS that sets flags) and verify.
// Mirrors a multi-word add/subtract: a carry-generating ADDS feeds ADC/SBC
static int jit_asmjit_adc_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3E00000u, // MVN  r0,#0    -> r0 = 0xFFFFFFFF
        0xE2901002u, // ADDS r1,r0,#2 -> r1 = 1; carry out -> C = 1
        0xE2A02000u, // ADC  r2,r0,#0 -> r2 = r0 + 0 + C = 0           (no-S; C stays 1)
        0xE2C05000u, // SBC  r5,r0,#0 -> r5 = r0 - 0 - !C = 0xFFFFFFFF (no-S; C stays 1)
        0xE2B04000u, // ADCS r4,r0,#0 -> r4 = r0 + 0 + C = 0; Z=1, C=1 (final CPSR)
    };
    const int count = (int)(sizeof(prog) / sizeof(prog[0]));

    JitRuntime rt;
    CodeHolder code;
    code.init(rt.environment());
    HostAssembler a(&code);
    FuncDetail func;
    func.init(FuncSignature::build<uint32_t, ArmCpuSelfTest *>(), code.environment());
    FuncFrame frame;
    frame.init(func);
    HostGp ctx = host_ctx_reg(a);
    host_add_block_regs(frame, a, ctx); // ctx + per-host callee-saved scratch
    FuncArgsAssignment args(&func);
    args.assign_all(ctx);
    args.update_func_frame(frame);
    frame.finalize();

    a.emit_prolog(frame);
    a.emit_args_assignment(frame, args);
    for (int i = 0; i < count; ++i)
        if (!emit_arm_insn(a, ctx, prog[i], 4u * (uint32_t)i, frame)) // synthetic PC: 0, 4, 8, ...
            return 0;
    emit_return_imm(a, 0u);
    a.emit_epilog(frame);

    SelfTestBlockFn fn = nullptr;
    if (rt.add(&fn, &code) != kErrorOk)
        return 0;
    ArmCpuSelfTest cpu{}; // CPSR starts 0
    fn(&cpu);
    rt.release(fn);

    return (cpu.r[0] == 0xFFFFFFFFu && cpu.r[1] == 1 && cpu.r[2] == 0 &&
            cpu.r[5] == 0xFFFFFFFFu && cpu.r[4] == 0 && cpu.cpsr == (kZ | kC)) ? 1 : 0;
}

// Translate flag-setting (S=1) ops with a register/shifted operand 2: arithmetic
// (incl. a shifted one) and a no-shift logical MOVS. Verifies results + CPSR
static int jit_asmjit_regop_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A0000Au, // MOV  r0,#10
        0xE3A01003u, // MOV  r1,#3
        0xE0502001u, // SUBS r2,r0,r1        -> r2=7;  C=1
        0xE1B04001u, // MOVS r4,r1           -> r4=3;  C preserved (no shift)
        0xE0905081u, // ADDS r5,r0,r1,LSL #1 -> r5=10+6=16; C=0
        0xE0503000u, // SUBS r3,r0,r0        -> r3=0;  Z=1, C=1  (final CPSR)
    };
    const int count = (int)(sizeof(prog) / sizeof(prog[0]));

    JitRuntime rt;
    CodeHolder code;
    code.init(rt.environment());
    HostAssembler a(&code);
    FuncDetail func;
    func.init(FuncSignature::build<uint32_t, ArmCpuSelfTest *>(), code.environment());
    FuncFrame frame;
    frame.init(func);
    HostGp ctx = host_ctx_reg(a);
    host_add_block_regs(frame, a, ctx); // ctx + per-host callee-saved scratch
    FuncArgsAssignment args(&func);
    args.assign_all(ctx);
    args.update_func_frame(frame);
    frame.finalize();

    a.emit_prolog(frame);
    a.emit_args_assignment(frame, args);
    for (int i = 0; i < count; ++i)
        if (!emit_arm_insn(a, ctx, prog[i], 4u * (uint32_t)i, frame)) // synthetic PC: 0, 4, 8, ...
            return 0;
    emit_return_imm(a, 0u);
    a.emit_epilog(frame);

    SelfTestBlockFn fn = nullptr;
    if (rt.add(&fn, &code) != kErrorOk)
        return 0;
    ArmCpuSelfTest cpu{}; // CPSR starts 0
    fn(&cpu);
    rt.release(fn);

    return (cpu.r[0] == 10 && cpu.r[1] == 3 && cpu.r[2] == 7 && cpu.r[3] == 0 &&
            cpu.r[4] == 3 && cpu.r[5] == 16 && cpu.cpsr == (kZ | kC)) ? 1 : 0;
}

// Translate LDR/STR/LDRB/STRB against the mock memory and verify word/byte,
// pre-index-with-offset, untouched-slot, and post-index writeback
static int jit_asmjit_ldrstr_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00010u, // MOV  r0,#16
        0xE3A010ABu, // MOV  r1,#0xAB
        0xE5801000u, // STR  r1,[r0]    -> mem.word[16] = 0xAB
        0xE5902000u, // LDR  r2,[r0]    -> r2 = 0xAB
        0xE3A0307Eu, // MOV  r3,#0x7E
        0xE5C03004u, // STRB r3,[r0,#4] -> mem.byte[20] = 0x7E
        0xE5D04004u, // LDRB r4,[r0,#4] -> r4 = 0x7E
        0xE5905008u, // LDR  r5,[r0,#8] -> r5 = 0 (untouched)
        0xE4906004u, // LDR  r6,[r0],#4 -> r6 = mem.word[16] = 0xAB; r0 := 20 (post-index)
    };
    const int count = (int)(sizeof(prog) / sizeof(prog[0]));

    JitRuntime rt;
    CodeHolder code;
    code.init(rt.environment());
    HostAssembler a(&code);
    FuncDetail func;
    func.init(FuncSignature::build<uint32_t, ArmCpuSelfTest *>(), code.environment());
    FuncFrame frame;
    frame.init(func);
    HostGp ctx = host_ctx_reg(a);
    host_add_block_regs(frame, a, ctx); // ctx + per-host callee-saved scratch
    FuncArgsAssignment args(&func);
    args.assign_all(ctx);
    args.update_func_frame(frame);
    frame.finalize();

    a.emit_prolog(frame);
    a.emit_args_assignment(frame, args);
    for (int i = 0; i < count; ++i)
        if (!emit_arm_insn(a, ctx, prog[i], 4u * (uint32_t)i, frame)) // synthetic PC: 0, 4, 8, ...
            return 0;
    emit_return_imm(a, 0u);
    a.emit_epilog(frame);

    SelfTestBlockFn fn = nullptr;
    if (rt.add(&fn, &code) != kErrorOk)
        return 0;
    ArmCpuSelfTest cpu{};
    fn(&cpu);
    rt.release(fn);

    return (cpu.r[0] == 20 && cpu.r[1] == 0xABu && cpu.r[2] == 0xABu && cpu.r[3] == 0x7Eu &&
            cpu.r[4] == 0x7Eu && cpu.r[5] == 0 && cpu.r[6] == 0xABu) ? 1 : 0;
}

// Translate LDR/STR with a register offset (plain, shifted, and post-index)
static int jit_asmjit_ldrstr_reg_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00010u, // MOV r0,#16
        0xE3A01004u, // MOV r1,#4
        0xE3A020CDu, // MOV r2,#0xCD
        0xE7802001u, // STR r2,[r0,r1]        -> mem.word[20] = 0xCD
        0xE7903001u, // LDR r3,[r0,r1]        -> r3 = 0xCD
        0xE3A04099u, // MOV r4,#0x99
        0xE7804081u, // STR r4,[r0,r1,LSL #1] -> mem.word[24] = 0x99
        0xE7905081u, // LDR r5,[r0,r1,LSL #1] -> r5 = 0x99
        0xE6906001u, // LDR r6,[r0],r1        -> r6 = mem.word[16] = 0; r0 := 20 (post-index)
    };
    const int count = (int)(sizeof(prog) / sizeof(prog[0]));

    JitRuntime rt;
    CodeHolder code;
    code.init(rt.environment());
    HostAssembler a(&code);
    FuncDetail func;
    func.init(FuncSignature::build<uint32_t, ArmCpuSelfTest *>(), code.environment());
    FuncFrame frame;
    frame.init(func);
    HostGp ctx = host_ctx_reg(a);
    host_add_block_regs(frame, a, ctx); // ctx + per-host callee-saved scratch
    FuncArgsAssignment args(&func);
    args.assign_all(ctx);
    args.update_func_frame(frame);
    frame.finalize();

    a.emit_prolog(frame);
    a.emit_args_assignment(frame, args);
    for (int i = 0; i < count; ++i)
        if (!emit_arm_insn(a, ctx, prog[i], 4u * (uint32_t)i, frame)) // synthetic PC: 0, 4, 8, ...
            return 0;
    emit_return_imm(a, 0u);
    a.emit_epilog(frame);

    SelfTestBlockFn fn = nullptr;
    if (rt.add(&fn, &code) != kErrorOk)
        return 0;
    ArmCpuSelfTest cpu{};
    fn(&cpu);
    rt.release(fn);

    return (cpu.r[0] == 20 && cpu.r[1] == 4 && cpu.r[2] == 0xCDu && cpu.r[3] == 0xCDu &&
            cpu.r[4] == 0x99u && cpu.r[5] == 0x99u && cpu.r[6] == 0) ? 1 : 0;
}

// Translate PC-relative transfers: a literal-pool LDR (PC base reads instr+8) and
// a STR of PC (stores instr+12). The block uses synthetic PCs 0,4,8,... so the
// computed addresses land inside the mock memory buffer
static int jit_asmjit_ldrstr_pc_selftest(void)
{
    static const uint32_t prog[] = {
        0xE59F0008u, // [pc=0]  LDR r0,[PC,#8] -> addr = 0+8+8 = 16; r0 = mem.word[16]
        0xE3A01020u, // [pc=4]  MOV r1,#32
        0xE581F000u, // [pc=8]  STR PC,[r1]    -> mem.word[32] = pc+12 = 20
        0xE5912000u, // [pc=12] LDR r2,[r1]    -> r2 = mem.word[32] = 20
    };
    const int count = (int)(sizeof(prog) / sizeof(prog[0]));

    JitRuntime rt;
    CodeHolder code;
    code.init(rt.environment());
    HostAssembler a(&code);
    FuncDetail func;
    func.init(FuncSignature::build<uint32_t, ArmCpuSelfTest *>(), code.environment());
    FuncFrame frame;
    frame.init(func);
    HostGp ctx = host_ctx_reg(a);
    host_add_block_regs(frame, a, ctx); // ctx + per-host callee-saved scratch
    FuncArgsAssignment args(&func);
    args.assign_all(ctx);
    args.update_func_frame(frame);
    frame.finalize();

    a.emit_prolog(frame);
    a.emit_args_assignment(frame, args);
    for (int i = 0; i < count; ++i)
        if (!emit_arm_insn(a, ctx, prog[i], 4u * (uint32_t)i, frame)) // synthetic PC: 0, 4, 8, ...
            return 0;
    emit_return_imm(a, 0u);
    a.emit_epilog(frame);

    SelfTestBlockFn fn = nullptr;
    if (rt.add(&fn, &code) != kErrorOk)
        return 0;
    ArmCpuSelfTest cpu{};
    cpu.mem[16] = 0x34; cpu.mem[17] = 0x12; // mem.word[16] = 0x1234 (little-endian)
    fn(&cpu);
    rt.release(fn);

    return (cpu.r[0] == 0x1234u && cpu.r[1] == 32 && cpu.r[2] == 20) ? 1 : 0;
}

// Build a one-shot block from an ARM program, run it against 'cpu', and return
// the block's next-PC result in 'retpc'. Shared by the control-flow self-tests
// (which exit the block early via a branch / PC write)
static bool run_block(const uint32_t *prog, int count, ArmCpuSelfTest &cpu, uint32_t &retpc)
{
    JitRuntime rt;
    CodeHolder code;
    code.init(rt.environment());
    HostAssembler a(&code);
    FuncDetail func;
    func.init(FuncSignature::build<uint32_t, ArmCpuSelfTest *>(), code.environment());
    FuncFrame frame;
    frame.init(func);
    HostGp ctx = host_ctx_reg(a);
    host_add_block_regs(frame, a, ctx);
    FuncArgsAssignment args(&func);
    args.assign_all(ctx);
    args.update_func_frame(frame);
    frame.finalize();

    a.emit_prolog(frame);
    a.emit_args_assignment(frame, args);
    for (int i = 0; i < count; ++i)
        if (!emit_arm_insn(a, ctx, prog[i], 4u * (uint32_t)i, frame))
            return false;
    emit_return_imm(a, 0xFFFFFFFFu); // fall-through sentinel (no branch taken)
    a.emit_epilog(frame);

    SelfTestBlockFn fn = nullptr;
    if (rt.add(&fn, &code) != kErrorOk)
        return false;
    retpc = fn(&cpu);
    rt.release(fn);
    return true;
}

// Translate B/BL: a not-taken BEQ (falls through), then a BL (sets LR, returns
// its target, and makes the following instruction dead)
static int jit_asmjit_branch_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00001u, // [pc=0]  MOV r0,#1
        0x0A000000u, // [pc=4]  BEQ . (Z=0 -> not taken, falls through)
        0xE3A01002u, // [pc=8]  MOV r1,#2
        0xEB00004Bu, // [pc=12] BL  -> target 0x140; LR = 16
        0xE3A02003u, // [pc=16] MOV r2,#3 (dead: after the taken BL)
    };
    ArmCpuSelfTest cpu{}; // CPSR = 0 (Z clear)
    uint32_t pc = 0;
    if (!run_block(prog, (int)(sizeof(prog) / sizeof(prog[0])), cpu, pc))
        return 0;
    return (pc == 0x140u && cpu.r[0] == 1 && cpu.r[1] == 2 && cpu.r[2] == 0 && cpu.r[14] == 16) ? 1 : 0;
}

// Translate the control-flow LDR/STR PC cases: (A) LDR into PC with post-index
// writeback (the classic return idiom), and (B) a PC-base writeback
static int jit_asmjit_ldrpc_selftest(void)
{
    // (A) LDR PC,[r0],#4 : PC = mem.word[r0]; r0 += 4.
    static const uint32_t progA[] = {
        0xE3A00010u, // MOV r0,#16
        0xE490F004u, // LDR PC,[r0],#4 -> PC = mem.word[16]; r0 := 20
        0xE3A01007u, // MOV r1,#7 (dead)
    };
    ArmCpuSelfTest cpuA{};
    cpuA.mem[16] = 0x00; cpuA.mem[17] = 0x03; // mem.word[16] = 0x300
    uint32_t pcA = 0;
    if (!run_block(progA, 3, cpuA, pcA))                       return 0;
    if (!(pcA == 0x300u && cpuA.r[0] == 20 && cpuA.r[1] == 0)) return 0;

    // (B) LDR r1,[PC],#4 : PC base reads instr+8; post-index writeback writes PC.
    static const uint32_t progB[] = {
        0xE49F1004u, // [pc=0] LDR r1,[PC],#4 -> r1 = mem.word[8]; PC := (0+8)+4 = 12
        0xE3A02009u, // MOV r2,#9 (dead)
    };
    ArmCpuSelfTest cpuB{};
    cpuB.mem[8] = 0x55;  // mem.word[8] = 0x55
    uint32_t pcB = 0;
    if (!run_block(progB, 2, cpuB, pcB)) return 0;
    return (pcB == 12u && cpuB.r[1] == 0x55u && cpuB.r[2] == 0) ? 1 : 0;
}

// Translate data-processing writes to PC: (A) a computed ADD PC,Rn,#imm jump, and
// (B) a not-taken MOVEQ PC then a MOV PC,LR return
static int jit_asmjit_dppc_selftest(void)
{
    // (A) ADD PC, r1, #0x40  -> PC = 0x80 + 0x40 = 0xC0
    static const uint32_t progA[] = {
        0xE3A00007u, // MOV r0,#7
        0xE3A01080u, // MOV r1,#0x80
        0xE281F040u, // ADD PC,r1,#0x40   -> returns 0xC0
        0xE3A02009u, // MOV r2,#9 (dead)
    };
    ArmCpuSelfTest cpuA{};
    uint32_t pcA = 0;
    if (!run_block(progA, 4, cpuA, pcA)) return 0;
    if (!(pcA == 0xC0u && cpuA.r[0] == 7 && cpuA.r[1] == 0x80u && cpuA.r[2] == 0)) return 0;

    // (B) MOVEQ PC,r14 (Z=0 -> not taken) then MOV PC,r14 (return)
    static const uint32_t progB[] = {
        0xE3A0EC02u, // MOV r14,#0x200
        0x01A0F00Eu, // MOVEQ PC,r14   (Z=0 -> not taken, falls through)
        0xE3A03055u, // MOV r3,#0x55
        0xE1A0F00Eu, // MOV PC,r14     -> returns 0x200
        0xE3A04001u, // MOV r4,#1 (dead)
    };
    ArmCpuSelfTest cpuB{};  // CPSR = 0 (Z clear)
    uint32_t pcB = 0;
    if (!run_block(progB, 5, cpuB, pcB)) return 0;
    return (pcB == 0x200u && cpuB.r[14] == 0x200u && cpuB.r[3] == 0x55u && cpuB.r[4] == 0) ? 1 : 0;
}

// Translate halfword/signed transfers and verify zero- vs sign-extension
static int jit_asmjit_halfword_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00010u, // MOV   r0,#16
        0xE3A010ABu, // MOV   r1,#0xAB
        0xE1C010B0u, // STRH  r1,[r0]    -> mem.half[16] = 0x00AB
        0xE1D020B0u, // LDRH  r2,[r0]    -> r2 = 0x00AB (zero-extended)
        0xE3A03080u, // MOV   r3,#0x80
        0xE5C03004u, // STRB  r3,[r0,#4] -> mem.byte[20] = 0x80
        0xE1D040D4u, // LDRSB r4,[r0,#4] -> r4 = 0xFFFFFF80 (sign-extended)
        0xE3A05C80u, // MOV   r5,#0x8000
        0xE1C050B8u, // STRH  r5,[r0,#8] -> mem.half[24] = 0x8000
        0xE1D060F8u, // LDRSH r6,[r0,#8] -> r6 = 0xFFFF8000 (sign-extended)
    };
    ArmCpuSelfTest cpu{};
    uint32_t pc = 0;
    if (!run_block(prog, (int)(sizeof(prog) / sizeof(prog[0])), cpu, pc))
        return 0;
    return (cpu.r[0] == 16 && cpu.r[1] == 0xABu && cpu.r[2] == 0xABu && cpu.r[3] == 0x80u &&
            cpu.r[4] == 0xFFFFFF80u && cpu.r[5] == 0x8000u && cpu.r[6] == 0xFFFF8000u) ? 1 : 0;
}

// Translate LDM/STM: (A) a STMIA!/LDMIA! round-trip with writeback, and (B) an
// LDM that loads PC (the return idiom) -> block exit
static int jit_asmjit_ldmstm_selftest(void)
{
    // (A) STMIA r0!,{r1,r2,r3} then LDMIA r0!,{r4,r5,r6}
    static const uint32_t progA[] = {
        0xE3A00040u, // MOV r0,#0x40
        0xE3A01011u, // MOV r1,#0x11
        0xE3A02022u, // MOV r2,#0x22
        0xE3A03033u, // MOV r3,#0x33
        0xE8A0000Eu, // STMIA r0!,{r1,r2,r3} -> mem[0x40,0x44,0x48]; r0 = 0x4C
        0xE3A00040u, // MOV r0,#0x40
        0xE8B00070u, // LDMIA r0!,{r4,r5,r6} -> r4=0x11,r5=0x22,r6=0x33; r0 = 0x4C
    };
    ArmCpuSelfTest cpuA{};
    uint32_t pcA = 0;
    if (!run_block(progA, 7, cpuA, pcA)) return 0;
    if (!(cpuA.r[4] == 0x11u && cpuA.r[5] == 0x22u && cpuA.r[6] == 0x33u && cpuA.r[0] == 0x4Cu))
        return 0;

    // (B) LDMIA r0!,{r2,pc} : r2 = mem[0x40]; PC = mem[0x44]; r0 = 0x48.
    static const uint32_t progB[] = {
        0xE3A00040u, // MOV r0,#0x40
        0xE8B08004u, // LDMIA r0!,{r2,pc}  -> r2 = mem.word[0x40]; returns mem.word[0x44]
    };
    ArmCpuSelfTest cpuB{};
    cpuB.mem[0x40] = 0x66; // mem.word[0x40] = 0x66  (-> r2)
    cpuB.mem[0x45] = 0x04; // mem.word[0x44] = 0x400 (-> PC)
    uint32_t pcB = 0;
    if (!run_block(progB, 2, cpuB, pcB)) return 0;
    return (pcB == 0x400u && cpuB.r[2] == 0x66u && cpuB.r[0] == 0x48u) ? 1 : 0;
}

// BX Rm: returns Rm as the next PC UNMASKED (interpreter parity), setting the
// CPSR T bit when bit 0 is set (Thumb -- unimplemented in this core, parity only)
static int jit_asmjit_bx_selftest(void)
{
    static const uint32_t progA[] = {
        0xE3A00041u, // MOV r0,#0x41 (odd)
        0xE12FFF10u, // BX r0
    };
    ArmCpuSelfTest cpuA{};
    uint32_t pcA = 0;
    if (!run_block(progA, 2, cpuA, pcA)) return 0;
    if (!(pcA == 0x41u && (cpuA.cpsr & 0x20u) != 0u)) return 0; // unmasked + T set
    static const uint32_t progB[] = {
        0xE3A01040u, // MOV r1,#0x40 (even)
        0xE12FFF11u, // BX r1
    };
    ArmCpuSelfTest cpuB{};
    uint32_t pcB = 0;
    if (!run_block(progB, 2, cpuB, pcB)) return 0;
    return (pcB == 0x40u && (cpuB.cpsr & 0x20u) == 0u) ? 1 : 0;
}

// R15 as a DP source operand: Rn==15 and Rm==15 read the pipelined PC --
// pc+8, or pc+12 with a register-specified shift amount (interpreter
// HandleALU addpc / decodeShift). Blocks start at pc 0, step 4
static int jit_asmjit_dpsrcpc_selftest(void)
{
    static const uint32_t prog[] = {
        0xE28F0004u, // [pc 0]  ADD r0,r15,#4        -> r0 = (0+8)+4  = 12
        0xE1A0100Fu, // [pc 4]  MOV r1,r15           -> r1 = 4+8      = 12
        0xE08F208Fu, // [pc 8]  ADD r2,r15,r15,LSL#1 -> r2 = 16+(16<<1) = 48
        0xE3A04000u, // [pc 12] MOV r4,#0
        0xE1A0341Fu, // [pc 16] MOV r3,r15,LSL r4    -> reg-amount: r3 = 16+12 = 28
        0xE04F5410u, // [pc 20] SUB r5,r15,r0,LSL r4 -> rn = 20+12; r5 = 32-12 = 20
    };
    ArmCpuSelfTest cpu{};
    uint32_t pc = 0;
    if (!run_block(prog, (int)(sizeof(prog) / sizeof(prog[0])), cpu, pc)) return 0;
    return (cpu.r[0] == 12u && cpu.r[1] == 12u && cpu.r[2] == 48u &&
            cpu.r[3] == 28u && cpu.r[5] == 20u) ? 1 : 0;
}

// Flag-setting logical ops with a SHIFTED register operand 2: C = the shifter
// carry-out (LSL/LSR/ROR immediate amounts, the LSR #0 = #32 special, and RRX)
static int jit_asmjit_shiftcarry_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A01003u, // MOV  r1,#3
        0xE1B000A1u, // MOVS r0,r1,LSR #1  -> r0 = 1;          C = bit0(3)  = 1
        0xE1B02F81u, // MOVS r2,r1,LSL #31 -> r2 = 0x80000000; C = bit1(3)  = 1, N=1
        0xE0113121u, // ANDS r3,r1,r1,LSR #2 -> r3 = 3&0 = 0;  C = bit1(3)  = 1, Z=1
        0xE1B040E1u, // MOVS r4,r1,ROR #1  -> r4 = 0x80000001; C = bit0(3)  = 1, N=1
        0xE1B05021u, // MOVS r5,r1,LSR #0  -> (= #32) r5 = 0;  C = bit31(3) = 0, Z=1
        0xE1B06061u, // MOVS r6,r1,RRX     -> C in = 0 -> r6 = 3>>1 = 1; C = bit0(3) = 1
    };
    ArmCpuSelfTest cpu{}; // CPSR starts 0
    uint32_t pc = 0;
    if (!run_block(prog, (int)(sizeof(prog) / sizeof(prog[0])), cpu, pc)) return 0;
    return (cpu.r[0] == 1u && cpu.r[2] == 0x80000000u && cpu.r[3] == 0u &&
            cpu.r[4] == 0x80000001u && cpu.r[5] == 0u && cpu.r[6] == 1u &&
            cpu.cpsr == kC) ? 1 : 0; // final: C=1 (RRX), N=0, Z=0
}

// MUL/MLA, including flag-setting: N/Z from the result, C preserved (the CPSR
// is preset with C so a legacy-JIT-style "C := 0" would be caught)
static int jit_asmjit_mul_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00006u, // MOV  r0,#6
        0xE3A01007u, // MOV  r1,#7
        0xE0020190u, // MUL  r2,r0,r1     -> r2 = 42
        0xE0232190u, // MLA  r3,r0,r1,r2  -> r3 = 6*7 + 42 = 84
        0xE3A04000u, // MOV  r4,#0
        0xE0150194u, // MULS r5,r4,r1     -> r5 = 0;  Z=1, N=0 (C preserved)
        0xE3E06000u, // MVN  r6,#0        -> r6 = -1
        0xE0170196u, // MULS r7,r6,r1     -> r7 = -7; N=1, Z=0 (C preserved)
    };
    ArmCpuSelfTest cpu{};
    cpu.cpsr = kC; // must survive both MULS
    uint32_t pc = 0;
    if (!run_block(prog, (int)(sizeof(prog) / sizeof(prog[0])), cpu, pc)) return 0;
    return (cpu.r[2] == 42u && cpu.r[3] == 84u && cpu.r[5] == 0u &&
            cpu.r[7] == 0xFFFFFFF9u && cpu.cpsr == (kC | kN)) ? 1 : 0;
}

// UMULL/SMULL/UMLAL, including S=1 on a negative 64-bit result: N = bit 63,
// Z = (64-bit == 0), C and V preserved (both preset)
static int jit_asmjit_mull_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00801u, // MOV   r0,#0x10000
        0xE3A01801u, // MOV   r1,#0x10000
        0xE0832190u, // UMULL r2,r3,r0,r1   -> r3:r2 = 0x1'00000000 (r2=0, r3=1)
        0xE3E04000u, // MVN   r4,#0         -> r4 = -1
        0xE0C65194u, // SMULL r5,r6,r4,r1   -> r6:r5 = -0x10000
        0xE3A07001u, // MOV   r7,#1
        0xE3A08002u, // MOV   r8,#2
        0xE0A87190u, // UMLAL r7,r8,r0,r1   -> r8:r7 = 0x2'00000001 + 0x1'00000000 = r7=1, r8=3
        0xE0DA9194u, // SMULLS r9,r10,r4,r1 -> r10:r9 = -0x10000; N=1, Z=0 (C,V preserved)
    };
    ArmCpuSelfTest cpu{};
    cpu.cpsr = kC | kV; // must survive the SMULLS
    uint32_t pc = 0;
    if (!run_block(prog, (int)(sizeof(prog) / sizeof(prog[0])), cpu, pc)) return 0;
    return (cpu.r[2] == 0u && cpu.r[3] == 1u &&
            cpu.r[5] == 0xFFFF0000u && cpu.r[6] == 0xFFFFFFFFu &&
            cpu.r[7] == 1u && cpu.r[8] == 3u &&
            cpu.r[9] == 0xFFFF0000u && cpu.r[10] == 0xFFFFFFFFu &&
            cpu.cpsr == (kC | kV | kN)) ? 1 : 0;
}

// Real-build partial translation: translate consecutive instructions in
// defer-memory mode, stopping at the first one that can't be translated (a memory
// op, or anything unsupported). The block returns the PC of that instruction so
// the interpreter resumes there. This mirrors what arm7_asmjit_xlat will do
// against the real register file; 'translated' returns how many were JITted
static bool run_block_partial(const uint32_t *prog, int count, ArmCpuSelfTest &cpu,
                              uint32_t &retpc, int &translated)
{
    JitRuntime rt;
    CodeHolder code;
    code.init(rt.environment());
    HostAssembler a(&code);
    FuncDetail func;
    func.init(FuncSignature::build<uint32_t, ArmCpuSelfTest *>(), code.environment());
    FuncFrame frame;
    frame.init(func);
    HostGp ctx = host_ctx_reg(a);
    host_add_block_regs(frame, a, ctx);
    FuncArgsAssignment args(&func);
    args.assign_all(ctx);
    args.update_func_frame(frame);
    frame.finalize();

    a.emit_prolog(frame);
    a.emit_args_assignment(frame, args);
    s_memMode = MemMode::Defer; // no callbacks: memory ops end the block
    int i = 0;
    for (; i < count; ++i)
        if (!emit_arm_insn(a, ctx, prog[i], 4u * (uint32_t)i, frame))
            break;
    s_memMode = MemMode::Mock; // restore default
    emit_return_imm(a, 4u * (uint32_t)i); // resume PC = first untranslated instruction
    a.emit_epilog(frame);
    translated = i;

    SelfTestBlockFn fn = nullptr;
    if (rt.add(&fn, &code) != kErrorOk)
        return false;
    retpc = fn(&cpu);
    rt.release(fn);
    return true;
}

// Verify the partial-JIT contract the integration relies on: register/branch ops
// translate, a memory op ends the block (deferred to the interpreter), and the
// block returns the resume PC (Register layout = real ARM7 sArmRegister)
static int jit_asmjit_realmode_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00003u, // [pc=0] MOV r0,#3
        0xE2801004u, // [pc=4] ADD r1,r0,#4 -> r1 = 7
        0xE5902000u, // [pc=8] LDR r2,[r0]  -> memory: deferred -> block stops, returns pc=8
        0xE3A03009u, // [pc=12] MOV r3,#9   -> not translated
    };
    ArmCpuSelfTest cpu{};
    uint32_t pc = 0;
    int n = 0;
    if (!run_block_partial(prog, 4, cpu, pc, n))
        return 0;
    return (n == 2 && pc == 8u && cpu.r[0] == 3 && cpu.r[1] == 7 &&
            cpu.r[2] == 0 && cpu.r[3] == 0) ? 1 : 0;
}


// ===========================================================================
//  asmjit JIT controller
//
//  A self-contained parallel to the legacy x86 jit_ctl: an address->block map
//  plus a JitRuntime that owns the generated code. Blocks are functions
//  `uint32_t block(void* ctx)` returning the next emulated PC; ctx is
//  &ARM7.sArmRegister[0] in the real build. The exec loop calls arm7_aj_get(pc):
//  it returns a cached block, translates one on demand, or returns nullptr (run
//  the instruction in the interpreter). Translation is partial: unsupported
//  instructions end the block and hand back to the interpreter; in the live build
//  memory ops emit calls to the real read/write thunks (Callback mode)
// ===========================================================================

typedef arm7_block_fn ArmBlockFn; // == uint32_t (*)(void*); matches jit_asmjit.h

// Translate the block at 'pc' (instructions fetched via 'fetch'). Returns the
// block fn and *count = #instructions translated, or nullptr if none.
// 'isBlockStart' (optional): translation additionally stops when it reaches an
// address that already STARTS a cached block, instead of duplicating that
// block's code as an overlapping suffix -- the chaining dispatcher hops into
// the existing block at run time (Existing blocks are never split, so a later
// branch INTO the middle of this block can still create an overlap; the probe
// only stops duplication from spreading forward)
static ArmBlockFn translate_block(JitRuntime &rt, const MemCallbacks &cb, uint32_t pc,
                                  uint32_t (*fetch)(uint32_t), int &count, int &cycles,
                                  int (*isBlockStart)(const void *u, uint32_t addr) = nullptr,
                                  void *user = nullptr, int *picount = nullptr)
{
    CodeHolder code;
    code.init(rt.environment());
    HostAssembler a(&code);
    FuncDetail func;
    func.init(FuncSignature::build<uint32_t, void *>(), code.environment());
    FuncFrame frame;
    frame.init(func);
    HostGp ctx = host_ctx_reg(a);
    host_add_block_regs(frame, a, ctx);
    FuncArgsAssignment args(&func);
    args.assign_all(ctx);
    args.update_func_frame(frame);

    // If real memory callbacks are set, the block CALLS them: reserve outgoing
    // call-stack space (Win64 shadow / x86 args) and keep the stack 16-aligned at
    // calls. Must be declared before finalize() so the prolog accounts for it
    const bool useCallbacks = (cb.r32 != nullptr);
    if (useCallbacks) {
        frame.set_call_stack_size(32);
        frame.update_call_stack_alignment(16);
    }
    frame.finalize();

    a.emit_prolog(frame);
    a.emit_args_assignment(frame, args);
    s_memcb   = cb;
    s_picount = picount;  // enables in-block cycle charging at every exit
    s_memMode = useCallbacks ? MemMode::Callback : MemMode::Defer;
    int i = 0, cyc = 0;
    while (i < 256) {  // bound block size
        const uint32_t at   = pc + 4u * (uint32_t)i;
        if (i > 0 && isBlockStart && isBlockStart(user, at))
            break; // 'at' already starts a cached block: join it via the dispatcher instead of duplicating its code
        const uint32_t insn = fetch(at);
        const int      cost = arm_insn_cycles(insn); // legacy-JIT cycle model (see arm_insn_cycles)
        // pass the path cost INCLUDING this instruction: any exit it emits
        // charges exactly the instructions executed up to that point
        if (!emit_arm_insn(a, ctx, insn, at, frame, cyc + cost))
            break;
        cyc += cost;
        ++i;
        // End the block at ANY control transfer (basic-block boundary), conditional
        // or not. If a conditional branch isn't taken at run time, the trailing
        // trailing "return next-pc" exit below returns the fall-through PC and that
        // address becomes its own block -- so instructions after a *taken* branch
        // are never wrongly cycle-counted. (A not-taken conditional branch is
        // charged 1, not its full cost: the skip-path refund in emit_arm_insn)
        if (arm_insn_ends_block(insn))
            break;
    }
    // trailing exit: charge the whole straight-line path, return the fall-through PC
    // (must be emitted BEFORE the statics are restored below)
    emit_charge_cycles(a, cyc);
    emit_return_imm(a, pc + 4u * (uint32_t)i); // resume PC = first untranslated instruction
    a.emit_epilog(frame);
    s_memMode = MemMode::Mock; // restore default (self-tests run direct, assume Mock)
    s_memcb   = MemCallbacks{};
    s_picount = nullptr;
    count  = i;
    cycles = cyc;
    if (i == 0)
        return nullptr; // first instruction unsupported -> emulate it
    ArmBlockFn fn = nullptr;
    if (rt.add(&fn, &code) != kErrorOk)
        return nullptr;
    return fn;
}

// One record per LIVE translated block, held in ArmAsmjitCtl::blocks. The
// per-opcode slot array stores only a 4-byte handle into this table, so the
// big allocation costs 4 bytes per opcode address (like the legacy JIT's
// native map) instead of a fn pointer + metadata per address; only the few
// thousand blocks a game actually translates pay for a BlockInfo
struct BlockInfo {
    ArmBlockFn fn;      // entry point (nullptr = entry recycled, on the free list)
    uint16_t   count;   // #instructions the block covers
    uint16_t   cyc;     // straight-line cycle total (informational: blocks charge
                        // ARM7_ICOUNT themselves at their exits with the executed
                        // path's exact cost -- see emit_charge_cycles)
};

struct ArmAsmjitCtl {
    JitRuntime  rt;
    uint32_t    minAddr = 0, maxAddr = 0;
    int         slots = 0;
    // Per opcode slot, a 4-byte handle: AJ_SLOT_UNTRIED (never looked at),
    // AJ_SLOT_INTERP (don't translate: nothing translatable starts here), or
    // blockId + AJ_SLOT_BASE indexing 'blocks'
    uint32_t   *slot = nullptr;
    static const uint32_t AJ_SLOT_UNTRIED = 0;
    static const uint32_t AJ_SLOT_INTERP  = 1;
    static const uint32_t AJ_SLOT_BASE    = 2;
    std::vector<BlockInfo> blocks;  // dense table of translated blocks (see BlockInfo)
    std::vector<uint32_t>  freeIds; // recycled 'blocks' indices (untranslated entries)
    // Raw mirror of blocks.data(), for the DISPATCHER (generated code cannot
    // chase a std::vector across reallocations). Updated after every mutation of
    // 'blocks'; all mutation happens on the C side, never during a chain
    BlockInfo  *blocksPtr = nullptr;
    // Block chaining (see aj_emit_dispatcher / arm7_aj_run): a JIT-emitted
    // dispatcher that CALLS cached blocks in a loop without returning to the C
    // exec loop between them. Emitted lazily; dropped (re-emitted) when a value
    // it embeds changes (picount, pIrqFlag). The stale code stays in the
    // JitRuntime until delete -- a few hundred bytes, not worth releasing
    void       *dispatcher = nullptr;
    int        *picount = nullptr; // &ARM7_ICOUNT (stable global), wired via arm7_aj_set_icount
    int         chainEnabled = 1;  // arm7_aj_set_chaining kill switch (A/B timing, bisection)
    // Per-slot count of live blocks COVERING this opcode address (their span
    // [start, start+4*count) includes it, not just starting at it) -- so a
    // store can detect a write into the MIDDLE of a translated block, which
    // the block-start slot alone cannot (the legacy JIT caught these via its
    // per-instruction native map). 0 on almost every store = the O(1) fast
    // path; nonzero triggers the bounded covering-block scan in
    // arm7_aj_untranslate. Saturates at 255 (then sticky: never decremented;
    // needs 255+ overlapping blocks on one word -- unreachable in practice)
    uint8_t    *cover = nullptr;
    int         maxBlockInsns = 0;  // longest block translated since reset (bounds the scan)
    MemCallbacks mem = {};          // real read/write thunks + abort/IRQ hooks
    int         enabled = 1;        // gate: when 0, arm7_aj_get returns null (interpreter). The AT91
                                    // driver disables until the RAM->page-0 remap, mirroring the
                                    // legacy jit_enable, so only the final post-remap program is JITted
    int         execOk = 1;         // host grants executable JIT memory (probed once at create).
                                    // 0 (e.g. iOS W^X denial) pins 'enabled' at 0: interpreter-only
    // DEBUG exclusion LIST: PCs in any [exclLo[i], exclHi[i]) are interpreted, not
    // JITted, even inside [minAddr,maxAddr). Lets a build JIT everything EXCEPT chosen
    // regions -- a debug aid for isolating a suspect region. (arm7_aj_clear_exclude /
    // arm7_aj_add_exclude)
    static const int AJ_MAX_EXCL = 32;
    uint32_t    exclLo[AJ_MAX_EXCL] = {};
    uint32_t    exclHi[AJ_MAX_EXCL] = {};
    int         exclN = 0;
    // Blocks retired by untranslate/reset whose code memory has NOT been freed
    // yet. rt.release() must never run while a block might be executing: the
    // store thunks call arm7_aj_untranslate from INSIDE a running block (SMC
    // write), so a block that stores to its own start address would otherwise
    // free the very code it is executing (use-after-free on return from the
    // thunk); arm7_aj_reset is likewise reachable from a write thunk (AT91 EBI
    // remap). Retired blocks are freed by aj_release_retired() at the next
    // arm7_aj_get() -- the exec loop's lookup, which never runs with generated
    // code on the host stack. (The legacy JIT was immune by construction: it
    // patched code in place and never freed it.)
    std::vector<ArmBlockFn> retired;
};

// Free the code of all retired blocks. ONLY call from a point where no
// generated code can be on the host stack: arm7_aj_get (the exec loop's
// between-blocks lookup) and the init-time aj_alloc_slots. NOT from
// arm7_aj_untranslate / arm7_aj_reset -- those are reachable from the write
// thunks a running block calls (see 'retired' above)
static void aj_release_retired(ArmAsmjitCtl *c)
{
    for (size_t i = 0; i < c->retired.size(); ++i)
        c->rt.release(c->retired[i]);
    c->retired.clear();
}

// (Re)allocate the per-opcode slot array for [minAddr, maxAddr). Releases any
// existing blocks/array first. slots may be 0 (an empty controller). Used by
// both create (initial) and set_range (reconfigure-in-place, pointer unchanged)
static void aj_alloc_slots(ArmAsmjitCtl *c, uint32_t minAddr, uint32_t maxAddr)
{
    // Init-time only (create / set_range), never called from generated code, so
    // freeing here is safe -- including anything retired earlier and not yet drained
    for (size_t i = 0; i < c->blocks.size(); ++i)
        if (c->blocks[i].fn) c->rt.release(c->blocks[i].fn);
    c->blocks.clear();
    c->freeIds.clear();
    c->blocksPtr = nullptr;
    aj_release_retired(c);
    delete[] c->slot;
    delete[] c->cover;
    c->dispatcher = nullptr; // embeds minAddr/maxAddr/slot base -> re-emit with the new values
    c->minAddr = minAddr;
    c->maxAddr = maxAddr;
    c->slots   = (maxAddr > minAddr) ? (int)((maxAddr - minAddr) >> 2) : 0;
    c->slot    = new uint32_t[c->slots]();  // zero = AJ_SLOT_UNTRIED
    c->cover   = new uint8_t[c->slots]();   // zero = not covered by any block
    c->maxBlockInsns = 0;
}

// One-shot probe: can this process map and execute JIT memory at all? iOS
// denies W^X to third-party apps (no dynamic-codesigning entitlement; a
// debugger-attached process CAN, which this probe then detects and allows),
// making asmjit fail at rt.add(). Probing once at create keeps the reaction
// explicit and cheap: the controller comes up permanently disabled, so no
// address ever pays a futile decode-emit-fail cycle -- the interpreter simply
// runs from the start. On every other supported host this succeeds
static bool aj_probe_exec_memory(JitRuntime &rt)
{
    CodeHolder code;
    code.init(rt.environment());
    HostAssembler a(&code);
#if AJ_HOST_X86
    a.mov(x86::eax, 0);
    a.ret();
#else
    a.mov(a64::w0, 0);
    a.ret(a64::x30);
#endif
    void *fn = nullptr;
    if (rt.add(&fn, &code) != kErrorOk)
        return false;
    rt.release(fn);
    return true;
}

extern "C" ArmAsmjitCtl *arm7_aj_create(uint32_t minAddr, uint32_t maxAddr)
{
    ArmAsmjitCtl *c = new ArmAsmjitCtl();
    c->execOk  = aj_probe_exec_memory(c->rt) ? 1 : 0;
    c->enabled = c->execOk;
    aj_alloc_slots(c, minAddr, maxAddr);
    return c;
}

// Reconfigure an EXISTING controller's address range in place (the pointer does
// NOT change). This matters for context-saved state: the controller pointer is
// created early in arm7_core_init (so it's captured in MAME's CPU-context
// snapshot and survives set_context), then the range is set here from the
// CPU-specific init (at91_init_jit) once the opcode span is known -- mirroring how
// the legacy jit pointer is created early and jit_create_map configures it later
extern "C" void arm7_aj_set_range(ArmAsmjitCtl *c, uint32_t minAddr, uint32_t maxAddr)
{
    if (!c) return;
    aj_alloc_slots(c, minAddr, maxAddr);
}

// DEBUG exclusion list: PCs in any added [lo, hi) are interpreted even inside
// [minAddr, maxAddr). Debug aid to JIT everything EXCEPT one or more suspect
// regions. Adding/clearing drops cached blocks
extern "C" void arm7_aj_clear_exclude(ArmAsmjitCtl *c)
{
    if (!c) return;
    c->exclN = 0;
    arm7_aj_reset(c);
}
extern "C" void arm7_aj_add_exclude(ArmAsmjitCtl *c, uint32_t lo, uint32_t hi)
{
    if (!c || lo >= hi || c->exclN >= ArmAsmjitCtl::AJ_MAX_EXCL) return;
    c->exclLo[c->exclN] = lo;
    c->exclHi[c->exclN] = hi;
    c->exclN++;
    arm7_aj_reset(c);
}

// Enable/disable translation. While disabled, arm7_aj_get returns null so the
// interpreter runs. The AT91 driver keeps it disabled until the RAM->page-0 remap
// (mirroring the legacy jit_enable), so the JIT only ever sees the final, stable
// post-remap program -- never the throwaway, self-modifying boot/copy/remap code
extern "C" void arm7_aj_set_enabled(ArmAsmjitCtl *c, int enabled)
{
    if (!c) return;
    // execOk pins the gate: if the create-time probe found no executable
    // memory (iOS W^X), the AT91 driver's post-remap enable must not stick
    c->enabled = (enabled && c->execOk) ? 1 : 0;
}

// Set the real memory read/write thunks the JIT calls for all memory transfers
// (LDR/STR, halfword, LDM/STM). With these set, translate_block runs in Callback
// mode; without them, memory ops end the block and the interpreter handles the
// access. Changing the callbacks discards any blocks translated under the old set
extern "C" void arm7_aj_set_mem_callbacks(ArmAsmjitCtl *c,
        arm7_aj_read_fn r8, arm7_aj_read_fn r16, arm7_aj_read_fn r32,
        arm7_aj_write_fn w8, arm7_aj_write_fn w16, arm7_aj_write_fn w32)
{
    if (!c) return;
    c->mem.r8  = (const void *)r8;  c->mem.r16 = (const void *)r16; c->mem.r32 = (const void *)r32;
    c->mem.w8  = (const void *)w8;  c->mem.w16 = (const void *)w16; c->mem.w32 = (const void *)w32;
    arm7_aj_reset(c);
}

// Wire the abort/IRQ hooks (legacy gen_test_abort/gen_test_irq parity). check_irq
// is the emulator's arm7_check_irq_state; pAbtD/pIrq/pFiq are the addresses of the
// pending-flag bytes used as the run-time guard. After a memory access a translated
// block consults these to vector a data abort / FIQ / IRQ raised by the access.
// Unset (self-tests only; the real build always wires them via arm7_aj_wire_mem),
// no post-check is emitted, so an exception raised synchronously BY a JITted memory
// access is DELAYED until the next external raise or interpreted LDR/STR check --
// externally raised IRQs are unaffected. Discards blocks translated under the old hooks
extern "C" void arm7_aj_set_irq_hooks(ArmAsmjitCtl *c,
        const void *check_irq, const void *pAbtD, const void *pIrq, const void *pFiq,
        const void *pIrqFlag)
{
    if (!c) return;
    c->mem.check_irq = check_irq;
    c->mem.pAbtD = pAbtD; c->mem.pIrq = pIrq; c->mem.pFiq = pFiq;
    c->mem.pIrqFlag = pIrqFlag;
    c->dispatcher = nullptr; // embeds pIrqFlag -> re-emit with the new value
    arm7_aj_reset(c);
}

// Wire the emulator's HandlePSRTransfer so translated blocks can execute
// MRS/MSR by calling it with the raw instruction word (exact interpreter
// semantics -- see emit_arm_psr). Unset, PSR transfers end the block and run
// interpreted. Discards blocks translated under the old value
extern "C" void arm7_aj_set_psr_transfer(ArmAsmjitCtl *c, const void *psr_transfer)
{
    if (!c) return;
    c->mem.psr_transfer = psr_transfer;
    arm7_aj_reset(c);
}

// Wire the emulator's exception-return helper (uint32_t fn(uint32_t newpc)) so
// translated blocks can execute MOVS PC / SUBS PC,LR,... -- the IRQ/exception
// exit -- with exact interpreter semantics (SPSR->CPSR, bank switch, PC mask,
// ARM7_CHECKIRQ; see emit_arm_insn). Unset, those defer to the interpreter.
// Discards blocks translated under the old value
extern "C" void arm7_aj_set_exc_return(ArmAsmjitCtl *c, const void *exc_return)
{
    if (!c) return;
    c->mem.exc_return = exc_return;
    arm7_aj_reset(c);
}

extern "C" void arm7_aj_reset(ArmAsmjitCtl *c)
{
    if (!c) return;
    // Retire, don't free -- reachable from a write thunk mid-block (see 'retired')
    for (size_t i = 0; i < c->blocks.size(); ++i)
        if (c->blocks[i].fn) c->retired.push_back(c->blocks[i].fn);
    c->blocks.clear();
    c->freeIds.clear();
    c->blocksPtr = nullptr;
    if (c->slots) {
        memset(c->slot, 0, (size_t)c->slots * sizeof(uint32_t)); // all AJ_SLOT_UNTRIED
        memset(c->cover, 0, (size_t)c->slots);
    }
    c->maxBlockInsns = 0;
}

extern "C" void arm7_aj_delete(ArmAsmjitCtl *c)
{
    if (!c) return;
    delete[] c->slot;
    delete[] c->cover;
    delete c; // JitRuntime destructor frees all generated code (live and retired)
}

// Retire the block starting at slot 'idx' (slot value must be a block handle):
// un-count its coverage span, move its code to the deferred-release list, and
// recycle its table entry. Retire, don't free -- reachable from the store
// thunks inside a running block; a block storing into itself must not free the
// code it is executing (see 'retired')
static void aj_retire_block_at(ArmAsmjitCtl *c, int idx)
{
    const uint32_t id = c->slot[idx] - ArmAsmjitCtl::AJ_SLOT_BASE;
    BlockInfo &bi = c->blocks[id];
    int end = idx + (int)bi.count;
    if (end > c->slots) end = c->slots;
    for (int i = idx; i < end; ++i)
        if (c->cover[i] && c->cover[i] != 0xFF) c->cover[i]--; // 0xFF = saturated, sticky
    c->retired.push_back(bi.fn);
    bi.fn = nullptr; // recycle the table entry
    c->freeIds.push_back(id);
    c->slot[idx] = ArmAsmjitCtl::AJ_SLOT_UNTRIED;
}

// Invalidate EVERY translated block covering 'addr' (self-modifying code) --
// including blocks the written address is in the MIDDLE of, matching what the
// legacy JIT achieved with its per-instruction native map. The common case
// (store to an address no block covers) is a single byte test. A covering
// block can start at most maxBlockInsns-1 slots back, which bounds the scan.
// NB: an AJ_SLOT_INTERP mark is deliberately NOT cleared by a write (legacy
// philosophy: a written location will likely be written again, so don't keep
// retrying translation there; the interpreter executes it correctly either way)
extern "C" void arm7_aj_untranslate(ArmAsmjitCtl *c, uint32_t addr)
{
    if (!c || addr < c->minAddr || addr >= c->maxAddr) return;
    const int idx = (int)((addr - c->minAddr) >> 2);
    if (!c->cover[idx]) return; // fast path: no live block covers this address
    int lo = idx - (c->maxBlockInsns - 1);
    if (lo < 0) lo = 0;
    for (int j = idx; j >= lo; --j) {
        const uint32_t v = c->slot[j];
        if (v >= ArmAsmjitCtl::AJ_SLOT_BASE &&
            (int)c->blocks[v - ArmAsmjitCtl::AJ_SLOT_BASE].count > idx - j)
            aj_retire_block_at(c, j); // spans across 'addr' -> stale
    }
}

// translate_block probe: does 'addr' already start a cached block? (Stops
// translation there so the dispatcher chains into it -- see translate_block)
static int aj_is_block_start(const void *u, uint32_t addr)
{
    const ArmAsmjitCtl *c = (const ArmAsmjitCtl *)u;
    if (addr < c->minAddr || addr >= c->maxAddr) return 0;
    return c->slot[(addr - c->minAddr) >> 2] >= ArmAsmjitCtl::AJ_SLOT_BASE;
}

// Look up (and translate on demand) the block at 'pc'. Returns the block fn, or
// nullptr to run 'pc' in the interpreter. *outCount = #instructions the block
// covers; *outCycles = emulated cycles it consumes (subtract from ARM7_ICOUNT)
extern "C" ArmBlockFn arm7_aj_get(ArmAsmjitCtl *c, uint32_t pc, uint32_t (*fetch)(uint32_t), int *outCount, int *outCycles)
{
    if (outCount) *outCount = 0;
    if (outCycles) *outCycles = 0;
    if (!c) return nullptr;
    // Safe point to free retired blocks: this lookup runs from the exec loop
    // between blocks, never with generated code on the host stack (see 'retired')
    if (!c->retired.empty()) aj_release_retired(c);
    if (!c->enabled || pc < c->minAddr || pc >= c->maxAddr) return nullptr;
    for (int ei = 0; ei < c->exclN; ++ei) // DEBUG exclusion holes
        if (pc >= c->exclLo[ei] && pc < c->exclHi[ei]) return nullptr;
    const int idx = (int)((pc - c->minAddr) >> 2);
    const uint32_t v = c->slot[idx];
    if (v >= ArmAsmjitCtl::AJ_SLOT_BASE) { // cached block
        const BlockInfo &bi = c->blocks[v - ArmAsmjitCtl::AJ_SLOT_BASE];
        if (outCount)  *outCount  = bi.count;
        if (outCycles) *outCycles = bi.cyc;
        return bi.fn;
    }
    if (v == ArmAsmjitCtl::AJ_SLOT_INTERP) return nullptr;
    int cnt = 0, cyc = 0;
    ArmBlockFn fn = translate_block(c->rt, c->mem, pc, fetch, cnt, cyc, aj_is_block_start, c, c->picount);
    if (fn) {
        BlockInfo bi;
        bi.fn    = fn;
        bi.count = (uint16_t)(cnt > 0xFFFF ? 0xFFFF : cnt);
        bi.cyc   = (uint16_t)(cyc > 0xFFFF ? 0xFFFF : cyc);
        uint32_t id;
        if (!c->freeIds.empty()) { id = c->freeIds.back(); c->freeIds.pop_back(); c->blocks[id] = bi; }
        else                     { id = (uint32_t)c->blocks.size(); c->blocks.push_back(bi); }
        c->blocksPtr = c->blocks.data(); // keep the dispatcher's raw mirror in sync
        c->slot[idx] = id + ArmAsmjitCtl::AJ_SLOT_BASE;
        // Count the block's whole span as covered so a store into its MIDDLE is
        // detected by arm7_aj_untranslate (clamped: a block may run past maxAddr)
        int end = idx + cnt;
        if (end > c->slots) end = c->slots;
        for (int i = idx; i < end; ++i)
            if (c->cover[i] != 0xFF) c->cover[i]++;
        if (cnt > c->maxBlockInsns) c->maxBlockInsns = cnt;
        if (outCount)  *outCount  = cnt;
        if (outCycles) *outCycles = cyc;
        return fn;
    }
    c->slot[idx] = ArmAsmjitCtl::AJ_SLOT_INTERP; // nothing translatable here; don't retry
    return nullptr;
}

// ===========================================================================
//  Block chaining: JIT-emitted dispatcher (see arm7_aj_run)
//
//  A small generated function that CALLS cached blocks in a loop -- lookup the
//  next pc, charge its cycles, check the budget/IRQ flag, call, repeat --
//  without returning to the C exec loop between blocks (which costs an
//  arm7_aj_get round trip per transition). Blocks are completely unchanged:
//  they are ordinary callees here, so every invariant (deferred release, SMC
//  invalidation via the write thunks, post-check exits) holds by construction.
//  The dispatcher never translates: any miss bails back to C with the pc, the
//  exec loop translates via arm7_aj_run/arm7_aj_get, and the next run chains
//  through the new block.
//
//  Mutable controller state ('enabled', the icount value, slot CONTENTS,
//  'blocksPtr') is read through stable addresses EVERY hop, so reset/retire
//  stay safe mid-chain. Values that only change at init time -- 'picount',
//  'pIrqFlag', and (since they are set exclusively by aj_alloc_slots) the
//  minAddr/maxAddr bounds and the slot array BASE -- are embedded as
//  constants; every site that changes one drops the dispatcher so it re-emits
// ===========================================================================

typedef uint32_t (*ArmDispatchFn)(void *ctx, uint32_t pc);

static void aj_emit_dispatcher(ArmAsmjitCtl *c)
{
    if (c->dispatcher || !c->picount) return;
    CodeHolder code;
    code.init(c->rt.environment());
    HostAssembler a(&code);
    FuncDetail func;
    func.init(FuncSignature::build<uint32_t, void *, uint32_t>(), code.environment());
    FuncFrame frame;
    frame.init(func);
    HostGp ctx = host_ctx_reg(a);
#if AJ_HOST_A64
    // pc lives in w20: callee-saved (AAPCS64) so it survives the block calls;
    // x30 (LR) is dirtied by every blr and must be saved/restored by the frame
    a64::Gp pc32 = a64::w20, pcFull = a64::x20;
    frame.add_dirty_regs(ctx, pcFull, a64::x30);
    FuncArgsAssignment args(&func);
    args.assign_all(ctx, pc32);
    args.update_func_frame(frame);
    frame.set_call_stack_size(32); // we call blocks
    frame.update_call_stack_alignment(16);
    frame.finalize();

    a.emit_prolog(frame);
    a.emit_args_assignment(frame, args);

    Label loop = a.new_label();
    Label out  = a.new_label();
    a.bind(loop);
    // enabled? (defensive: all current disable sites also reset, clearing the slots)
    a.mov(a64::x16, (uint64_t)(uintptr_t)&c->enabled);
    a.ldr(a64::w17, a64::ptr(a64::x16));
    a.cbz(a64::w17, out);
    // icount > 0? -- the C loop's while (ARM7_ICOUNT > 0), checked before each block
    a.mov(a64::x16, (uint64_t)(uintptr_t)c->picount);
    a.ldr(a64::w17, a64::ptr(a64::x16));
    a.cmp(a64::w17, 0);
    a.b(arm::CondCode::kSignedLE, out);
    // pc within [minAddr, maxAddr)? Constants, same drop-and-re-emit protocol as x86
    a.mov(a64::w16, c->minAddr);
    a.cmp(pc32, a64::w16);
    a.b(arm::CondCode::kUnsignedLT, out);              // below minAddr
    a.mov(a64::w16, c->maxAddr);
    a.cmp(pc32, a64::w16);
    a.b(arm::CondCode::kUnsignedGE, out);              // at/above maxAddr
    // v = slot[(pc - minAddr) >> 2]
    a.mov(a64::w16, c->minAddr);
    a.sub(a64::w0, pc32, a64::w16);
    a.lsr(a64::w0, a64::w0, 2);                        // idx (w-write zero-extends x0)
    a.mov(a64::x16, (uint64_t)(uintptr_t)c->slot);     // slot array base (embedded)
    a.ldr(a64::w0, a64::ptr(a64::x16, a64::x0, a64::lsl(2))); // v = slot[idx]
    a.cmp(a64::w0, ArmAsmjitCtl::AJ_SLOT_BASE);
    a.b(arm::CondCode::kUnsignedLT, out);              // UNTRIED / INTERP -> C loop
    a.sub(a64::w0, a64::w0, ArmAsmjitCtl::AJ_SLOT_BASE); // block id
    // x16 = &blocksPtr[id], then fn
    a.mov(a64::w17, (uint32_t)sizeof(BlockInfo));
    a.mul(a64::w0, a64::w0, a64::w17);                 // byte offset (zero-extends x0)
    a.mov(a64::x16, (uint64_t)(uintptr_t)&c->blocksPtr);
    a.ldr(a64::x16, a64::ptr(a64::x16));
    a.add(a64::x16, a64::x16, a64::x0);
    a.ldr(a64::x16, a64::ptr(a64::x16, (int)offsetof(BlockInfo, fn)));
    // no cycle charge here: blocks charge at their exits (see the x86 twin's note)
    a.mov(a64::x0, ctx);                               // call fn(ctx)
    a.blr(a64::x16);
    a.mov(pc32, a64::w0);                              // next emulated pc
    // post-check exit? the block set the irq flag: the C loop must vector NOW
    if (c->mem.pIrqFlag) {
        a.mov(a64::x16, (uint64_t)(uintptr_t)c->mem.pIrqFlag);
        a.ldrb(a64::w17, a64::ptr(a64::x16));
        a.cbnz(a64::w17, out);
    }
    a.b(loop);
    a.bind(out);
    a.mov(a64::w0, pc32);
    a.emit_epilog(frame);

    ArmDispatchFn fn = nullptr;
    if (c->rt.add(&fn, &code) == kErrorOk)
        c->dispatcher = (void *)fn;
#else
#if defined(_WIN64) || defined(__x86_64__)
    // pc must survive the block calls: r12 is callee-saved in BOTH x64 ABIs and
    // never used by generated blocks (rsi would be clobbered on SysV, where it is an argument register)
    x86::Gp pc32 = x86::r12d, pcFull = x86::r12;
#else
    // cdecl: esi is callee-saved; blocks use it but save/restore it (dirty reg)
    x86::Gp pc32 = x86::esi, pcFull = x86::esi;
#endif
    frame.add_dirty_regs(ctx, pcFull, a.zax());
    FuncArgsAssignment args(&func);
    args.assign_all(ctx, pc32);
    args.update_func_frame(frame);
    frame.set_call_stack_size(32); // we call blocks
    frame.update_call_stack_alignment(16);
    frame.finalize();

    a.emit_prolog(frame);
    a.emit_args_assignment(frame, args);

    using CC = x86::CondCode;
    Label loop = a.new_label();
    Label out  = a.new_label();
    a.bind(loop);
    // enabled? (defensive: all current disable sites also reset, clearing the slots)
    emit_mov_ptr(a, a.zcx(), &c->enabled);
    a.cmp(x86::dword_ptr(a.zcx()), 0);
    a.j(CC::kZero, out);
    // icount > 0? -- the C loop's while (ARM7_ICOUNT > 0), checked before each block
    emit_mov_ptr(a, a.zcx(), c->picount);
    a.cmp(x86::dword_ptr(a.zcx()), 0);
    a.j(CC::kSignedLE, out);
    // pc within [minAddr, maxAddr)? Bounds and the slot array base are embedded
    // as CONSTANTS: they change only in aj_alloc_slots (init-time create /
    // set_range), which drops this dispatcher so it re-emits with fresh values
    a.cmp(pc32, c->minAddr);
    a.j(CC::kCarry, out);                                 // below minAddr
    a.cmp(pc32, c->maxAddr);
    a.j(CC::kNotCarry, out);                              // at/above maxAddr
    // v = slot[(pc - minAddr) >> 2]
    a.mov(x86::eax, pc32);
    a.sub(x86::eax, c->minAddr);
    a.shr(x86::eax, 2);                                   // idx (zero-extends zax)
    emit_mov_ptr(a, a.zdx(), c->slot);                    // slot array base (embedded)
    a.mov(x86::eax, x86::ptr(a.zdx(), a.zax(), 2, 0, 4)); // v = slot[idx]
    a.cmp(x86::eax, ArmAsmjitCtl::AJ_SLOT_BASE);
    a.j(CC::kCarry, out);                                 // UNTRIED / INTERP -> C loop
    a.sub(x86::eax, ArmAsmjitCtl::AJ_SLOT_BASE);          // block id
    // zdx = &blocksPtr[id]
    a.imul(x86::eax, x86::eax, (int)sizeof(BlockInfo));
    emit_mov_ptr(a, a.zdx(), &c->blocksPtr);
    a.mov(a.zdx(), x86::ptr(a.zdx()));
    a.add(a.zdx(), a.zax());
    // no cycle charge here: blocks charge ARM7_ICOUNT themselves at their exits
    // with the executed path's cost (see emit_charge_cycles); the budget check
    // above still gates each hop like the C loop's while (ICOUNT > 0)
    // call fn(ctx)
    a.mov(a.zdx(), x86::ptr(a.zdx(), (int)offsetof(BlockInfo, fn)));
#if defined(_WIN64)
    a.mov(x86::rcx, ctx);
#elif defined(__x86_64__)
    a.mov(x86::rdi, ctx);
#else
    a.mov(x86::dword_ptr(x86::esp, 0), ctx);
#endif
    a.call(a.zdx());
    a.mov(pc32, x86::eax); // next emulated pc
    // post-check exit? the block set the irq flag: the C loop must vector NOW
    // (the flag is consumed there, not here)
    if (c->mem.pIrqFlag) {
        emit_mov_ptr(a, a.zdx(), c->mem.pIrqFlag);
        a.cmp(x86::byte_ptr(a.zdx()), 0);
        a.j(CC::kNotZero, out);
    }
    a.jmp(loop);
    a.bind(out);
    a.mov(x86::eax, pc32);
    a.emit_epilog(frame);

    ArmDispatchFn fn = nullptr;
    if (c->rt.add(&fn, &code) == kErrorOk)
        c->dispatcher = (void *)fn;
#endif // AJ_HOST_X86
}

// Wire the emulator's cycle counter (&ARM7_ICOUNT, a stable global). Required:
// arm7_aj_run charges cycles internally (chained or not), so without this it
// reports "no block" and the interpreter runs. Resets the controller and drops
// the dispatcher (which embeds this address)
extern "C" void arm7_aj_set_icount(ArmAsmjitCtl *c, int *picount)
{
    if (!c) return;
    c->picount = picount;
    c->dispatcher = nullptr;
    arm7_aj_reset(c);
}

// Block-chaining kill switch (default on) -- for A/B timing comparisons and
// bring-up bisection. Off, arm7_aj_run executes exactly one block per call,
// like the pre-chaining exec loop
extern "C" void arm7_aj_set_chaining(ArmAsmjitCtl *c, int enabled)
{
    if (!c) return;
    c->chainEnabled = enabled ? 1 : 0;
}

// Exec-loop entry point: execute translated code starting at 'pc'. Returns 0
// if 'pc' has no translated block (caller interprets it); otherwise runs one
// block -- or, with chaining, as many cached blocks as the cycle budget allows
// -- charges ARM7_ICOUNT internally for everything it ran, stores the resume
// PC in *out_newpc, and returns 1. The caller must then honor the IRQ flag
// (arm7_aj_irq_flag) exactly as before: a post-check exit stops the chain so
// the flag is pending when this returns
extern "C" int arm7_aj_run(ArmAsmjitCtl *c, void *cpu_ctx, uint32_t pc,
                           uint32_t (*fetch)(uint32_t), uint32_t *out_newpc)
{
    if (!c || !c->picount) return 0; // exec env not wired -> JIT inactive
    int cnt = 0, cyc = 0;
    ArmBlockFn fn = arm7_aj_get(c, pc, fetch, &cnt, &cyc);
    if (!fn) return 0;
    // Run the first (already-resolved) block right here instead of having the
    // dispatcher re-look-it-up, then chain the REST starting from the PC it
    // returns. No cycle charge here: blocks charge ARM7_ICOUNT themselves at
    // their exits with the cost of the path actually executed (exact for early
    // abort/IRQ exits and skipped conditionals -- see emit_charge_cycles)
    uint32_t next = fn(cpu_ctx);
    // Chain blocks 2..N via the dispatcher -- unless chaining is off, the dispatcher
    // couldn't be emitted, or this block requested an IRQ check (arm7_aj_irq_flag): a
    // post-check exit ends the run so the caller vectors now. The fn() call is a
    // compiler barrier, so the flag load sees what the block wrote
    if (c->chainEnabled && (!c->mem.pIrqFlag || !*(const uint8_t *)c->mem.pIrqFlag)) {
        if (!c->dispatcher) aj_emit_dispatcher(c);
        if (c->dispatcher)
            next = ((ArmDispatchFn)c->dispatcher)(cpu_ctx, next);
    }
    *out_newpc = next;
    return 1;
}

// ---- callback-mode memory self-tests --------------------------------------
// A tiny test "memory" plus thunks matching arm7_aj_read_fn/arm7_aj_write_fn, so
// a translated block can exercise the real call path (emit_mem_call) in-process
namespace {
uint32_t g_memcb_words[128];                                       // 512-byte test memory (word view)
inline uint8_t* memcb_bytes()  { return (uint8_t*)g_memcb_words; } // byte view (char aliasing OK)
uint32_t memcb_r32(uint32_t a) { return g_memcb_words[(a & 0x1FFu) >> 2]; }
uint32_t memcb_r8 (uint32_t a) { return memcb_bytes()[a & 0x1FFu]; }
uint32_t memcb_r16(uint32_t a) { const uint8_t* b = memcb_bytes(); const uint32_t o = a & 0x1FFu;
                                 return (uint32_t)(b[o] | (b[o + 1] << 8)); } // little-endian half
void     memcb_w32(uint32_t a, uint32_t d) { g_memcb_words[(a & 0x1FFu) >> 2] = d; }
void     memcb_w8 (uint32_t a, uint32_t d) { memcb_bytes()[a & 0x1FFu] = (uint8_t)d; }
void     memcb_w16(uint32_t a, uint32_t d) { uint8_t* b = memcb_bytes(); const uint32_t o = a & 0x1FFu;
                                             b[o] = (uint8_t)d; b[o + 1] = (uint8_t)(d >> 8); } // little-endian half

// Mock PSR-transfer handler standing in for the emulator's HandlePSRTransfer
// (real signature: void fn(uint32_t insn)). Implements just enough of MRS/MSR
// on the CPSR to validate that emit_arm_psr passes the raw instruction word and
// that the block observes the CPSR change (no SPSR banking in the mock)
ArmCpuSelfTest *g_psr_cpu = nullptr; // the harness's cpu (set by run_block_cb)
void mock_psr(uint32_t insn)
{
    if (insn & 0x00200000u) {    // MSR
        uint32_t val;
        if (insn & (1u << 25)) { // rotated immediate
            const uint32_t rot = ((insn >> 8) & 0xFu) * 2u;
            const uint32_t v   = insn & 0xFFu;
            val = rot ? ((v >> rot) | (v << (32u - rot))) : v;
        } else {
            val = g_psr_cpu->r[insn & 0xFu];
        }
        uint32_t cpsr = g_psr_cpu->cpsr;
        if (insn & (1u << 19)) cpsr = (cpsr & 0x0FFFFFFFu) | (val & 0xF0000000u); // f field (flags)
        if (insn & (1u << 16)) cpsr = (cpsr & ~0xFFu) | (val & 0xFFu);            // c field (control)
        g_psr_cpu->cpsr = cpsr;
    } else { // MRS: Rd = CPSR
        g_psr_cpu->r[(insn >> 12) & 0xFu] = g_psr_cpu->cpsr;
    }
}

// Mock exception-return helper standing in for arm7_aj_exc_return (real
// signature: uint32_t fn(uint32_t newpc)). Marks the CPSR (stand-in for the
// SPSR restore) and masks the PC like the real helper, so the test can verify
// the JIT passes the computed PC in and returns the helper's result as the
// block's next PC
uint32_t mock_exret(uint32_t newpc) { g_psr_cpu->cpsr = 0xA5000010u; return newpc & ~3u; }

// Build and run a block from 'prog' in CALLBACK mode (the block emits
// real host calls to the memcb_* thunks above, against g_memcb_words). Returns
// false on a translate/codegen error. Exercises emit_mem_call's per-ABI shuffle
// and the FuncFrame call-stack reservation end-to-end on the host
bool run_block_cb(const uint32_t* prog, int count, ArmCpuSelfTest& cpu, uint32_t* retpc = nullptr)
{
    JitRuntime rt;
    CodeHolder code;
    code.init(rt.environment());
    HostAssembler a(&code);
    FuncDetail func;
    func.init(FuncSignature::build<uint32_t, ArmCpuSelfTest *>(), code.environment());
    FuncFrame frame;
    frame.init(func);
    HostGp ctx = host_ctx_reg(a);
    host_add_block_regs(frame, a, ctx);
    FuncArgsAssignment args(&func);
    args.assign_all(ctx);
    args.update_func_frame(frame);
    frame.set_call_stack_size(32); // the block makes calls -> reserve call stack
    frame.update_call_stack_alignment(16);
    frame.finalize();

    a.emit_prolog(frame);
    a.emit_args_assignment(frame, args);
    s_memcb   = MemCallbacks{ (const void *)&memcb_r8,  (const void *)&memcb_r16, (const void *)&memcb_r32,
                              (const void *)&memcb_w8,  (const void *)&memcb_w16, (const void *)&memcb_w32 };
    s_memcb.psr_transfer = (const void *)&mock_psr;   // MRS/MSR against the mock CPSR
    s_memcb.exc_return   = (const void *)&mock_exret; // MOVS/SUBS PC against the mock
    g_psr_cpu = &cpu;
    s_memMode = MemMode::Callback;
    bool ok = true;
    for (int i = 0; i < count; ++i)
        if (!emit_arm_insn(a, ctx, prog[i], 4u * (uint32_t)i, frame)) { ok = false; break; }
    s_memMode = MemMode::Mock;
    s_memcb   = MemCallbacks{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
    if (!ok) return false;
    emit_return_imm(a, 0u);
    a.emit_epilog(frame);

    SelfTestBlockFn fn = nullptr;
    if (rt.add(&fn, &code) != kErrorOk) return false;
    const uint32_t r = fn(&cpu);
    if (retpc) *retpc = r;
    rt.release(fn);
    return true;
}
} // namespace

// LDR/STR/LDRB/STRB in CALLBACK mode: word/byte load+store, an untouched slot, and a post-index writeback
static int jit_asmjit_memcb_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00010u, // MOV  r0,#16
        0xE3A010ABu, // MOV  r1,#0xAB
        0xE5801000u, // STR  r1,[r0]    -> word[16] = 0xAB
        0xE5902000u, // LDR  r2,[r0]    -> r2 = 0xAB
        0xE3A0307Eu, // MOV  r3,#0x7E
        0xE5C03004u, // STRB r3,[r0,#4] -> byte[20] = 0x7E
        0xE5D04004u, // LDRB r4,[r0,#4] -> r4 = 0x7E
        0xE5905008u, // LDR  r5,[r0,#8] -> r5 = word[24] = 0 (untouched)
        0xE4906004u, // LDR  r6,[r0],#4 -> r6 = word[16] = 0xAB; r0 := 20 (post-index)
    };
    for (int i = 0; i < 128; ++i) g_memcb_words[i] = 0;
    ArmCpuSelfTest cpu{};
    if (!run_block_cb(prog, (int)(sizeof(prog) / sizeof(prog[0])), cpu))
        return 0;
    return (cpu.r[0] == 20 && cpu.r[1] == 0xABu && cpu.r[2] == 0xABu && cpu.r[3] == 0x7Eu &&
            cpu.r[4] == 0x7Eu && cpu.r[5] == 0 && cpu.r[6] == 0xABu &&
            g_memcb_words[16 >> 2] == 0xABu && memcb_bytes()[20] == 0x7Eu) ? 1 : 0;
}

// LDRH/STRH/LDRSB/LDRSH in CALLBACK mode: zero- vs sign-extension via the real
// host calls (r16 for H/SH, r8 for SB; the emitter sign-extends SB/SH). Same
// program/expectations as the mock halfword test, but through the call path
static int jit_asmjit_memcb_half_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00010u, // MOV   r0,#16
        0xE3A010ABu, // MOV   r1,#0xAB
        0xE1C010B0u, // STRH  r1,[r0]    -> half[16] = 0x00AB
        0xE1D020B0u, // LDRH  r2,[r0]    -> r2 = 0x00AB (zero-extended)
        0xE3A03080u, // MOV   r3,#0x80
        0xE5C03004u, // STRB  r3,[r0,#4] -> byte[20] = 0x80
        0xE1D040D4u, // LDRSB r4,[r0,#4] -> r4 = 0xFFFFFF80 (sign-extended)
        0xE3A05C80u, // MOV   r5,#0x8000
        0xE1C050B8u, // STRH  r5,[r0,#8] -> half[24] = 0x8000
        0xE1D060F8u, // LDRSH r6,[r0,#8] -> r6 = 0xFFFF8000 (sign-extended)
    };
    for (int i = 0; i < 128; ++i) g_memcb_words[i] = 0;
    ArmCpuSelfTest cpu{};
    if (!run_block_cb(prog, (int)(sizeof(prog) / sizeof(prog[0])), cpu))
        return 0;
    return (cpu.r[0] == 16 && cpu.r[1] == 0xABu && cpu.r[2] == 0xABu && cpu.r[3] == 0x80u &&
            cpu.r[4] == 0xFFFFFF80u && cpu.r[5] == 0x8000u && cpu.r[6] == 0xFFFF8000u) ? 1 : 0;
}

// LDM/STM in CALLBACK mode: (A) STMIA then LDMIA round-trip with writeback (all
// in one block), and (B) LDMIA r0!,{r2,pc} - a call per register, writeback, and
// the loaded PC ending the block (return value)
static int jit_asmjit_memcb_ldm_selftest(void)
{
    // (A) store three regs, reload them; verify values, writeback, and the buffer
    static const uint32_t progA[] = {
        0xE3A00040u, // MOV   r0,#64
        0xE3A01011u, // MOV   r1,#0x11
        0xE3A02022u, // MOV   r2,#0x22
        0xE3A03033u, // MOV   r3,#0x33
        0xE8A0000Eu, // STMIA r0!,{r1,r2,r3} -> word[64,68,72]; r0 = 76
        0xE3A00040u, // MOV   r0,#64
        0xE8B00070u, // LDMIA r0!,{r4,r5,r6} -> r4=0x11,r5=0x22,r6=0x33; r0 = 76
    };
    for (int i = 0; i < 128; ++i) g_memcb_words[i] = 0;
    ArmCpuSelfTest cpuA{};
    if (!run_block_cb(progA, (int)(sizeof(progA) / sizeof(progA[0])), cpuA)) return 0;
    if (!(cpuA.r[0] == 76 && cpuA.r[4] == 0x11u && cpuA.r[5] == 0x22u && cpuA.r[6] == 0x33u &&
          g_memcb_words[64 >> 2] == 0x11u && g_memcb_words[68 >> 2] == 0x22u &&
          g_memcb_words[72 >> 2] == 0x33u)) return 0;

    // (B) LDM into PC ends the block: r2 loaded, PC = loaded word, r0 written back
    static const uint32_t progB[] = {
        0xE3A00080u, // MOV   r0,#128
        0xE3A01040u, // MOV   r1,#0x40
        0xE5801004u, // STR   r1,[r0,#4]  -> word[132] = 0x40 (the new PC)
        0xE3A02099u, // MOV   r2,#0x99
        0xE5802000u, // STR   r2,[r0]     -> word[128] = 0x99
        0xE8B08004u, // LDMIA r0!,{r2,pc} -> r2 = 0x99; PC = 0x40; r0 = 136
        0xE3A03001u, // MOV   r3,#1 (dead)
    };
    for (int i = 0; i < 128; ++i) g_memcb_words[i] = 0;
    ArmCpuSelfTest cpuB{};
    uint32_t pcB = 0;
    if (!run_block_cb(progB, (int)(sizeof(progB) / sizeof(progB[0])), cpuB, &pcB)) return 0;
    return (pcB == 0x40u && cpuB.r[2] == 0x99u && cpuB.r[0] == 136u && cpuB.r[3] == 0) ? 1 : 0;
}

namespace {
const uint32_t *g_test_prog = nullptr;
uint32_t        g_test_base = 0;
uint32_t test_fetch(uint32_t pc) { return g_test_prog[(pc - g_test_base) >> 2]; }
} // namespace

// Exercise the controller end-to-end: create, translate-on-demand, run the block,
// confirm the cache hit, invalidate (SMC) and re-translate, delete
static int jit_asmjit_controller_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00003u, // MOV r0,#3
        0xE2801004u, // ADD r1,r0,#4 -> r1 = 7
        0xE5902000u, // LDR r2,[r0]  -> memory: deferred -> block ends here
        0xE3A03009u, // MOV r3,#9
    };
    g_test_prog = prog;
    g_test_base = 0x1000u;

    ArmAsmjitCtl *c = arm7_aj_create(0x1000u, 0x2000u);
    int cnt = 0, cyc = 0;
    ArmBlockFn fn = arm7_aj_get(c, 0x1000u, test_fetch, &cnt, &cyc);
    bool ok = (fn != nullptr) && (cnt == 2) && (cyc == 2); // MOV(1) + ADD(1) = 2 cycles
    if (ok) {
        ArmCpuSelfTest cpu{};
        uint32_t resume = fn(&cpu);
        ok = (resume == 0x1008u) && (cpu.r[0] == 3) && (cpu.r[1] == 7) && (cpu.r[2] == 0);
    }
    // cache hit returns the same fn
    int cnt2 = 0, cyc2 = 0;
    ok = ok && (arm7_aj_get(c, 0x1000u, test_fetch, &cnt2, &cyc2) == fn) && (cnt2 == 2) && (cyc2 == 2);
    // SMC invalidation then re-translation
    arm7_aj_untranslate(c, 0x1000u);
    int cnt3 = 0, cyc3 = 0;
    ok = ok && (arm7_aj_get(c, 0x1000u, test_fetch, &cnt3, &cyc3) != nullptr) && (cnt3 == 2);
    arm7_aj_delete(c);
    return ok ? 1 : 0;
}

// Mid-block SMC invalidation: a write into the MIDDLE of a translated block
// must retire it (not just a write to its start), including when several
// overlapping blocks cover the written address; a write nothing covers is a no-op
static int jit_asmjit_smc_midblock_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00001u, // [0x1000] MOV r0,#1
        0xE3A01002u, // [0x1004] MOV r1,#2
        0xE3A02003u, // [0x1008] MOV r2,#3
        0xEAFFFFFEu, // [0x100C] B .
    };
    g_test_prog = prog;
    g_test_base = 0x1000u;
    ArmAsmjitCtl *c = arm7_aj_create(0x1000u, 0x2000u);
    int cnt = 0, cyc = 0;
    ArmBlockFn fnA = arm7_aj_get(c, 0x1000u, test_fetch, &cnt, &cyc); // covers 0x1000-0x100C
    bool ok = (fnA != nullptr) && (cnt == 4);
    // Overlapping block, as if 0x1008 were also a branch target (Overlap is only possible in this order --
    // a later block starting inside an earlier one; the reverse now JOINS instead, see step 4)
    ArmBlockFn fnB = arm7_aj_get(c, 0x1008u, test_fetch, &cnt, &cyc); // covers 0x1008-0x100C
    ok = ok && (fnB != nullptr) && (cnt == 2);
    // (1) write nothing covers: no-op
    arm7_aj_untranslate(c, 0x1100u);
    ok = ok && c->retired.empty();
    // (2) write covered by BOTH blocks (0x100C is in A's and B's span): both
    //     retired in one call
    arm7_aj_untranslate(c, 0x100Cu);
    ok = ok && (c->retired.size() == 2);
    // (3) both retranslate (the get drains the retired pair)
    ArmBlockFn fnB2 = arm7_aj_get(c, 0x1008u, test_fetch, &cnt, &cyc);
    ok = ok && (fnB2 != nullptr) && (cnt == 2) && c->retired.empty();
    // (4) 0x1000 now JOINS the cached 0x1008 block instead of duplicating it:
    //     2 instructions, not 4 (translate_block's isBlockStart probe)
    ok = ok && (arm7_aj_get(c, 0x1000u, test_fetch, &cnt, &cyc) != nullptr) && (cnt == 2);
    // (5) write into the middle of the prefix block only: it retires, B2 kept
    arm7_aj_untranslate(c, 0x1004u);
    ok = ok && (c->retired.size() == 1);
    ok = ok && (arm7_aj_get(c, 0x1008u, test_fetch, &cnt, &cyc) == fnB2);
    arm7_aj_delete(c);
    return ok ? 1 : 0;
}

// ---- deferred-release (SMC self-invalidation) self-test --------------------
// The real store thunks (arm7core.c arm7_aj_w8/16/32) call arm7_aj_untranslate
// after every write. A block that stores to its OWN start address therefore
// retires itself while it is still executing; its code must stay valid until
// the exec loop's next arm7_aj_get (deferred release -- see 'retired')
namespace {
ArmAsmjitCtl *g_smc_ctl = nullptr;
// w32 thunk mirroring the real arm7_aj_w32: perform the write, then invalidate
// the written address in the controller (from INSIDE the running block)
void smc_w32(uint32_t a, uint32_t d) { g_memcb_words[(a & 0x1FFu) >> 2] = d; arm7_aj_untranslate(g_smc_ctl, a); }
} // namespace

static int jit_asmjit_smc_selfinval_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00A01u, // [0x1000] MOV r0,#0x1000  (= this block's start address)
        0xE3A010ABu, // [0x1004] MOV r1,#0xAB
        0xE5801000u, // [0x1008] STR r1,[r0]     -> thunk untranslates THIS block mid-run
        0xE3A02055u, // [0x100C] MOV r2,#0x55    -> must still execute (code not freed yet)
        0xEAFFFFFEu, // [0x1010] B .             -> end of block
    };
    g_test_prog = prog;
    g_test_base = 0x1000u;
    for (int i = 0; i < 128; ++i) g_memcb_words[i] = 0;

    ArmAsmjitCtl *c = arm7_aj_create(0x1000u, 0x2000u);
    g_smc_ctl = c;
    arm7_aj_set_mem_callbacks(c, memcb_r8, memcb_r16, memcb_r32, memcb_w8, memcb_w16, smc_w32);
    int cnt = 0, cyc = 0;
    ArmBlockFn fn = arm7_aj_get(c, 0x1000u, test_fetch, &cnt, &cyc);
    bool ok = (fn != nullptr) && (cnt == 5) && (cyc == 1 + 1 + 2 + 1 + 3); // MOV+MOV+STR+MOV+B
    if (ok) {
        ArmCpuSelfTest cpu{};
        const uint32_t resume = fn(&cpu);
        // The store landed, the instructions AFTER the self-invalidating store
        // still ran (r2), the block returned its branch target, and the block
        // was retired (not freed) rather than released out from under itself.
        ok = (resume == 0x1010u) && (cpu.r[1] == 0xABu) && (cpu.r[2] == 0x55u)
             && (g_memcb_words[0] == 0xABu) && (c->retired.size() == 1);
    }
    // The next lookup is the safe point: it drains the retired block and
    // (slot was cleared) retranslates the address
    int cnt2 = 0, cyc2 = 0;
    ok = ok && (arm7_aj_get(c, 0x1000u, test_fetch, &cnt2, &cyc2) != nullptr)
            && (cnt2 == 5) && c->retired.empty();
    g_smc_ctl = nullptr;
    arm7_aj_delete(c);
    return ok ? 1 : 0;
}

// Verify the legacy-JIT cycle model: (1) per-instruction costs for each class,
// and (2) the per-block total + that translation stops at an unconditional
// branch (dead code after it is neither emitted nor cycle-counted)
static int jit_asmjit_cycles_selftest(void)
{
    if (arm_insn_cycles(0xE3A00001u) != 1) return 0; // MOV r0,#1            (DP = 1)
    if (arm_insn_cycles(0xE0810312u) != 2) return 0; // ADD r0,r1,r2,LSL r3  (reg-shift = 2)
    if (arm_insn_cycles(0xE1A0F00Eu) != 3) return 0; // MOV pc,r14           (DP->PC = 1+2)
    if (arm_insn_cycles(0xE5901000u) != 3) return 0; // LDR r1,[r0]          (load = 3)
    if (arm_insn_cycles(0xE5801000u) != 2) return 0; // STR r1,[r0]          (store = 2)
    if (arm_insn_cycles(0xE590F000u) != 5) return 0; // LDR pc,[r0]          (load PC = 5)
    if (arm_insn_cycles(0xEA000000u) != 3) return 0; // B .                  (branch = 3)
    if (arm_insn_cycles(0xE8BD8004u) != 6) return 0; // LDM {r2,pc}          (n=2 +2 +PC2 = 6)
    if (arm_insn_cycles(0xE8AD000Eu) != 4) return 0; // STM {r1,r2,r3}       (n=3 +1 = 4)
    if (arm_insn_cycles(0xE0020190u) != 5) return 0; // MUL r2,r0,r1         (1S+4I worst case; run-time refund)
    if (arm_insn_cycles(0xE0232190u) != 6) return 0; // MLA r3,r0,r1,r2      (+1I accumulate)
    if (arm_insn_cycles(0xE0832190u) != 6) return 0; // UMULL r2,r3,r0,r1    (1S+5I worst case)
    if (arm_insn_cycles(0xE0A87190u) != 7) return 0; // UMLAL r7,r8,r0,r1    (+1I accumulate)
    if (arm_insn_cycles(0xE10F1000u) != 1) return 0; // MRS r1,CPSR          (PSR = 1)
    if (arm_insn_cycles(0xE328F20Fu) != 1) return 0; // MSR CPSR_f,#imm      (PSR = 1)
    if (arm_insn_cycles(0xE129F004u) != 1) return 0; // MSR CPSR_fc,r4       (PSR = 1)
    if (arm_insn_cycles(0xE12FFF10u) != 3) return 0; // BX r0                (BX = 3)
    if (arm_insn_cycles(0xE1002091u) != 4) return 0; // SWP r2,r1,[r0]       (SWP = 4)
    if (arm_insn_cycles(0xE1404093u) != 4) return 0; // SWPB r4,r3,[r0]      (SWPB = 4)
    if (arm_insn_cycles(0xE25EF003u) != 3) return 0; // SUBS PC,LR,#3        (DP PC dest = 3)

    // Block: MOV(1) + MOV(1) + B(3) = 5 cycles, 3 instructions; the trailing MOV
    // after the unconditional B must be dropped (stop-at-branch), not counted
    static const uint32_t prog[] = {
        0xE3A00001u, // MOV r0,#1
        0xE3A01002u, // MOV r1,#2
        0xEA000000u, // B   .      (unconditional -> ends the block)
        0xE3A02009u, // MOV r2,#9  (dead: must not be translated or counted)
    };
    g_test_prog = prog;
    g_test_base = 0x1000u;
    ArmAsmjitCtl *c = arm7_aj_create(0x1000u, 0x2000u);
    int cnt = 0, cyc = 0;
    ArmBlockFn fn = arm7_aj_get(c, 0x1000u, test_fetch, &cnt, &cyc);
    bool ok = (fn != nullptr) && (cnt == 3) && (cyc == 5);
    arm7_aj_delete(c);
    if (!ok) return 0;

    // A CONDITIONAL branch is also a basic-block boundary: the block ends at it,
    // so the following instruction belongs to a separate block (not counted here)
    static const uint32_t progB[] = {
        0xE3A00001u, // MOV r0,#1   (1)
        0x0A000000u, // BEQ .       (3, conditional -> still ends the block)
        0xE3A01002u, // MOV r1,#2   (next block: not part of this one)
    };
    g_test_prog = progB;
    g_test_base = 0x2000u;
    ArmAsmjitCtl *cb = arm7_aj_create(0x2000u, 0x3000u);
    int cntB = 0, cycB = 0;
    ArmBlockFn fnB = arm7_aj_get(cb, 0x2000u, test_fetch, &cntB, &cycB);
    ok = (fnB != nullptr) && (cntB == 2) && (cycB == 4); // MOV(1) + BEQ(3), stops at the branch
    arm7_aj_delete(cb);
    return ok ? 1 : 0;
}

// SWP/SWPB in callback mode: LDR-then-STR through the real memory thunks
static int jit_asmjit_swap_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00020u, // MOV  r0,#0x20
        0xE3A010AAu, // MOV  r1,#0xAA
        0xE1002091u, // SWP  r2,r1,[r0]  -> r2 = old word; word[0x20] = 0xAA
        0xE3A03055u, // MOV  r3,#0x55
        0xE1404093u, // SWPB r4,r3,[r0]  -> r4 = old byte (0xAA); byte[0x20] = 0x55
    };
    const int n = (int)(sizeof(prog) / sizeof(prog[0]));
    for (int i = 0; i < 128; ++i) g_memcb_words[i] = 0;
    g_memcb_words[0x20 >> 2] = 0x11223377u;
    ArmCpuSelfTest cpu{};
    uint32_t pc = 0;
    if (!run_block_cb(prog, n, cpu, &pc)) return 0;
    return (cpu.r[2] == 0x11223377u && cpu.r[4] == 0xAAu &&
            g_memcb_words[0x20 >> 2] == 0x00000055u) ? 1 : 0;
}

// MOVS/SUBS PC (exception return) through the exc_return callback: the block
// computes the new PC flag-lessly, the helper "restores" the CPSR (mock marker),
// masks bits 1:0, and its return value becomes the block's returned PC
static int jit_asmjit_exret_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A0EF41u, // MOV  r14,#0x104
        0xE25EF003u, // SUBS PC,LR,#3 -> helper(0x101) -> returns 0x100, sets CPSR marker
    };
    ArmCpuSelfTest cpu{};
    cpu.cpsr = kZ; // overwritten by the mock's marker (stand-in for the SPSR restore)
    uint32_t pc = 0;
    if (!run_block_cb(prog, 2, cpu, &pc)) return 0;
    return (pc == 0x100u && cpu.cpsr == 0xA5000010u) ? 1 : 0;
}

// ---- LDM/STM memory-access ORDER -------------------------------------------
// Real ARM7 block transfers access memory in ASCENDING address order even for
// the "decrementing" DA/DB modes -- observable through memory-mapped IO.
// JIT emitter has always worked that way (and the interpreter's loadDec*/
// storeDec* were aligned to match); this test locks the contract by recording
// the actual write sequence of an STMDB
namespace {
uint32_t g_order_addr[8];
int      g_order_n = 0;
void order_w32(uint32_t a, uint32_t d) { if (g_order_n < 8) g_order_addr[g_order_n++] = a; memcb_w32(a, d); }
}

static int jit_asmjit_stm_order_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00040u, // MOV r0,#0x40
        0xE3A01011u, // MOV r1,#0x11
        0xE3A02022u, // MOV r2,#0x22
        0xE3A03033u, // MOV r3,#0x33
        0xE920000Eu, // STMDB r0!,{r1,r2,r3} -> 0x34/0x38/0x3C, WRITTEN low-to-high
        0xEAFFFFFEu, // B .
    };
    g_test_prog = prog;
    g_test_base = 0x1000u;
    for (int i = 0; i < 128; ++i) g_memcb_words[i] = 0;
    g_order_n = 0;
    ArmAsmjitCtl *c = arm7_aj_create(0x1000u, 0x2000u);
    arm7_aj_set_mem_callbacks(c, memcb_r8, memcb_r16, memcb_r32, memcb_w8, memcb_w16, order_w32);
    int cnt = 0, cyc = 0;
    ArmBlockFn fn = arm7_aj_get(c, 0x1000u, test_fetch, &cnt, &cyc);
    bool ok = (fn != nullptr);
    ArmCpuSelfTest cpu{};
    ok = ok && (fn(&cpu) == 0x1014u) && (cpu.r[0] == 0x34u);
    ok = ok && (g_order_n == 3)
            && (g_order_addr[0] == 0x34u) && (g_order_addr[1] == 0x38u) && (g_order_addr[2] == 0x3Cu)
            && (g_memcb_words[0x34 >> 2] == 0x11u) && (g_memcb_words[0x38 >> 2] == 0x22u)
            && (g_memcb_words[0x3C >> 2] == 0x33u);
    arm7_aj_delete(c);
    return ok ? 1 : 0;
}

// MRS/MSR through the psr_transfer callback: an MSR's CPSR change must be
// visible to condition evaluation LATER IN THE SAME BLOCK (conditions read the
// ctx CPSR the handler mutated), and MRS must read the CPSR back into a register
static int jit_asmjit_psr_selftest(void)
{
    static const uint32_t prog[] = {
        0xE328F20Fu, // MSR CPSR_f,#0xF0000000 -> N,Z,C,V all set
        0xE10F1000u, // MRS r1,CPSR            -> r1 = 0xF0000000
        0x03A02005u, // MOVEQ r2,#5            -> Z set -> executes
        0x43A03007u, // MOVMI r3,#7            -> N set -> executes
        0xE3A04000u, // MOV r4,#0
        0xE129F004u, // MSR CPSR_fc,r4         -> flags cleared (register form)
        0x03A05009u, // MOVEQ r5,#9            -> Z now clear -> skipped
    };
    const int n = (int)(sizeof(prog) / sizeof(prog[0]));
    ArmCpuSelfTest cpu{};
    uint32_t pc = 1;
    if (!run_block_cb(prog, n, cpu, &pc)) return 0;
    return (pc == 0u && cpu.r[1] == 0xF0000000u && cpu.r[2] == 5u &&
            cpu.r[3] == 7u && cpu.r[5] == 0u && cpu.cpsr == 0u) ? 1 : 0;
}

// ---- abort/IRQ mid-block exit self-test (gen_test_abort/gen_test_irq parity) ----
// Mock pending-flag bytes wired into emit_mem_post_checks, so a translated block can
// exercise the mid-block exit end-to-end on the host (no emulator call involved)
namespace {
uint8_t g_irq_pending = 0; // stands in for ARM7.pendingIrq
uint8_t g_irq_zero    = 0; // stands in for pendingAbtD / pendingFiq (kept 0 here)
uint8_t g_irq_flag    = 0; // stands in for arm7_aj_irq_flag (set by the block on exit)
// Build + run a Callback-mode block with the abort/IRQ guard wired to the mocks.
bool run_block_cb_irq(const uint32_t* prog, int count, ArmCpuSelfTest& cpu, uint32_t* retpc)
{
    JitRuntime rt; CodeHolder code; code.init(rt.environment());
    HostAssembler a(&code);
    FuncDetail func; func.init(FuncSignature::build<uint32_t, ArmCpuSelfTest *>(), code.environment());
    FuncFrame frame; frame.init(func);
    HostGp ctx = host_ctx_reg(a);
    host_add_block_regs(frame, a, ctx);
    FuncArgsAssignment args(&func); args.assign_all(ctx); args.update_func_frame(frame);
    frame.set_call_stack_size(32); frame.update_call_stack_alignment(16); frame.finalize();
    a.emit_prolog(frame); a.emit_args_assignment(frame, args);
    s_memcb = MemCallbacks{};
    s_memcb.r8 = (const void*)&memcb_r8; s_memcb.r16 = (const void*)&memcb_r16; s_memcb.r32 = (const void*)&memcb_r32;
    s_memcb.w8 = (const void*)&memcb_w8; s_memcb.w16 = (const void*)&memcb_w16; s_memcb.w32 = (const void*)&memcb_w32;
    s_memcb.pAbtD = &g_irq_zero; s_memcb.pIrq = &g_irq_pending; s_memcb.pFiq = &g_irq_zero;
    s_memcb.pIrqFlag = &g_irq_flag;
    s_memMode = MemMode::Callback;
    bool ok = true;
    for (int i = 0; i < count; ++i)
        if (!emit_arm_insn(a, ctx, prog[i], 4u * (uint32_t)i, frame)) { ok = false; break; }
    s_memMode = MemMode::Mock; s_memcb = MemCallbacks{};
    if (!ok) return false;
    emit_return_imm(a, 4u * (uint32_t)count); // trailing resume PC = end of block
    a.emit_epilog(frame);
    SelfTestBlockFn fn = nullptr;
    if (rt.add(&fn, &code) != kErrorOk) return false;
    g_irq_flag = 0; // cleared before the run; the block sets it iff it exits for an exception
    const uint32_t r = fn(&cpu);
    if (retpc) *retpc = r;
    rt.release(fn);
    return true;
}
} // namespace

// A memory access with an exception pending must exit the block right after it
// (legacy gen_test_abort/gen_test_irq parity): the access itself completes, but the
// instruction after it does NOT run, and the block returns the next-instruction PC
// (where the exec loop's ARM7_CHECKIRQ then vectors). With nothing pending the block
// runs straight through.
static int jit_asmjit_memcb_irq_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00010u, // MOV  r0,#16    (pc 0)
        0xE3A010ABu, // MOV  r1,#0xAB  (pc 4)
        0xE5801000u, // STR  r1,[r0]   (pc 8)  -> memory write; may raise an exception
        0xE3A02055u, // MOV  r2,#0x55  (pc 12) -> must be SKIPPED when one is pending
    };
    const int n = (int)(sizeof(prog) / sizeof(prog[0]));
    // (1) IRQ pending and UNMASKED (cpsr=0): the STR completes (word[16]=0xAB), then
    //     the post-check exits the block at the next-instruction PC (12); r2 stays 0
    for (int i = 0; i < 128; ++i) g_memcb_words[i] = 0;
    ArmCpuSelfTest cpu{};
    g_irq_pending = 1;
    uint32_t pc1 = 0;
    if (!run_block_cb_irq(prog, n, cpu, &pc1)) return 0;
    if (!(pc1 == 12u && cpu.r[1] == 0xABu && cpu.r[2] == 0 && g_memcb_words[16 >> 2] == 0xABu
          && g_irq_flag == 1)) // block requested the exec-loop IRQ check
        return 0;
    // (2) nothing pending: runs through, r2 set, returns end-of-block PC
    for (int i = 0; i < 128; ++i) g_memcb_words[i] = 0;
    ArmCpuSelfTest cpu2{};
    g_irq_pending = 0;
    uint32_t pc2 = 0;
    if (!run_block_cb_irq(prog, n, cpu2, &pc2)) return 0;
    if (!(pc2 == 4u * (uint32_t)n && cpu2.r[2] == 0x55u)) return 0;
    // (3) IRQ pending but MASKED (cpsr I-bit set): must NOT break the block -- runs
    //     through, r2 set. If mishandled this spuriously exits every memory op
    for (int i = 0; i < 128; ++i) g_memcb_words[i] = 0;
    ArmCpuSelfTest cpu3{};
    cpu3.cpsr = 0x80u; // I_MASK (IRQ disabled)
    g_irq_pending = 1;
    uint32_t pc3 = 0;
    if (!run_block_cb_irq(prog, n, cpu3, &pc3)) return 0;
    g_irq_pending = 0; // leave the mock flag clear for any later use
    // Masked: block ran through (r2 set, end PC) and did NOT request an IRQ check.
    return (pc3 == 4u * (uint32_t)n && cpu3.r[2] == 0x55u && g_irq_flag == 0) ? 1 : 0;
}

// ---- block-chaining self-tests ---------------------------------------------
// Two code regions so a chain can hop between blocks: A at 0x1000 branches to B
// at 0x2000, which branches to 0x2800 (outside the controller range -> bail)
namespace {
int g_chain_icount = 0;
const uint32_t g_chainA[] = { 0xE3A00001u, 0xEA0003FDu }; // MOV r0,#1 ; B 0x2000
const uint32_t g_chainB[] = { 0xE3A01002u, 0xEA0001FDu }; // MOV r1,#2 ; B 0x2800
uint32_t chain_fetch(uint32_t pc)
{
    if (pc >= 0x2000u) return g_chainB[(pc - 0x2000u) >> 2];
    return g_chainA[(pc - 0x1000u) >> 2];
}
} // namespace

// One arm7_aj_run call must chain through BOTH cached blocks (no C round trip),
// charge the icount for both, and bail with the first un-runnable pc; with
// chaining off it must execute exactly one block. A pc with no block returns 0
static int jit_asmjit_chain_selftest(void)
{
    ArmAsmjitCtl *c = arm7_aj_create(0x1000u, 0x2400u);
    arm7_aj_set_icount(c, &g_chain_icount);
    int cnt = 0, cyc = 0;
    if (!arm7_aj_get(c, 0x2000u, chain_fetch, &cnt, &cyc)) return 0; // pre-translate B (cyc 4)
    ArmCpuSelfTest cpu{};
    uint32_t newpc = 0;
    g_chain_icount = 1000;
    bool ok = arm7_aj_run(c, &cpu, 0x1000u, chain_fetch, &newpc) == 1;
    // A (r0=1, 4 cycles) chained into B (r1=2, 4 cycles), bailed at 0x2800 (out of range)
    ok = ok && (newpc == 0x2800u) && (cpu.r[0] == 1u) && (cpu.r[1] == 2u) && (g_chain_icount == 992);
    // no block at 0x2800 (out of range) -> interpreter
    ok = ok && (arm7_aj_run(c, &cpu, 0x2800u, chain_fetch, &newpc) == 0);
    // kill switch: exactly one block per call
    arm7_aj_set_chaining(c, 0);
    ArmCpuSelfTest cpu2{};
    g_chain_icount = 1000;
    ok = ok && (arm7_aj_run(c, &cpu2, 0x1000u, chain_fetch, &newpc) == 1)
            && (newpc == 0x2000u) && (cpu2.r[0] == 1u) && (cpu2.r[1] == 0u) && (g_chain_icount == 996);
    arm7_aj_delete(c);
    return ok ? 1 : 0;
}

// Translation must STOP at an address that already starts a cached block
// (no overlapping duplicate code); execution then chains through the existing
// block via the dispatcher, producing the full result
static int jit_asmjit_join_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00001u, // [0x1000] MOV r0,#1
        0xE3A01002u, // [0x1004] MOV r1,#2
        0xE3A02003u, // [0x1008] MOV r2,#3
        0xEAFFFFFEu, // [0x100C] B .
    };
    g_test_prog = prog;
    g_test_base = 0x1000u;
    ArmAsmjitCtl *c = arm7_aj_create(0x1000u, 0x2000u);
    arm7_aj_set_icount(c, &g_chain_icount);
    int cnt = 0, cyc = 0;
    // translate the TAIL first (as if 0x1008 were a branch target)
    if (!arm7_aj_get(c, 0x1008u, test_fetch, &cnt, &cyc) || cnt != 2) return 0;
    // translating from 0x1000 must now stop AT 0x1008 (2 insns), not run past it (4)
    if (!arm7_aj_get(c, 0x1000u, test_fetch, &cnt, &cyc) || cnt != 2 || cyc != 2) return 0;
    // and a chained run executes prefix + tail as one arm7_aj_run call
    ArmCpuSelfTest cpu{};
    uint32_t newpc = 0;
    g_chain_icount = 1000;
    bool ok = arm7_aj_run(c, &cpu, 0x1000u, test_fetch, &newpc) == 1;
    ok = ok && (newpc == 0x100Cu) && (cpu.r[0] == 1u) && (cpu.r[1] == 2u) && (cpu.r[2] == 3u)
            && (g_chain_icount == 1000 - 2 - 4);
    arm7_aj_delete(c);
    return ok ? 1 : 0;
}

// Cycle accounting: a condition-false instruction must cost ARM's 1 cycle, not
// its full price -- blocks charge full path costs at their exits and refund
// cost-1 on the condition-false path (emit_arm_insn's refundSkip)
static int jit_asmjit_cyc_skip_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3500001u, // [0x1000] CMP r0,#1         (1)  Z = (r0 == 1)
        0x05901004u, // [0x1004] LDREQ r1,[r0,#4]  (3, or 1 when skipped)
        0xEAFFFFFEu, // [0x1008] B .               (3)
    };
    g_test_prog = prog;
    g_test_base = 0x1000u;
    for (int i = 0; i < 128; ++i) g_memcb_words[i] = 0;
    g_memcb_words[1] = 0xBEEFu; // LDREQ address = 1+4 = 5 -> mock word index 1
    ArmAsmjitCtl *c = arm7_aj_create(0x1000u, 0x2000u);
    arm7_aj_set_mem_callbacks(c, memcb_r8, memcb_r16, memcb_r32, memcb_w8, memcb_w16, memcb_w32);
    arm7_aj_set_icount(c, &g_chain_icount);
    int cnt = 0, cyc = 0;
    ArmBlockFn fn = arm7_aj_get(c, 0x1000u, test_fetch, &cnt, &cyc);
    bool ok = (fn != nullptr) && (cyc == 7); // straight-line metadata: 1+3+3
    // (1) condition TRUE: full price charged
    ArmCpuSelfTest cpuA{};
    cpuA.r[0] = 1; // CMP r0,#1 -> Z=1 -> LDREQ executes
    g_chain_icount = 100;
    ok = ok && (fn(&cpuA) == 0x1008u) && (cpuA.r[1] == 0xBEEFu) && (g_chain_icount == 93);
    // (2) condition FALSE: the skipped LDR costs 1, not 3 (refund of 2)
    ArmCpuSelfTest cpuB{};
    cpuB.r[0] = 0; // Z=0 -> LDREQ skipped
    g_chain_icount = 100;
    ok = ok && (fn(&cpuB) == 0x1008u) && (cpuB.r[1] == 0u) && (g_chain_icount == 95);
    arm7_aj_delete(c);
    return ok ? 1 : 0;
}

// Cycle accounting: multiply costs are data-dependent (1S + mI, m = 1..4 by
// multiplier magnitude) -- the block charges the worst case and the emitted
// refund corrects it from the ACTUAL Rs value, incl. |Rs| for the signed forms
static int jit_asmjit_cyc_mul_selftest(void)
{
    static const uint32_t progM[] = {
        0xE0020190u, // [0x1000] MUL r2,r0,r1  (5 worst case; real 1+m)
        0xEAFFFFFEu, // [0x1004] B .           (3)
    };
    g_test_prog = progM;
    g_test_base = 0x1000u;
    ArmAsmjitCtl *c = arm7_aj_create(0x1000u, 0x2000u);
    arm7_aj_set_icount(c, &g_chain_icount);
    int cnt = 0, cyc = 0;
    ArmBlockFn fn = arm7_aj_get(c, 0x1000u, test_fetch, &cnt, &cyc);
    bool ok = (fn != nullptr) && (cyc == 8); // worst-case metadata: 5+3
    // m = 1: |5| < 0x100 -> MUL costs 2, block 2+3 = 5
    ArmCpuSelfTest cpuA{};
    cpuA.r[0] = 2; cpuA.r[1] = 5;
    g_chain_icount = 100;
    ok = ok && (fn(&cpuA) == 0x1004u) && (cpuA.r[2] == 10u) && (g_chain_icount == 95);
    // m = 3: 0x12345 < 0x1000000 -> MUL costs 4, block 7
    ArmCpuSelfTest cpuB{};
    cpuB.r[0] = 2; cpuB.r[1] = 0x12345u;
    g_chain_icount = 100;
    ok = ok && (fn(&cpuB) == 0x1004u) && (g_chain_icount == 93);
    // signed: |-3| = 3 -> m = 1 (the raw value 0xFFFFFFFD would be m = 4)
    ArmCpuSelfTest cpuC{};
    cpuC.r[0] = 2; cpuC.r[1] = 0xFFFFFFFDu;
    g_chain_icount = 100;
    ok = ok && (fn(&cpuC) == 0x1004u) && (cpuC.r[2] == 0xFFFFFFFAu) && (g_chain_icount == 95);
    arm7_aj_delete(c);
    if (!ok) return 0;
    // UMULL classifies Rs UNSIGNED: 0xFFFFFFFD -> m = 4 (no refund)
    static const uint32_t progU[] = {
        0xE0832190u, // [0x1000] UMULL r2,r3,r0,r1  (6 worst case; real 2+m)
        0xEAFFFFFEu, // [0x1004] B .                (3)
    };
    g_test_prog = progU;
    ArmAsmjitCtl *cu = arm7_aj_create(0x1000u, 0x2000u);
    arm7_aj_set_icount(cu, &g_chain_icount);
    ArmBlockFn fnu = arm7_aj_get(cu, 0x1000u, test_fetch, &cnt, &cyc);
    ok = (fnu != nullptr) && (cyc == 9); // 6+3
    ArmCpuSelfTest cpuD{};
    cpuD.r[0] = 2; cpuD.r[1] = 0xFFFFFFFDu;
    g_chain_icount = 100;
    ok = ok && (fnu(&cpuD) == 0x1004u) && (g_chain_icount == 91); // m=4: full 9
    ArmCpuSelfTest cpuE{};
    cpuE.r[0] = 2; cpuE.r[1] = 0x10000u; // m = 3 -> refund 1 -> block 8
    g_chain_icount = 100;
    ok = ok && (fnu(&cpuE) == 0x1004u) && (g_chain_icount == 92);
    arm7_aj_delete(cu);
    return ok ? 1 : 0;
}

// Cycle accounting: an early abort/IRQ exit charges only the EXECUTED prefix;
// the unexecuted tail is charged (once) when it later runs as its own block --
// previously the exit charged the full block and the tail was charged again
static int jit_asmjit_cyc_irqexit_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00010u, // [0x1000] MOV r0,#16   (1)
        0xE5801000u, // [0x1004] STR r1,[r0]  (2)  -> pending IRQ -> exit at 0x1008
        0xE3A02005u, // [0x1008] MOV r2,#5    (1)
        0xEAFFFFFEu, // [0x100C] B .          (3)
    };
    g_test_prog = prog;
    g_test_base = 0x1000u;
    for (int i = 0; i < 128; ++i) g_memcb_words[i] = 0;
    ArmAsmjitCtl *c = arm7_aj_create(0x1000u, 0x2000u);
    arm7_aj_set_mem_callbacks(c, memcb_r8, memcb_r16, memcb_r32, memcb_w8, memcb_w16, memcb_w32);
    arm7_aj_set_irq_hooks(c, nullptr, &g_irq_zero, &g_irq_pending, &g_irq_zero, &g_irq_flag);
    arm7_aj_set_icount(c, &g_chain_icount);
    int cnt = 0, cyc = 0;
    ArmBlockFn fn = arm7_aj_get(c, 0x1000u, test_fetch, &cnt, &cyc);
    bool ok = (fn != nullptr) && (cyc == 7); // straight-line metadata: 1+2+1+3
    ArmCpuSelfTest cpu{}; // CPSR 0 -> IRQ unmasked
    g_irq_pending = 1;
    g_irq_flag = 0;
    g_chain_icount = 100;
    // early exit after the STR: only MOV+STR (1+2 = 3) charged, NOT the full 7
    ok = ok && (fn(&cpu) == 0x1008u) && (g_irq_flag == 1) && (g_chain_icount == 97);
    // the tail runs as its own block and pays its own 4 -- total once = 7
    ArmBlockFn fn2 = arm7_aj_get(c, 0x1008u, test_fetch, &cnt, &cyc);
    ok = ok && (fn2 != nullptr) && (fn2(&cpu) == 0x100Cu) && (cpu.r[2] == 5u)
            && (g_chain_icount == 93);
    g_irq_pending = 0;
    g_irq_flag = 0;
    arm7_aj_delete(c);
    return ok ? 1 : 0;
}

// The dispatcher checks the cycle budget BEFORE each hop (the C loop's
// while (ICOUNT > 0)): with exactly one block's budget, the chain must stop
// after the first block even though the next one is cached
static int jit_asmjit_chain_expiry_selftest(void)
{
    ArmAsmjitCtl *c = arm7_aj_create(0x1000u, 0x2400u);
    arm7_aj_set_icount(c, &g_chain_icount);
    int cnt = 0, cyc = 0;
    if (!arm7_aj_get(c, 0x2000u, chain_fetch, &cnt, &cyc)) return 0;
    ArmCpuSelfTest cpu{};
    uint32_t newpc = 0;
    g_chain_icount = 4; // exactly block A's cost
    bool ok = arm7_aj_run(c, &cpu, 0x1000u, chain_fetch, &newpc) == 1;
    ok = ok && (newpc == 0x2000u) && (cpu.r[0] == 1u) && (cpu.r[1] == 0u) && (g_chain_icount == 0);
    arm7_aj_delete(c);
    return ok ? 1 : 0;
}

// A post-check exit (pending+unmasked IRQ at a STR) must end the chain
// immediately with the irq flag still set for the C loop -- even though the
// block at the resume pc is cached
static int jit_asmjit_chain_irq_selftest(void)
{
    static const uint32_t prog[] = {
        0xE3A00010u, // [0x1000] MOV r0,#16
        0xE5801000u, // [0x1004] STR r1,[r0] -> pending IRQ -> post-check exit at 0x1008
        0xE3A02055u, // [0x1008] MOV r2,#0x55 (own block too; must NOT run)
        0xEAFFFFFEu, // [0x100C] B .
    };
    g_test_prog = prog;
    g_test_base = 0x1000u;
    for (int i = 0; i < 128; ++i) g_memcb_words[i] = 0;
    ArmAsmjitCtl *c = arm7_aj_create(0x1000u, 0x2000u);
    arm7_aj_set_mem_callbacks(c, memcb_r8, memcb_r16, memcb_r32, memcb_w8, memcb_w16, memcb_w32);
    arm7_aj_set_irq_hooks(c, nullptr, &g_irq_zero, &g_irq_pending, &g_irq_zero, &g_irq_flag);
    arm7_aj_set_icount(c, &g_chain_icount);
    int cnt = 0, cyc = 0;
    if (!arm7_aj_get(c, 0x1008u, test_fetch, &cnt, &cyc)) return 0; // cache the resume-pc block
    ArmCpuSelfTest cpu{}; // CPSR 0 -> IRQ unmasked
    g_irq_pending = 1;
    g_irq_flag = 0;
    g_chain_icount = 1000;
    uint32_t newpc = 0;
    bool ok = arm7_aj_run(c, &cpu, 0x1000u, test_fetch, &newpc) == 1;
    // The 0x1000 block stops at 0x1008 (already a cached block start -> join),
    // so it is MOV+STR, cyc 1+2 = 3, charged in full on the post-check exit.
    // The chain must NOT continue into the cached 0x1008 block (flag set)
    ok = ok && (newpc == 0x1008u) && (cpu.r[0] == 16u) && (cpu.r[2] == 0u)
            && (g_irq_flag == 1) && (g_chain_icount == 997);
    g_irq_pending = 0;
    g_irq_flag = 0;
    arm7_aj_delete(c);
    return ok ? 1 : 0;
}

// The full self-test suite, in run order. Each entry is one of the static
// jit_asmjit_*_selftest() functions above plus a short human-readable label.
// Add a test by appending one row here -- the runner and the report pick it up
// automatically (no parallel list to keep in sync)

namespace {
struct SelfTest { const char *label; int (*fn)(void); };
const SelfTest k_selftests[] = {
    { "block ABI",         jit_asmjit_selftest },           // hand-emitted block (per-host backend)
    { "ARM7 translator",   jit_asmjit_translate_selftest }, // real ARM7 decode -> asmjit
    { "conditions",        jit_asmjit_cond_selftest },      // portable condition evaluation
    { "shifts (imm)",      jit_asmjit_shift_selftest },     // shifted-register operand 2
    { "flags (logical)",   jit_asmjit_flags_selftest },     // flag-setting (S=1) logical ops
    { "flags (arith)",     jit_asmjit_arith_selftest },     // arithmetic flags + compares
    { "shifts (reg)",      jit_asmjit_regshift_selftest },  // register-specified shift amounts
    { "#0 specials+RRX",   jit_asmjit_special_selftest },   // LSR/ASR #0 specials + RRX
    { "ADC/SBC/RSC",       jit_asmjit_adc_selftest },       // ADC/SBC/RSC carry-in
    { "flags (reg op2)",   jit_asmjit_regop_selftest },     // flags with register/shifted operand 2
    { "LDR/STR (imm)",     jit_asmjit_ldrstr_selftest },    // LDR/STR/LDRB/STRB (immediate offset)
    { "LDR/STR (reg)",     jit_asmjit_ldrstr_reg_selftest },// LDR/STR (register offset)
    { "LDR/STR (PC)",      jit_asmjit_ldrstr_pc_selftest }, // LDR/STR PC-relative
    { "branch (B/BL)",     jit_asmjit_branch_selftest },    // B/BL
    { "LDR->PC / wb",      jit_asmjit_ldrpc_selftest },     // LDR-into-PC / PC-base writeback
    { "DP -> PC",          jit_asmjit_dppc_selftest },      // data-processing writes to PC
    { "LDRH/STRH/sign",    jit_asmjit_halfword_selftest },  // LDRH/STRH/LDRSB/LDRSH
    { "LDM/STM",           jit_asmjit_ldmstm_selftest },    // LDM/STM (incl. LDM ...,PC)
    { "BX",                jit_asmjit_bx_selftest },        // BX (unmasked PC, T bit on odd)
    { "R15-src DP",        jit_asmjit_dpsrcpc_selftest },   // Rn/Rm == R15 pipelined-PC constants
    { "shift carry (S)",   jit_asmjit_shiftcarry_selftest },// logical S with shifted op2 (shifter carry-out)
    { "MUL/MLA",           jit_asmjit_mul_selftest },       // 32-bit multiply (S=1: N/Z, C preserved)
    { "MULL (64-bit)",     jit_asmjit_mull_selftest },      // UMULL/SMULL/UMLAL (S=1 on 64-bit result)
    { "real-mode (defer)", jit_asmjit_realmode_selftest },  // partial-JIT (memory deferred to interpreter)
    { "memory callbacks",  jit_asmjit_memcb_selftest },     // LDR/STR via real host calls (callback mode)
    { "memcb halfword",    jit_asmjit_memcb_half_selftest },// LDRH/STRH/LDRSB/LDRSH via real host calls
    { "memcb LDM/STM",     jit_asmjit_memcb_ldm_selftest }, // LDM/STM (incl. LDM ...,PC) via real host calls
    { "memcb abort/IRQ",   jit_asmjit_memcb_irq_selftest }, // mid-block abort/IRQ exit (gen_test_irq parity)
    { "MRS/MSR (psr cb)",  jit_asmjit_psr_selftest },       // PSR transfer via the psr_transfer callback
    { "SWP/SWPB",          jit_asmjit_swap_selftest },      // single data swap via the memory thunks
    { "STM asc. order",    jit_asmjit_stm_order_selftest }, // STMDB writes ascending addresses (IO-visible contract)
    { "MOVS/SUBS PC",      jit_asmjit_exret_selftest },     // exception return via the exc_return callback
    { "JIT controller",    jit_asmjit_controller_selftest },// JIT controller: map/translate/cache/invalidate
    { "SMC mid-block",     jit_asmjit_smc_midblock_selftest },// write into a block's middle retires all covering blocks
    { "SMC self-inval",    jit_asmjit_smc_selfinval_selftest },// block untranslating itself mid-run (deferred release)
    { "chain (2 blocks)",  jit_asmjit_chain_selftest },     // dispatcher chains cached blocks; kill switch; miss
    { "block join",        jit_asmjit_join_selftest },      // translation stops at an existing block start (no dup)
    { "chain expiry",      jit_asmjit_chain_expiry_selftest },// budget check before each hop stops the chain
    { "chain IRQ bail",    jit_asmjit_chain_irq_selftest }, // post-check exit ends the chain, flag left for C
    { "cyc: cond skip",    jit_asmjit_cyc_skip_selftest },  // skipped conditional costs 1 (refund), executed full
    { "cyc: IRQ exit",     jit_asmjit_cyc_irqexit_selftest },// early exit charges executed prefix only (no double)
    { "cyc: MUL m-refund", jit_asmjit_cyc_mul_selftest },   // data-dependent multiply cost (1S+mI, signed/unsigned m)
    { "cycle accounting",  jit_asmjit_cycles_selftest },    // legacy-JIT cycle model + stop-at-branch
};
const int k_selftest_count = (int)(sizeof(k_selftests) / sizeof(k_selftests[0]));
} // namespace

// Single entry point: run every self-test, optionally appending a one-line-per-
// test "label: PASS/FAIL" summary to 'report' (a caller buffer of report_size
// bytes; pass NULL to skip). Returns 1 if all tests pass, 0 otherwise. Headless
// and platform-independent -- use this from a CI harness (assert the return, or
// print the report and exit non-zero on failure)
extern "C" int jit_asmjit_run_selftests(char *report, int report_size)
{
    int all_ok = 1;
    int used = 0;
    for (int i = 0; i < k_selftest_count; ++i) {
        const int ok = k_selftests[i].fn() ? 1 : 0;
        all_ok = all_ok && ok;
        if (report && report_size > 0 && used < report_size - 1) {
            const int n = std::snprintf(report + used, (size_t)(report_size - used),
                                        "  %-18s%s\n", k_selftests[i].label,
                                        ok ? "PASS" : "FAIL");
            if (n > 0) used += n;
        }
    }
    return all_ok;
}

// Optional Win32 verification aid: run the suite once per process and show the
// result in a message box. (Thin wrapper over jit_asmjit_run_selftests; the
// startup call from arm7_core_init() was removed once the JIT was verified, but
// this stays for manual regression checks.) Invoked at run time, NOT at
// static-init/DLL-load time, to avoid the loader-lock hazard of MessageBox in DllMain
extern "C" void jit_asmjit_selftest_report(void)
{
    static int reported = 0;
    if (reported)
        return;
    reported = 1;

    char body[2048];
    const int ok = jit_asmjit_run_selftests(body, (int)sizeof(body));
#if defined(EXPOSE_JIT_SELFTEST) && defined(_WIN32)
    char msg[2112];
    std::snprintf(msg, sizeof(msg), "asmjit self-tests:\n\n%s", body);
    MessageBoxA(NULL, msg, "PinMAME asmjit JIT", MB_OK | (ok ? MB_ICONINFORMATION : MB_ICONERROR));
#else
    (void)ok; // non-Windows: no message box (verification aid is Win32-only)
#endif
}

#endif // PINMAME_JIT_ASMJIT
