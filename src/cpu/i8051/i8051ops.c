/*******************************************************************************************
 NOTE: All registers are accessed directly, instead of using the SFR_R() function for speed
 Direct register access is availabe from the R_(register name) macros.. ex: R_ACC for the ACC
 with the exception of the PC
********************************************************************************************/

//Todo: Don't forget to ensure that all operations done on a port address read the data latch, not the 
//      input pins (and clarify what that means, ie, we don't issue a port_read/port_write?)


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
	UINT16 tmpRes = (UINT16)(R_ACC + data);
	SFR_W(ACC,(UINT8)tmpRes & 0xff);	//Store 8 bit result of addtion in ACC
	//TODO: figure out & implement the flag settings (CY,AC,OV)
#if 0
	//TODO: figure out the rest of these, and test
	//Set Flags
	SET_CY( ((tmpRes & 0x100) >> 8) );		//Carry Flag Set/Cleared based on result's bit 7
	SET_AC( ((tmpRes & 0x10) >> 4) );		//Alt. Carry Flag Set/Cleared based on result's bit 4
	SET_OV( ?? )
#endif
}

//ADD A, data addr							/* 1: 0010 0101 */
INLINE void add_a_mem(void)
{
	UINT8 addr = ROP_ARG(PC++);						//Grab data address
	UINT16 tmpRes = (UINT16)(R_ACC + IRAM_R(addr));	//Add value stored at data address to ACC
	SFR_W(ACC,(UINT8)tmpRes & 0xff);				//Store 8 bit result of addtion in ACC
	//TODO: figure out & implement the flag settings (CY,AC,OV)
}

//ADD A, @R0/@R1							/* 1: 0010 011i */
INLINE void add_a_ir(int r)
{
	UINT16 tmpRes = (UINT16)(R_ACC + IRAM_R(R_R(r)));	//Add value of memory pointed to by R0 or R1 to Acc
	SFR_W(ACC,(UINT8)tmpRes & 0xff);					//Store 8 bit result of addtion in ACC
	//TODO: figure out & implement the flag settings (CY,AC,OV)
}

//ADD A, R0 to R7							/* 1: 0010 1rrr */
INLINE void add_a_r(int r)
{
	UINT16 tmpRes = (UINT16)(R_ACC + R_R(r));	//Add value of R0-R7 to Acc
	SFR_W(ACC,(UINT8)tmpRes & 0xff);			//Store 8 bit result of addtion in ACC
	//TODO: figure out & implement the flag settings (CY,AC,OV)
}

//ADDC A, #data								/* 1: 0011 0100 */
INLINE void addc_a_byte(void)
{
	UINT8 data = ROP_ARG(PC++);							//Grab data
	UINT16 tmpRes = (UINT16)(R_ACC + data + GET_CY);	//Add Data + Carry Flag to ACC
	SFR_W(ACC,(UINT8)tmpRes & 0xff);					//Store 8 bit result of addtion in ACC
	//TODO: figure out & implement the flag settings (CY,AC,OV)
}

//ADDC A, data addr							/* 1: 0011 0101 */
INLINE void addc_a_mem(void)
{
	UINT8 addr = ROP_ARG(PC++);									//Grab data address
	UINT16 tmpRes = (UINT16)(R_ACC + IRAM_R(addr) + GET_CY);	//Add value stored at data address + Carry Flag to ACC
	SFR_W(ACC,(UINT8)tmpRes & 0xff);							//Store 8 bit result of addtion in ACC
	//TODO: figure out & implement the flag settings (CY,AC,OV)
}

//ADDC A, @R0/@R1							/* 1: 0011 011i */
INLINE void addc_a_ir(int r)
{
	UINT16 tmpRes = (UINT16)(R_ACC + IRAM_R(R_R(r)) + GET_CY);	//Add value of memory pointed to by R0 or R1 + Carry Flag To Acc
	SFR_W(ACC,(UINT8)tmpRes & 0xff);							//Store 8 bit result of addtion in ACC
	//TODO: figure out & implement the flag settings (CY,AC,OV)
}

//ADDC A, R0 to R7							/* 1: 0011 1rrr */
INLINE void addc_a_r(int r)
{
	UINT16 tmpRes = (UINT16)(R_ACC + R_R(r) + GET_CY);		//Add value of R0-R7 + Carry Flag to Acc
	SFR_W(ACC,(UINT8)tmpRes & 0xff);						//Store 8 bit result of addtion in ACC
	//TODO: figure out & implement the flag settings (CY,AC,OV)
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
	UINT8 data = IRAM_R(addr);		//Grab data from data address
	IRAM_W(addr,data & R_ACC);		//Set data address value to it's value Logical AND with ACC
}

//ANL data addr, #data						/* 1: 0101 0011 */
INLINE void anl_mem_byte(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	UINT8 data = ROP_ARG(PC++);		//Grab data
	UINT8 srcdata = IRAM_R(addr);	//Grab data from data address
	IRAM_W(addr,srcdata & data);	//Set data address value to it's value Logical AND with Data
}

//ANL A, #data								/* 1: 0101 0100 */
INLINE void anl_a_byte(void)
{
	UINT8 data = ROP_ARG(PC++);		//Grab data
	SFR_W(ACC,R_ACC & data);		//Set ACC to value of ACC Logical AND with Data
}

//ANL A, data addr							/* 1: 0101 0101 */
INLINE void anl_a_mem(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	UINT8 data = IRAM_R(addr);		//Grab data from data address
	SFR_W(ACC,R_ACC & data);		//Set ACC to value of ACC Logical AND with Data
}

//ANL A, @RO/@R1							/* 1: 0101 011i */
INLINE void anl_a_ir(int r)
{
	UINT8 data = IRAM_R(R_R(r));	//Grab data from address R0 or R1 points to
	SFR_W(ACC,R_ACC & data);		//Set ACC to value of ACC Logical AND with Data
}

//ANL A, RO to R7							/* 1: 0101 1rrr */
INLINE void anl_a_r(int r)
{
	UINT8 data = R_R(r);			//Grab data from R0 - R7
	SFR_W(ACC,R_ACC & data);		//Set ACC to value of ACC Logical AND with Data
}

//ANL C, bit addr							/* 1: 1000 0010 */
INLINE void anl_c_bitaddr(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab bit address
	//Todo: Implement bit addressing
}

//ANL C,/bit addr							/* 1: 1011 0000 */
INLINE void anl_c_nbitaddr(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab bit address
	//Todo: Implement bit addressing
}

//CJNE A, #data, code addr					/* 1: 1011 0100 */
INLINE void cjne_a_byte(void)
{
	//Todo: Find out if carry flag is set even if compare is =
	UINT8 data = ROP_ARG(PC++);		//Grab data
	INT8 rel_addr = ROP_ARG(PC++);	//Grab relative code address

	if(R_ACC != data)				//Jump if values are not equal
		PC = PC + rel_addr;

	//Set carry flag to 1 if 1st compare value is < 2nd compare value
	SET_CY( (R_ACC < data) );
}

//CJNE A, data addr, code addr				/* 1: 1011 0101 */
INLINE void cjne_a_mem(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	INT8 rel_addr = ROP_ARG(PC++);	//Grab relative code address
	UINT8 data = IRAM_R(addr);		//Pull value from data address

	if(R_ACC != data)				//Jump if values are not equal
		PC = PC + rel_addr;

	//Set carry flag to 1 if 1st compare value is < 2nd compare value
	SET_CY( (R_ACC < data) );
}

//CJNE @R0/@R1, #data, code addr			/* 1: 1011 011i */
INLINE void cjne_ir_byte(int r)
{
	UINT8 data = ROP_ARG(PC++);		//Grab data
	INT8 rel_addr = ROP_ARG(PC++);	//Grab relative code address
	UINT8 srcdata = IRAM_R(R_R(r));	//Grab value pointed to by R0 or R1

	if(srcdata != data)				//Jump if values are not equal
		PC = PC + rel_addr;

	//Set carry flag to 1 if 1st compare value is < 2nd compare value
	SET_CY( (srcdata < data) );
}

//CJNE R0 to R7, #data, code addr			/* 1: 1011 1rrr */
INLINE void cjne_r_byte(int r)
{
	UINT8 data = ROP_ARG(PC++);		//Grab data
	INT8 rel_addr = ROP_ARG(PC++);	//Grab relative code address
	UINT8 srcdata = R_R(r);			//Grab value of R0 - R7

	if(srcdata != data)				//Jump if values are not equal
		PC = PC + rel_addr;

	//Set carry flag to 1 if 1st compare value is < 2nd compare value
	SET_CY( (srcdata < data) );
}

//CLR bit addr								/* 1: 1100 0010 */
INLINE void clr_bitaddr(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab bit address
	//Todo: Implement bit addressing
}

//CLR C										/* 1: 1100 0011 */
INLINE void clr_c(void)
{
	SET_CY(0);						//Clear Carry Flag
}

//CLR A										/* 1: 1110 0100 */
INLINE void clr_a(void)
{
	SFR_W(ACC,0);					//Clear Accumulator
}

//CPL bit addr								/* 1: 1011 0010 */
INLINE void cpl_bitaddr(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab bit address
	//Todo: Implement bit addressing
}

//CPL C										/* 1: 1011 0011 */
INLINE void cpl_c(void)
{
	int bit = (~GET_CY)&1;			//Complement Carry Flag
	SET_CY(bit);					
}

//CPL A										/* 1: 1111 0100 */
INLINE void cpl_a(void)
{
	SFR_W(ACC,((~R_ACC)&0xff));		//Complement Accumulator
}

//DA A										/* 1: 1101 0100 */
INLINE void da_a(void)
{
	//Todo: Yuch, I don't really understand what this opcode does
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
	//Todo: Confirm this works as the manual suggest
	UINT16 result;
	if( R_B == 0 ) {
		//Overflow flag is set!
		SET_OV(1);
		//Todo: Manual says if B is 0, results of A register AND B register is undefined.. so what do we put here?
		SFR_W(ACC,0xff);
		SFR_W(B,0xff);
	}
	else {
		result = R_ACC/R_B;
		//A gets hi bits, B gets lo bits of result
		SFR_W(ACC,(UINT8)((result & 0xFF00) >> 8));
		SFR_W(B,(UINT8)(result & 0xFF));
		//Overflow flag is cleared
		SET_OV(0);
	}
	//Carry Flag is always cleared
	SET_CY(0);
}

//DJNZ data addr, code addr					/* 1: 1101 0101 */
INLINE void djnz_mem(void)
{
	UINT8 addr = ROP_ARG(PC++);			//Grab data address
	INT8 rel_addr = ROP_ARG(PC++);		//Grab relative code address
	IRAM_W(addr,IRAM_R(addr) - 1);		//Decrement value contained at data address
	if(IRAM_R(addr) != 0)				//Branch if contents of data address is not 0
		PC = PC + rel_addr;
}

//DJNZ R0 to R7,code addr					/* 1: 1101 1rrr */
INLINE void djnz_r(int r)
{
	INT8 rel_addr = ROP_ARG(PC++);		//Grab relative code address
	R_R(r) = R_R(r) - 1;				//Decrement value
	if(R_R(r) != 0)						//Branch if contents of R0 - R7 is not 0
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
	//Todo: Implement bit addressing
}

//JBC bit addr, code addr					/* 1: 0001 0000 */
INLINE void jbc(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab bit address
	INT8 rel_addr = ROP_ARG(PC++);	//Grab relative code address
	//Todo: Implement bit addressing
}

//JC code addr								/* 1: 0100 0000 */
INLINE void jc(void)
{
	INT8 rel_addr = ROP_ARG(PC++);	//Grab relative code address
	if(GET_CY)						//Jump if Carry Flag Set
		PC = PC + rel_addr;
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
	//Todo: Implement bit addressing
}

//JNC code addr								/* 1: 0101 0000 */
INLINE void jnc(void)
{
	INT8 rel_addr = ROP_ARG(PC++);	//Grab relative code address
	if(!GET_CY)						//Jump if Carry Flag not set
		PC = PC + rel_addr;
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
	//Todo: Implement bit addressing
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
	IRAM_W(R_R(r),R_ACC);	//Store A to location pointed to by R0 or R1
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
	//Todo: Implement bit addressing
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
	UINT16 result = R_ACC * R_B;
	//A gets hi bits, B gets lo bits of result
	SFR_W(ACC,(UINT8)((result & 0xFF00) >> 8));
	SFR_W(B,(UINT8)(result & 0xFF));
	//Set flags
	SET_OV( ((result & 0x100) >> 8) );		//Set/Clear Overflow Flag if result > 256
	SET_CY(0);								//Carry Flag always cleared
}

//NOP										/* 1: 0000 0000 */
INLINE void nop(void)
{
}

//ORL data addr, A							/* 1: 0100 0010 */
INLINE void orl_mem_a(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	UINT8 data = IRAM_R(addr);		//Grab data from data address
	IRAM_W(addr,data | R_ACC);		//Set data address value to it's value Logical OR with ACC
}

//ORL data addr, #data						/* 1: 0100 0011 */
INLINE void orl_mem_byte(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	UINT8 data = ROP_ARG(PC++);		//Grab data
	UINT8 srcdata = IRAM_R(addr);	//Grab data from data address
	IRAM_W(addr,srcdata | data);	//Set data address value to it's value Logical OR with Data
}

//ORL A, #data								/* 1: 0100 0100 */
INLINE void orl_a_byte(void)
{
	UINT8 data = ROP_ARG(PC++);		//Grab data
	SFR_W(ACC,R_ACC | data);		//Set ACC to value of ACC Logical OR with Data
}

//ORL A, data addr							/* 1: 0100 0101 */
INLINE void orl_a_mem(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	UINT8 data = IRAM_R(addr);		//Grab data from data address
	SFR_W(ACC,R_ACC | data);		//Set ACC to value of ACC Logical OR with Data
}

//ORL A, @RO/@R1							/* 1: 0100 011i */
INLINE void orl_a_ir(int r)
{
	UINT8 data = IRAM_R(R_R(r));	//Grab data from address R0 or R1 points to
	SFR_W(ACC,R_ACC | data);		//Set ACC to value of ACC Logical OR with Data
}

//ORL A, RO to R7							/* 1: 0100 1rrr */
INLINE void orl_a_r(int r)
{
	UINT8 data = R_R(r);			//Grab data from R0 - R7
	SFR_W(ACC,R_ACC | data);		//Set ACC to value of ACC Logical OR with Data
}

//ORL C, bit addr							/* 1: 0111 0010 */
INLINE void orl_c_bitaddr(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab bit address
	//Todo: Implement bit addressing
}

//ORL C, /bit addr							/* 1: 1010 0000 */
INLINE void orl_c_nbitaddr(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab bit address
	//Todo: Implement bit addressing
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
	//Left Shift A, Bit 7 carries to Bit 0
	int carry = ((R_ACC & 0x80) >> 7);
	int data = (R_ACC<<1) & 0xfe;
	SFR_W(ACC, data | carry);
}

//RLC A										/* 1: 0011 0011 */
INLINE void rlc_a(void)
{
	//Left Shift A, Bit 7 goes to Carry Flag, Bit 0 of ACC goes to original Carry Flag
	int carry = ((R_ACC & 0x80) >> 7);
	int data = ((R_ACC<<1) & 0xfe) | GET_CY;
	SFR_W(ACC, data);
	SET_CY(carry);
}

//RR A										/* 1: 0000 0011 */
INLINE void rr_a(void)
{
	//Right Shift A, Bit 0 carries to Bit 7
	int carry = ((R_ACC & 1) << 7);
	int data = (R_ACC>>1) & 0x7f;
	SFR_W(ACC, data | carry);
}

//RRC A										/* 1: 0001 0011 */
INLINE void rrc_a(void)
{
	//Right Shift A, Bit 0 goes to Carry Flag, Bit 7 of ACC gets set to original Carry Flag
	int carry = (R_ACC & 1);
	int data = ((R_ACC>>1) & 0x7f) | (GET_CY<<7);
	SFR_W(ACC, data);
	SET_CY(carry);
}

//SETB C									/* 1: 1101 0011 */
INLINE void setb_c(void)
{
	SET_CY(1);		//Set Carry Flag
}

//SETB bit addr								/* 1: 1101 0010 */
INLINE void setb_bitaddr(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab bit address
	//Todo: Implement bit addressing
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
	UINT8 data = ROP_ARG(PC++);							//Grab data
	UINT16 tmpRes = (UINT16)(R_ACC - GET_CY - data);	//Subtract ACC from data (and include carry flag)
	SFR_W(ACC,(UINT8)tmpRes & 0xff);					//Store 8 bit result of subtraction in ACC
	//TODO: figure out & implement the flag settings (CY,AC,OV)

}

//SUBB A, data addr							/* 1: 1001 0101 */
INLINE void subb_a_mem(void)
{
	UINT8 addr = ROP_ARG(PC++);									//Grab data address
	UINT16 tmpRes = (UINT16)(R_ACC - GET_CY - IRAM_R(addr));	//Subtract ACC from data stored at data address (and include carry flag)
	SFR_W(ACC,(UINT8)tmpRes & 0xff);							//Store 8 bit result of addtion in ACC
	//TODO: figure out & implement the flag settings (CY,AC,OV)
}

//SUBB A, @R0/@R1							/* 1: 1001 011i */
INLINE void subb_a_ir(int r)
{
	UINT16 tmpRes = (UINT16)(R_ACC - GET_CY - IRAM_R(R_R(r)));	//Subtract value of memory pointed to by R0 or R1 from Acc (and include carry flag)
	SFR_W(ACC,(UINT8)tmpRes & 0xff);							//Store 8 bit result of addtion in ACC
	//TODO: figure out & implement the flag settings (CY,AC,OV)
}

//SUBB A, R0 to R7							/* 1: 1001 1rrr */
INLINE void subb_a_r(int r)
{
	UINT16 tmpRes = (UINT16)(R_ACC - GET_CY - R_R(r));	//Subtract value of R0-R7 from Acc (and include carry flag)
	SFR_W(ACC,(UINT8)tmpRes & 0xff);					//Store 8 bit result of addtion in ACC
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
	UINT8 data = IRAM_R(addr);		//Grab data
	UINT8 oldACC = R_ACC;			//Hold value of ACC
	SFR_W(ACC,data);				//Sets ACC to data
	IRAM_W(addr,oldACC);			//Sets data address to old value of ACC
}

//XCH A, @RO/@R1							/* 1: 1100 011i */
INLINE void xch_a_ir(int r)
{
	UINT8 data = IRAM_R(R_R(r));	//Grab data pointed to by R0 or R1
	UINT8 oldACC = R_ACC;			//Hold value of ACC
	SFR_W(ACC,data);				//Sets ACC to data
	IRAM_W(R_R(r),oldACC);			//Sets data address to old value of ACC
}

//XCH A, RO to R7							/* 1: 1100 1rrr */
INLINE void xch_a_r(int r)
{
	UINT8 data = R_R(r);			//Grab data from R0-R7
	UINT8 oldACC = R_ACC;			//Hold value of ACC
	SFR_W(ACC,data);				//Sets ACC to data
	R_R(r) = oldACC;				//Sets data address to old value of ACC
}

//XCHD A, @R0/@R1							/* 1: 1101 011i */
INLINE void xchd_a_ir(int r)
{
	//Todo: Implement
}

//XRL data addr, A							/* 1: 0110 0010 */
INLINE void xrl_mem_a(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	UINT8 data = IRAM_R(addr);		//Grab data from data address
	IRAM_W(addr,data ^ R_ACC);		//Set data address value to it's value Logical XOR with ACC
}

//XRL data addr, #data						/* 1: 0110 0011 */
INLINE void xrl_mem_byte(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	UINT8 data = ROP_ARG(PC++);		//Grab data
	UINT8 srcdata = IRAM_R(addr);	//Grab data from data address
	IRAM_W(addr,srcdata ^ data);	//Set data address value to it's value Logical XOR with Data
}

//XRL A, #data								/* 1: 0110 0100 */
INLINE void xrl_a_byte(void)
{
	UINT8 data = ROP_ARG(PC++);		//Grab data
	SFR_W(ACC,R_ACC ^ data);		//Set ACC to value of ACC Logical XOR with Data
}

//XRL A, data addr							/* 1: 0110 0101 */
INLINE void xrl_a_mem(void)
{
	UINT8 addr = ROP_ARG(PC++);		//Grab data address
	UINT8 data = IRAM_R(addr);		//Grab data from data address
	SFR_W(ACC,R_ACC ^ data);		//Set ACC to value of ACC Logical XOR with Data
}

//XRL A, @R0/@R1							/* 1: 0110 011i */
INLINE void xrl_a_ir(int r)
{
	UINT8 data = IRAM_R(R_R(r));	//Grab data from address R0 or R1 points to
	SFR_W(ACC,R_ACC ^ data);		//Set ACC to value of ACC Logical XOR with Data
}

//XRL A, R0 to R7							/* 1: 0110 1rrr */
INLINE void xrl_a_r(int r)
{
	UINT8 data = R_R(r);			//Grab data from R0 - R7
	SFR_W(ACC,R_ACC ^ data);		//Set ACC to value of ACC Logical XOR with Data
}

//illegal opcodes
INLINE void illegal(void)
{
	logerror("i8051 #%d: illegal opcode at 0x%03x: %02x\n", cpu_getactivecpu(), PC, ROP(PC));
}
