#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <Windows.h>

#include "memory.h"

#define JIT_OPALIGN 0
#include "jit.h"

#if JIT_ENABLED

#if JIT_DEBUG
//
// debug mode
//

// enable ASSERTions
#define ASSERT(x) assert(x)

// In debug mode, set generated code pages to EXECUTE+READ mode only (no write access)
// during normal execution.  This protects our generated code against stray pointer overwrites
// when we're not explicitly generating code, which can be helpful in isolating bugs that
// corrupt random memory by causing a hard memory fault if we try to write a code page in
// error.  We only use this mode during debugging, because it costs us some extra time when
// we generate new JIT code, because we have to make a couple of Windows API calls to change
// the memory protection for the memory holding the generated code (to make it writable, then
// set it back to execute-only).
#define DbgVirtualProtect(addr, len, mode, pOldMode) { BOOL VPres = VirtualProtect(addr, len, mode, pOldMode); assert(VPres != 0); }

#else
//
// Release mode
//

// turn off assertions
#define ASSERT(x)

// Release mode - keep the code page in EXECUTE+READ+WRITE mode all the time.  This makes
// write access during code generation faster, the trade-off being that it exposes generated
// code pages to stray pointer overwrites.  That's only a problem if there are bugs, and
// release code *should* be bug-free, so...
#define DbgVirtualProtect(addr, len, mode, pOldMode) (*(pOldMode) = 0)

#endif // JIT_DEBUG

static struct jit_page *jit_add_page(struct jit_ctl *jit, int min_siz);
static byte *emit_lookup_code(struct jit_ctl *jit, int patch);
static void init_code_pages(struct jit_ctl *jit);
static void delete_code_pages(struct jit_ctl *jit);




// create the JIT control structure
struct jit_ctl *_jit_create(int *cycle_counter, int rshift)
{
	// allocate and initialize the control structure, including extra space for the jit_private
	// structure after the end of the main jit_ctl portion
	struct jit_ctl *jit = (struct jit_ctl *)malloc(sizeof(struct jit_ctl));
	jit->minAddr = 0;
	jit->maxAddr = 0;
	jit->native = 0;
	jit->rshift = rshift;
	jit->pages = 0;
	jit->mem_count = 0;
	jit->cycle_counter_ptr = cycle_counter;

	// initialize the native code page list
	init_code_pages(jit);

	// return the new control structure
	return jit;
}

// Reset the JIT
void jit_reset(struct jit_ctl *jit)
{
	int i;
	int nAddrs;

	// Forget all previous translation, and reset all addresses to 'emulate'.
	// This will disable new translation until we get the go-ahead from the 
	// emulator, via a call to jit_enable().
	nAddrs = (jit->maxAddr - jit->minAddr) >> jit->rshift;
	for (i = 0 ; i < nAddrs ; ++i)
		jit->native[i] = jit->pEmulate;

	// delete all native code pages
	delete_code_pages(jit);

	// re-initialize the code page list
	init_code_pages(jit);
}

// initialize the code page list
static void init_code_pages(struct jit_ctl *jit)
{
	byte *res, *p;
	int reslen;
	byte retn[] = { 0xC3 };  // RETN instruction
	
	// allocate the first page for native code storage (use the default length)
	jit_add_page(jit, 0);

	// reserve space for the boilerplate code
	p = res = jit_reserve_native(jit, reslen = 16, 0);

	// Create the special 'pending' and 'emulate' pointers.  Native code
	// can jump to any address in the mapping array, so every entry must
	// be populated with a pointer to valid code.  For any opcode that
	// hasn't been or can't be translated, a native code jump to that
	// location must return to the emulator.  So the 'pending' and 'emulate'
	// pointers must both simply point to native RETN instruction.  The
	// two pointer values must be distinct, though, because the emulator
	// uses the two values to determine whether or not to attempt
	// translation when it encounters an untranslated opcode.  So we
	// need to set up a separate RETN instruction for each one.
	jit->pPending = p = jit_store_native(jit, retn, 1) + 1;
	jit->pEmulate = p = jit_store_native(jit, retn, 1) + 1;

	// Generate a similar special location for the 'working' state.  This is
	// really just a distinguished pointer value; the code at the other end
	// of the pointer shouldn't matter since it should never be invoked.  The
	// working state can only exist while the translator is running, and that
	// has to complete before we resume execution of any emulated or translated
	// code.
	jit->pWorking = p = jit_store_native(jit, retn, 1) + 1;

	// close the reserved space
	jit_close_native(jit, res, reslen);

	// Create the emulated address lookup code.  This is a routine that
	// generated code can invoke to look up an emulated address at run-time
	// and jump to it, or return to the emulator if no native code is
	// available.  To invoke this code, the generated code loads the
	// target emulated address into EAX and jumps to jit->pLookup.
	jit->pLookup = emit_lookup_code(jit, 0);
	jit->pLookupPatch = emit_lookup_code(jit, 1);
}

// Run-time address lookup.  This is invoked from generated code to
// find the native code for an emulated address.  See emit_lookup_code()
// for how this is invoked.
static byte *rtlookup(struct jit_ctl *jit, data32_t addr)
{
	// if it's not a valid address, return to the emulator
	if (addr < jit->minAddr || addr > jit->maxAddr)
		return jit->pEmulate;

	// if we're out of cycles, return to the emulator
	if (*jit->cycle_counter_ptr == 0)
		return jit->pEmulate;

	// look up the address
	return JIT_NATIVE(jit, addr);
}

// Run-time address lookup and patch.  This is invoked from generated
// code to find the native code for an emulated address, and then patch
// the caller if the code has been translated.
static byte *rtlookup_patch(struct jit_ctl *jit, data32_t addr, byte *caller)
{
	// look up the address
	byte *nat = rtlookup(jit, addr);

	// If it's been translated, patch the calling code.  The calling code
	// will always look like this:
	//
	//    B8 imm32    MOV EAX, emuaddr
	//    E8 ofs32    CALL patchLookup
	//
	// We want to replace both instructions with a jump to the translated address,
	// so go back 10 bytes and replace the MOV.
	if (nat != jit->pEmulate && nat != jit->pPending)
	{
		DWORD prvPro;
		BOOL res;

		// back up the caller address to the MOV instruction
		caller -= 10;
		ASSERT(caller[0] == 0xB8 && caller[5] == 0xE8);  // MOV, CALL

		// make the code page temporarily writable
		DbgVirtualProtect(caller, 10, PAGE_EXECUTE_READWRITE, &prvPro);

		// patch the MOV with JMP ofs32
		caller[0] = 0xE9;       // JMP ofs32
		*(UINT32 *)&caller[1] = (UINT32)(nat - (caller+5));

		// restore the old page protection
		DbgVirtualProtect(caller, 10, prvPro, &prvPro);

		// flush the CPU instruction cache for the area where the new code resides
		res = FlushInstructionCache(GetCurrentProcess(), caller, 10);
		ASSERT(res != 0);
	}

	// return the native address to invoke
	return nat;
}

// Emit the pLookup code.  When the generated code encounters a jump to an
// address that hasn't been translated yet, it loads EAX with the destination
// emulator address (the target of the Branch instruction in the original
// emulator code) and jumps directly to pLookup.  The pLookup routine does
// a run-time lookup on the emulator address to see if it's been converted
// to generated code yet.  If so, the pLookup routine simply transfers control
// directly to the generated code.  This allows relatively fast calling from
// one native routine to another when the target was translated later than
// the caller.  If the target routine hasn't been translated, pLookup simply
// returns to the emulator with EAX still loaded with the target emulator
// address.  This allows the emulator to either do an on-demand translation
// of the target code, or simply emulate the target code.
//
// If 'patch' is true, we'll generate the version of the routine for
// pLookupPatch, which patches the calling code with a direct jump to the
// target address when the translation is successful.
static byte *emit_lookup_code(struct jit_ctl *jit, int patch)
{
	// The generated code looks like this.  "P" in the first columns means
	// that this is generated for the "patch" version only.
	//
	// P  5A           POP EDX         ; pop the caller address, for patching
	//    50           PUSH EAX        ; push the address to look up, to save
	// P  52           PUSH EDX        ; push the caller address, as an argument to lookup()
	//    50           PUSH EAX        ; push the address to look up, as an argument to lookup()
	//    68 <jit>     PUSH <jit>      ; put the jit_ctl pointer, as an argument to lookup>()
	//    E8 <lookup>  CALL <lookup>   ; call lookup()
	//    83 C4 nn     ADD SP,n        ; discard arguments
	//    5A           POP EDX         ; pop the saved destination address
	//    92           XCHG EAX,EDX    ; get the destination address back into EAX
	//    FF E0        JMP EDX         ; jump to the translated address
	//
	// Note that the reason we have to generate this code rather than use
	// a static handler is that we won't have the 'jit' structure pointer
	// stashed anywhere that generated code can reach.  We need that to
	// do the address lookup.  So we just need to generate this little bit
	// of glue that loads up the 'jit' pointer and calls a static handler
	// that takes that as an argument.
	//
	// If the emulator address we're jumping to has already been translated,
	// rtlookup will return its native address, so we simply want to jump there.
	// If the address hasn't been translated yet, rtlookup will return pEmulate,
	// which is native code that simply returns to the emulator.  So in either
	// case we simply jump to the address returned.  In the pEmulate case, we
	// must load EAX with the target emulator address first; doing this in the
	// case where the code has already been translated is harmless, so simply
	// load EAX with the target address in all cases.

	byte pushEAX[] = { 0x50 };               // PUSH EAX
	byte pushJit[] = { 0x68, 0, 0, 0, 0 };   // PUSH Imm32
	byte callLk[]  = { 0xE8, 0, 0, 0, 0 };   // CALL Ofs32
	byte addSp8[]  = { 0x83, 0xC4, 0x08 };   // ADD SP,8
	byte addSp12[] = { 0x83, 0xC4, 0x0C };   // ADD SP,12
	byte pushEDX[] = { 0x52 };               // PUSH EDX
	byte popEDX[]  = { 0x5A };               // POP EDX
	byte xchgED[]  = { 0x92 };               // XCHG EAX,EDX
	byte jmpEDX[]  = { 0xFF, 0xE2 };         // JMP EDX
	byte *code, *p;
	byte *lookup = (patch ? (byte *)rtlookup_patch : (byte *)rtlookup);
	int reslen;

	// reserve space for the handler code
	code = p = jit_reserve_native(jit, reslen = 64, 0);

	// if patching, pop the caller address into EDX
	if (patch)
		p = jit_store_native(jit, popEDX, 1) + 1;

	// PUSH EAX (save the target emu address)
	p = jit_store_native(jit, pushEAX, 1) + 1;

	// if patching, push the additional caller address argument from EDX
	if (patch)
		p = jit_store_native(jit, pushEDX, 1) + 1;

	// PUSH EAX (target emu address argument)
	p = jit_store_native(jit, pushEAX, 1) + 1;

	// PUSH <jit> ('jit' structure pointer argument)
	*(UINT32 *)(&pushJit[1]) = (UINT32)jit;
	p = jit_store_native(jit, pushJit, 5) + 5;

	// generate the CALL to the static handler
	*(UINT32 *)(&callLk[1]) = (UINT32)(lookup - (p+5));
	p = jit_store_native(jit, callLk, 5) + 5;

	// discard arguments
	p = jit_store_native(jit, patch ? addSp12 : addSp8, 3) + 3;

	// POP EDX (restore the saved target emu address)
	p = jit_store_native(jit, popEDX, 1) + 1;

	// XCHG EAX,EDX (get the target emu address into EAX)
	p = jit_store_native(jit, xchgED, 1) + 1;

	// JMP EDX (jump to the translated address)
	p = jit_store_native(jit, jmpEDX, 2) + 2;

	// end the store-native operation
	jit_close_native(jit, code, reslen);
	
	// return the generated code pointer
	return code;
}

// free the native code page list
static void delete_code_pages(struct jit_ctl *jit)
{
	// delete each page in the list
	struct jit_page *p = jit->pages;
	while (p != 0)
	{
		// remember the next page
		struct jit_page *nxt = p->nxt;

		// free the code space
		BOOL res = VirtualFree(p->b, 0, MEM_RELEASE);
		ASSERT(res != 0);

		// free the page descriptor
		free(p);

		// move on
		p = nxt;
	}

	// clear the list head pointer
	jit->pages = 0;

	// the internal native code pointers are no longer valid
	jit->pEmulate = 0;
	jit->pPending = 0;
	jit->pWorking = 0;
	jit->pLookup = 0;
	jit->mem_count = 0;
}

void jit_create_map(struct jit_ctl *jit, data32_t minAddr, data32_t maxAddr)
{
	int i, nBytes, nAddrs;
	
	// if there's an existing map, delete it
	if (jit->native != 0)
		free(jit->native);

	// figure the size in bytes of the address space (NB: the range is
	// exclusive of maxAddr)
	nBytes = maxAddr - minAddr;

	// figure the size in indices of the address space, taking into account
	// that we only store every other address for 2-byte alignment, every 4th
	// address for 4-byte alignment, etc
	nAddrs = nBytes >> jit->rshift;

	// store the new range parameters
	jit->minAddr = minAddr;
	jit->maxAddr = maxAddr;

	// allocate the new mapping array
	jit->native = (byte **)malloc(nAddrs * sizeof(byte *));
	memset(jit->native, 0, nAddrs * sizeof(byte *));

	// initialize the entire opcode address space to 'emulate', to disable
	// translation until the bootstrapping process is finished
	for (i = 0 ; i < nAddrs ; ++i)
		jit->native[i] = jit->pEmulate;
}

void jit_set_mem_callbacks(
	struct jit_ctl *jit,
	data8_t (*read8)(int addr),
	data16_t (*read16)(int addr),
	data32_t (*read32)(int addr),
	void (*write8)(int addr, data8_t data),
	void (*write16)(int addr, data16_t data),
	void (*write32)(int addr, data32_t data))
{
	jit->read8 = (byte *)read8;
	jit->read16 = (byte *)read16;
	jit->read32 = (byte *)read32;
	jit->write8 = (byte *)write8;
	jit->write16 = (byte *)write16;
	jit->write32 = (byte *)write32;
}

void jit_enable(struct jit_ctl *jit)
{
	// figure the size of the map array
	int nBytes = jit->maxAddr - jit->minAddr;
	int nAddrs = nBytes >> jit->rshift;
	int i;

	// set each address that's currently set to 'emulate' to 'pending'
	for (i = 0 ; i < nAddrs ; ++i) {
		if (jit->native[i] == jit->pEmulate)
			jit->native[i] = jit->pPending;
	}
}

void jit_untranslate(struct jit_ctl *jit, data32_t addr)
{
	byte *p;

	// if it's not in the JIT covered memory space, there's nothing to do
	if (addr < jit->minAddr || addr >= jit->maxAddr)
		return;

	//if (addr >= 0x97f0 || addr <= 0x98ff)  // WTF? 
	//	return;

	// un-translate only if the existing opcode is translated
	p = JIT_NATIVE(jit, addr);
	if (p != jit->pEmulate && p != jit->pPending)
	{
		BOOL res;

		// Replace the code with MOV EAX,<emulator address>, RETN.
		// This will return to the emulator and resume emulation at the
		// replaced code address.
		p[0] = 0xB8;                 // MOV EAX,Imm32
		*(UINT32 *)(&p[1]) = addr;   // ... the immediate data for the MOV
		p[5] = 0xC3;                 // RETN

		// Set the opcode mapping to 'emulate', so that we don't try
		// to translate it again in the future.  Once a location is written,
		// we'll assume that it will be written again, so we don't want to
		// waste time and memory on repeated translations that will just
		// be undone.
		jit->native[(addr - jit->minAddr) >> jit->rshift] = jit->pEmulate;

		// flush the instruction cache for this section of code
		res = FlushInstructionCache(GetCurrentProcess(), p, 128); //!! 128?!
		ASSERT(res != 0);
	}
}

void jit_delete(struct jit_ctl **jit)
{
	// delete all code pages
	delete_code_pages(*jit);

	// free the mapping array
	if ((*jit)->native != 0)
		free((*jit)->native);

	// free the control structure
	free(*jit);
	*jit = 0;
}

byte *jit_reserve_native(struct jit_ctl *jit, int len, struct jit_page **pgp)
{
	struct jit_page *pg;
	byte *res;
	DWORD prvPro;

	// find an existing page with space for the new code
	for (pg = jit->pages ; pg != 0 && pg->siz - pg->ofsFree < len ; pg = pg->nxt) ;

	// if that failed, allocate a new page with at least the required space
	if (pg == 0) {
		pg = jit_add_page(jit, len);
		if (pg == 0 || pg->siz - pg->ofsFree < len)
			return 0;
	}

	// give the caller the page reference if desired
	if (pgp != 0)
		*pgp = pg;

	// figure the reserved area pointer
	res = pg->b + pg->ofsFree;

	// open this memory to writing
	DbgVirtualProtect(res, len, PAGE_EXECUTE_READWRITE, &prvPro);

	// return the destination pointer
	return pg->b + pg->ofsFree;
}

void jit_close_native(struct jit_ctl *jit, byte *addr, int len)
{
	DWORD prvPro;

	// make the reserved memory executable and non-writable
	DbgVirtualProtect(addr, len, PAGE_EXECUTE_READ, &prvPro);
}

byte *jit_store_native(struct jit_ctl *jit, const byte *code, int len)
{
	// reserve space
	struct jit_page *pg;
	byte *dst = jit_reserve_native(jit, len, &pg);

	// copy the data, if any
	if (len != 0)
	{
		BOOL res;

		// store the instruction data
		memcpy(dst, code, len);

		// consume the space
		pg->ofsFree += len;
		
		// flush the CPU instruction cache for the area where the new code resides
		res = FlushInstructionCache(GetCurrentProcess(), dst, len);
		ASSERT(res != 0);
	}

	// return the new code address
	return dst;
}

void jit_store_native_from_reserved(struct jit_ctl *jit, const byte *code, int len, struct jit_page *pg, const byte *dst)
{
	// copy the data, if any
	if (len != 0)
	{
		BOOL res;

		// store the instruction data
		memcpy(dst, code, len);

		// consume the space
		pg->ofsFree += len;
		
		// flush the CPU instruction cache for the area where the new code resides
		res = FlushInstructionCache(GetCurrentProcess(), dst, len);
		ASSERT(res != 0);
	}
}

static struct jit_page *jit_add_page(struct jit_ctl *jit, int min_siz)
{
#if JIT_DEBUG
	DWORD prvPro;
	BOOL res;
#endif
	int siz;
	struct jit_page *p;

	// Figure the page size.  Allocate at least the minimum size requested
	// (plus the header structure overhead), or a default minimum if they didn't
	// request more.
	//
	// [TO DO!  The mystery factor of 17 should be removed as soon as possible.
	// But read the note below before doing so.
	// 
	// Note: the factor of 17 was an attempt to fix a jit crash bug, but it
	// unfortunately didn't help.  There's a mysterious crash that shows up on
	// some people's machines in specific configurations.  It's so configuration-
	// dependent that none of the developers have been unable to reproduce so far.
	// The bug remains a mystery as of this writing (8/2019).  In my estimation
	// it's some kind of stray pointer error, but beyond that I don't have any
	// more specific theory.  I'm leaving the factor of 17 in place for now,
	// NOT because it does any good towards solving the bug (it doesn't), but
	// rather because the bug is so configuration-sensitive that it's certainly
	// sensitive to this x17 randomness, just like it's sensitive to whether you
	// launch a VP game by pressing F5 or by double-clicking in Explorer.  If
	// I removed the x17, the bug would suddenly disappear on one person's
	// machine and show up on some random other person's machine.  If I changed
	// it to x19 it would pop up on a third random person's machine.  So I'm
	// just leaving bad enough alone because a crappy status quo is always
	// better than some fresh hell.  But if and when anyone ever manages to 
	// track down the actual problem, the factor of 17 should be removed post
	// haste, and the minimum allocation size should be restored to some more 
	// reasonable amount of memory, like the original 128K. 
	//
	// (The main reason that the smaller allocation unit size should be restored
	// isn't that 2GB vs 128K is all that big a deal these days.  Rather, it's
	// for the sake of exercising the code and making it more solid over time.
	// To that end, it would be BETTER to have MORE fragmentation, to give the 
	// assumptions behind this aspect of the JIT design more exercise.  I don't
	// think there are in fact any bugs related to multiple memory allocations, 
	// but if there are, they sure won't be easy to find if we're not exercising 
	// this part of the code.  Any such bugs will just stay in there longer.
	// Another reason to remove the x17 is just plain aesthetics; this kind of
	// abortive thought-it-might-help-but-didn't-oh-well-can't-change-it-now 
	// cruft always becomes a permanent relic in open source projects, and it's 
	// just sloppy.)
	//
	// The original attempt at the factor of 17 wasn't just random shooting in
	// the dark, by the way.  It's worth understanding why it was tried and why
	// it does no good.  The original intent was stated as:  "Make the code 
	// page artificially 17x larger to avoid unsafe jumps and memory references 
	// later".  What he meant by "unsafe jumps" was a JMP instruction that
	// tried to reference a location further than 2GB away in absolute memory.
	// The JMP instructions we generate use self-relative addressing, meaning
	// that they express the target address as an offset from the address of
	// the JMP itself.  The x17 coder's reasoning was thus: suppose that we
	// have allocation unit A at some very low address in memory, and then
	// some time later we have allocation unit B at some very high address in
	// memory, more than 2GB higher than unit A's location.  Now suppose that 
	// unit A contains a JMP that resolves to a target address in unit B.  The
	// JMP, being self-relative, would have to express an offset greater than
	// 2GB because of the distance between the two locations.  That's impossible
	// (the original coder's reasoning continues) because the self-relative 
	// addressing mode uses a signed 32-bit offset, which is limited to 
	// +/- 2GB distances, ergo the JMP instruction generated will be invalid
	// and will cause a crash.  The error in this reasoning is that the self-
	// relative addressing mode uses signed offsets.  It ACTS like the offsets
	// are signed, but in fact the calculation is actually done by treating
	// the address and offset as unsigned values, with the result taken
	// modulo 2^32.  It's the mod 2^32 step that makes the offset act like a
	// signed value when it's convenient for it to look that way - e.g., when
	// you're trying to address a target 5 bytes below the JMP.  In that
	// case you can look at the offset as a 2's complement signed value of -5,
	// but that's just a matter of convenience; you can also look at it as
	// adding unsigned 0xFFFFFFFB and taking the result mod 2^32.  The 2's 
	// complement representation has become the almost universal signed int
	// format precisely because it has this equivalence.  In this case, though,
	// it's confusing and misleading to think about the offset as signed,
	// because it leads you to this erroneous notion that you can't use it to 
	// express offsets over +/- 2GB.  It's better to think about it in terms
	// of what's really going on inside the CPU - unsigned addition mod 2^32.
	// That leads to the correct conclusion that a 32-bit self-relative JMP 
	// can reach any address in the 4GB space, no matter how far away.  So
	// this is why the x17 never fixed anything - there never was such a
	// thing as a JMP that was too far away, so there was no point in making
	// JMP targets closer.
	//
	// --mjr]
	min_siz += sizeof(struct jit_page);
	siz = 128*1024*17;
	if (siz < min_siz)
		siz = min_siz;

	// create the page descriptor structure
	p = (struct jit_page *)malloc(sizeof(struct jit_page));
	p->siz = siz;
	p->ofsFree = 0;

	// link it in at the head of the page list
	p->nxt = jit->pages;
	jit->pages = p;

	// allocate the code space
	p->b = (byte *)VirtualAlloc(0, siz, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	ASSERT(p->b != NULL);

#if JIT_DEBUG
	res = FlushInstructionCache(GetCurrentProcess(), p->b, siz);
	ASSERT(res != 0);

	// make the code space non-accessable (jit_reserve_native will redo it later-on on its own) 
	DbgVirtualProtect(p->b, siz, PAGE_NOACCESS, &prvPro);
#endif

	// return the new page pointer
	jit->mem_count+=siz;
	return p;
}

#endif /* JIT_ENABLED */
