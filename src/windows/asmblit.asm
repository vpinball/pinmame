[BITS 32]


;//============================================================
;//	IMPORTS
;//============================================================

extern _asmblit_srcdata
extern _asmblit_srcheight
extern _asmblit_srclookup

extern _asmblit_dstdata
extern _asmblit_dstpitch

extern _asmblit_dirtydata

extern _asmblit_mmxmask1;
extern _asmblit_mmxmask2;

extern _asmblit_rgbmask;


;//============================================================
;//	LOCAL VARIABLES
;//============================================================

[SECTION .data]
dirty_count:	db	0
last_row:		dd	0
row_index:		dd	0



;//============================================================
;//	CONSTANTS
;//============================================================

%define TAG_FIXUPADDRESS		0
%define TAG_FIXUPVALUE			1
%define TAG_SHIFT32TO16			2
%define TAG_REGOFFSET			3
%define TAG_REGOFFSET_MMX		4
%define TAG_ENDSNIPPET			15

%define REGISTER_eax			0
%define REGISTER_ecx			1
%define REGISTER_edx			2
%define REGISTER_ebx			3
%define REGISTER_ax				4
%define REGISTER_cx				5
%define REGISTER_dx				6
%define REGISTER_bx				7
%define REGISTER_al				8
%define REGISTER_cl				9
%define REGISTER_dl				10
%define REGISTER_bl				11
%define REGISTER_ah				12
%define REGISTER_ch				13
%define REGISTER_dh				14
%define REGISTER_bh				15

%define REGISTER_mm0			0
%define REGISTER_mm1			1
%define REGISTER_mm2			2
%define REGISTER_mm3			3
%define REGISTER_mm4			4
%define REGISTER_mm5			5
%define REGISTER_mm6			6
%define REGISTER_mm7			7
%define REGISTER_xmm0			8
%define REGISTER_xmm1			9
%define REGISTER_xmm2			10
%define REGISTER_xmm3			11
%define REGISTER_xmm4			12
%define REGISTER_xmm5			13
%define REGISTER_xmm6			14
%define REGISTER_xmm7			15

%define SHIFT32TO16TYPE_ebx_r	0
%define SHIFT32TO16TYPE_ebx_g	1
%define SHIFT32TO16TYPE_ebx_b	2
%define SHIFT32TO16TYPE_ecx_r	4
%define SHIFT32TO16TYPE_ecx_g	5
%define SHIFT32TO16TYPE_ecx_b	6

%define FIXUPADDR_YTOP			0
%define FIXUPADDR_MIDDLEXTOP	1
%define FIXUPADDR_MIDDLEXBOTTOM	2
%define FIXUPADDR_LASTXTOP		3
%define FIXUPADDR_YBOTTOM		4

%define FIXUPVAL_SRCBYTES1		0
%define FIXUPVAL_DSTBYTES1		1
%define FIXUPVAL_SRCBYTES16		2
%define FIXUPVAL_DSTBYTES16		3
%define FIXUPVAL_SRCADVANCE		4
%define FIXUPVAL_DSTADVANCE		5
%define FIXUPVAL_DIRTYADVANCE	6
%define FIXUPVAL_MIDDLEXCOUNT	7
%define FIXUPVAL_LASTXCOUNT		8

%define TAG_COMMON(t,x)			(0x00cccccc | ((t)<<28) | ((x)<<24))

%define FIXUPADDRESS(x)			($ + 6 + TAG_COMMON(TAG_FIXUPADDRESS, x))
%define FIXUPVALUE(x)			TAG_COMMON(TAG_FIXUPVALUE, x)
%define ENDSNIPPET				TAG_COMMON(TAG_ENDSNIPPET, 0)



;//============================================================
;//	SNIPPET tagging
;//============================================================

%macro SNIPPET_BEGIN 1
GLOBAL _%1
_%1:
%endmacro

%macro SNIPPET_END 0
	dd		ENDSNIPPET
	dd		0xcccccccc
%endmacro



;//============================================================
;//	REGCOUNT expander
;//============================================================

%macro REGOFFSET 1
	%ifidni %1,eax
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_eax)
	%elifidni %1,ebx
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_ebx)
	%elifidni %1,ecx
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_ecx)
	%elifidni %1,edx
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_edx)
	%elifidni %1,ax
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_ax)
	%elifidni %1,bx
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_bx)
	%elifidni %1,cx
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_cx)
	%elifidni %1,dx
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_dx)
	%elifidni %1,al
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_al)
	%elifidni %1,bl
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_bl)
	%elifidni %1,cl
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_cl)
	%elifidni %1,dl
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_dl)
	%elifidni %1,ah
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_ah)
	%elifidni %1,bh
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_bh)
	%elifidni %1,ch
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_ch)
	%elifidni %1,dh
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_dh)
	%elifidni %1,mm0
		dd		TAG_COMMON(TAG_REGOFFSET_MMX, REGISTER_mm0)
	%elifidni %1,mm1
		dd		TAG_COMMON(TAG_REGOFFSET_MMX, REGISTER_mm1)
	%elifidni %1,mm2
		dd		TAG_COMMON(TAG_REGOFFSET_MMX, REGISTER_mm2)
	%elifidni %1,mm3
		dd		TAG_COMMON(TAG_REGOFFSET_MMX, REGISTER_mm3)
	%elifidni %1,mm4
		dd		TAG_COMMON(TAG_REGOFFSET_MMX, REGISTER_mm4)
	%elifidni %1,mm5
		dd		TAG_COMMON(TAG_REGOFFSET_MMX, REGISTER_mm5)
	%elifidni %1,mm6
		dd		TAG_COMMON(TAG_REGOFFSET_MMX, REGISTER_mm6)
	%elifidni %1,mm7
		dd		TAG_COMMON(TAG_REGOFFSET_MMX, REGISTER_mm7)
	%elifidni %1,xmm0
		dd		TAG_COMMON(TAG_REGOFFSET_MMX, REGISTER_xmm0)
	%elifidni %1,xmm1
		dd		TAG_COMMON(TAG_REGOFFSET_MMX, REGISTER_xmm1)
	%elifidni %1,xmm2
		dd		TAG_COMMON(TAG_REGOFFSET_MMX, REGISTER_xmm2)
	%elifidni %1,xmm3
		dd		TAG_COMMON(TAG_REGOFFSET_MMX, REGISTER_xmm3)
	%elifidni %1,xmm4
		dd		TAG_COMMON(TAG_REGOFFSET_MMX, REGISTER_xmm4)
	%elifidni %1,xmm5
		dd		TAG_COMMON(TAG_REGOFFSET_MMX, REGISTER_xmm5)
	%elifidni %1,xmm6
		dd		TAG_COMMON(TAG_REGOFFSET_MMX, REGISTER_xmm6)
	%elifidni %1,xmm7
		dd		TAG_COMMON(TAG_REGOFFSET_MMX, REGISTER_xmm7)
	%else
		%error Invalid parameter
	%endif
%endmacro



;//============================================================
;//	SHIFT expander
;//============================================================

%macro SHIFT 1
	dd		TAG_COMMON(TAG_SHIFT32TO16, %1)
%endmacro



;//============================================================
;//	Y expander
;//============================================================

%macro store_multiple 2
	REGOFFSET %1
	dd		%2
%endmacro


%macro store_multiple2 4
	REGOFFSET %1
	REGOFFSET %3
	dd		%2
	dd		%4
%endmacro


%macro store_multiple3 6
	REGOFFSET %1
	REGOFFSET %3
	REGOFFSET %5
	dd		%2
	dd		%4
	dd		%6
%endmacro


%macro store_multiple4 8
	REGOFFSET %1
	REGOFFSET %3
	REGOFFSET %5
	REGOFFSET %7
	dd		%2
	dd		%4
	dd		%6
	dd		%8
%endmacro


%macro store_multiple5 10
	REGOFFSET %1
	REGOFFSET %3
	REGOFFSET %5
	REGOFFSET %7
	REGOFFSET %9
	dd		%2
	dd		%4
	dd		%6
	dd		%8
	dd		%10
%endmacro


%macro store_multiple6 12
	REGOFFSET %1
	REGOFFSET %3
	REGOFFSET %5
	REGOFFSET %7
	REGOFFSET %9
	REGOFFSET %11
	dd		%2
	dd		%4
	dd		%6
	dd		%8
	dd		%10
	dd		%12
%endmacro



;//============================================================
;//	8bpp to 8bpp blitters
;//============================================================

SNIPPET_BEGIN asmblit1_8_to_8_x1
	mov		al,[esi]
	store_multiple al,0
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_8_x1
	%assign iter 0
	%rep 2
		mov		eax,[esi+8*iter]
		mov		ebx,[esi+8*iter+4]
		store_multiple2 eax,8*iter,ebx,8*iter+4
		%assign iter iter+1
	%endrep
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_8_x1_mmx
	movq	mm0,[esi]
	movq	mm1,[esi+8]
	store_multiple2 mm0,0,mm1,8
SNIPPET_END


SNIPPET_BEGIN asmblit1_8_to_8_x2
	mov		al,[esi]
	mov		ah,al
	store_multiple ax,0
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_8_x2
	%assign iter 0
	%rep 4
		mov		eax,[esi+4*iter]
		shrd	ebx,eax,8
		shrd	ebx,eax,8
		shr		eax,8
		shrd	ebx,eax,8
		shrd	ebx,eax,8
		shr		eax,8
		store_multiple ebx,8*iter
		shrd	ebx,eax,8
		shrd	ebx,eax,8
		shr		eax,8
		shrd	ebx,eax,8
		shrd	ebx,eax,8
		store_multiple ebx,8*iter+4
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_8_to_8_x3
	mov		al,[esi]
	mov		ah,al
	store_multiple2 ax,0,al,2
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_8_x3
	%assign iter 0
	%rep 4
		mov		eax,[esi+4*iter]
		shrd	ebx,eax,8
		shrd	ebx,eax,8
		shrd	ebx,eax,8
		shr		eax,8
		shrd	ebx,eax,8
		store_multiple ebx,12*iter
		shrd	ebx,eax,8
		shrd	ebx,eax,8
		shr		eax,8
		shrd	ebx,eax,8
		shrd	ebx,eax,8
		store_multiple ebx,12*iter+4
		shrd	ebx,eax,8
		shr		eax,8
		shrd	ebx,eax,8
		shrd	ebx,eax,8
		shrd	ebx,eax,8
		store_multiple ebx,12*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


;//============================================================
;//	8bpp to 16bpp blitters
;//============================================================

SNIPPET_BEGIN asmblit1_8_to_16_x1
	movzx	eax,byte [esi]
	mov		eax,[ecx+eax*4]
	store_multiple ax,0
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_16_x1
	%assign iter 0
	%rep 8
		movzx	eax,byte [esi+2*iter]
		movzx	ebx,byte [esi+2*iter+1]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		shrd	eax,ebx,16
		store_multiple eax,4*iter
		%assign iter iter+1
	%endrep
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_16_x1_mmx
	%assign iter 0
	%rep 2
		movzx	eax,byte [esi+8*iter]
		movzx	ebx,byte [esi+8*iter+1]
		movd	mm0,[ecx+eax*4]
		movd	mm1,[ecx+ebx*4]
		movzx	eax,byte [esi+8*iter+2]
		movzx	ebx,byte [esi+8*iter+3]
		movd	mm2,[ecx+eax*4]
		movd	mm3,[ecx+ebx*4]
		punpcklwd mm0,mm1
		punpcklwd mm2,mm3
		punpckldq mm0,mm2

		movzx	eax,byte [esi+8*iter+4]
		movzx	ebx,byte [esi+8*iter+5]
		movd	mm1,[ecx+eax*4]
		movd	mm2,[ecx+ebx*4]
		movzx	eax,byte [esi+8*iter+6]
		movzx	ebx,byte [esi+8*iter+7]
		movd	mm3,[ecx+eax*4]
		movd	mm4,[ecx+ebx*4]
		punpcklwd mm1,mm2
		punpcklwd mm3,mm4
		punpckldq mm1,mm3

		store_multiple2 mm0,16*iter,mm1,16*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_8_to_16_x2
	movzx	eax,byte [esi]
	mov		eax,[ecx+eax*4]
	store_multiple2 ax,0,ax,2
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_16_x2
	%assign iter 0
	%rep 8
		movzx	eax,byte [esi+2*iter]
		movzx	ebx,byte [esi+2*iter+1]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple2 eax,8*iter,ebx,8*iter+4
		%assign iter iter+1
	%endrep
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_16_x2_mmx
	%assign iter 0
	%rep 4
		movzx	eax,byte [esi+4*iter]
		movzx	ebx,byte [esi+4*iter+1]
		movd	mm0,[ecx+eax*4]
		movd	mm1,[ecx+ebx*4]
		punpckldq mm0,mm1

		movzx	eax,byte [esi+4*iter+2]
		movzx	ebx,byte [esi+4*iter+3]
		movd	mm1,[ecx+eax*4]
		movd	mm2,[ecx+ebx*4]
		punpckldq mm1,mm2

		store_multiple2 mm0,16*iter,mm1,16*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_8_to_16_x3
	movzx	eax,byte [esi]
	mov		eax,[ecx+eax*4]
	store_multiple3 ax,0,ax,2,ax,4
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_16_x3
	%assign iter 0
	%rep 4
		movzx	eax,byte [esi+2*iter]
		movzx	ebx,byte [esi+2*iter+1]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple4 eax,12*iter,ax,12*iter+4,bx,12*iter+6,ebx,12*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_16_x3_mmx
	%assign iter 0
	%rep 4
		movzx	eax,byte [esi+4*iter]
		movzx	ebx,byte [esi+4*iter+1]
		movd	mm0,[ecx+eax*4]
		movd	mm2,[ecx+ebx*4]
		movzx	eax,byte [esi+4*iter+2]
		movzx	ebx,byte [esi+4*iter+3]
		movq	mm1,mm0
		movd	mm3,[ecx+eax*4]
		movd	mm5,[ecx+ebx*4]
		punpcklwd mm1,mm2
		movq	mm4,mm3
		punpcklwd mm4,mm5
		punpckldq mm0,mm1
		punpckldq mm2,mm3
		punpckldq mm4,mm5
		store_multiple3 mm0,24*iter,mm2,24*iter+8,mm4,24*iter+16
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_8_to_16_rgb
	movzx	eax,byte [esi]
	mov		eax,[ecx+eax*4]
	store_multiple2 ax,0,ax,2
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_16_rgb
	%assign iter 0
	%rep 4
		movzx	eax,byte [esi+4*iter]
		movzx	ebx,byte [esi+4*iter+1]
		movd	mm0,[ecx+eax*4]
		movd	mm1,[ecx+ebx*4]
		punpckldq mm0,mm1

		movzx	eax,byte [esi+4*iter+2]
		movzx	ebx,byte [esi+4*iter+3]
		movd	mm1,[ecx+eax*4]
		movd	mm2,[ecx+ebx*4]
		punpckldq mm1,mm2

		mov		eax,[_asmblit_srcheight]
		movq	mm2,mm0
		movq	mm3,mm1
		mov		ebx,[_asmblit_dstpitch]
		movq	mm6,mm0
		movq	mm7,mm1
		psrlq	mm0,2
		psrlq	mm1,2
		shl		eax,7
		movq	mm4,mm0
		movq	mm5,mm1
		pand	mm0,[_asmblit_rgbmask+eax+((16*iter)&31)+64]
		pand	mm1,[_asmblit_rgbmask+eax+((16*iter+8)&31)+64]
		pand	mm4,[_asmblit_rgbmask+eax+((16*iter)&31)]
		pand	mm5,[_asmblit_rgbmask+eax+((16*iter+8)&31)]
		psubd	mm2,mm0
		psubd	mm3,mm1
		psubd	mm6,mm4
		psubd	mm7,mm5
		movq	[edi+16*iter],mm2
		movq	[edi+16*iter+8],mm3
		movq	[edi+ebx+16*iter],mm6
		movq	[edi+ebx+16*iter+8],mm7
		%assign iter iter+1
	%endrep
SNIPPET_END


;//============================================================
;//	8bpp to 24bpp blitters
;//============================================================

SNIPPET_BEGIN asmblit1_8_to_24_x1
	movzx	eax,byte [esi]
	mov		eax,[ecx+eax*4]
	store_multiple ax,0
	shr		eax,16
	store_multiple al,2
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_24_x1
	%assign iter 0
	%rep 4
		movzx	eax,byte [esi+4*iter]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		movzx	eax,byte [esi+4*iter+1]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,8
		store_multiple ebx,12*iter
		shrd	ebx,eax,24
		movzx	eax,byte [esi+4*iter+2]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,16
		store_multiple ebx,12*iter+4
		shrd	ebx,eax,24
		movzx	eax,byte [esi+4*iter+3]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		store_multiple ebx,12*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_8_to_24_x2
	movzx	eax,byte [esi]
	mov		eax,[ecx+eax*4]
	shrd	ebx,eax,24
	shrd	ebx,eax,8
	shr		eax,8
	store_multiple2 ebx,0,ax,4
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_24_x2
	%assign iter 0
	%rep 8
		movzx	eax,byte [esi+2*iter]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,12*iter
		shrd	ebx,eax,24
		movzx	eax,byte [esi+2*iter+1]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,16
		store_multiple ebx,12*iter+4
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,12*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_8_to_24_x3
	movzx	eax,byte [esi]
	mov		eax,[ecx+eax*4]
	shrd	ebx,eax,24
	shrd	ebx,eax,8
	store_multiple ebx,0
	shrd	ebx,eax,24
	shrd	ebx,eax,16
	shr		eax,16
	store_multiple2 ebx,4,al,8
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_24_x3
	%assign iter 0
	%rep 4
		movzx	eax,byte [esi+4*iter]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,36*iter
		shrd	ebx,eax,24
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+4
		shrd	ebx,eax,24
		movzx	eax,byte [esi+4*iter+1]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+8
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,36*iter+12
		shrd	ebx,eax,24
		movzx	eax,byte [esi+4*iter+2]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+16
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+20
		movzx	eax,byte [esi+4*iter+3]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,8
		store_multiple ebx,36*iter+24
		shrd	ebx,eax,24
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+28
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+32
		%assign iter iter+1
	%endrep
SNIPPET_END


;//============================================================
;//	8bpp to 32bpp blitters
;//============================================================

SNIPPET_BEGIN asmblit1_8_to_32_x1
	movzx	eax,byte [esi]
	mov		eax,[ecx+eax*4]
	store_multiple eax,0
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_32_x1
	%assign iter 0
	%rep 8
		movzx	eax,byte [esi+2*iter]
		movzx	ebx,byte [esi+2*iter+1]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple2 eax,8*iter,ebx,8*iter+4
		%assign iter iter+1
	%endrep
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_32_x1_mmx
	%assign iter 0
	%rep 4
		movzx	eax,byte [esi+4*iter]
		movzx	ebx,byte [esi+4*iter+1]
		movd	mm0,[ecx+eax*4]
		movd	mm1,[ecx+ebx*4]
		punpckldq mm0,mm1

		movzx	eax,byte [esi+4*iter+2]
		movzx	ebx,byte [esi+4*iter+3]
		movd	mm1,[ecx+eax*4]
		movd	mm2,[ecx+ebx*4]
		punpckldq mm1,mm2

		store_multiple2 mm0,16*iter,mm1,16*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_8_to_32_x2
	movzx	eax,byte [esi]
	mov		eax,[ecx+eax*4]
	store_multiple2 eax,0,eax,4
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_32_x2
	%assign iter 0
	%rep 8
		movzx	eax,byte [esi+2*iter]
		movzx	ebx,byte [esi+2*iter+1]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple4 eax,16*iter,eax,16*iter+4,ebx,16*iter+8,ebx,16*iter+12
		%assign iter iter+1
	%endrep
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_32_x2_mmx
	%assign iter 0
	%rep 8
		movzx	eax,byte [esi+2*iter]
		movzx	ebx,byte [esi+2*iter+1]
		movd	mm0,[ecx+eax*4]
		movd	mm1,[ecx+ebx*4]
		punpckldq mm0,mm0
		punpckldq mm1,mm1
		store_multiple2 mm0,16*iter,mm1,16*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_8_to_32_x3
	movzx	eax,byte [esi]
	mov		eax,[ecx+eax*4]
	store_multiple3 eax,0,eax,4,eax,8
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_32_x3
	%assign iter 0
	%rep 8
		movzx	eax,byte [esi+2*iter]
		movzx	ebx,byte [esi+2*iter+1]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple6 eax,24*iter,eax,24*iter+4,eax,24*iter+8,ebx,24*iter+12,ebx,24*iter+16,ebx,24*iter+20
		%assign iter iter+1
	%endrep
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_32_x3_mmx
	%assign iter 0
	%rep 8
		movzx	eax,byte [esi+2*iter]
		movzx	ebx,byte [esi+2*iter+1]
		movd	mm0,[ecx+eax*4]
		movd	mm2,[ecx+ebx*4]
		movq	mm1,mm0
		punpckldq mm0,mm0
		punpckldq mm1,mm2
		punpckldq mm2,mm2
		store_multiple3 mm0,24*iter,mm1,24*iter+8,mm2,24*iter+16
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_8_to_32_rgb
	movzx	eax,byte [esi]
	mov		eax,[ecx+eax*4]
	store_multiple2 eax,0,eax,4
SNIPPET_END

SNIPPET_BEGIN asmblit16_8_to_32_rgb
	%assign iter 0
	%rep 8
		movzx	eax,byte [esi+2*iter]
		movzx	ebx,byte [esi+2*iter+1]
		movd	mm0,[ecx+eax*4]
		movd	mm1,[ecx+ebx*4]
		punpckldq mm0,mm0
		punpckldq mm1,mm1

		mov		eax,[_asmblit_srcheight]
		movq	mm2,mm0
		movq	mm3,mm1
		mov		ebx,[_asmblit_dstpitch]
		movq	mm6,mm0
		movq	mm7,mm1
		psrlq	mm0,2
		psrlq	mm1,2
		shl		eax,7
		movq	mm4,mm0
		movq	mm5,mm1
		pand	mm0,[_asmblit_rgbmask+eax+((16*iter)&63)+64]
		pand	mm1,[_asmblit_rgbmask+eax+((16*iter+8)&63)+64]
		pand	mm4,[_asmblit_rgbmask+eax+((16*iter)&63)]
		pand	mm5,[_asmblit_rgbmask+eax+((16*iter+8)&63)]
		psubd	mm2,mm0
		psubd	mm3,mm1
		psubd	mm6,mm4
		psubd	mm7,mm5
		movq	[edi+16*iter],mm2
		movq	[edi+16*iter+8],mm3
		movq	[edi+ebx+16*iter],mm6
		movq	[edi+ebx+16*iter+8],mm7
		%assign iter iter+1
	%endrep
SNIPPET_END


;//============================================================
;//	16bpp to 16bpp blitters
;//============================================================

SNIPPET_BEGIN asmblit1_16_to_16_x1
	movzx	eax,word [esi]
	mov		eax,[ecx+eax*4]
	store_multiple ax,0
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_16_x1
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+4*iter]
		movzx	ebx,word [esi+4*iter+2]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		shrd	eax,ebx,16
		store_multiple eax,4*iter
		%assign iter iter+1
	%endrep
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_16_x1_mmx
	%assign iter 0
	%rep 2
		movzx	eax,word [esi+16*iter]
		movzx	ebx,word [esi+16*iter+2]
		movd	mm0,[ecx+eax*4]
		movd	mm1,[ecx+ebx*4]
		movzx	eax,word [esi+16*iter+4]
		movzx	ebx,word [esi+16*iter+6]
		movd	mm2,[ecx+eax*4]
		movd	mm3,[ecx+ebx*4]
		punpcklwd mm0,mm1
		punpcklwd mm2,mm3
		punpckldq mm0,mm2

		movzx	eax,word [esi+16*iter+8]
		movzx	ebx,word [esi+16*iter+10]
		movd	mm1,[ecx+eax*4]
		movd	mm2,[ecx+ebx*4]
		movzx	eax,word [esi+16*iter+12]
		movzx	ebx,word [esi+16*iter+14]
		movd	mm3,[ecx+eax*4]
		movd	mm4,[ecx+ebx*4]
		punpcklwd mm1,mm2
		punpcklwd mm3,mm4
		punpckldq mm1,mm3

		store_multiple2 mm0,16*iter,mm1,16*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_16_to_16_x2
	movzx	eax,word [esi]
	mov		eax,[ecx+eax*4]
	store_multiple2 ax,0,ax,2
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_16_x2
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+4*iter]
		movzx	ebx,word [esi+4*iter+2]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple2 eax,8*iter,ebx,8*iter+4
		%assign iter iter+1
	%endrep
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_16_x2_mmx
	%assign iter 0
	%rep 4
		movzx	eax,word [esi+8*iter]
		movzx	ebx,word [esi+8*iter+2]
		movd	mm0,[ecx+eax*4]
		movd	mm1,[ecx+ebx*4]
		punpckldq mm0,mm1

		movzx	eax,word [esi+8*iter+4]
		movzx	ebx,word [esi+8*iter+6]
		movd	mm1,[ecx+eax*4]
		movd	mm2,[ecx+ebx*4]
		punpckldq mm1,mm2

		store_multiple2 mm0,16*iter,mm1,16*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_16_to_16_x3
	movzx	eax,word [esi]
	mov		eax,[ecx+eax*4]
	store_multiple3 ax,0,ax,2,ax,4
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_16_x3
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+4*iter]
		movzx	ebx,word [esi+4*iter+2]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple4 eax,12*iter,ax,12*iter+4,bx,12*iter+6,ebx,12*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_16_x3_mmx
	%assign iter 0
	%rep 4
		movzx	eax,word [esi+8*iter]
		movzx	ebx,word [esi+8*iter+2]
		movd	mm0,[ecx+eax*4]
		movd	mm2,[ecx+ebx*4]
		movzx	eax,word [esi+8*iter+4]
		movzx	ebx,word [esi+8*iter+6]
		movq	mm1,mm0
		movd	mm3,[ecx+eax*4]
		movd	mm5,[ecx+ebx*4]
		punpcklwd mm1,mm2
		movq	mm4,mm3
		punpcklwd mm4,mm5
		punpckldq mm0,mm1
		punpckldq mm2,mm3
		punpckldq mm4,mm5
		store_multiple3 mm0,24*iter,mm2,24*iter+8,mm4,24*iter+16
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_16_to_16_rgb
	movzx	eax,word [esi]
	mov		eax,[ecx+eax*4]
	store_multiple2 ax,0,ax,2
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_16_rgb
	%assign iter 0
	%rep 4
		movzx	eax,word [esi+8*iter]
		movzx	ebx,word [esi+8*iter+2]
		movd	mm0,[ecx+eax*4]
		movd	mm1,[ecx+ebx*4]
		punpckldq mm0,mm1

		movzx	eax,word [esi+8*iter+4]
		movzx	ebx,word [esi+8*iter+6]
		movd	mm1,[ecx+eax*4]
		movd	mm2,[ecx+ebx*4]
		punpckldq mm1,mm2

		mov		eax,[_asmblit_srcheight]
		movq	mm2,mm0
		movq	mm3,mm1
		mov		ebx,[_asmblit_dstpitch]
		movq	mm6,mm0
		movq	mm7,mm1
		psrlq	mm0,2
		psrlq	mm1,2
		shl		eax,7
		movq	mm4,mm0
		movq	mm5,mm1
		pand	mm0,[_asmblit_rgbmask+eax+((16*iter)&31)+64]
		pand	mm1,[_asmblit_rgbmask+eax+((16*iter+8)&31)+64]
		pand	mm4,[_asmblit_rgbmask+eax+((16*iter)&31)]
		pand	mm5,[_asmblit_rgbmask+eax+((16*iter+8)&31)]
		psubd	mm2,mm0
		psubd	mm3,mm1
		psubd	mm6,mm4
		psubd	mm7,mm5
		movq	[edi+16*iter],mm2
		movq	[edi+16*iter+8],mm3
		movq	[edi+ebx+16*iter],mm6
		movq	[edi+ebx+16*iter+8],mm7
		%assign iter iter+1
	%endrep
SNIPPET_END


;//============================================================
;//	16bpp to 24bpp blitters
;//============================================================

SNIPPET_BEGIN asmblit1_16_to_24_x1
	movzx	eax,word [esi]
	mov		eax,[ecx+eax*4]
	store_multiple ax,0
	shr		eax,16
	store_multiple al,2
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_24_x1
	%assign iter 0
	%rep 4
		movzx	eax,word [esi+8*iter]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		movzx	eax,word [esi+8*iter+2]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,8
		store_multiple ebx,12*iter
		shrd	ebx,eax,24
		movzx	eax,word [esi+8*iter+4]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,16
		store_multiple ebx,12*iter+4
		shrd	ebx,eax,24
		movzx	eax,word [esi+8*iter+6]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		store_multiple ebx,12*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_16_to_24_x2
	movzx	eax,word [esi]
	mov		eax,[ecx+eax*4]
	shrd	ebx,eax,24
	shrd	ebx,eax,8
	shr		eax,8
	store_multiple2 ebx,0,ax,4
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_24_x2
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+4*iter]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,12*iter
		shrd	ebx,eax,24
		movzx	eax,word [esi+4*iter+2]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,16
		store_multiple ebx,12*iter+4
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,12*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_16_to_24_x3
	movzx	eax,word [esi]
	mov		eax,[ecx+eax*4]
	shrd	ebx,eax,24
	shrd	ebx,eax,8
	store_multiple ebx,0
	shrd	ebx,eax,24
	shrd	ebx,eax,16
	shr		eax,16
	store_multiple2 ebx,4,al,8
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_24_x3
	%assign iter 0
	%rep 4
		movzx	eax,word [esi+8*iter]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,36*iter
		shrd	ebx,eax,24
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+4
		shrd	ebx,eax,24
		movzx	eax,word [esi+8*iter+2]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+8
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,36*iter+12
		shrd	ebx,eax,24
		movzx	eax,word [esi+8*iter+4]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+16
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+20
		movzx	eax,word [esi+8*iter+6]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,8
		store_multiple ebx,36*iter+24
		shrd	ebx,eax,24
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+28
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+32
		%assign iter iter+1
	%endrep
SNIPPET_END


;//============================================================
;//	16bpp to 32bpp blitters
;//============================================================

SNIPPET_BEGIN asmblit1_16_to_32_x1
	movzx	eax,word [esi]
	mov		eax,[ecx+eax*4]
	store_multiple eax,0
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_32_x1
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+4*iter]
		movzx	ebx,word [esi+4*iter+2]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple2 eax,8*iter,ebx,8*iter+4
		%assign iter iter+1
	%endrep
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_32_x1_mmx
	%assign iter 0
	%rep 4
		movzx	eax,word [esi+8*iter]
		movzx	ebx,word [esi+8*iter+2]
		movd	mm0,[ecx+eax*4]
		movd	mm1,[ecx+ebx*4]
		punpckldq mm0,mm1

		movzx	eax,word [esi+8*iter+4]
		movzx	ebx,word [esi+8*iter+6]
		movd	mm1,[ecx+eax*4]
		movd	mm2,[ecx+ebx*4]
		punpckldq mm1,mm2

		store_multiple2 mm0,16*iter,mm1,16*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_16_to_32_x2
	movzx	eax,word [esi]
	mov		eax,[ecx+eax*4]
	store_multiple2 eax,0,eax,4
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_32_x2
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+4*iter]
		movzx	ebx,word [esi+4*iter+2]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple4 eax,16*iter,eax,16*iter+4,ebx,16*iter+8,ebx,16*iter+12
		%assign iter iter+1
	%endrep
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_32_x2_mmx
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+4*iter]
		movzx	ebx,word [esi+4*iter+2]
		movd	mm0,[ecx+eax*4]
		movd	mm1,[ecx+ebx*4]
		punpckldq mm0,mm0
		punpckldq mm1,mm1
		store_multiple2 mm0,16*iter,mm1,16*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_16_to_32_x3
	movzx	eax,word [esi]
	mov		eax,[ecx+eax*4]
	store_multiple3 eax,0,eax,4,eax,8
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_32_x3
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+4*iter]
		movzx	ebx,word [esi+4*iter+2]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple6 eax,24*iter,eax,24*iter+4,eax,24*iter+8,ebx,24*iter+12,ebx,24*iter+16,ebx,24*iter+20
		%assign iter iter+1
	%endrep
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_32_x3_mmx
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+4*iter]
		movzx	ebx,word [esi+4*iter+2]
		movd	mm0,[ecx+eax*4]
		movd	mm2,[ecx+ebx*4]
		movq	mm1,mm0
		punpckldq mm0,mm0
		punpckldq mm1,mm2
		punpckldq mm2,mm2
		store_multiple3 mm0,24*iter,mm1,24*iter+8,mm2,24*iter+16
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_16_to_32_rgb
	movzx	eax,word [esi]
	mov		eax,[ecx+eax*4]
	store_multiple2 eax,0,eax,4
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_32_rgb
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+4*iter]
		movzx	ebx,word [esi+4*iter+2]
		movd	mm0,[ecx+eax*4]
		movd	mm1,[ecx+ebx*4]
		punpckldq mm0,mm0
		punpckldq mm1,mm1

		mov		eax,[_asmblit_srcheight]
		movq	mm2,mm0
		movq	mm3,mm1
		mov		ebx,[_asmblit_dstpitch]
		movq	mm6,mm0
		movq	mm7,mm1
		psrlq	mm0,2
		psrlq	mm1,2
		shl		eax,7
		movq	mm4,mm0
		movq	mm5,mm1
		pand	mm0,[_asmblit_rgbmask+eax+((16*iter)&63)+64]
		pand	mm1,[_asmblit_rgbmask+eax+((16*iter+8)&63)+64]
		pand	mm4,[_asmblit_rgbmask+eax+((16*iter)&63)]
		pand	mm5,[_asmblit_rgbmask+eax+((16*iter+8)&63)]
		psubd	mm2,mm0
		psubd	mm3,mm1
		psubd	mm6,mm4
		psubd	mm7,mm5
		movq	[edi+16*iter],mm2
		movq	[edi+16*iter+8],mm3
		movq	[edi+ebx+16*iter],mm6
		movq	[edi+ebx+16*iter+8],mm7
		%assign iter iter+1
	%endrep
SNIPPET_END


;//============================================================
;//	32bpp to 16bpp blitters
;//============================================================

SNIPPET_BEGIN asmblit1_32_to_16_x1
	mov		eax,[esi]
	mov		ecx,eax
	SHIFT	SHIFT32TO16TYPE_ecx_r
	mov		ebx,eax
	SHIFT	SHIFT32TO16TYPE_ebx_g
	or		ecx,ebx
	mov		ebx,eax
	SHIFT	SHIFT32TO16TYPE_ebx_b
	or		ecx,ebx
	store_multiple cx,0
SNIPPET_END

SNIPPET_BEGIN asmblit16_32_to_16_x1
	%assign iter 0
	%rep 16
		mov		eax,[esi+4*iter]
		mov		ecx,eax
		SHIFT	SHIFT32TO16TYPE_ecx_r
		mov		ebx,eax
		SHIFT	SHIFT32TO16TYPE_ebx_g
		or		ecx,ebx
		mov		ebx,eax
		SHIFT	SHIFT32TO16TYPE_ebx_b
		or		ecx,ebx
		store_multiple cx,2*iter
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_32_to_16_x2
	mov		eax,[esi]
	mov		ecx,eax
	SHIFT	SHIFT32TO16TYPE_ecx_r
	mov		ebx,eax
	SHIFT	SHIFT32TO16TYPE_ebx_g
	or		ecx,ebx
	mov		ebx,eax
	SHIFT	SHIFT32TO16TYPE_ebx_b
	or		ecx,ebx
	store_multiple2 cx,0,cx,2
SNIPPET_END

SNIPPET_BEGIN asmblit16_32_to_16_x2
	%assign iter 0
	%rep 16
		mov		eax,[esi+4*iter]
		mov		ecx,eax
		SHIFT	SHIFT32TO16TYPE_ecx_r
		mov		ebx,eax
		SHIFT	SHIFT32TO16TYPE_ebx_g
		or		ecx,ebx
		mov		ebx,eax
		SHIFT	SHIFT32TO16TYPE_ebx_b
		or		ecx,ebx
		store_multiple2 cx,4*iter,cx,4*iter+2
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_32_to_16_x3
	mov		eax,[esi]
	mov		ecx,eax
	SHIFT	SHIFT32TO16TYPE_ecx_r
	mov		ebx,eax
	SHIFT	SHIFT32TO16TYPE_ebx_g
	or		ecx,ebx
	mov		ebx,eax
	SHIFT	SHIFT32TO16TYPE_ebx_b
	or		ecx,ebx
	store_multiple3 cx,0,cx,2,cx,4
SNIPPET_END

SNIPPET_BEGIN asmblit16_32_to_16_x3
	%assign iter 0
	%rep 16
		mov		eax,[esi+4*iter]
		mov		ecx,eax
		SHIFT	SHIFT32TO16TYPE_ecx_r
		mov		ebx,eax
		SHIFT	SHIFT32TO16TYPE_ebx_g
		or		ecx,ebx
		mov		ebx,eax
		SHIFT	SHIFT32TO16TYPE_ebx_b
		or		ecx,ebx
		store_multiple3 cx,6*iter,cx,6*iter+2,cx,6*iter+4
		%assign iter iter+1
	%endrep
SNIPPET_END


;//============================================================
;//	32bpp to 24bpp blitters
;//============================================================

SNIPPET_BEGIN asmblit1_32_to_24_x1
	mov		eax,[esi]
	store_multiple ax,0
	shr		eax,16
	store_multiple al,2
SNIPPET_END

SNIPPET_BEGIN asmblit16_32_to_24_x1
	%assign iter 0
	%rep 4
		mov		eax,[esi+16*iter]
		shrd	ebx,eax,24
		mov		eax,[esi+16*iter+4]
		shrd	ebx,eax,8
		store_multiple ebx,12*iter
		shrd	ebx,eax,24
		mov		eax,[esi+16*iter+8]
		shrd	ebx,eax,16
		store_multiple ebx,12*iter+4
		shrd	ebx,eax,24
		mov		eax,[esi+16*iter+12]
		shrd	ebx,eax,24
		store_multiple ebx,12*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_32_to_24_x2
	mov		eax,[esi]
	shrd	ebx,eax,24
	shrd	ebx,eax,8
	shr		eax,8
	store_multiple2 ebx,0,ax,4
SNIPPET_END

SNIPPET_BEGIN asmblit16_32_to_24_x2
	%assign iter 0
	%rep 8
		mov		eax,[esi+8*iter]
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,12*iter
		shrd	ebx,eax,24
		mov		eax,[esi+8*iter+4]
		shrd	ebx,eax,16
		store_multiple ebx,12*iter+4
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,12*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_32_to_24_x3
	mov		eax,[esi]
	shrd	ebx,eax,24
	shrd	ebx,eax,8
	store_multiple ebx,0
	shrd	ebx,eax,24
	shrd	ebx,eax,16
	shr		eax,16
	store_multiple2 ebx,4,al,8
SNIPPET_END

SNIPPET_BEGIN asmblit16_32_to_24_x3
	%assign iter 0
	%rep 4
		mov		eax,[esi+16*iter]
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,36*iter
		shrd	ebx,eax,24
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+4
		shrd	ebx,eax,24
		mov		eax,[esi+16*iter+4]
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+8
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,36*iter+12
		shrd	ebx,eax,24
		mov		eax,[esi+16*iter+8]
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+16
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+20
		mov		eax,[esi+16*iter+12]
		shrd	ebx,eax,8
		store_multiple ebx,36*iter+24
		shrd	ebx,eax,24
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+28
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+32
		%assign iter iter+1
	%endrep
SNIPPET_END


;//============================================================
;//	32bpp to 32bpp blitters
;//============================================================

SNIPPET_BEGIN asmblit1_32_to_32_x1
	mov		eax,[esi]
	store_multiple eax,0
SNIPPET_END

SNIPPET_BEGIN asmblit16_32_to_32_x1
	%assign iter 0
	%rep 8
		mov		eax,[esi+8*iter]
		mov		ebx,[esi+8*iter+4]
		store_multiple2 eax,8*iter,ebx,8*iter+4
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_32_to_32_x2
	mov		eax,[esi]
	store_multiple2 eax,0,eax,4
SNIPPET_END

SNIPPET_BEGIN asmblit16_32_to_32_x2
	%assign iter 0
	%rep 8
		mov		eax,[esi+8*iter]
		mov		ebx,[esi+8*iter+4]
		store_multiple4 eax,16*iter,eax,16*iter+4,ebx,16*iter+8,ebx,16*iter+12
		%assign iter iter+1
	%endrep
SNIPPET_END


SNIPPET_BEGIN asmblit1_32_to_32_x3
	mov		eax,[esi]
	store_multiple3 eax,0,eax,4,eax,8
SNIPPET_END

SNIPPET_BEGIN asmblit16_32_to_32_x3
	%assign iter 0
	%rep 8
		mov		eax,[esi+8*iter]
		mov		ebx,[esi+8*iter+4]
		store_multiple6 eax,24*iter,eax,24*iter+4,eax,24*iter+8,ebx,24*iter+12,ebx,24*iter+16,ebx,24*iter+20
		%assign iter iter+1
	%endrep
SNIPPET_END


[SECTION .text]


;//============================================================
;//	core blitter
;//============================================================
;//
;// overall structure:
;//	in general, dirty code is copied after original
;// except for those blocks marked with a (*)
;//
;// 	header
;//
;// yloop:
;//		yloop_top
;//
;//		middlexloop_header
;// middlexloop:
;//		middlexloop_top (-> middlexloop_bottom)
;//		preblit16
;//		(blit16_core)
;//		postblit16
;//		middlexloop_bottom (-> middlexloop_top)
;//
;//		lastxloop_header (-> yloop_bottom)
;//	lastxloop:
;//		lastxloop_top
;//		(blit1_core)
;//		lastxloop_bottom (-> lastxloop_top)
;//
;//		yloop_bottom(*)
;//		(multiply eax by YSCALE)
;//		yloop_bottom2
;//
;// 	footer
;//
;//============================================================

;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_header
	;// save everything
	pushad

	;// load the source/dest pointers
	mov		esi,[_asmblit_srcdata]
	mov		edi,[_asmblit_dstdata]

	;// load the palette pointer
	mov		ecx,[_asmblit_srclookup]
SNIPPET_END


SNIPPET_BEGIN asmblit_header_dirty
	;// load dirty pointer and reset the dirty counter
	mov		edx,[_asmblit_dirtydata]
	mov		byte [dirty_count],16
SNIPPET_END


;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_yloop_top
	push	esi
	push	edi
SNIPPET_END


SNIPPET_BEGIN asmblit_yloop_top_dirty
	;// save the dirty row start
	push	edx
SNIPPET_END


;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_middlexloop_header
	;// determine the number of 16-byte chunks to blit
	mov		ebp,FIXUPVALUE(FIXUPVAL_MIDDLEXCOUNT)
SNIPPET_END


SNIPPET_BEGIN asmblit_middlexloop_header_dirty
	;// nothing to do here
SNIPPET_END


;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_middlexloop_top
	;// nothing to do here
SNIPPET_END


SNIPPET_BEGIN asmblit_middlexloop_top_dirty
	;// check for a dirty block; if not, skip
	test	byte [edx],0xff
	lea		edx,[edx+1]
	jz		near FIXUPADDRESS(FIXUPADDR_MIDDLEXBOTTOM)
SNIPPET_END


;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_middlexloop_bottom
	dec		ebp
	lea		esi,[esi+FIXUPVALUE(FIXUPVAL_SRCBYTES16)]
	lea		edi,[edi+FIXUPVALUE(FIXUPVAL_DSTBYTES16)]
	jne		near FIXUPADDRESS(FIXUPADDR_MIDDLEXTOP)
SNIPPET_END


SNIPPET_BEGIN asmblit_middlexloop_bottom_dirty
	;// nothing to do here
SNIPPET_END


;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_lastxloop_header
	mov		ebp,FIXUPVALUE(FIXUPVAL_LASTXCOUNT)
SNIPPET_END


SNIPPET_BEGIN asmblit_lastxloop_header_dirty
	test	byte [edx],0xff
	jz		near FIXUPADDRESS(FIXUPADDR_YBOTTOM)
SNIPPET_END


;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_lastxloop_top
	;// nothing to do here
SNIPPET_END


SNIPPET_BEGIN asmblit_lastxloop_top_dirty
	;// nothing to do here
SNIPPET_END


;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_lastxloop_bottom
	dec		ebp
	lea		esi,[esi+FIXUPVALUE(FIXUPVAL_SRCBYTES1)]
	lea		edi,[edi+FIXUPVALUE(FIXUPVAL_DSTBYTES1)]
	jne		near FIXUPADDRESS(FIXUPADDR_LASTXTOP)
SNIPPET_END


SNIPPET_BEGIN asmblit_lastxloop_bottom_dirty
	;// nothing to do here
SNIPPET_END


;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_yloop_bottom_dirty
	dec		byte [dirty_count]
	pop		edx
	jnz		.dontadvance
	mov		byte [dirty_count],16
	lea		edx,[edx+FIXUPVALUE(FIXUPVAL_DIRTYADVANCE)]
.dontadvance:
SNIPPET_END


SNIPPET_BEGIN asmblit_yloop_bottom
	dec		dword [_asmblit_srcheight]
	pop		edi
	pop		esi
	lea		edi,[edi+FIXUPVALUE(FIXUPVAL_DSTADVANCE)]
	lea		esi,[esi+FIXUPVALUE(FIXUPVAL_SRCADVANCE)]
	jne		near FIXUPADDRESS(FIXUPADDR_YTOP)
SNIPPET_END


;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_footer_mmx
	emms
SNIPPET_END

SNIPPET_BEGIN asmblit_footer_mmx_dirty
	;// nothing to do here
SNIPPET_END


;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_footer
	popad
	ret
SNIPPET_END

SNIPPET_BEGIN asmblit_footer_dirty
	;// nothing to do here
SNIPPET_END



;//============================================================
;//	MMX detection
;//============================================================

GLOBAL _asmblit_has_mmx
_asmblit_has_mmx:
	pushad

	;// attempt to change the ID flag
	pushfd
	pop		eax
	xor		eax,1<<21
	push	eax
	popfd

	;// if we can't, they definitely don't have it
	pushfd
	pop		ebx
	xor		eax,ebx
	test	eax,1<<21
	jnz		.DoesntHaveMMX

	;// use CPUID
	mov		eax,1
	cpuid
	test	edx,1<<23
	jz		.DoesntHaveMMX

	;// okay, we have it
	popad
	mov		eax,1
	ret

.DoesntHaveMMX:
	popad
	mov		eax,0
	ret



;//============================================================
;//	SSE2 detection
;//============================================================

GLOBAL _asmblit_has_xmm
_asmblit_has_xmm:
	pushad

	;// attempt to change the ID flag
	pushfd
	pop		eax
	xor		eax,1<<21
	push	eax
	popfd

	;// if we can't, they definitely don't have it
	pushfd
	pop		ebx
	xor		eax,ebx
	test	eax,1<<21
	jnz		.DoesntHaveXMM

	;// use CPUID
	mov		eax,1
	cpuid
	test	edx,1<<26
	jz		.DoesntHaveXMM

	;// okay, we have it
	popad
	mov		eax,1
	ret

.DoesntHaveXMM:
	popad
	mov		eax,0
	ret
