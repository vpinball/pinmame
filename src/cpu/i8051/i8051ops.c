/*******************************************************************************************
 NOTE: All registers are accessed directly, instead of using the SFR_R() function for speed
 Direct register access is availabe from the R_(register name) macros.. ex: R_ACC for the ACC
 with the exception of the PC
********************************************************************************************/

//ACALL code addr							/* 1: aaa1 0001 */
INLINE void acall(void)
{
	UINT8 op = ROP(PC-1);					//Grab the opcode for ACALL
	UINT8 addr = ROP_ARG(PC++);				//Grab code address byte
	push_pc();								//Save PC to the stack
	PC = ((op<<3) & 0x700) | addr;			//Set new PC
}

//ADD A, #data								/* 1: 0010 0100 */
INLINE void add_a_byte(void)
{
	UINT8 data = ROP_ARG(PC++);		//Grab data
}

//ADD A, data addr							/* 1: 0010 0101 */
INLINE void add_a_mem(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
}

//ADD A, @R0/@R1							/* 1: 0010 011i */
INLINE void add_a_ir(int r)
{
}

//ADD A, R0 to R7							/* 1: 0010 1rrr */
INLINE void add_a_r(int r)
{
}

//ADDC A, #data								/* 1: 0011 0100 */
INLINE void addc_a_byte(void)
{
	UINT8 data = ROP_ARG(PC++);		//Grab data
}

//ADDC A, data addr							/* 1: 0011 0101 */
INLINE void addc_a_mem(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
}

//ADDC A, @R0/@R1							/* 1: 0011 011i */
INLINE void addc_a_ir(int r)
{
}

//ADDC A, R0 to R7							/* 1: 0011 1rrr */
INLINE void addc_a_r(int r)
{
}

//AJMP code addr							/* 1: aaa0 0001 */
INLINE void ajmp(void)
{
	UINT8 op = ROP(PC-1);					//Grab the opcode for AJMP
	UINT8 addr = ROP_ARG(PC++);				//Grab code address byte
	PC = ((op<<3) & 0x700) | addr;			//Set new PC
}

//ANL data addr, A							/* 1: 0101 0010 */
INLINE void anl_mem_a(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
}

//ANL data addr, #data						/* 1: 0101 0011 */
INLINE void anl_mem_byte(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	UINT8 data = ROP_ARG(PC++);		//Grab data
}

//ANL A, #data								/* 1: 0101 0100 */
INLINE void anl_a_byte(void)
{
	UINT8 data = ROP_ARG(PC++);		//Grab data
}

//ANL A, data addr							/* 1: 0101 0101 */
INLINE void anl_a_mem(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
}

//ANL A, @RO/@R1							/* 1: 0101 011i */
INLINE void anl_a_ir(int r)
{
}

//ANL A, RO to R7							/* 1: 0101 1rrr */
INLINE void anl_a_r(int r)
{
}

//ANL C, bit addr							/* 1: 1000 0010 */
INLINE void anl_c_bitaddr(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab bit address
}

//ANL C,/bit addr							/* 1: 1011 0000 */
INLINE void anl_c_nbitaddr(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab bit address
}

//CJNE A, #data, code addr					/* 1: 1011 0100 */
INLINE void cjne_a_byte(void)
{
	UINT8 data = ROP_ARG(PC++);		//Grab data
	INT8 rel_addr = ROP_ARG(PC++);	//Grab relative code address
}

//CJNE A, data addr, code addr				/* 1: 1011 0101 */
INLINE void cjne_a_mem(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	INT8 rel_addr = ROP_ARG(PC++);	//Grab relative code address
}

//CJNE @R0/@R1, #data, code addr			/* 1: 1011 011i */
INLINE void cjne_ir_byte(int r)
{
	UINT8 data = ROP_ARG(PC++);		//Grab data
	INT8 rel_addr = ROP_ARG(PC++);	//Grab relative code address
}

//CJNE R0 to R7, #data, code addr			/* 1: 1011 1rrr */
INLINE void cjne_r_byte(int r)
{
	UINT8 data = ROP_ARG(PC++);		//Grab data
	INT8 rel_addr = ROP_ARG(PC++);	//Grab relative code address
}

//CLR bit addr								/* 1: 1100 0010 */
INLINE void clr_bitaddr(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab bit address
}

//CLR C										/* 1: 1100 0011 */
INLINE void clr_c(void)
{
}

//CLR A										/* 1: 1110 0100 */
INLINE void clr_a(void)
{
}

//CPL bit addr								/* 1: 1011 0010 */
INLINE void cpl_bitaddr(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab bit address
}

//CPL C										/* 1: 1011 0011 */
INLINE void cpl_c(void)
{
}

//CPL A										/* 1: 1111 0100 */
INLINE void cpl_a(void)
{
}

//DA A										/* 1: 1101 0100 */
INLINE void da_a(void)
{
}

//DEC A										/* 1: 0001 0100 */
INLINE void dec_a(void)
{
	//NOTE: If value is 0, value becomes 0xFF (Handled by C++ automatically)
	SFR_W(ACC,R_ACC-1);
}

//DEC data addr								/* 1: 0001 0101 */
INLINE void dec_mem(void)
{
	//NOTE: If value is 0, value becomes 0xFF (Handled by C++ automatically)
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	IRAM_W(addr,(UINT8)IRAM_R(addr)-1);
}

//DEC @R0/@R1								/* 1: 0001 011i */
INLINE void dec_ir(int r)
{
	//NOTE: If value is 0, value becomes 0xFF (Handled by C++ automatically)
	IRAM_W(R_R(r),(UINT8)IRAM_R(R_R(r))-1);
}

//DEC R0 to R7								/* 1: 0001 1rrr */
INLINE void dec_r(int r)
{
	//NOTE: If value is 0, value becomes 0xFF (Handled by C++ automatically)
	R_R(r) = R_R(r) - 1;
}

//DIV AB									/* 1: 1000 0100 */
INLINE void div_ab(void)
{
}

//DJNZ data addr, code addr					/* 1: 1101 0101 */
INLINE void djnz_mem(void)
{
	UINT8 addr = ROP_ARG(PC++);			//Grab data address
	INT8 rel_addr = ROP_ARG(PC++);		//Grab relative code address
	if(IRAM_R(addr) != 0)				//Branch if contents of data address is not 0
		PC = PC + rel_addr;
}

//DJNZ R0 to R7,code addr					/* 1: 1101 1rrr */
INLINE void djnz_r(int r)
{
	INT8 rel_addr = ROP_ARG(PC++);		//Grab relative code address
	if(R_R(r) != 0)						//Branchif contents of R0 - R7 is not 0
		PC = PC + rel_addr;
}

//INC A										/* 1: 0000 0100 */
INLINE void inc_a(void)
{
	//NOTE: If value is 0xff, value becomes 0 (Handled by C++ automatically)
	SFR_W(ACC,R_ACC+1);
}

//INC data addr								/* 1: 0000 0101 */
INLINE void inc_mem(void)
{
	//NOTE: If value is 0xff, value becomes 0 (Handled by C++ automatically)
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	IRAM_W(addr,(UINT8)IRAM_R(addr)+1);
}

//INC @R0/@R1								/* 1: 0000 011i */
INLINE void inc_ir(int r)
{
	//NOTE: If value is 0xff, value becomes 0 (Handled by C++ automatically)
	IRAM_W(R_R(r),(UINT8)IRAM_R(R_R(r))+1);
}

//INC R0 to R7								/* 1: 0000 1rrr */
INLINE void inc_r(int r)
{
	//NOTE: If value is 0xff, value becomes 0 (Handled by C++ automatically)
	R_R(r) = R_R(r) + 1;
}

//INC DPTR									/* 1: 1010 0011 */
INLINE void inc_dptr(void)
{
	//NOTE: If value is 0xffff, value becomes 0 (Handled by C++ automatically)
	UINT16 dptr = (R_DPTR)+1;
	DPTR_W(dptr);
}

//JB  bit addr, code addr					/* 1: 0010 0000 */
INLINE void jb(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab bit address
	INT8 rel_addr = ROP_ARG(PC++);	//Grab relative code address
}

//JBC bit addr, code addr					/* 1: 0001 0000 */
INLINE void jbc(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab bit address
	INT8 rel_addr = ROP_ARG(PC++);	//Grab relative code address
}

//JC code addr								/* 1: 0100 0000 */
INLINE void jc(void)
{
	INT8 rel_addr = ROP_ARG(PC++);	//Grab relative code address
}

//JMP @A+DPTR								/* 1: 0111 0011 */
INLINE void jmp_iadptr(void)
{
	PC = R_ACC+R_DPTR;
}

//JNB bit addr, code addr					/* 1: 0011 0000 */
INLINE void jnb(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab bit address
	INT8 rel_addr = ROP_ARG(PC++);	//Grab relative code address
}

//JNC code addr								/* 1: 0101 0000 */
INLINE void jnc(void)
{
	INT8 rel_addr = ROP_ARG(PC++);	//Grab relative code address
}

//JNZ code addr								/* 1: 0111 0000 */
INLINE void jnz(void)
{
	INT8 rel_addr = ROP_ARG(PC++);		//Grab relative code address
	if(R_ACC != 0)						//Branch if ACC is not 0
		PC = PC+rel_addr;
}

//JZ code addr								/* 1: 0110 0000 */
INLINE void jz(void)
{
	INT8 rel_addr = ROP_ARG(PC++);		//Grab relative code address
	if(R_ACC == 0)						//Branch if ACC is 0
		PC = PC+rel_addr;
}

//LCALL code addr							/* 1: 0001 0010 */
INLINE void lcall(void)
{
	UINT8 addr_hi, addr_lo;
	addr_hi = ROP_ARG(PC++);
	addr_lo = ROP_ARG(PC++);
	push_pc();
	PC = (addr_hi<<8) | addr_lo;
}

//LJMP code addr							/* 1: 0000 0010 */
INLINE void ljmp(void)
{
	UINT8 addr_hi, addr_lo;
	addr_hi = ROP_ARG(PC++);
	addr_lo = ROP_ARG(PC++);
	PC = (addr_hi<<8) | addr_lo;
}

//MOV A, #data								/* 1: 0111 0100 */
INLINE void mov_a_byte(void)
{
	UINT8 data = ROP_ARG(PC++);		//Grab data
	SFR_W(ACC,data);				//Store data to ACC
}

//MOV A, data addr							/* 1: 1110 0101 */
INLINE void mov_a_mem(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	SFR_W(ACC,IRAM_R(addr));		//Store contents of data address to ACC
}

//MOV A,@RO/@R1								/* 1: 1110 011i */
INLINE void mov_a_ir(int r)
{
	SFR_W(ACC,IRAM_R(R_R(r)));		//Store contents of address pointed by R0 or R1 to ACC
}

//MOV A,R0 to R7							/* 1: 1110 1rrr */
INLINE void mov_a_r(int r)
{
	SFR_W(ACC,R_R(r));				//Store contents of R0 - R7 to ACC
}

//MOV data addr, #data						/* 1: 0111 0101 */
INLINE void mov_mem_byte(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	UINT8 data = ROP_ARG(PC++);		//Grab data
	IRAM_W(addr,data);				//Store data to data address location
}

//MOV data addr, data addr					/* 1: 1000 0101 */
INLINE void mov_mem_mem(void)
{
	//1st address is src, 2nd is dst, but the mov command works as mov dst,src)
	UINT8 src,dst;
	src = ROP_ARG(PC++);			//Grab source data address
	dst = ROP_ARG(PC++);			//Grab destination data address
	IRAM_W(dst,IRAM_R(src));		//Read source address contents and store to destinatin address
}

//MOV @R0/@R1, #data						/* 1: 0111 011i */
INLINE void mov_ir_byte(int r)
{
	UINT8 data = ROP_ARG(PC++);		//Grab data
	IRAM_W(R_R(r),data);			//Store data to address pointed by R0 or R1
}

//MOV R0 to R7, #data						/* 1: 0111 1rrr */
INLINE void mov_r_byte(int r)
{
	UINT8 data = ROP_ARG(PC++);		//Grab data
	R_R(r) = data;					//Store to R0 - R7
}

//MOV data addr, @R0/@R1					/* 1: 1000 011i */
INLINE void mov_mem_ir(int r)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	IRAM_W(addr,IRAM_R(R_R(r)));	//Store contents pointed to by R0 or R1 to data address
}

//MOV data addr,R0 to R7					/* 1: 1000 1rrr */
INLINE void mov_mem_r(int r)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	IRAM_W(addr,R_R(r));			//Store contents of R0 - R7 to data address
}

//MOV DPTR, #data16							/* 1: 1001 0000 */
INLINE void mov_dptr_byte(void)
{
	UINT8 data_hi, data_lo;
	data_hi = ROP_ARG(PC++);				//Grab hi byte
	data_lo = ROP_ARG(PC++);				//Grab lo byte
	DPTR_W((UINT16)((data_hi<<8)|data_lo));	//Store to DPTR
}

//MOV bit addr, C							/* 1: 1001 0010 */
INLINE void mov_bitaddr_c(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab bit address
}

//MOV @R0/@R1, data addr					/* 1: 1010 011i */
INLINE void mov_ir_mem(int r)
{
	UINT8 addr = ROP_ARG(PC++);				//Grab data address
	IRAM_W(IRAM_R(R_R(r)),IRAM_R(addr));	//Store data from data address to address pointed to by R0 or R1
}

//MOV R0 to R7, data addr					/* 1: 1010 1rrr */
INLINE void mov_r_mem(int r)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	R_R(r) = IRAM_R(addr);			//Store to R0 - R7
}

//MOV data addr, A							/* 1: 1111 0101 */
INLINE void mov_mem_a(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	IRAM_W(addr,R_ACC);				//Store A to data address
}

//MOV @R0/@R1, A							/* 1: 1111 011i */
INLINE void mov_ir_a(int r)
{
	IRAM_W(IRAM_R(R_R(r)),R_ACC);	//Store A to location pointed to by R0 or R1
}

//MOV R0 to R7, A							/* 1: 1111 1rrr */
INLINE void mov_r_a(int r)
{
	R_R(r) = R_ACC;	//Store A to R0-R7
}

//MOVC A, @A + PC							/* 1: 1000 0011 */
INLINE void movc_a_iapc(void)
{
}

//MOV C, bit addr							/* 1: 1010 0010 */
INLINE void mov_c_bitaddr(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab bit address
}

//MOVC A, @A + DPTR							/* 1: 1001 0011 */
INLINE void movc_a_iadptr(void)
{
}

//MOVX A,@DPTR								/* 1: 1110 0000 */
INLINE void movx_a_idptr(void)
{
	UINT8 byte = RM(R_DPTR);	//Grab 1 byte from External memory pointed to by dptr
	SFR_W(ACC,byte);			//Store to ACC
}

//MOVX A, @R0/@R1							/* 1: 1110 001i */
INLINE void movx_a_ir(int r)
{
	UINT8 byte = RM(IRAM_R(R_R(r)));	//Grab 1 byte from External memory pointed to by R0 or R1
	SFR_W(ACC,byte);					//Store to ACC
}

//MOVX @DPTR,A								/* 1: 1111 0000 */
INLINE void movx_idptr_a(void)
{
	WM(R_DPTR, R_ACC);					//Store ACC to External memory address pointed to by DPTR
}

//MOVX @R0/@R1,A							/* 1: 1111 001i */
INLINE void movx_ir_a(int r)
{
	UINT8 addr = IRAM_R(R_R(r));	//Grab address by reading location pointed to by R0 or R1
	WM(addr, R_ACC);				//Store ACC to External memory address
}

//MUL AB									/* 1: 1010 0100 */
INLINE void mul_ab(void)
{
}

//NOP										/* 1: 0000 0000 */
INLINE void nop(void)
{
}

//ORL data addr, A							/* 1: 0100 0010 */
INLINE void orl_mem_a(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
}

//ORL data addr, #data						/* 1: 0100 0011 */
INLINE void orl_mem_byte(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	UINT8 data = ROP_ARG(PC++);		//Grab data
}

//ORL A, #data								/* 1: 0100 0100 */
INLINE void orl_a_byte(void)
{
	UINT8 data = ROP_ARG(PC++);		//Grab data
}

//ORL A, data addr							/* 1: 0100 0101 */
INLINE void orl_a_mem(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
}

//ORL A, @RO/@R1							/* 1: 0100 011i */
INLINE void orl_a_ir(int r)
{
}

//ORL A, RO to R7							/* 1: 0100 1rrr */
INLINE void orl_a_r(int r)
{
}

//ORL C, bit addr							/* 1: 0111 0010 */
INLINE void orl_c_bitaddr(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab bit address
}

//ORL C, /bit addr							/* 1: 1010 0000 */
INLINE void orl_c_nbitaddr(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab bit address
}

//POP data addr								/* 1: 1101 0000 */
INLINE void pop(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	IRAM_W(addr, IRAM_R(R_SP));		//Store to contents of data addr, data pointed to by Stack
	SFR_W(SP,R_SP-1);				//Decrement SP
}

//PUSH data addr							/* 1: 1100 0000 */
INLINE void push(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	UINT8 tmpSP = R_SP;				//Grab and Increment Stack Pointer
	tmpSP++;						// ""
	SFR_W(SP,tmpSP);				// ""
    if (tmpSP == R_SP)				//Ensure it was able to write to new stack location
		IRAM_W(tmpSP, IRAM_R(addr));//Store to stack contents of data address
}

//RET										/* 1: 0010 0010 */
INLINE void ret(void)
{
	pop_pc();
}

//RETI										/* 1: 0011 0010 */
INLINE void reti(void)
{
	pop_pc();
	//Todo: Clear Interrupt flags
}

//RL A										/* 1: 0010 0011 */
INLINE void rl_a(void)
{
}

//RLC A										/* 1: 0011 0011 */
INLINE void rlc_a(void)
{
}

//RR A										/* 1: 0000 0011 */
INLINE void rr_a(void)
{
}

//RRC A										/* 1: 0001 0011 */
INLINE void rrc_a(void)
{
}

//SETB C									/* 1: 1101 0011 */
INLINE void setb_c(void)
{
}

//SETB bit addr								/* 1: 1101 0010 */
INLINE void setb_bitaddr(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab bit address
}

//SJMP code addr							/* 1: 1000 0000 */
INLINE void sjmp(void)
{
	INT8 rel_addr = ROP_ARG(PC++);	//Grab relative code address
	PC = PC + rel_addr;				//Update PC
}

//SUBB A, #data								/* 1: 1001 0100 */
INLINE void subb_a_byte(void)
{
	UINT8 data = ROP_ARG(PC++);		//Grab data
}

//SUBB A, data addr							/* 1: 1001 0101 */
INLINE void subb_a_mem(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
}

//SUBB A, @R0/@R1							/* 1: 1001 011i */
INLINE void subb_a_ir(int r)
{
}

//SUBB A, R0 to R7							/* 1: 1001 1rrr */
INLINE void subb_a_r(int r)
{
}

//SWAP A									/* 1: 1100 0100 */
INLINE void swap_a(void)
{
	UINT8 a_nib_lo, a_nib_hi;
	a_nib_hi = (R_ACC & 0x0f) << 4;			//Grab lo byte of ACC and move to hi
	a_nib_lo = (R_ACC & 0xf0) >> 4;			//Grab hi byte of ACC and move to lo
	SFR_W(ACC, a_nib_hi | a_nib_lo);
}

//XCH A, data addr							/* 1: 1100 0101 */
INLINE void xch_a_mem(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
}

//XCH A, @RO/@R1							/* 1: 1100 011i */
INLINE void xch_a_ir(int r)
{
}

//XCH A, RO to R7							/* 1: 1100 1rrr */
INLINE void xch_a_r(int r)
{
}

//XCHD A, @R0/@R1							/* 1: 1101 011i */
INLINE void xchd_a_ir(int r)
{
}

//XRL data addr, A							/* 1: 0110 0010 */
INLINE void xrl_mem_a(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
}

//XRL data addr, #data						/* 1: 0110 0011 */
INLINE void xrl_mem_byte(void)
{
	UINT8 data = ROP_ARG(PC++);		//Grab data
}

//XRL A, #data								/* 1: 0110 0100 */
INLINE void xrl_a_byte(void)
{
	UINT8 data = ROP_ARG(PC++);		//Grab data
}

//XRL A, data addr							/* 1: 0110 0101 */
INLINE void xrl_a_mem(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
}

//XRL A, @R0/@R1							/* 1: 0110 011i */
INLINE void xrl_a_ir(int r)
{
}

//XRL A, R0 to R7							/* 1: 0110 1rrr */
INLINE void xrl_a_r(int r)
{
}

//illegal opcodes
INLINE void illegal(void)
{
	logerror("i8051 #%d: illegal opcode at 0x%03x: %02x\n", cpu_getactivecpu(), PC, ROP(PC));
}
