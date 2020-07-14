#include "p-roc.h"
#include "proc_shift_reg.h"

/*
  Designed to support a single shift register connected to the P-ROC for
  simplicity.  Otherwise, it is necessary to keep a list of proc_shift_reg_t
  pointers and pass an index to that list to proc_shiftRegCheckQueue()
  (by including it in the timer_pulse() initialization).
  
  Additionally requires caller to keep the structure and pass a pointer to it
  in calls to proc_shiftRegEnqueue(), or to keep the index and pass it instead.
*/

proc_shift_reg_t shr;

void proc_shiftRegInit(int procClockCoil, int procDataCoil) {
  memset(&shr, 0, sizeof(shr));
  shr.procClockCoil = procClockCoil;
  shr.procDataCoil = procDataCoil;
}

int proc_shiftRegIsEmpty(void) {
  return shr.bit == 0;
}

int proc_shiftRegDequeue(void) {
  int retval = -1;
  if (shr.bit > 0) {
    retval = shr.queue & 1;
    shr.queue >>= 1;
    --shr.bit;
  }
  return retval;
}

void proc_shiftRegDriveClock(void) {
  // pulse the clock, relying on YAML's pulseTime of 1 on C37
  procDriveCoil(shr.procClockCoil, 1);
  procFlush();
}

void proc_shiftRegCheckQueue(int dummy) {
  int data;

  if (shr.setClock) {
    shr.setClock = 0;
    proc_shiftRegDriveClock();
    return;
  }

  if (proc_shiftRegIsEmpty())
    return;
    
  data = proc_shiftRegDequeue();
  if (shr.procDataValue == data) {
    // already driving correct value on data pin
    proc_shiftRegDriveClock();
  } else {
    // update output pin on P-ROC first
    procDriveCoil(shr.procDataCoil, data);
    procFlush();
    shr.procDataValue = data;

    // set clock on next pass
    shr.setClock = 1;
  }
}

void proc_shiftRegEnqueue(int bit) {
  if (shr.bit == 63) {
    // queue is full and we've fallen behind, drop half the bits
    shr.queue >>= 32;
    shr.bit -= 32;
  }
  if (bit)
    shr.queue |= ((UINT64)1 << shr.bit);
  ++shr.bit;
  
  if (!shr.timerEnabled) {
    shr.timerEnabled = 1;
    timer_pulse(TIME_IN_MSEC(15.0), 0, &proc_shiftRegCheckQueue);
  }
}
