/*******************************************************
 *
 *      Portable 4004 emulator
 *
 *      Written by G. Volkenborn for use with PinMAME
 *
 *		Partially based on Z80Em by Marcel De Kogel
 *
 *      CPU related macros
 *
 *******************************************************/

#define M_IN													\
	I.accu = cpu_readport16(I.ramaddr.d & 0xff);

#define M_OUT(no)												\
	cpu_writeport16((I.ramaddr.d & 0xff) | (no * 0x100), I.accu);

#define M_JMP(romno, cc) {										\
	i4004_ICount -= 8;											\
	if (cc) {													\
		I.PC.w.l = ARG();										\
		I.PC.b.h = romno;										\
	} else {													\
		I.PC.w.l++;												\
	}															\
	change_pc16(I.PC.d);										\
}
