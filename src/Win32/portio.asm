;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
; Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
;    
; This file is part of MAME32, and may only be used, modified and
; distributed under the terms of the MAME license, in "readme.txt".
; By continuing to use, modify or distribute this file you indicate
; that you have read the license and understand and accept it fully.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; portio.asm
;
; Port input/output for Windows 9x
;
; for MSVC, use 'COFF' format:
;    nasmw -f win32 portio.asm
;
; for BCC32, use 'OMF' format:
;    nasmw -f obj portio.asm
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

BITS 32
SECTION .text USE32

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; int _outp(unsigned short port, int databyte);
;
; uses eax, edx
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

GLOBAL __outp
__outp:
        xor     eax, eax       ; clear result
        mov      dx, [esp + 4] ; port
        mov      al, [esp + 8] ; databyte
        out      dx, al        ; output data
        ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; int _inp(unsigned short port);
;
; uses eax, edx
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

GLOBAL __inp
__inp:
        xor     eax, eax       ; clear result
        mov      dx, [esp + 4] ; port
        in       al, dx        ; input data
        ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
