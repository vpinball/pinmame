/********************************/
/* PINMAME SOUND SAMPLE SUPPORT */
/********************************/
/* Created: 12/15/00 SJE*/

/* 010101, Tom:
     - added handling of on/off transitions and repeated play
     - changed: sample only plays once per sol activation
     - moved flipper activation to proc_mechsounds()
   240201  Updated handling to new solenoid numbering
           flippers trigger samples without map
   110301  Upper Flipper now only make sound if they are in
           the game FLIP_SOL()
   020801  Recoded proc_mechsound() to check for {-1} entry,
           and to also not continue searching the map, after a
		   match is found! (SJE)
*/

#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "wpcsam.h"

/*----------------
/ Samples
/-----------------*/
static const char *pinmame_sample_names[] = {
  "*pinmame",
  "knocker.wav",
  "lsling.wav",
  "rsling.wav",
  "solenoid.wav",
  "popper.wav",
  "ballrel.wav",
  "diverter.wav",
  "flapopen.wav",
  "flapclos.wav",
  "lflipper.wav",
  "rflipper.wav",
  "jet1.wav",
  "jet2.wav",
  "jet3.wav",
  "motor1.wav",
  "motor2.wav",
  "solon.wav",
  "soloff.wav",
  0	/* end of array */
};

struct Samplesinterface samples_interface = {
  6, 100, /* 6 channels full volume */
  pinmame_sample_names,
  "Mech. sounds"
};


/* Check if a sample is specified for changed solenoid */
void proc_mechsounds(int sol, int newState) {
  if ((sol == sLRFlip) ||
      ((sol == sURFlip) && (core_gameData->hw.flippers & FLIP_SOL(FLIP_UR))))
    sample_start(0, newState ? SAM_RFLIPPER : SAM_SOLENOID_OFF,0);
  else if ((sol == sLLFlip) ||
           ((sol == sULFlip) && (core_gameData->hw.flippers & FLIP_SOL(FLIP_UL))))
    sample_start(2, newState ? SAM_LFLIPPER : SAM_SOLENOID_OFF,0);
  else {
    wpc_tSamSolMap *item = core_gameData->hw.solsammap;
    if (item) {
	  while (item->solname >= 0 && item->solname != sol)
			item++;
	  if(item->solname >= 0 && item->solname == sol) {
		if (newState) {
	        if (item->flags == WPCSAM_F_OFFON)
	          sample_start(item->channel, item->samname, 0);
		    else if (item->flags == WPCSAM_F_CONT)
              sample_start(item->channel, item->samname, 1);
		}
		else {
			if (item->flags == WPCSAM_F_ONOFF)
              sample_start(item->channel, item->samname, 0);
            else if (item->flags == WPCSAM_F_CONT)
	          sample_stop(item->channel);
		}
	    item++;
      } /* if(item->solname >=0 */
    } /* if (item) */
  }
}

void wpc_play_sample(int channel, int samplename) {
  /*Don't play again if already playing it*/
  if(!sample_playing(channel))
   sample_start(channel,samplename,0);
}
