#ifndef FADECANDY_H
#define FADECANDY_H

typedef struct
{
  unsigned char is_mapped;
  int mapled;
  unsigned char exclusive;
  unsigned char r, g, b;
}
t_lisy_lamp_to_led_map;

int lisy_fadecandy_init(unsigned char system);
int lisy_fadecandy_set_led(int lamp, unsigned char value);

#endif  /* FADECANDY_H */

