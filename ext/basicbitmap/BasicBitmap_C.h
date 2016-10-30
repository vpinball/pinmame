#ifdef __cplusplus
extern "C"
{
#endif

void ResampleA1R5G5B5(unsigned short *dst, int dx, int dy, unsigned short *src, int sx, int sy);
int BasicBitmap_SSE2_AVX_Enable();

#ifdef __cplusplus
}
#endif
