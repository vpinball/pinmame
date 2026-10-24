// license:BSD-3-Clause

#ifndef INC_SPIKE1
#define INC_SPIKE1

/* The Spike 1 CPU as PinMAME's CPU interface sees it (src/cpuintrf.c): it runs the game program
   in src/spike1 for the cycles PinMAME hands it, on its own bus - the debugger's memory views
   show nothing */

extern int spike1cpu_ICount;

void spike1cpu_init(void);
void spike1cpu_reset(void *param);
void spike1cpu_exit(void);
int spike1cpu_execute(int cycles);
unsigned spike1cpu_get_context(void *dst);
void spike1cpu_set_context(void *src);
unsigned spike1cpu_get_reg(int regnum);
void spike1cpu_set_reg(int regnum, unsigned val);
void spike1cpu_set_irq_line(int irqline, int state);
void spike1cpu_set_irq_callback(int (*callback)(int irqline));
const char *spike1cpu_info(void *context, int regnum);
unsigned spike1cpu_dasm(char *buffer, unsigned pc);

#endif
