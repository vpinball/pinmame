#ifndef INC_WPCSAM
#define INC_WPCSAM

/*Exported Functions*/
void wpc_play_sample(int channel, int samplename);
void proc_mechsounds(int sol, int newState);

//Note: Must be all UNIT8 to prevent strange mame037b16 bug (SJE)

/*Exported Structures*/
typedef struct {
  int solname;	/*Name of Solenoid to trigger sound*/
  UINT8 channel;	/*Channel # to play sample*/
  UINT8 samname;	/*Name of Sample to play*/
  UINT8 flags;		/*When and how low to play*/
} wpc_tSamSolMap;

#ifdef PINMAME_SAMPLES
  extern struct Samplesinterface samples_interface;
  #define SAMPLESINTERFACE {SOUND_SAMPLES, &samples_interface}
#else
  #define SAMPLESINTERFACE {0}
#endif

#define WPCSAM_F_OFFON		 0	/* play sample if sol is activated (default) */
#define WPCSAM_F_ONOFF		 1	/* play samples if sol is deactivated */
#define WPCSAM_F_CONT            2      /* play sample continuesly (loop)*/

/*SOUND SAMPLES*/
#define WPCSAM_KNOCKER		 0
#define WPCSAM_LSLING		 1
#define WPCSAM_RSLING		 2
#define WPCSAM_SOLENOID		 3
#define WPCSAM_POPPER		 4
#define WPCSAM_BALLREL		 5
#define WPCSAM_DIVERTER		 6
#define WPCSAM_FLAPOPEN		 7
#define WPCSAM_FLAPCLOSE	 8
#define WPCSAM_LFLIPPER		 9
#define WPCSAM_RFLIPPER	    10
#define WPCSAM_JET1			11
#define WPCSAM_JET2			12
#define WPCSAM_JET3			13
#define WPCSAM_MOTOR_1		14
#define WPCSAM_MOTOR_2		15
#define WPCSAM_SOLENOID_ON	16
#define WPCSAM_SOLENOID_OFF	17


/*MECHANICAL SOUND SAMPLE NAMES*/
#define SAM_KNOCKER			WPCSAM_KNOCKER
#define SAM_LSLING			WPCSAM_LSLING
#define SAM_RSLING			WPCSAM_RSLING
#define SAM_SOLENOID		WPCSAM_SOLENOID		// usual for all not holded sols
#define SAM_POPPER			WPCSAM_POPPER
#define SAM_BALLREL			WPCSAM_BALLREL
#define SAM_DIVERTER		WPCSAM_DIVERTER
#define SAM_FLAPOPEN		WPCSAM_FLAPOPEN
#define SAM_FLAPCLOSE		WPCSAM_FLAPCLOSE
#define SAM_OUTHOLE			WPCSAM_SOLENOID
#define SAM_JET1			WPCSAM_JET1
#define SAM_JET2			WPCSAM_JET2
#define SAM_JET3			WPCSAM_JET3
#define SAM_JET4			WPCSAM_JET1
#define SAM_JET5			WPCSAM_JET2
#define SAM_JET6			WPCSAM_JET3
#define SAM_LFLIPPER		WPCSAM_LFLIPPER
#define SAM_RFLIPPER		WPCSAM_RFLIPPER
#define SAM_MOTOR_1			WPCSAM_MOTOR_1		/* motor sound #1, should be used with WPCSAM_F_CONT */
#define SAM_MOTOR_2			WPCSAM_MOTOR_2		/* motor sound #2, should be used with WPCSAM_F_CONT */
#define SAM_SOLENOID_ON		WPCSAM_SOLENOID_ON	/* sol is hold after activating */
#define SAM_SOLENOID_OFF	WPCSAM_SOLENOID_OFF	/* sol is deactivated and gets to it's rest position, should be used with WPCSAM_F_ONOFF */

#endif	/*INC_WPCSAM*/
