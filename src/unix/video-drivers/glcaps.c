/**
 * glcaps.c
 *
 * Copyright (C) 2001  Sven Goethel
 *
 * GNU Library General Public License 
 * as published by the Free Software Foundation
 *
 * http://www.gnu.org/copyleft/lgpl.html
 * General dynamical loading OpenGL (GL/GLU) support for:
 */

#include "gltool.h"

void LIBAPIENTRY printGLCapabilities ( GLCapabilities *glCaps )
{
    fprintf(stdout, "\t gl_supported: %d !\n", glCaps->gl_supported);
    fprintf(stdout, "\t doubleBuff: %d, ", (int)glCaps->buffer);
    fprintf(stdout, " rgba: %d, ", (int)glCaps->color);
    fprintf(stdout, " stereo: %d, ", (int)glCaps->stereo);
    fprintf(stdout, " depthSize: %d, ", (int)glCaps->depthBits);
    fprintf(stdout, " stencilSize: %d !\n", (int)glCaps->stencilBits);
    fprintf(stdout, "\t red: %d, ", (int)glCaps->redBits);
    fprintf(stdout, " green: %d, ", (int)glCaps->greenBits);
    fprintf(stdout, " blue: %d, ", (int)glCaps->blueBits);
    fprintf(stdout, " alpha: %d !\n", (int)glCaps->alphaBits);
    fprintf(stdout, "\t red accum: %d, ", (int)glCaps->accumRedBits);
    fprintf(stdout, " green accum: %d, ", (int)glCaps->accumGreenBits);
    fprintf(stdout, " blue accum: %d, ", (int)glCaps->accumBlueBits);
    fprintf(stdout, " alpha accum: %d !\n", (int)glCaps->accumAlphaBits);
    fprintf(stdout, "\t nativeVisualID: %ld !\n", (long)glCaps->nativeVisualID);

    fflush(stdout);
}

