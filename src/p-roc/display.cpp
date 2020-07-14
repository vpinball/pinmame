#if defined(PINMAME) && defined(PROC_SUPPORT)

extern "C" {
#include "driver.h"
#include "wpc/core.h"
}
#include "p-roc.h"

// Buffer to hold the next full DMD frame to send to the P-ROC
UINT8 procdmd[PROC_NUM_DMD_FRAMES][0x200];

// Initialize the DMD logic in the P-ROC
void procDMDInit(void) {
	int i;

	// Create the structure that holds the DMD settings
	PRDMDConfig dmdConfig;
	memset(&dmdConfig, 0x0, sizeof(dmdConfig));

	dmdConfig.numRows = 32;
	dmdConfig.numColumns = 128;
	dmdConfig.numSubFrames = 4;
	dmdConfig.numFrameBuffers = 3;
	dmdConfig.autoIncBufferWrPtr = TRUE;
	dmdConfig.enableFrameEvents = TRUE;

	for (i = 0; i < dmdConfig.numSubFrames; i++) {
		dmdConfig.rclkLowCycles[i] = 15;
		dmdConfig.latchHighCycles[i] = 15;
		dmdConfig.dotclkHalfPeriod[i] = 1;
	}

	dmdConfig.deHighCycles[0] = 90;
	dmdConfig.deHighCycles[1] = 190;
	dmdConfig.deHighCycles[2] = 50;
	dmdConfig.deHighCycles[3] = 377;

	PRDMDUpdateConfig(proc, &dmdConfig);

	for (i = 0; i < PROC_NUM_DMD_FRAMES; i++) {
		memset(procdmd[i], 0, 0x200);
	}
}

// Reset the buffer.
void procClearDMD(void) {
	int i;
	for (i = 0; i < 4; i++) {
		memset(procdmd[i], 0, 0x200);
	}
}

// Fill in a dot.
void procDrawDot(int x, int y, int color) {
	uint8_t temp_dot = 0;
	uint8_t position = 0;
	const int mappedColors[] = {0, 2, 8, 10, 1, 3, 9, 11, 4, 6, 12, 14, 5, 7, 13, 15};

	int i;
	for (i = 0; i < 4; i++) {
		if ((mappedColors[color] >> i) & 0x1) {
			temp_dot = procdmd[i][(((128*y)+x))/8];
			position = ((128*y)+x)%8;
			temp_dot = temp_dot | (1<<position);
			procdmd[i][(((128*y)+x))/8] = temp_dot;
		}

	}
}

// Copy the incoming dotData into a subframe.
void procFillDMDSubFrame(int frameIndex, UINT8 *dotData, int length)
{
	memcpy(procdmd[frameIndex], dotData, length);
}

void procReverseSubFrameBytes(int frameIndex) {
	int i;
	uint8_t byte, new_byte;
	for (i=0; i<0x200; i++) {
		byte = procdmd[frameIndex][i];
		new_byte = ((byte & 0x1) << 7) |
		           ((byte & 0x2) << 5) |
		           ((byte & 0x4) << 3) |
		           ((byte & 0x8) << 1) |
		           ((byte & 0x10) >> 1) |
		           ((byte & 0x20) >> 3) |
		           ((byte & 0x40) >> 5) |
		           ((byte & 0x80) >> 7);
		procdmd[frameIndex][i] = new_byte;
	}
}

// Turn on the requested alphanumeric segment.
void procDrawSegment(int x, int y, int seg) {
    
    // If y is 11, then this is the ball/credit display.
    // If so, quit the call if we don't want to put on DMD.
    if (y==11) {
        if ( (!S11CreditPos && !S11BallPos) ||
             (!S11CreditPos && x<=12) ||
             (!S11BallPos && x>12)
                ) {return;}
    }

    // If we're not showing the credit or ball displays, and it's not 2 lines of alphanumeric,
    // shift over 4 dots to centre things nicely
    if (!S11CreditPos && !S11BallPos && !doubleAlpha && (core_gameData->gen & GEN_ALLS11)) {
        x+=4;
    }
    if (y==11) {
        // This is the credit/ball display from an early Sys11
        // So need to reposition, then draw small digits
        if (x==6) {x=121;y=S11CreditPos-1;}
        else if (x==12) {x=125;y=S11CreditPos-1;}
        else if (x==24) {x=121;y=S11BallPos-1;}
        else if (x==30) {x=125;y=S11BallPos-1;}
        switch (seg) {
		case 0:
			procDrawDot(x, y, 0xf);
			procDrawDot(x+1, y, 0xf);
			procDrawDot(x+2, y, 0xf);
			break;
		case 1:
			procDrawDot(x+2, y, 0xf);
			procDrawDot(x+2, y+1, 0xf);
			procDrawDot(x+2, y+2, 0xf);
			break;
		case 2:
			procDrawDot(x+2, y+2, 0xf);
			procDrawDot(x+2, y+3, 0xf);
			procDrawDot(x+2, y+4, 0xf);
			break;
		case 3:
			procDrawDot(x, y+4, 0xf);
			procDrawDot(x+1, y+4, 0xf);
			procDrawDot(x+2, y+4, 0xf);
			break;
		case 4:
			procDrawDot(x, y+2, 0xf);
			procDrawDot(x, y+3, 0xf);
			procDrawDot(x, y+4, 0xf);
			break;
		case 5:
			procDrawDot(x, y, 0xf);
			procDrawDot(x, y+1, 0xf);
			procDrawDot(x, y+2, 0xf);
			break;
		case 6:
                        
			procDrawDot(x, y+2, 0xf);
                        procDrawDot(x+1, y+2, 0xf);
                        procDrawDot(x+2, y+2, 0xf);
                            
			break;
        }
    } else {
    // Regular segments
	switch (seg) {
		case 0:
			procDrawDot(x, y, 0x5);
			procDrawDot(x+1, y, 0xf);
			procDrawDot(x+2, y, 0xf);
			procDrawDot(x+3, y, 0xf);
			procDrawDot(x+4, y, 0xf);
			procDrawDot(x+5, y, 0xf);
			procDrawDot(x+6, y, 0x5);
			break;
		case 1:
			procDrawDot(x+6, y, 0x5);
			procDrawDot(x+6, y+1, 0xf);
			procDrawDot(x+6, y+2, 0xf);
			procDrawDot(x+6, y+3, 0xf);
			procDrawDot(x+6, y+4, 0xf);
			procDrawDot(x+6, y+5, 0x5);
			break;
		case 2:
			procDrawDot(x+6, y+5, 0x5);
			procDrawDot(x+6, y+6, 0xf);
			procDrawDot(x+6, y+7, 0xf);
			procDrawDot(x+6, y+8, 0xf);
			procDrawDot(x+6, y+9, 0xf);
			procDrawDot(x+6, y+10, 0x5);
			break;
		case 3:
			procDrawDot(x, y+10, 0x5);
			procDrawDot(x+1, y+10, 0xf);
			procDrawDot(x+2, y+10, 0xf);
			procDrawDot(x+3, y+10, 0xf);
			procDrawDot(x+4, y+10, 0xf);
			procDrawDot(x+5, y+10, 0xf);
			procDrawDot(x+6, y+10, 0x5);
			break;
		case 4:
			procDrawDot(x, y+5, 0x5);
			procDrawDot(x, y+6, 0xf);
			procDrawDot(x, y+7, 0xf);
			procDrawDot(x, y+8, 0xf);
			procDrawDot(x, y+9, 0xf);
			procDrawDot(x, y+10, 0x5);
			break;
		case 5:
			procDrawDot(x, y, 0x5);
			procDrawDot(x, y+1, 0xf);
			procDrawDot(x, y+2, 0xf);
			procDrawDot(x, y+3, 0xf);
			procDrawDot(x, y+4, 0xf);
			procDrawDot(x, y+5, 0x5);
			break;
		case 6:
                        // Segment 6 is different on the bottom line of
                        // a system 11 display as it's numeric, not alpha
			if ((core_gameData->gen & GEN_ALLS11) && (y==19)  && !doubleAlpha) {
                            procDrawDot(x, y+5, 0x5);
                            procDrawDot(x+1, y+5, 0xf);
                            procDrawDot(x+2, y+5, 0xf);
                            procDrawDot(x+3, y+5, 0xf);
                            procDrawDot(x+4, y+5, 0xf);
                            procDrawDot(x+5, y+5, 0xf);
                            procDrawDot(x+6, y+5, 0x5);
                        } else {
                            procDrawDot(x, y+5, 0x5);
                            procDrawDot(x+1, y+5, 0xf);
                            procDrawDot(x+2, y+5, 0xf);
                            procDrawDot(x+3, y+5, 0x5);
                        }
			break;
		case 7:
			procDrawDot(x+7, y+10, 0xf);
			break;
		case 8:
			procDrawDot(x+0, y+0, 0x5);
			procDrawDot(x+1, y+1, 0x8);
			procDrawDot(x+1, y+2, 0x8);
			procDrawDot(x+2, y+2, 0x3);
			procDrawDot(x+1, y+3, 0x3);
			procDrawDot(x+2, y+3, 0x8);
			procDrawDot(x+2, y+4, 0x8);
			procDrawDot(x+3, y+5, 0x5);
			break;
		case 9:
			procDrawDot(x+3, y+1, 0xf);
			procDrawDot(x+3, y+2, 0xf);
			procDrawDot(x+3, y+3, 0xf);
			procDrawDot(x+3, y+4, 0xf);
			procDrawDot(x+3, y+5, 0x5);
			break;
		case 10:
			procDrawDot(x+6, y+0, 0x5);
			procDrawDot(x+5, y+1, 0x8);
			procDrawDot(x+5, y+2, 0x8);
			procDrawDot(x+4, y+2, 0x3);
			procDrawDot(x+4, y+3, 0x8);
			procDrawDot(x+5, y+3, 0x3);
			procDrawDot(x+4, y+4, 0x8);
			procDrawDot(x+3, y+5, 0x5);
			break;
		case 11:
			procDrawDot(x+3, y+5, 0x5);
			procDrawDot(x+4, y+5, 0xf);
			procDrawDot(x+5, y+5, 0xf);
			procDrawDot(x+6, y+5, 0x5);
			break;
		case 12:
			procDrawDot(x+6, y+10, 0x5);
			procDrawDot(x+5, y+9, 0x8);
			procDrawDot(x+5, y+8, 0x8);
			procDrawDot(x+4, y+8, 0x3);
			procDrawDot(x+4, y+7, 0x8);
			procDrawDot(x+5, y+7, 0x3);
			procDrawDot(x+4, y+6, 0x8);
			procDrawDot(x+3, y+5, 0x5);
			break;
		case 13:
			procDrawDot(x+3, y+5, 0x5);
			procDrawDot(x+3, y+6, 0xf);
			procDrawDot(x+3, y+7, 0xf);
			procDrawDot(x+3, y+8, 0xf);
			procDrawDot(x+3, y+9, 0xf);
			break;
		case 14:
			procDrawDot(x+0, y+10, 0x5);
			procDrawDot(x+1, y+9, 0x8);
			procDrawDot(x+1, y+8, 0x8);
			procDrawDot(x+2, y+8, 0x3);
			procDrawDot(x+2, y+7, 0x8);
			procDrawDot(x+1, y+7, 0x3);
			procDrawDot(x+2, y+6, 0x8);
			procDrawDot(x+3, y+5, 0x5);
			break;
		case 15:
			procDrawDot(x+7, y+9, 0xf);
			procDrawDot(x+6, y+11, 0xf);
			procDrawDot(x+7, y+11, 0x5);
			break;
	}
    }
}

// Send the current DMD Frame to the P-ROC
void procUpdateDMD(void) {
	if (proc) {
		PRDMDDraw(proc, procdmd[0]);
	}
}

void procDisplayText(char * string_1, char * string_2)
{
	// Start at ASCII table offset 32: ' '
	const UINT16 asciiSegments[] = {0x0000,	// ' '
	                                0x0000,	// '!'
	                                0x0000,	// '"'
	                                0x0000,	// '#'
	                                0x0000,	// '$'
	                                0x0000,	// '%'
	                                0x0000,	// '&'
	                                0x0200,	// '''
	                                0x1400,	// '('
	                                0x4100,	// ')'
	                                0x7f40,	// '*'
	                                0x2a40,	// '+'
	                                0x8080,	// ','
	                                0x0840,	// '-'
	                                0x8000,	// '.'
	                                0x4400,	// '/'

	                                0x003f,	// '0'
	                                0x0006,	// '1'
	                                0x085b,	// '2'
	                                0x084f,	// '3'
	                                0x0866,	// '4'0
	                                0x086d,	// '5'
	                                0x087d,	// '6'
	                                0x0007,	// '7'
	                                0x087f,	// '8'
	                                0x086f,	// '9'

	                                0x0000,	// '1'
	                                0x0000,	// '1'
	                                0x0000,	// '1'
	                                0x0000,	// '1'
	                                0x0000,	// '1'
	                                0x0000,	// '1'
	                                0x0000,	// '1'

	                                0x0877,	// 'A'
	                                0x2a4f,	// 'B'
	                                0x0039,	// 'C'
	                                0x220f,	// 'D'
	                                0x0879,	// 'E'
	                                0x0871,	// 'F'
	                                0x083d,	// 'G'
	                                0x0876,	// 'H'
	                                0x2209,	// 'I'
	                                0x001e,	// 'J'
	                                0x1470,	// 'K'
	                                0x0038,	// 'L'
	                                0x0536,	// 'M'
	                                0x1136,	// 'N'
	                                0x003f,	// 'O'
	                                0x0873,	// 'P'
	                                0x103f,	// 'Q'
	                                0x1873,	// 'R'
	                                0x086d,	// 'S'
	                                0x2201,	// 'T'
	                                0x003e,	// 'U'
	                                0x4430,	// 'V'
	                                0x5036,	// 'W'
	                                0x5500,	// 'X'
	                                0x2500,	// 'Y'
	                                0x4409 	// 'Z'
                                	}; 

	int i, j;
	char char_a, char_b;
	UINT16 segs_a[16], segs_b[16];

	if (machineType != kPRMachineWPCAlphanumeric) procClearDMD();

	for (i=0; i<16; i++) {
		char_a = string_1[i];
		char_b = string_2[i];
                segs_a[i] = asciiSegments[char_a - 32];
		segs_b[i] = asciiSegments[char_b - 32]; 

		if (machineType != kPRMachineWPCAlphanumeric) {
			for (j=0; j<16; j++) {
				if ((segs_a[i] >> j) & 0x1) procDrawSegment(i*8, 3, j);
				if ((segs_b[i] >> j) & 0x1) procDrawSegment(i*8, 19, j);
			}
		}
	}

	if (machineType != kPRMachineWPCAlphanumeric) {
		procUpdateDMD();
	} else {
		procUpdateAlphaDisplay(segs_a, segs_b);
	}
}

// Send the new alphanumeric display commands to the P-ROC's Auxiliary port logic,
// which will send them to the alphanumeri displays.
void procUpdateAlphaDisplay(UINT16 *top, UINT16 *bottom) {

    int i, cmd_index=0;
	int segs_a, segs_b;
	const int DIS_STB = 8;
	const int STB_1   = 9;
	const int STB_2   = 10;
	const int STB_3   = 11;
	const int STB_4   = 12;
        if (proc) {
		PRDriverAuxCommand auxCommands[255];

		for (i=0; i<16; i++) {
			// Assert the STB line
			PRDriverAuxPrepareOutput(&(auxCommands[cmd_index++]), i, 0, DIS_STB, 0, 0);

                        // Very short delay, enough to let the select line settle
                        PRDriverAuxPrepareDelay(&auxCommands[cmd_index++], 1);
			segs_a = top[i];  
			segs_b = bottom[i];
                        PRDriverAuxPrepareOutput(&(auxCommands[cmd_index++]), segs_a & 0xff, 0, STB_1, 0, 0);
			PRDriverAuxPrepareOutput(&(auxCommands[cmd_index++]), (segs_a >> 8) & 0xff, 0, STB_2, 0, 0);
			PRDriverAuxPrepareOutput(&(auxCommands[cmd_index++]), segs_b & 0xff, 0, STB_3, 0, 0);
			PRDriverAuxPrepareOutput(&(auxCommands[cmd_index++]), (segs_b >> 8) & 0xff, 0, STB_4, 0, 0);

			PRDriverAuxPrepareDelay(&auxCommands[cmd_index++], 350);

			PRDriverAuxPrepareOutput(&(auxCommands[cmd_index++]), 0, 0, STB_1, 0, 0);
			PRDriverAuxPrepareOutput(&(auxCommands[cmd_index++]), 0, 0, STB_2, 0, 0);
			PRDriverAuxPrepareOutput(&(auxCommands[cmd_index++]), 0, 0, STB_3, 0, 0);
			PRDriverAuxPrepareOutput(&(auxCommands[cmd_index++]), 0, 0, STB_4, 0, 0);

			PRDriverAuxPrepareDelay(&auxCommands[cmd_index++], 40);
		}

		PRDriverAuxPrepareJump(&auxCommands[cmd_index++], 0);

		// Send the commands.
		PRDriverAuxSendCommands(proc, auxCommands, cmd_index, 0);

                }
}

#endif /* PINMAME && PROC_SUPPORT */
