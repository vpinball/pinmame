#include "driver.h"

/*
	Helper object for driving a shift register controlled by two P-ROC pins
	(examples include AFM and BOP).
	
	Queues bits at any speed and drives them at a fixed rate that won't
	overwhelm the P-ROC.
*/

typedef struct proc_shift_reg_t {
  UINT64 queue;        // queue of bits, insert at (1<<bit), remove from bit 0
  int bit;             // position to insert next bit into queue
  int procClockCoil;   // procDriveCoil() coil # for shift register's clock pin
  int procDataCoil;    // procDriveCoil() coil # for shift register's data pin
  int procDataValue;   // last value driven on procCoilData
  int setClock;        // 1 if proc_shr_check_queue() should set clock on next pass
  int timerEnabled;    // 1 if periodic timer enabled for this object
} proc_shift_reg_t;

void proc_shiftRegInit(int procClockCoil, int procDataCoil);
void proc_shiftRegEnqueue(int bit);
