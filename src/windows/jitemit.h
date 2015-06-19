/*
 *   JIT code emitter framework for Intel.  This provides a mechanism for
 *   generating native Intel machine code for the JIT.
 *   
 *   Caution!  This header file might not play well with others.  Include it
 *   only in the actual JIT code generator modules.  We use some very short
 *   names for macros to make the machine language code specs more concise
 *   and readable, but the tradeoff is that the symbol names might well
 *   conflict with those in other modules.  To avoid conflicts, restrict
 *   inclusion of this header to the JIT code generator .c files.
 *   
 *   In most cases, the top level of the translator should enqueue the
 *   instruction address that the emulator is currently at, using
 *   jit_emit_queue_instr(), then call jit_emit_process_queue() to process
 *   that instruction and any additional instructions it enqueues during that
 *   translation.
 *   
 *   The queue processor needs a CPU-specific callback that actually does the
 *   work of translating instructions from the source CPU to Intel.  The
 *   outline of this routine looks like this:
 *   
 *   1.  Call jit_emit_push() to create an emitter stack frame.
 *   
 *   2.  Call jit_emit_begin_instr() to begin the code for an emulated
 *   opcode.
 *   
 *   2a. Parse the current souce (emulated) CPU opcode to determine what it
 *   does.
 *   
 *   2b. Make one or more calls to emit() to generate the series of native
 *   code instructions to execute the emulated opcode you're translating.
 *   
 *   If you generate part of the translation for an opcode and then decide
 *   along the way that you can't translate the opcode after all, you can
 *   simply abandon the emitted code by calling jit_emit_cancel_instr().
 *   
 *   2c. If the next opcode after that can be sequentially reached from the
 *   current opcode, you can return to step 2 and translate the next opcode
 *   as part of the current code block.
 *   
 *   3. Call jit_emit_commit() to save the generated native code to the JIT's
 *   executable code page.  This finalizes the machine code generated with
 *   emit() calls since the stack frame was opened.  This step figures out
 *   the offsets for jump (branch) instructions within the generated code.
 *   
 *   4.  Call jit_emit_pop() to pop the stack level.
 *   
 *   * * * 
 *   
 *   emit() is essentially a min-assembler.  The first argument is the
 *   instruction mnemonic, and additional arguments give the operands.
 *   emit() figures out the Intel binary encoding for the instruction with
 *   the given operands, and stores it in temporary memory in the stack
 *   frame.
 *   
 *   * * *
 *   
 *   You can generate code recursively by using jit_emit_push() and
 *   jit_emit_pop() to bracket each recursive level.  This is useful for
 *   visiting subroutine code called from within the current code - the call
 *   tells you that the code is reachable, so you'll want to translate it,
 *   but you'll want it stored separately rather than inline.  The push/pop
 *   mechanism accomplishes this.  You can also use jit_emit_queue_instr() to
 *   queue an opcode for later processing; this is useful for forward jumps,
 *   where it's likely that the same code will be reached sequentially from
 *   the current instruction, in which case it won't be necessary to visit it
 *   separately.
 */


#ifndef INC_JITEMIT
#define INC_JITEMIT

/*
 *   Push/pop the code emitter state.  Use this to bracket translation of a
 *   code block.  All translation of contiguous code must occur within a
 *   stack level.
 *   
 *   The stacking mechanism lets you visit the code recursively.  This can be
 *   used to generate the code for the target of a subroutine call when the
 *   call is encountered, so that the calling code can be generated with a
 *   direct branch to the generated subroutine code.  For jumps ahead, it's
 *   better to queue the target instruction for later translation, as this
 *   defers generating the target instruction until we determine if the
 *   current code block contains the target code sequentially, in which case
 *   it will be generated with the current code block anyway and doesn't
 *   require recursion.
 *   
 *   push() returns true on success, false if the maximum stack depth is
 *   exceeeded.  
 */
int jit_emit_push();
void jit_emit_pop();

/*
 *   Queue an instruction for translation.  This is especially useful for
 *   forward jumps.  The target of a forward jump might end up being
 *   reachable as part of the sequence of instructions from the current
 *   point, in which case it will usually be translated automatically as part
 *   of the current block of code, since we usually work through instructions
 *   sequentially until encountering an unconditional branch of some kind.
 *   Queuing the target instruction in this case allows the target code to be
 *   generated contiguously with the current block, which makes the code a
 *   little more efficient by avoiding unnecessary jumps between blocks.
 */
void jit_emit_queue_instr(data32_t addr);

/*
 *   Process all queued instructions.
 */
void jit_emit_process_queue(struct jit_ctl *jit, int (*func)(struct jit_ctl *jit, data32_t addr));

/*
 *   Begin a new emulator instruction.  This sets the emulated code address
 *   to 'addr', and sets a marker that can be used to roll back any code
 *   generated for the instruction via jit_emit_cancel_instr().
 */
void jit_emit_begin_instr(struct jit_ctl *jit, data32_t addr);

/*
 *   Cancel the current instruction.  This discards code back to the market
 *   inserted at jit_emit_begin_instr().  This can be used if the translator
 *   determines midway through handling an opcode that it can't handle the
 *   opcode after all.  This will discard any incomplete code generated along
 *   the way.
 */
void jit_emit_cancel_instr(struct jit_ctl *jit);

/*
 *   Generate a return to emulator at the given emulated code address.  This
 *   can be called after generating a series of instructions to ensure that
 *   an instruction is generated at the end that properly returns to the
 *   emulator, in case execution in the generated native code falls through
 *   past the last previously generated instruction.
 */
void jit_emit_return_to_emu(data32_t addr);

/*
 *   Commit the current code block.  This saves the temporary code we've
 *   generated since the jit_emit_push() into the JIT executable page, and
 *   updates the address map so that each instruction points to the
 *   corresponding stored code.  If there are any unresolved forward labels,
 *   we'll resolve them to return to the emulator at the target emulated
 *   address.
 */
void jit_emit_commit(struct jit_ctl *jit);


/*
 *   Emit a native Intel machine code instruction.  This is a mini-assembler:
 *   we take an instruction mnemonic and operands, and encode this into the
 *   corresponding binary machine code.  The generated code is added to an
 *   internal buffer.
 *   
 *   The first argument is the mnemonic for the instruction (MOV, ADD, INC,
 *   etc - note that you DON'T use the 'im' prefix in the intelMneId list
 *   here, since the macro adds this for you to make the code more readable).
 *   This followed by zero or more additional arguments giving the operands
 *   for the instruction.  The number and types of the operands vary by
 *   instruction and addressing mode.
 *   
 *   emit() is a convenience macro: it adds the 'im' prefix to the mnemonic
 *   and calls the jit_emit() function.  This lets you use the plain Intel
 *   mnemonics (sans 'im' prefix) for better readability.
 *   
 *   emitv() calls the function *without* adding the 'im' prefix, letting you
 *   use a variable or expression (the 'v' in the name is for 'variable') as
 *   the mnemonic.  In this case you'll need to include the 'im' prefix
 *   explicitly in your mnemonic names.
 *   
 *   Note that if you call the function (jit_emit()) directly without one of
 *   the macros, you MUST supply the special value EndOfOps as the last
 *   argument, so that the function can figure out how many operands you
 *   supplied in the varargs.
 */
typedef enum intelMneId intelMneId;
#define emit(mne, ...) jit_emit(im##mne, __VA_ARGS__, EndOfOps)
#define emitv(mne, ...) jit_emit(mne, __VA_ARGS__, EndOfOps)
void jit_emit(intelMneId mne, ...);

/*
 *   Create a forward label.  Returns the label number, or -1 if no more
 *   labels are available.  The label will be marked as unresolved; any
 *   references will create fixups that will be set to their final values
 *   when jit_resolve_label() is called.
 *   
 *   Labels are automatically deleted in jit_reset(), so there's no need to
 *   delete labels individually.
 */
struct jit_label *jit_new_fwd_label();

/*
 *   Create a new label and set its address to the current code address.
 */
struct jit_label *jit_new_label_here();

/*
 *   Create a label that points to an emulated code address.  'addr' is an
 *   opcode address in the emulated address space.  This is used to generate
 *   branches to emulator locations that might be in the same code block.
 */
struct jit_label *jit_new_addr_label(data32_t addr);

/*
 *   Create a label that points to a native code address.  This can be used
 *   for jumps to know locations, such as generated helper routines or static
 *   C code.  For example, this can be used for jumps to the jit->pLookup
 *   handler.
 */
struct jit_label *jit_new_native_label(byte *addr);

/*
 *   Resolve a forward label.  This sets the label's target address to the
 *   current code pointer, and fixes up any references that have been emitted
 *   to the label so far. 
 */
void jit_resolve_label(struct jit_label *l);


/*
 *   Instruction mnemonic IDs for the Intel opcodes.  These are internal IDs
 *   that we map to binary machine code instructions through jit_emit(),
 *   which analyzes the operands to determine the encoding.
 *   
 *   Note that Intel instruction mnemonics don't map directly to hex opcodes,
 *   so we don't make any attempt here to assign these actual machine opcode
 *   numbers.  A given mnemonic can represent multiple opcodes; for example,
 *   MOV maps to 32 distinct hex opcodes, four of which are two-byte opcodes
 *   (with the 0F prefix byte).  The final binary encoding depends upon the
 *   number and types of operands present.
 */
enum intelMneId {
	imADD,
	imOR,
	imADC,
	imSBB,
	imAND,
	imSUB,
	imXOR,
	imNOT,
	imCMP,
	imINC,
	imDEC,
	imPUSH,
	imPOP,
	imPUSHA,
	imPOPA,
	imIMUL,
	imMUL,
	imIDIV,
	imDIV,
	imJO,
	imJNO,
	imJB,
	imJAE,
	imJE,
	imJNE,
	imJBE,
	imJA,
	imJS,
	imJNS,
	imJP,
	imJPE,
	imJNP,
	imLPO,
	imJL,
	imJGE,
	imJLE,
	imJG,
	imTEST,
	imXCHG,
	imMOV,
	imMOVSX,
	imMOVZX,
	imLEA,
	imNOP,
	imWAIT,
	imPUSHFD,
	imPUSHF,
	imPOPFD,
	imPOPF,
	imSAHF,
	imLAHF,
	imMOVSB,
	imMOVSD,
	imMOVSW,
	imSTOSB,
	imSTOSD,
	imSTOSW,
	imLODSB,
	imLODSD,
	imLODSW,
	imSCASB,
	imSCASD,
	imSCASW,
	imCMPSB,
	imCMPSW,
	imCMPSD,
	imROL,
	imROR,
	imRCL,
	imRCR,
	imSHL,
	imSHR,
	imSAR,
	imRETN,
	imRETN0,
	imLES,
	imLDS,
	imLFS,
	imLGS,
	imLSS,
	imENTER,
	imLEAVE,
	imRETF,
	imRETF0,
	imINT3,
	imINT,
	imINTO,
	imIRET,
	imLOOPNZ,
	imLOOPZ,
	imLOOP,
	imJECXZ,
	imJCXZ,
	imCALL,
	imJMP,
	imHLT,
	imCMC,
	imCLC,
	imSTC,
	imCLI,
	imSTI,
	imCLD,
	imSTD,
	imSETA,
	imSETBE,
	imSETC,
	imSETG,
	imSETGE,
	imSETL,
	imSETLE,
	imSETNC,
	imSETNO,
	imSETNP,
	imSETNS,
	imSETNZ,
	imSETO,
	imSETP,
	imSETS,
	imSETZ,
	imCBW,
	imCWD,
	imCWDE,
	imCDQ,

	nIntelMneId
};

/* synonyms for instructions with more than one name in the standard Intel set */
#define imSETAE imSETNC
#define imSETB  imSETC
#define imSETE  imSETZ
#define imSETNA imSETBE
#define imSETNAE imSETB
#define imSETNB imSETAE
#define imSETNBE imSETA
#define imSETNE imSETNZ
#define imSETNG imSETLE
#define imSETNGE imSETL
#define imSETNL imSETGE
#define imSETNLE imSETG
#define imSETPE imSETP
#define imSETPO imSETNP

#define imJZ    imJE
#define imJNZ   imJNE
#define imJNAE  imJB
#define imJNB   imJAE
#define imJNA   imJBE
#define imJNBE  imJA
#define imJNGE  imJL
#define imJNLE  imJG
#define imJNL   imJGE
#define imJNG   imJLE
#define imJNC   imJAE
#define imJC    imJB


/*
 *   Operand codes for registers.  Note that the first 8 are in the order for
 *   the 'rrr' field in instructions that encode registers this way (e.g.,
 *   PUSH, opcode 0x50 = b01010rrr, so PUSH ECX = 0101001 = 0x50 | rrr(ECX)
 */
#define is_r32(r) (((r) & ~0x0F) == 0x100)
#define rrr(r) ((r) & 7)  // convert from register ID to rrr opcode bit field
#define EAX  0x100   // rrr = 000
#define ECX  0x101   // rrr = 001
#define EDX  0x102   // rrr = 010
#define EBX  0x103   // rrr = 011
#define ESP  0x104   // rrr = 100
#define EBP  0x105   // rrr = 101
#define ESI  0x106   // rrr = 110
#define EDI  0x107   // rrr = 111

// the 16-bit registers are in the same order for rrr masking, but
// note that these require the size override prefix for access
#define is_r16(r) (((r) & ~0x0F) == 0x110)
#define AX   0x110
#define CX   0x111
#define DX   0x112
#define BX   0x113
#define SP   0x114
#define BP   0x115
#define SI   0x116
#define DI   0x117

// the byte registers have yet another order
#define is_r8(r) (((r) & ~0x0F) == 0x120)
#define AL   0x120
#define CL   0x121
#define DL   0x122
#define BL   0x123
#define AH   0x124
#define CH   0x125
#define DH   0x126
#define BH   0x127

// and then there are the segment registers and the instruction pointer
#define is_segreg(r) (((r) & ~0x0F) == 0x140)
#define DS   0x140
#define ES   0x141
#define FS   0x142
#define GS   0x143
#define SS   0x144
#define CS   0x145
#define IP   0x146

/*
 *   Type tags for emit() operand arguments.  For most opcodes, each operand
 *   will be specified with a type code followed by one or more value
 *   arguments.  Note that these are in separate value ranges from the
 *   instruction mnemonics to make them more easily distinguished when
 *   debugging, although this isn't strictly necessary since we can always
 *   tell what an argument means positionally.
 */

/*
 *   Operand code for immediate data.  In an emit() call, use Imm followed by
 *   the immediate data value: emit(MOV, EAX, Imm, 14) is equivalent to the
 *   assembly instruction MOV EAX, 14.
 */
#define Imm  0x500

/*
 *   Memory operand size specifiers - BYTE PTR, WORD PTR, DWORD PTR.  Use one
 *   of these before a memory operand (Idx, BaseDisp, etc) when the memory
 *   reference size is ambiguous.  When an instruction has a register operand
 *   and a memory operand, the size of the memory operand can be inferred
 *   from the register type; e.g., MOV AL, [SI] moves a byte, MOV EAX, [SI]
 *   moves a dword.  When there's no register operand, though, the size is
 *   ambiguous: MOV [SI], 1.  In such cases, a size specifier is required:
 *   MOV DWORD PTR [SI], 1.
 */
#define BytePtr  0x501
#define WordPtr  0x502
#define DwordPtr 0x503

/*
 *   Operand code for an indexed register.  In an emit() call, use Idx
 *   followed by the register name: emit(MOV, EAX, Idx, ESI) is equivalent to
 *   MOV EAX, [ESI].  An immediate displacement can also be used, to
 *   reference a memory location directly: emit(MOV, EAX, Idx, Imm, 0x1234)
 *   is equivalent to MOV EAX, [1234h].
 *   
 */
#define Idx  0x510

/*
 *   Operand code for index register + displacement (same as base register +
 *   displacement).  emit(MOV, EAX, BaseDisp, EBX, 0x100) means MOV EAX,
 *   EBX[100h]. 
 */
#define BaseDisp 0x520
#define IdxDisp  0x520

/*
 *   Operand code for a base+index address.  emit(MOV, EAX, BaseIdx, EBX,
 *   ESI) means MOV EAX, [EBX+ESI].
 */
#define BaseIdx  0x530

/*
 *   Operand code for a base+index+displacement address.  emit(MOV, EAX,
 *   BaseIdxDisp, EBX, EDI, 16) means MOV EAX, [EBX+EDI+1234h]. 
 */
#define BaseIdxDisp  0x540

/*
 *   Base + Index + Scale + Displacement, as in [EBX+EDI*4 + 1234h]
 */
#define BaseIdxScaleDisp  0x550

/*
 *   Index + Scale + Displacement: emit(MOV, EAX, IdxScaleDisp, ESI, 4,
 *   0x1234) means MOV EAX, [ESI*4 + 1234h]
 */
#define IdxScaleDisp  0x560


/* Label, for a JMP: emit(JNE, Label, 3) */
#define Label  0x600

/* Offset, for a JMP: emit(JNE, Offset, 2) -> JNE $+2 */
#define Offset 0x601

/* End of operands flag */
#define EndOfOps 0x1000


#endif /* INC_JITEMIT */
