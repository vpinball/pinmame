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

#define XF              0x08
#define VF              0x04
#define NF              0x02
#define CF              0x01

#define M_IN													\
	I.XX.d=ARG();												\
	I.AF.b.h=cpu_readport16(I.XX.d);

#define M_OUT													\
	I.XX.d=ARG();												\
	cpu_writeport16(I.XX.d,I.AF.b.h)

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
