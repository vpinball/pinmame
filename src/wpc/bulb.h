// license:GPLv3+
#ifndef INC_BULB
#define INC_BULB
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#define BULB_44   0
#define BULB_47   1
#define BULB_86   2
#define BULB_89   3
#define BULB_906  4
#define BULB_MAX  5

#define BULB_T_MAX 3400

extern void bulb_init();
extern float bulb_filament_temperature_to_emission(const int bulb, const float T);
extern void bulb_filament_temperature_to_tint(const float T, float* linear_RGB);
extern double bulb_emission_to_filament_temperature(const double p);
extern double bulb_cool_down_factor(const int bulb, const double T);
extern double bulb_cool_down(const int bulb, double T, float duration);
extern float bulb_heat_up_factor(const int bulb, const float T, const float U, const float serial_R);
extern double bulb_heat_up(const int bulb, double T, float duration, const float U, const float serial_R);

#endif /* INC_BULB */
