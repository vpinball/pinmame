#ifndef __SVGAINPUT_H
#define __SVGAINPUT_H

int svga_input_init(void);
int svga_input_open(void (*release_func)(void), void (*aqcuire_func)(void));
void svga_input_close(void);
void svga_input_exit(void);

#endif
