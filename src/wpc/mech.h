#ifndef INC_MECH
#define INC_MECH

/*----------------------------------------------------------------
/ MECH_LINEAR     = Linear movement (default)
/ MECH_NONLINEAR  = non-linear movement
/ MECH_ACC(x)     = Acceleration x = time to full speed [VBLANK 1/60s]
/ MECH_RET(x)     = Retardation x=times slower than acc (3 = 3 times slower)
/
/ MECH_CIRCLE     = circular movement = restart at end (default)
/ MECH_STOPEND    = Stop at endpoints
/ MECH_REVERSE    = Reverse at endpoints
/
/ MECH_ONESOL     = One motor solenoid (default)
/ MECH_ONEDIRSOL  = One motor and one direction solenoid
/ MECH_TWODIRSOL  = Two motor solenoids=one in each direction
/ MECH_TWOSTEPSOL = Two step motor solenoids
/---------------------------------------------------------------*/
#define MECH_LINEAR     0x00 // Default
#define MECH_NONLINEAR  0x01
#define MECH_ACC(x)     ((x)<<9)
#define MECH_RET(x)     ((x)<<24)
#define MECH_CIRCLE     0x00 // Default
#define MECH_STOPEND    0x02
#define MECH_REVERSE    0x04

#define MECH_ONESOL     0x00 // Default
#define MECH_ONEDIRSOL  0x10
#define MECH_TWODIRSOL  0x20
#define MECH_TWOSTEPSOL 0x40

#define MECH_SLOW       0x00 // Default
#define MECH_FAST       0x80
#define MECH_STEPSW     0x00 // Default
#define MECH_LENGTHSW   0x100

#define MECH_MAXMECH 10

typedef struct {
  int swNo, startPos, endPos, pulse;
} mech_tSwData;
typedef struct {
  int sol1, sol2;
  int type;
  int length, steps;
  mech_tSwData sw[20];
} mech_tInitData, *mech_ptInitData;

extern void mech_init(void);
extern void mech_emuInit(void);
extern void mech_emuExit(void);
extern void mech_addLong(int mechNo, int sol1, int sol2, int type, int length, int steps, mech_tSwData sw[]);
extern void mech_add(int mechNo, mech_ptInitData id);
extern int  mech_getPos(int mechNo);
extern int  mech_getSpeed(int mechNo);
extern void mech_nv(void *file, int write);

#endif /* INC_MECH */
