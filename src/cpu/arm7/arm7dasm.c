/*****************************************************************************
 *
 *	 arm7dasm.c
 *	 Portable ARM7TDMI Core Emulator - Disassembler
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
 *
 *  Because Co-Processor functions are highly specialized to the actual co-proc
 *  implementation being used, I've setup callback handlers to allow for custom
 *  dasm display of the co-proc functions so that the implementation specific
 *  commands/interpretation can be used. If not used, the default handlers which
 *  implement the ARM7TDMI guideline format is used
 ******************************************************************************/

#include <stdio.h>
#include "arm7core.h"

#ifdef MAME_DEBUG

//custom dasm callback handlers for co-processor instructions (setup in the core)
extern char *(*arm7_dasm_cop_dt_callback)( char *pBuf, data32_t opcode, char *pConditionCode, char *pBuf0 );
extern char *(*arm7_dasm_cop_rt_callback)( char *pBuf, data32_t opcode, char *pConditionCode, char *pBuf0 );
extern char *(*arm7_dasm_cop_do_callback)( char *pBuf, data32_t opcode, char *pConditionCode, char *pBuf0 );

static char *WritePadding( char *pBuf, const char *pBuf0 )
{
	pBuf0 += 8;
	while( pBuf<pBuf0 )
	{
		*pBuf++ = ' ';
	}
	return pBuf;
}

static char *DasmCoProc_RT( char *pBuf, data32_t opcode, char *pConditionCode, char *pBuf0)
{
	/* co processor register transfer */
	/* xxxx 1110 oooL nnnn dddd cccc ppp1 mmmm */
	if( opcode&0x00100000 )		//Bit 20 = Load or Store
	{
		pBuf += sprintf( pBuf, "MRC" );
	}
	else
	{
		pBuf += sprintf( pBuf, "MCR" );
	}
	pBuf += sprintf( pBuf, "%s", pConditionCode );
	pBuf = WritePadding( pBuf, pBuf0 );			
	pBuf += sprintf( pBuf, "p%d, %d, R%d, c%d, c%d",
					(opcode>>8)&0xf, (opcode>>21)&7, (opcode>>12)&0xf, (opcode>>16)&0xf, opcode&0xf );
	if((opcode>>5)&7) pBuf += sprintf( pBuf, ", %d",(opcode>>5)&7);
	return pBuf;
}

static char *DasmCoProc_DT( char *pBuf, data32_t opcode, char *pConditionCode, char *pBuf0 )
{
	/* co processor data transfer */
	/* xxxx 111P UNWL nnnn dddd pppp oooooooo */
	//todo: test this on valid instructions

	pBuf += sprintf(pBuf, "%s%s",(opcode&0x00100000)?"LDC":"STC",pConditionCode);	//Bit 20 = 1 for Load, 0 for Store
	//Long Operation
	if(opcode & 0x400000)	pBuf += sprintf(pBuf, "L");
	pBuf = WritePadding( pBuf, pBuf0 );

	//P# & CD #
	pBuf += sprintf(pBuf, "p%d, c%d, ",(opcode>>8)&0x0f,(opcode>>12)&0x0f);

	//Base Register (Rn)
	pBuf += sprintf(pBuf, "[R%d%s",(opcode>>16)&0x0f,(opcode&0x1000000)?"":"]");	//If Bit 24 = 1, Pre-increment, otherwise, Post increment so close brace

	//immediate value ( 8 bit value is << 2 according to manual )
	if(opcode & 0xff)	pBuf += sprintf(pBuf, ",%s#$%x",(opcode&0x800000)?"":"-",(opcode & 0xff)<<2);

	//Pre-Inc brace & Write back
	pBuf += sprintf(pBuf, "%s%s",(opcode&0x1000000)?"]":"",(opcode&0x200000)?"{!}":"");
	return pBuf;
}

static char *DasmCoProc_DO( char *pBuf, data32_t opcode, char *pConditionCode, char *pBuf0 )
{
	/* co processor data operation */
	/* xxxx 1110 oooo nnnn dddd cccc ppp0 mmmm */
	pBuf += sprintf( pBuf, "CDP" );
	pBuf += sprintf( pBuf, "%s", pConditionCode );
	pBuf = WritePadding( pBuf, pBuf0 );			
	//p#,CPOpc,cd,cn,cm
	pBuf += sprintf( pBuf, "p%d, %d, c%d, c%d, c%d",
		(opcode>>8)&0xf, (opcode>>20)&0xf, (opcode>>12)&0xf, (opcode>>16)&0xf, opcode&0xf );
	if((opcode>>5)&7) pBuf += sprintf( pBuf, ", %d",(opcode>>5)&7);
	return pBuf;
}

static char *WriteImmediateOperand( char *pBuf, data32_t opcode )
{
	/* rrrrbbbbbbbb */
	data32_t imm;
	int r;

	imm = opcode&0xff;
	r = ((opcode>>8)&0xf)*2;
	imm = (imm>>r)|(imm<<(32-r));
	pBuf += sprintf( pBuf, ", #$%x", imm );
	return pBuf;
}

static char *WriteDataProcessingOperand( char *pBuf, data32_t opcode, int printOp0, int printOp1, int printOp2 )
{
	/* ccccctttmmmm */
	const char *pRegOp[4] = { "LSL","LSR","ASR","ROR" };

	if (printOp0)
		pBuf += sprintf(pBuf,"R%d, ", (opcode>>12)&0xf);
	if (printOp1)
		pBuf += sprintf(pBuf,"R%d, ", (opcode>>16)&0xf);

	/* Immediate Op2 */
	if( opcode&0x02000000 )
		return WriteImmediateOperand(pBuf-2,opcode);

	/* Register Op2 */
	if (printOp2)
//SJE:	pBuf += sprintf(pBuf,"R%d, ", (opcode>>0)&0xf);
		pBuf += sprintf(pBuf,"R%d ", (opcode>>0)&0xf);

	//SJE: ignore if LSL#0 for register shift
	if( ((opcode&0x2000000) == 0) && (((opcode>>4) & 0xff)==0) )
		return pBuf;

	pBuf += sprintf(pBuf, ",%s ", pRegOp[(opcode>>5)&3] );
	//SJE: pBuf += sprintf(pBuf, "%s ", pRegOp[(opcode>>5)&3] );

	if( opcode&0x10 ) /* Shift amount specified in bottom bits of RS */
	{
		pBuf += sprintf( pBuf, "R%d", (opcode>>8)&0xf );
	}
	else /* Shift amount immediate 5 bit unsigned integer */
	{
		int c=(opcode>>7)&0x1f;
		if( c==0 ) c = 32;
		pBuf += sprintf( pBuf, "#%d", c );
	}
	return pBuf;
}

static char *WriteRegisterOperand1( char *pBuf, data32_t opcode )
{
	/* ccccctttmmmm */
	const char *pRegOp[4] = { "LSL","LSR","ASR","ROR" };

	pBuf += sprintf(
		pBuf,
		", R%d", /* Operand 1 register, Operand 2 register, shift type */
		(opcode>> 0)&0xf);

	//check for LSL 0
	if( (((opcode>>5)&3)==0) && (((opcode>>7)&0xf)==0) )
		return pBuf;
	else
	//Add rotation type
		pBuf += sprintf(pBuf," %s ",pRegOp[(opcode>>5)&3]);

	if( opcode&0x10 ) /* Shift amount specified in bottom bits of RS */
	{
		pBuf += sprintf( pBuf, "R%d", (opcode>>7)&0xf );
	}
	else /* Shift amount immediate 5 bit unsigned integer */
	{
		int c=(opcode>>7)&0x1f;
		if( c==0 ) c = 32;
		pBuf += sprintf( pBuf, "#%d", c );
	}
	return pBuf;
} /* WriteRegisterOperand */


static char *WriteBranchAddress( char *pBuf, data32_t pc, data32_t opcode )
{
	opcode &= 0x00ffffff;
	if( opcode&0x00800000 )
	{
		opcode |= 0xff000000; /* sign-extend */
	}
	pc += 8+4*opcode;
	sprintf( pBuf, "$%x", pc );
	return pBuf;
} /* WriteBranchAddress */

void arm7_disasm( char *pBuf, data32_t pc, data32_t opcode )
{
	const char *pBuf0;

	const char *pConditionCodeTable[16] =
	{
		"EQ","NE","CS","CC",
		"MI","PL","VS","VC",
		"HI","LS","GE","LT",
		"GT","LE","","NV"
	};
	const char *pOperation[16] =
	{
		"AND","EOR","SUB","RSB",
		"ADD","ADC","SBC","RSC",
		"TST","TEQ","CMP","CMN",
		"ORR","MOV","BIC","MVN"
	};
	const char *pConditionCode;

	pConditionCode= pConditionCodeTable[opcode>>28];
	pBuf0 = pBuf;

	if( (opcode&0x0ffffff0)==0x012fff10 ) { //bits 27-4 == 000100101111111111110001
		/* Branch and Exchange (BX) */
		pBuf += sprintf( pBuf, "B");
		pBuf += sprintf( pBuf, "%sX", pConditionCode );
		pBuf = WritePadding( pBuf, pBuf0 );
		pBuf += sprintf( pBuf, "R%d",(opcode&0xf));
	}
	else if( (opcode&0x0e000000)==0 && (opcode&0x80) && (opcode&0x10) )	//bits 27-25 == 000, bit 7=1, bit 4=1
	{
		/* multiply or swap or half word data transfer */

		if(opcode&0x60) {	//bits = 6-5 != 00
		/* half word data transfer */
			pBuf += sprintf(pBuf, "%s%s",(opcode&0x00100000)?"LDR":"STR",pConditionCode);	//Bit 20 = 1 for Load, 0 for Store

			//Signed? (if not, always unsigned half word)
			if(opcode&0x40)
				pBuf += sprintf(pBuf, "%s",(opcode&0x20)?"SH":"SB");	//Bit 5 = 1 for Half Word, 0 for Byte
			else
				pBuf += sprintf(pBuf, "H");

			pBuf = WritePadding( pBuf, pBuf0 );

			//Dest Register
			pBuf += sprintf(pBuf, "R%d, ",(opcode>>12)&0x0f);
			//Base Register
			pBuf += sprintf(pBuf, "[R%d%s",(opcode>>16)&0x0f,(opcode&0x1000000)?"":"]");	//If Bit 24 = 1, Pre-increment, otherwise, Post increment so close brace

			//Immediate or Register Offset?
			if(opcode&0x400000) {			//Bit 22 - 1 = immediate, 0 = register
				//immediate			( imm. value in high nibble (bits 8-11) and lo nibble (bit 0-3) )
				pBuf += sprintf(pBuf, ",%s#$%x",(opcode&0x800000)?"":"-",( (((opcode>>8)&0x0f)<<4) | (opcode&0x0f)));
			}
			else {
				//register
				pBuf += sprintf(pBuf, ",%sR%d",(opcode&0x800000)?"":"-",(opcode & 0x0f));
			}

			//Pre-Inc brace & Write back
			pBuf += sprintf(pBuf, "%s%s",(opcode&0x1000000)?"]":"",(opcode&0x200000)?"{!}":"");
		}
		else { 
			if(opcode&0x01000000) {		//bit 24 = 1
			/* swap */
			//todo: Test on valid instructions
				/* xxxx 0001 0B00 nnnn dddd 0000 1001 mmmm */
				pBuf += sprintf( pBuf, "SWP" );
				pBuf += sprintf( pBuf, "%s%s", pConditionCode, (opcode & 0x400000)?"B":"" );	//Bit 22 = Byte/Word selection
				//Rd, Rm, [Rn]
				pBuf += sprintf( pBuf, "R%d, R%d, [R%d]",
								(opcode>>12)&0xf, opcode&0xf, (opcode>>16)&0xf );
			}
			else {
				/* multiply or multiply long */

				if( opcode&0x800000 )	//Bit 23 = 1 for Multiply Long
				{
					/* Multiply Long */
					/* xxxx0001 UAShhhhllllnnnn1001mmmm */

					/* Signed? */
					if( opcode&0x00400000 )
						pBuf += sprintf( pBuf, "S" );
					else
						pBuf += sprintf( pBuf, "U" );
					
					/* Multiply & Accumulate? */
					if( opcode&0x00200000 )
					{
						pBuf += sprintf( pBuf, "MLAL" );
					}
					else
					{
						pBuf += sprintf( pBuf, "MULL" );
					}
					pBuf += sprintf( pBuf, "%s", pConditionCode );

					/* Set Status Flags */
					if( opcode&0x00100000 )
					{
						*pBuf++ = 'S';
					}
					pBuf = WritePadding( pBuf, pBuf0 );
					
					//Format is RLo,RHi,Rm,Rs
					pBuf += sprintf( pBuf,
						"R%d, R%d, R%d, R%d",
						(opcode>>12)&0xf,
						(opcode>>16)&0xf,
						(opcode&0xf),
						(opcode>>8)&0xf);
				}
				else 
				{
					/* Multiply */
					/* xxxx0000 00ASdddd nnnnssss 1001mmmm */

					/* Multiply & Accumulate? */
					if( opcode&0x00200000 )
					{
						pBuf += sprintf( pBuf, "MLA" );
					}
					/* Multiply */
					else
					{
						pBuf += sprintf( pBuf, "MUL" );
					}
					pBuf += sprintf( pBuf, "%s", pConditionCode );
					if( opcode&0x00100000 )
					{
						*pBuf++ = 'S';
					}
					pBuf = WritePadding( pBuf, pBuf0 );

					pBuf += sprintf( pBuf,
						"R%d, R%d, R%d",
						(opcode>>16)&0xf,
						(opcode&0xf),
						(opcode>>8)&0xf );

					if( opcode&0x00200000 )
					{
						pBuf += sprintf( pBuf, ", R%d", (opcode>>12)&0xf );
					}
				}
			}
		}
	}
	else if( (opcode&0x0c000000)==0 )		//bits 27-26 == 00 - This check can only exist properly after Multiplication check above
	{

		/* Data Processing OR PSR Transfer */

		//SJE: check for MRS & MSR ( S bit must be clear, and bit 24,23 = 10 )
		if( ((opcode&0x00100000)==0) && ((opcode&0x01800000)==0x01000000) ) {
			char strpsr[6];
			sprintf(strpsr, "%s",(opcode&0x400000)?"SPSR":"CPSR");

			//MSR ( bit 21 set )
			if( (opcode&0x00200000) ) {
				pBuf += sprintf(pBuf, "MSR%s",pConditionCode );
				//Flag Bits Only? (Bit 16 Clear)
				if( (opcode&0x10000)==0)	pBuf += sprintf(pBuf, "F");	
				pBuf = WritePadding( pBuf, pBuf0 );
				pBuf += sprintf(pBuf, "%s,",strpsr);
				WriteDataProcessingOperand(pBuf, opcode, (opcode&0x02000000)?1:0, 0, 1);
			}
			//MRS ( bit 21 clear )
			else {
				pBuf += sprintf(pBuf, "MRS%s",pConditionCode );
				pBuf = WritePadding( pBuf, pBuf0 );
				pBuf += sprintf(pBuf, "R%d,",(opcode>>12)&0x0f);
				pBuf += sprintf(pBuf, "%s",strpsr);
			}
		}
		else {
			/* Data Processing */
			/* xxxx001a aaaSnnnn ddddrrrr bbbbbbbb */
			/* xxxx000a aaaSnnnn ddddcccc ctttmmmm */
			int op=(opcode>>21)&0xf;
			pBuf += sprintf(
				pBuf, "%s%s",
				pOperation[op],
				pConditionCode );

			//SJE: corrected S-Bit bug here
			//if( (opcode&0x01000000) )
			if( (opcode&0x0100000) )
			{
				*pBuf++ = 'S';
			}

			pBuf = WritePadding( pBuf, pBuf0 );

			switch (op) {
			case 0x00:
			case 0x01:
			case 0x02:
			case 0x03:
			case 0x04:
			case 0x05:
			case 0x06:
			case 0x07:
			case 0x0c:
			case 0x0e:
				WriteDataProcessingOperand(pBuf, opcode, 1, 1, 1);
				break;
			case 0x08:
			case 0x09:
			case 0x0a:
			case 0x0b:
				WriteDataProcessingOperand(pBuf, opcode, 0, 1, 1);
				break;
			case 0x0d:
			case 0x0f:
				WriteDataProcessingOperand(pBuf, opcode, 1, 0, 1);
				break;
			}
		}
	}
	else if( (opcode&0x0c000000)==0x04000000 )		//bits 27-26 == 01
	{
		UINT32 rn = 0;
		UINT32 rnv = 0;

		/* Data Transfer */

		/* xxxx010P UBWLnnnn ddddoooo oooooooo  Immediate form */
		/* xxxx011P UBWLnnnn ddddcccc ctt0mmmm  Register form */
		if( opcode&0x00100000 )
		{
			pBuf += sprintf( pBuf, "LDR" );
		}
		else
		{
			pBuf += sprintf( pBuf, "STR" );
		}
		pBuf += sprintf( pBuf, "%s", pConditionCode );

		if( opcode&0x00400000 )
		{
			pBuf += sprintf( pBuf, "B" );
		}

		if( opcode&0x00200000 )
		{
			/* writeback addr */
			if( opcode&0x01000000 )
			{
				/* pre-indexed addressing */
				pBuf += sprintf( pBuf, "!" );
			}
			else
			{
				/* post-indexed addressing */
				pBuf += sprintf( pBuf, "T" );
			}
		}

		pBuf = WritePadding( pBuf, pBuf0 );
		pBuf += sprintf( pBuf, "R%d, [R%d",
			(opcode>>12)&0xf, (opcode>>16)&0xf );

		//grab value of pc if used as base register
		rn = (opcode>>16)&0xf;
		if(rn==15) rnv = pc+8;

		if( opcode&0x02000000 )
		{
			/* register form */
			pBuf += sprintf( pBuf, "%s",(opcode&0x01000000)?"":"]" );
			pBuf = WriteRegisterOperand1( pBuf, opcode );
			pBuf += sprintf( pBuf, "%s",(opcode&0x01000000)?"]":"" );
		}
		else
		{
			/* immediate form */
			pBuf += sprintf( pBuf, "%s",(opcode&0x01000000)?"":"]" );
			//hide zero offsets
			if(opcode&0xfff) {
				if( opcode&0x00800000 )
				{
					pBuf += sprintf( pBuf, ", #$%x", opcode&0xfff );
					rnv += (rnv)?opcode&0xfff:0;
				}
				else
				{
					pBuf += sprintf( pBuf, ", -#$%x", opcode&0xfff );
					rnv -= (rnv)?opcode&0xfff:0;
				}
			}
			pBuf += sprintf( pBuf, "%s",(opcode&0x01000000)?"]":"" );
			//show where the read will occur if we found a value
			if(rnv) pBuf += sprintf( pBuf, " (%x)",rnv);
		}
	}
	else if( (opcode&0x0e000000) == 0x08000000 )		//bits 27-25 == 100
	{
		/* xxxx100P USWLnnnn llllllll llllllll */
		/* Block Data Transfer */

		if( opcode&0x00100000 )
		{
			pBuf += sprintf( pBuf, "LDM" );
		}
		else
		{
			pBuf += sprintf( pBuf, "STM" );
		}
		pBuf += sprintf( pBuf, "%s", pConditionCode );

		if( opcode&0x01000000 )
		{
			pBuf += sprintf( pBuf, "P" );
		}
		if( opcode&0x00800000 )
		{
			pBuf += sprintf( pBuf, "U" );
		}
		if( opcode&0x00400000 )
		{
			pBuf += sprintf( pBuf, "^" );
		}
		if( opcode&0x00200000 )
		{
			pBuf += sprintf( pBuf, "W" );
		}

		pBuf = WritePadding( pBuf, pBuf0 );
		pBuf += sprintf( pBuf, "[R%d], {",(opcode>>16)&0xf);

		{
			int j=0,last=0,found=0;
			for (j=0; j<16; j++) {
				if (opcode&(1<<j) && found==0) {
					found=1;
					last=j;
				}
				else if ((opcode&(1<<j))==0 && found) {
					if (last==j-1)
						pBuf += sprintf( pBuf, " R%d,",last);
					else
						pBuf += sprintf( pBuf, " R%d-R%d,",last,j-1);
					found=0;
				}
			}
			if (found && last==15)
				pBuf += sprintf( pBuf, " R15,");
			else if (found)
				pBuf += sprintf( pBuf, " R%d-R%d,",last,15);
		}

		pBuf--;
		pBuf += sprintf( pBuf, " }");
	}
	else if( (opcode&0x0e000000)==0x0a000000 )		//bits 27-25 == 101
	{
		/* branch instruction */
		/* xxxx101L oooooooo oooooooo oooooooo */
		if( opcode&0x01000000 )
		{
			pBuf += sprintf( pBuf, "BL" );
		}
		else
		{
			pBuf += sprintf( pBuf, "B" );
		}

		pBuf += sprintf( pBuf, "%s", pConditionCode );

		pBuf = WritePadding( pBuf, pBuf0 );

		pBuf = WriteBranchAddress( pBuf, pc, opcode );
	}
	else if( (opcode&0x0e000000)==0x0c000000 )		//bits 27-25 == 110
	{
		/* co processor data transfer */
		if(arm7_dasm_cop_dt_callback)
			arm7_dasm_cop_dt_callback(pBuf,opcode,(char*)pConditionCode,(char*)pBuf0);
		else
			DasmCoProc_DT(pBuf,opcode,(char*)pConditionCode,(char*)pBuf0);
	}
	else if( (opcode&0x0f000000)==0x0e000000 )		//bits 27-24 == 1110
	{
		/* co processor data operation or register transfer */

		//Register Transfer
		if(opcode&0x10)
		{
			if(arm7_dasm_cop_rt_callback)
				arm7_dasm_cop_rt_callback(pBuf,opcode,(char*)pConditionCode,(char*)pBuf0);
			else
				DasmCoProc_RT(pBuf,opcode,(char*)pConditionCode,(char*)pBuf0);
		}
		//Data Op
		else
		{
			if(arm7_dasm_cop_do_callback)
				arm7_dasm_cop_do_callback(pBuf,opcode,(char*)pConditionCode,(char*)pBuf0);
			else
				DasmCoProc_DO(pBuf,opcode,(char*)pConditionCode,(char*)pBuf0);
		}
	}
	else if( (opcode&0x0f000000) == 0x0f000000 )	//bits 27-24 == 1111
	{
		/* Software Interrupt */
		pBuf += sprintf( pBuf, "SWI%s $%x",
			pConditionCode,
			opcode&0x00ffffff );
	}
	else
	{
		pBuf += sprintf( pBuf, "Undefined" );
	}
}

#endif	//MAME_DEBUG
