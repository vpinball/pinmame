// license:BSD-3-Clause

#pragma once

#include "input.h"

/* Scripted keyboard input, for headless and reproducible runs.  Enabled by
   -key_script <file>; with no option every function here is a no-op.

   Script syntax, one event per line, '#' comments and blank lines ignored,
   and applied in frame order regardless of file order (same-frame events
   keep file order):
     <frame>  down <KEY>[,<KEY>...]      hold these keys from this frame on
     <frame>  up   <KEY>[,<KEY>...]      release them
     <frame>  tap  <n> <KEY>[,<KEY>...]  hold for n frames from this frame
     <frame>  mark <text>                append "<frame> <ms> <text>" to <file>.marks
     <frame>  quit                       end the run (holds KEYCODE_ESC; backs out
                                          of a MAME UI menu instead, if one is open)
   <KEY> is a MAME input token, e.g. KEYCODE_LSHIFT, resolved with
   seq_set_string, so every name the cfg files accept works here.

   Do not script a MAME UI hotkey with `down` (never released): KEYCODE_P
   (IPT_UI_PAUSE) hangs a headless run indefinitely, and TAB/F3/F4/F6/F8-F12/
   SCRLOCK/backtick are the same class of hazard. */

/* Called once per frame from update_input_ports() (src/inptport.c, inside
   load_input_port_settings), which runs ahead of the emulated frame count --
   so ks.frame is always one frame ahead of core_gameData's own frame number.
   Deterministic and self-consistent with every committed script; do not
   "fix" the offset without re-cutting every script's frame numbers */
void keyscript_tick(void);              /* once per frame, from update_input_ports */
int  keyscript_pressed(InputCode code); /* is this code held by the script? */
