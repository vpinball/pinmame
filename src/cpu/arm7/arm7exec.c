/*****************************************************************************
 *
 *	 arm7exec.c
 *	 Portable ARM7TDMI Core Emulator
 *
 *	 Copyright (c) 2004 Steve Ellenoff, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   sellenoff@hotmail.com
 *	 - The author of this copywritten work reserves the right to change the
 *	   terms of its usage and license at any time, including retroactively
 *	 - This entire notice must remain in the source code.
 *
 *	This work is based on:
 *  #1) 'Atmel Corporation ARM7TDMI (Thumb) Datasheet - January 1999'
 *  #2) Arm 2/3/6 emulator By Bryan McPhail (bmcphail@tendril.co.uk) and Phil Stroffolino (MAME CORE 0.76)
 *
 *****************************************************************************/

/******************************************************************************
 *  Notes:
 *         This file contains the code to run during the CPU EXECUTE METHOD.
 *         It has been split into it's own file (from the arm7core.c) so it can be 
 *         directly compiled into any cpu core that wishes to use it. 
 *
 *         It should be included as follows in your cpu core:
 *
 *		   int arm7_execute( int cycles )
 *         {
 *		   #include "arm7exec.c"
 *         } 
 *
*****************************************************************************/

//#define ARM9 has newer MAME code, but that is only partially ported over to the old codebase
#ifdef ARM9
void arm7_cpu_device::arm9ops_undef(uint32_t insn)
{
	// unsupported instruction
	LOG(("ARM7: Instruction %08X unsupported\n", insn));
}

void arm7_cpu_device::arm9ops_1(uint32_t insn)
{
	/* Change processor state (CPS) */
	if ((insn & 0x00f10020) == 0x00000000)
	{
		// unsupported (armv6 onwards only)
		arm9ops_undef(insn);
		R15 += 4;
	}
	else if ((insn & 0x00ff00f0) == 0x00010000) /* set endianness (SETEND) */
	{
		// unsupported (armv6 onwards only)
		arm9ops_undef(insn);
		R15 += 4;
	}
	else
	{
		arm9ops_undef(insn);
		R15 += 4;
	}
}

void arm7_cpu_device::arm9ops_57(uint32_t insn)
{
	/* Cache Preload (PLD) */
	if ((insn & 0x0070f000) == 0x0050f000)
	{
		// unsupported (armv6 onwards only)
		arm9ops_undef(insn);
		R15 += 4;
	}
	else
	{
		arm9ops_undef(insn);
		R15 += 4;
	}
}

void arm7_cpu_device::arm9ops_89(uint32_t insn)
{
	/* Save Return State (SRS) */
	if ((insn & 0x005f0f00) == 0x004d0500)
	{
		// unsupported (armv6 onwards only)
		arm9ops_undef(insn);
		R15 += 4;
	}
	else if ((insn & 0x00500f00) == 0x00100a00) /* Return From Exception (RFE) */
	{
		// unsupported (armv6 onwards only)
		arm9ops_undef(insn);
		R15 += 4;
	}
	else
	{
		arm9ops_undef(insn);
		R15 += 4;
	}
}

void arm7_cpu_device::arm9ops_ab(uint32_t insn)
{
	// BLX
	HandleBranch(insn, true);
	set_cpsr(GET_CPSR|T_MASK);
}

void arm7_cpu_device::arm9ops_c(uint32_t insn)
{
	/* Additional coprocessor double register transfer */
	if ((insn & 0x00e00000) == 0x00400000)
	{
		// unsupported
		arm9ops_undef(insn);
		R15 += 4;
	}
	else
	{
		arm9ops_undef(insn);
		R15 += 4;
	}
}

void arm7_cpu_device::arm9ops_e(uint32_t insn)
{
	/* Additional coprocessor register transfer */
	// unsupported
	arm9ops_undef(insn);
	R15 += 4;
}
#endif

extern unsigned at91_get_reg(int regnum);

/* This implementation uses an improved switch() for hopefully faster opcode fetches compared to my last version
.. though there's still room for improvement. */
{
	data32_t pc;
	static data32_t pc_prev2 = 0, pc_prev1 = 0;
	data32_t insn;
	int op_offset;

	RESET_ICOUNT
	do
	{

		/* Hook for Pre-Opcode Processing */
		BEFORE_OPCODE_EXEC_HOOK

#ifdef MAME_DEBUG
		if (mame_debug)
			MAME_Debug();
#endif

		/* load 32 bit instruction, trying the JIT first */
		pc = R15;

		// Debug test triggers

		// Helpful to backtrace a crash :)

		JIT_FETCH(ARM7.jit, pc);
		insn = cpu_readop32(pc);
		op_offset = 0;

		pc_prev2 = pc_prev1;
		pc_prev1 = pc;

		/* process condition codes for this instruction */
		switch (insn >> INSN_COND_SHIFT)
		{
		case COND_EQ:
			if (Z_IS_CLEAR(GET_CPSR)) goto L_Next;
			break;
		case COND_NE:
			if (Z_IS_SET(GET_CPSR)) goto L_Next;
			break;
		case COND_CS:
			if (C_IS_CLEAR(GET_CPSR)) goto L_Next;
			break;
		case COND_CC:
			if (C_IS_SET(GET_CPSR)) goto L_Next;
			break;
		case COND_MI:
			if (N_IS_CLEAR(GET_CPSR)) goto L_Next;
			break;
		case COND_PL:
			if (N_IS_SET(GET_CPSR)) goto L_Next;
			break;
		case COND_VS:
			if (V_IS_CLEAR(GET_CPSR)) goto L_Next;
			break;
		case COND_VC:
			if (V_IS_SET(GET_CPSR)) goto L_Next;
			break;
		case COND_HI:
			if (C_IS_CLEAR(GET_CPSR) || Z_IS_SET(GET_CPSR)) goto L_Next;
			break;
		case COND_LS:
			if (C_IS_SET(GET_CPSR) && Z_IS_CLEAR(GET_CPSR)) goto L_Next;
			break;
		case COND_GE:
			if (!(GET_CPSR & N_MASK) != !(GET_CPSR & V_MASK)) goto L_Next; /* Use x ^ (x >> ...) method */
			break;
		case COND_LT:
			if (!(GET_CPSR & N_MASK) == !(GET_CPSR & V_MASK)) goto L_Next;
			break;
		case COND_GT:
			if (Z_IS_SET(GET_CPSR) || (!(GET_CPSR & N_MASK) != !(GET_CPSR & V_MASK))) goto L_Next;
			break;
		case COND_LE:
			if (Z_IS_CLEAR(GET_CPSR) && (!(GET_CPSR & N_MASK) == !(GET_CPSR & V_MASK))) goto L_Next;
			break;
		case COND_NV:
#ifdef ARM9
			if (m_archRev < 5)
#endif
				goto L_Next;
#ifdef ARM9
			else
				op_offset = 0x10;
#endif
			break;
		}
		/*******************************************************************/
		/* If we got here - condition satisfied, so decode the instruction */
		/*******************************************************************/		
		switch( ((insn & 0xF000000)>>24) + op_offset)
		{
			/* Bits 27-24 = 0000 -> Can be Data Proc, Multiply, Multiply Long, Halfword Data Transfer */
			case 0:
				/* Bits 7-4 */
				switch(insn & 0xf0)
				{
					// Multiply, Multiply Long
					case 0x90:		//1001
						/* multiply long */
						if( insn&0x800000 )	//Bit 23 = 1 for Multiply Long
						{
							/* Signed? */
							if( insn&0x00400000 )
								HandleSMulLong(insn);
							else
								HandleUMulLong(insn);
						}
						/* multiply */
						else
						{
							HandleMul(insn);
						}
						R15 += 4;
						break;

					// Halfword Data Transfer
					case 0xb0:		//1011
					case 0xd0:		//1101
						HandleHalfWordDT(insn);
						break;
					// Data Proc (Cannot be PSR Transfer since bit 24 = 0)
					default:
						HandleALU(insn);
				}
				break;

			/* Bits 27-24 = 0001 -> Can be BX, SWP, Halfword Data Transfer, Data Proc/PSR Transfer */
			case 1:

				/* Branch and Exchange (BX) */
				if( (insn&0x0ffffff0)==0x012fff10 )		//bits 27-4 == 000100101111111111110001
				{ 
					R15 = GET_REGISTER(insn & 0x0f);
					//If new PC address has A0 set, switch to Thumb mode
					if(R15 & 1) {
						SET_CPSR(GET_CPSR|T_BIT);
						LOG(("%08x: Setting Thumb Mode due to R15 change to %08x - but not supported\n",pc,R15));
					}
				}
				else
				/* Swap OR Half Word Data Transfer OR Data Proc/PSR Transfer */
				if( (insn & 0x80) && (insn & 0x10) )	// bit 7=1, bit 4=1
				{
					/* Half Word Data Transfer */
					if(insn & 0x60)			//bits = 6-5 != 00
					{
						HandleHalfWordDT(insn);
					}
					else 
					/* Swap */
					{
						HandleSwap(insn);
					}
				}
				else
				/* Data Processing OR PSR Transfer */
				{
					/* PSR Transfer (MRS & MSR) */
					if( ((insn&0x0100000)==0) && ((insn&0x01800000)==0x01000000) ) //( S bit must be clear, and bit 24,23 = 10 )
					{
						HandlePSRTransfer(insn);
						ARM7_ICOUNT += 2;		//PSR only takes 1 - S Cycle, so we add + 2, since at end, we -3..
						R15 += 4;
					}
					/* Data Processing */
					else
					{
						HandleALU(insn);
					}
				}
				break;

			/* Bits 27-24 = 0011 OR 0010 -> Can only be Data Proc/PSR Transfer */
			case 2:
			case 3:
				/* PSR Transfer (MRS & MSR) */
				if( ((insn&0x0100000)==0) && ((insn&0x01800000)==0x01000000) ) //( S bit must be clear, and bit 24,23 = 10 )
				{
					HandlePSRTransfer(insn);
					ARM7_ICOUNT += 2;		//PSR only takes 1 - S Cycle, so we add + 2, since at end, we -3..
					R15 += 4;
				}
				/* Data Processing */
				else
				{
					HandleALU(insn);
				}
				break;

			/* Data Transfer - Single Data Access */
			case 4:
			case 5:
			case 6:
			case 7:
				HandleMemSingle(insn);
				R15 += 4;
				ARM7_CHECKIRQ;
				break;
			/* Block Data Transfer/Access */
			case 8:
			case 9:
				HandleMemBlock(insn);
				R15 += 4;
				break;
			/* Branch or Branch & Link */
			case 0xa:
			case 0xb:
				HandleBranch(insn, 0);
				break;
			/* Co-Processor Data Transfer */
			case 0xc:
			case 0xd:
				HandleCoProcDT(insn);
				R15 += 4;
				break;
			/* Co-Processor Data Operation or Register Transfer */
			case 0xe:
				if(insn & 0x10)
					HandleCoProcRT(insn);
				else
					HandleCoProcDO(insn);
				R15 += 4;
				break;
			/* Software Interrupt */
			case 0x0f:
				ARM7.pendingSwi = 1;
				ARM7_CHECKIRQ;
				//couldn't find any cycle counts for SWI
				break;
			/* Undefined */
			default:
				ARM7.pendingSwi = 1;

				ARM7_ICOUNT -= 1;				//undefined takes 4 cycles (page 77)
				LOG(("%08x:  Undefined instruction\n",pc-4));
				L_Next:
					R15 += 4;
					ARM7_ICOUNT +=2;	//Any unexecuted instruction only takes 1 cycle (page 193)
					ARM7_CHECKIRQ;
		}
		/* All instructions remove 3 cycles.. Others taking less / more will have adjusted this # prior to here */
		ARM7_ICOUNT -= 3;

		/* Hook for capturing previous cycles */
		CAPTURE_NUM_CYCLES

		/* Hook for Post-Opcode Processing */
		AFTER_OPCODE_EXEC_HOOK

#if JIT_ENABLED
	resume_from_jit: ;
#endif

	} while( ARM7_ICOUNT > 0 );

	return cycles - ARM7_ICOUNT;


// --------------------------------------------------------------------------
//
// Windows/Intel JIT 
//
#if JIT_ENABLED

	// call native code translated by the JIT
jit_go_native:
	{
		// get the native code pointer and cycle counter into stack variables
		data32_t tmp1 = (data32_t)JIT_NATIVE(ARM7.jit, pc);
		data32_t tmp2 = ARM7_ICOUNT;
		
		__asm {
			// Allocate space for temporary variables we'll need on return from the
			// native code (see 'IMPORTANT' note below).  1 stack DWORD == 4 bytes.
			SUB ESP, 4;

			// save registers that the generated code uses and that the C caller
			// might expect to be preserved across function calls
			PUSH EBX;
			PUSH ECX;
			PUSH EDX;
			PUSH ESI;
			PUSH EDI;
			
			// get the native code address, and move the cycle counter into EDI for
			// use in the translated code
			MOV  EAX, tmp1;
			MOV  EDI, tmp2;

			// IMPORTANT: don't access any C local variables (tmp1, tmp2, etc) from
			// here until after the POPs below.  At least one VC optimization mode uses
			// EBX as the frame pointer, and it's possible that other modes or other
			// compilers use other registers.  C local access will be safe again
			// after the POPs below, which will recover the pre-call register values.
			// In the meantime, anything we need to store temporarily must be saved
			// explicitly in stack slots allocated with the SUB ESP, n above, and
			// addressed explicitly in terms of [ESP+n] addresses.  These are safe
			// because we control the stack layout in this section of code.

			// call the native code
			CALL EAX;

			// save the new cycle counter from EDI into a stack temp (before we restore
			// the pre-call EDI)
			MOV  [ESP+20], EDI;
			
			// restore saved registers
			POP  EDI;
			POP  ESI;
			POP  EDX;
			POP  ECX;
			POP  EBX;

			// recover the new PC and cycle count
			MOV  tmp1, EAX;
		    POP  tmp2;
		}
		R15 = tmp1;
		ARM7_ICOUNT = tmp2;
	}
	// resume emulation
	goto resume_from_jit;

#endif /* JIT_ENABLED */
}

