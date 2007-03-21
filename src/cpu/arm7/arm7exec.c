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

extern unsigned at91_get_reg(int regnum);

/* This implementation uses an improved switch() for hopefully faster opcode fetches compared to my last version
.. though there's still room for improvement. */
{
	data32_t pc;
	data32_t insn;

	RESET_ICOUNT
	do
	{

		/* Hook for Pre-Opcode Processing */
		BEFORE_OPCODE_EXEC_HOOK

#ifdef MAME_DEBUG
		if (mame_debug)
			MAME_Debug();
#endif

		/* load 32 bit instruction */
		pc = R15;
		insn = cpu_readop32(pc);

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
			goto L_Next;
		}
		/*******************************************************************/
		/* If we got here - condition satisfied, so decode the instruction */
		/*******************************************************************/		
		switch( (insn & 0xF000000)>>24 )
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
				HandleBranch(insn);
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
				ARM7_CHECKIRQ;
				ARM7_ICOUNT -= 1;				//undefined takes 4 cycles (page 77)
				LOG(("%08x:  Undefined instruction\n",pc-4));
				L_Next:
					R15 += 4;
					ARM7_ICOUNT +=2;	//Any unexecuted instruction only takes 1 cycle (page 193)
		}
		/* All instructions remove 3 cycles.. Others taking less / more will have adjusted this # prior to here */
		ARM7_ICOUNT -= 3;

		/* Hook for capturing previous cycles */
		CAPTURE_NUM_CYCLES

		/* Hook for Post-Opcode Processing */
		AFTER_OPCODE_EXEC_HOOK

	} while( ARM7_ICOUNT > 0 );

	return cycles - ARM7_ICOUNT;
}

