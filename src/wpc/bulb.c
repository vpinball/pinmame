// license:GPLv3+

#include <math.h>
#include "bulb.h"

// 2024.07.29 - Changes:
// * replace bulb characteristics by values based on real bulb resistance measures using the following method:
//   . collect U and I ratings, compute R at (unknown) stability temperature (T): R = U/I
//   . measure R0 at room temperature (T0) with a multimeter on a set of real bulbs
//   . estimate stability temperature (T) from the ratio between R and R0 using the 2 following equations:
//      R = p(T).L/A with p = resisitivity, L = filament length, A = wire section surface (Pi.r²)
//        we suppose L/A constant (limited dilatation), so R/R0 = p/p0 and therefore p = p0.R/R0.
//        knowing that p0 of tungsten is 5.65x10-8 Ohm.m at T0 = 293K
//      (p - p0) = a.p0.(T - T0) with a = temperature to resistivity of tungsten, approximated to 0.0045
//   . select a stability temperature from previous estimate, and compute back a corrected corresponding R0 using the previous equations
//   . select a filament radius, compute the corresponding filament length since we know L/A = R0/p0 = L/(PI.r²) so L = PI.r².R0/p0
//   . simulate the heating up to a stable state using the implementation provided in this module
//   . adjust filament radius until the resulting stability temperature correspond to the selected one
// * replace filament resistance model with a simpler one R = R0.(1 + 0.0045 (T-T0)) instead of R = R0 * (T/T0)^1.215 since it matches the model used to evaluate bulb characteristicvs and gives better results
// * add #906 bulb


/*-------------------
/  Bulb characteristics and precomputed LUTs
/-------------------*/
typedef struct {
  double rating_u; /* voltage rating for nominal operation */
  double rating_i; /* current rating for nominal operation */
  double rating_T; /* temperature rating for nominal operation */
  double r0; /* resistance at 293K */
  double surface; /* filament surface in m² */
  double mass;	/* filament mass in kg */
  double cool_down[BULB_T_MAX + 1]; /* precomputed cool down factor */
  double heat_factor[BULB_T_MAX + 1]; /* precomputed heat factor = 1.0 / (R * Mass * Specific Heat) */
} bulb_tLampCharacteristics;

// This used to be the following formula, r0 being already divided by pow(T0, 1.215), that is to say multiplied by 0.00100636573473889440796764003454
// #define BULB_R(bulb, T) (bulbs[bulb].r0 * pow(T, 1.215))
// This new formula seems to give more accurate results, with a supposed slightly lower performance impact
#define BULB_R(bulb, T) (bulbs[bulb].r0 * (1.0 + 0.0045 * (T - 293.0)))

/*-------------------
/  local variables
/-------------------*/
static int initialized = 0;

static struct {
  double                      t_to_p[BULB_T_MAX + 1 - 1500];
  double                      p_to_t[512];
  double                      specific_heat[BULB_T_MAX + 1];
} locals;

static bulb_tLampCharacteristics bulbs[BULB_MAX] = {
   {  6.3, 0.250, 2700.0, 2.12990745045007, 0.000001614540092669770, 0.000000201702717783132 }, //  #44 ( 33ms from 10% to 90%)
   {  6.3, 0.150, 2500.0, 3.84210767049353, 0.000001202491253698990, 0.000000111862951366603 }, //  #47 ( 31ms from 10% to 90%)
   {  6.3, 0.200, 2200.0, 3.28758545112978, 0.000003380553637833940, 0.000000467510791873652 }, //  #86 ( 99ms from 10% to 90%)
   { 13.0, 0.577, 2700.0, 1.90452041865641, 0.000007161323996498170, 0.000001525794698449980 }, //  #89 ( 57ms from 10% to 90%)
   { 13.0, 0.690, 2400.0, 1.79750796261460, 0.000015511737502181100, 0.000004359371396813600 }, // #906 (113ms from 10% to 90%)
};

// Linear RGB tint of a blackbody for temperatures ranging from 1500 to 3000K. The values are normalized for a relative luminance of 1.
// These values were evaluated using Blender's blackbody implementation, then normalized using the standard relative luminance formula (see https://en.wikipedia.org/wiki/Relative_luminance)
// We use a minimum channel value at 0.000001 to avoid divide by 0 in client application.
static const float temperatureToTint[3 * 16] = {
   3.253114f, 0.431191f, 0.000001f,
   3.074210f, 0.484372f, 0.000001f,
   2.914679f, 0.531794f, 0.000001f,
   2.769808f, 0.574859f, 0.000001f,
   2.643605f, 0.612374f, 0.000001f,
   2.523686f, 0.645953f, 0.020487f,
   2.414433f, 0.676211f, 0.042456f,
   2.316033f, 0.703137f, 0.065485f,
   2.225598f, 0.727599f, 0.089456f,
   2.144543f, 0.749200f, 0.114156f,
   2.070389f, 0.768694f, 0.139412f,
   1.997974f, 0.787618f, 0.165180f,
   1.935725f, 0.803465f, 0.191508f,
   1.876871f, 0.818242f, 0.218429f,
   1.821461f, 0.832006f, 0.245241f,
   1.772554f, 0.843853f, 0.271900f 
};

/*-------------------------------
/  Initialize all pre-computed LUTs
/-------------------------------*/
void bulb_init()
{
   if (initialized)
      return;
   initialized = 1;

   // Compute filament temperature to visible emission power LUT, normalized by visible emission power at T=2700K, according 
   // to the formula from "Luminous radiation from a black body and the mechanical equivalentt of light" by W.W.Coblentz and W.B.Emerson
   for (int i=0; i <= BULB_T_MAX - 1500; i++)
   {
      double T = 1500.0 + i;
      locals.t_to_p[i] = 1.247/pow(1.0+129.05/T, 204.0) + 0.0678/pow(1.0+78.85/T, 404.0) + 0.0489/pow(1.0+23.52/T, 1004.0) + 0.0406/pow(1.0+13.67/T, 2004.0);
   }

   // Compute visible emission power to filament temperature LUT, normalized for a relative power of 255 for visible emission power at T=2700K
   // For the time being we simply search in the previously created LUT
   int t_pos = 0;
   double P2700 = locals.t_to_p[2700 - 1500];
   for (int i=0; i<512; i++)
   {
      double p = i / 255.0;
      while (locals.t_to_p[t_pos] < p * P2700)
         t_pos++;
      locals.p_to_t[i] = 1500 + t_pos;
   }

   // Precompute main parameters of the heating/cooldown model for each of the supported bulbs
   for (int i=0; i <= BULB_T_MAX; i++)
   {
      double T = i;
      // Compute Tungsten specific heat (energy to temperature transfer, depending on temperature) according to formula from "Heating-times of tungsten filament incandescent lamps" by Dulli Chandra Agrawal
      locals.specific_heat[i] = 3.0 * 45.2268 * (1.0 - 310.0 * 310.0 / (20.0 * T*T)) + (2.0 * 0.0045549 * T) + (4 * 0.000000000577874 * T*T*T);
      // Compute cooldown and heat up factor for the predefined bulbs
      for (int j=0; j<BULB_MAX; j++)
      {
         // pow(T, 5.0796) is pow(T, 4) from Stefan/Boltzmann multiplied by tungsten overall (all wavelengths) emissivity which is 0.0000664*pow(T,1.0796)
         double delta_energy = -0.00000005670374419 * bulbs[j].surface * 0.0000664 * pow(T, 5.0796);
         bulbs[j].cool_down[i] = delta_energy / (locals.specific_heat[i] * bulbs[j].mass);
         bulbs[j].heat_factor[i] = 1.0 / (BULB_R(j, T) * locals.specific_heat[i] * bulbs[j].mass);
      }
   }
}

/*-------------------------------
/  Returns relative visible emission power for a given filament temperature. Result is relative to the emission power at the bulb rated stability temperature
/-------------------------------*/
float bulb_filament_temperature_to_emission(const int bulb, const float T)
{
   if (T < 1500.0f) return 0.f;
   if (T >= (float)BULB_T_MAX) return (float)locals.t_to_p[BULB_T_MAX - 1500];
   return (float)(locals.t_to_p[(int)T - 1500] / locals.t_to_p[(int)(bulbs[bulb].rating_T) - 1500]);
   // Linear interpolation is not worth its cost
   // int lower_T = (int) T, upper_T = lower_T+1;
   // float alpha = T - (float)lower_T;
   // return (1.0f - alpha) * (float)((locals.t_to_p[lower_T - 1500] + alpha * locals.t_to_p[upper_T - 1500])  / locals.t_to_p[(int)(bulbs[bulb].rating_T) - 1500]));
}

/*-------------------------------
// Returns linear RGB tint for a given filament temperature. The values are normalized for a perceived luminance of 1.
/-------------------------------*/
void bulb_filament_temperature_to_tint(const float T, float* linear_RGB)
{
   if (T < 1500.0f)
   {
      linear_RGB[0] = temperatureToTint[0];
      linear_RGB[1] = temperatureToTint[1];
      linear_RGB[2] = temperatureToTint[2];
   }
   else if (T >= 2999.0f)
   {
      linear_RGB[0] = temperatureToTint[15 * 3 + 0];
      linear_RGB[1] = temperatureToTint[15 * 3 + 1];
      linear_RGB[2] = temperatureToTint[15 * 3 + 2];
   }
   else
   {
      // Linear interpolation between the precomputed values
      float t_ref = (T - 1500.0f) * (float)(1./100.);
      int lower_T = (int)t_ref, upper_T = lower_T+1;
      float alpha = t_ref - (float)lower_T;
      linear_RGB[0] = (1.0f - alpha) * temperatureToTint[lower_T * 3 + 0] + alpha * temperatureToTint[upper_T * 3 + 0];
      linear_RGB[1] = (1.0f - alpha) * temperatureToTint[lower_T * 3 + 1] + alpha * temperatureToTint[upper_T * 3 + 1];
      linear_RGB[2] = (1.0f - alpha) * temperatureToTint[lower_T * 3 + 2] + alpha * temperatureToTint[upper_T * 3 + 2];
   }
}


/*-------------------------------
/  Returns filament temperature for a given visible emission power normalized for an emission power of 1.0 at 2700K
/-------------------------------*/
double bulb_emission_to_filament_temperature(const double p)
{
   int v = (int)(p * 255.);
   return v >= 512 ? locals.p_to_t[511] : locals.p_to_t[v];
}

/*-------------------------------
/  Compute cool down factor of a filament
/-------------------------------*/
double bulb_cool_down_factor(const int bulb, const double T)
{
   return bulbs[bulb].cool_down[(int) T];
}

/*-------------------------------
/  Compute cool down factor of a filament over a given period
/-------------------------------*/
double bulb_cool_down(const int bulb, double T, float duration)
{
   while (duration > 0.0f)
   {
      float dt = duration > 0.001f ? 0.001f : duration;
      T += dt * bulbs[bulb].cool_down[(int) T];
      if (T <= 294.0)
      {
         return 293.0;
      }
      duration -= dt;
   }
   return T;
}

/*-------------------------------
/  Compute heat up factor of a filament under a given voltage (sum of heating and cooldown)
/-------------------------------*/
float bulb_heat_up_factor(const int bulb, const float T, const float U, const float serial_R)
{
   double U1 = U;
   if (serial_R != 0.f)
   {
      const double R = BULB_R(bulb, T);
      U1 *= R / (R + serial_R);
   }
   return (float)(U1 * U1 * bulbs[bulb].heat_factor[(int)T] + bulbs[bulb].cool_down[(int)T]);
}

/*-------------------------------
/  Compute temperature of a filament under a given voltage over a given period (sum of heating and cooldown)
/-------------------------------*/
double bulb_heat_up(const int bulb, double T, float duration, const float U, const float serial_R)
{
   while (duration > 0.0f)
   {
      T = T < 293.0 ? 293.0 : T > BULB_T_MAX ? BULB_T_MAX : T; // Keeps T within the range of the LUT (between room temperature and melt down point)
      double energy;
      double U1 = U;
      if (serial_R != 0.f)
      {
         const double R = BULB_R(bulb, T);
         U1 *= R / (R + serial_R);
      }
      energy = U1 * U1 * bulbs[bulb].heat_factor[(int)T] + bulbs[bulb].cool_down[(int)T];
      if (-10 < energy && energy < 10)
      {
         // Stable state reached since electric heat (roughly) equals radiation cool down
         return T;
      }
      float dt;
      if (energy > 1000e3)
      {
         // Initial current surge, 0.5ms integration period in order to account for the fast resistor rise that will quickly lower the current
         dt = duration > 0.0005f ? 0.0005f : duration;
      }
      else
      {
         // Ramping up, 1ms integration period
         dt = duration > 0.001f ? 0.001f : duration;
      }
      T += dt * energy;
      duration -= dt;
   }
   return T;
}
