#ifndef INC_STSND
#define INC_STSND

/*-- St300 interface --*/
struct sndbrdst300 {
  int c0;
  int ax[9];
  int axb[9];	
  UINT16 timer1,timer2,timer3;
};

extern MACHINE_DRIVER_EXTERN(st300);

extern struct sndbrdst300 snddatst300;

#endif /* INC_STSND */


