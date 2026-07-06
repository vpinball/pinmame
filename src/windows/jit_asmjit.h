/*
 *   C-callable API for the asmjit-based ARM7 JIT controller
 *
 *   The emulator core (C) calls these to translate-and-run blocks. Active only when PINMAME_JIT_ASMJIT is defined;
 *   otherwise the emulator uses the interpreter (and/or the legacy x86-only JIT)
 */

#ifndef INC_JIT_ASMJIT
#define INC_JIT_ASMJIT

#ifdef PINMAME_JIT_ASMJIT

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif

/* Opaque JIT controller (address->block map + asmjit JitRuntime) */
typedef struct ArmAsmjitCtl ArmAsmjitCtl;

/* A translated block: takes the emulated register-file context
 * (&ARM7.sArmRegister[0]) and returns the emulated PC at which to resume */
typedef uint32_t (*arm7_block_fn)(void *cpu_ctx);

/* Create/destroy a controller covering [minAddr, maxAddr) of opcode space.
 * Create early (arm7_core_init) so the pointer lands in MAME's CPU-context
 * snapshot and survives set_context; pass an empty range and set it later */
ArmAsmjitCtl *arm7_aj_create(uint32_t minAddr, uint32_t maxAddr);
void          arm7_aj_delete(ArmAsmjitCtl *c);

/* Reconfigure an existing controller's opcode range IN PLACE (pointer unchanged).
 * Call from the CPU-specific init once the span is known; mutating the existing
 * object (rather than recreating) keeps the context-saved pointer valid */
void          arm7_aj_set_range(ArmAsmjitCtl *c, uint32_t minAddr, uint32_t maxAddr);

/* DEBUG: exclusion list -- interpret PCs in any added [lo, hi) even inside
 * [minAddr,maxAddr). Debug aid to JIT everything EXCEPT one or more suspect regions */
void          arm7_aj_clear_exclude(ArmAsmjitCtl *c);
void          arm7_aj_add_exclude(ArmAsmjitCtl *c, uint32_t lo, uint32_t hi);

/* Enable/disable translation (disabled -> arm7_aj_get returns NULL -> interpreter).
 * Gate the JIT to the stable post-remap program, mirroring the legacy jit_enable */
void          arm7_aj_set_enabled(ArmAsmjitCtl *c, int enabled);

/* Real memory access thunks the JIT calls from translated blocks. Reads return
 * the value (byte/half zero/sign-extended by the thunk); writes take (addr,data).
 * Uniform uint32_t signatures keep the emitted call ABI-uniform across sizes */
typedef uint32_t (*arm7_aj_read_fn)(uint32_t addr);
typedef void     (*arm7_aj_write_fn)(uint32_t addr, uint32_t data);

/* Install the read/write thunks. With them set the JIT emits direct calls for
 * memory ops (blocks span LDR/STR); without them memory ops end the block and the
 * interpreter performs the access. Resets the controller (drops cached blocks) */
void          arm7_aj_set_mem_callbacks(ArmAsmjitCtl *c,
                  arm7_aj_read_fn r8, arm7_aj_read_fn r16, arm7_aj_read_fn r32,
                  arm7_aj_write_fn w8, arm7_aj_write_fn w16, arm7_aj_write_fn w32);

/* Wire the abort/IRQ hooks for legacy-JIT parity (gen_test_abort/gen_test_irq).
 * 'check_irq' is the emulator's arm7_check_irq_state; pAbtD/pIrq/pFiq are addresses
 * of the pending-flag bytes (a run-time guard). After a memory access a translated
 * block uses these to vector a data abort / FIQ / IRQ raised by the access, before
 * running the rest of the block. Unset (self-tests only; the real build always
 * wires them), no post-check is emitted, so an exception raised synchronously BY a
 * JITted memory access is DELAYED until the next external raise or interpreted
 * LDR/STR check -- externally raised IRQs are unaffected either way.
 * Resets the controller (drops cached blocks) */
void          arm7_aj_set_irq_hooks(ArmAsmjitCtl *c, const void *check_irq,
                  const void *pAbtD, const void *pIrq, const void *pFiq,
                  const void *pIrqFlag);

/* Wire the emulator's PSR-transfer handler (HandlePSRTransfer, as
 * void fn(uint32_t insn)). With it set, translated blocks execute MRS/MSR by
 * calling it with the raw instruction word -- exact interpreter semantics,
 * including SPSR banking and CPSR mode switches (the bank swap goes through
 * the active register window, so the block's fixed context offsets stay
 * valid). Unset, PSR transfers end the block and the interpreter runs them.
 * Resets the controller (drops cached blocks) */
void          arm7_aj_set_psr_transfer(ArmAsmjitCtl *c, const void *psr_transfer);

/* Wire the emulator's exception-return helper (uint32_t fn(uint32_t newpc)) so
 * translated blocks can execute MOVS PC / SUBS PC,LR,... (the IRQ/exception
 * exit) with exact interpreter semantics: SPSR->CPSR restore, bank switch, PC
 * mask, ARM7_CHECKIRQ; the helper returns the FINAL PC (the IRQ check may have
 * vectored), which the block returns to the exec loop. Unset, those defer to
 * the interpreter. Resets the controller (drops cached blocks) */
void          arm7_aj_set_exc_return(ArmAsmjitCtl *c, const void *exc_return);

/* Discard all translated blocks (e.g. on CPU reset / program reload).
 * Safe to call from a memory-write thunk while a block is executing: the code
 * memory is only RETIRED here and freed at the next arm7_aj_get (never while
 * generated code can be on the host stack) */
void          arm7_aj_reset(ArmAsmjitCtl *c);

/* Invalidate every translated block COVERING 'addr' (self-modifying-code
 * write) -- both a block starting there and blocks it lies in the middle of.
 * O(1) when no block covers the address (the overwhelmingly common store).
 * Safe to call from the store thunks while an affected block is executing (a
 * block whose STR hits its own span retires itself): freeing of the code
 * memory is deferred to the next arm7_aj_get */
void          arm7_aj_untranslate(ArmAsmjitCtl *c, uint32_t addr);

/* Return the block for 'pc' (translating on demand), or NULL to interpret 'pc'.
 * 'fetch' returns the instruction word at a given address. *outCount receives the
 * number of emulated instructions the block covers; *outCycles its straight-line
 * cycle total (informational: blocks charge ARM7_ICOUNT themselves at each exit
 * with the executed path's exact cost -- early abort/IRQ exits charge only the
 * executed prefix, skipped conditionals cost 1). Either may be NULL
 * (The exec loop uses arm7_aj_run below instead; this remains for tests/tools) */
arm7_block_fn arm7_aj_get(ArmAsmjitCtl *c, uint32_t pc,
                          uint32_t (*fetch)(uint32_t), int *outCount, int *outCycles);

/* Wire the emulator's cycle counter (&ARM7_ICOUNT, a stable global address).
 * REQUIRED for arm7_aj_run, which charges cycles internally; unwired, it
 * reports "no block" so the interpreter runs. Resets the controller */
void          arm7_aj_set_icount(ArmAsmjitCtl *c, int *picount);

/* Block-chaining kill switch (default ON) for A/B timing or bisection: off,
 * arm7_aj_run executes exactly one block per call */
void          arm7_aj_set_chaining(ArmAsmjitCtl *c, int enabled);

/* Exec-loop entry point: execute translated code starting at 'pc' with the
 * register-file context 'cpu_ctx' (&ARM7.sArmRegister[0]). Returns 0 if 'pc'
 * has no translated block -> interpret it. Otherwise runs one block -- or,
 * with chaining, as many already-cached blocks as the cycle budget allows,
 * via a JIT-emitted dispatcher that never returns to C between blocks --
 * charges ARM7_ICOUNT for everything it ran (the caller must NOT subtract),
 * stores the resume PC in *out_newpc, and returns 1. A block that requests
 * an IRQ check (arm7_aj_irq_flag) always ends the chain first, so the
 * caller's flag handling is unchanged */
int           arm7_aj_run(ArmAsmjitCtl *c, void *cpu_ctx, uint32_t pc,
                          uint32_t (*fetch)(uint32_t), uint32_t *out_newpc);

/* Run the full code-generation self-test suite. Returns 1 if every test passes,
 * 0 otherwise. If 'report' is non-NULL, a "label: PASS/FAIL" line per test is
 * written into it (up to report_size bytes). Headless/portable -- intended for a
 * CI smoke test. (jit_asmjit_selftest_report() is the Win32 MessageBox variant.) */
int  jit_asmjit_run_selftests(char *report, int report_size);

/* Win32 MessageBox variant of the suite: runs it once per process and shows a
 * PASS/FAIL report. No-op (no box) on non-Windows. For manual/visual checks. */
void jit_asmjit_selftest_report(void);

#ifdef __cplusplus
}
#endif

#endif /* PINMAME_JIT_ASMJIT */

#endif /* INC_JIT_ASMJIT */
