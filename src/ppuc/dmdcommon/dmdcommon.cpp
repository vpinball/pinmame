#include "dmdcommon.h"

#pragma warning(disable: 4996)

DMD_COMMON(void) DmdCommon_ConvertFrameToPlanes(UINT32 width, UINT32 height, UINT8* frame, UINT8* planes, int bitDepth)
{
    UINT8 bitMask = 1;
    UINT32 tj = 0;
    const UINT32 frameSize = height * width;
    const UINT32 planeOffset = frameSize / 8;

    for (UINT8 tk = 0; tk < bitDepth; tk++) planes[tk * planeOffset + tj] = 0;

    for (UINT32 ti = 0; ti < frameSize; ti++)
    {
        UINT8 tl = 1;
        for (UINT8 tk = 0; tk < bitDepth; tk++)
        {
            if ((frame[ti] & tl) > 0)
            {
                planes[tk * planeOffset + tj] |= bitMask;
            }
            tl <<= 1;
        }

        if (bitMask == 0x80)
        {
            bitMask = 1;
            tj++;
            if (tj < planeOffset)
            {
                for (UINT8 tk = 0; tk < bitDepth; tk++)
                {
                    planes[tk * planeOffset + tj] = 0;
                }
            }
        }
        else bitMask <<= 1;
    }
}
