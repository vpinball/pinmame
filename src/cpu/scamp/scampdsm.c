#include <stdio.h>
#include "osd_cpu.h"
#include "memory.h"
#include "scamp.h"

static UINT8 OP(unsigned pc)
{
	return cpu_readop(pc);
}

static UINT8 ARG(unsigned pc)
{
	return cpu_readop_arg(pc);
}

unsigned DasmScamp(char *buff, unsigned pc)
{
	UINT8 op, arg = 0;
	unsigned PC = pc;
	char s[7];

	op = OP(pc);
	if (op > 0x8f && op <= 0xff) {
		arg = ARG(++pc);
		if ((op & 0xf3) == 0x90) {
			sprintf(s, "$%04x", (PC + (INT8)arg + 1) & 0xffff);
		} else {
			if (arg == 0x80)
				sprintf (s, "P%d[E]", op & 0x03);
			else if (arg & 0x80)
				sprintf (s, "P%d-$%02x", op & 0x03, 0x100-arg);
			else
				sprintf (s, "P%d+$%02x", op & 0x03, arg);
		}
	}

	switch (op)
	{
		case 0x00: sprintf (buff,"halt");                            break;
		case 0x01: sprintf (buff,"xae");                             break;
		case 0x02: sprintf (buff,"ccl");                             break;
		case 0x03: sprintf (buff,"scl");                             break;
		case 0x04: sprintf (buff,"dint");                            break;
		case 0x05: sprintf (buff,"ien");                             break;
		case 0x06: sprintf (buff,"csa");                             break;
		case 0x07: sprintf (buff,"cas");                             break;
		case 0x08: sprintf (buff,"nop");                             break;

		case 0x19: sprintf (buff,"sio");                             break;
		case 0x1c: sprintf (buff,"sr");                              break;
		case 0x1d: sprintf (buff,"srl");                             break;
		case 0x1e: sprintf (buff,"rr");                              break;
		case 0x1f: sprintf (buff,"rrl");                             break;

		case 0x30: case 0x31: case 0x32: case 0x33:
			 sprintf (buff,"xpal P%d", op & 0x03);                   break;
		case 0x34: case 0x35: case 0x36: case 0x37:
			 sprintf (buff,"xpah P%d", op & 0x03);                   break;
		case 0x3c: case 0x3d: case 0x3e: case 0x3f:
			 sprintf (buff,"xppc P%d", op & 0x03);                   break;

		case 0x40: sprintf (buff,"lde");                             break;
		// instruction 0x48 might exist undocumented.
		case 0x48: sprintf (buff,"ste");                             break;

		case 0x50: sprintf (buff,"ande");                            break;
		case 0x58: sprintf (buff,"ore");                             break;

		case 0x60: sprintf (buff,"xore");                            break;
		case 0x68: sprintf (buff,"dade");                            break;

		case 0x70: sprintf (buff,"adde");                            break;
		case 0x78: sprintf (buff,"cade");                            break;

		case 0x8f: sprintf (buff,"dly  $%02x", ARG(++pc));           break;

		case 0x90: case 0x91: case 0x92: case 0x93:
			 sprintf (buff,"jmp  %s", s);                            break;
		case 0x94: case 0x95: case 0x96: case 0x97:
			 sprintf (buff,"jp   %s", s);                            break;
		case 0x98: case 0x99: case 0x9a: case 0x9b:
			 sprintf (buff,"jz   %s", s);                            break;
		case 0x9c: case 0x9d: case 0x9e: case 0x9f:
			 sprintf (buff,"jnz  %s", s);                            break;

		case 0xa8: case 0xa9: case 0xaa: case 0xab:
			 sprintf (buff,"ild  %s", s);                            break;

		case 0xb8: case 0xb9: case 0xba: case 0xbb:
			 sprintf (buff,"dld  %s", s);                            break;

		case 0xc0: case 0xc1: case 0xc2: case 0xc3:
			 sprintf (buff,"ld   %s", s);                            break;
		case 0xc4: sprintf (buff,"ldi  #$%02x", arg);                break;
		case 0xc5: case 0xc6: case 0xc7:
			 sprintf (buff,"lda  %s", s);                            break;

		case 0xc8: case 0xc9: case 0xca: case 0xcb:
			 sprintf (buff,"st   %s", s);                            break;
		case 0xcc: case 0xcd: case 0xce: case 0xcf:
			 sprintf (buff,"sta  %s", s);                            break;

		case 0xd0: case 0xd1: case 0xd2: case 0xd3:
			 sprintf (buff,"and  %s", s);                            break;
		case 0xd4: sprintf (buff,"andi #$%02x", arg);                break;
		case 0xd5: case 0xd6: case 0xd7:
			 sprintf (buff,"anda %s", s);                            break;

		case 0xd8: case 0xd9: case 0xda: case 0xdb:
			 sprintf (buff,"or   %s", s);                            break;
		case 0xdc: sprintf (buff,"ori  #$%02x", arg);                break;
		case 0xdd: case 0xde: case 0xdf:
			 sprintf (buff,"ora  %s", s);                            break;

		case 0xe0: case 0xe1: case 0xe2: case 0xe3:
			 sprintf (buff,"xor  %s", s);                            break;
		case 0xe4: sprintf (buff,"xori #$%02x", arg);                break;
		case 0xe5: case 0xe6: case 0xe7:
			 sprintf (buff,"xora %s", s);                            break;

		case 0xe8: case 0xe9: case 0xea: case 0xeb:
			 sprintf (buff,"dad  %s", s);                            break;
		case 0xec: sprintf (buff,"dadi #$%02x", arg);                break;
		case 0xed: case 0xee: case 0xef:
			 sprintf (buff,"dada %s", s);                            break;

		case 0xf0: case 0xf1: case 0xf2: case 0xf3:
			 sprintf (buff,"add  %s", s);                            break;
		case 0xf4: sprintf (buff,"addi #$%02x", arg);                break;
		case 0xf5: case 0xf6: case 0xf7:
			 sprintf (buff,"adda %s", s);                            break;

		case 0xf8: case 0xf9: case 0xfa: case 0xfb:
			 sprintf (buff,"cad  %s", s);                            break;
		case 0xfc: sprintf (buff,"cadi #$%02x", arg);                break;
		case 0xfd: case 0xfe: case 0xff:
			 sprintf (buff,"cada %s", s);                            break;

		default:   sprintf (buff,"*ill $%02x*", op);
	}
	pc++;

	return pc - PC;
}
