// license:BSD-3-Clause

#include "filter.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#if defined(SSE_FILTER_OPT)
 #include <xmmintrin.h>
 #if !defined(_MSC_VER) || !defined(_WIN32) || defined(__clang__)
     typedef union __attribute__ ((aligned (16))) Windows__m128
     {
         __m128 v;
         float m128_f32[4];
     } Windows__m128;
 #else
     #define USE_WINDOWS_CODE
 #endif
#endif

static filter* filter_alloc(void) {
	filter* f = malloc(sizeof(filter));
	return f;
}

void filter_free(filter* f) {
	free(f);
}

void filter_state_reset(filter* f, filter_state* s) {
	unsigned int i;
	s->prev_mac = 0;
	for(i=0;i<f->order;++i)
		s->xprev[i] = 0;
}

filter_state* filter_state_alloc(void) {
	unsigned int i;
	filter_state* s = malloc(sizeof(filter_state));
	s->prev_mac = 0;
	for(i=0;i<FILTER_ORDER_MAX;++i)
		s->xprev[i] = 0;
	return s;
}

void filter_state_free(filter_state* s) {
	free(s);
}

/****************************************************************************/
/* FIR */

#if defined(SSE_FILTER_OPT)
INLINE __m128 horizontal_add(const __m128 a)
{
#if 0 //!! needs SSE3
    const __m128 ftemp = _mm_hadd_ps(a, a);
    return _mm_hadd_ps(ftemp, ftemp);
#else    
    const __m128 ftemp = _mm_add_ps(a, _mm_movehl_ps(a, a)); //a0+a2,a1+a3
    return _mm_add_ss(ftemp, _mm_shuffle_ps(ftemp, ftemp, _MM_SHUFFLE(1, 1, 1, 1))); //(a0+a2)+(a1+a3)
#endif
}
#endif

float filter_compute(const filter* f, const filter_state* s) {
	const float * const __restrict xcoeffs = f->xcoeffs;
	const float * const __restrict xprev = s->xprev;

	const int order = f->order;
	const int midorder = f->order / 2;
#ifdef SSE_FILTER_OPT
	__m128 y128;
	float
#else
	double
#endif
	y = 0;
	int i,j,k;

	/* i == [0] */
	/* j == [-2*midorder] */
	i = s->prev_mac;
	j = i + 1;
	if (j == order)
		j = 0;

	/* x */
	k = 0;

#if defined(SSE_FILTER_OPT)
        y128 = _mm_setzero_ps();
        for (; k<midorder-3; k+=4) {
            __m128 coeffs = _mm_loadu_ps(xcoeffs + (midorder - (k + 3)));

#ifdef USE_WINDOWS_CODE
            __m128 xprevj,xprevi;
#else
            Windows__m128 xprevj,xprevi;
#endif
            if (j + 3 < order)
            {
#ifdef USE_WINDOWS_CODE
                xprevj = _mm_loadu_ps(xprev + j);
                xprevj = _mm_shuffle_ps(xprevj, xprevj, _MM_SHUFFLE(0, 1, 2, 3));
#else
                xprevj.v = _mm_loadu_ps(xprev + j);
                xprevj.v = _mm_shuffle_ps(xprevj.v, xprevj.v, _MM_SHUFFLE(0, 1, 2, 3));
#endif
                j += 4;
                if (j == order)
                    j = 0;
            }
            else
            {
                xprevj.m128_f32[3] = xprev[j];
                ++j;
                if (j == order)
                    j = 0;

                xprevj.m128_f32[2] = xprev[j];
                ++j;
                if (j == order)
                    j = 0;

                xprevj.m128_f32[1] = xprev[j];
                ++j;
                if (j == order)
                    j = 0;

                xprevj.m128_f32[0] = xprev[j];
                ++j;
                if (j == order)
                    j = 0;
            }

            if (i - 3 >= 0)
            {
#ifdef USE_WINDOWS_CODE
                xprevi = _mm_loadu_ps(xprev + (i-3));
#else
                xprevi.v = _mm_loadu_ps(xprev + (i-3));
#endif
                i -= 4;
                if (i == -1)
                    i = order - 1;
            }
            else
            {
                xprevi.m128_f32[3] = xprev[i];
                if (i == 0)
                    i = order - 1;
                else
                    --i;

                xprevi.m128_f32[2] = xprev[i];
                if (i == 0)
                    i = order - 1;
                else
                    --i;

                xprevi.m128_f32[1] = xprev[i];
                if (i == 0)
                    i = order - 1;
                else
                    --i;

                xprevi.m128_f32[0] = xprev[i];
                if (i == 0)
                    i = order - 1;
                else
                    --i;
            }

#ifdef USE_WINDOWS_CODE
            y128 = _mm_add_ps(y128,_mm_mul_ps(coeffs,_mm_add_ps(xprevi, xprevj)));
#else
            y128 = _mm_add_ps(y128,_mm_mul_ps(coeffs,_mm_add_ps(xprevi.v, xprevj.v)));
#endif
        }
#endif

        for (; k < midorder; ++k) {
            y += xcoeffs[midorder - k] * (xprev[i] + xprev[j]);

            ++j;
            if (j == order)
                j = 0;
            if (i == 0)
                i = order - 1;
            else
                --i;
        }

        y += 
#if defined(SSE_FILTER_OPT)
			_mm_cvtss_f32(horizontal_add(y128)) +
#endif
			xcoeffs[0] * xprev[i];

	return (float)y;
}

filter* filter_lp_fir_alloc(double freq, const int order) {
	filter* f = filter_alloc();
	const unsigned midorder = (order - 1) / 2;
	unsigned i;
	double gain;
	double xcoeffs[(FILTER_ORDER_MAX+1)/2];

	assert( order <= FILTER_ORDER_MAX );
	assert( order % 2 == 1 );
	assert( 0 < freq && freq <= 0.5 );

	/* Compute the antitransform of the perfect low pass filter */
	gain = 2.*freq;
	xcoeffs[0] = gain;

	freq *= 2.*M_PI;

	for(i=1;i<=midorder;++i) {
		/* number of the sample starting from 0 to (order-1) included */
		const unsigned n = i + midorder;

		/* sample value */
		double c = sin(freq*i) / (M_PI*i);

		/* apply only one window or none */
		/* double w = 2. - 2.*n/(order-1); */ /* Bartlett (triangular) */
		//double w = 0.5 * (1. - cos((2.*M_PI)*n/(order-1))); /* Hann(ing) */
		//double w = 0.54 - 0.46 * cos((2.*M_PI)*n/(order-1)); /* Hamming */
		//double w = 0.42 - 0.5 * cos((2.*M_PI)*n/(order-1)) + 0.08 * cos((4.*M_PI)*n/(order-1)); /* Blackman */
		const double w = 0.355768 - 0.487396 * cos((2.*M_PI)*n/(order - 1)) + 0.144232 * cos((4.*M_PI)*n/(order - 1)) - 0.012604 * cos((6.*M_PI)*n/(order - 1)); /* Nutall */
		//double w = 0.35875 - 0.48829 * cos((2.*M_PI)*n/(order - 1)) + 0.14128 * cos((4.*M_PI)*n/(order - 1)) - 0.01168 * cos((6.*M_PI)*n/(order - 1)); /* Blackman-Harris */

		/* apply the window */
		c *= w;

		/* update the gain */
		gain += 2.*c;

		/* insert the coeff */
		xcoeffs[i] = c;
	}

	/* adjust the gain to be exact 1.0 */
	for (i = 0; i <= midorder; ++i)
		f->xcoeffs[i] = (float)(xcoeffs[i] / gain);

	/* decrease the order if the last coeffs are near 0 */
	i = midorder;
	while (i > 0 && (int)(fabsf(f->xcoeffs[i])*32767.f) == 0) // cutoff low coefficients
		--i;

	f->order = i * 2 + 1;

	return f;
}

//
// IIR, see for example: https://www.beis.de/Elektronik/Filter/AnaDigFilt/AnaDigFilt.html
//

void filter2_setup(const int type, const double fc, const double d, const double gain,
	filter2_context * const __restrict filter2, const unsigned int sample_rate)
{
	const double two_over_T = 2*sample_rate;
	const double two_over_T_squared = (double)((long long)(2*sample_rate) * (long long)(2*sample_rate));

	/* calculate digital filter coefficents */
	/*w = (2.0*M_PI)*fc; no pre-warping */
	const double w = two_over_T*tan(M_PI*fc/sample_rate); /* pre-warping */ /* cutoff freq, in radians/sec */
	const double w_squared = w*w;

	const double den = two_over_T_squared + d*w*two_over_T + w_squared; /* temp variable */

	filter2->a1 = 2.0*(-two_over_T_squared + w_squared)/den;
	filter2->a2 = (two_over_T_squared - d*w*two_over_T + w_squared)/den;

	switch (type)
	{
		case FILTER_LOWPASS:
			filter2->b0 = filter2->b2 = w_squared/den;
			filter2->b1 = 2.0*filter2->b0;
			break;
		case FILTER_BANDPASS:
			filter2->b0 = d*w*two_over_T/den;
			filter2->b1 = 0.0;
			filter2->b2 = -filter2->b0;
			break;
		case FILTER_HIGHPASS:
			filter2->b0 = filter2->b2 = two_over_T_squared/den;
			filter2->b1 = -2.0*filter2->b0;
			break;
		default:
			//logerror("filter2_setup() - Invalid filter type for 2nd order filter.");
			break;
	}

	filter2->b0 *= gain;
	filter2->b1 *= gain;
	filter2->b2 *= gain;
}


/* Reset the input/output voltages to 0. */
void filter2_reset(filter2_context * const __restrict filter2)
{
	filter2->x1 = 0;
	filter2->x2 = 0;
	filter2->y1 = 0;
	filter2->y2 = 0;
}


// directly set digital coefficients, for example by using a tool like https://www.beis.de/Elektronik/Filter/AnaDigFilt/AnaDigFilt.html#AnaDigFilt
void filter_setup(const double b0, const double b1, const double b2, const double a1, const double a2, // a0 = 1
	filter2_context * const __restrict context)
{
	context->b0 = b0;
	context->b1 = b1;
	context->b2 = b2;
	context->a1 = a1;
	context->a2 = a2;

	// reset the filter inputs
	filter2_reset(context);
}

/* Set up a filter2 structure based on an op-amp multipole bandpass circuit. */
void filter_opamp_m_bandpass_setup(const double r1, const double r2, const double r3, const double c1, const double c2,
	filter2_context * const __restrict filter2, const unsigned int sample_rate)
{
	double r_in, fc, d, gain;

	if (r1 == 0.)
	{
		//logerror("filter_opamp_m_bandpass_setup() - r1 can not be 0");
		return;	/* Filter can not be setup.  Undefined results. */
	}

	if (r2 == 0.)
	{
		gain = 1.;
		r_in = r1;
	}
	else
	{
		gain = r2 / (r1 + r2);
		r_in = 1.0 / (1.0/r1 + 1.0/r2);
	}

	fc = 1.0 / ((2. * M_PI) * sqrt(r_in * r3 * c1 * c2));
	d = (c1 + c2) / sqrt(r3 / r_in * c1 * c2);
	gain *= -r3 / r_in * c2 / (c1 + c2);

	filter2_setup(FILTER_BANDPASS, fc, d, gain, filter2, sample_rate);
}

// General filter2 biquad setup.  The biquad transform is characterized
// by the reference point for the frequency warping, f0, and by the coefficients
// c1 and c2 in the generic continuous-time transfer function:
//
//                    1
//  H(s) = K * -----------------
//             1 + c1*s + c2*s^2
//
// The appropriate f0 reference frequency for a low-pass filter is generally 
// the  corner frequency, usually denoted Fc, also known as the cut-off frequency.
// The warping frequency is the point where the response of the digital and 
// analog versions of the filter match exactly, and the best results for
// low-pass filters are usually at the corner frequency.
//
static void filter2_biquad_setup(const double f0, const double c1, const double c2, filter2_context * const __restrict context, const int sample_rate)
{
	double gn, cc;

	// figure radians from frequency
	double w0 = f0 * (2.0 * PI);

	// keep it in -PI/T .. PI/T
	const double wdMax = PI * sample_rate;
	if (w0 > wdMax)
		w0 -= 2.0 * wdMax;

	// calculate the time step factor, pre-warping to f0
	gn = w0 / tan(w0 / (2 * sample_rate));
	//gn = 2 * sample_rate; // without pre-warping

	// calculate the difference equation coefficients
	cc = 1.0 + gn*c1 + gn*gn*c2;
	context->b0 = 1.0 / cc;
	context->b1 = 2.0 / cc;
	context->b2 = 1.0 / cc;
	context->a1 = 2.0 * (1.0 - gn*gn*c2) / cc;
	context->a2 = (1.0 - gn*c1 + gn*gn*c2) / cc;

	// reset the inputs
	filter2_reset(context);
}

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
	filter2_context * const __restrict context, const int sample_rate)
{
	// Continuous-time transfer function for the analog filter:
	//
	//            1/(C1*R1)
	//  H(s) = ---------------
	//          s + 1/(C1*R1)
	//
	// Refactoring to canonical form:
	//
	//             1
	//   H(s) = ---------
	//          1 + c1*s
	//
	const double R = (R2 + R3 > 0.) ? (R1*(R2 + R3)) / (R1 + R2 + R3) : R1;
	const double c1 = C1*R;

	// calculate the cutoff frequency for the filter
	const double Fc = 1.0 / ((2.0 * PI) * c1);

	// set up the biquad filter parameters
	filter2_biquad_setup(Fc, c1, 0.0, context, sample_rate);
}


//
// Multiple feedback active low pass filter
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
void filter_mf_lp_setup(const double R1, const double R2, const double R3, const double C1, const double C2,
	filter2_context * const __restrict context, const int sample_rate)
{
	// Continuous-time transfer function for the analog version of
	// this filter:
	//
	//                         -1/(C1*C2*R1*R2)
	//  H(s) = -----------------------------------------------------
	//         s^2 + (1/C1)*(1/R1 + 1/R2 + 1/R3)*s + 1/(C1*C2*R2*R3)
	//
	// Refactor that to the canonical form:
	//
	//                    1
	//  H(s) = K * -----------------
	//             1 + c1*s + c2*s^2
	//
	// ...and calculate the coefficients c1 and c2.  K is the analog
	// filter's DC gain; for the digital filter, we want unit gain, so
	// after factoring into the form above, we replace K with 1.0.
	//
	//double K = -R3/R1;  // analog filter's DC gain
	const double c1 = C2*R2*R3*(1.0 / R1 + 1.0 / R2 + 1.0 / R3);
	const double c2 = C1*C2*R2*R3;

	// calculate the cutoff frequency for the filter
	const double Fc = 1.0 / ((2.0 * PI) * sqrt(c2));

	// set up the biquad parameters
	filter2_biquad_setup(Fc, c1, c2, context, sample_rate);
}

// 
// Single-pole active low-pass filter
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
void filter_active_lp_setup(const double R1, const double R2, const double R3, const double C1,
	filter2_context * const __restrict context, const int sample_rate)
{
	// Continuous-time transfer function for the analog filter:
	//
	//                     -1/(R1*R2*C1)
	//   H(s) = --------------------------------------
	//           (1/R1 + 1/R2 + 1/R3)*s + 1/(R2*R3*C1)
	// 
	// Refactor to canonical form:
	//
	//                 1
	//   H(s) = K * --------
	//              1 + c1*s
	//
	// ... and calculate the coefficient c1.  K is the analog filter's
	// DC gain, which we replace (after refactoring into the form above)
	// with 1 for unit gain in the digital implementation.
	//
	// double K = -R3/R1;   // analog filter DC gain
	const double c1 = R2*R3*C1*(1.0 / R1 + 1.0 / R2 + 1.0 / R3);

	// calculate the cutoff frequency for the filter
	const double Fc = 1.0 / ((2.0 * PI) * c1);

	// set up the biquad parameters
	filter2_biquad_setup(Fc, c1, 0.0, context, sample_rate);
}

// Sallen-Key low-pass filter
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
void filter_sallen_key_lp_setup(const double R1, const double R2, const double C1, const double C2,
	filter2_context * const __restrict context, const int sample_rate)
{
	// Continuous-time transfer function for the analog filter:
	//
	//                       1/(R1*R2*C1*C2)
	//   H(s) = ----------------------------------------------
	//          s^2 + (1/C1)*(1/R2 + 1/R1)*s + 1/(R1*R2*C1*C2)
	// 
	// Refactor to canonical form:
	//
	//                  1
	//   H(s) =  -----------------
	//           1 + c1*s + c2*s^2
	//
	// ... and calculate the coefficients c1 and c2.
	//
	const double c1 = C2*(R1 + R2);
	const double c2 = R1*R2*C1*C2;

	// calculate the cutoff frequency for the filter
	const double Fc = 1.0 / ((2.0 * PI) * sqrt(c2));

	// set up the biquad parameters
	filter2_biquad_setup(Fc, c1, c2, context, sample_rate);
}
