;
; Blitting routines for Mame, using MMX for stretching
; January/February 2000
; Bernd Wiebelt <bernardo@mame.net>

%assign SCANLINE_BRIGHTNESS 0			; this can be 0%, 50% or 75% brightness

[BITS 32]

extern _palette_16bit_lookup			; for 16 bit palettized mode
										; assuming entries are _doubled_
										; to their respective 32 bit values

[SECTION .data]
halfbright15_mask:	dd 0x3def3def		; mask for halfbright (4-4-4)
					dd 0x3def3def
halfbright16_mask:	dd 0x7bef7bef		; mask for halfbright (4-5-4)
					dd 0x7bef7bef


[SECTION .text]

; Here comes the generic template for blitting
; Note that the function names which are used by C are
; created on the fly by macro expansion. Cool feature.


; ******************************************
; function prototype
; void asmblit_Mx_Ny_Bsl_Xbpp[_palettized] (
; 			int width, int height,
;			void* src, int src_offset,
;			void* dst, int dst_offset)
; ******************************************

; list of parameters
width		equ 8						; number of bitmap rows to copy
height		equ 12						; number of bitmap dwords to copy
src			equ 16						; source (memory bitmap)
src_offset	equ 20						; offset to next bitmap row
dst			equ 24						; destination (LINEAR framebuffer)
dst_offset	equ 28						; offset to next framebuffer row


; *************************************************
; macro header for creation of custom blit function
; *************************************************
%macro blit_template 4-5
	GLOBAL _asmblit_%1x_%2y_%3sl_%4bpp%5
	_asmblit_%1x_%2y_%3sl_%4bpp%5:

%assign MX  %1						; X-Stretch factor
%assign MY  %2						; Y-Stretch factor
%assign SL  %3						; emulate scanlines
%assign BPP %4						; bits per pixel (8/16/32 bpp)

; decide wether palettized blit by the number of parameters passed

%if (%0 == 4)
	%assign PALETTIZED 0
%else
	%assign PALETTIZED 1
%endif

; adjust parameters for 0, 50 or 75% scanlines

%if ((BPP == 16) && (SL == 1) && (SCANLINE_BRIGHTNESS > 0))
	%assign SL 0
	%assign REDUCED_SCANLINES 1
%else
	%assign REDUCED_SCANLINES 0
%endif

; ***************************
; generic function call intro
; ***************************

push ebp
mov ebp, esp

; **************************
; blit in chunks of 4 dwords
; **************************

add [ebp + width], DWORD 3
shr	DWORD [ebp + width], 2


%%next_line:

mov esi, [ebp + src]
mov edi, [ebp + dst]
mov ecx, [ebp + width]

align 16								; align the loop

%%next_linesegment:

; ********************************
; load and process backbuffer data
; ********************************

%if (PALETTIZED == 1)					; stretch pixels palettized

	mov eax, 0
	mov ebx, 0

	mov edx, [_palette_16bit_lookup]

	mov ax, [esi + 0]
	mov bx, [esi + 2]
	movd mm0, [edx+eax*4]
	movd mm1, [edx+ebx*4]

	mov ax, [esi + 4]
	mov bx, [esi + 6]
	movd mm2, [edx+eax*4]
	movd mm3, [edx+ebx*4]

	mov ax, [esi + 8]
	mov bx, [esi + 10]
	movd mm4, [edx+eax*4]
	movd mm5, [edx+ebx*4]

	mov ax, [esi + 12]
	mov bx, [esi + 14]
	movd mm6, [edx+eax*4]
	movd mm7, [edx+ebx*4]

	%if (MX == 1)						; now rearrange the stuff
		punpcklwd mm0, mm1				; add words to dwords
		punpcklwd mm2, mm3
		punpcklwd mm4, mm5
		punpcklwd mm6, mm7

		punpckldq mm0, mm2				; add dwords to quadwords
		punpckldq mm4, mm6
	%elif (MX == 2)
		punpckldq mm0, mm1
		punpckldq mm2, mm3
		punpckldq mm4, mm5
		punpckldq mm6, mm7
	%endif
%endif ; (PALETTIZED == 1)


%if (PALETTIZED == 0)					; stretch pixel non-palettized

	movq mm0, [esi+0] 					; always read 16 bytes at once
	movq mm4, [esi+8]

	%if (MX >= 2)
		movq mm2, mm0
		movq mm6, mm4
		%if (BPP == 8)
			punpcklbw mm0, mm0
			punpckhbw mm2, mm2
			punpcklbw mm4, mm4
			punpckhbw mm6, mm6
		%elif (BPP = 16)
			punpcklwd mm0, mm0
			punpckhwd mm2, mm2
			punpcklwd mm4, mm4
			punpckhwd mm6, mm6
		%elif (BPP = 32)
			punpckldq mm0, mm0
			punpckhdq mm2, mm2
			punpckldq mm4, mm4
			punpckhdq mm6, mm6
		%endif
	%endif
%endif

; *******************
; copy to framebuffer
; *******************

%if (MY > 1)
	mov edx, [ebp + dst_offset] 			; only if y-stretching
%endif

%if (MX == 1)
	movq [fs:edi+0], mm0					; simply store the stuff back
	movq [fs:edi+8], mm4
	%if ((MY >= 2) && (SL = 0))
		%if ((REDUCED_SCANLINES == 1) && (SCANLINE_BRIGHTNESS >= 50 ))
			psrlq mm0, 1
			psrlq mm4, 1
			pand mm0, [halfbright16_mask]
			pand mm4, [halfbright16_mask]
			%if (SCANLINE_BRIGHTNESS >= 75)
				movq mm1, mm0
				movq mm5, mm4
				psrlq mm1, 1
				psrlq mm5, 1
				pand mm1, [halfbright16_mask]
				pand mm5, [halfbright16_mask]
				paddw mm0, mm1
				paddw mm4, mm5
			%endif
		%endif
		movq [fs:edi+edx], mm0
		movq [fs:edi+edx+8], mm4
	%endif
%endif

%if (MX >= 2)
	movq [fs:edi], mm0
	movq [fs:edi+8], mm2
	movq [fs:edi+16], mm4
	movq [fs:edi+24], mm6
	%if ((MY >= 2) && (SL == 0))
		%if ((REDUCED_SCANLINES == 1) && (SCANLINE_BRIGHTNESS >= 50))
			psrlq mm0, 1
			psrlq mm2, 1
			psrlq mm4, 1
			psrlq mm6, 1
			pand mm0, [halfbright16_mask]
			pand mm2, [halfbright16_mask]
			pand mm4, [halfbright16_mask]
			pand mm6, [halfbright16_mask]
			%if (SCANLINE_BRIGHTNESS >= 75)
				movq mm1, mm0
				movq mm3, mm2
				movq mm5, mm4
				movq mm7, mm6
				psrlq mm1, 1
				psrlq mm3, 1
				psrlq mm5, 1
				psrlq mm7, 1
				pand mm1, [halfbright16_mask]
				pand mm3, [halfbright16_mask]
				pand mm5, [halfbright16_mask]
				pand mm7, [halfbright16_mask]
				paddw mm0, mm1
				paddw mm2, mm3
				paddw mm4, mm5
				paddw mm6, mm7
			%endif
		%endif
		movq [fs:edi+edx], mm0
		movq [fs:edi+edx+8], mm2
		movq [fs:edi+edx+16], mm4
		movq [fs:edi+edx+24], mm6
	%endif
%endif 									; end case (MX == 2)

; ********************
; next iteration
; ********************


	lea esi, [esi+16]					; next 16 src bytes
	lea edi, [edi+(16*MX)]				; next 16*MX dst bytes

	dec ecx
	jnz NEAR %%next_linesegment			; row done?

	mov eax, [ebp+src_offset]			; next src row
	add [ebp+src], eax

	mov eax, [ebp+dst_offset]			; next dst row
	imul eax, MY
	add [ebp+dst], eax

	dec DWORD [ebp+height]				;
	jnz NEAR %%next_line				; all rows done?

	mov esp, ebp						; yes, restore ebp
	pop ebp
	emms								; mmx cleanup [necessary?]
	ret


%endmacro

;
; helper macro for creating the blitting functions
; argument 1: bpp, argument 2: palettized/empty
;
%macro create_stretch 1-2
	%assign i 1
	%rep 4
		%assign j 1
		%rep 4
			blit_template i,j,0,%1,%2	; noscanline
			blit_template i,j,1,%1,%2	; scanline
			%assign j j+1
		%endrep
	%assign i i+1
	%endrep
%endmacro


;
; create all blitting routines now
; this is all too easy...
;
create_stretch 8
create_stretch 16
create_stretch 16,_palettized
create_stretch 32
