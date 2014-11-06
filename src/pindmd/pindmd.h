#ifndef INC_USBDMD
#define INC_USBDMD

extern void pindmdInit(void);
extern void pindmdDeInit(void);
extern void sendLogo(void);
extern void renderDMDFrame(UINT64 gen, UINT32 width, UINT32 height, UINT8 *currbuffer_in, UINT8 doDumpFrame);
extern void renderAlphanumericFrame(UINT64 gen, UINT16 *seg_data, UINT8 total_disp, UINT8 *disp_lens);
extern void drawPixel(int x, int y, UINT8 colour);
extern UINT8 getPixel(int x, int y);
extern void dumpFrames(UINT8 *currbuffer, UINT16 buffersize, UINT8 doDumpFrame, UINT64 gen);
extern void frameClock(void);

UINT8 frame_buf[3072];

#endif
