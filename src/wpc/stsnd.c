
#include "driver.h"
#include "core.h"
#include "snd_cmd.h"
#include "sndbrd.h"
#include "stsnd.h"
#include "math.h"
/*----------------------------------------
/ Stern Sound System
/ 3 different boards:
/
/ ST-100:  discrete
/ ST-300:  discrete, (+ VS-1000 speech)
/ ASTRO:   discrete, can switch between SB-100 and SB-300
/-----------------------------------------*/
// support added soundboard st-300 by Oliver Kaegi (14/09/2004)
// interface definition for soundboard st-300
#define ST300_INTCLOCK    1000000  // clock speed in hz of mpu ST-200 board ! -> 0.000'001 sec

#define ST300_VOL 1 // volume for channels
static const int st300_mixing_levels = 30;

struct sndbrdst300 snddatst300;

static struct {
  UINT16 timlat1,timlat2,timlat3,timer1,timer2,timer3,t4562c;
  UINT16 timlats1,timlats2,timlats3;
  int    cr1,cr2,cr3,channel,timp1,timp2,timp3,tfre1,tfre2,tfre3,noise,conx,altx,dir;
  int    volnr,reset,extfreq;	
} st300loc;

static  INT16  sineWaveinp[] = {
   0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF,
  -0x7FFF,-0x7FFF,-0x7FFF,-0x7FFF,-0x7FFF,-0x7FFF,-0x7FFF,-0x7FFF
}; // q2 wave 

static  INT16  sineWaveinpq1[] = {
   0x4000, 0x2000, 0x7fff, 0x1000, 0x0000, 0x0000, 0x0000, 0x0000,
  -0x4000,-0x2000,-0x7fff,-0x1000,-0x0000,-0x0000,-0x0000,-0x0000
}; // q1 wave 


static  INT16  volume300[] = {
       0,  0,  0,  0, 0, 10, 20, 20, 30, 30, 40, 40,  60,  60,  80,  90,100 ,100	
};

static  INT16  sineWaveext[32000]; // wave triggered by external clock


static int setvol(int param)	{
  if ((st300loc.volnr) == 0) return 0;
  if (st300loc.dir) { // count up   
        logerror("volume up %04d ind %04d \n",volume300[st300loc.volnr],st300loc.volnr );
    	return volume300[st300loc.volnr];
  } else {
        logerror("volume down %04d ind %04d \n",volume300[17-st300loc.volnr],st300loc.volnr );
    	return volume300[17-st300loc.volnr] ;  	
  }  
}
static void startvol(int param)	{
  st300loc.volnr = 1;
 }

static void nextvol(int param) {
  if ((st300loc.volnr > 0) && (st300loc.volnr <= 16)) st300loc.volnr++;	
  if (st300loc.volnr > 16) {
	if (st300loc.conx == 0) {
		if ((st300loc.dir) == 0) {
			st300loc.volnr = 0;	// quiet if we are going down...
		} else {
		}	
	} else {
		startvol(0);
		if (st300loc.altx) {
			 st300loc.dir =  (st300loc.dir ? 0 : 1) ;
		}
	} 
  }
}


static void playsam3(int param) {
// timer 3 (q3) is volume....
   if ((st300loc.cr3 & 0x80) && (st300loc.timlat3 > 0) && (st300loc.reset == 0))   { // output is enabled...
	startvol(0);
	mixer_set_volume(st300loc.channel,setvol(0)*ST300_VOL);
	mixer_set_volume(st300loc.channel+2,setvol(0)*ST300_VOL);	
	logerror("playsam3 \n");
    } else {	// q3 is not running...
	startvol(0);
	mixer_set_volume(st300loc.channel,setvol(0)*ST300_VOL);
	mixer_set_volume(st300loc.channel+2,setvol(0)*ST300_VOL);	
	logerror("playsam3 don't move \n");	
    }	
}	

static void playsam2(int param){
// timer 2 (q2) is easy wave + volume from q3
   if ((st300loc.cr2 & 0x80)  && (st300loc.timlat2 > 0) && (st300loc.reset == 0))   { // output is enabled...
 	mixer_play_sample_16(st300loc.channel,sineWaveinp, sizeof(sineWaveinp), st300loc.tfre2*sizeof(sineWaveinp) / 2 / 1.137, 1);
	logerror("playsam2 %04d \n",st300loc.tfre2);
	}
}	

static void playsam1(int param){
// timer 1 (q1) is easy wave + volume always 100 
   if ((st300loc.cr1 & 0x80)  && (st300loc.timlat1 > 0) && (st300loc.reset == 0))   { // output is enabled...
 	mixer_play_sample_16(st300loc.channel+1,sineWaveinpq1, sizeof(sineWaveinpq1), st300loc.tfre1*sizeof(sineWaveinpq1) / 2 / 1.137, 1);
	logerror("playsam1 %04d \n",st300loc.tfre1);
	}
}	


static void playsamext(int param){
// external timer + volume from q3
   int f;

//   f = (17-(st300loc.extfreq))*sizeof(sineWaveext);
   f = 625000 / (17-(st300loc.extfreq));

   if (st300loc.noise)   { // output is enabled...
 	mixer_play_sample_16(st300loc.channel+2,sineWaveext, sizeof(sineWaveext), f , 1);
	logerror("playsamext frequenz %08d data %04d \n",f,st300loc.extfreq);
        } else {
	mixer_stop_sample(st300loc.channel+2);
	logerror("playsamext stop \n");
   	}
}	 



static void softreset (int param) {
  st300loc.reset = param;
  if (st300loc.reset) { // reset
	snddatst300.timer1 = st300loc.timlat1;
	snddatst300.timer2 = st300loc.timlat2;	
	snddatst300.timer3 = st300loc.timlat3;		
	st300loc.timp1 = 0;
	st300loc.timp2 = 0;
	st300loc.timp2 = 0;
	mixer_stop_sample(st300loc.channel);
	mixer_stop_sample(st300loc.channel+1);
  } else {
  }
}



	
static void st300_pulse (int param) {
// param = 0x02 -> internal 6840 clock
// param = 0 -> external 4049 clock
// timpX is the output level of the 6840 (only 0 or 1)
// decrase timers and update interface
// missig is external clock for the 6840 timers
  if (((st300loc.cr1 & 0x02) == param) && (st300loc.cr1 & 0x80) && (st300loc.reset ==0)) {
  	if (snddatst300.timer1 > 0) {
        	snddatst300.timer1--;
  	}
  	if ((snddatst300.timer1 == 0) && (st300loc.timlat1 != 0)) {
    		snddatst300.timer1 = st300loc.timlat1;
    		if (st300loc.timlat1 != st300loc.timlats1) {
    			playsam1(0);
    		}
    		st300loc.timlats1 = st300loc.timlat1;
    		st300loc.timp1 =  (st300loc.timp1 ? 0 : 1);
    	}
  }
  if (((st300loc.cr2 & 0x02) == param) && (st300loc.cr2 & 0x80) && (st300loc.reset ==0)) {
  	if (snddatst300.timer2 > 0) {
        	snddatst300.timer2--;
  	}
  	if ((snddatst300.timer2 == 0) && (st300loc.timlat2 != 0)) {
    		snddatst300.timer2 = st300loc.timlat2;
    		if (st300loc.timlat2 != st300loc.timlats2) {
    			playsam2(0);
    		}
    		st300loc.timlats2 = st300loc.timlat2;
    		st300loc.timp2 =  (st300loc.timp2 ? 0 : 1);

    	}
  }

  if (((st300loc.cr3 & 0x02) == param) && (st300loc.cr3 & 0x80) && (st300loc.reset ==0)) {
  	if (snddatst300.timer3 > 0) {
        	snddatst300.timer3--;
  	}
  	if ((snddatst300.timer3 == 0) && (st300loc.timlat3 != 0)) {
    		snddatst300.timer3 = st300loc.timlat3;
    		st300loc.timp3 =  (st300loc.timp3 ? 0 : 1);
		if (st300loc.timp3) {
			nextvol(0);
			mixer_set_volume(st300loc.channel,setvol(0)*ST300_VOL);
			mixer_set_volume(st300loc.channel+2,setvol(0)*ST300_VOL);			
    		}
    	}
  }
 
}


static int st300_sh_start(const struct MachineSound *msound)  {
  int i;
  int s;
  s= 0;
  memset(&st300loc, 0, sizeof(st300loc)); 
  for (i = 0;i < 9;i++) {
  	snddatst300.ax[i] = 0;
  	snddatst300.axb[i] = 0;  	
  	snddatst300.c0 = 0;
        }
  for (i = 0;i < 32000;i++) {
   	s =  (s ? 0 : 1);
  	if (s) {
  		sineWaveext[i] = rand() ;
  	} else {sineWaveext[i] = 0-rand() ;}
  	} 
  st300loc.channel = mixer_allocate_channels(3,&st300_mixing_levels) ;
  mixer_set_volume(st300loc.channel,0);	   // the sound from timer pulse q2
  mixer_set_volume(st300loc.channel+1,70*ST300_VOL);  // the sound from timer pulse q1
  mixer_set_volume(st300loc.channel+2,0);  // external timer pulse (after 4536)
  timer_pulse(TIME_IN_HZ(ST300_INTCLOCK),0x02,st300_pulse); // start internal clock
  return 0;
}
static void st300_sh_stop(void) {
}

static struct CustomSound_interface st300_custInt = {st300_sh_start, st300_sh_stop};


static struct {
  struct sndbrdData brdData;
} sts_locals;


static WRITE_HANDLER(st300_ctrl_w) {
// cycles for 4536
// c0 -> 00 -> 2 clock cycles
// c0 -> 10 -> 4 clock cycles
// c0 -> 20 -> 8 clock cycles
  snddatst300.c0=data;
  st300loc.extfreq  = ((data & 0xf0) >> 4) + 1;
  st300loc.noise    =  (data & 0x08) >> 3;
  st300loc.conx = (data & 0x04) >> 2;
  st300loc.altx = (data & 0x02) >> 1;  
  if (snddatst300.c0 & 0x01)  { // count up 
  	st300loc.dir  = 1;
  } else {
  	st300loc.dir  = 0;
  }
  logerror("%04x: st300_CTRL_W adress C0 data %02x noise %04x \n", activecpu_get_previouspc(), data,st300loc.noise);
  playsamext(0); 
  playsam3(0);
}
	
	
static WRITE_HANDLER(st300_data_w) {
  int w;
  long int w1;
  if (data == 3) {
	st300loc.timlat1 = snddatst300.ax[data] + snddatst300.ax[(data-1)] * 256;
	snddatst300.timer1 = st300loc.timlat1;
	w1 = ST300_INTCLOCK / (2 * (snddatst300.timer1 + 1));
	st300loc.tfre1 = w1;
	logerror("%04x: st300_data_w timlat1 loaded %04x  \n", activecpu_get_previouspc(), st300loc.timlat1);
  }
  if (data == 5) {
	st300loc.timlat2 = snddatst300.ax[data] + snddatst300.ax[(data-1)] * 256;
	snddatst300.timer2 = st300loc.timlat2;
	st300loc.tfre2 = ST300_INTCLOCK / (2 * (snddatst300.timer2 + 1));
        logerror("%04x: st300_data_w timlat2 loaded %04x freq %04d  \n", activecpu_get_previouspc(), st300loc.timlat2,st300loc.tfre2);
  }
  if (data == 7) {
	st300loc.timlat3 = snddatst300.ax[data] + snddatst300.ax[(data-1)] * 256;
	snddatst300.timer3 = st300loc.timlat3;
	st300loc.tfre3 = (ST300_INTCLOCK / (2 * (snddatst300.timer3 + 1)));		
	logerror("%04x: st300_data_w timlat3 loaded %04x freq %04d  \n", activecpu_get_previouspc(), st300loc.timlat3,st300loc.tfre3);
  }
  if (data == 1)  {
	st300loc.cr2= snddatst300.ax[data];
	logerror("%04x: st300_data_w CR2 %02x ", activecpu_get_previouspc(), st300loc.cr2);
	if ((st300loc.cr2 & 0x80) == 0) {	
  	}  
	if (st300loc.cr2 & 0x80)  {
		logerror ("Output enabl ");
 		playsam2(0);
	} else {
		logerror ("Output OFF   ");
		mixer_stop_sample(st300loc.channel);
  		st300loc.volnr = 0;
	}
	if (st300loc.cr2 & 0x40)  {
		logerror ("Inter  ENABLE ");
	} else {
		logerror ("Inter  off    ");
	}
	w = (st300loc.cr2 & 0x38) >> 3;
 	logerror ("Mode (N 2)   %01x ",w);
	if (st300loc.cr2 & 0x04)  {
		logerror ("count d 8 ");
	} else {
		logerror ("count 16  ");
	}
	if (st300loc.cr2 & 0x02)  {
		logerror ("int clock ");
	} else {
		logerror ("ext clock ");
	}
	logerror ("\n");
        st300loc.timp2 =  0;
        }

	if (data == 0) {
	        if ((st300loc.cr2 & 0x01) == 0x01) {
			st300loc.cr1 = snddatst300.ax[data];
			logerror("%04x: st300_data_w CR1 %02x ", activecpu_get_previouspc(), st300loc.cr1);
// check reset very early !!!
			if (st300loc.cr1 & 0x01)  {
				logerror ("reset ");
				softreset(1);
			}
			else {
				softreset(0);
				logerror ("normal mode ");
			}
			if (st300loc.cr1 & 0x80)  {
				logerror ("Output enabl ");
  				playsam1(0);
			}
			else {
				logerror ("Output OFF   ");
				mixer_stop_sample(st300loc.channel+1);
			}
			if (st300loc.cr1 & 0x40)  {
				logerror ("Inter  ENABLE ");
			}
			else {
				logerror ("Inter  off    ");
			}
			w = (st300loc.cr1 & 0x38) >> 3;
 			logerror ("Mode (N 2)   %01x ",w);
			if (st300loc.cr1 & 0x04)  {
				logerror ("count d 8 ");
			}
			else {
				logerror ("count 16  ");
			}
			if (st300loc.cr1 & 0x02)  {
				logerror ("int clock ");
			}
			else {
				logerror ("ext clock ");
			}
			logerror ("\n");
        	} else {
			st300loc.cr3 = snddatst300.ax[data];
			logerror("%04x: st300_data_w CR3 %02x ", activecpu_get_previouspc(), st300loc.cr3);
			if (st300loc.cr3 & 0x80)  {
				logerror ("Output enabl ");
			} else {
				logerror ("Output OFFM   ");
			}
			if (st300loc.cr3 & 0x40)  {
				logerror ("Inter  ENABLE ");
			}
			else {
				logerror ("Inter  off    ");
			}
			w = (st300loc.cr3 & 0x38) >> 3;
 			logerror ("Mode (N 2)   %01x ",w);
			if (st300loc.cr3 & 0x04)  {
				logerror ("count d 8 ");
			} else {
				logerror ("count 16  ");
			}
			if (st300loc.cr3 & 0x02)  {
				logerror ("int clock ");
			} else {
				logerror ("ext clock ");
			}
			if (st300loc.cr3 & 0x01)  {
				logerror ("clock / 8 ");
			} else {
				logerror ("clock / 1 ");
			}
			logerror ("\n");
       		}
       }
  }




static WRITE_HANDLER(sts_ctrl_w)
{
	logerror("snd_ctrl_w: %i\n", data);
}

static WRITE_HANDLER(sts_data_w)
{
    logerror("snd_data_w: %i\n", data);
}

static void sts_init(struct sndbrdData *brdData)
{
	memset(&sts_locals, 0x00, sizeof(sts_locals));
	sts_locals.brdData = *brdData;
}

/*-------------------
/ exported interfaces
/--------------------*/
const struct sndbrdIntf st100Intf = {
  "ST100", sts_init, NULL, NULL, sts_data_w, sts_data_w, NULL, sts_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

const struct sndbrdIntf st300Intf = {
  "ST300", sts_init, NULL, NULL, st300_data_w, st300_data_w,NULL, st300_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

const struct sndbrdIntf astroIntf = {
  "ASTRO", sts_init, NULL, NULL, st300_data_w, st300_data_w,NULL, st300_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

MACHINE_DRIVER_START(st300)
  MDRV_SOUND_ADD(CUSTOM, st300_custInt)
MACHINE_DRIVER_END
