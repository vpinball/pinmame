/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2000 Michael Soderstrom and Chris Kirmse
    
  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

***************************************************************************/
 
/***************************************************************************

  dilib3.c

  Replacement for c_dfDIJoystick in dinput.lib.

***************************************************************************/

#include <dinput.h>

static DIOBJECTDATAFORMAT c_rgodfDIJoy[] =
{ 
    { &GUID_XAxis,   DIJOFS_X,         0x80000000 | DIDFT_AXIS   | DIDFT_ANYINSTANCE, DIDOI_ASPECTPOSITION, },
    { &GUID_YAxis,   DIJOFS_Y,         0x80000000 | DIDFT_AXIS   | DIDFT_ANYINSTANCE, DIDOI_ASPECTPOSITION, },
    { &GUID_ZAxis,   DIJOFS_Z,         0x80000000 | DIDFT_AXIS   | DIDFT_ANYINSTANCE, DIDOI_ASPECTPOSITION, },
    { &GUID_RxAxis,  DIJOFS_RX,        0x80000000 | DIDFT_AXIS   | DIDFT_ANYINSTANCE, DIDOI_ASPECTPOSITION, },
    { &GUID_RyAxis,  DIJOFS_RY,        0x80000000 | DIDFT_AXIS   | DIDFT_ANYINSTANCE, DIDOI_ASPECTPOSITION, },
    { &GUID_RzAxis,  DIJOFS_RZ,        0x80000000 | DIDFT_AXIS   | DIDFT_ANYINSTANCE, DIDOI_ASPECTPOSITION, },
    { &GUID_Slider,  DIJOFS_SLIDER(0), 0x80000000 | DIDFT_AXIS   | DIDFT_ANYINSTANCE, DIDOI_ASPECTPOSITION, },
    { &GUID_Slider,  DIJOFS_SLIDER(1), 0x80000000 | DIDFT_AXIS   | DIDFT_ANYINSTANCE, DIDOI_ASPECTPOSITION, },
    { &GUID_POV,     DIJOFS_POV(0),    0x80000000 | DIDFT_POV    | DIDFT_ANYINSTANCE, 0,                    },
    { &GUID_POV,     DIJOFS_POV(1),    0x80000000 | DIDFT_POV    | DIDFT_ANYINSTANCE, 0,                    },
    { &GUID_POV,     DIJOFS_POV(2),    0x80000000 | DIDFT_POV    | DIDFT_ANYINSTANCE, 0,                    },
    { &GUID_POV,     DIJOFS_POV(3),    0x80000000 | DIDFT_POV    | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON0,   0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON1,   0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON2,   0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON3,   0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON4,   0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON5,   0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON6,   0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON7,   0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON8,   0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON9,   0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON10,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON11,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON12,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON13,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON14,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON15,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON16,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON17,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON18,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON19,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON20,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON21,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON22,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON23,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON24,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON25,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON26,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON27,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON28,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON29,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON30,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    },
    { NULL,          DIJOFS_BUTTON31,  0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,                    }
};
 
const DIDATAFORMAT c_dfDIJoystick =
{ 
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_ABSAXIS,
    sizeof(DIJOYSTATE), 
    (sizeof(c_rgodfDIJoy) / sizeof(c_rgodfDIJoy[0])),
    c_rgodfDIJoy,
};
