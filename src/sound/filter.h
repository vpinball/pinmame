// license:BSD-3-Clause

#ifndef __FILTER_H
#define __FILTER_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "osd_cpu.h"

/* Max FIR filter order */
#define FILTER_ORDER_MAX 501

#if (defined(_M_IX86_FP) && _M_IX86_FP >= 1) || defined(__SSE__) || defined(_M_X64) || defined(_M_AMD64)
 #define SSE_FILTER_OPT
#else
 #pragma message ( "Warning: No SSE optimizations for Filter enabled" )
#endif

typedef struct filter_struct {
	float xcoeffs[(FILTER_ORDER_MAX+1)/2];
	unsigned order;
} filter;

typedef struct filter_state_struct {
	unsigned prev_mac;
	float xprev[FILTER_ORDER_MAX];
} filter_state;

/* Allocate a FIR Low Pass filter */
filter* filter_lp_fir_alloc(double freq, const int order);
/* Free the filter */
void filter_free(filter* f);

/* Allocate a filter state */
filter_state* filter_state_alloc(void);
/* Free the filter state */
void filter_state_free(filter_state* s);

/* Clear the filter state */
void filter_state_reset(filter* f, filter_state* s);

/* Insert a value in the filter state */
INLINE void filter_insert(const filter* f, filter_state* s, const float x) {
	/* next state */
	++s->prev_mac;
	if (s->prev_mac >= f->order)
		s->prev_mac = 0;

	/* set x[0] */
	s->xprev[s->prev_mac] = x;
}

/* Compute the filter output */
float filter_compute(const filter* f, const filter_state* s);

//
// The following IIR filter is much lower computational overhead, but is more restricted to what can be done with filters that are based on it
//

/* Filter types */
#define FILTER_LOWPASS		0
#define FILTER_HIGHPASS		1
#define FILTER_BANDPASS		2

typedef struct filter2_context_struct { // could also be reduced to only 2 history regs: https://en.wikipedia.org/wiki/Digital_biquad_filter#Direct_form_2, but Form 1 has better characteristics apparently
	double /*x0,*/ x1, x2;	/* x[k], x[k-1], x[k-2], current and previous 2 input values */
	double /*y0,*/ y1, y2;	/* y[k], y[k-1], y[k-2], current and previous 2 output values */
	double a1, a2;		/* digital filter coefficients, denominator */
	double b0, b1, b2;	/* digital filter coefficients, numerator */
} filter2_context;


/* Setup the filter context based on the passed filter type info.
 * type - 1 of the 3 defined filter types
 * fc   - center frequency
 * d    - damp = 1/Q
 * gain - overall filter gain. Set to 1 if not needed.
 */
void filter2_setup(const int type, const double fc, const double d, const double gain,
	filter2_context * const __restrict filter2, const double sample_rate);


/* Reset the input/output voltages to 0. */
void filter2_reset(filter2_context * const __restrict filter2);


/* 
 *  Step the filter with a given input, returning the new output.
 */
/* Step the filter with input x0. */
INLINE double filter2_step_with(filter2_context * const __restrict filter2, const double x0)
{
	// Form 1 Biquad Section Calc // Direct Form 1 is usually the best choice for fixed point
	const double y0 = /*a0**/(filter2->b0 * x0 + filter2->b1 * filter2->x1 + filter2->b2 * filter2->x2) - filter2->a1 * filter2->y1 - filter2->a2 * filter2->y2;
	filter2->x2 = filter2->x1;
	filter2->x1 = x0;
	filter2->y2 = filter2->y1;
	filter2->y1 = y0;
	return y0;
}

/* 
 *  Step the filter with a given input, returning the new output.
 */
double filter2_step_with(filter2_context * const __restrict filter2, const double input);


// directly set digital coefficients
void filter_setup(const double b0, const double b1, const double b2, const double a1, const double a2, // a0 = 1
	filter2_context * const __restrict filter2);


/* Set up a filter2 structure based on an op-amp multipole bandpass circuit.
 * NOTE: If r2 is not used then set to 0.
 *       vRef is not needed to setup filter.
 *
 *                             .--------+---------.
 *                             |        |         |
 *                            --- c1    Z         |
 *                            ---       Z r3      |
 *                             |        Z         |
 *            r1               |  c2    |  |\     |
 *   In >----ZZZZ----+---------+--||----+  | \    |
 *                   Z                  '--|- \   |
 *                   Z r2                  |   >--+------> out
 *                   Z                  .--|+ /
 *                   |                  |  | /
 *                  gnd        vRef >---'  |/
 *
 */
void filter_opamp_m_bandpass_setup(const double r1, const double r2, const double r3, const double c1, const double c2,
	filter2_context * const __restrict filter2, const double sample_rate);

// 
// Passive RC low-pass filter (set R2 & R3 = 0 for first variant)
//
//
// Vin --- R1 --- + --- Vout   or    Vin --- R1 --- + --- R2 --- +
//                |                                 |            |
//                C1                                C1           R3 --- Vout
//                |                                 |            |
//               GND                               GND          GND
//
void filter_rc_lp_setup(const double R1, const double R2, const double R3, const double C1,
	filter2_context * const __restrict context, const double sample_rate);


// Multiple Feedback Low-pass Filter
//
// This implements a low-pass filter of the type found in many Williams sound 
// boards.  This is a standard design known as a multiple feedback low pass 
// filter.  It's good for removing high-frequency noise, particularly 
// quantization noise from playing back digitized sounds, so Williams used
// it at the output stage of DACs and HC55516 chips.
//
// You can find these filters in the WPC89 and System 11 schematics.  The
// filter's pass band is a function of the resistor and capacitor values, so
// to model a particular machine's filter, find the corresponding components
// in the machine's sound board schematic and plug in the values to the
// initialization function.
//
//               +--- R3 ---+------------+
//               |          |            |
//               |         C2   |\       |
//               |          |   | \      |
// Vin --- R1 ---+--- R2 ---+---|- \     |
//               |              |   \____|_____ Vout
//               |              |   /
//              C1          +---|+ /
//               |          |   | /
//               |         Rb   |/
//               |          |
//              GND        GND
//
// Circuit variations: In the Williams schematics, Rb might or might not be
// present.  This is a bias correction resistor for the op-amp, which is a
// detail of the analog circuit design that doesn't affect the digital
// version, it can be ignored either way.  The Williams schematics also
// typically place a capacitor in series between Vin and R1.  That's there
// to decouple the DAC output and remove any DC offset (it's essentially a
// high-pass filter with a very low cutoff frequency of 1-10 Hz or so). 
// That's another analog detail we can ignore for the digital version.
//
// In analog form, these filters typically have non-unity gain (that is, 
// they amplify or reduce the signal).  Our digital version of the filter 
// normalizes to unity gain to preserve the caller's control over the gain
// level.
//
// The Williams boards often used two stages of this filter in series.  
// You can do the same thing with the digital filter by setting up two
// virtual filters, and feeding the output of the first stage into the
// input of the second stage.
//
// Specify resistor values in Ohms and capacitors in Farads.
//
void filter_mf_lp_setup(const double R1, const double R2, const double R3, const double C1, const double C2,
	filter2_context * const __restrict context, const double sample_rate);


// Active single-pole low-pass filter
//
// This is another type of active (op-amp) low-pass filter found in some
// Williams sound boards.  This differs only slightly from the multiple 
// feedback filter above: the only difference is that C1 from the MF filter
// (the capacitor that connects the input signal to ground) isn't present.
//
//               +--- R3 ---+------------+
//               |          |            |
//               |         C1   |\       |
//               |          |   | \      |
// Vin --- R1 ---+--- R2 ---+---|- \     |
//                              |   \____|_____ Vout
//                              |   /
//                          +---|+ /
//                          |   | /
//                         Rb   |/
//                          |
//                         GND
//
// As with the MF filter, Rb isn't needed for the digital filter, and any
// capacitor between Vin and R1 can be ignored.  Any DC gain in the analog
// incarnation is normalized to unity gain in the digital simulation.
//
// Specify resistor values in Ohms and capacitors in Farads.
//
void filter_active_lp_setup(const double R1, const double R2, const double R3, const double C1,
	filter2_context * const __restrict context, const double sample_rate);

// Sallen-Key low-pass filter
//
// This is yet another type of active low-pass filter.  This one is found
// in early Bally voice games (Xenon), and is also used in the DCS boards.
// This is similar to the multiple feedback filter, but has the special 
// feature of unity gain (that is, no amplification or attenutation).
//
//               +------------ C1 ------------+
//               |                            |
//               |                   |\       |
//               |                   | \      |
// Vin --- R1 ---+--- R2 ---+--------|+ \     |
//                          |        |   \----+-+------ Vout
//                          |        |   /    |
//                         C2    +---|- /     |
//                          |    |   | /      |
//                          |    |   |/       |
//                         GND   |            |
//                               +------------+
//
// Specify resistor values in Ohms and capacitors in Farads.
//
void filter_sallen_key_lp_setup(const double R1, const double R2, const double C1, const double C2,
	filter2_context * const __restrict context, const double sample_rate);

#endif
