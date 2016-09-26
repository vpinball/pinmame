/*
 *   Just-in-time machine code translator infrastructure
 *   
 *   This module contains the core of the JIT translator for Windows.
 *   Individual translators must be built on top of this core for each source
 *   CPU type - that is, for each instruction set that we want to translate
 *   to native Intel machine code for execution.
 *   
 *   One of MAME's main functions is to execute binary machine code for a
 *   variety of different CPUs, providing software emulation of processors
 *   that obviously aren't physically present in the host PC.  It does this
 *   by interpreting the binary machine code from non-native instruction
 *   sets, one instruction at a time.  This works well for many older, slower
 *   CPUs, since modern Intel hardware is so fast that it can interpret and
 *   execute machine code from older CPUs much faster than the original
 *   hardware ever ran.  For example, many pinball controller boards from the
 *   1980s and 1990s were based on the 6809, which we can emulate on a modern
 *   PC at many times the original hardware speed.  However, interpreted
 *   emulation isn't always fast enough for more recent pinballs that were
 *   originally based on more modern hardware.  For example, the Stern
 *   Whitestar II series had a sound board based on the AT91, which is a
 *   modern 32-bit RISC chip based on an ARM7 core, with an instruction set
 *   that's unfriendly to emulation.  The MAME AT91 emulator struggles to
 *   keep up with real time when executing the Stern audio ROM code,
 *   resulting unacceptably slow playback and/or audio glitches on many
 *   current PCs.
 *   
 *   The JIT translator is an optional supplement to the standard interpreted
 *   emulation.  With the JIT, instead of interpreting the meaning each
 *   original machine code instruction as we execute it, we *translate* the
 *   original machine code instruction to native Intel instructions.  This
 *   translation process only has to be performed once for each instruction.
 *   Once an instruction is translated, we execute the translated code
 *   directly.  This is many times faster than interpreting the original
 *   opcodes, because (a) we only have to do the translation once per
 *   instruction, rather than interpreting the meaning of each instruction
 *   every time we execute it, (b) we eliminate much of the overhead of
 *   stepping through and fetching instructions, since this task is carried
 *   out in the JIT version the real CPU hardware, and (c) we get better
 *   locality of reference to the program memory, since we don't have to run
 *   through a large emulator loop on every instruction.  (Locality of
 *   reference is important on modern hardware because it makes better use of
 *   the CPU cache, which is much faster than accessing the main RAM.)
 *   
 *   JIT translation is obviously dependent on both the host hardware we're
 *   running on and the original CPU we're emulating.  This core module is
 *   specific to JITs for hosts running Windows on Intel 32-bit x86 hardware.
 *   This core will have to be re-implemented if anyone ever wants to port
 *   the JIT to other host platforms in the future.
 *   
 *   Our basic strategy with the JIT is to translate each source instruction
 *   to a self-contained block of native host instructions.  The translation
 *   must be done such that we can jump back and forth between JIT and
 *   emulation on any instruction boundary.  This allows a JIT to be
 *   incomplete, handling only a subset of the source instruction set.  This
 *   makes it much easier and quicker to develop a working translator for a
 *   given source CPU, because (a) we can create it incrementally, and (b) we
 *   can omit instructions that are difficult to handle natively.  It's
 *   likely that most ROMs we target for JIT translation will mostly use a
 *   small subset of the source CPU's instruction set, and an even smaller
 *   set on the critical performance path.  The important thing for a JIT is
 *   to translate the instructions on the critical performance path; for
 *   instructions that aren't used frequently, we can just let the emulator
 *   handle them.
 *   
 *   For each CPU that we apply the JIT to, we maintain an address mapping
 *   array that gives us the physical address of the native code
 *   corresponding to each emulated instruction.  When the emulator is about
 *   to fetch an instruction to interpret, it checks the mapping table to see
 *   if the instruction at the current PC (program counter) address has been
 *   translated.  If so, the emulator simply jumps to the translated native
 *   code.
 */

/*
 *   Here are the steps to set up a JIT translator for a CPU type:
 *   
 *   1. Create a CPU-specific xxxjit.h file for the machine you're adding the
 *   JIT to.  E.g., at91jit.h for AT91.  In this file, define the following
 *   items, then #include "windows/jit.h" at the end:
 *   
 *   - #define JIT_NAME xxx - defines a name prefix for CPU-specific
 *   functions for your CPU JIT.  This is up to you; it just needs to be a
 *   valid C identifier, since it's used to construct function names.
 *   
 *   - #define JIT_OPALIGN n - n can be 8, 16, or 32, specifying the
 *   alignment type for your CPU's machine language instructions.  Use 8 if
 *   instructions can occur at any byte address, 16 if they're always aligned
 *   on 16-bit word boundaries, 32 if they're always aligned on dword
 *   boundaries.
 *   
 *   - Again, at the end of the file, #include "windows/jit.h".
 *   
 *   2. In your CPU emulator modules, #include the xxxjit.h file you created
 *   above.
 *   
 *   3. Find the static context structure for the CPU.  This is the structure
 *   used in the xxx_set_context() and xxx_get_context() calls, and usually
 *   contains the emulated machine's registers, flags, etc.  Add a new
 *   element of type 'struct jit_ctl *' to the context structure (call the
 *   new element anything you like).
 *   
 *   4. In the xxx_init() machine initializer routine for your emulated CPU,
 *   make a call to jit_create() to allocate the jit_ctl structure you
 *   declared above, and assign the result to the pointer you declared.
 *   
 *   5. Call jit_set_mem_callbacks() to establish your memory access
 *   callbacks.  This lets you specify callback functions for reading and
 *   writing memory within the emulated machine's RAM.  The generated code
 *   will call these functions for memory access.
 *   
 *   6. Determine the opcode address range that you want the JIT to cover.
 *   For a machine with a relatively small address space, like an 8-bit CPU
 *   with a 64K address space, you can simply cover the whole address space.
 *   For an emulated CPU with a large address space, you'll probably only
 *   want to cover the portion where the actual program code will be loaded,
 *   because the JIT map takes real memory proportional to the size of the
 *   emulated address space covered.
 *   
 *   Once you know the address range to cover, make a call to
 *   jit_create_map().  If the address space is fixed by the processor
 *   architecture, this can simply be done immediately after the jit_create()
 *   call above.  You can alternatively put this call in the code that loads
 *   the individual game ROMs, if the address space will vary by game.
 *   
 *   The map doesn't have to cover every possible opcode location.  If the
 *   JIT encounters an opcode outside of the map range, it will simply use
 *   the emulator for that opcode.  
 *   
 *   7. Find the machine shutdown code for the CPU.  This is usually called
 *   'void xxx_exit(void)', where xxx is the CPU name.  Call jit_delete()
 *   here to release the memory allocated above.
 *   
 *   8. Call jit_enable() *after* you've detected that the final program code
 *   is loaded into the emulated address space.  If the CPU starts off with
 *   the program code already loaded, you can call this right after you call
 *   jit_create().  If the program code is loaded or remapped dynamically by
 *   an emulated boot loader, though, you'll have to detect when the loading
 *   process is finished, and wait until then to call jit_enable().
 *   
 *   9. In the emulator execution loop, put a JIT_FETCH() macro just before
 *   the code that normally fetches the next opcode.  This will insert code
 *   that checks the state of the opcode address and determines if the code
 *   has already been translated or is eligible for translation.  If the code
 *   has already been translated, or can be newly translated, the macro will
 *   bypass the emulator and execute the native code for the instruction;
 *   otherwise execution will simply continue into the emulator as though the
 *   JIT weren't there.  Emulation will resume when the native code reaches
 *   an untranslated instruction.
 *   
 *   10.  Define a function 'int xxx_jit_xlat(struct jit_ctl *jit, data32_t
 *   pc)', where xxx is the JIT_NAME that you assigned in your xxxjit.h
 *   header.  This is where you implement the translator.  Sorry, but this is
 *   where the framework stops, and the real work begins; it's up to you to
 *   define the translation.  You have to decode the emulated opcodes and
 *   generate the corresponding Intel native code.
 *   
 *   A key principle in writing JIT translation code is that each opcode from
 *   the emulated program must stand alone.  You must expect that the
 *   emulator can call in to translated code at any instruction boundary.
 *   This means that you shouldn't assume in your translation for emulated
 *   instruction N that the registers and other ephemeral state are as you
 *   left them in your translated code for instruction N-1.  This constraint
 *   means that you shouldn't try to do global optimizations on the generated
 *   code; write each instruction's code like a little self-contained
 *   subroutine.  (Even with this optimization constraint, translated code
 *   will almost always run many times faster than emulation for the same
 *   code, since the translation doesn't have any overhead for decoding
 *   instructions, and has much better locality of reference for caching.)
 *   
 *   It's up to you to decide upon the stack and register environment that
 *   your generated code will use.  You'll have a chance to establish this
 *   environment at run time in the code that jumps into the native code,
 *   which we'll get to in the next step.  The only rule imposed by the
 *   general framework is that the native code must return to the emulator
 *   when it's finished by loading EAX with the instruction pointer for the
 *   next emulated instruction to execute, then do a RETN (0xC3) opcode.
 *   This requires you to use a CALL instruction in your environment setup
 *   code at the point where you actually jump to the generated native code.
 *   Other than that, you're free to set up the native Intel stack and
 *   registers as you decide is best.
 *   
 *   The translation routine should analyze the opcode at the given address
 *   and determine if it can be translated.  If so, generate the code and
 *   store it in the jit_page memory by calling jit_store_native().  The
 *   translator is also encouraged to continue translating more consecutive
 *   instructions at this point.  The recommended algorithm is to translate
 *   all consecutive instrutions until encountering either an opcode that
 *   can't be translated or an unconditional branch.  On most machines,
 *   control can proceed to the next instruction sequentially for anything
 *   but an unconditional branch, so it's a good bet that the whole series of
 *   instructions up to the next branch is all executable code.
 *   
 *   If the routine succeeds at translating the instruction at 'pc'
 *   (regardless of whether or not it also translates more code after that),
 *   return true (non-zero).  If the instruction isn't translated, return
 *   false.  Usually, you should also update the jit->native address map at
 *   'pc' to jit->pEmulate, because an instruction that can't be translated
 *   now can usually never be translated.  Changing the map to jit->pEmulate
 *   tells the emulator not to try translating the same instruction again the
 *   next time it's encountered, which will save time.  If you have some
 *   reason to think that the instruction might be translatable in the future
 *   (e.g., the failure to translate was due to a temporary resource
 *   limitation), you can leave the jit->native entry unchanged, in which
 *   case the emulator will invoke the translation routine again the next
 *   time the address is reached.
 *   
 *   11. In the emulator loop, add a label "jit_go_native:" at the end of the
 *   loop, AFTER the main code has already explicitly returned.  This code
 *   MUST be unreachable except by 'goto jit_go_native', which the JIT_FETCH
 *   above generated.  After the label, you must establish the native
 *   processor environment that you assume in your translated code generation
 *   routine (the real CPU registers, stack variables, etc).  For example, if
 *   you decide on a convention where the native EAX always contains the
 *   emulated stack pointer register value, you must load EAX with the stack
 *   register at this point.  These conventions are up to you to define; the
 *   JIT framework imposes no assumptions of its own here.
 *   
 *   Once you establish the native register and stack environment, make an
 *   assembly language CALL to the JIT_NATIVE address.  Refer to the model
 *   code in the comments at JIT_CALL_NATIVE below.  When the native code
 *   reaches a point where it decides to resume emulation, it will execute a
 *   native RETN instruction.  This will return control to the C statement
 *   following your assembly language CALL instruction.
 *   
 *   IMPORTANT: Before returning, generated code always loads EAX with the
 *   new instruction pointer where emulation should resume.  The code you
 *   write here must use the value in EAX as the new instruction pointer.
 *   You'll probably store your instruction pointer somewhere else, probably
 *   in a stack variable, for other purposes, but solely for the purposes of
 *   the return to the emulator, you MUST ignore your own convention and use
 *   the value in EAX.  The generic JIT framework uses EAX to transmit the
 *   new instruction pointer on return because we don't want to have to
 *   establish a more complex convention for storing other emulated
 *   registers, and we don't want the generic JIT framework tied to the
 *   conventions for any one CPU.
 *   
 *   After the JIT_CALL_NATIVE call, the final step is to undo the stack and
 *   register setup you established above.  Continuing our example where EAX
 *   contains the emulated stack pointer value, you must at this point move
 *   the new contents of EAX back to the emulator variable representing the
 *   stack pointer register.  This ensures that changes to the machine state
 *   made by the native code are reflected properly in the emulator
 *   variables.  Make sure the emulated program counter (PC) register is
 *   updated, too.  After you've copied back any state changes and undone any
 *   stack changes, use a "goto" to jump back to the top of the emulator
 *   loop.  The emulator will pick up where the translated native code left
 *   off.
 */

#ifndef INC_JIT
#define INC_JIT

typedef unsigned char byte;

#if defined(_MSC_VER) && (_MSC_VER >= 1400) && !defined(__LP64__) // visual studio & > 6 & 32bit compile
#define JIT_ENABLED  1   // enable the JIT (false -> use only the standard emulator code)
#else
#define JIT_ENABLED  0
#endif

#define JIT_DEBUG    0   // enable additional debugging code in the JIT

#if JIT_ENABLED

// figure the address-to-index right shift based on the opcode alignment
#if JIT_OPALIGN == 8
# define JIT_RSHIFT  0
#elif JIT_OPALIGN == 16
# define JIT_RSHIFT 1
#elif JIT_OPALIGN == 32
# define JIT_RSHIFT 2
#elif JIT_OPALIGN == 64
# define JIT_RSHIFT 3
#elif JIT_OPALIGN == 0
// special case for generic jit.c - use dynamic rshift from the structure
# define JIT_RSHIFT (jit->rshift)
#else
# error Invalid JIT_OPALIGN value - must be 8, 16, 32, or 64
#endif

/*
 *   JIT control structure.  This contains internal information that the JIT
 *   uses for translating and executing code.
 */
struct jit_ctl
{
	// Array of pointers to native code jump locations, indexed by
	// emulated opcode address scaled by 'rshift'.
	byte **native;

	// Address range for the map (inclusive of minAddr, exclusive
	// of maxAddr)
	data32_t minAddr, maxAddr;

	// The right-shift to go from an emulated address offset to an
	// array index.  For machines where an opcode can start at any
	// byte address, the index is the same as the offset, so the
	// shift is 0.  For 16-bit alignment, we only store every other
	// address, so we divide the offset by 2 to get the index, hence
	// the shift is 1.  For 32-bit alignment, we only store every 4th
	// address, so the shift is 2.
	int rshift;

	// Cycle count pointer.  MAME requires each CPU emulator
	// instruction execution loop to yield (by returning to MAME)
	// after its time slice expires.  Time slices are allocated by
	// cycle count, based on the nominal clock speed of the emulated
	// machine.  The translated JIT code needs to track the number
	// of emulated instructions it carries out in order to determine
	// when the time slice has expired.  To do this, the emulator
	// has to give us the address of an int32 with the remaining
	// cycle counter.  The generated code has to decrement this
	// for each emulated instruction, and must also test from time
	// to time to see if this reaches zero.
	//
	// The cycle counter MUST be a static variable that persists
	// for the lifetime of the JIT structure, since we hang onto
	// a pointer to it and generate code that refers directly to
	// the given address.  The existing emulators all use statics
	// for this, so those should be directly usable.
	//
	// The emulator loops generally check for expiration on every
	// instruction, but this doesn't seem strictly necessary, as
	// the main MAME loop can tolerate a bit of overshoot or
	// undershoot.  The original physical CPUs generally ran much
	// slower than we can emulate them, so the main MAME loop ends
	// up running the emulators in little bursts, then stalling
	// to keep execution in sync with real time.  (The real time
	// sync is important because the original programs perform
	// audio, video, and other tasks that must be carried out in
	// real time.)
	//
	// Because MAME can tolerate overshoot and undershoot in the
	// time slice consumption, and because translated native code
	// tends to run much faster than emulated code (that's the
	// whole point!), it seems acceptable for translated code to
	// check the cycle counter only occasionally.  A reasonable
	// strategy might be to check it whenever making a subroutine
	// call or an unconditional branch.
	int *cycle_counter_ptr;

	// Special native addresses for opcodes in the "pending" and
	// "emulate" states.  If an address mapping contains the
	// "pending" pointer value, it means that the opcode is
	// eligible for translation but hasn't been translated yet.
	// If it contains the "emulate" pointer value, it means that
	// the opcode is ineligible for translation and should always
	// be emulated.
	//
	// These two pointers must have distinct values, because the
	// emulator uses the pointer value to determine which treatment
	// (pending or emulate).  Both of them point to native code
	// consisting simply of a RETN instruction.  If translated native
	// code jumps to an opcode in either state, the RETN will cause
	// the native code to return to the emulator, so that the
	// emulator can emulate or translate the original opcode.
	//
	// Note that the mapping pointer for a translated instruction
	// has neither of these values - it will instead point directly
	// to the translated native code.  So any opcode whose address
	// mapping is any other pointer value is in the "translated"
	// state.  In this case, the emulator will jump to the native
	// translation to execute the opcode.
	byte *pPending;
	byte *pEmulate;

	// Special native opcode address for the "working" state.  This
	// is only used during translation, to mark an opcode as currently
	// being translated.
	byte *pWorking;

	// Address of the emulated address lookup routine for generated
	// code.  When we initialize the JIT, we generate code that takes
	// an emulator address in EAX, looks it up in our address map,
	// and jumps directly to the native code if the address is valid.
	// If the address doesn't contain native code, we simply return
	// to the emulator.  To invoke this, load EAX with the native
	// address and perform a JMP here.
	byte *pLookup;

	// Lookup-and-patch.  This must be reached by a CALL rather than
	// a JMP.  We look up the current native address for the emulated
	// instruction as in pLookup, but if the address now has native
	// translated code, we'll patch the caller (thus the need for a
	// CALL - we need to know where the invocation came from) with
	// a direct JMP to the new native code.  This lets the caller
	// bypass the lookup step on future invocations, which speeds
	// things up a bit.  This is only suitable for static jumps -
	// dynamic jumps (e.g., subroutine returns or indirect jumps)
	// must always do a run-time lookup.
	byte *pLookupPatch;

	// Head of native code program memory list allocated by the JIT
	// for this CPU.  The JIT uses this internally to manage the memory
	// containing the translated code.
	struct jit_page *pages;
	data32_t mem_count;

	// Read and write callbacks.  The generated code calls these
	// functions to access memory.
	byte *read8;     // data8_t  (*read8)(int addr);
	byte *read16;    // data16_t (*read16)(int addr);
	byte *read32;    // data32_t (*read32)(int addr);
	byte *write8;    // void (*write8)(int addr, data8_t data);
	byte *write16;   // void (*write16)(int addr, data16_t data);
	byte *write32;   // void (*write32)(int addr, data32_t data);
};


/*
 *   Allocate the JIT control structure.  This must be called in the emulated
 *   CPU's xxx_init() machine init routine.
 */
#define jit_create(cycle_counter) _jit_create(cycle_counter, JIT_RSHIFT)
struct jit_ctl *_jit_create(int *cycle_counter, int rshift);

/*
 *   Reset the JIT.  This should be called if the CPU is reset in such a way
 *   that it invalidates the program memory contents.  Resetting an emulated
 *   CPU will usually clear program RAM restart the boot loading procedure,
 *   so the JIT has to start over from scratch as well.
 *   
 *   This routine discards all previously translated native code and disables
 *   translation, just like after jit_create() is first called.  After
 *   calling this, you must call jit_enable() to re-enable translation at the
 *   appropriate point after the bootstrap loading process has completed.
 */
void jit_reset(struct jit_ctl *jit);

/* 
 *   Free the JIT control structure and all of its components (including any
 *   translated code).  Call this from the CPU's xxx_exit() routine, to
 *   release all JIT-related memory on shutdown.
 */
void jit_delete(struct jit_ctl **jit);

/*
 *   Create the JIT address map, covering the given range within the emulated
 *   CPU's address space.  The address range is inclusive of minAddr and
 *   exclusive of maxAddr.  The JIT will only translate opcodes within this
 *   address range.  Any instructions outside of this range will be emulated.
 *   
 *   All locations in the map are initialized to "emulate" status, which
 *   effectively disables JIT translation initially.  Call jit_enable() to
 *   enable translation.
 */
void jit_create_map(struct jit_ctl *jit, data32_t minAddr, data32_t maxAddr);

/*
 *   Set the memory access callbacks.  The caller must invoke this during
 *   initialization to tell us how to access memory from generated code.
 */
void jit_set_mem_callbacks(
	struct jit_ctl *jit,
	data8_t (*read8)(int addr),
	data16_t (*read16)(int addr),
	data32_t (*read32)(int addr),
	void (*write8)(int addr, data8_t data),
	void (*write16)(int addr, data16_t data),
	void (*write32)(int addr, data32_t data));


/*
 *   Enable translation.  This changes the state of every instruction in the
 *   covered address range from "emulate" to "pending".  The emulator must
 *   call this when the bootstrap procedure is finished and the final program
 *   is loaded into the machine's program address space.
 */
void jit_enable(struct jit_ctl *jit);

/*
 *   Un-translate an instruction at the given byte address.  This should be
 *   called any time an individual memory location within the address map is
 *   written during normal execution.  Writing into program memory might
 *   change the meaning of an instruction that was previously translated, so
 *   it's necessary to delete any existing translation at the modified
 *   address when this happens.  This will have no effect if an instruction
 *   hasn't already been translated, so it's flexible in cases where code is
 *   dynamically loaded or generated.
 *   
 *   (It's not necessary to call this during bootstrapping, since translation
 *   is initially disabled for the whole address space.)
 */
void jit_untranslate(struct jit_ctl *jit, data32_t addr);

/* 
 *   get the native code pointer for a given machine code address (this isn't
 *   range-checked - always check that the address is in range before
 *   evaluating this) 
 */
#define JIT_NATIVE(jit, addr) ((jit)->native[((addr) - (jit)->minAddr) >> JIT_RSHIFT])

/*
 *   Fetch the next instruction, try translating to native, and jump to
 *   native if possible.  This must be placed just before the emulator code
 *   that fetches the next opcode.  The emulator's program counter register
 *   must contain the address of the next instruction at this point.  If the
 *   next instruction can't be translated, we'll simply proceed to the next
 *   statement to process the opcode in the normal emulator loop.
 *   
 *   'jit' is the jit_ctl structure pointer for the machine.  'pc' is the
 *   current instruction pointer (an address in the emulated CPU address
 *   space).
 */
#define JIT_FETCH(jit, pc) \
	if ((pc) >= (jit)->minAddr && (pc) < (jit)->maxAddr) \
    { \
		byte *tmp = JIT_NATIVE(jit, pc); \
		if (tmp == (jit)->pPending) { if (JIT_XLAT_FUNC(JIT_NAME)(jit, pc)) goto jit_go_native; } \
		else if (tmp != (jit)->pEmulate) goto jit_go_native; \
	}

/*
 *   JIT_CALL_NATIVE - comments on how to invoke the native code.
 *   
 *   We originally provided a macro here called JIT_CALL_NATIVE() to invoke
 *   the native code, but that didn't seem flexible enough.  Instead, we
 *   provide some model code that you can customize as needed.  This process
 *   is a little tricky, but there are well-defined rules - you can make the
 *   code robust and reliable if you understand what's going on.  Here we try
 *   to explain not just what you need to do but also why, to help make the
 *   code adaptable to different cases.
 *   
 *   Here are the steps involved.
 *   
 *   - If there are any C expressions that you will need to evaluate after
 *   this point that are more complex than accessing local or static
 *   variables, evaluate those expressions now and store them in C local
 *   variables.  You musn't evaluate any C expressions once you enter
 *   assembly language in the next step.  For one thing, __asm mode won't let
 *   you.  But also don't give in to the temptation to exit __asm briefly to
 *   evaluate a C expression, because C expression evaluation will use the
 *   Intel CPU registers in unpredictable ways that could corrupt the
 *   environment setup that we're attempting.  It's best to keep everything
 *   explicitly in __asm once we get going, so that we have everything
 *   completely under our control, with no meddling by the compiler.
 *   
 *   One expression that you'll definitely need to save here is
 *   JIT_NATIVE(jit, pc), where pc is the emulator instruction pointer for
 *   the native code that we're about to invoke.  JIT_NATIVE() gives us a
 *   pointer to that native code.  We'll need that later to make the call to
 *   the native code, which is after all the entire point of this exercise.
 *   
 *   - Enter an assembly language block with __asm { }.
 *   
 *   - If you need any temporary stack slots, allocate them now using "SUB
 *   ESP, n", where n is the number of bytes you need (this is usually 4x the
 *   number of int or pointer variables you need to store).
 *   
 *   - Save the (real) CPU registers that the C compiler might use in
 *   generated code, by pushing them onto the stack.  It's best not to make
 *   assumptions based on your particular build settings, because there's
 *   more than one way for the compiler to use registers.  The actual usage
 *   will depend upon the choice of compiler and optimization settings.  To
 *   make the code robust for future changes in the overall VPinMAME build,
 *   you should save EBX, ECX, EDX, ESI, EDI, and EBP, except that you don't
 *   have to save any of these that your generated translation code won't
 *   directly modify.  (You only have to consider the code you directly
 *   generate; code you call as subroutines should be safe because the
 *   compiler will presumably apply its own register usage conventions
 *   consistently across the whole build, and will thus save any registers
 *   that need to be saved according to its own conventions when entering
 *   subroutines that you call from generated code.)
 *   
 *   - If you need to access any C local variables, it's time to move them
 *   into registers or into the stack.  The only truly safe register to
 *   modify during this process is EAX, because all known Intel calling
 *   conventions use that as a free register for return values and
 *   intermediate expression results.  Any other register (particularly EBP
 *   and EBX) could be the compiler's frame pointer register, which means
 *   that you will lose access to C locals as soon as you modify it.  Since
 *   we want this code to be robust against changes in optimization settings,
 *   we don't want to make assumptions about which registers are safe here.
 *   If you need to access only one C local, you can move it into any
 *   register, since even if this turns out to be the frame pointer, it won't
 *   be overwritten until after the C local has already been read.  If you
 *   need to access two C locals, you can load the first into EAX and the
 *   second into any other register, by the same logic.  If you need to
 *   access three or more, you will have to move them into stack temp slots
 *   that you allocated above, before saving registers.  Address the temp
 *   slots using [ESP+n].
 *   
 *   - Get the JIT_NATIVE value that you saved earlier into a register (e.g.,
 *   EAX), and CALL that register.  This will transfer control to the native
 *   generated code.  When that code wants to return to emulation, it will
 *   execute a RETN instruction, which will return control to the next
 *   instruction here.
 *   
 *   - Now we basically need to reverse the steps above.  Start by saving any
 *   registers that you need to copy back from the native environment to the
 *   emulator environment.  The place to save a register is in one of the
 *   stack temp slots you allocated earlier, using [ESP+n] addressing again.
 *   
 *   - Restore the C compiler's saved registers by POPping them off the
 *   stack.  This will restore the compiler's frame pointer, making it safe
 *   to access C local variables once again.  Sigh of relief - things are
 *   almost back to normal!
 *   
 *   - At this point, we can access both the C local variables and the stack
 *   temp slots you allocated.  If you stashed anything important in one of
 *   these stack temp slots that you want to restore to the emulated
 *   environment, move it from the stack temp into a C local variable.  (You
 *   will have to do this with two MOV instructions: MOV EAX, [ESP+n], then
 *   MOV c_local, EAX).
 *   
 *   - Discard the stack temp slots with an ADD ESP, n, where n is the same
 *   number of bytes you used in the SUB ESP, n earlier.
 *   
 *   - Exit the assembly block.  You can now evaluate any complex C
 *   expressions that you need to move updated values from C locals into the
 *   normal emulator environment.
 *   
 *   - Jump back to the appropriate point in the emulator loop with a C
 *   'goto'.
 */
// Sample code illustrating the process described above (from arm7exec.c):
//
// jit_go_native:
// {
// 	// Get the native code pointer and cycle counter into C local variables.  We
//  // do this here because these are complex C expressions that might modify CPU
//  // registers in unpredictable ways.  Locals can be accessed from assembler
//  // directly without modifying any registers, so these act as a bridge between
//  // the current C environment and the assembler environment we're about to
//  // establish.
// 	data32_t tmp1 = (data32_t)JIT_NATIVE(ARM7.jit, pc);
// 	data32_t tmp2 = ARM7_ICOUNT;
// 
// 	__asm {
// 		// Allocate space for temporary variables we'll need while transitioning
//      // between C and assembler (and back).  See the 'IMPORTANT' note below.
// 		// 1 stack DWORD == 4 bytes.
// 		SUB ESP, 4;
// 
// 		// Save registers that the generated code uses and that the C caller
// 		// might expect to be preserved across function calls.  To be robust
//      // across different optimization modes and compiler versions, we will
//      // push all registers that our generated code modifies, except for EAX,
//      // which is fairly certain to be a safe scratch variable for any
//      // compiler using any optimization mode.
// 		PUSH EBX;
// 		PUSH ECX;
// 		PUSH EDX;
// 		PUSH ESI;
// 		PUSH EDI;
// 		
// 		// Get the native code address, and move the cycle counter into EDI for
// 		// use in the translated code.  Note that any register update here could
//      // cause us to lose the C frame pointer and thus lose access to our C
//      // local variables (tmp1, tmp2, etc), except that EAX is safe.  We
//      // deliberately modify EDI last just in case it's the frame pointer.
//      // If we needed to access more than two C locals here, we'd have to move
//      // them all into temp stack slots ([ESP+n] slots) for safe keeping before
//      // moving any of them into registers, to ensure that we don't modify any
//      // registers until all the C locals are safely tucked away somewhere that
//      // we can get to after losing the C frame pointer.
// 		MOV  EAX, tmp1;
// 		MOV  EDI, tmp2;
// 		
// 		// IMPORTANT: don't access any C local variables (tmp1, tmp2, etc) from
// 		// here until after the POPs below.  At least one VC optimization mode uses
// 		// EBX as the frame pointer, and it's possible that other modes or other
// 		// compilers use other registers.  C local access will be safe again
// 		// after the POPs below, which will recover the pre-call register values.
// 		// In the meantime, anything we need to store temporarily must be saved
// 		// explicitly in stack slots allocated with the 'SUB ESP, n' above, and
// 		// addressed explicitly in terms of [ESP+n] addresses.  These are safe
// 		// because we control the stack layout in this section of code.
// 		
// 		// call the native code
// 		CALL EAX;
// 		
// 		// save the new cycle counter from EDI into a stack temp (before we restore
// 		// the pre-call EDI)
// 		MOV  [ESP+20], EDI;
// 		
// 		// restore saved registers - C locals are safe to access again after these
//      // POPs, because the frame pointer will be restored if it was one of these
//      // (and will never have been lost if it wasn't)
// 		POP  EDI;
// 		POP  ESI;
// 		POP  EDX;
// 		POP  ECX;
// 		POP  EBX;
// 		
// 		// move the new PC and cycle count into C locals, so that we can move them
//      // into their real locations below (those might involve C expressions that
//      // could modify registers, so we want them in simple C locals first so
//      // that we can stop caring about any of the registers)
// 		MOV  tmp1, EAX;
// 		POP  tmp2;
// 	}
// 	R15 = tmp1;
// 	ARM7_ICOUNT = tmp2;
// }
// resume emulation
// goto resume_from_jit;

/*
 *   The JIT translator function.  This must be implemented per emulated CPU.
 *   This takes the address of an instruction to translate.
 *   
 *   If the instruction can be translated, this routine generates the
 *   translated native code, stores it via jit_store_native(), and returns
 *   true.  The routine is encouraged to continue translating sequential
 *   instructions - we know that the code at 'pc' is invoked, and that
 *   usually implies that the code sequentially following 'pc' will also be
 *   invoked, at least up to the next unconditional branch.
 *   
 *   If the instruction at 'pc' can't be translated, return false.
 */
#ifdef JIT_NAME
# define JIT_XLAT_FUNC_(x) x ## _jit_xlat
# define JIT_XLAT_FUNC(x) JIT_XLAT_FUNC_(x)
int JIT_XLAT_FUNC(JIT_NAME)(struct jit_ctl *jit, data32_t pc);
#endif

/*
 *   Reserve a contiguous block in the native code page of the given size,
 *   and set the protection attributes on the page to allow writing for the
 *   duration of the oparation.  This guarantees that the next 'len' bytes
 *   stored with jit_store_native() will be in contiguous memory.  Returns a
 *   pointer to the reserved memory.
 */
byte *jit_reserve_native(struct jit_ctl *jit, int len, /*OUT*/ struct jit_page **pgp);

/*
 *   Store a translated block of native code in the JIT executable memory
 *   page.  Returns the address of the stored code.
 */
byte *jit_store_native(struct jit_ctl *jit, const byte *code, int len);

/* Same as jit_store_native but does not try to find a new location for code. Needed for blocks of code with relative jumps */

void jit_store_native_from_reserved(struct jit_ctl *jit, const byte *code, int len, struct jit_page *pg, const byte *dst);


/*
 *   End a store-native operation.  Each call to jit_reserve_native() should
 *   have a corresponding call to jit_close_native().  This restores the page
 *   protection attributes on the executable page.  'addr' is the address
 *   returned from jit_reserve_native(), and 'len' is the length originally
 *   reserved.
 */
void jit_close_native(struct jit_ctl *jit, byte *addr, int len);


/*
 *   Native code memory page structure.  This is an internal allocation block
 *   that the JIT uses to manage memory containing the translated code for an
 *   emulated CPU.
 */
struct jit_page {
	// next page in the list for this CPU
	struct jit_page *nxt;

	// total amount of space on this page
	int siz;

	// offset of next free byte
	int ofsFree;

	// native code (allocated as a separate block via VirtualAlloc())
	byte *b;
};

#else /* JIT_ENABLED */

struct jit_ctl { int foo; };
#define jit_create(icnt) 0
#define jit_create_map(jit, minAddr, maxAddr)
#define jit_set_mem_callbacks(jit, r8, r16, r32, w8, w16, w32)
#define jit_enable(jit)
#define jit_reset(jit)
#define jit_delete(jitp)
#define JIT_FETCH(jit,pc)

#endif /* JIT_ENABLED */

#endif /* INC_JIT */
