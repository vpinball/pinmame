#include <stdio.h>
#include "osd_cpu.h"
#include "memory.h"

#define OP(A)   cpu_readop(A)
#define ARG(A)  cpu_readop_arg(A)

unsigned Dasm4004(char *buff, unsigned pc)
{
	UINT8 op;
	unsigned PC = pc;
	switch (op = OP(pc))
	{
		case 0x00: sprintf (buff,"nop");                             break;

		case 0x10: sprintf (buff,"jcn (*),$%X%02X", PC >> 8, ARG(++pc)); break;
		case 0x11: sprintf (buff,"jcn (~T),$%X%02X", PC >> 8, ARG(++pc)); break;
		case 0x12: sprintf (buff,"jcn (C),$%X%02X", PC >> 8, ARG(++pc)); break;
		case 0x13: sprintf (buff,"jcn (C~T),$%X%02X", PC >> 8, ARG(++pc)); break;
		case 0x14: sprintf (buff,"jcn (~A),$%X%02X", PC >> 8, ARG(++pc)); break;
		case 0x15: sprintf (buff,"jcn (~A~T),$%X%02X", PC >> 8, ARG(++pc)); break;
		case 0x16: sprintf (buff,"jcn (C~A),$%X%02X", PC >> 8, ARG(++pc)); break;
		case 0x17: sprintf (buff,"jcn (C~A~T),$%X%02X", PC >> 8, ARG(++pc)); break;
		case 0x18: sprintf (buff,"jcn (~*),$%X%02X", PC >> 8, ARG(++pc)); break;
		case 0x19: sprintf (buff,"jcn (T),$%X%02X", PC >> 8, ARG(++pc)); break;
		case 0x1a: sprintf (buff,"jcn (~C),$%X%02X", PC >> 8, ARG(++pc)); break;
		case 0x1b: sprintf (buff,"jcn (T~C),$%X%02X", PC >> 8, ARG(++pc)); break;
		case 0x1c: sprintf (buff,"jcn (A),$%X%02X", PC >> 8, ARG(++pc)); break;
		case 0x1d: sprintf (buff,"jcn (AT),$%X%02X", PC >> 8, ARG(++pc)); break;
		case 0x1e: sprintf (buff,"jcn (A~C),$%X%02X", PC >> 8, ARG(++pc)); break;
		case 0x1f: sprintf (buff,"jcn (AT~C),$%X%02X", PC >> 8, ARG(++pc)); break;

		case 0x20: sprintf (buff,"fim [01],#$%02x", ARG(++pc));      break;
		case 0x22: sprintf (buff,"fim [23],#$%02x", ARG(++pc));      break;
		case 0x24: sprintf (buff,"fim [45],#$%02x", ARG(++pc));      break;
		case 0x26: sprintf (buff,"fim [67],#$%02x", ARG(++pc));      break;
		case 0x28: sprintf (buff,"fim [89],#$%02x", ARG(++pc));      break;
		case 0x2a: sprintf (buff,"fim [AB],#$%02x", ARG(++pc));      break;
		case 0x2c: sprintf (buff,"fim [CD],#$%02x", ARG(++pc));      break;
		case 0x2e: sprintf (buff,"fim [EF],#$%02x", ARG(++pc));      break;

		case 0x21: sprintf (buff,"src [01]");                        break;
		case 0x23: sprintf (buff,"src [23]");                        break;
		case 0x25: sprintf (buff,"src [45]");                        break;
		case 0x27: sprintf (buff,"src [67]");                        break;
		case 0x29: sprintf (buff,"src [89]");                        break;
		case 0x2b: sprintf (buff,"src [AB]");                        break;
		case 0x2d: sprintf (buff,"src [CD]");                        break;
		case 0x2f: sprintf (buff,"src [EF]");                        break;

		case 0x30: sprintf (buff,"fin [01]");                        break;
		case 0x32: sprintf (buff,"fin [23]");                        break;
		case 0x34: sprintf (buff,"fin [45]");                        break;
		case 0x36: sprintf (buff,"fin [67]");                        break;
		case 0x38: sprintf (buff,"fin [89]");                        break;
		case 0x3a: sprintf (buff,"fin [AB]");                        break;
		case 0x3c: sprintf (buff,"fin [CD]");                        break;
		case 0x3e: sprintf (buff,"fin [EF]");                        break;

		case 0x31: sprintf (buff,"jin [01]");                        break;
		case 0x33: sprintf (buff,"jin [23]");                        break;
		case 0x35: sprintf (buff,"jin [45]");                        break;
		case 0x37: sprintf (buff,"jin [67]");                        break;
		case 0x39: sprintf (buff,"jin [89]");                        break;
		case 0x3b: sprintf (buff,"jin [AB]");                        break;
		case 0x3d: sprintf (buff,"jin [CD]");                        break;
		case 0x3f: sprintf (buff,"jin [EF]");                        break;

		case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
		case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
			sprintf (buff,"jun $%X%02X", (op & 0x0f), ARG(++pc)); break;

		case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
		case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f:
			sprintf (buff,"jms $%X%02X", (op & 0x0f), ARG(++pc)); break;

		case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
		case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f:
			sprintf (buff,"inc [%X]", (op & 0x0f)); break;

		case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
		case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f:
			sprintf (buff,"isz [%X],$%X%02X", (op & 0x0f), PC >> 8, ARG(++pc)); break;

		case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
		case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f:
			sprintf (buff,"add [%X]", (op & 0x0f)); break;

		case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
		case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f:
			sprintf (buff,"sub [%X]", (op & 0x0f)); break;

		case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
		case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
			sprintf (buff,"ld  [%X]", (op & 0x0f)); break;

		case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
		case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
			sprintf (buff,"xch [%X]", (op & 0x0f)); break;

		case 0xc0: case 0xc1: case 0xc2: case 0xc3: case 0xc4: case 0xc5: case 0xc6: case 0xc7:
		case 0xc8: case 0xc9: case 0xca: case 0xcb: case 0xcc: case 0xcd: case 0xce: case 0xcf:
			sprintf (buff,"bbl #$%x", (op & 0x0f)); break;

		case 0xd0: case 0xd1: case 0xd2: case 0xd3: case 0xd4: case 0xd5: case 0xd6: case 0xd7:
		case 0xd8: case 0xd9: case 0xda: case 0xdb: case 0xdc: case 0xdd: case 0xde: case 0xdf:
			sprintf (buff,"ldm #$%x", (op & 0x0f)); break;

		case 0xe0: sprintf (buff,"wrm");                             break;
		case 0xe1: sprintf (buff,"wmp");                             break;
		case 0xe2: sprintf (buff,"wrr");                             break;
		case 0xe3: sprintf (buff,"wpm");                             break; /* 4008 only opcode? */
		case 0xe4: sprintf (buff,"wr0");                             break;
		case 0xe5: sprintf (buff,"wr1");                             break;
		case 0xe6: sprintf (buff,"wr2");                             break;
		case 0xe7: sprintf (buff,"wr3");                             break;
		case 0xe8: sprintf (buff,"sbm");                             break;
		case 0xe9: sprintf (buff,"rdm");                             break;
		case 0xea: sprintf (buff,"rdr");                             break;
		case 0xeb: sprintf (buff,"adm");                             break;
		case 0xec: sprintf (buff,"rd0");                             break;
		case 0xed: sprintf (buff,"rd1");                             break;
		case 0xee: sprintf (buff,"rd2");                             break;
		case 0xef: sprintf (buff,"rd3");                             break;

		case 0xf0: sprintf (buff,"clb");                             break;
		case 0xf1: sprintf (buff,"clc");                             break;
		case 0xf2: sprintf (buff,"iac");                             break;
		case 0xf3: sprintf (buff,"cmc");                             break;
		case 0xf4: sprintf (buff,"cma");                             break;
		case 0xf5: sprintf (buff,"ral");                             break;
		case 0xf6: sprintf (buff,"rar");                             break;
		case 0xf7: sprintf (buff,"tcc");                             break;
		case 0xf8: sprintf (buff,"dac");                             break;
		case 0xf9: sprintf (buff,"tcs");                             break;
		case 0xfa: sprintf (buff,"stc");                             break;
		case 0xfb: sprintf (buff,"daa");                             break;
		case 0xfc: sprintf (buff,"kbp");                             break;
		case 0xfd: sprintf (buff,"dcl");                             break;

		default:   sprintf (buff,"illegal");
	}
	pc++;

	return pc - PC;
}

