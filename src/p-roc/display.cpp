#include "driver.h"
#include "p-roc.hpp"
extern "C" {
#include <wpc/core.h>
}

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
	dmdConfig.enableFrameEvents = FALSE;

	for (i = 0; i < dmdConfig.numSubFrames; i++) {
		dmdConfig.rclkLowCycles[i] = 15;
		dmdConfig.latchHighCycles[i] = 15;
		dmdConfig.dotclkHalfPeriod[i] = 1;
	}

	dmdConfig.deHighCycles[0] = 90;
	dmdConfig.deHighCycles[1] = 190;
	dmdConfig.deHighCycles[2] = 50;
	dmdConfig.deHighCycles[3] = 377;

	PRDMDUpdateConfig(coreGlobals.proc, &dmdConfig);

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
	uint8_t temp_dot;
	uint8_t position;

	int i;
	for (i = 0; i < 4; i++) {
		if ((color >> i) & 0x1) {
			temp_dot = procdmd[i][(((128*y)+x))/8];
			position = ((128*y)+x)%8;
			temp_dot = temp_dot | (1<<position);
		}

		procdmd[i][(((128*y)+x))/8] = temp_dot;
	}
}

// Copy the incoming dotData into a subframe.
void procFillDMDSubFrame(int frameIndex, UINT8 *dotData, int length)
{
	memcpy(procdmd[frameIndex], dotData, length);
}

// Turn on the requested alphanumeric segment.
void procDrawSegment(int x, int y, int seg) {
	switch (seg) {
	 case 0:
		procDrawDot(x+1, y, 0xf);
		procDrawDot(x+2, y, 0xf);
		procDrawDot(x+3, y, 0xf);
		procDrawDot(x+4, y, 0xf);
		procDrawDot(x+5, y, 0xf);
		break;
	 case 1:
		procDrawDot(x+6, y+1, 0xf);
		procDrawDot(x+6, y+2, 0xf);
		procDrawDot(x+6, y+3, 0xf);
		procDrawDot(x+6, y+4, 0xf);
		break;
	 case 2:
		procDrawDot(x+6, y+6, 0xf);
		procDrawDot(x+6, y+7, 0xf);
		procDrawDot(x+6, y+8, 0xf);
		procDrawDot(x+6, y+9, 0xf);
		break;
	 case 3:
		procDrawDot(x+1, y+10, 0xf);
		procDrawDot(x+2, y+10, 0xf);
		procDrawDot(x+3, y+10, 0xf);
		procDrawDot(x+4, y+10, 0xf);
		procDrawDot(x+5, y+10, 0xf);
		break;
	 case 4:
		procDrawDot(x, y+6, 0xf);
		procDrawDot(x, y+7, 0xf);
		procDrawDot(x, y+8, 0xf);
		procDrawDot(x, y+9, 0xf);
		break;
	 case 5:
		procDrawDot(x, y+1, 0xf);
		procDrawDot(x, y+2, 0xf);
		procDrawDot(x, y+3, 0xf);
		procDrawDot(x, y+4, 0xf);
		break;
	 case 6:
		procDrawDot(x+1, y+5, 0xf);
		procDrawDot(x+2, y+5, 0xf);
		procDrawDot(x+3, y+5, 0xf);
		break;
	 case 7:
		procDrawDot(x+7, y+10, 0xf);
		break;
	 case 8:
		procDrawDot(x+0, y+0, 0xf);
		procDrawDot(x+1, y+1, 0xf);
		procDrawDot(x+1, y+2, 0xf);
		procDrawDot(x+2, y+3, 0xf);
		procDrawDot(x+2, y+4, 0xf);
		procDrawDot(x+3, y+5, 0xf);
		break;
	 case 9:
		procDrawDot(x+3, y+1, 0xf);
		procDrawDot(x+3, y+2, 0xf);
		procDrawDot(x+3, y+3, 0xf);
		procDrawDot(x+3, y+4, 0xf);
		break;
	 case 10:
		procDrawDot(x+6, y+0, 0xf);
		procDrawDot(x+5, y+1, 0xf);
		procDrawDot(x+5, y+2, 0xf);
		procDrawDot(x+4, y+3, 0xf);
		procDrawDot(x+4, y+4, 0xf);
		procDrawDot(x+3, y+5, 0xf);
		break;
	 case 11:
		procDrawDot(x+3, y+5, 0xf);
		procDrawDot(x+4, y+5, 0xf);
		procDrawDot(x+5, y+5, 0xf);
		break;
	 case 12:
		procDrawDot(x+6, y+10, 0xf);
		procDrawDot(x+5, y+9, 0xf);
		procDrawDot(x+5, y+8, 0xf);
		procDrawDot(x+4, y+7, 0xf);
		procDrawDot(x+4, y+6, 0xf);
		procDrawDot(x+3, y+5, 0xf);
		break;
	 case 13:
		procDrawDot(x+3, y+6, 0xf);
		procDrawDot(x+3, y+7, 0xf);
		procDrawDot(x+3, y+8, 0xf);
		procDrawDot(x+3, y+9, 0xf);
		break;
	 case 14:
		procDrawDot(x+0, y+10, 0xf);
		procDrawDot(x+1, y+9, 0xf);
		procDrawDot(x+1, y+8, 0xf);
		procDrawDot(x+2, y+7, 0xf);
		procDrawDot(x+2, y+6, 0xf);
		procDrawDot(x+3, y+5, 0xf);
		break;
	 case 15:
		procDrawDot(x+7, y+9, 0xf);
		procDrawDot(x+6, y+11, 0xf);
		break;
	}
}

// Send the current DMD Frame to the P-ROC
void procUpdateDMD(void) {
	PRDMDDraw(coreGlobals.proc, procdmd[0]);
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

	PRDriverAuxCommand auxCommands[256];

	// Disable the first entry so the Aux logic won't begin immediately.
	PRDriverAuxPrepareDisable(&auxCommands[cmd_index++]);

	for (i=0; i<16; i++) {
		// Assert the STB line
		PRDriverAuxPrepareOutput(&(auxCommands[cmd_index++]), i, 0, DIS_STB, 0, 0);

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

	PRDriverAuxPrepareJump(&auxCommands[cmd_index++], 1);

	// Send the commands.
	PRDriverAuxSendCommands(coreGlobals.proc, auxCommands, cmd_index, 0);

	cmd_index = 0;
	// Jump from addr 0 to 1 to begin.
	PRDriverAuxPrepareJump(&auxCommands[cmd_index++],1);
	PRDriverAuxSendCommands(coreGlobals.proc, auxCommands, cmd_index, 0);
}
