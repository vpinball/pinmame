/*******************************************************
 *
 *      Portable PPS/4 emulator
 *
 *      Written by G. Volkenborn for use with PinMAME
 *
 *		Partially based on Z80Em by Marcel De Kogel
 *
 *      CPU related macros
 *
 *******************************************************/

#define M_IN(no)												\
	I.accu = cpu_readport16(no) & 0x0f;

#define M_OUT(no, val)											\
	cpu_writeport16(no, val);

#define M_JMP(hibyte) {											\
	PPS4_ICount--;												\
	I.PC.w.l = ARG();											\
	I.PC.b.h = hibyte;											\
	change_pc16(I.PC.d);										\
}
