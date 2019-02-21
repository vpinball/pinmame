/*****************************************************************************
 *
 *	 arm7core.c
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

    **This core comes from my AT91 cpu core contributed to PinMAME, 
	  but with all the AT91 specific junk removed, 
	  which leaves just the ARM7TDMI core itself. I further removed the CPU specific MAME stuff
	  so you just have the actual ARM7 core itself, since many cpu's incorporate an ARM7 core, but add on
	  many cpu specific functionality.

	  Therefore, to use the core, you simpy include this file along with the .h file into your own cpu specific
	  implementation, and therefore, this file shouldn't be compiled as part of your project directly.
	  Additionally, you will need to include arm7exec.c in your cpu's execute routine.

	  For better or for worse, the code itself is very much intact from it's arm 2/3/6 origins from 
	  Bryan & Phil's work. I contemplated merging it in, but thought the fact that the CPSR is
	  no longer part of the PC was enough of a change to make it annoying to merge.
	**

	Coprocessor functions are heavily implementation specific, so callback handlers are used to allow the 
	implementation to handle the functionality. Custom DASM handlers are included as well to allow the DASM
	output to be tailored to the co-proc implementation details.

	Todo:
	Thumb mode support not yet implemented. 26 bit compatibility mode not implemented.
	Data Processing opcodes need cycle count adjustments (see page 194 of ARM7TDMI manual for instruction timing summary)
	Multi-emulated cpu support untested, but probably will not work too well, as no effort was made to code for more than 1.
	Could not find info on what the TEQP opcode is from page 44..
	I have no idea if user bank switching is right, as I don't fully understand it's use.
	Search for Todo: tags for remaining items not done.
	

	Differences from Arm 2/3 (6 also?)
	-Full 32 bit address support
	-PC no longer contains CPSR information, CPSR is own register now
	-New register SPSR to store previous contents of CPSR (this register is banked in many modes)
	-New opcodes for CPSR transfer, Long Multiplication, Co-Processor support, and some others
	-User Bank Mode transfer using certain flags which were previously unallowed (LDM/STM with S Bit & R15)
	-New operation modes? (unconfirmed)

	Based heavily on arm core from MAME 0.76:
    *****************************************
	ARM 2/3/6 Emulation

	Todo:
	Software interrupts unverified (nothing uses them so far, but they should be ok)
	Timing - Currently very approximated, nothing relies on proper timing so far.
	IRQ timing not yet correct (again, nothing is affected by this so far).

	By Bryan McPhail (bmcphail@tendril.co.uk) and Phil Stroffolino
*****************************************************************************/
#define ARM7_DEBUG_CORE 0

#if 0
#define LOG(x) printf x
#else
#define LOG(x) logerror x
#endif

/* Prototypes */

//SJE: should these be inline? or are they too big to see any benefit?

static void HandleCoProcDO(data32_t insn);
static void HandleCoProcRT(data32_t insn);
static void HandleCoProcDT(data32_t insn);
static void HandleHalfWordDT(data32_t insn);
static void HandleSwap(data32_t insn);
static void HandlePSRTransfer( data32_t insn );
static void HandleALU( data32_t insn);
static void HandleMul( data32_t insn);
static void HandleUMulLong( data32_t insn);
static void HandleSMulLong( data32_t insn);
//static void HandleBranch( data32_t insn, data8_t h_bit);
INLINE void HandleBranch( data32_t insn, data8_t h_bit);		//pretty short, so inline should be ok
static void HandleMemSingle( data32_t insn);
static void HandleMemBlock( data32_t insn);
INLINE data32_t decodeShift( data32_t insn, data32_t *pCarry);
INLINE void SwitchMode( int );
static void arm7_check_irq_state(void);

INLINE void arm7_cpu_write32( int addr, data32_t data );
INLINE void arm7_cpu_write16( int addr, data16_t data );
INLINE void arm7_cpu_write8( int addr, data8_t data );
INLINE data32_t arm7_cpu_read32( int addr );
INLINE data16_t arm7_cpu_read16( int addr );
INLINE data8_t arm7_cpu_read8( int addr );

/***************************************************************************
 * Default Memory Handlers 
 ***************************************************************************/
INLINE void arm7_cpu_write32( int addr, data32_t data )
{
	//Call normal 32 bit handler
	cpu_writemem32ledw_dword(addr,data);

	/* Unaligned writes are treated as normal writes */
	#if ARM7_DEBUG_CORE
		if(addr&3)
			LOG(("%08x: Unaligned write %08x\n",R15,addr));
	#endif
}


INLINE void arm7_cpu_write16( int addr, data16_t data )
{
	//Call normal 16 bit handler ( for 32 bit cpu )
	cpu_writemem32ledw_word(addr,data);
}

INLINE void arm7_cpu_write8( int addr, data8_t data )
{
	//Call normal 8 bit handler ( for 32 bit cpu )
	cpu_writemem32ledw(addr,data);
}

INLINE data32_t arm7_cpu_read32( int addr )
{
	data32_t result = 0;

	//Handle through normal 32 bit handler
	result = cpu_readmem32ledw_dword(addr);

	/* Unaligned reads rotate the word, they never combine words */
	if (addr&3) {
		#if ARM7_DEBUG_CORE
			if(addr&1)
				LOG(("%08x: Unaligned byte read %08x\n",R15,addr));
		#endif

		if ((addr&3)==3)
			return ((result&0x000000ff)<<24)|((result&0xffffff00)>> 8);
		if ((addr&3)==2)
			return ((result&0x0000ffff)<<16)|((result&0xffff0000)>>16);
		if ((addr&3)==1)
			return ((result&0x00ffffff)<< 8)|((result&0xff000000)>>24);
	}
	return result;
}

INLINE data16_t arm7_cpu_read16( int addr )
{
#if ARM7_DEBUG_CORE
	if (addr & 3)
	{
		int val = addr & 3;
		if(val != 2)
			LOG(("%08x: MISALIGNED half word read @ %08x:\n",R15,addr));
	}
#endif

	//Handle through normal 32 bit handler ( for 32 bit cpu )
	return cpu_readmem32ledw_word(addr);
}

INLINE data8_t arm7_cpu_read8( int addr )
{
	//Handle through normal 8 bit handler ( for 32 bit cpu )
	return cpu_readmem32ledw(addr);
}

/***************
 * helper funcs
 ***************/

/* Simplified (not using >> 31), as only used in the two Macros below, so sign bit is extracted there */
#define IsNeg(i) (i)
#define IsPos(i) ((i)^SIGN_BIT)
#define SIGN_BITS_DIFFER(a,b) ((a)^(b))
#define SIGN_BITS_DO_NOT_DIFFER(a,b) ((a)^(b)^SIGN_BIT)

/* Set NZCV flags for ADDS / SUBS */
#define HandleALUAddFlags(rd, rn, op2)                                                                           \
	if (insn & INSN_S)                                                                                           \
	SET_CPSR(((GET_CPSR & ~(N_MASK | Z_MASK | V_MASK | C_MASK))                                                  \
				| (((SIGN_BITS_DO_NOT_DIFFER(rn, op2) & SIGN_BITS_DIFFER(rn, rd)) & SIGN_BIT) == SIGN_BIT ? V_MASK : 0) \
				| ((((IsNeg(rn) & IsNeg(op2)) | (IsNeg(rn) & IsPos(rd)) | (IsNeg(op2) & IsPos(rd))) & SIGN_BIT) == SIGN_BIT ? C_MASK : 0) \
				| HandleALUNZFlags(rd)));                                                                        \
	R15 += 4;

#define HandleALUSubFlags(rd, rn, op2)                                                                           \
	if (insn & INSN_S)                                                                                           \
	SET_CPSR(((GET_CPSR & ~(N_MASK | Z_MASK | V_MASK | C_MASK))                                                  \
				| (((SIGN_BITS_DIFFER(rn, op2) & SIGN_BITS_DIFFER(rn, rd)) & SIGN_BIT) == SIGN_BIT ? V_MASK : 0) \
				| ((((IsNeg(rn) & IsPos(op2)) | (IsNeg(rn) & IsPos(rd)) | (IsPos(op2) & IsPos(rd))) & SIGN_BIT) == SIGN_BIT ? C_MASK : 0) \
				| HandleALUNZFlags(rd)));                                                                        \
	R15 += 4;

/* Set NZC flags for logical operations. */

//This macro (which I didn't write) - doesn't make it obvious that the SIGN BIT = 31, just as the N Bit does,
//therefore, N is set by default
#define HandleALUNZFlags(rd) \
  (((rd) & SIGN_BIT) | (((rd)==0) ? Z_MASK : 0))


//Long ALU Functions use bit 63 
#define HandleLongALUNZFlags(rd) \
  ((((rd) & ((UINT64)1<<63))>>32) | (((rd)==0) ? Z_MASK : 0))

#define HandleALULogicalFlags(rd, sc) \
  if (insn & INSN_S) \
    SET_CPSR( ((GET_CPSR &~ (N_MASK | Z_MASK | C_MASK)) \
                     | HandleALUNZFlags(rd) \
                     | (((sc) != 0) ? C_MASK : 0)));\
  R15 += 4;

//convert cpsr mode num into to text
static const char modetext[ARM7_NUM_MODES][5] = {
		 "USER","FIRQ","IRQ","SVC","ILL1","ILL2","ILL3","ABT",
		 "ILL4","ILL5","ILL6","UND","ILL7","ILL8","ILL9","SYS"
};
static const char* GetModeText( int cpsr )
{
	return modetext[cpsr & MODE_FLAG];
}

// Old register access macros: these accessed registers in the bank for the current
// mode.  For clarity, we've removed these so that all access is explicitly to the
// active set, a saved bank, or a mode-specific set.
//#define GetRegister(rIndex) ARMREG(sRegisterTable[GET_MODE][rIndex])
//#define SetRegister(rIndex,value) ARMREG(sRegisterTable[GET_MODE][rIndex]) = value

// Active register access: read/write a register in the active bank.  These registers
// are always in the same memory locations, shared among all modes.  When we switch
// modes, we copy the active registers to the outgoing mode's banked registers, and
// copy the incoming mode's banked registers to the active registers.
#define GetActiveRegister(rIndex) ARMREG(rIndex)
#define SetActiveRegister(rIndex,value) (ARMREG(rIndex) = value)

// Saved mode register access: read/write a register in the saved bank for the given
// mode.  These access the saved copies of registers for the various modes.
// Important!  These read/write the saved bank for the mode even if the mode is
// currently active.  Use Get/SetModeRegister to access the current or saved bank
// according to the active mode.
#define GetSavedRegister(mode, rIndex)  ARMREG(sRegisterTable[mode][rIndex])
#define SetSavedRegister(mode, rIndex, value) (ARMREG(sRegisterTable[mode][rIndex]) = value)

// Mode register access: read/write a register in the bank for the given mode.  If the
// given mode is the current mode, these read/write the active registers.  Otherwise,
// these read/write the saved bank for the given mode.
#define GetModeRegister(mode, rIndex)  ((mode) == GET_MODE ? GetActiveRegister(rIndex) : GetSavedRegister(mode, rIndex))
#define SetModeRegister(mode, rIndex, value)  ((mode) == GET_MODE ? SetActiveRegister(rIndex, value) : SetSavedRegister(mode, rIndex, value))

INLINE void SwitchMode (int cpsr_mode_val)
{
	static int old_mode = 0;
	
	// set the new mode
	data32_t cspr = GET_CPSR & ~MODE_FLAG;
	SET_CPSR(cspr | cpsr_mode_val);

	// swap banked registers if changing modes
	if (old_mode != cpsr_mode_val)
	{
		int i;
		
		// swap out the banked registers (R8-R14 and SPSR)
		SetSavedRegister(old_mode, SPSR, GetActiveRegister(SPSR));
		for (i = 8 ; i <= 14 ; ++i)
			SetSavedRegister(old_mode, i, GetActiveRegister(i));

		// swap in the banked registers
		SetActiveRegister(SPSR, GetSavedRegister(cpsr_mode_val, SPSR));
		for (i = 8 ; i <= 14 ; ++i)
			SetActiveRegister(i, GetSavedRegister(cpsr_mode_val, i));

		// remember the new mode
		old_mode = cpsr_mode_val;
	}
}


/* Decodes an Op2-style shifted-register form.  If @carry@ is non-zero the
 * shifter carry output will manifest itself as @*carry == 0@ for carry clear
 * and @*carry != 0@ for carry set.

   SJE: Rules: 
   IF RC = 256, Result = no shift.
   LSL   0   = Result = RM, Carry = Old Contents of CPSR C Bit
   LSL(0,31) = Result shifted, least significant bit is in carry out
   LSL  32   = Result of 0, Carry = Bit 0 of RM
   LSL >32   = Result of 0, Carry out 0
   LSR   0   = LSR 32 (see below)
   LSR  32   = Result of 0, Carry = Bit 31 of RM
   LSR >32   = Result of 0, Carry out 0
   ASR >=32  = ENTIRE Result = bit 31 of RM
   ROR  32   = Result = RM, Carry = Bit 31 of RM
   ROR >32   = Same result as ROR n-32 until amount in range of 1-32 then follow rules
*/

INLINE data32_t decodeShift( data32_t insn, data32_t *pCarry)
{
	data32_t k	= (insn & INSN_OP2_SHIFT) >> INSN_OP2_SHIFT_SHIFT;	//Bits 11-7
	data32_t rm	= GET_REGISTER( insn & INSN_OP2_RM );
	data32_t t	= (insn & INSN_OP2_SHIFT_TYPE) >> INSN_OP2_SHIFT_TYPE_SHIFT;

	if ((insn & INSN_OP2_RM)==0xf) {
		// "If a register is used to specify the shift amount the PC will be 12 bytes ahead." (instead of 8)
		rm += (t & 1) ? 12 : 8;
	}

	/* All shift types ending in 1 are Rk, not #k */
	if( t & 1 )
	{
//		LOG(("%08x:  RegShift %02x %02x\n",R15, k>>1,GET_REGISTER(k >> 1)));
		#if ARM7_DEBUG_CORE
			if((insn&0x80)==0x80)
				LOG(("%08x:  RegShift ERROR (p36)\n",R15));
		#endif

		//Keep only the bottom 8 bits for a Register Shift
		k = GET_REGISTER(k >> 1)&0xff;

		if( k == 0 ) /* Register shift by 0 is a no-op */
		{
//			LOG(("%08x:  NO-OP Regshift\n",R15));
			if (pCarry) *pCarry = GET_CPSR & C_MASK;
			return rm;
		}
	}
	/* Decode the shift type and perform the shift */
	switch (t >> 1)
	{
	case 0:						/* LSL */
		//LSL  32   = Result of 0, Carry = Bit 0 of RM
		//LSL >32   = Result of 0, Carry out 0
		if(k>=32) 
		{
			if(pCarry)	*pCarry = (k==32)?rm&1:0;
			return 0;
		}
		else
		{
			if (pCarry)
			{
			//LSL      0   = Result = RM, Carry = Old Contents of CPSR C Bit
			//LSL (0,31)   = Result shifted, least significant bit is in carry out
			*pCarry = k ? (rm & (1u << (32 - k))) : (GET_CPSR & C_MASK);
			}
			return k ? LSL(rm, k) : rm;
		}
		break;

	case 1:			       			/* LSR */
		if (k == 0 || k == 32)
		{
			if (pCarry) *pCarry = rm & SIGN_BIT;
			return 0;
		}
		else if (k > 32)
		{
			if (pCarry) *pCarry = 0;
			return 0;
		}
		else
		{
			if (pCarry) *pCarry = (rm & (1u << (k - 1)));
			return LSR(rm, k);
		}
		break;

	case 2:						/* ASR */
		if (k == 0 || k > 32)
			k = 32;

		if (pCarry) *pCarry = (rm & (1u << (k - 1)));
		if (k >= 32)
			return (rm & SIGN_BIT) ? 0xffffffffu : 0;
		else
		{
			if (rm & SIGN_BIT)
				return LSR(rm, k) | (0xffffffffu << (32 - k));
			else
				return LSR(rm, k);
		}
		break;

	case 3:						/* ROR and RRX */
		if (k)
		{
			k &= 31;
			if (k)
			{
				if (pCarry)
					*pCarry = rm & (1u << (k - 1));
				return ROR(rm, k);
			}
			else
			{
				if (pCarry)
					*pCarry = rm & SIGN_BIT;
				return rm;
			}
		}
		else
		{
			/* RRX */
			if (pCarry)
				*pCarry = (rm & 1);
			return LSR(rm, 1) | ((GET_CPSR & C_MASK) << 2);
		}
		break;
	}

#if ARM7_DEBUG_CORE
	LOG(("%08x: Decodeshift error\n", R15));
#endif
	return 0;
} /* decodeShift */


static int loadInc ( data32_t pat, data32_t rbv, data32_t s)
{
	int i, result;
	UINT32 data;

	result = 0;
	rbv &= ~3;
	for (i = 0; i < 16; i++)
	{
		if ((pat >> i) & 1)
		{
			if (ARM7.pendingAbtD == 0) {
				data = READ32(rbv += 4);
				if (i == 15) {
					//if (s) /* Pull full contents from stack */
						SET_REGISTER( 15, data );
					//else /* Pull only address, preserve mode & status flags */
					//	SET_REGISTER( 15, data );
				} else
					SET_REGISTER( i, data );
			}

			result++;
		}
	}
	return result;
}

static int loadIncMode(data32_t pat, data32_t rbv, data32_t s, int mode)
{
	int i, result;
	UINT32 data;

	result = 0;
	rbv &= ~3;
	for (i = 0; i < 16; i++)
	{
		if ((pat >> i) & 1)
		{
			if (ARM7.pendingAbtD == 0) // "Overwriting of registers stops when the abort happens."
			{
			data = READ32(rbv += 4);
			if (i == 15) {
				//if (s) /* Pull full contents from stack */
					SET_MODE_REGISTER(mode, 15, data);
				//else /* Pull only address, preserve mode & status flags */
				//	SET_MODE_REGISTER(mode, 15, data);
			} else
				SET_MODE_REGISTER(mode, i, data);
			}
			result++;
		}
	}
	return result;
}

static int loadDec( data32_t pat, data32_t rbv, data32_t s)
{
	int i, result;
	UINT32 data;

	result = 0;
	rbv &= ~3;
	for (i = 15; i >= 0; i--)
	{
		if ((pat >> i) & 1)
		{
			if (ARM7.pendingAbtD == 0) {
				data = READ32(rbv -= 4);
				if (i == 15) {
					//if (s) /* Pull full contents from stack */
						SET_REGISTER( 15, data );
					//else /* Pull only address, preserve mode & status flags */
					//	SET_REGISTER( 15, data );
				}
				else
					SET_REGISTER( i, data );
			}
			result++;
		}
	}
	return result;
}

static int loadDecMode(data32_t pat, data32_t rbv, data32_t s, int mode)
{
	int i, result;
	UINT32 data;

	result = 0;
	rbv &= ~3;
	for (i = 15; i >= 0; i--)
	{
		if ((pat >> i) & 1)
		{
			if (ARM7.pendingAbtD == 0) // "Overwriting of registers stops when the abort happens."
			{
			data = READ32(rbv -= 4);
			if (i == 15) {
				//if (s) /* Pull full contents from stack */
					SET_MODE_REGISTER(mode, 15, data);
				//else /* Pull only address, preserve mode & status flags */
				//	SET_MODE_REGISTER(mode, 15, data);
			}
			else
				SET_MODE_REGISTER(mode, i, data);
			}
			result++;
		}
	}
	return result;
}

static int storeInc( data32_t pat, data32_t rbv)
{
	int i,result;

	result = 0;
	for( i=0; i<16; i++ )
	{
		if( (pat>>i)&1 )
		{
			#if ARM7_DEBUG_CORE
				if(i==15) /* R15 is plus 12 from address of STM */
					LOG(("%08x: StoreInc on R15\n",R15));
			#endif
			WRITE32( rbv += 4, GET_REGISTER(i) );
			result++;
		}
	}
	return result;
} /* storeInc */

static int storeIncMode(data32_t pat, data32_t rbv, int mode)
{
	int i, result;

	result = 0;
	for (i = 0; i < 16; i++)
	{
		if ((pat >> i) & 1)
		{
#if ARM7_DEBUG_CORE
			if (i == 15) /* R15 is plus 12 from address of STM */
				LOG(("%08x: StoreInc on R15\n", R15));
#endif
			WRITE32(rbv += 4, GET_MODE_REGISTER(mode, i));
			result++;
		}
	}
	return result;
} /* storeInc */

// classic CV: 3005aa0 does the DMA thing
static int storeDec( data32_t pat, data32_t rbv)
{
        int i, result = 0, cnt;

	// pre-count the # of registers doing DMA
	for (i = 15; i >= 0; i--)
	{
		if ((pat >> i) & 1)
		{
			result++;

			// starting address
			rbv -= 4;
		}
	}

	cnt = 0;
	for (i = 0; i <= 15; i++)
	{
		if ((pat >> i) & 1)
		{
#if ARM7_DEBUG_CORE
			if (i == 15) /* R15 is plus 12 from address of STM */
				LOG(("%08x: StoreDec on R15\n", R15));
#endif
			WRITE32(rbv + (cnt * 4), GET_REGISTER(i));
			cnt++;
		}
	}
	return result;
} /* storeDec */

static int storeDecMode(data32_t pat, data32_t rbv, int mode)
{
	int i, result;

	result = 0;
	for (i = 15; i >= 0; i--)
	{
		if ((pat >> i) & 1)
		{
#if ARM7_DEBUG_CORE
			if (i == 15) /* R15 is plus 12 from address of STM */
				LOG(("%08x: StoreDec on R15\n", R15));
#endif
			WRITE32(rbv -= 4, GET_MODE_REGISTER(mode, i));
			result++;
		}
	}
	return result;
} /* storeDec */

/***************************************************************************
 *                            Main CPU Funcs
 ***************************************************************************/

//CPU INIT
static void arm7_core_init(const char *cpuname)
{
	int cpu = cpu_getactivecpu(),i;
	char buf[8];
	for (i=0; i<kNumRegisters; i++) {
		sprintf(buf,"R%d",i);
		state_save_register_UINT32(cpuname, cpu, buf, &ARMREG(i), 4);
	}
	state_save_register_UINT8(cpuname, cpu, "IRQ", &ARM7.pendingIrq, 1);
	state_save_register_UINT8(cpuname, cpu, "FIQ", &ARM7.pendingFiq, 1);
	state_save_register_UINT8(cpuname, cpu, "ABTD", &ARM7.pendingAbtD, 1);
	state_save_register_UINT8(cpuname, cpu, "ABTP", &ARM7.pendingAbtP, 1);
	state_save_register_UINT8(cpuname, cpu, "UND", &ARM7.pendingUnd, 1);
	state_save_register_UINT8(cpuname, cpu, "SWI", &ARM7.pendingSwi, 1);

	// create the JIT translator
	ARM7.jit = jit_create(&ARM7_ICOUNT);
	jit_set_mem_callbacks(
		ARM7.jit,
		PTR_READ8, PTR_READ16, PTR_READ32,
		PTR_WRITE8, PTR_WRITE16, PTR_WRITE32);
}

//CPU EXIT
static void arm7_core_exit(void)
{
	jit_delete(&ARM7.jit);
}

//CPU RESET
static void arm7_core_reset(void *param)
{
	/* save the JIT object, so that we don't lose it when clearing the registers */
	struct jit_ctl *jit = ARM7.jit;

	/* reset the machine registers */
	memset(&ARM7, 0, sizeof(ARM7));

	/* 
	 *   Reset the JIT and restore our context pointer to it.  We have to
	 *   reset the JIT because resetting the ARM emulation restores the boot
	 *   RAM, which invalidates any translated code. 
	 */
	jit_reset(jit);
	ARM7.jit = jit;

	/* start up in SVC mode with interrupts disabled. */
	SwitchMode(eARM7_MODE_SVC);
	SET_CPSR(GET_CPSR | I_MASK | F_MASK);
	R15 = 0;
    //change_pc(R15);
}

//Execute used to be here.. moved to separate file (arm7exec.c) to be included by cpu cores separately

//CPU CHECK IRQ STATE
//Note: couldn't find any exact cycle counts for most of these exceptions
static void arm7_check_irq_state(void)
{
	data32_t cpsr = GET_CPSR;	/* save current CPSR */
	data32_t pc = R15+4;		/* save old pc (already incremented in pipeline) */;

	/* Exception priorities:

		Reset
		Data abort
		FIRQ
		IRQ
		Prefetch abort
		Undefined instruction
		Software Interrupt
	*/

	//Data Abort
	if (ARM7.pendingAbtD) {
		SwitchMode(eARM7_MODE_ABT);				/* Set ABT mode so PC is saved to correct R14 bank */
		SET_REGISTER( 14, pc - 8 + 8);			/* save PC to R14 */
		SET_REGISTER( SPSR, cpsr );				/* Save current CPSR */
		SET_CPSR(GET_CPSR | I_MASK);            /* Mask IRQ */
		SET_CPSR(GET_CPSR & ~T_MASK);
		R15 = 0x10;								/* IRQ Vector address */
		ARM7.pendingAbtD = 0;
		return;
	}

	//FIRQ
	if (ARM7.pendingFiq && (cpsr & F_MASK)==0) {
		//ARM7.pendingFiq = 0;
		SwitchMode(eARM7_MODE_FIQ);				/* Set FIQ mode so PC is saved to correct R14 bank */
		SET_REGISTER( 14, pc - 4 + 4);			/* save PC to R14 */
		SET_REGISTER( SPSR, cpsr );				/* Save current CPSR */
		SET_CPSR(GET_CPSR | I_MASK | F_MASK);	/* Mask both IRQ & FIRQ*/
		SET_CPSR(GET_CPSR & ~T_MASK);
		R15 = 0x1c;								/* IRQ Vector address */
		return;
	}

	//IRQ
	if (ARM7.pendingIrq && (cpsr & I_MASK)==0) {
		//ARM7.pendingIrq = 0;
		SwitchMode(eARM7_MODE_IRQ);				/* Set IRQ mode so PC is saved to correct R14 bank */
		SET_REGISTER( 14, pc - 4 + 4);			/* save PC to R14 */
		SET_REGISTER( SPSR, cpsr );				/* Save current CPSR */
		SET_CPSR(GET_CPSR | I_MASK);			/* Mask IRQ */
		SET_CPSR(GET_CPSR & ~T_MASK);
		R15 = 0x18;								/* IRQ Vector address */
		return;
	}

	//Prefetch Abort
	if (ARM7.pendingAbtP) {
		SwitchMode(eARM7_MODE_ABT);				/* Set ABT mode so PC is saved to correct R14 bank */
		SET_REGISTER( 14, pc - 4 + 4);			/* save PC to R14 */
		SET_REGISTER( SPSR, cpsr );				/* Save current CPSR */
		SET_CPSR(GET_CPSR | I_MASK);            /* Mask IRQ */
		SET_CPSR(GET_CPSR & ~T_MASK);
		R15 = 0x0c;								/* IRQ Vector address */
		ARM7.pendingAbtP = 0;
		return;
	}

	//Undefined instruction
	if (ARM7.pendingUnd) {
		SwitchMode(eARM7_MODE_UND);				/* Set UND mode so PC is saved to correct R14 bank */
		//if (T_IS_SET(GET_CPSR))
		//{
			//SET_REGISTER( 14, pc - 4 + 2);		/* save PC to R14 */
		//}
		//else
		{
			SET_REGISTER( 14, pc - 4 + 4 - 4);	/* save PC to R14 */
		}
		SET_REGISTER( SPSR, cpsr );				/* Save current CPSR */
		SET_CPSR(GET_CPSR | I_MASK);            /* Mask IRQ */
		SET_CPSR(GET_CPSR & ~T_MASK);
		R15 = 0x04;								/* IRQ Vector address */
		ARM7.pendingUnd = 0;
		return;
	}

	//Software Interrupt
	if (ARM7.pendingSwi) {
		SwitchMode(eARM7_MODE_SVC);				/* Set SVC mode so PC is saved to correct R14 bank */
		//if (T_IS_SET(GET_CPSR))
		//{
			//SET_REGISTER( 14, pc - 4 + 2);		/* save PC to R14 */
		//}
		//else
		{
			SET_REGISTER( 14, pc - 4 + 4);       /* save PC to R14 */
		}	
		SET_REGISTER( SPSR, cpsr );				/* Save current CPSR */
		SET_CPSR(GET_CPSR | I_MASK);            /* Mask IRQ */
		SET_CPSR(GET_CPSR & ~T_MASK);           /* Go to ARM mode */
		R15 = 0x08;								/* IRQ Vector address */
		ARM7.pendingSwi = 0;
		return;
	}
}

//CPU - SET IRQ LINE
static void arm7_core_set_irq_line(int irqline, int state)
{
	switch (irqline) {

	case ARM7_IRQ_LINE: /* IRQ */
		ARM7.pendingIrq = (state & 1);
		break;

	case ARM7_FIRQ_LINE: /* FIRQ */
		ARM7.pendingFiq = (state & 1);
		break;

	case ARM7_ABORT_EXCEPTION:
		ARM7.pendingAbtD= (state & 1);
		break;
	case ARM7_ABORT_PREFETCH_EXCEPTION:
		ARM7.pendingAbtP= (state & 1);
		break;

	case ARM7_UNDEFINE_EXCEPTION:
		ARM7.pendingUnd= (state & 1);
		break;
	}

	ARM7_CHECKIRQ;
}

/***************************************************************************
 *                            OPCODE HANDLING
 ***************************************************************************/

// Co-Processor Data Operation
static void HandleCoProcDO(data32_t insn)
{
	// This instruction simply instructs the co-processor to do something, no data is returned to ARM7 core
	if(arm7_coproc_do_callback)
		arm7_coproc_do_callback(insn,0,0);		//simply pass entire opcode to callback - since data format is actually dependent on co-proc implementation
#if ARM7_DEBUG_CORE
	else
		LOG(("%08x: Co-Processor Data Operation executed, but no callback defined!\n",R15));
#endif
}

//Co-Processor Register Transfer - To/From Arm to Co-Proc
static void HandleCoProcRT(data32_t insn)
{
	
	/* xxxx 1110 oooL nnnn dddd cccc ppp1 mmmm */

	// Load (MRC) data from Co-Proc to ARM7 register
	if( insn&0x00100000 )		//Bit 20 = Load or Store
		{
			if(arm7_coproc_rt_r_callback)
			{
				data32_t res = arm7_coproc_rt_r_callback(insn,0);	//RT Read handler must parse opcode & return appropriate result
				SET_REGISTER((insn>>12)&0xf,res);
			}
#if ARM7_DEBUG_CORE
			else
				LOG(("%08x: Co-Processor Register Transfer executed, but no RT Read callback defined!\n",R15));
#endif
		}
	// Store (MCR) data from ARM7 to Co-Proc register
	else
		{
		if(arm7_coproc_rt_r_callback)
			arm7_coproc_rt_w_callback(insn,GET_REGISTER((insn>>12)&0xf),0);
#if ARM7_DEBUG_CORE
		else
			LOG(("%08x: Co-Processor Register Transfer executed, but no RT Write callback defined!\n",R15));
#endif
		}
}

/*Data Transfer - To/From Arm to Co-Proc
   Loading or Storing, the co-proc function is responsible to read/write from the base register supplied + offset
   8 bit immediate value Base Offset address is << 2 to get the actual #
 
  issues - #1 - the co-proc function, needs direct access to memory reads or writes (ie, so we must send a pointer to a func)
         - #2 - the co-proc may adjust the base address (especially if it reads more than 1 word), so a pointer to the register must be used
                but the old value of the register must be restored if write back is not set..
		 - #3 - when post incrementing is used, it's up to the co-proc func. to add the offset, since the transfer
		        address supplied in that case, is simply the base. I suppose this is irrelevant if write back not set
				but if co-proc reads multiple address, it must handle the offset adjustment itself.
*/
//todo: test with valid instructions
static void HandleCoProcDT(data32_t insn)
{
	data32_t rn = (insn>>16)&0xf;
	data32_t rnv = GET_REGISTER(rn);	// Get Address Value stored from Rn 
	data32_t ornv = rnv;				// Keep value of Rn 
	data32_t off = (insn&0xff)<<2;		// Offset is << 2 according to manual
	data32_t* prn = &ARMREG(rn);		// Pointer to our register, so it can be changed in the callback

	//Pointers to read32/write32 functions
	void (*write32)(int addr, data32_t data);
	data32_t (*read32)(int addr);
	write32 = PTR_WRITE32;
	read32 = PTR_READ32;

	#if ARM7_DEBUG_CORE
		if(((insn>>16)&0xf)==15 && (insn & 0x200000))
			LOG(("%08x: Illegal use of R15 as base for write back value!\n",R15));
	#endif

	//Pre-Increment base address (IF POST INCREMENT - CALL BACK FUNCTION MUST DO IT)
	if(insn&0x1000000 && off)
	{
		//Up - Down bit
		if(insn&0x800000)
			rnv+=off; //!! rnv never used again??!
		else
			rnv-=off;
	}

	// Load (LDC) data from ARM7 memory to Co-Proc memory
	if( insn&0x00100000 )
		{
			if(arm7_coproc_dt_r_callback)
				arm7_coproc_dt_r_callback(insn,prn,read32);
#if ARM7_DEBUG_CORE
			else
				LOG(("%08x: Co-Processer Data Transfer executed, but no READ callback defined!\n",R15));
#endif
		}
	// Store (STC) data from Co-Proc to ARM7 memory
	else
		{
			if(arm7_coproc_dt_w_callback)
				arm7_coproc_dt_w_callback(insn,prn,write32);
#if ARM7_DEBUG_CORE
			else
				LOG(("%08x: Co-Processer Data Transfer executed, but no WRITE callback defined!\n",R15));
#endif
		}
		
	if (ARM7.pendingUnd != 0) return;

	//If writeback not used - ensure the original value of RN is restored in case co-proc callback changed value
	if((insn & 0x200000)==0)
		SET_REGISTER(rn,ornv);
}

static void HandleBranch(  data32_t insn, data8_t h_bit )
{
	data32_t off = (insn & INSN_BRANCH) << 2;
	if (h_bit)
	{
		// H goes to bit1
		off |= (insn & 0x01000000) >> 23;
	}

	/* Save PC into LR if this is a branch with link */
	if (insn & INSN_BL)
	{
		SET_REGISTER(14,R15 + 4);
	}

	/* Sign-extend the 24-bit offset in our calculations */
	if (off & 0x2000000u)
	{
		R15 -= ((~(off | 0xfc000000u)) + 1) - 8;
	}
	else
	{
		R15 += off + 8;
	}
}

static void HandleMemSingle( data32_t insn )
{
	data32_t rn, rnv, off, rd, rnv_old = 0;

	/* Fetch the offset */
	if (insn & INSN_I)
	{
		/* Register Shift */
		off = decodeShift(insn, NULL);
	}
	else
	{
		/* Immediate Value */
		off = insn & INSN_SDT_IMM;
	}

	/* Calculate Rn, accounting for PC */
	rn = (insn & INSN_RN) >> INSN_RN_SHIFT;

	if (insn & INSN_SDT_P)
	{
		/* Pre-indexed addressing */
		if (insn & INSN_SDT_U)
		{
			rnv = (GET_REGISTER(rn) + off);
		}
		else
		{
			rnv = (GET_REGISTER(rn) - off);
		}

		if (insn & INSN_SDT_W)
		{
		    rnv_old = GET_REGISTER(rn);
			SET_REGISTER(rn,rnv);
			//check writeback???
		}
		else if (rn == eR15)
		{
			rnv = rnv + 8;
		}
	}
	else
	{
		/* Post-indexed addressing */
		if (rn == eR15)
		{
			rnv = R15 + 8;
		}
		else
		{
			rnv = GET_REGISTER(rn);
		}
	}

	// TODO: Post-Index + Write-Back (P=0 + W=1) mode is special: it means that
	// the memory access is non-privileged regardless of processor mode.  This
	// tells the external memory manager hardware to use a user-mode address
	// even if the processor is in a privileged mode.  This isn't currently
	// implemented, and no physical hardware that MAME currently emulates uses
	// this capability of the CPU.  This would only be needed to emulate a
	// system that has a hardware memory manager that distinguishes user-mode
	// and privileged memory accesses.

	/* Do the transfer */
	rd = (insn & INSN_RD) >> INSN_RD_SHIFT;
	if (insn & INSN_SDT_L)
	{
		/* Load */
		if (insn & INSN_SDT_B)
		{
			data32_t data = READ8(rnv);
			if (ARM7.pendingAbtD == 0)
			{
				SET_REGISTER(rd, data);
			}
		}
		else
		{
			data32_t data = READ32(rnv);
			if (ARM7.pendingAbtD == 0)
			{
				if (rd == eR15)
				{
					R15 = data - 4;
					//LDR, PC takes 2S + 2N + 1I (5 total cycles)
					ARM7_ICOUNT -= 2;
				}
				else
				{
					SET_REGISTER(rd,data);
				}
			}
		}
	}
	else
	{
		/* Store */
		if (insn & INSN_SDT_B)
		{
			#if ARM7_DEBUG_CORE
				if(rd==eR15)
					LOG(("Wrote R15 in byte mode\n"));
			#endif

			WRITE8(rnv, (data8_t) GET_REGISTER(rd) & 0xffu);
		}
		else
		{
			#if ARM7_DEBUG_CORE
				if(rd==eR15)
					LOG(("Wrote R15 in 32bit mode\n"));
			#endif

			//WRITE32(rnv, rd == eR15 ? R15 + 8 : GET_REGISTER(rd));
			WRITE32(rnv, rd == eR15 ? R15 + 8 + 4 : GET_REGISTER(rd)); //manual says STR rd = PC, +12
#if JIT_ENABLED
			// This is to handle code in self-modifying color patches.
			jit_untranslate(ARM7.jit, rnv);
#endif
		}
		//Store takes only 2 N Cycles, so add + 1
		ARM7_ICOUNT += 1;
	}

	// If ABORT is asserted, undo the index register write-back that we did earlier - the index
	// write-back doesn't happen in the hardware version if the memory access triggers an ABORT.
	// (The write-back we're undoing only happened if in pre-indexing mode (P flag set) and
	// write-back mode (W flag set).
	//
	// If ABORT isn't asserted, and we're in post-indexing mode, write back the register.  Note
	// that write-back occurs in post-indexing mode whether or not the W flag is set.  (W+P has
	// the special meaning of "non-privileged address mode", which signals the external memory
	// manager hardware that a user-mode address should be used when the processor is in a
	// privileged mode.)
	if (ARM7.pendingAbtD != 0)
	{
		// ABORT asserted - undo the pre-indexing write-back we did earlier, if we did it at all
		if ((insn & INSN_SDT_P) && (insn & INSN_SDT_W))
		{
			SET_REGISTER(rn, rnv_old);
		}
	}
	else if (!(insn & INSN_SDT_P))
	{
		// No ABORT, and post-indexing mode - write back the index register.  Note that write-back
		// is implied by post-index mode whether or not the W flag is set.
		if (insn & INSN_SDT_U)
		{
			/* Writeback is applied in pipeline, before value is read from mem,
				so writeback is effectively ignored */
			if (rd==rn) {
				SET_REGISTER(rn,GET_REGISTER(rd));
				//todo: check for offs... ?
			}
			else {

			#if ARM7_DEBUG_CORE
				if ((insn&INSN_SDT_W) != 0)
					LOG(("%08x:  RegisterWritebackIncrement %d %d %d\n",R15,(insn & INSN_SDT_P)!=0,(insn&INSN_SDT_W)!=0,(insn & INSN_SDT_U)!=0));
			#endif

				SET_REGISTER(rn,(rnv + off));
			}
		}
		else
		{
			/* Writeback is applied in pipeline, before value is read from mem,
				so writeback is effectively ignored */
			if (rd==rn) {
				SET_REGISTER(rn,GET_REGISTER(rd));
			}
			else {
				SET_REGISTER(rn,(rnv - off));

			#if ARM7_DEBUG_CORE
				if ((insn&INSN_SDT_W) != 0)
					LOG(("%08x:  RegisterWritebackDecrement %d %d %d\n",R15,(insn & INSN_SDT_P)!=0,(insn&INSN_SDT_W)!=0,(insn & INSN_SDT_U)!=0));
			#endif
			}
		}
	}
	// Can't do this here, R15 gets incremented after. 
	//ARM7_CHECKIRQ;

} /* HandleMemSingle */

static void HandleHalfWordDT(data32_t insn)
{
	data32_t rn, rnv, off, rd, rnv_old = 0;

	//Immediate or Register Offset?
	if(insn & 0x400000) {				//Bit 22 - 1 = immediate, 0 = register
		//imm. value in high nibble (bits 8-11) and lo nibble (bit 0-3)
		off = (((insn>>8)&0x0f)<<4) | (insn&0x0f);
	}
    else {
		//register
		off = GET_REGISTER(insn & 0x0f);
	}

	/* Calculate Rn, accounting for PC */
	rn = (insn & INSN_RN) >> INSN_RN_SHIFT;

	if (insn & INSN_SDT_P)
	{
		/* Pre-indexed addressing */
		if (insn & INSN_SDT_U)
		{
			rnv = (GET_REGISTER(rn) + off);
		}
		else
		{
			rnv = (GET_REGISTER(rn) - off);
		}

		if (insn & INSN_SDT_W)
		{
			rnv_old = GET_REGISTER(rn);
			SET_REGISTER(rn,rnv);

		//check writeback???
		}
		else if (rn == eR15)
		{
			rnv = (rnv) + 8;
		}
	}
	else
	{
		/* Post-indexed addressing */
		if (rn == eR15)
		{
			rnv = R15 + 8;
		}
		else
		{
			rnv = GET_REGISTER(rn);
		}
	}

	/* Do the transfer */
	rd = (insn & INSN_RD) >> INSN_RD_SHIFT;

	// TODO: Determine if post-index + write-back (P=0 + W=1) mode triggers the
	// same special user-mode memory access as in LDR/STR.  In LDR/STR, P=0 W=1
	// tells the external hardware memory manager to use user-mode addressing
	// even if the process is in privileged mode.  This isn't currently implemented
	// for ANY instructions, as we don't have any notion of user vs privileged
	// access modes for memory.  If we ever add such a feature, we *might* have
	// to include it here.  It's not clear from the ARM7 docs if this function
	// applies to the half-word instructions, but it seems possible because
	// they have the same special rule as LDR/STR that P=0 implies write-back
	// mode even if W=0.

	/* Load */
	if (insn & INSN_SDT_L)
	{
		//Signed?
		if(insn & 0x40)
		{
			data32_t newval = 0;

			//Signed Half Word?
			if(insn & 0x20) {
				data16_t signbyte,databyte;
				databyte = READ16(rnv) & 0xFFFF;
				signbyte = (databyte & 0x8000) ? 0xffff : 0;
				newval = (data32_t)(signbyte<<16)|databyte;
			}
			//Signed Byte
			else {
				data8_t databyte;
				data32_t signbyte;
				databyte = READ8(rnv) & 0xff;
				signbyte = (databyte & 0x80) ? 0xffffff : 0;
				newval = (data32_t)(signbyte<<8)|databyte;
			}

			if (ARM7.pendingAbtD == 0)
			{
			//PC?
			if(rd == eR15)
			{
				// MJR - this was newval+8, but I think that was a bug.  R15 is 8 bytes ahead
				// when used as the source register in a STORE (actually +12 with a halfword
				// store, so this was apprently doubly buggy), but this doesn't apply when it's
				// the destination of a LOAD.
				R15 = newval;
				
				//LDR(H,SH,SB) PC takes 2S + 2N + 1I (5 total cycles)
				ARM7_ICOUNT -= 2;

			}
			else
			{
				SET_REGISTER(rd,newval);
				R15 += 4;
			}
			
			}
			else
			{
				R15 += 4;
			}
		}
		//Unsigned Half Word
		else
		{
			data32_t newval = READ16(rnv);
			
			if (ARM7.pendingAbtD == 0)
			{
			if (rd == eR15)
			{
				// MJR - this was newval+8, but I think that was a bug.  R15 is 8 bytes ahead
				// when used as the source register in a STORE (actually +12 with a halfword
				// store, so this was apprently doubly buggy), but this doesn't apply when it's
				// the destination of a LOAD.
				R15 = newval;
				
				// extra cycles for LDR(H,SH,SB) PC (5 total cycles)
				ARM7_ICOUNT -= 2;
			}
			else
			{
				SET_REGISTER(rd,newval);
				R15 += 4;
			}
			
			}
			else
			{
				R15 += 4;
			}
		}
	}
	/* Store or ARMv5+ dword insns */
	else
	{
		if ((insn & 0x60) == 0x40)  // LDRD
		{
			SET_REGISTER(rd, READ32(rnv));
			SET_REGISTER(rd+1, READ32(rnv+4));
			R15 += 4;
		}
		else if ((insn & 0x60) == 0x60) // STRD
		{
			WRITE32(rnv, GET_REGISTER(rd));
			WRITE32(rnv+4, GET_REGISTER(rd+1));
			R15 += 4;
		}
		/* Store */
		else
		{
			//WRITE16(rnv, rd == eR15 ? R15 + 8 : GET_REGISTER(rd));
			WRITE16(rnv, rd == eR15 ? R15 + 8 + 4 : GET_REGISTER(rd)); //manual says STR RD=PC, +12 of address
			
			// if R15 is not increased then e.g. "STRH R10, [R15,#$10]" will be executed over and over again
#if 0
			if(rn != eR15)
#endif
				R15 += 4;
			//STRH takes 2 cycles, so we add + 1
			ARM7_ICOUNT += 1;
		}
	}

	// If the ABORT flag is set, UNDO any previous write-back we did to the index register.
	//
	// If the ABORT flag isn't set, and we're in post-indexing mode, do the write-back to
	// the index register.  NB: Write-back is implied by post-indexing mode, regardless
	// of the W bit setting.
	if (ARM7.pendingAbtD != 0)
	{
		if ((insn & INSN_SDT_P) && (insn & INSN_SDT_W))
		{
			SET_REGISTER(rn, rnv_old);
		}
	}
	else if (!(insn & INSN_SDT_P))
	{
		// Post-indexing mode and no ABORT - write back the updated index register.
		// This always happens in post-index mode regardless of the W flag.
		if (insn & INSN_SDT_U)
		{
			/* Writeback is applied in pipeline, before value is read from mem,
				so writeback is effectively ignored */
			if (rd==rn) {
				SET_REGISTER(rn,GET_REGISTER(rd));
				//todo: check for offs... ?
			}
			else {

			#if ARM7_DEBUG_CORE
				if ((insn&INSN_SDT_W) != 0)
					LOG(("%08x:  RegisterWritebackIncrement %d %d %d\n",R15,(insn & INSN_SDT_P)!=0,(insn&INSN_SDT_W)!=0,(insn & INSN_SDT_U)!=0));
			#endif

				SET_REGISTER(rn,(rnv + off));
			}
		}
		else
		{
			/* Writeback is applied in pipeline, before value is read from mem,
				so writeback is effectively ignored */
			if (rd==rn) {
				SET_REGISTER(rn,GET_REGISTER(rd));
			}
			else {
				SET_REGISTER(rn,(rnv - off));

			#if ARM7_DEBUG_CORE
				if ((insn&INSN_SDT_W) != 0)
					LOG(("%08x:  RegisterWritebackDecrement %d %d %d\n",R15,(insn & INSN_SDT_P)!=0,(insn&INSN_SDT_W)!=0,(insn & INSN_SDT_U)!=0));
			#endif
			}
		}
	}
}

static void HandleSwap(data32_t insn)
{
	//According to manual - swap is an LDR followed by an STR and all endian rules apply
	//Process: Read original data from address pointed by Rn then store data from address
	//         pointed by Rm to Rn address, and store original data from Rn to Rd.
	// Todo: I have no valid source to verify this function works.. so it needs to be tested
	data32_t rn,rm,rd,tmp;

#if ARM7_DEBUG_CORE
	LOG(("%08x: HandleSwap called!\n", R15));
#endif

	rn = GET_REGISTER((insn>>16)&0xf);
	rm = GET_REGISTER(insn&0xf);
	rd = (insn >> 12) & 0xf;//GET_REGISTER((insn>>12)&0xf);

	#if ARM7_DEBUG_CORE
	if(rn == 15 || rm == 15 || rd == 15)
		LOG(("%08x: Illegal use of R15 in Swap Instruction\n",R15));
	#endif

	//Byte Swap?
	if(insn & 0x400000)
	{
		tmp = READ8(rn);
		WRITE8(rn, rm);
		SET_REGISTER(rd, tmp);
	}
	else
	{
		tmp = READ32(rn);
		WRITE32(rn, rm);
		SET_REGISTER(rd, tmp);
	}

	R15 += 4;
	//Instruction takes 1S+2N+1I cycles - so we subtract one more..
	ARM7_ICOUNT -=1;
}

// MSR: store val in CPSR or SPSR, masking bits according to privileges for current CPU mode.
// 'fields' is the bit mask from bits 16-19 of the instruction, specifying which bits of
// the register to set.  It's okay to just pass the whole instruction dword for this.
static void HandleMSR(int spsr, data32_t val, data32_t fields)
{
	int oldmode = GET_CPSR & MODE_FLAG;
	int reg = (spsr && oldmode != eARM7_MODE_USER) ? SPSR : eCPSR;        //Either CPSR or SPSR
	data32_t newval;

	// get current value of CPSR/SPSR - we'll use this as the basis for
	// any bits not affected by the MSR
	newval = GET_REGISTER(reg);
	
	// apply field code bits
	if (reg == eCPSR)
	{
		if (oldmode != eARM7_MODE_USER)
		{
			if (fields & 0x00010000)
			{
				newval = (newval & 0xffffff00) | (val & 0x000000ff);
			}
			if (fields & 0x00020000)
			{
				newval = (newval & 0xffff00ff) | (val & 0x0000ff00);
			}
			if (fields & 0x00040000)
			{
				newval = (newval & 0xff00ffff) | (val & 0x00ff0000);
			}
		}
		// status flags can be modified regardless of mode
		if (fields & 0x00080000)
		{
			// TODO for non ARMv5E mask should be 0xf0000000 (ie mask Q bit)
			newval = (newval & 0x00ffffff) | (val & 0xf8000000);
		}
	}
	else    // SPSR has stricter requirements
	{
		if (((GET_CPSR & 0x1f) > 0x10) && ((GET_CPSR & 0x1f) < 0x1f))
		{
			if (fields & 0x00010000)
			{
				newval = (newval & 0xffffff00) | (val & 0xff);
			}
			if (fields & 0x00020000)
			{
				newval = (newval & 0xffff00ff) | (val & 0xff00);
			}
			if (fields & 0x00040000)
			{
				newval = (newval & 0xff00ffff) | (val & 0xff0000);
			}
			if (fields & 0x00080000)
			{
				// TODO for non ARMv5E mask should be 0xf0000000 (ie mask Q bit)
				newval = (newval & 0x00ffffff) | (val & 0xf8000000);
			}
		}
	}

#if 0
	// force valid mode
	newval |= 0x10;
#endif
	// Update the Register
	if (reg == eCPSR)
		SET_CPSR(newval);
	else
		SET_REGISTER(reg, newval);
	
	// Switch to new mode if changed
	if ((newval & MODE_FLAG) != oldmode)
		SwitchMode(GET_MODE);
}

// MRS: store current CPSR or SPSR value in register Rd
static void HandleMRS(int rd, int spsr)
{
	int reg = (spsr && GET_MODE != eARM7_MODE_USER) ? SPSR : eCPSR;        //Either CPSR or SPSR
	SET_REGISTER(rd, GET_REGISTER(reg));
}

static void HandlePSRTransfer( data32_t insn )
{
	int spsr = (insn & 0x400000);

	//MSR ( bit 21 set ) - Copy value to CPSR/SPSR
	if( (insn & 0x00200000) )
	{
		data32_t val = 0;

		//Immediate Value?
		if(insn & INSN_I) {
			//Value can be specified for a Right Rotate, 2x the value specified.
			int by = (insn & INSN_OP2_ROTATE) >> INSN_OP2_ROTATE_SHIFT;
			if (by)
				val = ROR(insn & INSN_OP2_IMM, by << 1);
			else
				val = insn & INSN_OP2_IMM;
		}
		//Value from Register
		else {
			val = GET_REGISTER(insn & 0x0f);
		}
		
		// apply the update
		HandleMSR(spsr, val, insn);
	}
	//MRS ( bit 21 clear ) - Copy CPSR or SPSR to specified Register
	else
	{
		HandleMRS((insn>>12)& 0x0f, spsr);
	}
}

static void HandleALU( data32_t insn )
{
	data32_t op2, sc=0, rd, rn, opcode;
	data32_t rdn;

	// Normal Data Processing : 1S
	// Data Processing with register specified shift : 1S + 1I
	// Data Processing with PC written : 2S + 1N
	// Data Processing with register specified shift and PC written : 2S + 1N + 1I

	opcode = (insn & INSN_OPCODE) >> INSN_OPCODE_SHIFT;

	rd = 0;
	rn = 0;

	/* --------------*/
	/* Construct Op2 */
	/* --------------*/

	/* Immediate constant */
	if (insn & INSN_I)
	{
		data32_t by = (insn & INSN_OP2_ROTATE) >> INSN_OP2_ROTATE_SHIFT;
		if (by)
		{
			op2 = ROR(insn & INSN_OP2_IMM, by << 1);
			sc = op2 & SIGN_BIT;
		}
		else
		{
			op2 = insn & INSN_OP2_IMM;
			sc = GET_CPSR & C_MASK;
		}
	}
	/* Op2 = Register Value */
	else
	{
		op2 = decodeShift(insn, (insn & INSN_S) ? &sc : NULL);

		// LD TODO sc will always be 0 if this applies
		if (!(insn & INSN_S))
			sc = 0;

		// extra cycle (register specified shift)
		ARM7_ICOUNT -= 1;
	}

	// LD TODO this comment is wrong
	/* Calculate Rn to account for pipelining */
	if ((opcode & 0xd) != 0xd) /* No Rn in MOV */
	{
		if ((rn = (insn & INSN_RN) >> INSN_RN_SHIFT) == eR15)
		{
			int addpc = ((insn&INSN_I)?8:(insn&0x10u)?12:8);
			#if ARM7_DEBUG_CORE
				LOG(("%08x:  Pipelined R15 (Shift %d)\n",R15,addpc));
			#endif
			//rn=R15+8;
			rn=R15+addpc;
		}
		else
		{
			rn = GET_REGISTER(rn);
		}
	}

	/* Perform the operation */
			
	switch (opcode)
	{
	/* Arithmetic operations */
	case OPCODE_SBC:
		rd = (rn - op2 - ((GET_CPSR & C_MASK) ? 0 : 1));
		HandleALUSubFlags(rd, rn, op2);
		break;
	case OPCODE_CMP:
	case OPCODE_SUB:
		rd = (rn - op2);
		HandleALUSubFlags(rd, rn, op2);
		break;
	case OPCODE_RSC:
		rd = (op2 - rn - ((GET_CPSR & C_MASK) ? 0 : 1));
		HandleALUSubFlags(rd, op2, rn);
		break;
	case OPCODE_RSB:
		rd = (op2 - rn);
		HandleALUSubFlags(rd, op2, rn);
		break;
	case OPCODE_ADC:
		rd = (rn + op2 + ((GET_CPSR & C_MASK) >> C_BIT));
		HandleALUAddFlags(rd, rn, op2);
		break;
	case OPCODE_CMN:
	case OPCODE_ADD:
		rd = (rn + op2);
		HandleALUAddFlags(rd, rn, op2);
		break;

	/* Logical operations */
	case OPCODE_AND:
	case OPCODE_TST:
		rd = rn & op2;
		HandleALULogicalFlags(rd, sc);
		break;
	case OPCODE_BIC:
		rd = rn & ~op2;
		HandleALULogicalFlags(rd, sc);
		break;
	case OPCODE_TEQ:
	case OPCODE_EOR:
		rd = rn ^ op2;
		HandleALULogicalFlags(rd, sc);
		break;
	case OPCODE_ORR:
		rd = rn | op2;
		HandleALULogicalFlags(rd, sc);
		break;
	case OPCODE_MOV:
		rd = op2;
		HandleALULogicalFlags(rd, sc);
		break;
	case OPCODE_MVN:
		rd = (~op2);
		HandleALULogicalFlags(rd, sc);
		break;
	}

	/* Put the result in its register if not one of the test only opcodes (TST,TEQ,CMP,CMN) */
	rdn = (insn & INSN_RD) >> INSN_RD_SHIFT;
	if ((opcode & 0xc) != 0x8)
	{
		//If Rd = R15, but S Flag not set, Result is placed in R15, but CPSR is not affected (page 44)
		if (rdn == eR15 && !(insn & INSN_S))
		{
			R15 = rd;

			// extra cycles (PC written)
			ARM7_ICOUNT -= 2;
		}
		else
		{
			//Rd = 15 and S Flag IS set, Result is placed in R15, and current mode SPSR moved to CPSR
			if (rdn==eR15) {

				// When Rd is R15 and the S flag is set the result of the operation is placed in R15 and the SPSR corresponding to
				// the current mode is moved to the CPSR. This allows state changes which automatically restore both PC and
				// CPSR. --> This form of instruction should not be used in User mode. <--

				if (GET_MODE != eARM7_MODE_USER)
				{
					// Update CPSR from SPSR
					SET_CPSR(GET_REGISTER(SPSR));
					SwitchMode(GET_MODE);
				}

				R15 = rd;
				
				// extra cycles (PC written)
				ARM7_ICOUNT -= 2;

				/* IRQ masks may have changed in this instruction */
				ARM7_CHECKIRQ;
			}
			else
				/* S Flag is set - Write results to register & update CPSR (which was already handled using HandleALU flag macros) */
				SET_REGISTER(rdn,rd);
		}
	} 
	//SJE: Don't think this applies any more.. (see page 44 at bottom)
	/* TST & TEQ can affect R15 (the condition code register) with the S bit set */
	else if (rdn==eR15)
	{
		if (insn & INSN_S) {
			#if ARM7_DEBUG_CORE
				LOG(("%08x: TST class on R15 s bit set\n",R15));
			#endif
			R15 = rd;

			/* IRQ masks may have changed in this instruction */
			ARM7_CHECKIRQ;
		}
		else
		{
			#if	ARM7_DEBUG_CORE
				LOG(("%08x: TST class on R15 no s bit set\n",R15));
			#endif
		}
		
		// extra cycles (PC written)
		ARM7_ICOUNT -= 2;
	}
	
	// compensate for the -3 at the end
	ARM7_ICOUNT += 2;
}

static void HandleMul( data32_t insn)
{
	UINT32 r, rm, rs;

	// MUL takes 1S + mI and MLA 1S + (m+1)I cycles to execute, where S and I are as
	// defined in 6.2 Cycle Types on page 6-2.
	// m is the number of 8 bit multiplier array cycles required to complete the
	// multiply, which is controlled by the value of the multiplier operand
	// specified by Rs.

	rm = GET_REGISTER(insn & INSN_MUL_RM);
	rs = GET_REGISTER((insn & INSN_MUL_RS) >> INSN_MUL_RS_SHIFT);

	/* Do the basic multiply of Rm and Rs */
	r = rm * rs;

	#if ARM7_DEBUG_CORE
	if( 
	    ((insn&INSN_MUL_RM)==0xf) || 
		(((insn&INSN_MUL_RS)>>INSN_MUL_RS_SHIFT )==0xf) ||
		(((insn&INSN_MUL_RN)>>INSN_MUL_RN_SHIFT)==0xf)
	   )
		LOG(("%08x:  R15 used in mult\n",R15));
	#endif

	/* Add on Rn if this is a MLA */
	if (insn & INSN_MUL_A)
	{
		r += GET_REGISTER((insn & INSN_MUL_RN) >> INSN_MUL_RN_SHIFT);
		// extra cycle for MLA
		ARM7_ICOUNT -= 1;
	}

	/* Write the result */
	SET_REGISTER((insn&INSN_MUL_RD)>>INSN_MUL_RD_SHIFT,r);

	/* Set N and Z if asked */
	if( insn & INSN_S )
	{
		SET_CPSR ( (GET_CPSR &~ (N_MASK | Z_MASK)) | HandleALUNZFlags(r));
	}

	if (rs & SIGN_BIT) rs = -rs;
	if (rs < 0x00000100) ARM7_ICOUNT -= 1 + 1;
	else if (rs < 0x00010000) ARM7_ICOUNT -= 1 + 2;
	else if (rs < 0x01000000) ARM7_ICOUNT -= 1 + 3;
	else ARM7_ICOUNT -= 1 + 4;

	ARM7_ICOUNT += 3;
}

//todo: add proper cycle counts
static void HandleSMulLong( data32_t insn)
{
	INT32 rm, rs;
	data32_t rhi,rlo;
	INT64 res=0;

	// MULL takes 1S + (m+1)I and MLAL 1S + (m+2)I cycles to execute, where m is the
	// number of 8 bit multiplier array cycles required to complete the multiply, which is
	// controlled by the value of the multiplier operand specified by Rs.

	rm  = (INT32)GET_REGISTER(insn&0xf);
	rs  = (INT32)GET_REGISTER(((insn>>8)&0xf));
	rhi = (insn>>16)&0xf;
	rlo = (insn>>12)&0xf;

	#if ARM7_DEBUG_CORE
		if( ((insn&0xf) == 15) || (((insn>>8)&0xf) == 15) || (((insn>>16)&0xf) == 15) || (((insn>>12)&0xf) == 15) )
			LOG(("%08x: Illegal use of PC as a register in SMULL opcode\n",R15));
	#endif

	/* Perform the multiplication */
	res = (INT64)rm * rs;

	/* Add on Rn if this is a MLA */
	if (insn & INSN_MUL_A)
	{
		INT64 acum = (INT64)((((INT64)(GET_REGISTER(rhi)))<<32) | GET_REGISTER(rlo));
		res += acum;
		// extra cycle for MLA
		ARM7_ICOUNT -= 1;
	}

	/* Write the result (upper dword goes to RHi, lower to RLo) */
	SET_REGISTER(rhi, res>>32);
	SET_REGISTER(rlo, res & 0xFFFFFFFF);
	
	/* Set N and Z if asked */
	if( insn & INSN_S )
	{
		SET_CPSR ( (GET_CPSR &~ (N_MASK | Z_MASK)) | HandleLongALUNZFlags(res));
	}

	if (rs < 0) rs = -rs;
	if (rs < 0x00000100) ARM7_ICOUNT -= 1 + 1 + 1;
	else if (rs < 0x00010000) ARM7_ICOUNT -= 1 + 2 + 1;
	else if (rs < 0x01000000) ARM7_ICOUNT -= 1 + 3 + 1;
	else ARM7_ICOUNT -= 1 + 4 + 1;

	ARM7_ICOUNT += 3;
}

//todo: add proper cycle counts
static void HandleUMulLong( data32_t insn)
{
	UINT32 rm, rs;
	data32_t rhi,rlo;
	UINT64 res=0;

	// MULL takes 1S + (m+1)I and MLAL 1S + (m+2)I cycles to execute, where m is the
	// number of 8 bit multiplier array cycles required to complete the multiply, which is
	// controlled by the value of the multiplier operand specified by Rs.

	rm  = (UINT32)GET_REGISTER(insn&0xf);
	rs  = (UINT32)GET_REGISTER(((insn>>8)&0xf));
	rhi = (insn>>16)&0xf;
	rlo = (insn>>12)&0xf;

	#if ARM7_DEBUG_CORE
		if( ((insn&0xf) == 15) || (((insn>>8)&0xf) == 15) || (((insn>>16)&0xf) == 15) || (((insn>>12)&0xf) == 15) )
			LOG(("%08x: Illegal use of PC as a register in SMULL opcode\n",R15));
	#endif

	/* Perform the multiplication */
	res = (UINT64)rm * rs;

	/* Add on Rn if this is a MLA */
	if (insn & INSN_MUL_A)
	{
		UINT64 acum = (UINT64)((((UINT64)(GET_REGISTER(rhi)))<<32) | GET_REGISTER(rlo));
		res += acum;
		// extra cycle for MLA
		ARM7_ICOUNT -= 1;
	}

	/* Write the result (upper dword goes to RHi, lower to RLo) */
	SET_REGISTER(rhi, res>>32);
	SET_REGISTER(rlo, res & 0xFFFFFFFF);
	
	/* Set N and Z if asked */
	if( insn & INSN_S )
	{
		SET_CPSR ( (GET_CPSR &~ (N_MASK | Z_MASK)) | HandleLongALUNZFlags(res));
	}

	if (rs < 0x00000100) ARM7_ICOUNT -= 1 + 1 + 1;
	else if (rs < 0x00010000) ARM7_ICOUNT -= 1 + 2 + 1;
	else if (rs < 0x01000000) ARM7_ICOUNT -= 1 + 3 + 1;
	else ARM7_ICOUNT -= 1 + 4 + 1;

	ARM7_ICOUNT += 3;
}

// LDMS Mode Change.  An LDM instruction with the S bit set and R15 in the
// transfer list triggers a mode change by loading SPSR_<mode> into CPSR at
// the same time that R15 is loaded.  This handles the operation.  Note
// that we ignore this if in user mode.  This isn't allowed by privilege
// rules, and it's non-sensical in that no SPSR exists in user mode.
static void HandleLDMS_ModeChange(void)
{
	if (GET_MODE != eARM7_MODE_USER)
	{
		SET_CPSR(GET_REGISTER(SPSR));
		SwitchMode(GET_MODE);
	}
}

static void HandleMemBlock( data32_t insn)
{
	data32_t rb = (insn & INSN_RN) >> INSN_RN_SHIFT;
	data32_t rbp = GET_REGISTER(rb);
	int result;

#if ARM7_DEBUG_CORE
	if(rbp & 3)
		LOG(("%08x: Unaligned Mem Transfer @ %08x\n",R15,rbp));
#endif

	// Normal LDM instructions take nS + 1N + 1I and LDM PC takes (n+1)S + 2N + 1I
	// incremental cycles, where S,N and I are as defined in 6.2 Cycle Types on page 6-2.
	// STM instructions take (n-1)S + 2N incremental cycles to execute, where n is the
	// number of words transferred.

	if (insn & INSN_BDT_L)
	{
		/* Loading */
		if (insn & INSN_BDT_U)
		{
			/* Incrementing */
			if (!(insn & INSN_BDT_P))
			{
				rbp = rbp + (- 4);
			}

			//S Flag Set, but R15 not in list = User Bank Transfer
			if(insn & INSN_BDT_S && ((insn & 0x8000)==0))
			{
				// !! actually switching to user mode triggers a section permission fault in Happy Fish 302-in-1 (BP C0030DF4, press F5 ~16 times) !!
				// set to user mode - then do the transfer, and set back
				//int curmode = GET_MODE;
				//SwitchMode(eARM7_MODE_USER);
			#if ARM7_DEBUG_CORE
				LOG(("%08x: User Bank Transfer not fully tested - please check if working properly!\n", R15));
			#endif
				result = loadIncMode(insn & 0xffff, rbp, insn & INSN_BDT_S, eARM7_MODE_USER);
				//todo - not sure if Writeback occurs on User registers also.. 
				//SwitchMode(curmode);
			}
			else
				result = loadIncMode(insn & 0xffff, rbp, insn & INSN_BDT_S, GET_MODE);

			if ((insn & INSN_BDT_W) && (ARM7.pendingAbtD == 0))
			{
				#if ARM7_DEBUG_CORE
					if(rb==15)
						LOG(("%08x:  Illegal LDRM writeback to r15\n",R15));
				#endif
				// "A LDM will always overwrite the updated base if the base is in the list." (also for a user bank transfer?)
				// GBA "V-Rally 3" expects R0 not to be overwritten with the updated base value [BP 8077B0C]
				if (((insn >> rb) & 1) == 0)
				{
					SET_REGISTER(rb,GET_REGISTER(rb)+result*4);
				}
			}

			//R15 included? (NOTE: CPSR restore must occur LAST otherwise wrong registers restored!)
			if ((insn & 0x8000) && (ARM7.pendingAbtD == 0)) {
				R15-=4;		//SJE: Remove 4, since we're adding +4 after this code executes
				// S - Flag Set? Signals transfer of current mode SPSR->CPSR
				if((insn & INSN_BDT_S) && GET_MODE != eARM7_MODE_USER)
					HandleLDMS_ModeChange();

				//LDM PC - takes 2 extra cycles
				ARM7_ICOUNT -=2;
			}			
		}
		else
		{
			/* Decrementing */
			if (!(insn & INSN_BDT_P))
			{
				rbp = rbp - (- 4);
			}

			//S Flag Set, but R15 not in list = User Bank Transfer
			if(insn & INSN_BDT_S && ((insn & 0x8000)==0))
			{
				//set to user mode - then do the transfer, and set back
				//int curmode = GET_MODE;
				//SwitchMode(eARM7_MODE_USER);
			#if ARM7_DEBUG_CORE
				LOG(("%08x: User Bank Transfer not fully tested - please check if working properly!\n",R15));
			#endif
				result = loadDecMode(insn & 0xffff, rbp, insn & INSN_BDT_S, eARM7_MODE_USER);
				//todo - not sure if Writeback occurs on User registers also.. 
				//SwitchMode(curmode);
			}
			else
				result = loadDecMode(insn & 0xffff, rbp, insn & INSN_BDT_S, GET_MODE);

			if ((insn & INSN_BDT_W) && (ARM7.pendingAbtD == 0))
			{
			#if ARM7_DEBUG_CORE
				if (rb == 0xf)
					LOG(("%08x:  Illegal LDRM writeback to r15\n",R15));
			#endif
				// "A LDM will always overwrite the updated base if the base is in the list." (also for a user bank transfer?)
				if (((insn >> rb) & 1) == 0)
				{
					SET_REGISTER(rb,GET_REGISTER(rb)-result*4);
				}
			}
			
			//R15 included? (NOTE: CPSR restore must occur LAST otherwise wrong registers restored!)
			if ((insn & 0x8000) && (ARM7.pendingAbtD == 0)) {
				R15-=4;		//SJE: Remove 4, since we're adding +4 after this code executes
				//S - Flag Set? Signals transfer of current mode SPSR->CPSR
				if(insn & INSN_BDT_S)
					HandleLDMS_ModeChange();

				//LDM PC - takes 2 extra cycles
				ARM7_ICOUNT -=2;
			}
		}
		//LDM (NO PC) takes nS + 1n + 1I cycles (n = # of register transfers)
		ARM7_ICOUNT -= (result+1+1);
	} /* Loading */
	else
	{
		/* Storing - STM*/
		if (insn & (1<<eR15))
		{
			#if ARM7_DEBUG_CORE
				LOG(("%08x: Writing R15 in strm\n",R15));
			#endif
			/* special case handling if writing to PC */
			R15 += 12;
		}
		if (insn & INSN_BDT_U)
		{
			/* Incrementing */
			if (!(insn & INSN_BDT_P))
			{
				rbp = rbp + (- 4);
			}

			//S Flag Set, but R15 not in list = User Bank Transfer
			if(insn & INSN_BDT_S/* && ((insn & 0x8000)==0)*/)
			{
				//todo: needs to be tested..
				
				//set to user mode - then do the transfer, and set back
				//int curmode = GET_MODE;
				//SwitchMode(eARM7_MODE_USER);
			#if ARM7_DEBUG_CORE
				LOG(("%08x: User Bank Transfer not fully tested - please check if working properly!\n",R15));
			#endif
				result = storeIncMode(insn & 0xffff, rbp, eARM7_MODE_USER);
				//todo - not sure if Writeback occurs on User registers also.. 
				//SwitchMode(curmode);
			}
			else
				result = storeIncMode(insn & 0xffff, rbp, GET_MODE);

			if(( insn & INSN_BDT_W ) && (ARM7.pendingAbtD == 0))
			{
				SET_REGISTER(rb,GET_REGISTER(rb)+result*4);
			}
		}
		else
		{
			/* Decrementing - but real CPU writes in incrementing order */
			if (!(insn & INSN_BDT_P))
			{
				rbp = rbp - (- 4);
			}

			//S Flag Set, but R15 not in list = User Bank Transfer
			if(insn & INSN_BDT_S /*&& ((insn & 0x8000)==0)*/)
			{
				//set to user mode - then do the transfer, and set back
				//int curmode = GET_MODE;
				//SwitchMode(eARM7_MODE_USER);
			#if ARM7_DEBUG_CORE
				LOG(("%08x: User Bank Transfer not fully tested - please check if working properly!\n",R15));
			#endif
				result = storeDecMode(insn & 0xffff, rbp, eARM7_MODE_USER);
				//todo - not sure if Writeback occurs on User registers also.. 
				//SwitchMode(curmode);
			}
			else
				result = storeDecMode(insn & 0xffff, rbp, GET_MODE);

			if(( insn & INSN_BDT_W ) && (ARM7.pendingAbtD == 0))
			{
				SET_REGISTER(rb,GET_REGISTER(rb)-result*4);
			}
		}
		if( insn & (1<<eR15) )
			R15 -= 12; //SJE: We added 12 for storing, but put it back as it was for executing

		// STM takes (n-1)S + 2N cycles (n = # of register transfers)
		ARM7_ICOUNT -= (result - 1) + 2;
	}

	// We will specify the cycle count for each case, so remove the -3 that occurs at the end
	ARM7_ICOUNT += 3;

} /* HandleMemBlock */
