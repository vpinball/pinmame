#ifdef __cplusplus
extern "C"
{
#endif

void ResampleX1R5G5B5(unsigned short *dst, int dx, int dy, unsigned short *src, int sx, int sy);
void ResampleR5G6B5(unsigned short *dst, int dx, int dy, unsigned short *src, int sx, int sy);
void ResampleX8R8G8B8(unsigned int *dst, int dx, int dy, unsigned int *src, int sx, int sy);
void ResampleR8G8B8(unsigned char *dst, int dx, int dy, unsigned char *src, int sx, int sy);

int BasicBitmap_SSE2_AVX_Enable();

#ifdef __cplusplus
}
#endif
