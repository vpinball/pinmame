// license:BSD-3-Clause

#include "mips32.h"
#include <stdarg.h>
#include <stdio.h>

static const char *const rn[32] = {
	"$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
	"$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
	"$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
	"$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"
};

#define RS(op)    (((op) >> 21) & 31)
#define RT(op)    (((op) >> 16) & 31)
#define RD(op)    (((op) >> 11) & 31)
#define SA(op)    (((op) >> 6) & 31)
#define FUNCT(op) ((op) & 63)
#define SIMM(op)  ((int)(int16_t)((op) & 0xFFFF))
#define UIMM(op)  ((unsigned)((op) & 0xFFFF))
#define BTARGET(pc, op) ((unsigned)((pc) + 4 + ((uint32_t)(int32_t)(int16_t)((op) & 0xFFFF) << 2)))

static void f(char *buf, const char *m, const char *fmt, ...)
{
	va_list ap;
	int n = sprintf(buf, "%-8s", m);
	va_start(ap, fmt);
	vsprintf(buf + n, fmt, ap);
	va_end(ap);
}

static const char *special_r3(unsigned fn)
{
	switch (fn) {
	case 0x0A: return "movz";
	case 0x0B: return "movn";
	case 0x20: return "add";
	case 0x21: return "addu";
	case 0x22: return "sub";
	case 0x23: return "subu";
	case 0x24: return "and";
	case 0x25: return "or";
	case 0x26: return "xor";
	case 0x27: return "nor";
	case 0x2A: return "slt";
	case 0x2B: return "sltu";
	}
	return NULL;
}
static const char *const special_trap[8] = { "tge", "tgeu", "tlt", "tltu", "teq", NULL, "tne", NULL };
static const char *loadstore(unsigned o)
{
	switch (o) {
	case 0x20: return "lb";
	case 0x21: return "lh";
	case 0x22: return "lwl";
	case 0x23: return "lw";
	case 0x24: return "lbu";
	case 0x25: return "lhu";
	case 0x26: return "lwr";
	case 0x28: return "sb";
	case 0x29: return "sh";
	case 0x2A: return "swl";
	case 0x2B: return "sw";
	case 0x2E: return "swr";
	case 0x30: return "ll";
	case 0x38: return "sc";
	}
	return NULL;
}
static const char *regimm(unsigned rt)
{
	switch (rt) {
	case 0x00: return "bltz";
	case 0x01: return "bgez";
	case 0x02: return "bltzl";
	case 0x03: return "bgezl";
	case 0x10: return "bltzal";
	case 0x11: return "bgezal";
	case 0x12: return "bltzall";
	case 0x13: return "bgezall";
	}
	return NULL;
}
static const char *const regimm_trap[8] = { "tgei", "tgeiu", "tlti", "tltiu", "teqi", NULL, "tnei", NULL };

static void word(char *buf, uint32_t op) { sprintf(buf, "%-8s0x%08x", ".word", (unsigned)op); }

static void dasm_special(char *buf, uint32_t op)
{
	unsigned fn = FUNCT(op);

	if (special_r3(fn)) { f(buf, special_r3(fn), "%s,%s,%s", rn[RD(op)], rn[RS(op)], rn[RT(op)]); return; }
	if (fn >= 0x30 && fn <= 0x37 && special_trap[fn - 0x30]) { f(buf, special_trap[fn - 0x30], "%s,%s", rn[RS(op)], rn[RT(op)]); return; }
	switch (fn) {
	case 0x00:
		if (op == 0) sprintf(buf, "nop");
		else if (op == 0x40) sprintf(buf, "ssnop");
		else if (op == 0xC0) sprintf(buf, "ehb");
		else f(buf, "sll", "%s,%s,%u", rn[RD(op)], rn[RT(op)], SA(op));
		break;
	case 0x02: f(buf, (op & (1u << 21)) ? "rotr" : "srl", "%s,%s,%u", rn[RD(op)], rn[RT(op)], SA(op)); break;
	case 0x03: f(buf, "sra", "%s,%s,%u", rn[RD(op)], rn[RT(op)], SA(op)); break;
	case 0x04: f(buf, "sllv", "%s,%s,%s", rn[RD(op)], rn[RT(op)], rn[RS(op)]); break;
	case 0x06: f(buf, (op & (1u << 6)) ? "rotrv" : "srlv", "%s,%s,%s", rn[RD(op)], rn[RT(op)], rn[RS(op)]); break;
	case 0x07: f(buf, "srav", "%s,%s,%s", rn[RD(op)], rn[RT(op)], rn[RS(op)]); break;
	case 0x08: f(buf, "jr", "%s", rn[RS(op)]); break;
	case 0x09: f(buf, "jalr", "%s,%s", rn[RD(op)], rn[RS(op)]); break;
	case 0x0C: sprintf(buf, "syscall"); break;
	case 0x0D: sprintf(buf, "break"); break;
	case 0x0F: sprintf(buf, "sync"); break;
	case 0x10: f(buf, "mfhi", "%s", rn[RD(op)]); break;
	case 0x11: f(buf, "mthi", "%s", rn[RS(op)]); break;
	case 0x12: f(buf, "mflo", "%s", rn[RD(op)]); break;
	case 0x13: f(buf, "mtlo", "%s", rn[RS(op)]); break;
	case 0x18: f(buf, "mult", "%s,%s", rn[RS(op)], rn[RT(op)]); break;
	case 0x19: f(buf, "multu", "%s,%s", rn[RS(op)], rn[RT(op)]); break;
	case 0x1A: f(buf, "div", "%s,%s", rn[RS(op)], rn[RT(op)]); break;
	case 0x1B: f(buf, "divu", "%s,%s", rn[RS(op)], rn[RT(op)]); break;
	default: word(buf, op); break;
	}
}

static void dasm_cop0(char *buf, uint32_t op)
{
	if (op & (1u << 25)) {
		if (FUNCT(op) == 0x18) sprintf(buf, "eret");
		else if (FUNCT(op) == 0x20) sprintf(buf, "wait");
		else word(buf, op);
		return;
	}
	switch (RS(op)) {
	case 0x00: f(buf, "mfc0", "%s,$%u,%u", rn[RT(op)], RD(op), (unsigned)(op & 7)); break;
	case 0x04: f(buf, "mtc0", "%s,$%u,%u", rn[RT(op)], RD(op), (unsigned)(op & 7)); break;
	case 0x0A: f(buf, "rdpgpr", "%s,%s", rn[RD(op)], rn[RT(op)]); break;
	case 0x0B:
		if (RT(op)) f(buf, (op & 0x20) ? "ei" : "di", "%s", rn[RT(op)]);
		else sprintf(buf, "%s", (op & 0x20) ? "ei" : "di");
		break;
	case 0x0E: f(buf, "wrpgpr", "%s,%s", rn[RD(op)], rn[RT(op)]); break;
	default: word(buf, op); break;
	}
}

static void dasm_special2(char *buf, uint32_t op)
{
	switch (FUNCT(op)) {
	case 0x00: f(buf, "madd", "%s,%s", rn[RS(op)], rn[RT(op)]); break;
	case 0x01: f(buf, "maddu", "%s,%s", rn[RS(op)], rn[RT(op)]); break;
	case 0x02: f(buf, "mul", "%s,%s,%s", rn[RD(op)], rn[RS(op)], rn[RT(op)]); break;
	case 0x04: f(buf, "msub", "%s,%s", rn[RS(op)], rn[RT(op)]); break;
	case 0x05: f(buf, "msubu", "%s,%s", rn[RS(op)], rn[RT(op)]); break;
	case 0x20: f(buf, "clz", "%s,%s", rn[RD(op)], rn[RS(op)]); break;
	case 0x21: f(buf, "clo", "%s,%s", rn[RD(op)], rn[RS(op)]); break;
	default: word(buf, op); break;
	}
}

static void dasm_special3(char *buf, uint32_t op)
{
	switch (FUNCT(op)) {
	case 0x00: f(buf, "ext", "%s,%s,%u,%u", rn[RT(op)], rn[RS(op)], SA(op), RD(op) + 1); break;
	case 0x04: f(buf, "ins", "%s,%s,%u,%u", rn[RT(op)], rn[RS(op)], SA(op), RD(op) - SA(op) + 1); break;
	case 0x20:
		if (SA(op) == 0x02) f(buf, "wsbh", "%s,%s", rn[RD(op)], rn[RT(op)]);
		else if (SA(op) == 0x10) f(buf, "seb", "%s,%s", rn[RD(op)], rn[RT(op)]);
		else if (SA(op) == 0x18) f(buf, "seh", "%s,%s", rn[RD(op)], rn[RT(op)]);
		else word(buf, op);
		break;
	case 0x3B: f(buf, "rdhwr", "%s,$%u", rn[RT(op)], RD(op)); break;
	default: word(buf, op); break;
	}
}

unsigned mips32_dasm(char *buf, uint32_t pc, uint32_t op)
{
	unsigned o = op >> 26;

	if (loadstore(o)) { f(buf, loadstore(o), "%s,%d(%s)", rn[RT(op)], SIMM(op), rn[RS(op)]); return 4; }
	switch (o) {
	case 0x00: dasm_special(buf, op); break;
	case 0x01:
		if (regimm(RT(op))) f(buf, regimm(RT(op)), "%s,0x%08x", rn[RS(op)], BTARGET(pc, op));
		else if (RT(op) >= 8 && RT(op) <= 15 && regimm_trap[RT(op) - 8]) f(buf, regimm_trap[RT(op) - 8], "%s,%d", rn[RS(op)], SIMM(op));
		else if (RT(op) == 0x1F) f(buf, "synci", "%d(%s)", SIMM(op), rn[RS(op)]);
		else word(buf, op);
		break;
	case 0x02: f(buf, "j", "0x%08x", (unsigned)(((pc + 4) & 0xF0000000u) | ((op & 0x03FFFFFFu) << 2))); break;
	case 0x03: f(buf, "jal", "0x%08x", (unsigned)(((pc + 4) & 0xF0000000u) | ((op & 0x03FFFFFFu) << 2))); break;
	case 0x04: case 0x05: case 0x14: case 0x15:
		f(buf, o == 4 ? "beq" : o == 5 ? "bne" : o == 0x14 ? "beql" : "bnel", "%s,%s,0x%08x", rn[RS(op)], rn[RT(op)], BTARGET(pc, op));
		break;
	case 0x06: case 0x07: case 0x16: case 0x17:
		f(buf, o == 6 ? "blez" : o == 7 ? "bgtz" : o == 0x16 ? "blezl" : "bgtzl", "%s,0x%08x", rn[RS(op)], BTARGET(pc, op));
		break;
	case 0x08: f(buf, "addi", "%s,%s,%d", rn[RT(op)], rn[RS(op)], SIMM(op)); break;
	case 0x09: f(buf, "addiu", "%s,%s,%d", rn[RT(op)], rn[RS(op)], SIMM(op)); break;
	case 0x0A: f(buf, "slti", "%s,%s,%d", rn[RT(op)], rn[RS(op)], SIMM(op)); break;
	case 0x0B: f(buf, "sltiu", "%s,%s,%d", rn[RT(op)], rn[RS(op)], SIMM(op)); break;
	case 0x0C: f(buf, "andi", "%s,%s,0x%x", rn[RT(op)], rn[RS(op)], UIMM(op)); break;
	case 0x0D: f(buf, "ori", "%s,%s,0x%x", rn[RT(op)], rn[RS(op)], UIMM(op)); break;
	case 0x0E: f(buf, "xori", "%s,%s,0x%x", rn[RT(op)], rn[RS(op)], UIMM(op)); break;
	case 0x0F: f(buf, "lui", "%s,0x%x", rn[RT(op)], UIMM(op)); break;
	case 0x10: dasm_cop0(buf, op); break;
	case 0x1C: dasm_special2(buf, op); break;
	case 0x1F: dasm_special3(buf, op); break;
	case 0x2F: f(buf, "cache", "0x%x,%d(%s)", RT(op), SIMM(op), rn[RS(op)]); break;
	case 0x33: f(buf, "pref", "%u,%d(%s)", RT(op), SIMM(op), rn[RS(op)]); break;
	default: word(buf, op); break;
	}
	return 4;
}
