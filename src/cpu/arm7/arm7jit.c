/*
 *   JIT translator for ARM7 on Windows/Intel
 *   
 *   IMPORTANT USAGE NOTE!  As with the rest of the ARM7 core code, this
 *   module is written to be generic across multiple ARM7-derived processors,
 *   so to use it, you must #include it in your derived CPU .c file rather
 *   than incorporating it directly into the project file as a separate .c
 *   module.  This is currently #included from arm7.c and at91.c, since those
 *   are the concrete ARM7 CPUs in the current set.
 *   
 *   * * *
 *   
 *   A note on cycle counting: Most ARM instructions have fixed cycle timing
 *   that we can determine statically from the opcode.  For a few (e.g.,
 *   multiply), the timing varies by operand value.  For opcodes with
 *   variable timing, we don't bother generating the extra native code that
 *   would be required to figure the exact timing, since that would add
 *   run-time overhead that doesn't seem justified.  Instead, we use simple
 *   fixed approximations based on the range of cycle times for the
 *   instructions in question.
 *   
 *   There are really two dependencies on our cycle counting, and neither
 *   seems to need very high precision.  The first dependency is MAME itself.
 *   MAME uses cycle counting to allocate CPU time slices.  Everything in
 *   MAME is single-threaded, so it's up to a CPU emulator to figure out when
 *   its time slice is up and return to the MAME scheduler.  This makes it
 *   important that we have some idea of how long we've been running, because
 *   we'd otherwise never yield our time slice.  But MAME doesn't depend on
 *   CPU cycle counting for actual time sync; for that it uses external
 *   references, such as the system real-time clock and the video sync rate.
 *   So for MAME's purposes, cycle counting only needs to be approximate.
 *   
 *   The second dependency is the application code (the game ROM code that
 *   we're running on the emulated CPU).  It's conceivable that some
 *   application code could depend on exact cycle timing for real-time
 *   activity, by using intentional CPU wait loops or cycle-counted routines.
 *   This was in fact common practice in the days of 8-bit microcomputers.
 *   But such practices have long gone by the wayside.  ARM7 is a modern
 *   architecture, and applications for such a modern system would never do
 *   timing this way; the modern approach is to use external time references,
 *   like MAME does.  So it seems reasonable to assume that application code
 *   is even less interested in exact cycle counting than MAME is.
 */

#if JIT_ENABLED

#include <Windows.h>
#include <stdarg.h>

#include "memory.h"
#include "arm7jit.h"
#include "arm7core.h"
#include "windows/jitemit.h"

// forward definitions
static int xlat(struct jit_ctl *jit, data32_t addr);
static int xlat_recursive(struct jit_ctl *jit, data32_t addr);

// macros we redefine from the arm7core code
#undef R15
#undef BX

// Active registers are in the fixed array ARM7.sArmRegister.  This
// array is indexed by register number.
#define Rn(n) Idx, Imm, ((UINT32)&ARM7.sArmRegister[n])
#define RCPSR Rn(eCPSR)

// Emit a conditional jump.  We'll TEST CPSR against the given flag mask,
// then do the given conditional jump.
#define testCPSR(mask)          emit(TEST, DwordPtr, RCPSR, Imm, mask)
#define condJump(jcc, cpsrMask) testCPSR(cpsrMask), emit(jcc, Label, condLabel = jit_new_fwd_label())
#define condJumpIfNot(cpsrMask) condJump(JZ, cpsrMask)
#define condJumpIf(cpsrMask)    condJump(JNZ, cpsrMask)
#define condJumpOnFlags(jcc)    cpsr_to_flags(), emit(jcc, Label, condLabel = jit_new_fwd_label())

// Populate the native Intel EFLAGS with N, Z, C, V from the CPSR flags.
// This requires reshuffling the bits from ARM order to Intel order:
//
//   MOV EAX, CPSR   NZCVxxxx xxxxxxxx xxxxxxxx xxxxxxxx
//   SHR EAX, 22     00000000 00000000 000000NZ CVxxxxxx
//   SHL AH,  5      00000000 00000000 0NZ00000 CVxxxxxx
//   SHL EAX, 1      00000000 00000000 NZ00000C Vxxxxxx0
//   
// AH is now in the correct format to set EFLAGS N,Z,C via SAHF.  If we
// interpret AL as a signed 8-bit int, V is its sign bit, so the value
// is positive if V is clear and negative if V is set.  Adding -128 (0x80)
// to AL will overflow if AL is negative, and won't overflow if AL is
// positive, so ADD AL,0x80 will effectively set the native V flag to
// match the CPSR V bit.  Then we just SAHF to restore N,Z,C and we
// have all four flags copied to Intel.  NB: the ARM C flag has the
// opposite sense from the Intel C flag for subtraction (SUB/CMP).  The
// JA,JB,JAE,JBE tests work differently on Intel as a result, so care
// must be taken if using CPSR flags with these jump types.
#define cpsr_to_flags() \
	emit(MOV, EAX, RCPSR), \
	emit(SHR, EAX, Imm, 22), \
	emit(SHL, AH, Imm, 5), \
	emit(SHL, EAX, Imm, 1), \
	emit(ADD, AL, Imm, 0x80), \
	emit(SAHF)

// Store the native Intel EFLAGS N, Z, C, and V flags into CPSR.  No
// other CPSR flags are affected.
//
//   LAHF          xxxxxxxx xxxxxxxx NZxxxxxC xxxxxxxx
//   SETO AL       xxxxxxxx xxxxxxxx NZxxxxxC 0000000V
//   SHL AL,7      xxxxxxxx xxxxxxxx NZxxxxxC V0000000
//   SHR EAX, 1    0xxxxxxx xxxxxxxx xNZxxxxx CV000000
//   SHR AH, 5     0xxxxxxx xxxxxxxx 00000xNZ CV000000
//   SHL EAX, 22   NZCV0000 00000000 00000000 00000000
#define flags_to_cpsr() \
	emit(LAHF), \
	emit(SETO, AL), \
	emit(SHL, AL, Imm, 7), \
	emit(SHR, EAX, Imm, 1), \
	emit(SHR, AH, Imm, 5), \
	emit(SHL, EAX, Imm, 22), \
	emit(AND, DwordPtr, RCPSR, Imm, 0x0FFFFFFF), \
	emit(OR, RCPSR, EAX)

// Copy just the carry flag to intel register.

#define cpsr_to_carry() \
	emit(MOV, EAX, RCPSR), \
	emit(SHR, EAX, Imm, 21), \
	emit(SAHF)

// Store just the carry flag
// ECX = xxxxxxxx xxxxxxxx xxxxxxxx 0000000C
//       00C00000 00000000 00000000 00000000

#define carry_to_cpsr() \
	emit(SETC, CL), \
	emit(SHL, ECX, Imm, 29), \
	emit(AND, DwordPtr, RCPSR, Imm, ~C_MASK), \
	emit(OR, RCPSR, ECX)
	
// Cycle counter.  This is in the static variable ARM7_ICOUNT, but because we
// access it once for every translated opcode, we keep it in EDI while in
// translated code for fast access (and smaller Intel machine code).  arm7exec
// must load the static into EDI before calling into native code, and store
// EDI back in the static when the native code returns.
//#define RCYCLECNT       DwordPtr, Idx, Imm, (UINT32)&ARM7_ICOUNT
#define RCYCLECNT       EDI

// Count 'n' cycles for an opcode.  The cycle counter tells us how many cycles
// we have left in the current CPU time slice allocated by the MAME scheduler.
// We decrement it for each instruction we execute, and return to the MAME
// scheduler when it reaches zero.  This macro only decrements the counter; we
// only check it against 0 on certain types of instructions, such as jumps and
// subroutine calls.
#define count_cycles(n) if ((n)==1) emit(DEC, RCYCLECNT); else if ((n)>0) emit(SUB, RCYCLECNT, Imm, n); else


// signed long mulitply
static int MUL64(struct jit_ctl *jit, data32_t addr, data32_t insn, int *cycles)
{
	int rm, rs;
	int rdhi, rdlo;
	int is_signed = insn & 0x00400000;

	// get the source and destination registers
	rm  = insn & 0xf;
	rs  = (insn >> 8) & 0xf;
	rdhi = (insn >> 16) & 0xf;
	rdlo = (insn >> 12) & 0xf;

	// R15 can't be used in an SMULL/UMULL
	if (rm == 15 || rs == 15 || rdhi == 15 || rdlo == 15)
		return 0;

	// load registers and perform the multiplication - result is in EDX:EAX
	emit(MOV, EAX, Rn(rm));
	emitv(is_signed ? imIMUL : imMUL, DwordPtr, Rn(rs));

	// if this is MLA, add the result to the destination registers; otherwise
	// store the result in the destination register
	if (insn & INSN_MUL_A)
	{
		// MLA - this op means Rdhi:Rdlo += Rm*Rs
		// Add the low dwords, then add the high dwords with the carry
		emit(ADD, Rn(rdlo), EAX);
		emit(ADC, Rn(rdhi), EDX);

		// count the extra cycle for MLA
		*cycles += 1;
	}
	else
	{
		// regular multiply - store the result in Rdhi:Rdlo
		emit(MOV, Rn(rdlo), EAX);
		emit(MOV, Rn(rdhi), EDX);
	}

	// if the S flag is set, we must set N and Z according to the result
	if (insn & INSN_S)
	{
		// if this was SMULA, pull the sum result back into EDX:EAX to figure flags
		if (insn & INSN_MUL_A) {
			emit(MOV, EAX, Rn(rdlo));
			emit(MOV, EDX, Rn(rdhi));
		}

		// figure Z by ORing the two operands together to see if any bits are set
		emit(OR, EAX, EDX);
		emit(SETZ, AL);

		// figure N from the high bit of the result
		emit(TEST, EDX, Imm, 0x80000000);
		emit(SETNZ, AH);

		// code these two bits into the CPSR flags:
		//                     EAX = xxxxxxxx xxxxxxxx 0000000N 0000000Z
		emit(SHL, AH, Imm, 7);    // xxxxxxxx xxxxxxxx 0000000N Z0000000
		emit(SHL, EAX, Imm, 23);  // NZ000000 00000000 00000000 00000000
		emit(AND, DwordPtr, RCPSR, ~(N_MASK | Z_MASK));
		emit(OR, RCPSR, EAX);
	}

	// MULL takes 1S + (m+1)I and MLAL 1S + (m+2)I cycles to execute, where m is the
	// number of 8 bit multiplier array cycles required to complete the multiply, which is
	// controlled by the value of the multiplier operand specified by Rs.
	// 
	// The cycle timing is data-dependent, so we'd have to calculate it at run-time to
	// get the correct results.  This doesn't seem worth the overhead - use an average.
	// This instruction can take from 3 to 6 cycles, so use 5 as a guess.
	*cycles += 2;  // bump up from default 3 cycles

	// successful translation
	return 1;
}

// 32-bit multiply
static int MUL32(struct jit_ctl *jit, data32_t addr, data32_t insn, int *cycles)
{
	int rd, rm, rs, rn;
	int is_mla = insn & INSN_MUL_A;

	// MUL takes 1S + mI and MLA 1S + (m+1)I cycles to execute, where S and I are as
	// defined in 6.2 Cycle Types on page 6-2.
	// m is the number of 8 bit multiplier array cycles required to complete the
	// multiply, which is controlled by the value of the multiplier operand
	// specified by Rs.
	//
	// Since the cycle timing is data-dependent, we'd have to generate run-time code
	// to get the exact count.  This doesn't seem worth the overhead, so use an
	// approximation based on the range of 2-5.
	*cycles += 4;

	// get the source and destination registers
	rm = insn & INSN_MUL_RM;
	rs = (insn & INSN_MUL_RS) >> INSN_MUL_RS_SHIFT;
	rd = (insn & INSN_MUL_RD) >> INSN_MUL_RD_SHIFT;
	rn = (insn & INSN_MUL_RN) >> INSN_MUL_RN_SHIFT;

	// R15 isn't allowed for any register in a multiply
	if (rm == 15 || rs == 15 || rd == 15 || (is_mla && rn == 15))
		return 0;
	
	// Load EAX and do the multiply.  Per the ARM documentation, signed and unsigned
	// multipy operations are the same for the low-order 32 bits of the result, so
	// it doesn't matter which type we use.
	emit(MOV, EAX, Rn(rm));
	emit(MUL, DwordPtr, Rn(rs));

	// add Rn if this is MLA
	if (insn & INSN_MUL_A)
	{
		// add Rn
		emit(ADD, EAX, Rn(rn));

		// extra cycle for MLA
		*cycles += 1;
	}

	// store the result in Rd
	emit(MOV, Rn(rd), EAX);

	// if the S flag is set, set the N and Z flags
	if (insn & INSN_S)
	{
		// Bugfixes here -- djrobx 
		// figure Z from the result
		emit(CMP, EAX, Imm, 0);
		emit(SETZ, BL);          // EBX = xxxxxxxx xxxxxxxx xxxxxxxx 0000000Z
		emit(SHL, BL, Imm, 7);        // EBX = xxxxxxxx xxxxxxxx xxxxxxxx Z0000000

		// figure N from the result
		emit(TEST, EAX, Imm, 0x80000000);
		emit(SETNZ, BH);         // EBX = xxxxxxxx xxxxxxxx 0000000N Z0000000
		emit(SHL, EBX, Imm, 23); // EBX = NZ000000 00000000 00000000 00000000

		// MUL always sets C to 0 and leaves V unchanged - we conveniently have
		// 0 in the right bit for C at this point.  Mask out the bits in CPSR
		// and OR in the new flags from EBX.
		emit(AND, DwordPtr, RCPSR, Imm, 0x1FFFFFFF);
		emit(OR, RCPSR, EBX);
	}

	// successful translation
	return 1;
}

// Generate an ABORT test.
//
// This generates code to test the ARM7 pending-ABORT flag to see if a data abort has
// occurred.  If so, we immediately return to the emulator, aborting the current
// instruction processing.  Before returning, we zero the cycle counter.  This causes
// the emulator to exit its main loop and return to the MAME scheduler.  When the MAME
// scheduler re-schedules this CPU, we'll check for the pending abort, and trap to the
// ABORT interrupt handler.
//
// The ABORT interrupt is designed to be raised by a hardware memory manager to signal
// virtual memory exceptions to the operating system software.  An ABORT can therefore
// be raised by any call to an emulator memory access function (jit->read8, jit->write8,
// etc).  Whenever we generate a call to a memory function, we should always call this
// routine immediately afterwards to generate the ABORT test code.
//
// Callers should also take care to process the side effects of a memory instruction in
// the proper order with respect to the ABORT test.  In particular, index register
// write-backs and register data loads should happen after the ABORT test, since an
// ABORT on the real ARM7 bypasses these effects.  Refer to the ARM documentation for
// details on the effects of ABORT on individual opcodes.
//
// 'sp_inc' tells us how many bytes of temps the caller has saved on the stack.  If
// we need to abort, we'll add this number to ESP to remove temps and restore the
// stack for the RETN to the emulator.

static void gen_test_abort(struct jit_ctl *jit, data32_t addr, int sp_inc)
{
	data8_t *abt = &ARM7.pendingAbtD;

	struct jit_label *lbl;

	emit(CMP, BytePtr, Idx, Imm, (UINT32)abt, Imm, 0);   // test pending ABORT flag
	emit(JE, Label, lbl = jit_new_fwd_label());          // if no abort, jump ahead
	emit(MOV, RCYCLECNT, Imm, 0);                        // clear the remaining cycle counter so we exit the emulator loop
	emit(MOV, EAX, Imm, addr + 4);                       // set PC to next instruction address on return to emulator
	if (sp_inc != 0) emit(ADD, ESP, Imm, sp_inc);        // clear temps off the stack
	emit(RETN);                                          // return to the emulator
	jit_resolve_label(lbl);                              // no abort - proceed as normal
}

// A memory read/write may trigger an IRQ.  We need to check and possibly branch
// to a new address. 

static void gen_test_irq(struct jit_ctl *jit, data32_t addr)
{
	//data8_t *abt = &ARM7.pendingIrq;
	struct jit_label *lbl;

	// If no pending IRQ, skip IRQ check. Probably should also
	// check FIQ here, but the implementation that calls for this
	// doesn't need it, and the IRQ check wasn't here before, so 
	// probably shouldn't take performance penalty for no good reason.
	// The gist is that a memory read/write may have caused an external source
	// to throw an IRQ.  FIQ tends to be for something like a timer, which MAME
	// will check for on its own schedule. 
//	emit(CMP, BytePtr, Idx, Imm, (UINT32)abt, Imm, 0);
//	emit(JE, Label, lbl = jit_new_fwd_label());  
	emit(MOV, DwordPtr, Rn(15), Imm, addr+4);
	emit(CALL, Label, jit_new_native_label((byte *)&arm7_check_irq_state));
	// If R15 changed, then do a lookup, otherwise move along.
	emit(MOV, EAX, Rn(15));
	emit(CMP, EAX, Imm, addr+4);
	emit(JE, Label, lbl = jit_new_fwd_label());
	emit(JMP, Label, jit_new_native_label(jit->pLookup));
	jit_resolve_label(lbl);  
	// While we're at it, let's look at the cycle count.  
/*	emit(CMP, RCYCLECNT, Imm, 0);
	emit(JG, Label, lbl = jit_new_fwd_label());
	emit(MOV, EAX, Imm, addr+4);
	emit(RETN);
	jit_resolve_label(lbl);*/
}

// service routines for gen_mem
static void gen_mem_read(struct jit_ctl *jit, int siz, int rn, data32_t immAddr, int ofs)
{
	// push the address (either the immediate address or the index register)
	if (rn == Imm) {
		emit(PUSH, Imm, immAddr + ofs);
	}
	else {
		emit(PUSH, rn);
		if (ofs != 0)
			emit(ADD, DwordPtr, Idx, ESP, Imm, ofs);
	}

	// call the emulator memory reader
	emit(CALL, Label, jit_new_native_label(siz == 8 ? jit->read8 : siz == 16 ? jit->read16 : jit->read32));

	// pop arguments
	emit(ADD, ESP, Imm, 4);
}
static void gen_mem_write(struct jit_ctl *jit, int siz, data32_t addr, int rd, int rn, data32_t immAddr, int ofs)
{
	// Push the register value as the operand.  Special case:  if the source
	// register is R15, use the emulated instruction address plus 12.  (Note
	// that this isn't the usual +12 - it seems to be ahead by even more than
	// usual, according to the manual, when used as the right operand of a
	// load of any kind, including the half-word and byte loads.)
	if (rd == 15)
		emit(PUSH, Imm, addr + 12);
	else
		emit(PUSH, DwordPtr, Rn(rd));

	// push the address
	if (rn == Imm) {
		emit(PUSH, Imm, immAddr + ofs);
	}
	else {
		emit(PUSH, rn);
		if (ofs != 0)
			emit(ADD, DwordPtr, Idx, SP, Imm, ofs);
	}

	// call the emulator memory writer
	emit(CALL, Label, jit_new_native_label(siz == 8 ? jit->write8 : siz == 16 ? jit->write16 : jit->write32));

	// pop arguments
	emit(ADD, ESP, Imm, 8);
}

// Generate a memory access (read or write).
//
// Writes/reads register 'rd' to/from the emulated memory address in 'nativeReg'
// (this is one of the code emitter register constants - EAX, EBX, etc).  The
// caller is responsible for emitting code to load nativeReg with the address
// before calling this function.  If reading/writing a pre-calculated address
// (rather than one that needs to be calculated at run-time), set nativeReg
// to the special value 'Imm' (immediate) and pass the pre-calculated address
// in 'immAddr'.  immAddr is ignored if nativeReg is a real register.
//
// 'addr' is the instruction address.
//
// 'siz' is the bit size of the operand - 8, 16, 32, or 64.  If 'sx' is true,
// bytes and half-words will be sign-extended to 32-bits; otherwise they'll
// be zero-extended.
//
// The 64-bit operand size is for the LDRD and SDRD instructions, which operate
// on Rd and Rd+1 together.
//
// 'sp_inc' is needed for the ABORT test generator.  That test can return to the
// emulator, which requires doing a RETN, which requires that any temp items on
// the stack are removed first.  sp_inc is the number of bytes of temp items
// placed there by the caller that we would need to remove.
static void gen_mem(struct jit_ctl *jit, int rd, int ld, int siz, int sx, int addr, int *is_br,
					int nativeReg, data32_t immAddr, int sp_inc)
{
	if (ld)
	{
		// Loading register 'rd':  generate the memory reader call to load the value.
		gen_mem_read(jit, siz, nativeReg, immAddr, 0);

		// for a 64-bit operand, we need to do a second memory access
		if (siz == 64) {
			emit(PUSH, EAX);         // save the low 32 bits on the stack
			gen_mem_read(jit, siz, nativeReg, immAddr, 4);
		}

		// test ABORT - do this before loading the result into the output register
		gen_test_abort(jit, addr, sp_inc);

		// Store the result in the stack slot for the register.  Special case: if writing
		// R15, generate a jump to the new address, which we'll have to look up at run-time
		// via the pLookup handler.  Note that the target address is already in EAX as
		// required to invoke the pLookup handler.
		if (rd == 15) {
			emit(JMP, Label, jit_new_native_label(jit->pLookup));
			*is_br = 1;
		}
		else
		{
			if (siz == 64)
			{
				// 64-bit operand - EAX has the high 32 bits, and the low 32 bits are on the stack
				emit(MOV, Rn(rd+1), EAX);
				emit(POP, EAX);
			}
			/*else if (siz == 32)
			{
				// 32-bit operand
				// Nothing to do, this was being generated twice.
				// emit(MOV, Rn(rd), EAX);
			}*/
			else if (siz == 16)
			{
				// 16 bits - sign-extend or zero-extend AX to EAX 
				if (sx)
					emit(CWDE);
				else
					emit(MOVZX, EAX, AX);
			}
			else if (siz == 8)
			{
				// 8 bits - sign-extend or zero-extend AL to EAX 
				if (sx)
					emit(MOVSX, EAX, AL);
				else
					emit(MOVZX, EAX, AL);
			}

			// store the result from EAX into the destination register Rd
			emit(MOV, Rn(rd), EAX);
		}
	}
	else
	{
		// Storing register rd.  Generate the memory writer call.
		gen_mem_write(jit, siz, addr, rd, nativeReg, immAddr, 0);

		// Generate the second memory write for a 64-bit operand
		if (siz == 64) {
			// NB - it's not clear if we should do an ABORT test here.  I'm
			// going to assume that we should, on the basis that ARM7 has a
			// 32-bit data bus, hence the physical implementation has to do
			// two bus cycles to execute a STRD, hence an ABORT on the first
			// cycle would presumably prevent the second from occurring.
			// It seems like an edge case to me anyway; the only way it
			// would matter is if the qword is straddling a page boundary
			// such that the first write is invalid and the second one is
			// valid.  If the ABORT on the first half doesn't block the
			// second half write on the real hardware, the instruction would
			// end up writing the second half before taking the ABORT trap.
			// This seems undesirable.  On the other hand, if we're
			// straddling a page boundary in the other direction, such that
			// the first half write succeeds and the second ABORTs, we'll
			// also leave things half done.  That much does seem to be the
			// case, as there doesn't seem to be any way that the two writes
			// could be atomic.  Again, it's a real edge case; the typical
			// scenario like this is that the OS ABORT trap handler maps in
			// the missing page and retries the operation, so the half write
			// that we finished is just repeated harmlessly.  But if the OS
			// remaps the *other* page, the one where the half write succeeded,
			// that could cause some weirdness.
			gen_test_abort(jit, addr, sp_inc);

			// write the high-order 32 bits
			gen_mem_write(jit, siz, addr, rd+1, nativeReg, immAddr, 0);
		}

		// test ABORT
		gen_test_abort(jit, addr, sp_inc);
	}
}

// halfword data transfer and byte data transfer
static int LDRHB_STRHB(struct jit_ctl *jit, data32_t addr, data32_t insn, int *is_br, int *cycles)
{
	int rn, rd, rm;
	int ld, up, isImmOfs, wrt, preIdx;
	int siz, sx;
	data32_t immOfs;
	int ddinstr;
	int non_priv_access_mode;   // flag: the memory transfer uses non-privileged memory mode (documentary only)

	// Decode the instruction fields
	ld = (insn & INSN_SDT_L);
	rn = (insn & INSN_RN) >> INSN_RN_SHIFT;
	rd = (insn & INSN_RD) >> INSN_RD_SHIFT;
	wrt = (insn & INSN_SDT_W) && rn != 15;
	siz = (insn & 0x20) ? 16 : 8;
	sx = (insn & 0x40);
	isImmOfs = (insn & 0x400000);     // Bit 22 = offset type - 1 = immediate, 0 = register
	immOfs = (((insn >> 8) & 0x0f) << 4) | (insn & 0x0f); // immediate value in high nibble (bits 8-11) and lo nibble (bit 0-3)
									  // note: immediate offset is valid only if isImmOfs is true
	rm = insn & 0x0f;                 // offset register; valid only if isImmOfs is false
	preIdx = (insn & INSN_SDT_P);
	up = insn & INSN_SDT_U;
	ddinstr = (!ld && ((insn & 0x60) == 0x40 || (insn & 0x60) == 0x60));

	// If we have a double-word instruction (LDRD, STRD), the encoding is special.
	// NOTE!  These instructions aren't exercised in any Whitestar code.
	if (ddinstr)
	{
		// the load/store status isn't in the usual place - it's in !(bit 6)
		ld = ((insn & 0x60) == 0x40);

		// note the 64-bit operand size
		siz = 64;
	}

	// The W bit has a special meaning in post-indexing mode: it sets non-privileged
	// memory access if the CPU is running in a privileged mode.  The MAME ARM7
	// emulator doesn't appear to make any provisions for this case, and we don't
	// do anything with it here, but I'm noting it explicitly in case it becomes
	// important in the future.
	non_priv_access_mode = wrt && !preIdx;

	// Because of the special meaning of the W bit in post-indexing mode, post-indexing
	// ALWAYS uses write-back.
	wrt |= !preIdx;

	// it appears that byte stores aren't allowed - turn this into a halfword store
	if (!ld && siz == 8)
		siz = 16;

	// Figure if we have a moot write-back.  There's no need to do the write-back if:
	//   - Rn == R15 - this isn't allowed
	//   - we have an immediate 0 offset - the index register isn't changed, so there's no
	//     need to actually write it back
	//   - loading into Rd, and Rn == Rd - the writeback is pipelined before the memory
	//     load, so the memory load overwrites the write-back value; we can skip it entirely
	if (rn == 15 || (isImmOfs && immOfs == 0) || (ld && rd == rn))
		wrt = 0;

	// If we're doing an R15-relative operation with a constant address, we
	// can short-circuit most of the work
	if (rn == 15 && immOfs && preIdx) {
		gen_mem(jit, rd, ld, siz, sx, addr, is_br, Imm, addr+8, 0);
		return 1;
	}

	// Get the base register (Rn), and load the contents into EBX.
	// If Rn is R15, use the instruction address+8 as usual.
	if (rn == 15)
		emit(MOV, EBX, Imm, addr + 8);
	else
		emit(MOV, EBX, Rn(rn));

	// If post-indexing, do the load/store now, ahead of the indexing operation
	if (!preIdx && !ddinstr)
	{
		// If we're not writing back, the memory operation is the only effect, so
		// we can just generate it and return.  Otherwise, we'll have to save the
		// index register so that we can update it and write it back.
		if (!wrt)
		{
			// no write-back (or no effect from write-back) - just do the memory operation and return
			gen_mem(jit, rd, ld, siz, sx, addr, is_br, EBX, 0, 0);
			return 1;
		}
		else
		{
			// we do need to do the write-back - save the index, do the memory access,
			// and restore the index
			emit(PUSH, EBX);
			gen_mem(jit, rd, ld, siz, sx, addr, is_br, EBX, 0, 4);
			emit(POP, EBX);
		}
	}

	// Do the indexing operation - add/subtract the offset to/from EBX
	if (isImmOfs)
	{
		if (immOfs != 0)
		{
			if (up)
				emit(ADD, EBX, Imm, immOfs);
			else
				emit(SUB, EBX, Imm, immOfs);
		}
	}
	else
	{
		// register
		if (up)
			emit(ADD, EBX, Rn(rm));
		else
			emit(SUB, EBX, Rn(rm));
	}

	// If pre-indexing, we have the adjusted index, so do the memory operation
	if (preIdx)
	{
		int sp_inc = 0;

		// If we're going to do a write-back of the index register, save it on the stack.
		// We can't just do the write-back first, because the memory operation could trigger
		// an ABORT, in which case the write-back is skipped.
		if (wrt) {
			emit(PUSH, EBX);
			sp_inc += 4;
		}
		
		// perform the memory operation
		gen_mem(jit, rd, ld, siz, sx, addr, is_br, EBX, 0, sp_inc);

		// if writing back, restore the index value
		if (wrt) {
			emit(POP, EBX);
			sp_inc -= 4;
		}
	}

	// if writing back the index register, it's finally time
	if (wrt)
		emit(MOV, Rn(rn), EBX);

	// LDR(H,SH,SB) PC takes 2S+2N+1I (5 total cycles)
	// STRH takes 2 cycles
	if (ld && rd == 15)
		*cycles += 2;
	else if (!ld && siz == 16)
		*cycles -= 1;

	gen_test_irq(jit, addr);

	// successful translation
	return 1;
}

// Generate code for the shift for an ALU, LDR, or STR instruction.
// The generated code loads the shifted value into EAX.  It also overwrites ECX.
//
// If carry_out is true, it means that the result carry flag is significant
// to the caller and must follow ARM rules.
//
// Returns true if there's a carry out that is or might be different from the
// carry in, false otherwise.  There's definitely no carry out for LSL #0
// (with immediate shift amount rather than register-specified shift), since
// this operation is defined as preserving the carry-in.  All other immediate
// shift amounts alter the carry, and all register-specified shifts *might*
// affect the carry (we can't know at compile time because the shift amount
// is specified in a register).
static int genShift(data32_t insn, data32_t addr, int carry_out)
{
	data32_t k	= (insn & INSN_OP2_SHIFT) >> INSN_OP2_SHIFT_SHIFT;	          // shift amount - #k or Rk (bits 11-7)
	data32_t rm	= insn & INSN_OP2_RM;                                         // shift register Rm (bits 
	data32_t t	= (insn & INSN_OP2_SHIFT_TYPE) >> INSN_OP2_SHIFT_TYPE_SHIFT;  // shift type (bits 6-4)
	int shift_by_reg = (t & 1);                                               // shift amount is specified by register Rk
	int shift_op = (t >> 1);                                                  // shift opcode
	int pcofs = 0;                                                            // pipelining R15 offset, added to rm if rm is R15
	int alters_carry = 1;                                                     // presume could affect the carry

	// Special case: if rm is R15, and a register is used to specify the shift,
	// the PC value will be 12 bytes ahead; otherwise it's 8 bytes ahead.  (This
	// is a quirk of the ARM pipelining.)
	if (rm == 15)
		pcofs = (t & 1) ? 12 : 8;

	// The carry flag on input is visible in the results in certain cases:
	//
	// 1. The nominal ROR #0 actually means RRX #1, rotate right extend,
	// which feeds the carry flag into the shift result.
	//
	// 2. For LSL #0, the carry in is passed through to the carry out.  This
	// only matters if the caller cares about the carry out.
	//
	// 3. For <any op> with a register-specified shift, the carry *could be*
	// passed through to the carry out, since a 0 shift passes through the
	// carry flag for all ops.  This only matters if the caller cares about
	// the carry out.
	//
	// In any of these cases, we have to pre-load the Intel carry flag with
	// the CPSR carry flag.  Note that we have to do this *before* loading
	// the shift value, since 
	if ((!shift_by_reg && shift_op == 3 && k == 0)                  // case 1: ROR #0
		|| (!shift_by_reg && shift_op == 0 && k == 0 && carry_out)  // case 2: LSL #0 and caller uses carry out
		|| (shift_by_reg && carry_out))                             // case 3: register-specified shift and caller uses carry out
	{
		// ROR #0 == RRX #1 - pre-load the Intel carry flag with the CPSR C flag
		cpsr_to_carry();
	}

	// Load the shift register (Rm) value into EAX.
	// (NB: Intel MOV doesn't affect flags, so the carry bit we loaded above survives this.)
	if (rm == 15)
		emit(MOV, EAX, Imm, addr + pcofs);
	else
		emit(MOV, EAX, Rn(rm));

	// if the low bit of t is 1, the shift amount is specified in a register Rk;
	// otherwise it's an immediate value #k
	if (shift_by_reg)
	{
		byte iop;
		struct jit_label *l1, *l2;
		
		// shift amount is in register Rk - get the register number and load
		// its bottom 8 bits into CL (zero-extending to 32 bits)
		int rk = k >> 1;
		emit(MOVZX, ECX, BytePtr, Rn(rk));

		// Figure the shift type.  The ARM shifts map onto the Intel shifts
		// directly:
		//    0 = ARM LSL -> Intel SHL
		//    1 = ARM LSR -> Intel SHR
		//    2 = ARM ASR -> Intel SAR
		//    3 = ARM ROR -> Intel ROR
		//
		// The semantics of the shifts are *almost* identical, including the
		// carry-out treatment.  The only difference is in handling shifts
		// of >= 32 bits.  On ARM, the result is the same as shifting by
		// 32 + k MOD 32.  On Intel, the result is the same as shiftting by
		// k MOD 32.  So the difference is that extra 32-bit shift.  On
		// Intel, there's unfortunately no way to specify a 32-bit shift in
		// one instruction because of that very MOD rule.  But we can get
		// the same effect by doing two smaller shifts that add up to 32.
		// So to get the exact ARM handling, we have to generate extra run-
		// time code that does this:
		//
		//   If k >= 32 { shift by 31, shift by 1 }
		//   shift by k
		//
		// Note that there's no need to reduce k MOD 32, since the shift
		// instruction does that inherently.  There's also no need to do
		// extra loops for even higher k values, because the results are
		// always mod 32.
		//
		// Note that we choose 31+1 as the two shifts that add up to 32.  Any
		// combination of positive values that add up to 32 will have the same
		// effect, but choosing 1 for one of them is efficient because the
		// Intel shift opcodes all have special compact forms for the shift-by
		// immediate value 1.  (It's not efficient to do 32 of them in a row,
		// though; the shift by #31 form is only a byte longer.)

		// first, figure the Intel shift operator we're going to use
		switch (shift_op)
		{
		case 0:
			// 0 = LSL (logical shift left)
			// LSL <32   = Carry out = last bit shifted out
			// LSL  32   = Result = 0, Carry out = Bit 0 of RM
			// LSL >32   = Result = 0, Carry out = 0
			// These are all the natural results of the Intel SHL when augmented
			// with our excess-32 code described above.
			iop = imSHL;
			break;

		case 1:
			// 1 = LSR (logical shift right)
			// LSR <32   = Carry out = last bit shifted out
			// LSR  32   = Result = 0, Carry out = Bit 31 or RM
			// LSR >32   = Result = 0, Carry out = 0
			// These are the natural results of Intel SHR (augmented as above).
			iop = imSHR;
			break;

		case 2:
			// 2 = ASR (arithmetic shift right)
			// ASR <32   = Carry out = last bit shifted out
			// ASR >=32  = Result ~0, Carry out 1
			// This is the natural results of the Intel SAR (augmented as above).
			iop = imSAR;
			break;

		case 3:
			// 3 = ROR (rotate right)
			// Same effect as intel ROR (augmented as above).
			iop = imROR;
			break;
		}

		// Generate the augmented excess-32 shift code:
		//   if (CL > 32) { shift by 31, shift by 1 }
		//   shift by CL
		//
		// Note that if the caller doesn't care about the carry-out, it's
		// okay to clobber the flags with the CMP to test if CL > 32, since
		// none of the register-specified shifts has a carry-in.  If the caller
		// does care about the carry-out, however, we have to check for the
		// special case where the shift amount is zero, because in this case
		// the carry-out equals the carry-in.  That's important because we
		// *can't* clobber the flags - we have to preserve the carry-in.  We
		// can fortuntely do this with a JECXZ, which tests ECX against 0
		// without affecting any flags.  If ECX == 0, jump around the whole
		// operation, since the shift will have no effect.
		if (carry_out)
			emit(JECXZ, Label, l2 = jit_new_fwd_label());

		// Now handle shifts of 32 and higher.  We can safely clobber the flags
		// with the CMP because we either don't care about the carry-in at all,
		// or we know courtesy of the JECXZ that we have a non-zero shift, meaning
		// that the carry-out will come from the shift, not the carry-in.
		emit(CMP, CL, Imm, 32);      // if CL < 32...
		emit(JB, Label, l1 = jit_new_fwd_label());  // ... then jump to final SHIFT MOD 32
		emitv(iop, EAX, Imm, 31);    // SHIFT 31 ...
		emitv(iop, EAX, Imm, 1);     // ... + SHIFT 1 effective makes SHIFT 32

		// And finally, do the SHIFT MOD 32.  All cases except 0 up here.
		// If we had a shift amount >= 32, we just shifted by 32, so we only
		// have to do the remaining MOD 32 bits.  If we started with <32, we'll
		// come directly here to do the full shift.
		jit_resolve_label(l1);
		emitv(iop, EAX, CL);

		// if we have a carry-out, come here (skipping the shift) if shift-by is 0
		if (carry_out)
			jit_resolve_label(l2);
	}
	else
	{
		// Immediate shift amount.  These are similar to the register-specified
		// shifts, but have some additional special cases when the shift amount
		// is #0.  Note an immediate shift-by value is always in the 0-31 range,
		// because the instruction field only has 5 bits - we don't need to worry
		// about the >=32 cases here (except to the extent that some of the
		// special #0 cases actually mean #32 - see below).
		switch (shift_op)
		{
		case 0:
			// 0 = LSL (logical shift left)
			// LSL 0  = no effect on value or carry flag
			// Same effect as Intel SHL.  If k == 0, the shift has no effect, so
			// we can simply omit it.  Note also that k == 0 has no effect on the
			// carry flag - carry out == carry in for this case.
			if (k == 0)
				alters_carry = 0;
			else
				emit(SHL, EAX, Imm, k);
			break;

		case 1:
			// 1 = LSR (logical shift right).
			// Same effect as Intel SHR.
			// SPECIAL CASE: LSR #0 actually means LSR #32.  This has the same effect
			// as 32 bits of Intel right shifting, but because the Intel SHR instruction
			// masks the shift amount to 5 bits, we can't do a 32-bit shift in a single
			// instruction.  Use the 31+1 trick we use for the register shifts above.
			if (k == 0) {
				emit(SHR, EAX, Imm, 31);
				emit(SHR, EAX, Imm, 1);
			}
			else
				emit(SHR, EAX, Imm, k);
			break;

		case 2:
			// 2 = ASR (arithmetic shift right)
			// Same effect as Intel SAR.
			// SPECIAL CASE: ASR #0 actually means ASR #32.  As above, this has the same
			// effect as 32 bits of Intel SAR shifting, but because of the Intel operand
			// masking, we need to do the 31+1 trick.
			if (k == 0) {
				emit(SAR, EAX, Imm, 31);
				emit(SAR, EAX, Imm, 1);
			}
			else
				emit(SAR, EAX, Imm, k);
			break;

		case 3:
			// 3 = ROR (rotate right)
			// Same effect as Intel ROR.
			//
			// SPECIAL CASE: ROR #0 actually means RRX #1 (rotate right extend), which
			// is equivalent to Intel RCR (rotate through carry right).  RRX and RCR
			// explicitly shift the carry flag in to the high bit, so we need to load
			// the ARM carry flag into the Intel EFLAGS before doing the RCR.  We
			// already did this above, so we just need to do the RCR now.
			if (k == 0) {
				// ROR #0 -> RRX #1 -> Intel RCR #1
				emit(RCR, EAX, Imm, 1);
			}
			else {
				// anything else is ROR #n -> Intel ROR #n
				emit(ROR, EAX, Imm, k);
			}
		}
	}

	// tell the caller if the generated code changes or could change the carry flag
	return alters_carry;
}


// Helper routine for MOVS R15,rn.  This opcode has the special side effect of
// loading CPSR from SPSR and switching modes before doing the branch implied
// by the R15 load.
static void spsr_to_cpsr(void)
{
	// this operation isn't allowed in user mode
	if (GET_MODE != eARM7_MODE_USER)
	{
		// load CPSR from SPSR
		SET_CPSR(GET_REGISTER(SPSR));
		// do the mode switch
		SwitchMode(GET_MODE);
	}
	ARM7_CHECKIRQ;
}


// Emit code for a jump to an emulator address.  We normally generate the cycle count
// update at the end of an instruction's generated code, but we have to generate it
// BEFORE a jump, since it would otherwise be unreachable.  For a jump, we also check
// to see if we've run out of cycles.  This is necessary because a jump could be part
// of a loop that's waiting for an external event, in which case we'd get stuck in
// the loop if we didn't check for cycle exhaustion.  MAME is single-threaded, so 
// external events can't update without an emulator yielding periodically.  Of course,
// most jumps aren't actually part of this kind of loop, so this check adds some
// theoretically unnecessary overhead most of the time.  But the translator has to
// be conservative, because it can't see the big picture - we have to assume that any
// jump could be the kind that can get us stuck in a loop.
static void emit_jump(struct jit_ctl *jit, data32_t dest_addr, int *cycles)
{
	// update the cycle counter
	emit(SUB, RCYCLECNT, Imm, *cycles);

	// if the cycle counter didn't reach zero (or below), proceed to the destination address
	emit(JG, Label, jit_new_addr_label(dest_addr));

	// If we get here, it means that the cycle count reached zero.  We're still
	// going to branch, but do it in the emulator instead of going to native code.
	emit(MOV, EAX, Imm, dest_addr);
	emit(RETN);

	// we've now generated the cycle count - zero the counter so tha main loop knows
	// it doesn't need to do this again
	*cycles = 0;
}

// arithmetic operation (add, sub, eor, orr, and, cmp, etc)
static int ALU(struct jit_ctl *jit, data32_t addr, data32_t insn, int *is_br, int *cycles)
{
	data32_t opcode;
	int result_reg;
	int is_logical_op;
	int is_sub_op;
	int is_test_op;
	int need_shift_carry_out;
	//struct jit_label *lbl;

	// Normal data processing is 1 cycle - reduce the default 3-cycle count by 2
	*cycles -= 2;

	// get the ALU operation
	opcode = (insn & INSN_OPCODE) >> INSN_OPCODE_SHIFT;

	// note the class of operation - logical (AND, EOR, TST, TEQ, ORR, MOV, BIC, MVN)
	// or arithmetic (SUB, RSB, ADD, ADC, SBC, RSC, CMP, CMN)
	is_logical_op = ((opcode & 6) == 0 || (opcode & 0xC) == 0xC);

	// note if it's a subtractive op (SUB, RSB, SBC, RSC, CMP, CMN)
	is_sub_op = ((opcode & 2) == 2 && (opcode & 8) != 8) || opcode == 0xA;

	// note if it's a test op with no stored result (TST, TEQ, CMP, CMN)
	is_test_op = ((opcode & 0xC) == 8);

	// If we have a logical op with the S flag set, we will normally have to set the
	// CPSR carry flag to the carry out from the op2 shift.  (There's an exception,
	// though, which is why we need this variable: if op2 is an unshifted constant,
	// the carry out from the shifter will be the carry in from CPSR, so the end
	// result is that the CPSR carry is unaffected.  In that case we can omit the
	// extra code needed to update the CPSR carry.)
	need_shift_carry_out = (is_logical_op && (insn & INSN_S));

	// If this is a "with carry" operation we need to get carry from CPSR and put it into
	// the intel carry flag.
	if (opcode == OPCODE_SBC || opcode == OPCODE_RSC || opcode == OPCODE_ADC)
	{
		cpsr_to_carry();
		if (is_sub_op)
			emit(CMC);
	}

	// figure OP2
	if (insn & INSN_I)
	{
		// Immediate OP2, with possible immediate shift
		data32_t op2;

		// get the shift-by amount
		data32_t by = (insn & INSN_OP2_ROTATE) >> INSN_OP2_ROTATE_SHIFT;
		if (by)
		{
			// shifted immediate - apply the shift
			op2 = ROR(insn & INSN_OP2_IMM, by << 1);

			// If this is a logical operator with the S flag, the carry flag is the
			// carry out from the shifter; for ROR this is the high bit of the result.
			// We can update the CPSR C bit now, since the rest of the operation won't
			// affect the carry bit.
			if (need_shift_carry_out)
			{
				// set or clear the CPSR C bit, according to the high bit of the immediate result
				if (op2 & SIGN_BIT)
					emit(OR, DwordPtr, RCPSR, Imm, C_MASK);
				else
					emit(AND, DwordPtr, RCPSR, Imm, ~C_MASK);
			}
		}
		else {
			// unshifted immediate - get the value
			op2 = insn & INSN_OP2_IMM;

			// If this is a logical operator with the S flag, the carry out from the
			// shift is simply the carry in, which is the current CPSR carry flag.
			// So the net effect will be nil - the CPSR carry flag will be unaffected.
			// We can therefore omit the code we'd normally generate to update the
			// CPSR carry flag for a Logical+S operation.
			need_shift_carry_out = 0;
		}

		// load the operand into EAX
		emit(MOV, EAX, Imm, op2);
	}
	else
	{
		// OP2 is a shifted register.  Decode the shift and generate code.
		// This will leave the shifted value in EAX.  If the generated code
		// can't affect the carry, there's no need to update it even if we're
		// saving result flags, since the new carry out of the shifter is
		// known to be the same as the old carry in this case.
		if (genShift(insn, addr, need_shift_carry_out) && need_shift_carry_out)
		{
			// Logical operators with the S flag save the carry from the shift to CPSR.
			// We can do so now in this case, since the rest of the operation won't affect
			// the carry bit.
			//if (need_shift_carry_out) //already checked above
			{
				carry_to_cpsr();
			}
		}

		// extra cycle (register specified shift)
		*cycles += 1;
	}

	// Get Rn into EBX, if applicable.  This is the left-hand operand for a
	// two-operand operator.  This applies to all instructions except MOV and MVN.
	if (opcode != OPCODE_MOV && opcode != OPCODE_MVN)
	{
		int rn = (insn & INSN_RN) >> INSN_RN_SHIFT;
		if (rn == 15) {
			int addpc = ((insn & INSN_I) ? 8 : (insn & 0x10u) ? 12 : 8);
			emit(MOV, EBX, Imm, addr + addpc);
		}
		else {
			emit(MOV, EBX, Rn(rn));
		}
	}



	// Perform the operation.  We have rn in EBX and op2 in EAX, so for most operations
	// it will be most efficient to leave the result in EBX.  There are some reverse
	// operations that will leave it in EAX.
	result_reg = EBX;
	switch (opcode)
	{
	/*
	 *   Arithmetic operations
	 */
	case OPCODE_SBC:
		// Subtract with carry.  ARM SBC uses the carry flag in the opposite sense
		// of Intel SBB, but we can use SBB if we complement the carry first.
		emit(SBB, EBX, EAX);
		break;

	case OPCODE_CMP:
		// CMP - works just like Intel CMP
		emit(CMP, EBX, EAX);
		break;

	case OPCODE_SUB:
		// SUB - works just like Intel SUB
		emit(SUB, EBX, EAX);
		break;

	case OPCODE_RSC:
		// RSC - reverse subtract with carry: computes op2 - rn
		emit(SBB, EAX, EBX);
		result_reg = EAX;
		break;

	case OPCODE_RSB:
		// RSB - reverse subtract: computes op2 - rn
		emit(SUB, EAX, EBX);
		result_reg = EAX;
		break;

	case OPCODE_ADC:
		// ADC - add with carry - works just like Intel ADC
		emit(ADC, EBX, EAX);
		break;

	case OPCODE_CMN:
		// CMN - add without storing result; do the addition for the sake of flags
		emit(ADD, EBX, EAX);
		break;

	case OPCODE_ADD:
		// ADD - same as Intel ADD
		emit(ADD, EBX, EAX);
		break;

	/* 
	 *   Logical operations 
	 */
	case OPCODE_AND:
		// AND - works like Intel AND
		emit(AND, EBX, EAX);
		break;

	case OPCODE_TST:
		// TST - works like Intel TEST
		emit(TEST, EBX, EAX);
		break;

	case OPCODE_BIC:
		// BIC - rn & ~op2
		emit(NOT, EAX);
		emit(AND, EBX, EAX);
		break;

	case OPCODE_TEQ:
		// TEQ - XOR for flags only, without saving results
		emit(XOR, EBX, EAX);
		break;
		
	case OPCODE_EOR:
		// EOR - equivalent to Intel XOR
		emit(XOR, EBX, EAX);
		break;

	case OPCODE_ORR:
		// ORR - Intel OR
		emit(OR, EBX, EAX);
		break;
		
	case OPCODE_MOV:
		// MOV - moves op2
		result_reg = EAX;
		// If the S flag is set, we need to TEST the result.  
		// Intel MOV does not set flags.
		if ((insn & INSN_S))
			emit(TEST, EAX, EAX);
		break;

	case OPCODE_MVN:
		// MVN - moves ~op2
		emit(NOT, EAX);
		// If the S flag is set, we need to TEST the result.  
		// Intel MOV does not set flags.
		if ((insn & INSN_S))
			emit(TEST, EAX, EAX);

		result_reg = EAX;
		break;
	}

	// save flags to CPSR if desired
	if ((insn & INSN_S))
	{
		// Logical and arithmetic ops have different treatments for the flags
		if (is_logical_op)
		{
			// Logical operator.  ARM has different rules from Intel:
			//   V is unaffected
			//   C is the carry out from the shift - we've already applied this
			//   Z is set if the result is 0 (same as Intel result from operator)
			//   N is set if bit 31 of the result is 1 (same as Intel result from operator)
			emit(SETZ, CL);           // ECX = xxxxxxxx xxxxxxxx xxxxxxxx 0000000Z
			emit(SETS, CH);           //       xxxxxxxx xxxxxxxx 0000000N 0000000Z
			emit(SHL, CH, Imm, 1);    //       xxxxxxxx xxxxxxxx 000000N0 0000000Z
			emit(OR, CL, CH);         //       xxxxxxxx xxxxxxxx 000000N0 000000NZ
			emit(SHL, ECX, Imm, 30);  //       NZ000000 00000000 00000000 00000000
			emit(AND, DwordPtr, RCPSR, Imm, 0x3FFFFFFF);
			emit(OR, RCPSR, ECX);
		}
		else
		{
			// Arithmetic operator: ARM and Intel have the same rules, except that the
			// carry flag has the reverse sense if the operation was subtractive (SUB, SBC,
			// RSB, RSC, CMP).  So invert the carry flag if the opcode was subtractive, then
			// copy the Intel N, Z, C, and V flags into the CPSR.
			if (is_sub_op)
				emit(CMC);

			// we need to overwrite EAX to do the flag storage, so if the result ended up
			// here, swap it into EBX
			if (result_reg == EAX) {
				emit(XCHG, EAX, EBX);
				result_reg = EBX;
			}

			// To convert the Intel flags to the ARM format, we need to rearrange the
			// bits into the proper order.  Here's the algorithm:
			//   LAHF          xxxxxxxx xxxxxxxx NZxxxxxC xxxxxxxx
			//   SETO AL       xxxxxxxx xxxxxxxx NZxxxxxC 0000000V
			//   SHL AL,7      xxxxxxxx xxxxxxxx NZxxxxxC V0000000
			//   SHR EAX, 1    0xxxxxxx xxxxxxxx xNZxxxxx CV000000
			//   SHR AH, 5     0xxxxxxx xxxxxxxx 00000xNZ CV000000
			//   SHL EAX, 22   NZCV0000 00000000 00000000 00000000

			flags_to_cpsr();
		}
	}


	/* Put the result in its register if not one of the test only opcodes (TST,TEQ,CMP,CMN) */
	if (!is_test_op)
	{
		// get the destination register
		int rd = (insn & INSN_RD) >> INSN_RD_SHIFT;

		// if rd is R15, this is a jump to the calculated address
		if (rd == 15)
		{
			// If Rd = R15 and S flag is set, current mode SPSR is moved to CPSR
			if (insn & INSN_S)
			{
				// When Rd is R15 and the S flag is set the result of the operation is placed in R15 and the SPSR corresponding to
				// the current mode is moved to the CPSR. This allows state changes which automatically restore both PC and
				// CPSR. --> This form of instruction cannot be used in User mode. <--

				// save the result register (the destination address for the branch) on the stack
				// (djrobx - the mode change may result in R15 pointing to an IRQ handler, so we need to 
				// move it there, and look at result when it comes back. ) 
				//emit(PUSH, result_reg);
				emit(MOV, Rn(rd), result_reg);

				// call our helper routine to load SPSR into CPSR and switch modes.  Also check if run count exhausted.
				emit(CALL, Label, jit_new_native_label((byte *)&spsr_to_cpsr));
				emit(MOV, EAX, Rn(15));
				/*emit(CMP, RCYCLECNT, Imm, 0);
				emit(JG, Label, lbl = jit_new_fwd_label());
				emit(RETN);
				jit_resolve_label(lbl);*/
				emit(JMP, Label, jit_new_native_label(jit->pLookup));		
			}
			else
			{
				// Jump to the result value by loading it into R15.  Check IRQs and run count.
				emit(MOV, Rn(rd), result_reg);
				emit(CALL, Label, jit_new_native_label((byte *)&arm7_check_irq_state));
				emit(MOV, EAX, Rn(15));
			/*	emit(CMP, RCYCLECNT, Imm, 0);
				emit(JG, Label, lbl = jit_new_fwd_label());
				emit(RETN);
				jit_resolve_label(lbl);*/
				emit(JMP, Label, jit_new_native_label(jit->pLookup));
			}
				
			// tell our caller this is effectively a branch instruction
			*is_br = 1;
			
			// extra cycles (PC written)
			*cycles += 2;
		}
		else
		{
			// for all other registers, just write the result
			emit(MOV, Rn(rd), result_reg);
		}
	}
	gen_test_irq(jit, addr);
	// successful translation
	return 1;
}

// LDR or STR (single register memory transfer, load or store)
static int LDR_STR(struct jit_ctl *jit, data32_t addr, data32_t insn, int *is_br, int *cycles)
{
	int rd = (insn >> 12) & 0x0F;   // source/destination register
	int rn = (insn >> 16) & 0x0F;   // base register
	int ld = insn & (1 << 20);      // load/store bit (true -> load register)
	int wrt = insn & (1 << 21);     // write-back bit (true -> write address into base)
	int byt = insn & (1 << 22);     // byte bit (true -> byte transfer, false -> dword transfer)
	int up = insn & (1 << 23);      // direction bit (true -> up = add offset to base)
	int pre = insn & (1 << 24);     // pre/post bit (true -> pre, add offset before transfer)
	int oty = insn & (1 << 25);     // offset type: 0=immediate offset, 1=register with shift
	data32_t ofs = (oty == 0 ? insn & 0xfff : 0);    // immediate offset, if oty == 0
	int non_priv_access_mode;       // the memory transfer uses non-privileged memory mode (documentary only)

	// The W bit has a special meaning in post-indexing mode: it sets non-privileged
	// memory access if the CPU is running in a privileged mode.  The MAME ARM7
	// emulator doesn't appear to make any provisions for this case, and we don't
	// do anything with it here, but I'm noting it explicitly in case it becomes
	// important in the future.
	non_priv_access_mode = wrt && !pre;

	// Because of the special meaning of the W bit in post-indexing mode, post-indexing
	// ALWAYS uses write-back.
	wrt |= !pre;

	// Write-back only applies if the index register isn't R15, and it's
	// not the same as Rd on a load.  If it's the same as Rd on a load,
	// the ARM pipeline order causes the load to overwrite the index update.
	// We can also skip the write-back if we have an immediate 0 offset, since
	// the write-back would have no effect in this case.
	if (rn == 15 || (ld && rn == rd) || (oty == 0 && ofs == 0))
		wrt = 0;

	// If rn is R15, we can short-circuit most of the run-time work
	if (rn == 15 && oty == 0 && pre)
	{
		// Fold the immediate index into the address, since we know what R15
		// will be at run-time.  This saves us an unnecessary register load.
		gen_mem(jit, rd, ld, byt ? 8 : 32, 0, addr, is_br, Imm, addr + 8 + (up ? ofs : -ofs), 0);
		return 1;
	}

	// Start by loading EBX with the base register value
	if (rn == 15) {
		// For R15, use the instruction address + 8 for pipelining.  NB: the
		// register-specified form of the shift field isn't allowed for LDR/STR,
		// so the PC pipelining offset is always 8.
		emit(MOV, EBX, Imm, addr + 8);
	}
	else {
		// for any register other than R15, simply retrieve the register
		emit(MOV, EBX, Rn(rn));
	}

	// if we're post-indexing, do the memory access now, before modifying the index
	if (!pre)
	{
		// Post-indexing.  There are three possibilities here:
		//  1. We're not writing back the index register.  In this case, the
		//     post-indexing operation has no effect, since it will just do
		//     a calculation that will be discarded.  We can skip generating it.
		//  2. We're writing back, but the offset is an immediate 0 value.
		//     In this case the write-back will have no effect.
		//  3. We're writing back and we have a register or non-zero constant
		//     offset value.  In this case the write-back is meaningful.
		if (!wrt || (oty == 0 && (insn & 0xfff) == 0))
		{
			// not writing back at all, or writing back a manifestly unchanged
			// value - just generate the memory operation and return
			gen_mem(jit, rd, ld, byt ? 8 : 32, 0, addr, is_br, EBX, 0, 0);
			return 1;
		}
		else if (oty == 0) //&& (insn & 0xfff) == 0)
		{
			// writing back a (possibly) changed value - save the index register
			// for the write-back
			emit(PUSH, EBX);

			// do the memory operation
			gen_mem(jit, rd, ld, byt ? 8 : 32, 0, addr, is_br, EBX, 0, 4);

			// restore the index value for the write-back
			emit(POP, EBX);
		}
	}

	// add/subtract the offset register, if applicable
	if (oty == 0) {
		// Simple immediate displacement - add it to the index value
		if (ofs != 0)
			emitv(up ? imADD : imSUB, EBX, Imm, ofs);
	}
	else {
		// Shifted register displacement - generate the shift code, which loads
		// the shift value into EAX.  LDR/STR don't affect the result flags, so
		// we don't care about the carry-out from the shift.
		genShift(insn, addr, 0);

		// add the shifted offset to EBX
		emitv(up ? imADD : imSUB, EBX, EAX);
	}

	// if pre-indexing, we waited until after adjusting the index to load/store
	// the value, so now it's time to do the memory access
	if (pre)
	{
		int sp_inc = 0;                 // bytes of temps we've saved on the stack
		// if writing back the index register, save the updated index for
		// the duration of the call - we'll need it to write back afterwards
		// (we can't do ahead of time because of the possibility of an ABORT)
		if (wrt) {
			sp_inc += 4;
			emit(PUSH, EBX);
		}

		// do the load/store
		gen_mem(jit, rd, ld, byt ? 8 : 32, 0, addr, is_br, EBX, 0, sp_inc);

		// recover the index register if writing back
		if (wrt) {
			emit(POP, EBX);
			sp_inc -= 4;
		}
	}

	// if writing back the index register, do so now
	if (wrt)
		emit(MOV, Rn(rn), EBX);

	// adjust the cycle counter from the default 3, if appropriate:
	//   - load R15 takes 5 cycles
	//   - store takes 2 cycles
	if (ld && rd == 15)
		*cycles += 2;
	else if (!ld)
		*cycles -= 1;

	gen_test_irq(jit, addr);
	// successful translation
	return 1;
}

// SWAP
//
// NOTE! This instruction doesn't seem to be used anywhere in the Whitestar II ROMs,
// so this code hasn't been exercised.  If it causes trouble, try replacing it with
// a simple "return 0" to use emulation for SWAP instructions.  (But keep in mind
// that the emulator SWAP code hasn't been exercised either, for the same reason.)
static int SWAP(struct jit_ctl *jit, data32_t addr, data32_t insn, int *cycles)
{
	//According to manual - swap is an LDR followed by an STR and all endian rules apply
	//Process: Read original data from address pointed by Rn then store data from address
	//         pointed by Rm to Rn address, and store original data from Rn to Rd.
	int rn, rm, rd;
	int siz;

	// get registers
	rn = (insn >> 16) &0xf;
	rm = insn & 0xf;
	rd = (insn >> 12) & 0xf;

	// SWAP can't be used with R15
	if(rn == 15 || rm == 15 || rd == 15)
		return 0;

	// figure the data size - byte or dword
	siz = (insn & 0x400000) ? 8 : 32;

	// retrieve rn into EBX - this is the memory address we're reading and writing
	emit(MOV, EBX, Rn(rn));

	// read a byte/dword from the memory location indexed by rn (now in EBX)
	emit(PUSH, EBX);           // save EBX on the stack for the duration of the call
	emit(PUSH, EBX);           // push Rn index value as argument
	emit(CALL, Label, jit_new_native_label(siz == 8 ? jit->read8 : jit->read32));  // call read8/32
	emit(ADD, ESP, Imm, 4);    // discard arguments
	emit(POP, EBX);            // restore the rn value in EBX

	// The byte/dword we read from the memory location is now in EAX.  Save it for a moment.
	emit(PUSH, EAX);

	// store the contents of rm in the memory location
	emit(PUSH, DwordPtr, Rn(rm));  // push value argument (the contents of Rm)
	emit(PUSH, EBX);           // push address argument (EBX, the contents of Rn)
	emit(CALL, Label, jit_new_native_label(siz == 8 ? jit->write8 : jit->write32)); // call write8/32
	emit(ADD, ESP, Imm, 8);    // discard arguments

	// recover the saved data that we read from te memory location earlier, and store it in Rd
	emit(POP, EAX);
	emit(MOV, Rn(rd), EAX);

	// swap takes 1S+2N+1I cycles - update from default 3
	*cycles += 1;

	// successful translation
	return 1;
}

// branch and exchange
static int BX(struct jit_ctl *jit, data32_t addr, data32_t insn)
{
	// source register = (insn & 0x0F)
	int rn = insn & 0x0f;

	// set EAX to the destination value and jump to pLookup
	emit(MOV, EAX, Rn(rn));
	emit(JMP, Label, jit_new_native_label(jit->pLookup));

	return 1;
}

// PSR (program status register) transfer
static int PSRX(struct jit_ctl *jit, data32_t addr, data32_t insn, int *cycles)
{
	// note if we're working on SPSR or CPSR
	int spsr = (insn & 0x400000);

	// this only takes one cycle
	*cycles -= 2;

	// figure the instruction type - MSR or MRS
	if( (insn & 0x00200000) )
	{
		// push the instruction for the bit mask argument
		emit(PUSH, Imm, insn);

		// MSR ( bit 21 set ) - Copy value to CPSR/SPSR
		if (insn & INSN_I)
		{
			// Immediate value
			// Value can be specified for a Right Rotate, 2x the value specified.
			int by = (insn & INSN_OP2_ROTATE) >> INSN_OP2_ROTATE_SHIFT;
			data32_t val;
			if (by)
				val = ROR(insn & INSN_OP2_IMM, by << 1);
			else
				val = insn & INSN_OP2_IMM;
			
			emit(PUSH, Imm, val);
		}
		else
		{
			// Value from Register
			int rd = insn & 0x0f;
			emit(PUSH, DwordPtr, Rn(rd));
		}
		//emit(MOV, DwordPtr, Rn(15), Imm, addr+4);
		// call HandleMSR(spsr, val, insn) to do the update
		emit(PUSH, Imm, spsr);
		emit(CALL, Label, jit_new_native_label((byte *)&HandleMSR));
		emit(ADD, ESP, Imm, 12);
		// Changing IRQ mask may have caused a branch. 
		//emit(MOV, EAX, Rn(15));
		//emit(JMP, Label, jit_new_native_label(jit->pLookup));
		gen_test_irq(jit, addr);
		//*is_br = 1;
	}
	else
	{
		// MRS ( bit 21 clear ) - Copy CPSR or SPSR to specified Register
		int rd = (insn >> 12) & 0x0f;

		// call HandleMRS(rd, spsr) to retrieve the value
		emit(PUSH, Imm, spsr);
		emit(PUSH, Imm, rd);
		emit(CALL, Label, jit_new_native_label((byte *)&HandleMRS));
		emit(ADD, ESP, Imm, 8);
	}

	// successful translation
	return 1;
}

// LDM or STM (multiple register memory transfer, load or store)
static int LDM_STM(struct jit_ctl *jit, data32_t addr, data32_t insn, int *is_br, int *cycles)
{
	data32_t rb = (insn & INSN_RN) >> INSN_RN_SHIFT; // base register number
	data32_t pat = (insn & 0xffff);                  // register mask
	int ld = insn & INSN_BDT_L;                      // load (1) or store (0)
	int up = insn & INSN_BDT_U;                      // increment direction - up/inc (1) or down/dec (0)
	int preIdx = (insn & INSN_BDT_P);                // pre-index mode (1) or post-index mode (0)
	int S = (insn & INSN_BDT_S);                     // S flag -> set result flags
	int wrt = (insn & INSN_BDT_W);                   // write-back mode
	int user_bank_xfer;                              // S flag set, but R15 not in list = User Bank Transfer
	data8_t *abt = &ARM7.pendingAbtD;                // ABORT flag variable, for testing in generated code
	struct jit_label *lAbort, *lNoAbort;//, *lbl;   // assembler labels for the abort handler
	int rcnt;                                        // number of registers transferred
	int i;

	// Normal LDM instructions take nS + 1N + 1I and LDM PC takes (n+1)S + 2N + 1I
	// incremental cycles, where S,N and I are as defined in 6.2 Cycle Types on page 6-2.
	// STM instructions take (n-1)S + 2N incremental cycles to execute, where n is the
	// number of words transferred.

	// we'll figure the cycle count exactly as we go, so start at 0
	*cycles = 0;

	// Determine if we have a user bank transfer.  It's a user bank transfer if
	// (a) the op is STM and S is set, or (b) the op is LDM and S is set AND R15
	// isn't in the list.
	if (ld)
		user_bank_xfer = S && ((insn & 0x8000) == 0);
	else
		user_bank_xfer = S;
	
	// Don't bother with the write-back for LDM if the base register is in the load list.
	// Per the ARM documentation, the load happens after the write-back in the pipeline,
	// so the index update is lost.  We can just skip generating any code for it.
	if (ld && ((insn >> rb) & 1))
		wrt = 0;

	// count how many registers are in the mask
	for (i = 0, rcnt = 0 ; i < 16 ; ++i) {
		if ((pat >> i) & 1)
			++rcnt;
	}

	// generate a label for ABORT
	lAbort = jit_new_fwd_label();

	// Check for the special user-bank transfer mode
	if (user_bank_xfer)
	{
		// For user-bank transfers, we can't generate much more efficient
		// code than the (load|store)(Inc|Dec)Mode() subroutines, so just
		// call those.

		// If we're storing R15, update the active R15 register with the
		// instruction address + 12.  Note that PC is 12 bytes ahead in an
		// STM, not the usual 8).  Since the storeXxxMode() routines read
		// directly from the active registers, we need to store the R15
		// value before calling the subroutine.
		if (!ld && ((insn >> 15) & 1))
			emit(MOV, DwordPtr, Rn(15), Imm, addr + 12);

		// Get the starting address into EAX.  
		// If we're in post-index mode, we (perversely) need to pre-adjust the
		// base pointer, because the xxxXxxMode() routines all assume pre-index
		// mode as the default, meaning that they inc/dec the register BEFORE
		// they load/store the first register.  To compensate for this, we have
		// to cancel the effect of that initial inc/dec for post-index mode by
		// doing the opposite operation before calling the function.
		emit(MOV, EAX, Rn(rb));
		if (!preIdx)
			emitv(up ? imSUB : imADD, EBX, Imm, 4);
		
		if (ld)
		{
			// figure which function to call
			byte *func = (up? (byte *)loadIncMode : (byte *)loadDecMode);

			// call load(Inc|Dec)Mode(pat, baseAddress, S, eARM7_MODE_USER)
			emit(PUSH, Imm, eARM7_MODE_USER);
			emit(PUSH, Imm, S);
			emit(PUSH, EAX);
			emit(PUSH, Imm, pat);
			emit(CALL, Label, jit_new_native_label(func));
			emit(ADD, ESP, Imm, 16);
		}
		else
		{
			// figure which function to call
			byte *func = (up ? (byte *)storeIncMode : (byte *)storeDecMode);

			// call store(Inc|Dec)Mode(pat, baseAddress, eARM7_MODE_USER)
			emit(PUSH, Imm, eARM7_MODE_USER);
			emit(PUSH, EAX);
			emit(PUSH, Imm, pat);
			emit(CALL, Label, jit_new_native_label(func));
			emit(ADD, ESP, Imm, 12);
		}
	}
	else
	{
		int r, dr;
		int ngen = 0;
		
		// For transfers to/from the active registers, we can generate somewhat more
		// efficient code in-line than we'd get by calling the xxxXxxMode() subroutines.
		// Start by loading ESI with the base register value.  Use ESI because the C
		// compiler calling conventions require this register to be preserved across
		// function calls, so we don't have to worry about the memory read/write
		// functions clobbering it on us.
		emit(MOV, ESI, Rn(rb));

		// If inc mode, work from register R0 to R15, otherwise work backwards
		// from R15 to R0.
		if (up)
			r = 0, dr = 1;
		else
			r = 15, dr = -1;

		// Generate code for each register in the mask
		for (i = 0 ; i < 16 ; ++i, r += dr)
		{
			// generate code if this register is in the mask
			if ((pat >> r) & 1)
			{
				// if this isn't the first register we've generated, bump ESI
				if (ngen++ != 0 || preIdx)
					emitv(up ? imADD : imSUB, ESI, Imm, 4);

				// load or store the register
				if (ld)
				{
					// call read32(ESI)
					emit(PUSH, ESI);
					emit(CALL, Label, jit_new_native_label(jit->read32));
					emit(ADD, ESP, Imm, 4);

					// test ABORT - loading registers stops on abort
					emit(CMP, BytePtr, Idx, Imm, (UINT32)abt, Imm, 0);
					emit(JNE, Label, lAbort);

					// store the result in the current register
					emit(MOV, Rn(r), EAX);
				}
				else
				{
					// Call write32(ESI, Rr).  Note that R15 is special - this is
					// the current instruction address + 12 for STM (not the usual 8).
					if (r == 15)
						emit(PUSH, Imm, addr + 12);
					else
						emit(PUSH, DwordPtr, Rn(r));
					emit(PUSH, ESI);
					emit(CALL, Label, jit_new_native_label(jit->write32));
					emit(ADD, ESP, Imm, 8);

					// Note that the emulator version of Store doesn't stop on abort,
					// so we won't either
				}
			}
		}
	}

	// We're done with the memory transfers.  These could have triggered an ABORT
	// from the memory manager, so check for it.
	emit(CMP, BytePtr, Idx, Imm, (UINT32)abt, Imm, 0);
	emit(JE, Label, lNoAbort = jit_new_fwd_label());

	// generate the abort handler - zero the cycle counter and return to the emulator
	jit_resolve_label(lAbort);
	emit(MOV, RCYCLECNT, Imm, 0);
	emit(MOV, EAX, Imm, addr + 4);
	emit(RETN);

	// no abort - proceed as normal
	jit_resolve_label(lNoAbort);

	// write back the updated index register if applicable
	if (wrt)
		emitv(up ? imADD : imSUB, DwordPtr, Rn(rb), Imm, rcnt * 4);
		
	// check for loading R15
	if (ld && (insn & 0x8000))
	{
		// return 0;

		// loading R15 costs 2 extra cycles
		*cycles += 2;

		// S - Flag Set Signals transfer of current mode SPSR->CPSR
		if (insn & INSN_BDT_S)
			emit(CALL, Label, jit_new_native_label((byte *)&HandleLDMS_ModeChange));

		// Generate a jump to the new R15
		*is_br = 1;
		// Check to see if changed flags causes an IRQ jump
		emit(CALL, Label, jit_new_native_label((byte *)&arm7_check_irq_state));
		emit(MOV, EAX, Rn(15));
	/*	emit(CMP, RCYCLECNT, Imm, 0);
		emit(JG, Label, lbl = jit_new_fwd_label());
		emit(RETN);
		jit_resolve_label(lbl);*/
		emit(JMP, Label, jit_new_native_label(jit->pLookup));
	}

	// LDM (NO PC) takes nS + 1n + 1I cycles (n = # of register transfers)
	// STM takes (n-1)S + 2N cycles (n = # of register transfers)
	if (ld)
		*cycles += (rcnt + 1) + 1;
	else
		*cycles += (rcnt - 1) + 2;

	gen_test_irq(jit, addr);

	// successful translation
	return 1;
}

// branch, branch link
static int B(struct jit_ctl *jit, data32_t addr, data32_t insn, int *is_br, int *cycles)
{
	int link = (insn & INSN_BL);

	// Pull out the offset - this is the bottom 24 bits shifted left by 2
	data32_t ofs = (insn & INSN_BRANCH) << 2;

	// figure the absolute target address - the offset is signed and
	// relative to PC+8
	if (ofs & 0x2000000u)
		ofs = (addr+8) - ((~(ofs | 0xfc000000u)) + 1);
	else
		ofs += addr+8;

	// check for BL
	if (link)
	{
		//struct jit_label *lbl;

		// BL = branch link = subroutine call.  This is a good time to check
		// the cycle counter to see if we need to return to the emulator and
		// yield our emulated time slice.
		/*emit(CMP, RCYCLECNT, Imm, 0);
		emit(JG, Label, lbl = jit_new_fwd_label());
		emit(MOV, EAX, Imm, addr);
		emit(RETN);
		jit_resolve_label(lbl);*/

		// Load R14 with the return address
		emit(MOV, DwordPtr, Rn(14), Imm, addr+4);
	}
	else
	{
		// B = regular branch.  Tell the caller this is a branch op.
		*is_br = 1;
	}

	// We know the current (branch) instruction is potentially reachable,
	// so this makes the instruction at the target of the branch reachable.
	// Recursively translate the target.  If that succeeds, that will also
	// give us the native code address of the target, so that we can jump
	// directly to the target in native code rather than having to go through
	// the mapping table.  Note that 'jump to self' is a special case - we
	// have to skip the recursive translation since that would get us into
	// an infinite loop, but we can jump to our own address.
	if (ofs == addr)
	{
		// Jump to self.  This isn't a good candidate for translation;
		// it's probably a spin loop that expects to be interrupted
		// asynchronously, and the mechanism for testing for interrupts
		// is better left to the emulator.  There's also no performance
		// advantage in doing a spin in native code, as it does no
		// useful work; it just consumes CPU until interrupted.
		return 0;
	}
	else if (link)
	{
		// it's a subroutine call - recursively translate code at the
		// target address
		xlat_recursive(jit, ofs);
		
		// Jump to the target emulated code address.
		emit_jump(jit, ofs, cycles);

		// success
		return 1;
	}
	else
	{
		// Direct branch.  If it's a forward branch, it could be reachable
		// sequentially from the current instruction, in which case we'll
		// get to it in the current translation loop.  It's actually better
		// not to recursively translate in this case because doing so would
		// put the target of the branch into another code block, requiring
		// a jump that we could avoid by keeping it in the current code block.
		// So queue the address if it's a forward jump.  If it's a backwards
		// jump, there's no way we'll get to it sequentially, so we can just
		// do it recursively to try to resolve its native code address
		// immediately.
		if (ofs > addr)
		{
			// it's a forward jump - queue it
			jit_emit_queue_instr(ofs);
		}
		else
		{
			// it's a back jump - translate recursively
			xlat_recursive(jit, ofs);
		}

		// jump to the target address
		emit_jump(jit, ofs, cycles);

		// success
		return 1;
	}
}

// Translate code recursively.  This checks that the code address is
// within the covered address space and that it hasn't already been
// translated or marked as emulate-only; if those tests pass, we do
// the translation.  We don't change the opcode state if the translation
// fails.
static int xlat_recursive(struct jit_ctl *jit, data32_t addr)
{
	void *addrp;

	// if it's not a JIT-able address, do nothing
	if (addr < jit->minAddr || addr >= jit->maxAddr)
		return 0;

	// If this opcode has already been marked as untranslatable, return
	// failure.  If it's already been translated, or it's marked as
	// currently undergoing translation, return success.
	addrp = JIT_NATIVE(jit, addr);
	if (addrp == jit->pEmulate)
		return 0;
	else if (addrp == jit->pWorking)
		return 0;
	else if (addrp != jit->pPending)
		return 1;

	// proceed with the translation
	return xlat(jit, addr);
}


// Translate code.  (Note: this shouldn't be called directly.  The emulator
// calls us through the public entrypoint, arm7_jit_xlat().  Recursive
// calls go through xlat_recursive().)
static int xlat(struct jit_ctl *jit, data32_t pc)
{
	data32_t insn;
	int cnt;
	int ok;
	data32_t addr, retAddr;

	// push a stack level in the translator; if that fails, return failure
	if (!jit_emit_push())
		return 0;

	// keep going until we reach an unconditional branch, or something
	// we can't translate
	for (addr = pc, cnt = 0, ok = 1 ; ; addr += 4)
	{
		void *pn;
		// assume there's no conditional label
		struct jit_label *condLabel = 0, *condLabel2 = 0;
		
		// most instructions takes 3 cycles; we'll adjust this as needed
		// for instructions that have different timing
		int cycles = 3;

		// presume this instruction won't perform a branch
		int br = 0;

		/* Debugging trap
		if (addr == 0x12790)
		{ 
			addr = 0x12790;
		}*/

		// if we've strayed outside the JIT-able range, stop here
		if (addr < jit->minAddr || addr >= jit->maxAddr)
		{
			// act like this is a 'pEmulate' instruction
			retAddr = addr;
			break;
		}

		// Only translate this instruction if it's in the "pending" state.
		pn = JIT_NATIVE(jit, addr);
		if (pn == jit->pWorking)
		{
			// The instruction is already in the process of translation. 
			// Generate a jump to that code, and stop translating.
			emit(JMP, Label, jit_new_addr_label(addr));
			retAddr = addr;
			break;
		}
		else if (pn == jit->pEmulate)
		{
			// The instruction has already been found to be untranslatable.
			// Generate a return to the emulator, and stop translating.
			retAddr = addr;
			break;
		}
		else if (pn != jit->pPending)
		{
			// The instruction has already been translated.  Jump directly
			// to the native code, and stop translating.
			emit(JMP, Label, jit_new_native_label(pn));
			retAddr = addr;
			break;
		}

		// begin the instruction in the emitter
		jit_emit_begin_instr(jit, addr);

		// grab the opcode
		insn = cpu_readop32(addr);

		// Pull out the condition flags, and generate a jump past the generated
		// code for this instruction if the condition is false.
		//
		// NB: all conditions that test the carry flag must use the inverted sense
		// of the ARM carry flag.  We always store the inverted value because the
		// conditions that test the carry and zero flags together are only available
		// in certain combinations.  The tests that ARM instructions can perform
		// only map to the corresponding Intel tests by inverting the flag value.
 		switch (insn >> INSN_COND_SHIFT)
		{
		case COND_EQ:
			condJumpIfNot(Z_MASK);
			break;
		case COND_NE:
			condJumpIf(Z_MASK);
			break;
		case COND_CS:
			condJumpIfNot(C_MASK);
			break;
		case COND_CC:
			condJumpIf(C_MASK);
			break;
		case COND_MI:
			condJumpIfNot(N_MASK);
			break;
		case COND_PL:
			condJumpIf(N_MASK);
			break;
		case COND_VS:
			condJumpIfNot(V_MASK);
			break;
		case COND_VC:
			condJumpIf(V_MASK);
			break;
		case COND_HI:
			// HI = C & !Z -> jump if !(C & !Z) == !C | Z
			condJumpIfNot(C_MASK);
			testCPSR(Z_MASK);
			emit(JNZ, Label, condLabel);
			break;
		case COND_LS:
			// LS = !C | Z
			// first, jump TO the code if !C, otherwise fall through...
			condJumpIfNot(C_MASK);
			condLabel2 = condLabel;

			// ...now C is set, but the condition is still true if Z is set: so
			// jump PAST the code if !Z
			condJumpIfNot(Z_MASK);

			// set the label for the "TO the code" jump above
			jit_resolve_label(condLabel2);
			break;
		case COND_GE:
			// GE - N==V.  This is complex to test, so use the native jump test
			// by loading the N and V flags from CPSR into the real Intel flags,
			// then jumping past the code with JL if the condition is false.
		    condJumpOnFlags(JL);
			break;
		case COND_LT:
			condJumpOnFlags(JGE);
			break;
		case COND_GT:
			condJumpOnFlags(JLE);
			break;
		case COND_LE:
			condJumpOnFlags(JG);
			break;
		case COND_NV:
			// NEVER condition - this opcode never executes.  Don't bother generating
			// it in this case - just generate a NOP for the instruction, and proceed
			// to the next one.
			emit(NOP);
			goto end_of_instr;
		}

		// figure the instruction type based on the top byte
		switch ((insn & 0xF000000) >> 24)
		{
		case 0:
			// 0000 -> data proc, multiply, multiply long, halfword data transfer
			switch (insn & 0xf0)
			{
			case 0x90: // 1001 - multiply, multiply long
				if (insn & 0x800000) {
					// 64-bit multiply
					ok = MUL64(jit, addr, insn, &cycles);
				}
				else {
					// 32-bit multiply
					ok = MUL32(jit, addr, insn, &cycles);
				}
				break;

			case 0xb0: // 1011 - halfword data transfer
			case 0xd0: // 1101 - halfword data transfer
				ok = LDRHB_STRHB(jit, addr, insn, &br, &cycles);
				break;

			default: // all others are arithmetic operations
				ok = ALU(jit, addr, insn, &br, &cycles);
				break;
			}
			break;

		case 1:
			// 0001 -> BX, SWP, halfword data transfer, arithmetic, PSR transfer
			if((insn & 0x0ffffff0) == 0x012fff10)    // bits 27-4 == 000100101111111111110001 -> BX
			{
				ok = BX(jit, addr, insn);
				br = 1;
			}
			else if ((insn & 0x80) && (insn & 0x10))   // bit 7=1, bit 4=1 -> half word data transfer, swap
			{
				if (insn & 0x60)  // bits = 6-5 != 00 -> half word data transfer
				{
					ok = LDRHB_STRHB(jit, addr, insn, &br, &cycles);
				}
				else              // bits = 6-5 == 00 -> swap
					ok = SWAP(jit, addr, insn, &cycles);
			}
			else if (((insn & 0x0100000) == 0) && ((insn & 0x01800000) == 0x01000000)) // S=0, and bits 24-23 == 10 -> PSR transfer
			{
				ok = PSRX(jit, addr, insn, &cycles);
			}
			else // anything else is arithmetic
			{
				ok = ALU(jit, addr, insn, &br, &cycles);
			}
			break;

		case 2:
		case 3:
			// 0010, 0011 -> data proc/PSR transfer
			if (((insn & 0x0100000) == 0) && ((insn & 0x01800000) == 0x01000000)) // S=0, and bits 24-23 == 10 -> PSR transfer
			{
				ok = PSRX(jit, addr, insn, &cycles);
			}
			else
			{
				ok = ALU(jit, addr, insn, &br, &cycles);
			}
			break;

		case 4:
		case 5:
		case 6:
		case 7:
			// data transfer - single data access
			ok = LDR_STR(jit, addr, insn, &br, &cycles);
			break;

		case 8:
		case 9:
			// block data transfer/access
			ok = LDM_STM(jit, addr, insn, &br, &cycles);
			break;

		case 0xA:
		case 0xB:
			// branch, branch/link
			ok = B(jit, addr, insn, &br, &cycles);
			break;

		case 0xC:
		case 0xD:
		case 0xE:
			// co-processor data transfer, data operation, or register transfer - not handled
			ok = 0;
			break;

		case 0xF:
			// software interrupt - not handled
			ok = 0;
			break;

		default:
			// invalid/other - not handled
			ok = 0;
			break;
		}

	end_of_instr:
		// if we successfully translated this instruction, commit the code
		// we generated and count the success
		if (ok)
		{
			// if we have a forward jump for the condition, the destination
			// is the next instruction
			if (condLabel != 0)
			{
				// allocate all but one cycle to the main instruction
				count_cycles(cycles-1);

				// resolve the label to jump around the instruction
				jit_resolve_label(condLabel);

				// one more cycle for the condition check, whether or not it passed
				count_cycles(1);
			}
			else
			{
				// allocate the whole instruction count adjustment to this instruction
				count_cycles(cycles);
			}

			// count the translated instruction
			++cnt;
		}
		else
		{
			// cancel the instruction, in case we emitted code for the condition
			jit_emit_cancel_instr(jit);

			// since we can't parse this instruction, mark it as 'emulate' in the
			// map so that we don't waste time trying to translate it again
			JIT_NATIVE(jit, addr) = jit->pEmulate;

			// stop here; we didn't generate anything for this opcode, so we
			// want to return to the emulator here
			retAddr = addr;
			break;
		}

		// If this statement did an unconditional branch (either an explicit
		// B or BX instruction, or a LDR or LDM that loads R15), stop translating.
		// An unconditional branch or subroutine return generally indicates the
		// end of a section of code, so it's not safe to assume that the next
		// location holds a valid address or that it will ever be reached.
		// Branches are often used to jump around inline static data, for 
		// example.  If the next instruction is ever actually reached, it will
		// trigger a new JIT translation, so we'll get to it eventually if it
		// turns out to be code after all.
		//
		// Note that if we somehow misinterpreted this branch, and we end up
		// falling through the end of this code, we'll want to resume at the
		// *next* instruction, since we've generated code for this one.  So
		// the return address is addr+4.

#ifdef _DEBUG
		// Force this to TRUE if you want 1 by 1 code execution, good for debugging. 
		// Keeping it in an #ifdef so I don't accidentally release this way.
		if (/*1 || */br && condLabel == 0) {
#else
		if (br && condLabel == 0) {
#endif
			retAddr = addr + 4;
			break;
		}
	}

	// if we were successful, commit the code block to the JIT executable page
	// so that we can execute it for real
	if (cnt != 0)
	{
		// Generate extra code after the last instruction we generated to return
		// to the emulator.  This ensures that we exit to the emulator properly
		// if the code up to this point didn't branch somewhere.  Note that we
		// generate this code as part of the last valid instruction we generated,
		// but the target address is the *next* opcode after that, since we
		// execute this code if we fall off the end of the generated code.
		jit_emit_return_to_emu(retAddr);

		// commit the code
		jit_emit_commit(jit);
	}

	// pop emitter state
	jit_emit_pop();

	// The return code indicates whether or not we were able to translate
	// the instruction at 'pc'.  If we were able to, we might have also
	// processed additional instructions, but the return code doesn't
	// care about that.  So if the count of translated instructions is
	// non-zero, we successfully translated the first instruction and
	// can return true.
	return cnt;
}

// Translate instructions starting at the given address.  We'll work
// forward through sequentially executable instructions, translating
// everything until we reach something we can't translate or an
// unconditional branch.  We'll also process the code at any branch
// and subroutine targets.
static int arm7_jit_xlat(struct jit_ctl *jit, data32_t pc)
{
	byte *p;

	// queue the instruction
	jit_emit_queue_instr(pc);

	// process the queue
	jit_emit_process_queue(jit, xlat);

	// return true if we translated the instruction
	p = JIT_NATIVE(jit, pc);
	return (p != jit->pPending && p != jit->pEmulate);
}


#endif /* JIT_ENABLED */
