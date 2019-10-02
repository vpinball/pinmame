/*
 *   JIT native code emitter framework for Intel.  This is essentially a
 *   mini-assembler for Intel machine code.  It lets us write instructions
 *   with C macros that look a lot like assembly language mnemonics (although
 *   with some adjustments to conform to C macro syntax), and generates the
 *   corresponding binary machine code bytes.  The assembler performs jump
 *   offset fixups and optimizes branch offset size.  It also takes handles
 *   linkage to external C code for CALLs and JMPs.
 *   
 *   Note that there's no *actual* assembler involved.  Don't look for a call
 *   out to NASM or anything like that - you won't find it.  This module does
 *   the whole job by itself.  We're just performing roughly the same work
 *   that an assembler would, in that we translate from a mnemonic
 *   instruction format to binary machine code and arrange the machine code
 *   bytes in memory for execution.
 *   
 *   The assembly process is relatively fast, since it doesn't have to do any
 *   string parsing (everything's pre-digested as macro arguments), but it
 *   does impose some overhead.  There's work involved in mapping the
 *   mnemonic instruction format to binary opcodes and encoding the bit
 *   fields for operands.  This is the trade-off for readability.  The
 *   fastest way to generate the machine code would be to hand-code the
 *   binary instructions, but this would be quite tedious.  It would also
 *   make the code hard to read, and it would be hugely error-prone.
 *   Fortunately, we can afford a little overhead time for this assembler,
 *   since it only has to run once for a given instruction; after an
 *   instruction has been translated once, the binary machine code can be
 *   executed arbitrarily many times without incurring any further
 *   translation overhead.  That's actually the whole point of the JIT -
 *   whereas the emulator has to decode every instruction it executes, every
 *   time it executes it, the JIT only has to decode and assemble an
 *   instruction once.  From then on it exists as native code that can be
 *   invoked directly.
 *   
 *   It's quite possible to write ill-formed instruction mnemonics using the
 *   macros.  The assembler is intolerant of errors; it uses ASSERT to check
 *   that instructions are well-formed.  This is acceptable because all
 *   assembly input comes from internal sources.  Any error in the assembly
 *   input is simply a bug in the calling subroutine that must be fixed, so
 *   an ASSERT failure is the proper handling.
 *   
 *   * * *
 *   
 *   Some notes on Intel opcode notation: There are several complex patterns
 *   in Intel opcodes that recur frequently enough to need some special
 *   notation for brevity.
 *   
 *   r8, r16, r32 -> one of the basic 8 data registers, in their 8-bit,
 *   16-bit, or 32-bit forms.
 *   
 *   r/m8, r/m16, r/m32 -> a MOD REG R/M byte encoding an 8-, 16-, or 32-bit
 *   quantity.  All of our generated code is in a 32-bit data segment, so
 *   r/m16 means that an OPSIZE prefix byte must be used to specify a 16-bit
 *   operand.
 *   
 *   imm8, imm16, imm32 -> an immediate 8-, 16-, or 32-bit operand.
 *   
 *   OPCODE /n -> an opcode that has a 3-bit extension, n, encoded into the
 *   REG field of the MOD REG R/M byte.  These are generally one-operand
 *   instructions, or instructions with one register/memory operand and one
 *   immediate operand.  For example, SHR r/m32,imm8 is opcode C1 /5, meaning
 *   that the opcode byte is C1, and then the constant value 5 is coded in
 *   the REG field of the MOD REG R/M byte after the opcode.  The 5 in the
 *   REG field doesn't indicate an operand; it's just an extension of the
 *   opcode byte that tells the processor which of 8 possible instructions
 *   the C1 should perform.  C1 can also mean ROL (C1 /0), ROR (C1 /1), RCL
 *   (C1 /2), RCR (C1 /3), SAL (C1 /4), SHR (C1 /5), or SAR (C1 /7).  The
 *   remaining bits of the MOD REG R/M byte do specify an operand as usual.
 *   
 *   OPCODE+r -> an opcode that has a register ID encoded in the low-order 3
 *   bits of the opcode.  For example, INC r32 is opcode 40+r: INC EAX is 40,
 *   INC ECX is 41, etc.  The Intel documentation denotes this as simply
 *   OPCODE+ (sans the 'r').
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "memory.h"
#include "jit.h"
#include "jitemit.h"

#if JIT_ENABLED

#if JIT_DEBUG
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)
#endif


/*
 *   Code gen stack.  This allows for recursive code generation by letting
 *   the caller save the current emitter state in the middle of generating a
 *   given code block, generate code for a second block, and then restore the
 *   state and resume generating the first block.
 */
struct stackele {
	struct instr *ihead;      // head of instruction list (first instruction)
	struct instr *itail;      // tail of instruction list (most recent instruction generated)
	struct instr *iop;        // first instruction for current emulator opcode
	struct jit_label *l;      // head of label list
	struct stackele *parent;  // parent stack element
};
struct stackele *stktop = 0;

/*
 *   Instruction queue.  
 */
struct queueele {
	data32_t emuaddr;         // emulator address of this queued instruction
	struct queueele *nxt;     // next queue element
};
struct queueele *queueHead = 0, *queueTail = 0;

/*
 *   Queue an instruction for later processing. 
 */
void jit_emit_queue_instr(data32_t addr)
{
	// create a queue item
	struct queueele *q = (struct queueele *)malloc(sizeof(struct queueele));
	q->nxt = 0;
	q->emuaddr = addr;

	// link it into the queue
	if (queueTail != 0)
		queueTail->nxt = q;
	else
		queueHead = q;
	queueTail = q;
}

/*
 *   Process queued instructions. 
 */
void jit_emit_process_queue(struct jit_ctl *jit, int (*func)(struct jit_ctl *jit, data32_t addr))
{
	// keep going until the queue is empty
	while (queueHead != 0)
	{
		byte *p;
		struct queueele *q;

		// pull the next item off the queue
		q = queueHead;
		queueHead = q->nxt;
		q->nxt = 0;

		// Look up the address.  If it's in the 'pending' state, translate it,
		// otherwise just discard it.
		if (q->emuaddr >= jit->minAddr && q->emuaddr < jit->maxAddr)
		{
			p = JIT_NATIVE(jit, q->emuaddr);
			if (p == jit->pPending)
			{
				// translate it
				(*func)(jit, q->emuaddr);
			}
		}

		// done with the queue element - discard it
		free(q);
	}

	// the queue is now empty
	queueHead = queueTail = 0;
}

/*
 *   Instruction struct.  We create one of these for each Intel opcode we
 *   generate.  These are linked into the master list at the stack level.
 *   
 *   The main purpose of keeping the instructions in these little structs is
 *   to allow editing of the list when finalizing jump distances.  The Intel
 *   conditional jump instructions can only encode 8-bit offsets, so if a
 *   conditional jump exceeds +/-127 bytes, we need to rewrite it as a
 *   conditional jump (with the opposite condition) around a long jump that
 *   does the actual jump to the distant location.  For example, if we have
 *   "JZ $1", and label $1 is 1000 bytes away, we have to rewrite this as
 *   "JNZ $+5; JMP $1", where the second jump is the 32-bit operand form that
 *   allows us to make the more distance branch.
 *   
 *   This type of editing creates the complication that we don't know how
 *   many bytes our conditional jump instructions will actually use in the
 *   instruction stream until we have the final code layout.  This is further
 *   complicated by the fact that rewriting one jump could insert enough code
 *   that it pushes a nearby but otherwise unrelated jump over its 127-byte
 *   limit, requiring the same rewrite on that second jump.  This could
 *   cascade further still, so before we can commit to a byte stream layout,
 *   we have to go through the code repeatedly and check for out-of-bounds
 *   jumps, editing each one we find, and then repeating the whole check from
 *   the beginning.  We need to repeat this until we go through an entire
 *   pass without rewrites.  Keeping instructions in a list makes this
 *   process easier because it lets us avoid committing to a byte layout
 *   until we've done all of these rewrites.
 */
struct instr {
	struct instr *nxt;       // next instruction in list at stack level
	struct instr *prv;       // previous instruction in the list
	data32_t emuaddr;        // emulator address of the code
	void *old_native_state;  // original JIT_NATIVE() entry for emuaddr
	byte *nataddr;           // native address of the code
	struct jit_label *lbl;   // label - used for jmp/call instructions
	byte *op;                // pointer to primary opcode within b[] - this is the opcode
							 // byte, skipping any prefix bytes (TWOBYTE, OPSIZE)
	int effop;               // Effective opcode.  For a regular single-byte opcode, this is simply
							 // the opcode byte.  For a TWOBYTE opcode, this is 0x0Fxx, where xx
							 // is the opcode byte.  E.g., JG <ofs32> has effective opcode 0x0F8F.
	int len;                 // size of bytes of the opcode sequence
	byte b[1];               // opcode bytes; we overallocate to make room
};

/*
 *   Temporary code labels.  These are used to generate all jump
 *   instructions.  These can be used for forward references within generated
 *   code, or for references to the native code locations for translated
 *   emulator opcodes.
 *   
 *   A label can refer to either a generated instruction or an emulator
 *   opcode, not both.  If the instruction pointer is non-null, it's an
 *   instruction reference; otherwise it's an emulator opcode reference.  For
 *   the latter, we'll generate the appropriate code depending on whether or
 *   not the emulator opcode has been translated yet.  If it has, we'll
 *   generate a jump directly to the native code; if not, we'll do a run-time
 *   lookup and either jump to the native code if it's been generated since,
 *   or return to the emulator if not.
 */
struct jit_label {
	struct instr *i;        // target instruction, if an internal reference to generated code
	data32_t emuaddr;       // target opcode address, if a reference to an emulator code location
	byte *nataddr;          // target native address, if a reference to a fixed native routine
							// (e.g., a static C routine or a generated helper, such as jit->pLookup)
	struct jit_label *nxt;  // next label in list at this stack level
};

/* Intel opcodes prefixes */
#define TWOBYTE  0x0F
#define OPSIZE   0x66

/* some Intel opcodes we need for the instruction stream editing process */
#define opCALL     0xE8    // call 32-bit offset
#define opJMP_8    0xEB    // jump to 8-bit immediate offset
#define opJMP_32   0xE9    // jump to 32-bit immediate offset
#define opLOOPNZ   0xE0
#define opLOOPZ    0xE1
#define opLOOP     0xE2
#define opJECXZ    0xE3
#define opJO       0x70
#define opJG       0x7F
#define opJO32     0x0F80  // JO to 32-bit offset; uses TWOBYTE prefix
#define opJG32     0x0F8F  // JG to 32-bit offset; uses TWOBYTE prefix
#define opNOP      0x90

/* allocate a new instruction */
static struct instr *alloc_instr(int len, const byte *b)
{
	struct instr *iop;
	int alolen;
	const byte *op;
	int rem;
	int twobyte = 0;
	struct instr *i;

	// Find the primary opcode, skipping any prefix bytes
	op = b;
	rem = len;
	if (rem > 0 && *op == TWOBYTE) {
		++op, --rem;
		twobyte = 1;
	}
	if (rem > 0 && *op == OPSIZE)
		++op, --rem;

	// Figure the allocation size.  This is usually just the actual instruction
	// size given, but make allowances for instruction editing in some cases:
	//
	// - JMP with 8-bit operand: allocate space for expansion to a 32-bit operand,
	//   in case the jump turns out to be out of range and we need to use the
	//   long form.
	//
	// - Jcc with 8-bit operand: allocate space for expansion to a 32-bit operand.
	//   The ofs32 forms require not just the added operand space but also another
	//   byte for the TWOBYTE prefix, for a total of six bytes (0F 8x ofs32).
	//
	// See jit_emit_commit() for details on instruction editing.
	//
	alolen = len;
	if (len == 2 && b[0] == opJMP_8)
		alolen = 5;
	else if (len == 2 && (b[0] >= opJO && b[0] <= opJG))
		alolen = 6;
	
	// allocate the instruction
	i = (struct instr *)malloc(sizeof(struct instr) + alolen - 1);
	i->nxt = 0;
	i->prv = 0;
	i->nataddr = 0;
	i->lbl = 0;

	// copy the emulator address information from the lead generated code object
	// for the current emulator opcode, if set
	if ((iop = stktop->iop) != 0) {
		i->emuaddr = iop->emuaddr;
		i->old_native_state = iop->old_native_state;
	}
	else {
		i->emuaddr = 0;
		i->old_native_state = 0;
	}

	// copy the opcode bytes
	i->len = len;
	memcpy(i->b, b, len);

	// remember where the opcode starts
	i->op = i->b + (op - b);

	// set the effective opcode
	i->effop = op != 0 ? ((twobyte ? 0x0F00 : 0x0000) | *op) : opNOP;

	// return the instruction
	return i;
}

/* allocate a new instruction and link it into the list */
static struct instr *add_instr(int len, const byte *b)
{
	// allocate a new instruction
	struct instr *i = alloc_instr(len, b);
	
	// link it at the tail of the current stack level's list
	i->prv = stktop->itail;
	if (stktop->itail != 0)
		stktop->itail->nxt = i;
	else
		stktop->ihead = i;
	stktop->itail = i;

	// if there are no instructions for this opcode yet, it's the first
	if (stktop->iop == 0)
		stktop->iop = i;

	// return the new instruction
	return i;
}

static struct instr *ins_instr_before(struct instr *ibefore, int len, const byte *b)
{
	// allocate a new instruction
	struct instr *i = alloc_instr(len, b);
	i->emuaddr = ibefore->emuaddr;

	// link before the existing item
	i->nxt = ibefore;
	i->prv = ibefore->prv;
	if (ibefore->prv != 0)
		ibefore->prv->nxt = i;
	else
		stktop->ihead = i;
	ibefore->prv = i;

	// return the new instruction
	return i;
}

static struct instr *ins_instr_after(struct instr *iafter, int len, const byte *b)
{
	// allocate a new instruction
	struct instr *i = alloc_instr(len, b);
	i->emuaddr = iafter->emuaddr;

	// link after the existing item
	i->prv = iafter;
	i->nxt = iafter->nxt;
	if (iafter->nxt != 0)
		iafter->nxt->prv = i;
	else
		stktop->itail = i;
	iafter->nxt = i;

	// return the new instruction
	return i;
}


/*
 *   End the current instruction
 */
void end_instr()
{
	// if there's an instruction in progress, end it
	if (stktop->iop != 0)
	{
		struct instr *i;
		static byte nop[1] = { imNOP };
		int len;
		
		// Make sure we have at least 6 bytes of generated code, to
		// ensure there's room for the replacement code if we need
		// to un-translate this instruction in the future.  The
		// un-translate replacement is (MOV EAX,Imm32, RETN), which
		// requires 6 bytes.
		for (i = stktop->iop, len = 0 ; i != 0 ; len += i->len, i = i->nxt) ;
		for ( ; len < 6 ; ++len)
			add_instr(1, nop);

		// forget the open instruction
		stktop->iop = 0;
	}
}

/*
 *   Start a new emulator instruction 
 */
void jit_emit_begin_instr(struct jit_ctl *jit, data32_t addr)
{
	struct instr *i;

	// close out any existing instruction
	end_instr();

	// insert a pseudo-instruction to mark the address
	i = add_instr(0, 0);
	i->emuaddr = addr;

	// put the instruction in the "working" state, saving its old state in case we cancel
	i->old_native_state = JIT_NATIVE(jit, addr);
	JIT_NATIVE(jit, addr) = jit->pWorking;

	// mark this as the start of the current emulator opcode
	stktop->iop = i;
}

void jit_emit_cancel_instr(struct jit_ctl *jit)
{
	struct instr *i, *nxt;
	
	// do nothing if there's no current instruction
	if ((i = stktop->iop) == 0)
		return;

	// restore the instruction's original JIT_NATIVE entry
	JIT_NATIVE(jit, i->emuaddr) = i->old_native_state;

	// unlink stktop->iop
	stktop->itail = i->prv;
	if (stktop->itail != 0)
		stktop->itail->nxt = 0;
	else
		stktop->ihead = 0;

	// delete instructions from stktop->iop to end of list
	for ( ; i != 0 ; i = nxt)
	{
		nxt = i->nxt;
		free(i);
	}

	// Find the previous instruction head - this is the first instruction
	// with the same emulator address as the last instruction in the list.
	for (i = stktop->itail ; i != 0 && i->prv != 0 && i->prv->emuaddr == i->emuaddr ; i = i->prv) ;
	stktop->iop = i;
}


static struct jit_label *jit_new_label()
{
	// create a label structure
	struct jit_label *l = (struct jit_label *)malloc(sizeof(struct jit_label));
	l->i = 0;
	l->nataddr = 0;
	l->emuaddr = 0;

	// link it into the current stack list
	l->nxt = stktop->l;
	stktop->l = l;

	// return the new label
	return l;
}

struct jit_label *jit_new_fwd_label()
{
	return jit_new_label();
}

struct jit_label *jit_new_label_here()
{
	struct jit_label *l = jit_new_label();
	jit_resolve_label(l);
	return l;
}

// create a new label that targets the given instruction
struct jit_label *jit_new_label_at(struct instr *i)
{
	struct jit_label *l = jit_new_label();
	l->i = i;
	return l;
}

// create a label that points to the next instruction after 'i'
struct jit_label *jit_new_label_after(struct instr *i)
{
	// create the label
	struct jit_label *l = jit_new_label();

	// if there's no next instruction, add a NOP that we can jump to
	if (i->nxt == 0) {
		byte nop[1] = { imNOP };
		add_instr(1, nop);
	}

	// set the label instruction
	l->i = i->nxt;

	// return the new label
	return l;
}

struct jit_label *jit_new_addr_label(data32_t addr)
{
	struct jit_label *l = jit_new_label();
	l->emuaddr = addr;
	return l;
}

struct jit_label *jit_new_native_label(byte *addr)
{
	struct jit_label *l = jit_new_label();
	l->nataddr = addr;
	return l;
}

void jit_resolve_label(struct jit_label *l)
{
	// Generate a pseudo-instruction here as a placeholder for the
	// target.  This doesn't actually generate any native code, since
	// it has zero length; it's just in the list as a bookmark for
	// figuring the real address when we do the final address
	// calculations.
	l->i = add_instr(0, 0);
}

// Figure the native code address for a label.  This returns a valid
// native code address, or one of these special codes:
//
//   0 -> non-translatable instruction -> return to emulator
//   1 -> address pending translation -> do run-time lookup
//
#define TO_EMU     ((byte *)0)
#define TO_LOOKUP  ((byte *)1)
#define lbl_addr_special(p) ((p) <= TO_LOOKUP)
static byte *label_to_native(struct jit_ctl *jit, struct jit_label *l)
{
	byte *p;
	
	// if the label is attached to a generated instruction, it's the
	// generated instruction's native code address
	if (l->i != 0)
		return l->i->nataddr;

	// if the label is attached to a fixed native code location, use that
	if (l->nataddr != 0)
		return l->nataddr;

	// if the label isn't in the address range covered by the JIT, we
	// need to generate a return to the emulator
	if (l->emuaddr < jit->minAddr || l->emuaddr >= jit->maxAddr)
		return TO_EMU;

	// look up the native address for the emulator address
	p = JIT_NATIVE(jit, l->emuaddr);

	// If it's an "emulate" instruction, it will never become native code, so
	// just generate a return to the emulator
	if (p == jit->pEmulate)
		return TO_EMU;

	// If it's a "pending" or "working" instruction, it might become native code
	// at some point, but it's not yet, so generate a run-time lookup
	if (p == jit->pPending || p == jit->pWorking)
		return TO_LOOKUP;

	// this is a valid native code address for a translated instruction
	return p;
}

void jit_emit_commit(struct jit_ctl *jit)
{
	int mod;
	int len, reslen;
	data32_t emuaddr;
	byte *nataddr;
	byte *resp;
	struct instr *i;
	struct jit_page *pg;

	// if there's no code, there's nothing to do
	if (stktop->ihead == 0)
		return;

	// close out the current instruction, if any
	end_instr();

	// Figure the worst-case code size we need to reserve.  For
	// most instructions, the code size in the instruction record
	// is the actual generated code size.  However, we might need
	// to edit branch instructions that have 8-bit offsets to
	// reach destination addresses that are too far away.  These
	// might need to expand from 2 bytes (opcode + ofs8) to
	// 6 bytes (TwoByte + opcode + ofs32).  Those without ofs32
	// forms, such as the LOOP instructions and JECXZ, might need
	// to expand from 2 bytes (opcode + ofs) to 9 (for inserting
	// a 5-byte proxy jump and a 2-byte jump around the proxy).
	for (i = stktop->ihead, len = 0 ; i != 0 ; i = i->nxt)
	{
		// count the current generated size
		len += i->len;

		// If it has a label, and it's a 2-byte opcode, add space
		// to allow for editing:
		//   J<cond> can expand by 4 (opcode + ofs8 -> TwoByte + opcode + ofs32)
		//   LOOP/JECXZ can expand by 7 (for inserting a proxy 32-bit jump and 8-bit jump around proxy)
		//   JMP(short) can expand by 3 (for switching from 8-bit offset to 32-bit offset)
		if (i->len == 2 && i->lbl != 0)
		{
			if (i->effop >= opJO && i->effop <= opJG)
				len += 4;
			else if (i->effop >= opLOOPNZ && i->effop <= opJECXZ)
				len += 7;
			else if (i->effop == opJMP_8)
				len += 3;
		}

		// For any labeled jump that points to an emulator address, we might
		// need to replace a 32-bit JMP with the sequence MOV EAX,emuaddr
		// (B8 imm32) + RETN (C3), so we'd be replacing a 5-byte instruction
		// with 5+1 bytes.  This doesn't apply if the label points to an 
		// opcode that's already been translated, since we can jump directly
		// to the emulated code in that case.
		//
		// Some special conditions generate an additional MOV EAX, <retaddr>. 
		// Allocate 5 extra bytes in case.
		if (i->lbl != 0)
		{
			if (i->lbl->i != 0)
				len += 1;
			else
				len += 5;
		}
	}

	// add room for a NOP at the end, just in case
	len += 1;

	// Reserve the space
	resp = jit_reserve_native(jit, reslen = len, &pg);  

	// Edit jumps that exceed +/- 127 bytes.  On each pass, we'll
	// look for any two-byte jumps that are out of bounds.  For
	// each one we find, we'll replace it as follows:
	//
	//   JMP <byte offset> (EB oo) -> JMP <dword offset> (E9 oooooooo)
	//
	//   J<cond> <byte offset> (7x oo) -> J<cond> <dword offset> (0F 8x oooooooo)
	//
	// In either case, this requires moving all of the instructions
	// after the edited instruction to account for the expanded or
	// inserted opcode.  That in turn could trigger *new* out-of-bounds
	// jumps that we didn't detect on the first pass, because a jump
	// can span one of these insertions and can thus go from being
	// in bounds before the insertion to out of bounds after the edit.
	// So any time we make a change, we'll have to run a whole new
	// pass.  This process necessarily converges: an instruction
	// can only require editing once, since after editing it will be
	// able to reach anywhere, so in the worst case the loop
	// terminates once every instruction has been edited.
	do
	{
		// no modifications yet
		mod = 0;

		// Assign tentative native code addresses, based on the
		// assumption that each instruction will be generated at
		// its current size.  We have to re-figure this at the
		// start of each pass, because if we edit any instructions
		// (which is what triggers another pass), the subsequent
		// instructions will move to new addresses account for
		// the change in size.
		for (i = stktop->ihead, emuaddr = i->emuaddr - 1, nataddr = resp ; i != 0 ; i = i->nxt)
		{
			// assign the tentative native address to the instruction
			i->nataddr = nataddr;

			// assign it in the emulator address table as well if starting a new opcode
			if (emuaddr != i->emuaddr)
			{
				emuaddr = i->emuaddr;
				JIT_NATIVE(jit, emuaddr) = nataddr;
			}

			// move past the instruction's bytes
			nataddr += i->len;
		}

		// scan for jumps that require editing
		for (i = stktop->ihead ; i != 0 ; i = i->nxt)
		{
			// If it's a two-byte jump with a label, and the label is
			// further then -128..+127 bytes away, we need to expand this
			// to a 32-bit offset jump.  For each Jcc ofs8 (7x oo) opcode,
			// there's a corresponding opcode Jcc ofs32 (0F 8x oooooooo).
			// The low 4 bits of the 7x and 8x opcodes are the same for
			// a given condition, so we can adjust the primary opcode byte
			// simply by adding 0x10.  Note that the 0F (TWOBYTE) prefix
			// is also required for the ofs32 form.
			if (i->lbl != 0)
			{
				struct instr *inew, *inew2;
				static byte j32[5] = { opJMP_32, 0, 0, 0, 0 };  // 32-bit jump for insertion
				static byte j8[2] = { opJMP_8, 0 };             // 8-bit jump for insertion
				int use_proxy = 0;

				// get the label address
				byte *lblnat = label_to_native(jit, i->lbl);
				int lbldelta = lblnat - (i->nataddr + i->len);

				// If it's one of the special emulator handlers, it will definitely be
				// out of range of a one-byte jump.  Further, we must use a proxy jump,
				// even if we could do a more efficient single jump, because we'll need
				// to generate an extra instruction to load EAX with the target address
				// for the return or lookup.
				if (lblnat == TO_EMU || lblnat == TO_LOOKUP) {
					lbldelta = 0x7fffffff;
					use_proxy = 1;
				}

				// Check for native jumps that are out of range
				if (i->len == 2 && (lbldelta < -128 || lbldelta > 127))
				{
					// Out of bounds for 8-bit offset.  Check which type
					// of jump we have (conditional or unconditional).
					if (i->effop >= opJO && i->effop <= opJG)
					{
						// Conditional jump.
						if (use_proxy)
						{
							// We need a proxy jump.  Change from
							//       J<cond> <ofs8>  (7x oo)
							// to
							//       J<!cond> $1
							//       JMP <ofs32>
							//   $1:

							// Invert the condition.  The Intel Jcc opcodes are conveniently
							// arranged so that the hex opcodes for J<cond> and J<!cond> are
							// the same except for inverting the 0th bit of the opcode.
							i->op[0] ^= 1;
							i->effop ^= 1;

							// insert the proxy jump, and move the original label to the proxy
							inew = ins_instr_after(i, 5, j32);
							inew->lbl = i->lbl;

							// relabel the conditional jump to jump after the proxy jump
						    i->lbl = jit_new_label_after(inew);
						}
						else
						{
							// No proxy is needed, so we can use the more efficient ofs32
							// form of the conditional jump.  Change from
							//       J<cond> <ofs8> (7x oo)
							// to
							//       J<cond> <ofs32> (0F 8x oooooooo)
							//
							// Note that we already pre-allocated space for the expansion
							// when we created the instruction structure, so we can overwrite
							// it in place.
							i->b[1] = i->op[0] + 0x10;    // add 0x10 to go from 7x to 0F 8x primary opcode, move to second byte
							i->b[0] = 0x0F;               // add the TWOBYTE prefix
							i->op = i->b + 1;             // bump the primary opcode pointer past the TWOBYTE
							i->effop = 0x0F00 | i->op[0]; // fix the effective opcode
							i->len = 6;                   // expand to six bytes (0F 8x ofs32)
						}

						// note the edit
						mod = 1;
					}
					else if (i->effop >= opLOOPNZ && i->effop <= opJECXZ)
					{
						// LOOPxx/JECXZ instructions.  These instructions can't be
						// simply inverted the way the conditional jumps can.  For
						// these, we always need a proxy jump.  Point the LOOP to the
						// proxy jump, which will jump to the original LOOP target,
						// and then insert a jump around the proxy jump right after
						// the LOOP, so that we jump to the original code after the loop
						// when the LOOP falls through.  So we'll change LOOP <target> to:
						//
						//       LOOPxx/JECXZ $1     ; if continuing loop, jump to proxy jump 
						//       JMP(short) $2       ; this is reached on loop exit: jump around proxy jump
						//   $1: JMP(long) <target>  ; this is the proxy jump: jump to the LOOP target
						//   $2:                     ; next instruction after proxy jump

						// insert the jump around the proxy
						inew = ins_instr_after(i, 2, j8);

						// insert the proxy jump, and move the original target label to the proxy
						inew2 = ins_instr_after(inew, 5, j32);
						inew2->lbl = i->lbl;

						// change the LOOP target to a new label at the proxy jump
						i->lbl = jit_new_label_at(inew2);

						// set the label for the jump around the proxy
						inew->lbl = jit_new_label_after(inew2);

						// note the edit
						mod = 1;
					}
					else if (i->effop == opJMP_8)
					{
						// A simple unconditional 8-bit jump - simply change this to
						// a 32-bit jump.  Note that when we allocate a JMP struct, we
						// always allocate at the 32-bit size to allow for just this
						// expansion, so we can simply change the length now without
						// reallocating.
						i->op[0] = i->effop = opJMP_32;
						i->len = 5;

						// note the edit
						mod = 1;
					}
					else
					{
						ASSERT(0);
					}
				}
			}

			// Check again for a label in the post-edited instruction.  This time,
			// edit jumps that don't resolve to native code.
			if (i->lbl != 0)
			{
				byte mov[5] = { 0xB8, 0, 0, 0, 0 };  // MOV EAX,Imm32
				byte ret[1] = { 0xC3 };              // RETN, no operands

				// get the label address
				struct jit_label *lbl = i->lbl;
				byte *lblnat = label_to_native(jit, lbl);

				// If it's one of the special cases, it should only be possible
				// at this point for the instruction to be a 32-bit JMP.  All others
				// (conditionals, LOOP, etc) should have been replaced with new code
				// sequences where the only outside JMP is the 32-bit proxy JMP.
				ASSERT(!lbl_addr_special(lblnat) || (i->len == 5 && i->effop == opJMP_32));

				// check for the special cases
				if (lblnat == TO_EMU)
				{
					// Jump to non-translatable	emulator address.  These just do
					// a return to the emulator.  Set EAX to the target emulator
					// address, then do a RETN to return to the emulator.

					// delete the original JMP by making it a zero-byte instruction
					i->len = 0;

					// Remove the label from the instruction.  It's no longer
					// necessary, since we know where we'll be jumping.
					i->lbl = 0;

					// generate the MOV EAX, <target emulator address>
					*(UINT32 *)(&mov[1]) = lbl->emuaddr;
					i = ins_instr_after(i, 5, mov);

					// generate the RETN
					ins_instr_after(i, 1, ret);

					// we did an edit
					mod = 1;
				}
				else if (lblnat == TO_LOOKUP)
				{
					// Jump to pending instruction.  This instruction isn't yet
					// translated, but the JIT can translate more code in the future
					// as it's encountered, so the target instruction could be
					// translated by the time we execute the code we're generating
					// here.  Rather than just returning to the emulator, generate
					// a run-time lookup to see if the code is newly available.
					// To do this, load EAX with the target address and call the
					// lookup-and-patch handler.  If the target address still hasn't
					// been translated when we invoke the patch handler at run-time,
					// the patch handler will simply pop the CALL return address and
					// return to the emulator, so this is exactly like jumping to
					// the regular lookup handler.  If the address has been translated
					// on a run-time call, the patch handler will (per its name)
					// patch the caller's address (i.e., the instruction we're
					// generating here) with a jump directly to the newly translated
					// code address, bypassing the patch handler on subsequent
					// invocations of the caller and getting the code up to full speed.

					// insert the MOV EAX, <emulator address>
					*(UINT32 *)(&mov[1]) = lbl->emuaddr;
					ins_instr_before(i, 5, mov);

					// Change the destination label to the lookup-and-patch code.
					if (i->effop == opJMP_32) {
						i->lbl = jit_new_native_label(jit->pLookupPatch);
						i->op[0] = i->effop = opCALL;
					}
					else {
						ASSERT(0);
					}

					// we did an edit
					mod = 1;
				}
			}
		}
	}
	while (mod);

	// save each instruction
#if JIT_DEBUG
	int totallen = 0;
#endif
	emuaddr = stktop->ihead->emuaddr - 1;
	for (i = stktop->ihead ; i != 0 ; i = i->nxt)
	{
		// If the instruction has a label, set the final offset value
		if (i->lbl != 0)
		{
			// get the native code address of the label target, and
			// calculate the offset from the end of this instruction
			byte *lblnat = label_to_native(jit, i->lbl);
			int lbldelta = lblnat - (i->nataddr + i->len);

			// There can be no unresolved labels remaining at this point.
			// Anything that was pointing to non-translated code must have
			// been changed in the editing step above to point to the native
			// code that handles the emulator return, address lookup, or
			// whatever other special handling is required.
			ASSERT(!lbl_addr_special(lblnat));

			// find out what kind of instruction we have
			if (i->len == 2
				&& (i->effop == opJMP_8
					|| (i->effop >= opJO && i->effop <= opJG)
					|| (i->effop >= opLOOPNZ && i->effop <= opJECXZ)))
			{
				// 8-bit offset
				ASSERT(lbldelta >= -128 && lbldelta <= 127);
				i->b[1] = (byte)lbldelta;
			}
			else if (i->len == 5
					 && (i->effop == opCALL || i->effop == opJMP_32))
			{
				// 32-bit offset
				*(UINT32 *)(&i->b[1]) = lbldelta;
			}
			else if (i->len == 6
					 && (i->effop >= opJO32 && i->effop <= opJG32))
			{
				// 32-bit offset
				*(UINT32 *)(&i->op[1]) = lbldelta;
			}
			else
			{
				ASSERT(0);
			}
		}
		
		// copy this instruction to the JIT executable code page

#if JIT_DEBUG
		totallen += i->len;
#endif

		jit_store_native_from_reserved(jit, i->b, i->len, pg, i->nataddr);

		// if this is the first native instruction for this emulated opcode,
		// set the address mapping
		if (i->emuaddr != emuaddr) {
			emuaddr = i->emuaddr;
			JIT_NATIVE(jit, emuaddr) = i->nataddr;
		}
	}

#if JIT_DEBUG
	ASSERT(totallen <= reslen);
#endif

	// end the store-native operation
	jit_close_native(jit, resp, reslen);
}

// Emit a return to the emulator, resuming at the given address.
void jit_emit_return_to_emu(data32_t addr)
{
	byte mov[5] = { 0xB8, 0, 0, 0, 0 };   // MOV EAX,Imm32
	byte ret[1] = { 0xC3 };               // RETN, no operands
	
	// load EAX with the address, then return to the emulator
	*(UINT32 *)(&mov[1]) = addr;
	add_instr(5, mov);
	add_instr(1, ret);
}

int jit_emit_push()
{
	// create a new stack level
	struct stackele *e = (struct stackele *)malloc(sizeof(struct stackele));
	if (e == 0)
		return 0;

	// set it up
	e->ihead = 0;
	e->itail = 0;
	e->iop = 0;
	e->l = 0;

	// push it
	e->parent = stktop;
	stktop = e;

	// success
	return 1;
}

void jit_emit_pop()
{
	struct stackele *e = stktop;
	struct instr *i, *inxt;
	struct jit_label *l, *lnxt;

	// ignore if there's nothing on the stack
	if (e == 0)
		return;

	// unlink the top of stack
	stktop = e->parent;

	// delete instructions
	for (i = e->ihead ; i != 0 ; i = inxt)
	{
		inxt = i->nxt;
		free(i);
	}

	// delete labels
	for (l = e->l ; l != 0 ; l = lnxt)
	{
		lnxt = l->nxt;
		free(l);
	}
}

/*
 *   Operand types, for the opcode table.
 */
enum optype {
	none,         // no operand

	// This group matches types of registers.  These do not result in any
	// bytes in the instruction stream; the register ID is encoded either
	// into the MOD REG R/M byte, or into the opcode byte.
	r8,           // 8-bit general purpose register (AL, CL, DL, BL, AH, CH, DH, BH)
	r16,          // 16-bit general purpose register (AX, CX, DX, BX, BP, SP, SI, DI)
	r32,          // 32-bit general purpose register (EAX, EBX, ECX, EDX, EBP, ESP, ESI, EDI)
	rseg,         // segment register/instruction pointer (DS, ES, FS, GS, SS, CS, IP)

	// This group matches general memory/register operands.  These always
	// result in a MOD REG R/M byte, and possibly a SIB byte and displacement.
	rm8,          // 8-bit register/memory, encoded as a MOD R/M value
	rm16,         // 16-bit register/memory, encoded as a MOD R/M value
	rm32,         // 32-bit register/memory, encoded as a MOD R/M value

	// Immediates and offsets.  These are encoded directly into bytes
	// following the opcode, MOD REG R/M byte, SIB byte, and displacement.
	imm8,         // 8-bit immediate
	imm8S,        // sign-extended 8-bit immediate
	imm16,        // 16-bit immediate
	imm32,        // 32-bit immediate
	ofs8,         // 8-bit IP-relative offset
	ofs16,        // 16-bit IP-relative offset
	ofs32,        // 16-bit IP-relative offset

	// This group matches specific registers.  These don't result in any
	// bytes in the instruction stream; the register is implicit in the opcode.
	rAL,          // the AL register specifically
	rEAX,         // the EAX register specifically, or AX with an OPSIZE override
	rEBX,
	rECX,
	rEDX,
	rESP,
	rEBP,
	rESI,
	rEDI,
	rAX,
	rBX,
	rCX,
	rDX,
	rSP,
	rBP,
	rSI,
	rDI,
	rCL,
	rDS,          // the DS segment register
	rES,
	rSS,
	rCS,
	rFS,
	rGS,

	// This group matches specific immediates.  These don't result in any
	// bytes in the instruction stream; they value is implicit in the opcode.
	imm_1,        // the immediate value 1 specifically
	imm_3         // the immediate value 3 specifically
};
typedef enum optype optype;


/*
 *   Assembly language mapping table.  This defines the binary encoding for
 *   the each instruction mnemonic + operand set that we can generate.  In
 *   jit_emit(), we parse the mnemonic and operands, then search the table
 *   for an entry that matches the syntax given.
 */
struct mnedef {
	intelMneId mne;       // mnemonic ID for the instruction (imMOV, imADD, etc)
	optype     op1;       // left operand type
	optype     op2;       // right operand type
	optype     op3;       // third operand type (e.g., IMUL has imm8S or imm32 as third op)

	byte       opcode;    // Intel primary opcode byte
	byte       ext;       // 3-bit /n extension code (e.g., 4 for SHL rm32,imm8 -> C1 /4)

	int        cost;      // relative "cost" of instruction; when multiple templates match,
						  // we'll pick the one with the lowest cost, and then the one with
						  // the shortest binary encoding if the costs are the same

	byte       flags;     // encoding flags:
#define E_Ext     0x01    // opcode has a 3-bit /n extension
#define E_RExt    0x02    // opcode has a 3-bit /r extension (like /n, but encoding the operand 1 register number)
#define E_ImpR    0x04    // implied right operand (op #2) == left operand (e.g., 69 /r id IMUL EAX,4 -> IMUL EAX,EAX,4)
#define E_R       0x08    // opcode has register number encoded in bottom 3 bits
#define E_16      0x10    // explicit OPSIZE prefix required for this coding
#define E_TwoByte 0x20    // TWOBYTE prefix required for this coding

};

// IMPORTANT:  All entries for a given mnemonic must be contiguous.  We
// build an index table by mnenomic at startup that expects the entries
// to be grouped by mnemonic.
const static struct mnedef mnetab[] = {
  // opid   op1    op2    op3    opcode /n    cost  flags

	{ imMOV,  rm8,   r8,    none,  0x88,  0,    0,    0 },
	{ imMOV,  rm16,  r16,   none,  0x89,  0,    0,    E_16 },
	{ imMOV,  rm32,  r32,   none,  0x89,  0,    0,    0 },
	{ imMOV,  r8,    rm8,   none,  0x8A,  0,    0,    0 },
	{ imMOV,  r16,   rm16,  none,  0x8B,  0,    0,    E_16 },
	{ imMOV,  r32,   rm32,  none,  0x8B,  0,    0,    0 },
	{ imMOV,  rm16,  rseg,  none,  0x8C,  0,    0,    0 },
	{ imMOV,  rseg,  rm16,  none,  0x8E,  0,    0,    0 },
	{ imMOV,  r8,    imm8,  none,  0xB0,  0,    0,    E_R },
	{ imMOV,  r16,   imm16, none,  0xB8,  0,    0,    E_R | E_16 },
	{ imMOV,  r32,   imm32, none,  0xB8,  0,    0,    E_R },
	{ imMOV,  rm8,   imm8,  none,  0xC6,  0,    0,    0 },
	{ imMOV,  rm16,  imm16, none,  0xC7,  0,    0,    E_16 | E_Ext },
	{ imMOV,  rm32,  imm32, none,  0xC7,  0,    0,    E_Ext },

	{ imMOVSX,r16,   rm8,   none,  0xBE,  0,    0,    E_16 | E_TwoByte },
	{ imMOVSX,r32,   rm8,   none,  0xBE,  0,    0,    E_TwoByte },
	{ imMOVSX,r32,   rm16,  none,  0xBF,  0,    0,    E_TwoByte },

	{ imMOVZX,r16,   rm8,   none,  0xB6,  0,    0,    E_16 | E_TwoByte | E_RExt },
	{ imMOVZX,r32,   rm8,   none,  0xB6,  0,    0,    E_TwoByte | E_RExt },
	{ imMOVZX,r32,   rm16,  none,  0xB7,  0,    0,    E_TwoByte | E_RExt },

	{ imMOVSB,none,  none,  none,  0xA4,  0,    0,    0 },
	{ imMOVSW,none,  none,  none,  0xA5,  0,    0,    E_16 },
	{ imMOVSD,none,  none,  none,  0xA5,  0,    0,    0 },
	{ imCMPSB,none,  none,  none,  0xA6,  0,    0,    0 },
	{ imCMPSW,none,  none,  none,  0xA7,  0,    0,    E_16 },
	{ imCMPSD,none,  none,  none,  0xA7,  0,    0,    0 },
	{ imSTOSB,none,  none,  none,  0xAA,  0,    0,    0 },
	{ imSTOSW,none,  none,  none,  0xAB,  0,    0,    E_16 },
	{ imSTOSD,none,  none,  none,  0xAB,  0,    0,    0 },
	{ imLODSB,none,  none,  none,  0xAC,  0,    0,    0 },
	{ imLODSW,none,  none,  none,  0xAD,  0,    0,    E_16 },
	{ imLODSD,none,  none,  none,  0xAD,  0,    0,    0 },
	{ imSCASB,none,  none,  none,  0xAE,  0,    0,    0 },
	{ imSCASW,none,  none,  none,  0xAF,  0,    0,    E_16 },
	{ imSCASD,none,  none,  none,  0xAF,  0,    0,    0 },

	{ imADD,  rm8,   r8,    none,  0x00,  0,    0,    0 },
	{ imADD,  rm16,  r16,   none,  0x01,  0,    0,    E_16 },
	{ imADD,  rm32,  r32,   none,  0x01,  0,    0,    0 },
	{ imADD,  r8,    rm8,   none,  0x02,  0,    0,    0 },
	{ imADD,  r16,   rm16,  none,  0x03,  0,    0,    E_16 },
	{ imADD,  r32,   rm32,  none,  0x03,  0,    0,    0 },
	{ imADD,  rAL,   imm8,  none,  0x04,  0,    0,    0 },
	{ imADD,  rAX,   imm16, none,  0x05,  0,    0,    E_16 },
	{ imADD,  rEAX,  imm32, none,  0x05,  0,    0,    0 },
	{ imADD,  rm8,   imm8,  none,  0x80,  0,    0,    E_Ext },
	{ imADD,  rm16,  imm16, none,  0x81,  0,    0,    E_16 | E_Ext },
	{ imADD,  rm32,  imm32, none,  0x81,  0,    0,    E_Ext },
	{ imADD,  rm16,  imm8S, none,  0x83,  0,    0,    E_16 | E_Ext },
	{ imADD,  rm32,  imm8S, none,  0x83,  0,    0,    E_Ext },

	{ imOR,   rm8,   r8,    none,  0x08,  0,    0,    0 },
	{ imOR,   rm16,  r16,   none,  0x09,  0,    0,    E_16 },
	{ imOR,   rm32,  r32,   none,  0x09,  0,    0,    0 },
	{ imOR,   r8,    rm8,   none,  0x0A,  0,    0,    0 },
	{ imOR,   r16,   rm16,  none,  0x0B,  0,    0,    E_16 },
	{ imOR,   r32,   rm32,  none,  0x0B,  0,    0,    0 },
	{ imOR,   rAL,   imm8,  none,  0x0C,  0,    0,    0 },
	{ imOR,   rAX,   imm16, none,  0x0D,  0,    0,    E_16 },
	{ imOR,   rEAX,  imm32, none,  0x0D,  0,    0,    0 },
	{ imOR,   rm8,   imm8,  none,  0x80,  1,    0,    E_Ext },
	{ imOR,   rm16,  imm16, none,  0x81,  1,    0,    E_16 | E_Ext },
	{ imOR,   rm32,  imm32, none,  0x81,  1,    0,    E_Ext },
	{ imOR,   rm16,  imm8S, none,  0x83,  1,    0,    E_16 | E_Ext },
	{ imOR,   rm32,  imm8S, none,  0x83,  1,    0,    E_Ext },

	{ imADC,  rm8,   r8,    none,  0x10,  0,    0,    0 },
	{ imADC,  rm16,  r16,   none,  0x11,  0,    0,    E_16 },
	{ imADC,  rm32,  r32,   none,  0x11,  0,    0,    0 },
	{ imADC,  r8,    rm8,   none,  0x12,  0,    0,    0 },
	{ imADC,  r16,   rm16,  none,  0x13,  0,    0,    E_16 },
	{ imADC,  r32,   rm32,  none,  0x13,  0,    0,    0 },
	{ imADC,  rAL,   imm8,  none,  0x14,  0,    0,    0 },
	{ imADC,  rAX,   imm16, none,  0x15,  0,    0,    E_16 },
	{ imADC,  rEAX,  imm32, none,  0x15,  0,    0,    0 },
	{ imADC,  rm8,   imm8,  none,  0x80,  2,    0,    E_Ext },
	{ imADC,  rm16,  imm16, none,  0x81,  2,    0,    E_16 | E_Ext },
	{ imADC,  rm32,  imm32, none,  0x81,  2,    0,    E_Ext },
	{ imADC,  rm16,  imm8S, none,  0x83,  2,    0,    E_16 | E_Ext },
	{ imADC,  rm32,  imm8S, none,  0x83,  2,    0,    E_Ext },

	{ imSBB,  rm8,   r8,    none,  0x18,  0,    0,    0 },
	{ imSBB,  rm16,  r16,   none,  0x19,  0,    0,    E_16 },
	{ imSBB,  rm32,  r32,   none,  0x19,  0,    0,    0 },
	{ imSBB,  r8,    rm8,   none,  0x1A,  0,    0,    0 },
	{ imSBB,  r16,   rm16,  none,  0x1B,  0,    0,    E_16 },
	{ imSBB,  r32,   rm32,  none,  0x1B,  0,    0,    0 },
	{ imSBB,  rAL,   imm8,  none,  0x1C,  0,    0,    0 },
	{ imSBB,  rAX,   imm16, none,  0x1D,  0,    0,    E_16 },
	{ imSBB,  rEAX,  imm32, none,  0x1D,  0,    0,    0 },
	{ imSBB,  rm8,   imm8,  none,  0x80,  3,    0,    E_Ext },
	{ imSBB,  rm16,  imm16, none,  0x81,  3,    0,    E_16 | E_Ext },
	{ imSBB,  rm32,  imm32, none,  0x81,  3,    0,    E_Ext },
	{ imSBB,  rm16,  imm8S, none,  0x83,  3,    0,    E_16 | E_Ext },
	{ imSBB,  rm32,  imm8S, none,  0x83,  3,    0,    E_Ext },

	{ imAND,  rm8,   r8,    none,  0x20,  0,    0,    0 },
	{ imAND,  rm16,  r16,   none,  0x21,  0,    0,    E_16 },
	{ imAND,  rm32,  r32,   none,  0x21,  0,    0,    0 },
	{ imAND,  r8,    rm8,   none,  0x22,  0,    0,    0 },
	{ imAND,  r16,   rm16,  none,  0x23,  0,    0,    E_16 },
	{ imAND,  r32,   rm32,  none,  0x23,  0,    0,    0 },
	{ imAND,  rAL,   imm8,  none,  0x24,  0,    0,    0 },
	{ imAND,  rAX,   imm16, none,  0x25,  0,    0,    E_16 },
	{ imAND,  rEAX,  imm32, none,  0x25,  0,    0,    0 },
	{ imAND,  rm8,   imm8,  none,  0x80,  4,    0,    E_Ext },
	{ imAND,  rm16,  imm16, none,  0x81,  4,    0,    E_16 | E_Ext },
	{ imAND,  rm32,  imm32, none,  0x81,  4,    0,    E_Ext },
	{ imAND,  rm16,  imm8S, none,  0x83,  4,    0,    E_16 | E_Ext },
	{ imAND,  rm32,  imm8S, none,  0x83,  4,    0,    E_Ext },

	{ imNOT,  rm8,   none,  none,  0xF6,  2,    0,    E_Ext },
	{ imNOT,  rm16,  none,  none,  0xF7,  2,    0,    E_16 | E_Ext },
	{ imNOT,  rm32,  none,  none,  0xF7,  2,    0,    E_Ext },

	{ imSUB,  rm8,   r8,    none,  0x28,  0,    0,    0 },
	{ imSUB,  rm16,  r16,   none,  0x29,  0,    0,    E_16 },
	{ imSUB,  rm32,  r32,   none,  0x29,  0,    0,    0 },
	{ imSUB,  r8,    rm8,   none,  0x2A,  0,    0,    0 },
	{ imSUB,  r16,   rm16,  none,  0x2B,  0,    0,    E_16 },
	{ imSUB,  r32,   rm32,  none,  0x2B,  0,    0,    0 },
	{ imSUB,  rAL,   imm8,  none,  0x2C,  0,    0,    0 },
	{ imSUB,  rAX,   imm16, none,  0x2D,  0,    0,    E_16 },
	{ imSUB,  rEAX,  imm32, none,  0x2D,  0,    0,    0 },
	{ imSUB,  rm16,  imm16, none,  0x83,  0,    0,    E_16 },
	{ imSUB,  rm8,   imm8,  none,  0x80,  5,    0,    E_Ext },
	{ imSUB,  rm16,  imm16, none,  0x81,  5,    0,    E_16 | E_Ext },
	{ imSUB,  rm32,  imm32, none,  0x81,  5,    0,    E_Ext },
	{ imSUB,  rm16,  imm8S, none,  0x83,  5,    0,    E_16 | E_Ext },
	{ imSUB,  rm32,  imm8S, none,  0x83,  5,    0,    E_Ext },

	{ imXOR,  rm8,   r8,    none,  0x30,  0,    0,    0 },
	{ imXOR,  rm16,  r16,   none,  0x31,  0,    0,    E_16 },
	{ imXOR,  rm32,  r32,   none,  0x31,  0,    0,    0 },
	{ imXOR,  r8,    rm8,   none,  0x32,  0,    0,    0 },
	{ imXOR,  r16,   rm16,  none,  0x33,  0,    0,    E_16 },
	{ imXOR,  r32,   rm32,  none,  0x33,  0,    0,    0 },
	{ imXOR,  rAL,   imm8,  none,  0x34,  0,    0,    0 },
	{ imXOR,  rAX,   imm16, none,  0x35,  0,    0,    E_16 },
	{ imXOR,  rEAX,  imm32, none,  0x35,  0,    0,    0 },
	{ imXOR,  rm8,   imm8,  none,  0x80,  6,    0,    E_Ext },
	{ imXOR,  rm16,  imm16, none,  0x81,  6,    0,    E_16 | E_Ext },
	{ imXOR,  rm32,  imm32, none,  0x81,  6,    0,    E_Ext },
	{ imXOR,  rm16,  imm8S, none,  0x83,  6,    0,    E_16 | E_Ext },
	{ imXOR,  rm32,  imm8S, none,  0x83,  6,    0,    E_Ext },

	{ imCMP,  rm8,   r8,    none,  0x38,  0,    0,    0 },
	{ imCMP,  rm16,  r16,   none,  0x39,  0,    0,    E_16 },
	{ imCMP,  rm32,  r32,   none,  0x39,  0,    0,    0 },
	{ imCMP,  r8,    rm8,   none,  0x3A,  0,    0,    0 },
	{ imCMP,  r16,   rm16,  none,  0x3B,  0,    0,    E_16 },
	{ imCMP,  r32,   rm32,  none,  0x3B,  0,    0,    0 },
	{ imCMP,  rAL,   imm8,  none,  0x3C,  0,    0,    0 },
	{ imCMP,  rAX,   imm16, none,  0x3D,  0,    0,    E_16 },
	{ imCMP,  rEAX,  imm32, none,  0x3D,  0,    0,    0 },
	{ imCMP,  rm8,   imm8,  none,  0x80,  7,    0,    E_Ext },
	{ imCMP,  rm16,  imm16, none,  0x81,  7,    0,    E_16 | E_Ext },
	{ imCMP,  rm32,  imm32, none,  0x81,  7,    0,    E_Ext },
	{ imCMP,  rm16,  imm8S, none,  0x83,  7,    0,    E_16 | E_Ext },
	{ imCMP,  rm32,  imm8S, none,  0x83,  7,    0,    E_Ext },

	{ imTEST, rm8,   r8,    none,  0x84,  0,    0,    0 },
	{ imTEST, rm16,  r16,   none,  0x85,  0,    0,    E_16 },
	{ imTEST, rm32,  r32,   none,  0x85,  0,    0,    0 },
// These were incorrectly set as A-to-register comparisons before,
// and generated bad code.  They don't seem to be needed by current code.
// Commenting out until they are needed and can be tested since I'm unsure
// of the encodings. 
//	{ imTEST, rAL,   imm8,    none,  0xA8,  0,    0,    0 },    
///	{ imTEST, rAX,   imm16,   none,  0xA9,  0,    0,    E_16 | E_RExt },
//	{ imTEST, rEAX,  imm32,   none,  0xA9,  0,    0,    E_RExt },  
	{ imTEST, rm8,   imm8,  none,  0xF6,  0,    0,    E_Ext },
	{ imTEST, rm16,  imm16, none,  0xF7,  0,    0,    E_16 | E_Ext },
	{ imTEST, rm32,  imm32, none,  0xF7,  0,    0,    E_Ext },

	{ imXCHG, rm8,   r8,    none,  0x86,  0,    0,    0 },
	{ imXCHG, rm16,  r16,   none,  0x87,  0,    0,    0 },
	{ imXCHG, rm32,  r32,   none,  0x87,  0,    0,    0 },
	{ imXCHG, r8,    rm8,   none,  0x86,  0,    0,    0 },
	{ imXCHG, r16,   rm16,  none,  0x87,  0,    0,    0 },
	{ imXCHG, r32,   rm32,  none,  0x87,  0,    0,    0 },
	{ imXCHG, rEAX,  rECX,  none,  0x91,  0,    0,    0 },
	{ imXCHG, rAX,   rCX,   none,  0x91,  0,    0,    E_16 },
	{ imXCHG, rECX,  rEAX,  none,  0x91,  0,    0,    0 },
	{ imXCHG, rCX,   rAX,   none,  0x91,  0,    0,    E_16 },
	{ imXCHG, rEAX,  rEDX,  none,  0x92,  0,    0,    0 },
	{ imXCHG, rAX,   rDX,   none,  0x92,  0,    0,    E_16 },
	{ imXCHG, rEDX,  rEAX,  none,  0x92,  0,    0,    0 },
	{ imXCHG, rDX,   rAX,   none,  0x92,  0,    0,    E_16 },
	{ imXCHG, rEAX,  rEBX,  none,  0x93,  0,    0,    0 },
	{ imXCHG, rAX,   rBX,   none,  0x93,  0,    0,    E_16 },
	{ imXCHG, rEBX,  rEAX,  none,  0x93,  0,    0,    0 },
	{ imXCHG, rBX,   rAX,   none,  0x93,  0,    0,    E_16 },
	{ imXCHG, rEAX,  rESP,  none,  0x94,  0,    0,    0 },
	{ imXCHG, rAX,   rSP,   none,  0x94,  0,    0,    E_16 },
	{ imXCHG, rESP,  rEAX,  none,  0x94,  0,    0,    0 },
	{ imXCHG, rSP,   rAX,   none,  0x94,  0,    0,    E_16 },
	{ imXCHG, rEAX,  rEBP,  none,  0x95,  0,    0,    0 },
	{ imXCHG, rAX,   rBP,   none,  0x95,  0,    0,    E_16 },
	{ imXCHG, rEBP,  rEAX,  none,  0x95,  0,    0,    0 },
	{ imXCHG, rBP,   rAX,   none,  0x95,  0,    0,    E_16 },
	{ imXCHG, rEAX,  rESI,  none,  0x96,  0,    0,    0 },
	{ imXCHG, rAX,   rSI,   none,  0x96,  0,    0,    E_16 },
	{ imXCHG, rESI,  rEAX,  none,  0x96,  0,    0,    0 },
	{ imXCHG, rSI,   rAX,   none,  0x96,  0,    0,    E_16 },
	{ imXCHG, rEAX,  rEDI,  none,  0x97,  0,    0,    0 },
	{ imXCHG, rAX,   rDI,   none,  0x97,  0,    0,    E_16 },
	{ imXCHG, rEDI,  rEAX,  none,  0x97,  0,    0,    0 },
	{ imXCHG, rDI,   rAX,   none,  0x97,  0,    0,    E_16 },

	{ imINC,  r16,   none,  none,  0x40,  0,    0,    E_R | E_16 },
	{ imINC,  r32,   none,  none,  0x40,  0,    0,    E_R },
	{ imINC,  rm8,   none,  none,  0xFE,  0,    0,    E_Ext },
	{ imINC,  rm16,  none,  none,  0xFF,  0,    0,    E_Ext | E_16 },
	{ imINC,  rm32,  none,  none,  0xFF,  0,    0,    E_Ext },

	{ imDEC,  r16,   none,  none,  0x48,  0,    0,    E_R | E_16 },
	{ imDEC,  r32,   none,  none,  0x48,  0,    0,    E_R },
	{ imDEC,  rm8,   none,  none,  0xFE,  1,    0,    E_Ext },
	{ imDEC,  rm16,  none,  none,  0xFF,  1,    0,    E_Ext | E_16 },
	{ imDEC,  rm32,  none,  none,  0xFF,  1,    0,    E_Ext },

	{ imIMUL, rm8,   none,  none,  0xF6,  5,    0,    E_Ext },
	{ imIMUL, rm16,  none,  none,  0xF7,  5,    0,    E_Ext | E_16 },
	{ imIMUL, rm32,  none,  none,  0xF7,  5,    0,    E_Ext },
	{ imIMUL, r16,   rm16,  none,  0xAF,  0,    0,    E_TwoByte | E_16 },
	{ imIMUL, r32,   rm32,  none,  0xAF,  0,    0,    E_TwoByte },
	{ imIMUL, r16,   rm16,  imm16, 0x69,  0,    0,    E_16 },
	{ imIMUL, r32,   rm32,  imm32, 0x69,  0,    0,    0 },
	{ imIMUL, r16,   rm16,  imm8S, 0x6B,  0,    0,    E_16 },
	{ imIMUL, r32,   rm32,  imm8S, 0x6B,  0,    0,    0 },
	{ imIMUL, r16,   imm16, none,  0x69,  0,    0,    E_16 | E_ImpR | E_RExt },
	{ imIMUL, r32,   imm32, none,  0x69,  0,    0,    E_ImpR | E_RExt },
	{ imIMUL, r16,   imm8S, none,  0x6B,  0,    0,    E_16 | E_ImpR | E_RExt },
	{ imIMUL, r32,   imm8S, none,  0x6B,  0,    0,    E_ImpR | E_RExt },

	{ imMUL,  rm8,   none,  none,  0xF6,  4,    0,    E_Ext },
	{ imMUL,  rm16,  none,  none,  0xF7,  4,    0,    E_16 | E_Ext },
	{ imMUL,  rm32,  none,  none,  0xF7,  4,    0,    E_Ext },

	{ imDIV,  rm8,   none,  none,  0xF6,  6,    0,    E_Ext },
	{ imDIV,  rm16,  none,  none,  0xF7,  6,    0,    E_16 | E_Ext },
	{ imDIV,  rm8,   none,  none,  0xF7,  6,    0,    E_Ext },

	{ imIDIV, rm8,   none,  none,  0xF6,  7,    0,    E_Ext },
	{ imIDIV, rm16,  none,  none,  0xF7,  7,    0,    E_16 | E_Ext },
	{ imIDIV, rm8,   none,  none,  0xF7,  7,    0,    E_Ext },

	{ imJMP,  ofs8,  none,  none,  0xEB,  0,    0,    0 },
	{ imJMP,  ofs32, none,  none,  0xE9,  0,    0,    0 },
	{ imJMP,  rm16,  none,  none,  0xFF,  4,    0,    E_16 | E_Ext },
	{ imJMP,  rm32,  none,  none,  0xFF,  4,    0,    E_Ext },
	{ imJO,   ofs8,  none,  none,  0x70,  0,    0,    0 },
	{ imJO,   ofs32, none,  none,  0x80,  0,    0,    E_TwoByte },
	{ imJNO,  ofs8,  none,  none,  0x71,  0,    0,    0 },
	{ imJNO,  ofs32, none,  none,  0x81,  0,    0,    E_TwoByte },
	{ imJB,   ofs8,  none,  none,  0x72,  0,    0,    0 },
	{ imJB,   ofs32, none,  none,  0x82,  0,    0,    E_TwoByte },
	{ imJNB,  ofs8,  none,  none,  0x73,  0,    0,    0 },
	{ imJNB,  ofs32, none,  none,  0x83,  0,    0,    E_TwoByte },
	{ imJZ,   ofs8,  none,  none,  0x74,  0,    0,    0 },
	{ imJZ,   ofs32, none,  none,  0x84,  0,    0,    E_TwoByte },
	{ imJNZ,  ofs8,  none,  none,  0x75,  0,    0,    0 },
	{ imJNZ,  ofs32, none,  none,  0x85,  0,    0,    E_TwoByte },
	{ imJBE,  ofs8,  none,  none,  0x76,  0,    0,    0 },
	{ imJBE,  ofs32, none,  none,  0x86,  0,    0,    E_TwoByte },
	{ imJA,   ofs8,  none,  none,  0x77,  0,    0,    0 },
	{ imJA,   ofs32, none,  none,  0x87,  0,    0,    E_TwoByte },
	{ imJS,   ofs8,  none,  none,  0x78,  0,    0,    0 },
	{ imJS,   ofs32, none,  none,  0x88,  0,    0,    E_TwoByte },
	{ imJNS,  ofs8,  none,  none,  0x79,  0,    0,    0 },
	{ imJNS,  ofs32, none,  none,  0x89,  0,    0,    E_TwoByte },
	{ imJP,   ofs8,  none,  none,  0x7A,  0,    0,    0 },
	{ imJP,   ofs32, none,  none,  0x8A,  0,    0,    E_TwoByte },
	{ imJNP,  ofs8,  none,  none,  0x7B,  0,    0,    0 },
	{ imJNP,  ofs32, none,  none,  0x8B,  0,    0,    E_TwoByte },
	{ imJL,   ofs8,  none,  none,  0x7C,  0,    0,    0 },
	{ imJL,   ofs32, none,  none,  0x8C,  0,    0,    E_TwoByte },
	{ imJGE,  ofs8,  none,  none,  0x7D,  0,    0,    0 },
	{ imJGE,  ofs32, none,  none,  0x8D,  0,    0,    E_TwoByte },
	{ imJLE,  ofs8,  none,  none,  0x7E,  0,    0,    0 },
	{ imJLE,  ofs32, none,  none,  0x8E,  0,    0,    E_TwoByte },
	{ imJG,   ofs8,  none,  none,  0x7F,  0,    0,    0 },
	{ imJG,   ofs32, none,  none,  0x8F,  0,    0,    E_TwoByte },

	{ imLOOPNZ,ofs8, none,  none,  0xE0,  0,    0,    0 },
	{ imLOOPZ,ofs8,  none,  none,  0xE1,  0,    0,    0 },
	{ imLOOP, ofs8,  none,  none,  0xE2,  0,    0,    0 },
	{ imJECXZ,ofs8,  none,  none,  0xE3,  0,    0,    0 },
	{ imJCXZ, ofs8,  none,  none,  0xE3,  0,    0,    E_16 },

	{ imCALL, ofs32, none,  none,  0xE8,  0,    0,    0 },
	{ imENTER,imm16, imm8,  none,  0xC8,  0,    0,    0 },
	{ imLEAVE,none,  none,  none,  0xC9,  0,    0,    0 },
	{ imRETF, imm16, none,  none,  0xCA,  0,    0,    0 },
	{ imRETF, none,  none,  none,  0xCB,  0,    0,    0 },
	{ imRETN, imm16, none,  none,  0xC2,  0,    0,    0 },
	{ imRETN, none,  none,  none,  0xC3,  0,    0,    0 },

	{ imINT,  imm_3, none,  none,  0xCC,  0,    0,    0 },
	{ imINT,  imm8,  none,  none,  0xCD,  0,    0,    0 },
	{ imINTO, none,  none,  none,  0xCE,  0,    0,    0 },
	{ imIRET, none,  none,  none,  0xCF,  0,    0,    0 },

	{ imPUSH, rES,   none,  none,  0x06,  0,    0,    0 },
	{ imPUSH, rCS,   none,  none,  0x0E,  0,    0,    0 },
	{ imPUSH, rSS,   none,  none,  0x16,  0,    0,    0 },
	{ imPUSH, rDS,   none,  none,  0x1E,  0,    0,    0 },
	{ imPUSH, rFS,   none,  none,  0xA0,  0,    0,    E_TwoByte },
	{ imPUSH, rGS,   none,  none,  0xA8,  0,    0,    E_TwoByte },
	{ imPUSH, imm32, none,  none,  0x68,  0,    0,    0 },
	{ imPUSH, imm8S, none,  none,  0x6A,  0,    0,    0 },
	{ imPUSH, r16,   none,  none,  0x50,  0,    0,    E_R | E_16 },
	{ imPUSH, r32,   none,  none,  0x50,  0,    0,    E_R },
	{ imPUSH, rm16,  none,  none,  0xFF,  6,    0,    E_16 | E_Ext },
	{ imPUSH, rm32,  none,  none,  0xFF,  6,    0,    E_Ext },

	{ imPOP,  rm16,  none,  none,  0x8f,  0,    0,    E_16 },
	{ imPOP,  rm32,  none,  none,  0x8f,  0,    0,    0 },
	{ imPOP,  rES,   none,  none,  0x07,  0,    0,    0 },
	{ imPOP,  rSS,   none,  none,  0x17,  0,    0,    0 },
	{ imPOP,  rDS,   none,  none,  0x1F,  0,    0,    0 },
	{ imPOP,  r16,   none,  none,  0x58,  0,    0,    E_R | E_16 },
	{ imPOP,  r32,   none,  none,  0x58,  0,    0,    E_R },

	{ imPUSHA,none,  none,  none,  0x60,  0,    0,    0 },
	{ imPUSHF,none,  none,  none,  0x9C,  0,    0,    0 },
	{ imPOPA, none,  none,  none,  0x61,  0,    0,    0 },
	{ imPOPF, none,  none,  none,  0x9D,  0,    0,    0 },
	{ imSAHF, none,  none,  none,  0x9E,  0,    0,    0 },
	{ imLAHF, none,  none,  none,  0x9F,  0,    0,    0 },

	{ imSETA, rm8,   none,  none,  0x97,  0,    0,    E_TwoByte },
	{ imSETBE,rm8,   none,  none,  0x96,  0,    0,    E_TwoByte },
	{ imSETC, rm8,   none,  none,  0x92,  0,    0,    E_TwoByte },
	{ imSETZ, rm8,   none,  none,  0x94,  0,    0,    E_TwoByte },
	{ imSETG, rm8,   none,  none,  0x9F,  0,    0,    E_TwoByte },
	{ imSETGE,rm8,   none,  none,  0x9D,  0,    0,    E_TwoByte },
	{ imSETL, rm8,   none,  none,  0x9C,  0,    0,    E_TwoByte },
	{ imSETLE,rm8,   none,  none,  0x9E,  0,    0,    E_TwoByte },
	{ imSETNC,rm8,   none,  none,  0x93,  0,    0,    E_TwoByte },
	{ imSETNO,rm8,   none,  none,  0x91,  0,    0,    E_TwoByte },
	{ imSETNP,rm8,   none,  none,  0x9B,  0,    0,    E_TwoByte },
	{ imSETNS,rm8,   none,  none,  0x99,  0,    0,    E_TwoByte },
	{ imSETNZ,rm8,   none,  none,  0x95,  0,    0,    E_TwoByte },
	{ imSETO, rm8,   none,  none,  0x90,  0,    0,    E_TwoByte },
	{ imSETP, rm8,   none,  none,  0x9A,  0,    0,    E_TwoByte },
	{ imSETS, rm8,   none,  none,  0x98,  0,    0,    E_TwoByte },

	{ imROL,  rm8,   imm_1, none,   0xD0, 0,    0,    E_Ext },
	{ imROL,  rm8,   rCL,   none,   0xD2, 0,    0,    E_Ext },
	{ imROL,  rm8,   imm8,  none,   0xC0, 0,    0,    E_Ext },
	{ imROL,  rm16,  imm_1, none,   0xD1, 0,    0,    E_Ext | E_16 },
	{ imROL,  rm16,  rCL,   none,   0xD3, 0,    0,    E_Ext | E_16 },
	{ imROL,  rm16,  imm8,  none,   0xC1, 0,    0,    E_Ext | E_16 },
	{ imROL,  rm32,  imm_1, none,   0xD1, 0,    0,    E_Ext },
	{ imROL,  rm32,  rCL,   none,   0xD3, 0,    0,    E_Ext },
	{ imROL,  rm32,  imm8,  none,   0xC1, 0,    0,    E_Ext },

	{ imROR,  rm8,   imm_1, none,   0xD0, 1,    0,    E_Ext },
	{ imROR,  rm8,   rCL,   none,   0xD2, 1,    0,    E_Ext },
	{ imROR,  rm8,   imm8,  none,   0xC0, 1,    0,    E_Ext },
	{ imROR,  rm16,  imm_1, none,   0xD1, 1,    0,    E_Ext | E_16 },
	{ imROR,  rm16,  rCL,   none,   0xD3, 1,    0,    E_Ext | E_16 },
	{ imROR,  rm16,  imm8,  none,   0xC1, 1,    0,    E_Ext | E_16 },
	{ imROR,  rm32,  imm_1, none,   0xD1, 1,    0,    E_Ext },
	{ imROR,  rm32,  rCL,   none,   0xD3, 1,    0,    E_Ext },
	{ imROR,  rm32,  imm8,  none,   0xC1, 1,    0,    E_Ext },

	{ imRCL,  rm8,   imm_1, none,   0xD0, 2,    0,    E_Ext },
	{ imRCL,  rm8,   rCL,   none,   0xD2, 2,    0,    E_Ext },
	{ imRCL,  rm8,   imm8,  none,   0xC0, 2,    0,    E_Ext },
	{ imRCL,  rm16,  imm_1, none,   0xD1, 2,    0,    E_Ext | E_16 },
	{ imRCL,  rm16,  rCL,   none,   0xD3, 2,    0,    E_Ext | E_16 },
	{ imRCL,  rm16,  imm8,  none,   0xC1, 2,    0,    E_Ext | E_16 },
	{ imRCL,  rm32,  imm_1, none,   0xD1, 2,    0,    E_Ext },
	{ imRCL,  rm32,  rCL,   none,   0xD3, 2,    0,    E_Ext },
	{ imRCL,  rm32,  imm8,  none,   0xC1, 2,    0,    E_Ext },

	{ imRCR,  rm8,   imm_1, none,   0xD0, 3,    0,    E_Ext },
	{ imRCR,  rm8,   rCL,   none,   0xD2, 3,    0,    E_Ext },
	{ imRCR,  rm8,   imm8,  none,   0xC0, 3,    0,    E_Ext },
	{ imRCR,  rm16,  imm_1, none,   0xD1, 3,    0,    E_Ext | E_16 },
	{ imRCR,  rm16,  rCL,   none,   0xD3, 3,    0,    E_Ext | E_16 },
	{ imRCR,  rm16,  imm8,  none,   0xC1, 3,    0,    E_Ext | E_16 },
	{ imRCR,  rm32,  imm_1, none,   0xD1, 3,    0,    E_Ext },
	{ imRCR,  rm32,  rCL,   none,   0xD3, 3,    0,    E_Ext },
	{ imRCR,  rm32,  imm8,  none,   0xC1, 3,    0,    E_Ext },

	{ imSHL,  rm8,   imm_1, none,   0xD0, 4,    0,    E_Ext },
	{ imSHL,  rm8,   rCL,   none,   0xD2, 4,    0,    E_Ext },
	{ imSHL,  rm8,   imm8,  none,   0xC0, 4,    0,    E_Ext },
	{ imSHL,  rm16,  imm_1, none,   0xD1, 4,    0,    E_Ext | E_16 },
	{ imSHL,  rm16,  rCL,   none,   0xD3, 4,    0,    E_Ext | E_16 },
	{ imSHL,  rm16,  imm8,  none,   0xC1, 4,    0,    E_Ext | E_16 },
	{ imSHL,  rm32,  imm_1, none,   0xD1, 4,    0,    E_Ext },
	{ imSHL,  rm32,  rCL,   none,   0xD3, 4,    0,    E_Ext },
	{ imSHL,  rm32,  imm8,  none,   0xC1, 4,    0,    E_Ext },

	{ imSHR,  rm8,   imm_1, none,   0xD0, 5,    0,    E_Ext },
	{ imSHR,  rm8,   rCL,   none,   0xD2, 5,    0,    E_Ext },
	{ imSHR,  rm8,   imm8,  none,   0xC0, 5,    0,    E_Ext },
	{ imSHR,  rm16,  imm_1, none,   0xD1, 5,    0,    E_Ext | E_16 },
	{ imSHR,  rm16,  rCL,   none,   0xD3, 5,    0,    E_Ext | E_16 },
	{ imSHR,  rm16,  imm8,  none,   0xC1, 5,    0,    E_Ext | E_16 },
	{ imSHR,  rm32,  imm_1, none,   0xD1, 5,    0,    E_Ext },
	{ imSHR,  rm32,  rCL,   none,   0xD3, 5,    0,    E_Ext },
	{ imSHR,  rm32,  imm8,  none,   0xC1, 5,    0,    E_Ext },

	{ imSAR,  rm8,   imm_1, none,   0xD0, 7,    0,    E_Ext },
	{ imSAR,  rm8,   rCL,   none,   0xD2, 7,    0,    E_Ext },
	{ imSAR,  rm8,   imm8,  none,   0xC0, 7,    0,    E_Ext },
	{ imSAR,  rm16,  imm_1, none,   0xD1, 7,    0,    E_Ext | E_16 },
	{ imSAR,  rm16,  rCL,   none,   0xD3, 7,    0,    E_Ext | E_16 },
	{ imSAR,  rm16,  imm8,  none,   0xC1, 7,    0,    E_Ext | E_16 },
	{ imSAR,  rm32,  imm_1, none,   0xD1, 7,    0,    E_Ext },
	{ imSAR,  rm32,  rCL,   none,   0xD3, 7,    0,    E_Ext },
	{ imSAR,  rm32,  imm8,  none,   0xC1, 7,    0,    E_Ext },

	{ imCLC,  none,  none,  none,   0xF8, 0,    0,    0 },
	{ imSTC,  none,  none,  none,   0xF9, 0,    0,    0 },
	{ imCMC,  none,  none,  none,   0xF5, 0,    0,    0 },
	{ imCLD,  none,  none,  none,   0xFC, 0,    0,    0 },
	{ imSTD,  none,  none,  none,   0xFD, 0,    0,    0 },

	{ imCBW,  none,  none,  none,   0x98, 0,    0,    E_16 },
	{ imCWDE, none,  none,  none,   0x98, 0,    0,    0 },
	{ imCWD,  none,  none,  none,   0x99, 0,    0,    E_16 },
	{ imCDQ,  none,  none,  none,   0x99, 0,    0,    0 },

	{ imNOP,  none,  none,  none,  0x90,  0,    0,    0 }
};

// figure the encoded length of an op table entry, excluding the SIB and displacement
static int immlen(optype t)
{
	switch (t)
	{
	case imm8:   return 1;
	case imm8S:  return 1;
	case imm16:  return 2;
	case imm32:  return 4;
	case ofs8:   return 1;
	case ofs16:  return 2;
	case ofs32:  return 4;
	default:     return 0;
	}
}
static int oplen(const struct mnedef *o)
{
	// start with one byte for the opcode
	int len = 1;

	// if it has a TWOBYTE prefix, that adds a byte
	if (o->flags & E_TwoByte)
		++len;

	// if it requires an OPSIZE prefix, that adds a byte
	if (o->flags & E_16)
		++len;

	// if it has a MODrm field, that adds a byte (note that op3 is never a MODrm)
	if (o->op1 == rm8 || o->op1 == rm16 || o->op1 == rm32
		|| o->op2 == rm8 || o->op2 == rm16 || o->op2 == rm32)
		++len;

	// add the lengths of the immediates encoded in the instruction
	len += immlen(o->op1) + immlen(o->op2) + immlen(o->op3);

	// return the result
	return len;
}

// Actual operand descriptor.  This describes one operand from an
// instruction to encode, for matching against the opcode template table.
enum opdesctyp {
	opNone,        // no operand at this position
	opReg8,        // an 8-bit general-purpose register; reg is the register number (0=AL, 1=CL, etc)
	opReg16,       // a 16-bit general-purpose register; reg is the register number (0=AX, 1=CX, etc)
	opReg32,       // a 32-bit general-purpose register; reg is the register number (0=EAX, 1=ECX, etc)
	opSegReg,      // a segment register or IP; reg is the register number
	opMem,         // a memory reference of indeterminate size; the mem substructure contains the encoded reference
	opMem8,        //    a BYTE PTR memory reference
	opMem16,       //    a WORD PTR memory reference
	opMem32,       //    a DWORD PTR memory reference
	opImm,         // an immediate value; imm is the immediate
	opOfs,         // an offset value; ofs is the offset
	opLbl          // a label; lbl is the label pointer
};
	
typedef enum opdesctyp opdesctyp;
struct opdesc {
	opdesctyp typ;
	union {
		byte     reg;     // register number (0-7, using Intel opcode encoding for the various reg files)
		UINT32   imm;     // immediate value
		INT32    ofs;     // an offset value (for a jump or call)
		struct jit_label *lbl;  // a label (for a jump or call)
		struct {
			byte   MODrm;   // MOD R/M byte
			byte   sib;     // SIB byte, if applicable
			UINT32 disp;    // displacement, if applicable
			int    hasSib;  // true if there's a SIB byte
			int    dispLen; // length in bytes of the displacement value
		} mem;
	} val;
};

// Match an operand descriptor to an operand field in an mnedef template
static int match_operand(optype t, struct opdesc *op)
{
	switch (t)
	{
	case none:  return op->typ == opNone;
	case r8:	return op->typ == opReg8;
	case r16:   return op->typ == opReg16;
	case r32:   return op->typ == opReg32;
	case rseg:  return op->typ == opSegReg;
	case rm8:   return op->typ == opMem || op->typ == opMem8 || op->typ == opReg8;
	case rm16:  return op->typ == opMem || op->typ == opMem16 || op->typ == opReg16;
	case rm32:  return op->typ == opMem || op->typ == opMem32 || op->typ == opReg32;
	case imm8:  return op->typ == opImm && op->val.imm <= 0xFF;
	case imm8S: return op->typ == opImm && op->val.imm <= 0x7F;
	case imm16: return op->typ == opImm && op->val.imm <= 0xFFFF;
	case imm32: return op->typ == opImm;
	case ofs8:  return (op->typ == opOfs && op->val.ofs >= -128 && op->val.ofs <= 127) || op->typ == opLbl;
	case ofs16: return (op->typ == opOfs && op->val.ofs >= -32768 && op->val.ofs <= 32767) || op->typ == opLbl;
	case ofs32: return op->typ == opOfs || op->typ == opLbl;
	case rAL:   return op->typ == opReg8 && op->val.reg == rrr(AL);
	case rCL:   return op->typ == opReg8 && op->val.reg == rrr(CL);
	case rEAX:  return op->typ == opReg32 && op->val.reg == rrr(EAX);
	case rEBX:  return op->typ == opReg32 && op->val.reg == rrr(EBX);
	case rECX:  return op->typ == opReg32 && op->val.reg == rrr(ECX);
	case rEDX:  return op->typ == opReg32 && op->val.reg == rrr(EDX);
	case rESP:  return op->typ == opReg32 && op->val.reg == rrr(ESP);
	case rEBP:  return op->typ == opReg32 && op->val.reg == rrr(EBP);
	case rESI:  return op->typ == opReg32 && op->val.reg == rrr(ESI);
	case rEDI:  return op->typ == opReg32 && op->val.reg == rrr(EDI);
	case rDS:   return op->typ == opSegReg && op->val.reg == rrr(DS);
	case rES:   return op->typ == opSegReg && op->val.reg == rrr(ES);
	case rSS:   return op->typ == opSegReg && op->val.reg == rrr(SS);
	case rCS:   return op->typ == opSegReg && op->val.reg == rrr(CS);
	case rFS:   return op->typ == opSegReg && op->val.reg == rrr(FS);
	case rGS:   return op->typ == opSegReg && op->val.reg == rrr(GS);
	case imm_1: return op->typ == opImm && op->val.imm == 1;
	case imm_3: return op->typ == opImm && op->val.imm == 3;
	default:    return 0;
	}
}

// Encode the MOD REG R/M byte for an instruction.  'rm' is the operand to encode
// into the MOD RM fields, and 'r' is the operand to encode in the REG field.
static byte *encode_modrm(byte *p, const struct mnedef *o, const struct opdesc *rm, const struct opdesc *r)
{
	// Start with the memory operand
	if (rm->typ == opMem || rm->typ == opMem8 || rm->typ == opMem16 || rm->typ == opMem32)
	{
		// The mem operand is already encoded as MOD R/M, so start with it
		*p = rm->val.mem.MODrm;
	}
	else if (rm->typ == opReg8 || rm->typ == opReg16 || rm->typ == opReg32)
	{
		// The mem operand is a register, so encode it in MOD RM format as 11 000 rr.
		// Note that we don't have to test type compatibility here, since we've
		// already guaranteed that by finding a valid match in the opcode table.
		*p = 0xC0 | rm->val.reg;
	}
	else
	{
		// other operand types are invalid here
		ASSERT(0);
	}

	// If we have a register operand, encode the register in the REG field (xx REG xxx).
	if (o->flags & E_Ext)
	{
		*p |= o->ext << 3;
	}
	else if (r->typ == opReg8 || r->typ == opReg16 || r->typ == opReg32)
	{
		*p |= r->val.reg << 3;
	}
	else if (r->typ == opNone && o->op2 == none)
	{
		// this opcode takes a single r/m operand, so leave the rrr field empty (000)
	}
	else
	{
		// anything else is invalid
		ASSERT(0);
	}

	// done with the MOD REG R/M byte
	++p;

	if (rm->typ == opMem || rm->typ == opMem8 || rm->typ == opMem16 || rm->typ == opMem32)
	{
		// add the SIB byte if present
		if (rm->val.mem.hasSib)
			*p++ = rm->val.mem.sib;
		
		// add the displacement if present
		if (rm->val.mem.dispLen != 0) {
			memcpy(p, &rm->val.mem.disp, rm->val.mem.dispLen);
			p += rm->val.mem.dispLen;
		}
	}

	// return the updated output pointer
	return p;
}

// encode an immediate operand
static byte *encode_immediate(byte *p, optype t, struct opdesc *op)
{
	switch (t)
	{
	case imm8:
	case imm8S:
		ASSERT(op->typ == opImm && op->val.imm <= 0xFF);
		*p = (byte)op->val.imm;
		return p + 1;

	case imm16:
		ASSERT(op->typ == opImm && op->val.imm <= 0xFFFF);
		*(UINT16 *)p = (INT16)op->val.imm;
		return p + 2;

	case imm32:
		ASSERT(op->typ == opImm);
		*(UINT32 *)p = op->val.imm;
		return p + 4;

	case ofs8:
		if (op->typ == opOfs) {
			ASSERT(op->val.ofs >= -128 && op->val.ofs <= 127);
			*p = (byte)op->val.ofs;
			return p + 1;
		}
		else {
			ASSERT(op->typ == opLbl);
			*p = 0;
			return p + 1;
		}

	case ofs16:
		if (op->typ == opOfs) {
			ASSERT(op->val.ofs >= -32768 && op->val.ofs <= 32767);
			*(INT16 *)p = (INT16)op->val.ofs;
			return p + 2;
		}
		else {
			ASSERT(op->typ == opLbl);
			*(INT16 *)p = 0;
			return p + 2;
		}

	case ofs32:
		if (op->typ == opOfs) {
			*(INT32 *)p = (INT32)op->val.ofs;
			return p + 4;
		}
		else {
			ASSERT(op->typ == opLbl);
			*(INT32 *)p = 0;
			return p + 4;
		}

	default:
		// this slot doesn't hold an immediate value
		return p;
	}
}

static int mneIdx[nIntelMneId], mneIdxInit = 0;
static void initMneIdx()
{
	int i;
	intelMneId prv = nIntelMneId;

	// clear out the index table
	for (i = 0 ; i < nIntelMneId ; ++i)
		mneIdx[i] = -1;

	// build the index table
	mneIdxInit = 1;
	for (i = 0 ; i < sizeof(mnetab)/sizeof(mnetab[0]) ; ++i)
	{
		// if this is a new code, start this group
		intelMneId cur = mnetab[i].mne;
		if (cur != prv)
		{
			// integrity check: each group must be contiguous, so we must
			// not have an index entry for this group yet
			ASSERT(mneIdx[cur] == -1);

			// set the index to point to the first member of the group
			mneIdx[cur] = i;

			// note the new working section
			prv = cur;
		}
	}
}

// figure the inferred memory type based on a register operand
static opdesctyp inferMemType(opdesctyp m, struct opdesc *o)
{
	// If the memory size is indeterminate, and this operand is a register
	// type, set the memory size to the register size.
	if (m == opMem) {
		if (o->typ == opReg8)
			return opMem8;
		if (o->typ == opReg16)
			return opMem16;
		if (o->typ == opReg32)
			return opMem32;
	}

	// keep the existing type
	return m;
}
static void setInferredMemType(struct opdesc *o, opdesctyp t)
{
	// if the operand is of indeterminate memory type, set it to match
	// the size type of the other operand
	if (o->typ == opMem)
	{
		// If t is opMem, it means that we were unable to infer any definite
		// type from any operand.  In such cases it's not allowed to have a
		// memory operand of indetermine type, since the operand size is
		// ambiguous.  If this assert fails, it means that a size specifier
		// must be added to this memory type operand.  E.g., change this:
		//    emit(NOT, Idx, EAX)            // NOT [EAX] - ambiguous size
		// to this:
		//    emit(NOT, BytePtr, Idx, EAX)   // NOT BYTE PTR [EAX] 
		ASSERT(t != opMem);
		o->typ = t;
	}
}

void jit_emit(intelMneId mne, ...)
{
	va_list va;
	int n;
	struct opdesc op[3];
	int i;
	const struct mnedef *m, *mbest;
	byte buf[32], *p;
	struct instr *ins;
	opdesctyp memType = opNone;

	// initialize the opcode table if we haven't already
	if (!mneIdxInit)
		initMneIdx();

	// no operands yet
	op[0].typ = opNone;
	op[1].typ = opNone;
	op[2].typ = opNone;
	
	// parse each actual operand value from the arguments
	va_start(va, mne);
	for (n = 0 ; n < 3 ; ++n)
	{
		// read the next operand type; stop at end of operands
		int a = va_arg(va, int);
		if (a == EndOfOps)
			break;
		
		// assume that a memory operand will be of indetermine type
		memType = opMem;

		// Check for a BYTE PTR, WORD PTR, or DWORD PTR prefix for a memory type.
		if (a == BytePtr) {
			memType = opMem8;
			a = va_arg(va, int);
		}
		else if (a == WordPtr) {
			memType = opMem16;
			a = va_arg(va, int);
		}
		else if (a == DwordPtr) {
			memType = opMem32;
			a = va_arg(va, int);
		}

		// check what we have
		if (is_r8(a)) {
			op[n].typ = opReg8;
			op[n].val.reg = rrr(a);
			ASSERT(memType == opMem);
		}
		else if (is_r16(a)) {
			op[n].typ = opReg16;
			op[n].val.reg = rrr(a);
			ASSERT(memType == opMem);
		}
		else if (is_r32(a)) {
			op[n].typ = opReg32;
			op[n].val.reg = rrr(a);
			ASSERT(memType == opMem);
		}
		else if (is_segreg(a)) {
			op[n].typ = opSegReg;
			op[n].val.reg = rrr(a);
			ASSERT(memType == opMem);
		}
		else if (a == Imm) {
			op[n].typ = opImm;
			op[n].val.imm = va_arg(va, int);
			ASSERT(memType == opMem);
		}
		else if (a == Idx) {
			int r;

			// Indexed register with no displacement, as in [BX] - get the register,
			// and check that it's a 32-bit register (others can't be used in index
			// mode in 32-bit code).  Also, indexed immediate address, as [1234h].

			// presume a simple MOD REG RM encoding
			op[n].typ = memType;
			op[n].val.mem.hasSib = 0;
			op[n].val.mem.disp = 0;
			op[n].val.mem.dispLen = 0;

			// get the register or immediate value to index
			r = va_arg(va, int);
			if (r == Imm)
			{
				// indexing an immediate address - MOD REG RM = 00 xxx 101
				// (disp32 with no index register)
				op[n].val.mem.disp = va_arg(va, int);
				op[n].val.mem.dispLen = 4;
				op[n].val.mem.MODrm = 0x05;
			}
			else
			{
				// the index has to be a 32-bit register
				ASSERT(is_r32(r));
				
				// check for special cases: EBP and ESP need special encodings
				if (r == EBP) {
					// EBP requires the [EBP]+disp8 mode, with a zero displacement
					op[n].val.mem.MODrm = 0x45;
					op[n].val.mem.dispLen = 1;
				}
				else if (r == ESP) {
					// ESP requires SIB mode: 04 24
					op[n].val.mem.hasSib = 1;
					op[n].val.mem.MODrm = 0x04;
					op[n].val.mem.sib = 0x24;
				}
				else {
					// other 32-bit registers can be indexed directly with MOD REG RM = 00 000 rrr
					op[n].val.mem.MODrm = rrr(r);
				}
			}
		}
		else if (a == BaseDisp) {
			// base + displacement mode
			int r = va_arg(va, int);
			UINT32 disp = va_arg(va, int);

			ASSERT(is_r32(r));

			// figure the displacement size - use a byte if it fits, a dword otherwise
			op[n].val.mem.disp = disp;
			op[n].val.mem.dispLen = (disp <= 0xFF ? 1 : 4);

			op[n].typ = memType;
			op[n].val.mem.hasSib = 0;

			// check for registers that need special encodings
			if (r == ESP) {
				// requires SIB mode: 44 24 <disp8> for byte displacement, 84 24 <disp3> for dword displacement
				op[n].val.mem.hasSib = 1;
				op[n].val.mem.MODrm = (disp <= 0xFF ? 0x44 : 0x84);
				op[n].val.mem.sib = 0x24;
			}
			else {
				// for others, use 40|rrr <byte> or 80|rrr <dword> with no SIB
				op[n].val.mem.MODrm = (disp <= 0xFF ? 0x40 : 0x80) | rrr(r);
			}
		}
		else if (a == BaseIdx || a == BaseIdxDisp || a == IdxScaleDisp || a == BaseIdxScaleDisp) {
			// base + index, base + index + displacement, or base + index + displacement*scale modes.
			// These all require SIB mode.

			// Get the base register.
			// 
			// If this is index-scale-displacement mode, with no base register, the pseudo
			// base register for the SIB is EBP.  This gives us a SIB base register code of 5,
			// which usually means EBP, but it means "no base register" in the special case
			// where MOD=00.
			//
			// For all other modes, get the actual base register from the arguments.
			int rb = (a == IdxScaleDisp ? EBP : va_arg(va, int));

			// Get the index register - all of these modes specify an index register argument
			int ri = va_arg(va, int);

			// assume no scale (scale mode 00 -> multiplier is 1)
			int ss = 0;

			// base and index registers must be 32-bit
			ASSERT(is_r32(rb) && is_r32(ri));

			// set up the basic descriptor entries
			op[n].typ = memType;
			op[n].val.mem.MODrm = 0x04;  // assume we'll use a SIB with no displacement
			op[n].val.mem.hasSib = 1;
			op[n].val.mem.disp = 0;
			op[n].val.mem.dispLen = 0;

			// check if there's a scale
			if (a == IdxScaleDisp || a == BaseIdxScaleDisp) {
				int scale = va_arg(va, int);
				if (scale == 1)
					ss = 0x00;
				else if (scale == 2)
					ss = 0x40;
				else if (scale == 4)
					ss = 0x80;
				else if (scale == 8)
					ss = 0xC0;
				else
					ASSERT(0);
			}

			// check if there's a displacement as well
			if (a == BaseIdxDisp || a == IdxScaleDisp || a == BaseIdxScaleDisp) {
				op[n].val.mem.disp = va_arg(va, int);
				if (a == IdxScaleDisp) {
					// IdxScaleDisp mode - 04 SIB <dword>
					op[n].val.mem.MODrm = 0x44;
					op[n].val.mem.dispLen = 4;
				}
				else if (op[n].val.mem.disp <= 0xFF) {
					// Use an 8-bit displacement - 44 SIB <byte>.
					op[n].val.mem.MODrm = 0x44;
					op[n].val.mem.dispLen = 1;
				}
				else {
					// 32-bit displacement - 84 SIB <dword>
					op[n].val.mem.MODrm = 0x84;
					op[n].val.mem.dispLen = 4;
				}
			}

			// ESP can only be the base - swap them if it's the index (the
			// base+index operation is commutative, so if they specified it
			// in the other order we can just swap them to fix this)
			if (ri == ESP) {
				int tmp = ri;
				ri = rb;
				rb = tmp;
			}
			ASSERT(ri != ESP);

			// If EBP is the base, we have to use an offset even if one wasn't
			// specified, because MODrm 04 + bbb=101 (normally EBP) is a special
			// case that means disp32[index] with no base register.  This is an
			// Intel quirk, not my clever idea!
			if (a != IdxScaleDisp && rb == EBP && op[n].val.mem.MODrm == 0x04) {
				op[n].val.mem.MODrm = 0x44;
				op[n].val.mem.dispLen = 1;
			}

			// encode the SIB: 04 ssiiibbb
			op[n].val.mem.sib = ss | (ri << 3) | rb;
		}
		else if (a == Label) {
			op[n].typ = opLbl;
			op[n].val.lbl = va_arg(va, struct jit_label *);
		}
		else if (a == Offset) {
			op[n].typ = opOfs;
			op[n].val.ofs = va_arg(va, int);
		}
		else {
			// we're out of valid operand types
			ASSERT(0);
		}
	}

	// Figure the size of any indeterminate memory operands.  The size of a
	// memory operand is implied by the size of a register operand:  MOV EAX,[SI]
	// implicitly makes the [SI] a DWORD PTR type.  If there's no register operand,
	// the size can't be inferred, so it must be explicitly specified with a
	// BytePtr, WordPtr, or DwordPtr prefix.
	//
	// There are some exceptions:
	//    ROL|ROR|RCL|RCR|SHL|SHR|SAR rm,CL - the CL register doesn't imply the rm size
	//
	memType = inferMemType(memType, &op[0]);
	if (!(mne == imROL || mne == imROR || mne == imRCL || mne == imRCR || mne == imSHL || mne == imSHR || mne == imSAR))
		memType = inferMemType(memType, &op[1]);

	// Set any memory operands of indeterminate type to the inferred type
	setInferredMemType(&op[0], memType);
	setInferredMemType(&op[1], memType);

	// Find the best matching instruction in the opcode table.  Use the index to
	// jump straight to the first entry at the opcode mnemonic, and stop scanning
	// when we reach an entry with a different opcode.  All entries for a given
	// mnemonic are grouped, so we only have to run through the contiguous block
	// for the target opcode.
	for (mbest = 0, i = mneIdx[mne], m = mnetab + i ;
		 i < sizeof(mnetab)/sizeof(mnetab[0]) && m->mne == mne ;
		 ++i, ++m)
	{
		// check for an instruction match and operand matches
		if (m->mne == mne
			&& match_operand(m->op1, &op[0])
			&& match_operand(m->op2, &op[1])
			&& match_operand(m->op3, &op[2]))
		{
			// if this is the only instruction so far, or the least costly, or the
			// shortest, keep it
			if (mbest == 0
				|| m->cost < mbest->cost
				|| (m->cost == mbest->cost && oplen(m) < oplen(mbest)))
				mbest = m;
		}
	}

	// a match is required
	ASSERT(mbest != 0);
			
	// start generating the instruction machine code bytes
	p = buf;

	// add the TWOBYTE prefix if necessary
	if (mbest->flags & E_TwoByte)
		*p++ = TWOBYTE;

	// add the OPSIZE prefix if we have 16-bit operands
	if (mbest->flags & E_16)
		*p++ = OPSIZE;

	// Add the primary opcode
	*p = mbest->opcode;

	// If the opcode encodes a register value, add the register from operand 1
	if (mbest->flags & E_R)
	{
		ASSERT(op[0].typ == opReg8 || op[0].typ == opReg16 || op[0].typ == opReg32);
		*p |= op[0].val.reg;
	}

	// done with the opcode byte
	++p;

	// Encode the MOD REG R/M byte, SIB, and displacement, if either operand
	// 1 or 2 is an rmxx type
	if (mbest->op1 == rm8 || mbest->op1 == rm16 || mbest->op1 == rm32)
	{
		// the left operand is the MOD R/M
		p = encode_modrm(p, mbest, &op[0], &op[1]);
	}
	else if (mbest->op2 == rm8 || mbest->op2 == rm16 || mbest->op2 == rm32)
	{
		// the right operand is the MOD R/M
		p = encode_modrm(p, mbest, &op[1], &op[0]);
	}
	else if ((mbest->flags & (E_ImpR | E_RExt)) != 0
			 && (mbest->op1 == r8 || mbest->op1 == r16 || mbest->op1 == r32))
	{
		// operand 1 is a register, and operand 2 is implicitly the same register -
		// encode the operand 1 register in both MOD R/M and REG fields: 11 rrr rrr
		*p = 0xC0 | (op[0].val.reg << 3) | op[0].val.reg;
	}

	// Add any immediate values
	p = encode_immediate(p, mbest->op1, &op[0]);
	p = encode_immediate(p, mbest->op2, &op[1]);
	p = encode_immediate(p, mbest->op3, &op[2]);

	// store the generated instruction
	ins = add_instr(p - buf, buf);

	// set the label, if there is one
	ins->lbl = (op[0].typ == opLbl ? op[0].val.lbl :
				op[1].typ == opLbl ? op[1].val.lbl :
				op[2].typ == opLbl ? op[2].val.lbl : (struct jit_label *)0);

	// close out the varargs
	va_end(va);
}

#endif /* JIT_ENABLED */
