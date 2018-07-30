//!! look at MAME exidy.c, for the ST-300 noise generator?? (intro'ed in MAME 0.56 (src/sndhrdw/exidy.c))

#include "driver.h"
#include "core.h"
#include "snd_cmd.h"
#include "sndbrd.h"
#include "stsnd.h"
#include "math.h"

#define MASTER_CLOCK 10000000
#define S14001_CLOCK (MASTER_CLOCK / 4)

/*----------------------------------------
/ Stern Sound System
/ 3 different boards:
/
/ ST-100:  discrete. dip sw 23 on plays simple tones, off plays electronic chimes
/ ST-100B: discrete. plays simple tones, no electronic chimes
/ ST-300:  discrete, + VS-1000 speech
/ ASTRO:   discrete, can switch between SB-100 and SB-300
/-----------------------------------------*/
// support added soundboard st-100 by Oliver Kaegi (12/12/2004)
// support added soundboard st-300 by Oliver Kaegi (14/09/2004)
// support added for vs-100 speech by Oliver Kaegi (10/10/2004)


static INT16 s100Waveb1[] = {
	 32736,  26136,  19008,  12144,  7392, 3960,  2376,  264,
	-32736, -26136, -19008, -12144, -7392,-3960, -2376, -264
};
//	 0x7FFF, 0x6000, 0x5000, 0x4000, 0x3000, 0x2000,  0x1000, 0x0000
//	-0x7FFF,-0x6000,-0x5000,-0x4000,-0x3000,-0x2000, -0x1000,-0x0000

//	 0x7FFF, 0x4500, 0x4000, 0x3000, 0x2000, 0x1500,  0x1000, 0x0500
//	-0x7FFF,-0x4500,-0x4000,-0x3000,-0x2000,-0x1500, -0x1000,-0x0500

static INT16 s100Waveb5[] = {
	     0, -16000, -18000, -17800, -17400, -17000, -16800, -16400, -16000, -15800, -15400, -15000,
	-14800, -14400, -14000, -13800, -13400, -13000,  -7000,   8000,  15000,  18000,  20000,  20000,
	 20000,  19000,  19000,  18000,  17000,  16000,  15000,  14500
//	     0, -16000, -18000, -16000, -16000, -16000, -16000, -14000, -15000, -15000,
//	-14000, -14000, -13000, -14000, -12000, -12000, -11000, -14000,  -7000,   8000,
//	 15000,  18000,  20000,  20000,  20000,  19000,  19000,  18000,  19000,  16000, 16000, 16000
};


static INT16 freqarb5[] = {
	500,400,300,200,200,300,9999
};

static INT16 s100Waveb6[] = {
//	0x7FFF,-0x7FFF,0x7FFF,-0x7FFF,0x7FFF,-0x7FFF,0x7FFF,-0x7FFF,0x7FFF,-0x7FFF,0x7FFF,-0x7FFF,0x7FFF,-0x7FFF,0x7FFF,-0x7FFF
	0x7FFF,-0x7FFF,0x7FFF,-0x7FFF,0x7FFF,-0x7FFF,0x7FFF,-0x7FFF
};

static INT16 freqarb6[] = {
//	170,150,130,110,90,70,
//	50,30,10,1,1,1,9999,
//	120,120,120,120,120,120,
//	100,100,100,100,100,100,
	340,320,300,280,260,240,220,200,180,160,140,120,100,80,60,40,30,20,
	9999
};


static struct {
	void	*ecdecayTimer;
	int		channel;
	int		freqb5;
	int		freqb6;
	int		ecvol[4];
} st100loc;


// 3 LM324 square wave circuits. Revision A boards were set to higher frequencies

#define ST100_FREQ1 	1046            // Note C6
#define ST100_FREQ2 	880             // Note A5
#define ST100_FREQ3 	698             // Note F5
#define ST100_FREQ4 	ST100_FREQ1/2   // Note C5   // (Flip flop divides Freq1 by 2)

// 3 LM324 square wave circuits. Revision B and C-1 boards were set to lower frequencies

#define ST100B_FREQ1	208             // Note G#3
#define ST100B_FREQ2	172             // Note F3
#define ST100B_FREQ3	138             // Note C#3
#define ST100B_FREQ4	ST100B_FREQ1/2  // Note G#2  // (Flip flop divides Freq1 by 2)


static void changefr(int param) {
//	emulating the 556
	if (freqarb5 [st100loc.freqb5] < 9999) {
		st100loc.freqb5++;
		logerror("setfreq5 %i\n", freqarb5 [st100loc.freqb5] );
	}
	if (freqarb5 [st100loc.freqb5] < 9999) {
		mixer_set_sample_frequency(st100loc.channel+4,freqarb5 [st100loc.freqb5] * sizeof(s100Waveb5));
	}
//	emulating the two LM324
	if (freqarb6 [st100loc.freqb6] < 9999) {
		st100loc.freqb6++;
		logerror("setfreq6 %i\n", freqarb6 [st100loc.freqb6] );
	}
	if (freqarb6 [st100loc.freqb6] < 9999) {
		mixer_set_sample_frequency(st100loc.channel+5,freqarb6 [st100loc.freqb6] * sizeof(s100Waveb6));
	} else {
		logerror("stopsample 6\n");
		mixer_set_volume(st100loc.channel+5,0);		// bit 5
	}
}

static void st100_elec_chime(int dummy) {
//	Simulate the volume decay for the electronic chimes
	if (st100loc.ecvol[0]) {
		st100loc.ecvol[0] -= 2;
		mixer_set_volume(st100loc.channel+6,st100loc.ecvol[0]);
		mixer_set_volume(st100loc.channel+7,st100loc.ecvol[0]);
	}
	if (st100loc.ecvol[1]) {
		st100loc.ecvol[1] -= 2;
		mixer_set_volume(st100loc.channel+8,st100loc.ecvol[1]);
		mixer_set_volume(st100loc.channel+9,st100loc.ecvol[1]);
	}
	if (st100loc.ecvol[2]) {
		st100loc.ecvol[2] -= 2;
		mixer_set_volume(st100loc.channel+10,st100loc.ecvol[2]);
		mixer_set_volume(st100loc.channel+11,st100loc.ecvol[2]);
	}
	if (st100loc.ecvol[3]) {
		st100loc.ecvol[3] -= 2;
		mixer_set_volume(st100loc.channel+12,st100loc.ecvol[3]);
		mixer_set_volume(st100loc.channel+13,st100loc.ecvol[3]);
	}
	if ((st100loc.ecvol[0]==0) && (st100loc.ecvol[1]==0) && (st100loc.ecvol[2]==0) && (st100loc.ecvol[3]== 0)) {
		timer_enable(st100loc.ecdecayTimer, FALSE);
	}
//	logerror("st100_elec_chime: Vol0=%02x, Vol1=%02x, Vol2=%02x, Vol3=%02x\n", st100loc.ecvol[0], st100loc.ecvol[1], st100loc.ecvol[2], st100loc.ecvol[3]);
}


static int st100b_sh_start(const struct MachineSound *msound) {
	int mixing_levels[6] = {25,25,25,25,25,25};
	memset(&st100loc, 0, sizeof(st100loc));
	st100loc.channel = mixer_allocate_channels(6, mixing_levels);

	mixer_set_name(st100loc.channel + 0, "ST100 0");			//     10 point sound
	mixer_set_volume(st100loc.channel+0,0);		// bit 1
	mixer_set_name(st100loc.channel + 1, "ST100 1");			//    100 point sound
	mixer_set_volume(st100loc.channel+1,0);		// bit 2
	mixer_set_name(st100loc.channel + 2, "ST100 2");			//  1,000 point sound
	mixer_set_volume(st100loc.channel+2,0);		// bit 3
	mixer_set_name(st100loc.channel + 3, "ST100 3");			// 10,000 point sound
	mixer_set_volume(st100loc.channel+3,0);		// bit 4
	mixer_set_name(st100loc.channel + 4, "ST100 4");			// Add bonus sound
	mixer_set_volume(st100loc.channel+4,0);		// bit 5
	mixer_set_name(st100loc.channel + 5, "ST100 5");			// Pop Bumper chirp
	mixer_set_volume(st100loc.channel+5,0);		// bit 6

	mixer_play_sample_16(st100loc.channel+0,s100Waveb1, sizeof(s100Waveb1), ST100B_FREQ1*(sizeof(s100Waveb1)/2), 1);
	mixer_play_sample_16(st100loc.channel+1,s100Waveb1, sizeof(s100Waveb1), ST100B_FREQ2*(sizeof(s100Waveb1)/2), 1);
	mixer_play_sample_16(st100loc.channel+2,s100Waveb1, sizeof(s100Waveb1), ST100B_FREQ3*(sizeof(s100Waveb1)/2), 1);
	mixer_play_sample_16(st100loc.channel+3,s100Waveb1, sizeof(s100Waveb1), ST100B_FREQ4*(sizeof(s100Waveb1)/2), 1);

	st100loc.freqb5 = 0;
	mixer_play_sample_16(st100loc.channel+4,s100Waveb5, sizeof(s100Waveb5), (freqarb5[st100loc.freqb5])*sizeof(s100Waveb5), 1);
	st100loc.freqb6 = 0;
	mixer_play_sample_16(st100loc.channel+5,s100Waveb6, sizeof(s100Waveb6), (freqarb6[st100loc.freqb6])*sizeof(s100Waveb6), 1);
	timer_pulse(TIME_IN_SEC(0.02),0,changefr);

	return 0;
}

static int st100_sh_start(const struct MachineSound *msound) {
	int mixing_levels[14] = {25,25,25,25,25,25,20,20,20,20,20,20,20,20};
	memset(&st100loc, 0, sizeof(st100loc));
	st100loc.channel = mixer_allocate_channels(14, mixing_levels);

	mixer_set_name(st100loc.channel +  0, "ST100 0");			//     10 point sound
	mixer_set_volume(st100loc.channel+ 0,0);	// bit 1
	mixer_set_name(st100loc.channel +  1, "ST100 1");			//    100 point sound
	mixer_set_volume(st100loc.channel+ 1,0);	// bit 2
	mixer_set_name(st100loc.channel +  2, "ST100 2");			//  1,000 point sound
	mixer_set_volume(st100loc.channel+ 2,0);	// bit 3
	mixer_set_name(st100loc.channel +  3, "ST100 3");			// 10,000 point sound
	mixer_set_volume(st100loc.channel+ 3,0);	// bit 4
	mixer_set_name(st100loc.channel +  4, "ST100 4");			// Add bonus sound
	mixer_set_volume(st100loc.channel+ 4,0);	// bit 5
	mixer_set_name(st100loc.channel +  5, "ST100 5");			// Pop Bumper chirp
	mixer_set_volume(st100loc.channel+ 5,0);	// bit 6

	mixer_set_name(st100loc.channel +  6, "ST100 CL0");			//     10 point chime base frequency
	mixer_set_volume(st100loc.channel+ 6,0);	// bit 1
	mixer_set_name(st100loc.channel +  7, "ST100 CH0");			//     10 point chime next harmonic frequency
	mixer_set_volume(st100loc.channel+ 7,0);	// bit 1
	mixer_set_name(st100loc.channel +  8, "ST100 CL1");			//    100 point chime base frequency
	mixer_set_volume(st100loc.channel+ 8,0);	// bit 2
	mixer_set_name(st100loc.channel +  9, "ST100 CH1");			//    100 point chime next harmonic frequency
	mixer_set_volume(st100loc.channel+ 9,0);	// bit 2
	mixer_set_name(st100loc.channel + 10, "ST100 CL2");			//  1,000 point chime base frequency
	mixer_set_volume(st100loc.channel+10,0);	// bit 3
	mixer_set_name(st100loc.channel + 11, "ST100 CH2");			//  1,000 point chime next harmonic frequency
	mixer_set_volume(st100loc.channel+11,0);	// bit 3
	mixer_set_name(st100loc.channel + 12, "ST100 CL3");			// 10,000 point chime base frequency
	mixer_set_volume(st100loc.channel+12,0);	// bit 4
	mixer_set_name(st100loc.channel + 13, "ST100 CH3");			// 10,000 point chime next harmonic frequency
	mixer_set_volume(st100loc.channel+13,0);	// bit 4

	mixer_play_sample_16(st100loc.channel+ 0,s100Waveb1, sizeof(s100Waveb1), ST100_FREQ1*(sizeof(s100Waveb1)/2), 1);
	mixer_play_sample_16(st100loc.channel+ 1,s100Waveb1, sizeof(s100Waveb1), ST100_FREQ2*(sizeof(s100Waveb1)/2), 1);
	mixer_play_sample_16(st100loc.channel+ 2,s100Waveb1, sizeof(s100Waveb1), ST100_FREQ3*(sizeof(s100Waveb1)/2), 1);
	mixer_play_sample_16(st100loc.channel+ 3,s100Waveb1, sizeof(s100Waveb1), ST100_FREQ4*(sizeof(s100Waveb1)/2), 1);

	st100loc.freqb5 = 0;
	mixer_play_sample_16(st100loc.channel+ 4,s100Waveb5, sizeof(s100Waveb5), (freqarb5[st100loc.freqb5])*sizeof(s100Waveb5), 1);
	st100loc.freqb6 = 0;
	mixer_play_sample_16(st100loc.channel+ 5,s100Waveb6, sizeof(s100Waveb6), (freqarb6[st100loc.freqb6])*sizeof(s100Waveb6), 1);
	timer_pulse(TIME_IN_SEC(0.02),0,changefr);

	mixer_play_sample_16(st100loc.channel+ 6,s100Waveb1, sizeof(s100Waveb1), ST100_FREQ1*(sizeof(s100Waveb1)/2), 1);	// Electronic chimes base frequency
	mixer_play_sample_16(st100loc.channel+ 7,s100Waveb1, sizeof(s100Waveb1), ST100_FREQ1*sizeof(s100Waveb1), 1);		// Electronic chimes next harmonic
	mixer_play_sample_16(st100loc.channel+ 8,s100Waveb1, sizeof(s100Waveb1), ST100_FREQ2*(sizeof(s100Waveb1)/2), 1);
	mixer_play_sample_16(st100loc.channel+ 9,s100Waveb1, sizeof(s100Waveb1), ST100_FREQ2*sizeof(s100Waveb1), 1);
	mixer_play_sample_16(st100loc.channel+10,s100Waveb1, sizeof(s100Waveb1), ST100_FREQ3*(sizeof(s100Waveb1)/2), 1);
	mixer_play_sample_16(st100loc.channel+11,s100Waveb1, sizeof(s100Waveb1), ST100_FREQ3*sizeof(s100Waveb1), 1);
	mixer_play_sample_16(st100loc.channel+12,s100Waveb1, sizeof(s100Waveb1), ST100_FREQ4*(sizeof(s100Waveb1)/2), 1);
	mixer_play_sample_16(st100loc.channel+13,s100Waveb1, sizeof(s100Waveb1), ST100_FREQ4*sizeof(s100Waveb1), 1);
	st100loc.ecdecayTimer = timer_alloc(st100_elec_chime);
	timer_enable(st100loc.ecdecayTimer, FALSE);

	return 0;
}

static void st100b_sh_stop(void) {
	mixer_stop_sample(st100loc.channel+0);
	mixer_stop_sample(st100loc.channel+1);
	mixer_stop_sample(st100loc.channel+2);
	mixer_stop_sample(st100loc.channel+3);
	mixer_stop_sample(st100loc.channel+4);
	mixer_stop_sample(st100loc.channel+5);
}

static void st100_sh_stop(void) {
	st100b_sh_stop();

	mixer_stop_sample(st100loc.channel+6);
	mixer_stop_sample(st100loc.channel+7);
	mixer_stop_sample(st100loc.channel+8);
	mixer_stop_sample(st100loc.channel+9);
	mixer_stop_sample(st100loc.channel+10);
	mixer_stop_sample(st100loc.channel+11);
	mixer_stop_sample(st100loc.channel+12);
	mixer_stop_sample(st100loc.channel+13);
}


static WRITE_HANDLER(sts_data_w)
{
	if (data & 0x01) {
		mixer_set_volume(st100loc.channel,100);		// bit 1
		logerror("playsample 1 %i\n", data);
	} else {
		mixer_set_volume(st100loc.channel,0);		// bit 1
	}
	if (data & 0x02) {
		mixer_set_volume(st100loc.channel+1,100);	// bit 2
		logerror("playsample 2 %i\n", data);
	} else {
		mixer_set_volume(st100loc.channel+1,0);		// bit 2
	}
	if (data & 0x04) {
		mixer_set_volume(st100loc.channel+2,100);	// bit 3
		logerror("playsample 3 %i\n", data);
	} else {
		mixer_set_volume(st100loc.channel+2,0);		// bit 3
	}
	if (data & 0x08) {
		mixer_set_volume(st100loc.channel+3,100);	// bit 4
		logerror("playsample 4 %i\n", data);
	} else {
		mixer_set_volume(st100loc.channel+3,0);		// bit 4
	}
	if (data & 0x10) {
		st100loc.freqb5 = 0;
		mixer_set_sample_frequency(st100loc.channel+4,freqarb5 [st100loc.freqb5] * sizeof(s100Waveb5));
		mixer_set_volume(st100loc.channel+4,100);	// bit 5
		logerror("playsample 5 %i\n", data);
	} else {
		logerror("stopsample 5 %i\n", data);
		mixer_set_volume(st100loc.channel+4,0);		// bit 5
	}
	if (data & 0x20) {
		st100loc.freqb6 = 0;
		mixer_set_sample_frequency(st100loc.channel+5,freqarb6 [st100loc.freqb6] * sizeof(s100Waveb6));
		mixer_set_volume(st100loc.channel+5,40);	// bit 6
		logerror("playsample 6 %i\n", data);
	} else {
//		logerror("stopsample 6 %i\n", data);
//		mixer_set_volume(st100loc.channel+5,0);		// bit 6
	}
	logerror("snd_data_w: %i\n", data);
}

static WRITE_HANDLER(sts_ctrl_w)
{
	if (data & 0x01) {
		st100loc.ecvol[0] = 100;
		mixer_set_volume(st100loc.channel+6,st100loc.ecvol[0]);
		mixer_set_volume(st100loc.channel+7,st100loc.ecvol[0]);
		if (timer_enable(st100loc.ecdecayTimer, TRUE) == FALSE) {
			timer_adjust(st100loc.ecdecayTimer, TIME_NOW, 0, TIME_IN_MSEC(8.0));
		}
//		logerror("playsample electronic chime 1 %i\n", data);
	}
	if (data & 0x02) {
		st100loc.ecvol[1] = 100;
		mixer_set_volume(st100loc.channel+8,st100loc.ecvol[1]);
		mixer_set_volume(st100loc.channel+9,st100loc.ecvol[1]);
		if (timer_enable(st100loc.ecdecayTimer, TRUE) == FALSE) {
			timer_adjust(st100loc.ecdecayTimer, TIME_NOW, 0, TIME_IN_MSEC(8.0));
		}
//		logerror("playsample electronic chime 2 %i\n", data);
	}
	if (data & 0x04) {
		st100loc.ecvol[2] = 100;
		mixer_set_volume(st100loc.channel+10,st100loc.ecvol[2]);
		mixer_set_volume(st100loc.channel+11,st100loc.ecvol[2]);
		if (timer_enable(st100loc.ecdecayTimer, TRUE) == FALSE) {
			timer_adjust(st100loc.ecdecayTimer, TIME_NOW, 0, TIME_IN_MSEC(8.0));
		}
//		logerror("playsample electronic chime 3 %i\n", data);
	}
	if (data & 0x08) {
		st100loc.ecvol[3] = 100;
		mixer_set_volume(st100loc.channel+12,st100loc.ecvol[3]);
		mixer_set_volume(st100loc.channel+13,st100loc.ecvol[3]);
		if (timer_enable(st100loc.ecdecayTimer, TRUE) == FALSE) {
			timer_adjust(st100loc.ecdecayTimer, TIME_NOW, 0, TIME_IN_MSEC(8.0));
		}
//		logerror("playsample electronic chime 4 %i\n", data);
	}
	logerror("snd_ctrl_w: %i\n", data);
}


#define ST300_INTCLOCK    1000000  // clock speed in hz of mpu ST-200 board ! -> 0.000'001 sec

#define ST300_VOL 1 // volume for channels

struct sndbrdst300 snddatst300;

static struct {
	UINT16	timlat1,timlat2,timlat3,timer1,timer2,timer3,t4562c;
	UINT16	timlats1,timlats2,timlats3;
	int		cr1,cr2,cr3,channel,timp1,timp2,timp3,tfre1,tfre2,tfre3,noise,conx,altx,dir;
	int		volnr,reset,extfreq,voiceSw;
} st300loc;

static INT16 sineWaveinp[] = {
	 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF,
	-0x7FFF,-0x7FFF,-0x7FFF,-0x7FFF,-0x7FFF,-0x7FFF,-0x7FFF,-0x7FFF
}; // q2 wave

static INT16 sineWaveinpq1[] = {
	 0x4000, 0x2000, 0x7fff, 0x1000, 0x0000, 0x0000, 0x0000, 0x0000,
	-0x4000,-0x2000,-0x7fff,-0x1000,-0x0000,-0x0000,-0x0000,-0x0000
}; // q1 wave


static INT16 volume300[] = {
	0, 0, 0, 0, 0, 10, 20, 20, 30, 30, 40, 40, 60, 60, 80, 90, 100, 100
};

static INT16 sineWaveext[16][32000]; // wave triggered by external clock, very rough approximation for noise generator (related to exidy?)


static int setvol(int param) {
	if ((st300loc.volnr) == 0) return 0;
	if (st300loc.dir) { // count up
//		logerror("volume up %04d ind %04d \n",volume300[st300loc.volnr],st300loc.volnr );
		return volume300[st300loc.volnr];
	} else {
//		logerror("volume down %04d ind %04d \n",volume300[17-st300loc.volnr],st300loc.volnr );
		return volume300[17-st300loc.volnr];
	}
}
static void startvol(int param) {
	st300loc.volnr = 1;
}

static void nextvol(int param) {
	if ((st300loc.volnr > 0) && (st300loc.volnr <= 16)) st300loc.volnr++;
	if (st300loc.volnr > 16) {
		if (st300loc.conx == 0) {
			if ((st300loc.dir) == 0) {
				st300loc.volnr = 0;	// quiet if we are going down...
			}
		} else {
			startvol(0);
			if (st300loc.altx) {
				st300loc.dir = (st300loc.dir ? 0 : 1);
			}
		}
	}
}


static void playsam3(int param) {
// timer q3 from 6840 is used for volume control
	if ((st300loc.cr3 & 0x80) && (st300loc.timlat3 > 0) && (st300loc.reset == 0)) {		// output is enabled...
		startvol(0);
		if (setvol(0) == 0) {
			logerror("playsam Q2/Q3noise volume off \n");
		}
		if (setvol(0) == 100) {
			logerror("playsam Q2/Q3noise volume maximum\n");
		}

		mixer_set_volume(st300loc.channel,setvol(0)*ST300_VOL);
		mixer_set_volume(st300loc.channel+2,setvol(0)*ST300_VOL);
	} else {	// q3 is not running... //!! exact same code!
		startvol(0);
		if (setvol(0) == 0) {
			logerror("playsam Q2/EXT noise volume off \n");
		}
		if (setvol(0) == 100) {
			logerror("playsam q2/EXT noise volume maximum\n");
		}

		mixer_set_volume(st300loc.channel,setvol(0)*ST300_VOL);
		mixer_set_volume(st300loc.channel+2,setvol(0)*ST300_VOL);
	}
}

static void playsam2(int param){
// timer 2 (q2) is easy wave + volume from q3
	if ((st300loc.cr2 & 0x80) && (st300loc.timlat2 > 0) && (st300loc.reset == 0)) {		// output is enabled...
		mixer_play_sample_16(st300loc.channel,sineWaveinp, sizeof(sineWaveinp), (int)(st300loc.tfre2*sizeof(sineWaveinp) / 2 / 1.137), 1);
		logerror("*** playsam Q2 start %04d ***\n",st300loc.tfre2);
	}
}

static void playsam1(int param){
// timer 1 (q1) is easy wave + volume always 100
	if ((st300loc.cr1 & 0x80) && (st300loc.timlat1 > 0) && (st300loc.reset == 0)) {		// output is enabled...
		mixer_play_sample_16(st300loc.channel+1,sineWaveinpq1, sizeof(sineWaveinpq1), (int)(st300loc.tfre1*sizeof(sineWaveinpq1) / 2 / 1.137), 1);
		logerror("*** playsam Q1 start %04d ***\n",st300loc.tfre1);
	}
}


static void playsamext(int param){
//	external timer + volume from q3
//  extfreq = 1..16

//	Noise is not 100 % accurate, there is a wav file available from the author (solenoid 22 - 29)
//	formula for f not correct
	//int f;
	//f = (17-(st300loc.extfreq))*sizeof(sineWaveext);
	//f = 625000 / (17-(st300loc.extfreq)); //!! do real lowpass filter then instead of passing this on to the non-filtering mixer (in this case)?!

	if (st300loc.noise) {	// output is enabled...
		//mixer_play_sample_16(st300loc.channel+2,sineWaveext, sizeof(sineWaveext), f , 1);
		//logerror("*** playsam EXT noise frequence %08d data %04d ***\n",f,st300loc.extfreq);
		mixer_play_sample_16(st300loc.channel + 2, sineWaveext[st300loc.extfreq-1], sizeof(sineWaveext[st300loc.extfreq-1]), 48000, 1);
		logerror("*** playsam EXT noise data %04d ***\n", st300loc.extfreq);
	} else {
		mixer_stop_sample(st300loc.channel+2);
		logerror("playsam EXT noise stop \n");
	}
}


static void softreset (int param) {
	st300loc.reset = param;
	if (st300loc.reset) {	// reset
		snddatst300.timer1 = st300loc.timlat1;
		snddatst300.timer2 = st300loc.timlat2;
		snddatst300.timer3 = st300loc.timlat3;
		st300loc.timp1 = 0;
		st300loc.timp2 = 0;
		st300loc.timp3 = 0;
		mixer_stop_sample(st300loc.channel);
		logerror ("Playsam Q2 off ");
		mixer_stop_sample(st300loc.channel+1);
		logerror ("Playsam Q1 off ");
	}
}


static void st300_pulse (int param) {
//	param = 0x02 -> internal 6840 clock
//	param = 0 -> external 4049 clock
//	timpX is the output level of the 6840 (only 0 or 1)
//	decrase timers and update interface
//	missig is external clock for the 6840 timers
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
			st300loc.timp1 = (st300loc.timp1 ? 0 : 1);
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
			st300loc.timp2 = (st300loc.timp2 ? 0 : 1);
		}
	}

	if (((st300loc.cr3 & 0x02) == param) && (st300loc.cr3 & 0x80) && (st300loc.reset ==0)) {
		if (snddatst300.timer3 > 0) {
			snddatst300.timer3--;
		}
		if ((snddatst300.timer3 == 0) && (st300loc.timlat3 != 0)) {
			snddatst300.timer3 = st300loc.timlat3;
			st300loc.timp3 = (st300loc.timp3 ? 0 : 1);
			if (st300loc.timp3) {
				nextvol(0);
				if (setvol(0) == 0) {
					logerror("playsam Q2/EXT noise volume off \n");
				}
				if (setvol(0) == 100) {
					logerror("playsam Q2/EXT noise volume maximum\n");
				}
				mixer_set_volume(st300loc.channel,setvol(0)*ST300_VOL);
				mixer_set_volume(st300loc.channel+2,setvol(0)*ST300_VOL);
			}
		}
	}
}


// return pseudo random white noise number in the range -scale to scale
static unsigned long noise_seed = 0xdeadbabe;
static float white_noise(const float scale)
{
	unsigned long white;
	noise_seed = noise_seed * 196314165 + 907633515;
	white = noise_seed >> 9;
	white |= 0x40000000;
	return ((*(float*)&white) - 3.0f)*scale;
}

// return brown noise random number in the range -scale to scale
static float noise_brown = 0.0f;
static float brown_noise(const float scale)
{
	while(1)
	{
		const float white = white_noise(0.5f);
		noise_brown += white;
		if((noise_brown < -8.0f) || (noise_brown > 8.0f))
			noise_brown -= white;
		else
			break;
	}
	return noise_brown*scale*(float)(0.0625/0.5);
}


static int st300_sh_start(const struct MachineSound *msound) {
	int mixing_levels[3] = {30,30,30};
	int i,j;
	//int s = 0;

	memset(&st300loc, 0, sizeof(st300loc));
	for (i = 0;i < 9;i++) {
		snddatst300.ax[i] = 0;
		snddatst300.axb[i] = 0;
		snddatst300.c0 = 0;
	}

	// very very rough approximation sample table for what the real noise generator is doing:
	// original noise sample table in [0]
	for (i = 0;i < 32000;++i) {
		//s = (s ? 0 : 1);
		//if (s) {
			sineWaveext[0][i] = (INT16)white_noise((float)(32767*0.85)); //rand()*2-32767;
		//} else {
		//	sineWaveext[i] = 0-rand();
		//}
	}
	// add some brown noise to the white noise
	for (i = 0;i < 32000;++i)
		sineWaveext[0][i] += (INT16)brown_noise((float)(32767*0.15));

	// box filtered noise sample tables in [1]..[15]
	for (j = 1; j < 16; ++j) {
		for (i = 0; i < 32000; ++i) {
			int tmp = 0;
			int k;
			for (k = -j*2; k <= j*2; ++k) // magic, filter down to something that loosely matches the original
			{
				int ofs;
				if ((i + k) < 0)
					ofs = 32000+(i+k);
				else if ((i+k) >= 32000)
					ofs = (i+k)-32000;
				else
					ofs = i+k;
				tmp += sineWaveext[0][ofs];
			}
			tmp /= (j+1)/2; //tmp /= j*2 * 2 + 1; // magic, to loosely match the volume of the original
			sineWaveext[j][i] = (tmp < -32768) ? -32768 : ((tmp > 32767) ? 32767 : tmp);
		}
	}


	st300loc.channel = mixer_allocate_channels(3, mixing_levels);
	mixer_set_name  (st300loc.channel, "MC6840 #Q2");		// 6840 Output timer 2 (q2) is easy wave + volume from q3
	mixer_set_volume(st300loc.channel,0);
	mixer_set_name  (st300loc.channel+1, "MC6840 #Q1");		// 6840 Output timer 1 (q1) is easy wave + volume always 100
	mixer_set_volume(st300loc.channel+1,70*ST300_VOL);
	mixer_set_name  (st300loc.channel+2, "EXT TIM");		// External Timer (U10) is Noise generator + volume from q3
	mixer_set_volume(st300loc.channel+2,0);
	timer_pulse(TIME_IN_HZ(ST300_INTCLOCK),0x02,st300_pulse); // start internal clock
	return 0;
}

static void st300_sh_stop(void) {
	samples_sh_stop();
}


static struct CustomSound_interface st300_custInt = {st300_sh_start, st300_sh_stop};
static struct CustomSound_interface st300v_custInt = {st300_sh_start, st300_sh_stop};
static struct CustomSound_interface st100_custInt = {st100_sh_start, st100_sh_stop};
static struct CustomSound_interface st100b_custInt = {st100b_sh_start, st100b_sh_stop};

static struct {
	struct sndbrdData brdData;
} sts_locals;


static WRITE_HANDLER(st300_ctrl_w) {
	if (!st300loc.voiceSw) {
//	cycles for 4536
//	c0 -> 00 -> 2 clock cycles
//	c0 -> 10 -> 4 clock cycles
//	c0 -> 20 -> 8 clock cycles
		snddatst300.c0 = data;
		st300loc.extfreq = ((data & 0xf0) >> 4) + 1;
		st300loc.noise   =  (data & 0x08) >> 3;
		st300loc.conx = (data & 0x04) >> 2;
		st300loc.altx = (data & 0x02) >> 1;
		if (snddatst300.c0 & 0x01) {	// count up
			st300loc.dir = 1;
		} else {
			st300loc.dir = 0;
		}
		logerror("st300_CTRL_W adress C0 data %02x noise %04x \n", data,st300loc.noise);
		playsamext(0);
		playsam3(0);
	} else {
		logerror("st300_CTRL_W xxxx data %02x  \n", data);
		if (data & 0x80) {	// VSU-1000 control write
			int clock_divisor = 16 - (data & 0x07);
			logerror("st300_CTRL_W Voicespeed data %02x speed %02x vol %02x  \n", data, data & 0x07, ((data >> 3) & 0xf));
			/* volume and frequency control goes here */
			S14001A_set_volume(15-((data >> 3) & 0xf));
			//m_s14001a->set_output_gain(0, ((data >> 3 & 0xf) + 1) / 16.0); //!! from MAME
			/* clock control - the first LS161 divides the clock by 9 to 16, the 2nd by 8,
			   giving a final clock from 19.5kHz to 34.7kHz */
			S14001A_set_rate(/*data & 0x07*/S14001_CLOCK / clock_divisor / 8);
		}
		else if (data & 0x40)
		{
			logerror("st300_CTRL_W Voice data %02x sam %02x \n", data, data & 0x3f);
			/* write to the register */
			S14001A_reg_0_w(data & 0x3f);
			S14001A_rst_0_w(1);
			S14001A_rst_0_w(0);
		}
	}
	st300loc.voiceSw = 0;
}


static WRITE_HANDLER(st300_data_w) {
	int w;
	long int w1;
	if (data == 3) {
		st300loc.timlat1 = snddatst300.ax[data] + snddatst300.ax[(data-1)] * 256;
		snddatst300.timer1 = st300loc.timlat1;
		w1 = ST300_INTCLOCK / (2 * (snddatst300.timer1 + 1));
		st300loc.tfre1 = w1;
//		logerror("%04x: st300_data_w timlat1 loaded %04x  \n", activecpu_get_previouspc(), st300loc.timlat1);
		if (st300loc.timlat1 == 0) {
			mixer_stop_sample(st300loc.channel+1);
			logerror ("Playsam Q1 off\n");
		}
	}
	if (data == 5) {
		st300loc.timlat2 = snddatst300.ax[data] + snddatst300.ax[(data-1)] * 256;
		snddatst300.timer2 = st300loc.timlat2;
		st300loc.tfre2 = ST300_INTCLOCK / (2 * (snddatst300.timer2 + 1));
//		logerror("%04x: st300_data_w timlat2 loaded %04x freq %04d  \n", activecpu_get_previouspc(), st300loc.timlat2,st300loc.tfre2);
	}
	if (data == 7) {
		st300loc.timlat3 = snddatst300.ax[data] + snddatst300.ax[(data-1)] * 256;
		snddatst300.timer3 = st300loc.timlat3;
		st300loc.tfre3 = (ST300_INTCLOCK / (2 * (snddatst300.timer3 + 1)));
		logerror("%04x: st300_data_w timlat3 loaded %04x freq %04d  \n", activecpu_get_previouspc(), st300loc.timlat3,st300loc.tfre3);
	}
	if (data == 1) {
		st300loc.cr2= snddatst300.ax[data];
		logerror("%04x: st300_data_w CR2 %02x       ", activecpu_get_previouspc(), st300loc.cr2);
//		if ((st300loc.cr2 & 0x80) == 0) {
//		}
		if (st300loc.cr2 & 0x80) {
			logerror ("Output enabl ");
			playsam2(0);
		} else {
//		logerror ("Output OFF   ");
		logerror ("PlaysamQ2off ");
		mixer_stop_sample(st300loc.channel);
		st300loc.volnr = 0;
		}
		if (st300loc.cr2 & 0x40) {
			logerror ("Inter  ENABLE ");
		} else {
			logerror ("Inter  off    ");
		}
		w = (st300loc.cr2 & 0x38) >> 3;
		logerror ("Mode (N 2)   %01x ",w);
		if (st300loc.cr2 & 0x04) {
			logerror ("count d 8 ");
		} else {
			logerror ("count 16  ");
		}
		if (st300loc.cr2 & 0x02) {
			logerror ("int clock ");
		} else {
			logerror ("ext clock ");
		}
		logerror ("\n");
		st300loc.timp2 = 0;
	}

	if (data == 0) {
		if ((st300loc.cr2 & 0x01) == 0x01) {
			st300loc.cr1 = snddatst300.ax[data];
			logerror("%04x: st300_data_w CR1 %02x ", activecpu_get_previouspc(), st300loc.cr1);
//	check reset very early !!!
			if (st300loc.cr1 & 0x01) {
				logerror ("reset ");
				softreset(1);
			}
			else {
				softreset(0);
				logerror ("norm  ");
			}
			if (st300loc.cr1 & 0x80) {
				logerror ("Output enabl ");
				playsam1(0);
			}
			else {
//				logerror ("Output OFF   ");
				logerror ("PlaysamQ1off ");
				mixer_stop_sample(st300loc.channel+1);
			}
			if (st300loc.cr1 & 0x40) {
				logerror ("Inter  ENABLE ");
			}
			else {
				logerror ("Inter  off    ");
			}
			w = (st300loc.cr1 & 0x38) >> 3;
			logerror ("Mode (N 2)   %01x ",w);
			if (st300loc.cr1 & 0x04) {
				logerror ("count d 8 ");
			}
			else {
				logerror ("count 16  ");
			}
			if (st300loc.cr1 & 0x02) {
				logerror ("int clock ");
			}
			else {
				logerror ("ext clock ");
			}
			logerror ("\n");
		} else {
			st300loc.cr3 = snddatst300.ax[data];
			logerror("%04x: st300_data_w CR3 %02x       ", activecpu_get_previouspc(), st300loc.cr3);
			if (st300loc.cr3 & 0x80) {
				logerror ("Output enabl ");
			} else {
				logerror ("Output OFFM   ");
			}
			if (st300loc.cr3 & 0x40) {
				logerror ("Inter  ENABLE ");
			}
			else {
				logerror ("Inter  off    ");
			}
			w = (st300loc.cr3 & 0x38) >> 3;
			logerror ("Mode (N 2)   %01x ",w);
			if (st300loc.cr3 & 0x04) {
				logerror ("count d 8 ");
			} else {
				logerror ("count 16  ");
			}
			if (st300loc.cr3 & 0x02) {
			logerror ("int clock ");
			} else {
				logerror ("ext clock ");
			}
			if (st300loc.cr3 & 0x01) {
				logerror ("clock / 8 ");
			} else {
				logerror ("clock / 1 ");
			}
			logerror ("\n");
		}
	}
}



static void st300_switch_w(int sw) {
	st300loc.voiceSw = sw;
}

static WRITE_HANDLER(st300_man_w) {
	st300loc.voiceSw = 1;
	st300_ctrl_w(0, data);
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
	"ST300", sts_init, NULL, st300_switch_w, st300_man_w, st300_data_w,NULL, st300_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

const struct sndbrdIntf astroIntf = {
	"ASTRO", sts_init, NULL, NULL, st300_data_w, st300_data_w,NULL, st300_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

static struct S14001A_interface s14001a_interface = {
	VSU100_ROMREGION	/* memory region */
};

MACHINE_DRIVER_START(st100)
	MDRV_SOUND_ADD(CUSTOM, st100_custInt)
	MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st100b)
	MDRV_SOUND_ADD(CUSTOM, st100b_custInt)
	MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st300)
	MDRV_SOUND_ADD(CUSTOM, st300_custInt)
	MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(st300v)
	MDRV_SOUND_ADD(CUSTOM, st300v_custInt)
	MDRV_SOUND_ADD(SAMPLES, samples_interface)
	MDRV_SOUND_ADD_TAG("S14001A", S14001A, s14001a_interface)
MACHINE_DRIVER_END
