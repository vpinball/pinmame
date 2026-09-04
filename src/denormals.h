#ifndef INC_DENORMALS
#define INC_DENORMALS

/*
  Put the calling thread's FPU into flush-to-zero mode.

  Nearly every recursive filter in the sound cores is a float/double IIR whose
  input goes to 0 when a machine falls silent, but such a filter must not
  settle at 0 in practice.  A one-pole decay stalls as soon as the decrement rounds below half a
  denormal ULP - the state then sits at e.g. ~5e-324 forever - and a two-pole section drops
  into a limit cycle down there instead.  Measured stuck this way: the DAC and AY8910
  DC blockers, HC55516's and MC3417's output biquads, the DCS DAC's, plus the discrete
  system's dst_filter1/filter2/rcfilter/crfilter/rcdisc* and both CVSD cores' syllabic
  and integrator recursions.  Every sample after that point costs a denormal multiply,
  measured at ~23x a normal one, on a modern x86-64 setup, and far worse on cores that trap denormals
  to software support code - e.g. phones and SBCs.

  Note this is thread-local state on x86 (MXCSR) and on ARM (FPCR/FPSCR), so it has to
  be set on whichever thread actually runs the DSP - hence the call in run_game(),
  which every front end enters on its emulation thread.

  NOTE the one place that is worth knowing about: the i386 core, which shares this
  thread.  Its x87 is softfloat (floatx80), so that side is untouched, but its SSE ops
  are implemented with host float arithmetic - sse_addps() and friends in pentops.hxx
  literally do XMM(a).f[0] + XMM(b).f[0].  With FTZ/DAZ set, an emulated SSE denormal
  is therefore flushed regardless of what the guest's own emulated MXCSR asks for.
  That is harmless as things stand, because the Pinball 2000 hardware is a Pentium MMX
  and has no SSE for the game code to use, but it would stop being harmless if this
  core were ever pointed at guest code that does!

  x86:
    FTZ (MXCSR bit 15): flushes denormal *results* to zero
    DAZ (MXCSR bit  6): additionally treats denormal *inputs* as zero
  On ARM there is no such split - FPCR/FPSCR.FZ already forces denormal operands as well as results to zero.

  Not all platforms support it/both, but for all modern ones its fine
*/

#if defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(__amd64__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1) || defined(__SSE__) || defined(__SSE2__)
  #define DENORMALS_VIA_MXCSR 1
  #include <xmmintrin.h>
  #include <string.h>
  #if defined(_MSC_VER)
    #include <intrin.h>
  #endif
#elif defined(_MSC_VER) && (defined(_M_ARM64) || defined(_M_ARM))
  #define DENORMALS_VIA_CONTROLFP 1
  #include <float.h>
#endif

#if defined(DENORMALS_VIA_MXCSR)
/* Does this CPU implement MXCSR.DAZ?  A zero MXCSR_MASK means the legacy default of 0xFFBF, in which DAZ (bit 6) is clear. */
static int denormals_daz_supported(void)
{
#if defined(_MSC_VER)
	__declspec(align(16)) unsigned char buf[512];
#elif defined(__GNUC__) || defined(__clang__)
	unsigned char buf[512] __attribute__((aligned(16)));
#else
	return 0; /* no way to run FXSAVE portably here; skip DAZ, keep FTZ */
#endif
#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
	{
		unsigned int mask;
		memset(buf, 0, sizeof(buf));
#if defined(_MSC_VER)
		_fxsave(buf);
#else
		__asm__ __volatile__("fxsave %0" : "=m"(*(char(*)[512])buf));
#endif
		memcpy(&mask, buf + 28, sizeof(mask)); /* MXCSR_MASK */
		if (mask == 0u)
			mask = 0xFFBFu;
		return (mask & 0x0040u) != 0u;
	}
#endif
}
#endif

static void set_denormals_flush_to_zero(void)
{
#if defined(DENORMALS_VIA_MXCSR)
	unsigned int csr = _mm_getcsr() | 0x8000u; /* FTZ, always available with SSE */
	if (denormals_daz_supported())
		csr |= 0x0040u;                        /* DAZ, only where implemented */
	_mm_setcsr(csr);

#elif defined(DENORMALS_VIA_CONTROLFP)
	/* ARM under MSVC: FZ covers operands and results alike, and there is no DAZ bit to probe for */
	unsigned int old;
	_controlfp_s(&old, _DN_FLUSH, _MCW_DN);

#elif defined(__aarch64__)
	unsigned long long fpcr;
	__asm__ __volatile__("mrs %0, fpcr" : "=r"(fpcr));
	fpcr |= (1ull << 24); /* FPCR.FZ - operands and results both */
	__asm__ __volatile__("msr fpcr, %0" : : "r"(fpcr));

/* __ARM_FP (ACLE) is the test for "VFP instructions are usable", and is absent under
   -mfloat-abi=soft.  __VFP_FP__ is NOT usable for this - clang defines it even for
   soft-float, where these two instructions then fail to assemble.  __ARM_PCS_VFP is
   kept only as a fallback for toolchains predating __ARM_FP; it means hard-float ABI,
   so it is narrower (it misses softfp) but never wrong */
#elif defined(__arm__) && (defined(__ARM_FP) || defined(__ARM_PCS_VFP))
	unsigned int fpscr;
	__asm__ __volatile__("vmrs %0, fpscr" : "=r"(fpscr));
	fpscr |= (1u << 24); /* FPSCR.FZ, present since VFPv2 - so a Pi Zero too */
	__asm__ __volatile__("vmsr fpscr, %0" : : "r"(fpscr));

#else
	/* nothing safe to set here; the filters still work, just slower once a machine goes quiet */
#endif
}

#endif /* INC_DENORMALS */
