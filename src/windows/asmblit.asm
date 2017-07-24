[BITS 32]

;//============================================================
;// ELF compatibility
;//============================================================

%ifdef NO_UNDERSCORE
%macro CGLOBAL 1
          GLOBAL %1
%endmacro

%macro cextern 1
          extern %1
%endmacro
%else
%macro CGLOBAL 1
          GLOBAL _%1
%define %1 _%1
%endmacro

%macro cextern 1
          extern _%1
%define %1 _%1
%endmacro
%endif


;//============================================================
;//	IMPORTS
;//============================================================

cextern asmblit_srcdata
cextern asmblit_srcheight
cextern asmblit_srclookup

cextern asmblit_dstdata
cextern asmblit_dstpitch

cextern asmblit_rgbmask;


;//============================================================
;//	LOCAL VARIABLES
;//============================================================

[SECTION .data]
last_row:		dd	0
row_index:		dd	0



;//============================================================
;//	CONSTANTS
;//============================================================

%define TAG_FIXUPADDRESS		0
%define TAG_FIXUPVALUE			1
%define TAG_SHIFT32TO16			2
%define TAG_REGOFFSET			3
%define TAG_ENDSNIPPET			7

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

%define REGISTER_mm0			16
%define REGISTER_mm1			17
%define REGISTER_mm2			18
%define REGISTER_mm3			19
%define REGISTER_mm4			20
%define REGISTER_mm5			21
%define REGISTER_mm6			22
%define REGISTER_mm7			23
%define REGISTER_xmm0			24
%define REGISTER_xmm1			25
%define REGISTER_xmm2			26
%define REGISTER_xmm3			27
%define REGISTER_xmm4			28
%define REGISTER_xmm5			29
%define REGISTER_xmm6			30
%define REGISTER_xmm7			31

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

%define FIXUPVAL_DSTBYTES1		0
%define FIXUPVAL_DSTBYTES16		1
%define FIXUPVAL_DSTADVANCE		2
%define FIXUPVAL_MIDDLEXCOUNT	3
%define FIXUPVAL_LASTXCOUNT		4
%define FIXUPVAL_SRCBYTES1		5
%define FIXUPVAL_SRCBYTES16		6
%define FIXUPVAL_SRCADVANCE		7
%define FIXUPVAL_SRCPREFETCH16	8
%define FIXUPVAL_PIXOFFSET0		9
%define FIXUPVAL_PIXOFFSET15	24

%define TAG_COMMON(t,x)			(0x00cccccc | ((t)<<29) | ((x)<<24))

%define FIXUPADDRESS(x)			($ + 6 + TAG_COMMON(TAG_FIXUPADDRESS, x))
%define FIXUPVALUE(x)			TAG_COMMON(TAG_FIXUPVALUE, x)
%define FIXUPPIXEL(x)			FIXUPVALUE((x) + FIXUPVAL_PIXOFFSET0)
%define ENDSNIPPET				TAG_COMMON(TAG_ENDSNIPPET, 0)



;//============================================================
;//	SNIPPET tagging
;//============================================================

%macro SNIPPET_BEGIN 1
CGLOBAL %1
%1:
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
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_mm0)
	%elifidni %1,mm1
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_mm1)
	%elifidni %1,mm2
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_mm2)
	%elifidni %1,mm3
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_mm3)
	%elifidni %1,mm4
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_mm4)
	%elifidni %1,mm5
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_mm5)
	%elifidni %1,mm6
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_mm6)
	%elifidni %1,mm7
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_mm7)
	%elifidni %1,xmm0
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_xmm0)
	%elifidni %1,xmm1
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_xmm1)
	%elifidni %1,xmm2
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_xmm2)
	%elifidni %1,xmm3
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_xmm3)
	%elifidni %1,xmm4
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_xmm4)
	%elifidni %1,xmm5
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_xmm5)
	%elifidni %1,xmm6
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_xmm6)
	%elifidni %1,xmm7
		dd		TAG_COMMON(TAG_REGOFFSET, REGISTER_xmm7)
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
;//	16bpp to 16bpp blitters (1x scale)
;//============================================================

SNIPPET_BEGIN asmblit1_16_to_16_x1
	movzx	eax,word [esi+FIXUPPIXEL(0)]
	mov		eax,[ecx+eax*4]
	store_multiple ax,0
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_16_x1
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+FIXUPPIXEL(0+2*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(1+2*iter)]
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
		movzx	eax,word [esi+FIXUPPIXEL(0+8*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(1+8*iter)]
		movd	mm0,[ecx+eax*4]
		movd	mm1,[ecx+ebx*4]
		movzx	eax,word [esi+FIXUPPIXEL(2+8*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(3+8*iter)]
		movd	mm2,[ecx+eax*4]
		movd	mm3,[ecx+ebx*4]
		punpcklwd mm0,mm1
		punpcklwd mm2,mm3
		punpckldq mm0,mm2

		movzx	eax,word [esi+FIXUPPIXEL(4+8*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(5+8*iter)]
		movd	mm1,[ecx+eax*4]
		movd	mm2,[ecx+ebx*4]
		movzx	eax,word [esi+FIXUPPIXEL(6+8*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(7+8*iter)]
		movd	mm3,[ecx+eax*4]
		movd	mm4,[ecx+ebx*4]
		punpcklwd mm1,mm2
		punpcklwd mm3,mm4
		punpckldq mm1,mm3

		store_multiple2 mm0,16*iter,mm1,16*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_16_x1_sse
	movzx	eax,word [esi+FIXUPPIXEL(0)]
	movzx	ebx,word [esi+FIXUPPIXEL(4)]
	movzx	edx,word [esi+FIXUPPIXEL(8)]
	movd	mm0,[ecx+eax*4]
	movd	mm1,[ecx+ebx*4]
	movd	mm2,[ecx+edx*4]

	movzx	eax,word [esi+FIXUPPIXEL(12)]
	movzx	ebx,word [esi+FIXUPPIXEL(1)]
	movzx	edx,word [esi+FIXUPPIXEL(5)]
	movd	mm3,[ecx+eax*4]
	pinsrw	mm0,[ecx+ebx*4],1
	pinsrw	mm1,[ecx+edx*4],1

	movzx	eax,word [esi+FIXUPPIXEL(9)]
	movzx	ebx,word [esi+FIXUPPIXEL(13)]
	movzx	edx,word [esi+FIXUPPIXEL(2)]
	pinsrw	mm2,[ecx+eax*4],1
	pinsrw	mm3,[ecx+ebx*4],1
	pinsrw	mm0,[ecx+edx*4],2

	movzx	eax,word [esi+FIXUPPIXEL(6)]
	movzx	ebx,word [esi+FIXUPPIXEL(10)]
	movzx	edx,word [esi+FIXUPPIXEL(14)]
	pinsrw	mm1,[ecx+eax*4],2
	pinsrw	mm2,[ecx+ebx*4],2
	pinsrw	mm3,[ecx+edx*4],2

	movzx	eax,word [esi+FIXUPPIXEL(3)]
	movzx	ebx,word [esi+FIXUPPIXEL(7)]
	movzx	edx,word [esi+FIXUPPIXEL(11)]
	pinsrw	mm0,[ecx+eax*4],3
	pinsrw	mm1,[ecx+ebx*4],3
	pinsrw	mm2,[ecx+edx*4],3

	movzx	eax,word [esi+FIXUPPIXEL(15)]
	pinsrw	mm3,[ecx+eax*4],3

	store_multiple4 mm0,0,mm1,8,mm2,16,mm3,24
SNIPPET_END


;//============================================================
;//	16bpp to 16bpp blitters (2x scale)
;//============================================================

SNIPPET_BEGIN asmblit1_16_to_16_x2
	movzx	eax,word [esi+FIXUPPIXEL(0)]
	mov		eax,[ecx+eax*4]
	store_multiple2 ax,0,ax,2
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_16_x2
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+FIXUPPIXEL(0+2*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(1+2*iter)]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple2 eax,8*iter,ebx,8*iter+4
		%assign iter iter+1
	%endrep
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_16_x2_mmx
	%assign iter 0
	%rep 4
		movzx	eax,word [esi+FIXUPPIXEL(0+4*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(1+4*iter)]
		movd	mm0,[ecx+eax*4]
		movd	mm1,[ecx+ebx*4]
		punpckldq mm0,mm1

		movzx	eax,word [esi+FIXUPPIXEL(2+4*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(3+4*iter)]
		movd	mm1,[ecx+eax*4]
		movd	mm2,[ecx+ebx*4]
		punpckldq mm1,mm2

		store_multiple2 mm0,16*iter,mm1,16*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


;//============================================================
;//	16bpp to 16bpp blitters (3x scale)
;//============================================================

SNIPPET_BEGIN asmblit1_16_to_16_x3
	movzx	eax,word [esi+FIXUPPIXEL(0)]
	mov		eax,[ecx+eax*4]
	store_multiple3 ax,0,ax,2,ax,4
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_16_x3
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+FIXUPPIXEL(0+2*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(1+2*iter)]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple4 eax,12*iter,ax,12*iter+4,bx,12*iter+6,ebx,12*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_16_x3_mmx
	%assign iter 0
	%rep 4
		movzx	eax,word [esi+FIXUPPIXEL(0+4*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(1+4*iter)]
		movd	mm0,[ecx+eax*4]
		movd	mm2,[ecx+ebx*4]
		movzx	eax,word [esi+FIXUPPIXEL(2+4*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(3+4*iter)]
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


;//============================================================
;//	16bpp to 16bpp blitters (RGB)
;//============================================================

SNIPPET_BEGIN asmblit1_16_to_16_rgb
	movzx	eax,word [esi+FIXUPPIXEL(0)]
	movd	mm0,[ecx+eax*4]

	mov		eax,[asmblit_srcheight]
	movq	mm2,mm0
	mov		ebx,FIXUPVALUE(FIXUPVAL_LASTXCOUNT)
	movq	mm6,mm0
	sub		ebx,ebp
	psrlq	mm0,2
	shl		ebx,2
	shl		eax,7
	and		ebx,31
	movq	mm4,mm0
	pand	mm0,[asmblit_rgbmask+eax+ebx+64]
	pand	mm4,[asmblit_rgbmask+eax+ebx]
	mov		ebx,[asmblit_dstpitch]
	psubd	mm2,mm0
	psubd	mm6,mm4
	movd	[edi],mm2
	movd	[edi+ebx],mm6
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_16_rgb
	%assign iter 0
	%rep 4
		movzx	eax,word [esi+FIXUPPIXEL(0+4*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(1+4*iter)]
		movd	mm0,[ecx+eax*4]
		movd	mm1,[ecx+ebx*4]
		punpckldq mm0,mm1

		movzx	eax,word [esi+FIXUPPIXEL(2+4*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(3+4*iter)]
		movd	mm1,[ecx+eax*4]
		movd	mm2,[ecx+ebx*4]
		punpckldq mm1,mm2

		mov		eax,[asmblit_srcheight]
		movq	mm2,mm0
		movq	mm3,mm1
		mov		ebx,[asmblit_dstpitch]
		movq	mm6,mm0
		movq	mm7,mm1
		psrlq	mm0,2
		psrlq	mm1,2
		shl		eax,7
		movq	mm4,mm0
		movq	mm5,mm1
		pand	mm0,[asmblit_rgbmask+eax+((16*iter)&31)+64]
		pand	mm1,[asmblit_rgbmask+eax+((16*iter+8)&31)+64]
		pand	mm4,[asmblit_rgbmask+eax+((16*iter)&31)]
		pand	mm5,[asmblit_rgbmask+eax+((16*iter+8)&31)]
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
;//	16bpp to 24bpp blitters (1x scale)
;//============================================================

SNIPPET_BEGIN asmblit1_16_to_24_x1
	movzx	eax,word [esi+FIXUPPIXEL(0)]
	mov		eax,[ecx+eax*4]
	store_multiple ax,0
	shr		eax,16
	store_multiple al,2
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_24_x1
	%assign iter 0
	%rep 4
		movzx	eax,word [esi+FIXUPPIXEL(0+4*iter)]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		movzx	eax,word [esi+FIXUPPIXEL(1+4*iter)]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,8
		store_multiple ebx,12*iter
		shrd	ebx,eax,24
		movzx	eax,word [esi+FIXUPPIXEL(2+4*iter)]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,16
		store_multiple ebx,12*iter+4
		shrd	ebx,eax,24
		movzx	eax,word [esi+FIXUPPIXEL(3+4*iter)]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		store_multiple ebx,12*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


;//============================================================
;//	16bpp to 24bpp blitters (2x scale)
;//============================================================

SNIPPET_BEGIN asmblit1_16_to_24_x2
	movzx	eax,word [esi+FIXUPPIXEL(0)]
	mov		eax,[ecx+eax*4]
	shrd	ebx,eax,24
	shrd	ebx,eax,8
	shr		eax,8
	store_multiple2 ebx,0,ax,4
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_24_x2
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+FIXUPPIXEL(0+2*iter)]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,12*iter
		shrd	ebx,eax,24
		movzx	eax,word [esi+FIXUPPIXEL(1+2*iter)]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,16
		store_multiple ebx,12*iter+4
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,12*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


;//============================================================
;//	16bpp to 24bpp blitters (3x scale)
;//============================================================

SNIPPET_BEGIN asmblit1_16_to_24_x3
	movzx	eax,word [esi+FIXUPPIXEL(0)]
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
		movzx	eax,word [esi+FIXUPPIXEL(0+4*iter)]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,36*iter
		shrd	ebx,eax,24
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+4
		shrd	ebx,eax,24
		movzx	eax,word [esi+FIXUPPIXEL(1+4*iter)]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+8
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,36*iter+12
		shrd	ebx,eax,24
		movzx	eax,word [esi+FIXUPPIXEL(2+4*iter)]
		mov		eax,[ecx+eax*4]
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+16
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+20
		movzx	eax,word [esi+FIXUPPIXEL(3+4*iter)]
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
;//	16bpp to 32bpp blitters (1x scale)
;//============================================================

SNIPPET_BEGIN asmblit1_16_to_32_x1
	movzx	eax,word [esi+FIXUPPIXEL(0)]
	mov		eax,[ecx+eax*4]
	store_multiple eax,0
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_32_x1
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+FIXUPPIXEL(0+2*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(1+2*iter)]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple2 eax,8*iter,ebx,8*iter+4
		%assign iter iter+1
	%endrep
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_32_x1_mmx
	%assign iter 0
	%rep 2
		movzx	eax,word [esi+FIXUPPIXEL(0+8*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(1+8*iter)]
		movzx	edx,word [esi+FIXUPPIXEL(2+8*iter)]
		movd	mm0,[ecx+eax*4]
		movd	mm1,[ecx+ebx*4]
		movd	mm2,[ecx+edx*4]

		movzx	eax,word [esi+FIXUPPIXEL(3+8*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(4+8*iter)]
		movzx	edx,word [esi+FIXUPPIXEL(5+8*iter)]
		movd	mm3,[ecx+eax*4]
		movd	mm4,[ecx+ebx*4]
		movd	mm5,[ecx+edx*4]

		movzx	eax,word [esi+FIXUPPIXEL(6+8*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(7+8*iter)]
		movd	mm6,[ecx+eax*4]
		movd	mm7,[ecx+ebx*4]

		punpckldq mm0,mm1
		punpckldq mm2,mm3
		punpckldq mm4,mm5
		punpckldq mm6,mm7

		store_multiple2 mm0,32*iter,mm2,32*iter+8
		store_multiple2 mm4,32*iter+16,mm6,32*iter+24

		%assign iter iter+1
	%endrep
SNIPPET_END


;//============================================================
;//	16bpp to 32bpp blitters (2x scale)
;//============================================================

SNIPPET_BEGIN asmblit1_16_to_32_x2
	movzx	eax,word [esi+FIXUPPIXEL(0)]
	mov		eax,[ecx+eax*4]
	store_multiple2 eax,0,eax,4
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_32_x2
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+FIXUPPIXEL(0+2*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(1+2*iter)]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple4 eax,16*iter,eax,16*iter+4,ebx,16*iter+8,ebx,16*iter+12
		%assign iter iter+1
	%endrep
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_32_x2_mmx
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+FIXUPPIXEL(0+2*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(1+2*iter)]
		movd	mm0,[ecx+eax*4]
		movd	mm1,[ecx+ebx*4]
		punpckldq mm0,mm0
		punpckldq mm1,mm1
		store_multiple2 mm0,16*iter,mm1,16*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


;//============================================================
;//	16bpp to 32bpp blitters (3x scale)
;//============================================================

SNIPPET_BEGIN asmblit1_16_to_32_x3
	movzx	eax,word [esi+FIXUPPIXEL(0)]
	mov		eax,[ecx+eax*4]
	store_multiple3 eax,0,eax,4,eax,8
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_32_x3
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+FIXUPPIXEL(0+2*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(1+2*iter)]
		mov		eax,[ecx+eax*4]
		mov		ebx,[ecx+ebx*4]
		store_multiple6 eax,24*iter,eax,24*iter+4,eax,24*iter+8,ebx,24*iter+12,ebx,24*iter+16,ebx,24*iter+20
		%assign iter iter+1
	%endrep
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_32_x3_mmx
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+FIXUPPIXEL(0+2*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(1+2*iter)]
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


;//============================================================
;//	16bpp to 32bpp blitters (RGB)
;//============================================================

SNIPPET_BEGIN asmblit1_16_to_32_rgb
	movzx	eax,word [esi+FIXUPPIXEL(0)]
	movd	mm0,[ecx+eax*4]
	punpckldq mm0,mm0

	mov		eax,[asmblit_srcheight]
	movq	mm2,mm0
	mov		ebx,FIXUPVALUE(FIXUPVAL_LASTXCOUNT)
	movq	mm6,mm0
	sub		ebx,ebp
	psrlq	mm0,2
	shl		ebx,3
	shl		eax,7
	and		ebx,63
	movq	mm4,mm0
	pand	mm0,[asmblit_rgbmask+eax+ebx+64]
	pand	mm4,[asmblit_rgbmask+eax+ebx]
	mov		ebx,[asmblit_dstpitch]
	psubd	mm2,mm0
	psubd	mm6,mm4
	movq	[edi],mm2
	movq	[edi+ebx],mm6
SNIPPET_END

SNIPPET_BEGIN asmblit16_16_to_32_rgb
	%assign iter 0
	%rep 8
		movzx	eax,word [esi+FIXUPPIXEL(0+2*iter)]
		movzx	ebx,word [esi+FIXUPPIXEL(1+2*iter)]
		movd	mm0,[ecx+eax*4]
		movd	mm1,[ecx+ebx*4]
		punpckldq mm0,mm0
		punpckldq mm1,mm1

		mov		eax,[asmblit_srcheight]
		movq	mm2,mm0
		movq	mm3,mm1
		mov		ebx,[asmblit_dstpitch]
		movq	mm6,mm0
		movq	mm7,mm1
		psrlq	mm0,2
		psrlq	mm1,2
		shl		eax,7
		movq	mm4,mm0
		movq	mm5,mm1
		pand	mm0,[asmblit_rgbmask+eax+((16*iter)&63)+64]
		pand	mm1,[asmblit_rgbmask+eax+((16*iter+8)&63)+64]
		pand	mm4,[asmblit_rgbmask+eax+((16*iter)&63)]
		pand	mm5,[asmblit_rgbmask+eax+((16*iter+8)&63)]
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
;//	32bpp to 16bpp blitters (1x scale)
;//============================================================

SNIPPET_BEGIN asmblit1_32_to_16_x1
	mov		eax,[esi+FIXUPPIXEL(0)]
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
		mov		eax,[esi+FIXUPPIXEL(iter)]
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


;//============================================================
;//	32bpp to 16bpp blitters (2x scale)
;//============================================================

SNIPPET_BEGIN asmblit1_32_to_16_x2
	mov		eax,[esi+FIXUPPIXEL(0)]
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
		mov		eax,[esi+FIXUPPIXEL(iter)]
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


;//============================================================
;//	32bpp to 16bpp blitters (3x scale)
;//============================================================

SNIPPET_BEGIN asmblit1_32_to_16_x3
	mov		eax,[esi+FIXUPPIXEL(0)]
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
		mov		eax,[esi+FIXUPPIXEL(iter)]
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
;//	32bpp to 24bpp blitters (1x scale)
;//============================================================

SNIPPET_BEGIN asmblit1_32_to_24_x1
	mov		eax,[esi+FIXUPPIXEL(0)]
	store_multiple ax,0
	shr		eax,16
	store_multiple al,2
SNIPPET_END

SNIPPET_BEGIN asmblit16_32_to_24_x1
	%assign iter 0
	%rep 4
		mov		eax,[esi+FIXUPPIXEL(0+4*iter)]
		shrd	ebx,eax,24
		mov		eax,[esi+FIXUPPIXEL(1+4*iter)]
		shrd	ebx,eax,8
		store_multiple ebx,12*iter
		shrd	ebx,eax,24
		mov		eax,[esi+FIXUPPIXEL(2+4*iter)]
		shrd	ebx,eax,16
		store_multiple ebx,12*iter+4
		shrd	ebx,eax,24
		mov		eax,[esi+FIXUPPIXEL(3+4*iter)]
		shrd	ebx,eax,24
		store_multiple ebx,12*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


;//============================================================
;//	32bpp to 24bpp blitters (2x scale)
;//============================================================

SNIPPET_BEGIN asmblit1_32_to_24_x2
	mov		eax,[esi+FIXUPPIXEL(0)]
	shrd	ebx,eax,24
	shrd	ebx,eax,8
	shr		eax,8
	store_multiple2 ebx,0,ax,4
SNIPPET_END

SNIPPET_BEGIN asmblit16_32_to_24_x2
	%assign iter 0
	%rep 8
		mov		eax,[esi+FIXUPPIXEL(0+2*iter)]
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,12*iter
		shrd	ebx,eax,24
		mov		eax,[esi+FIXUPPIXEL(1+2*iter)]
		shrd	ebx,eax,16
		store_multiple ebx,12*iter+4
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,12*iter+8
		%assign iter iter+1
	%endrep
SNIPPET_END


;//============================================================
;//	32bpp to 24bpp blitters (3x scale)
;//============================================================

SNIPPET_BEGIN asmblit1_32_to_24_x3
	mov		eax,[esi+FIXUPPIXEL(0)]
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
		mov		eax,[esi+FIXUPPIXEL(0+4*iter)]
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,36*iter
		shrd	ebx,eax,24
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+4
		shrd	ebx,eax,24
		mov		eax,[esi+FIXUPPIXEL(1+4*iter)]
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+8
		shrd	ebx,eax,24
		shrd	ebx,eax,8
		store_multiple ebx,36*iter+12
		shrd	ebx,eax,24
		mov		eax,[esi+FIXUPPIXEL(2+4*iter)]
		shrd	ebx,eax,16
		store_multiple ebx,36*iter+16
		shrd	ebx,eax,24
		shrd	ebx,eax,24
		store_multiple ebx,36*iter+20
		mov		eax,[esi+FIXUPPIXEL(3+4*iter)]
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
;//	32bpp to 32bpp blitters (1x scale)
;//============================================================

SNIPPET_BEGIN asmblit1_32_to_32_x1
	mov		eax,[esi+FIXUPPIXEL(0)]
	store_multiple eax,0
SNIPPET_END

SNIPPET_BEGIN asmblit16_32_to_32_x1
	%assign iter 0
	%rep 8
		mov		eax,[esi+FIXUPPIXEL(0+2*iter)]
		mov		ebx,[esi+FIXUPPIXEL(1+2*iter)]
		store_multiple2 eax,8*iter,ebx,8*iter+4
		%assign iter iter+1
	%endrep
SNIPPET_END


;//============================================================
;//	32bpp to 32bpp blitters (2x scale)
;//============================================================

SNIPPET_BEGIN asmblit1_32_to_32_x2
	mov		eax,[esi+FIXUPPIXEL(0)]
	store_multiple2 eax,0,eax,4
SNIPPET_END

SNIPPET_BEGIN asmblit16_32_to_32_x2
	%assign iter 0
	%rep 8
		mov		eax,[esi+FIXUPPIXEL(0+2*iter)]
		mov		ebx,[esi+FIXUPPIXEL(1+2*iter)]
		store_multiple4 eax,16*iter,eax,16*iter+4,ebx,16*iter+8,ebx,16*iter+12
		%assign iter iter+1
	%endrep
SNIPPET_END


;//============================================================
;//	32bpp to 32bpp blitters (3x scale)
;//============================================================

SNIPPET_BEGIN asmblit1_32_to_32_x3
	mov		eax,[esi+FIXUPPIXEL(0)]
	store_multiple3 eax,0,eax,4,eax,8
SNIPPET_END

SNIPPET_BEGIN asmblit16_32_to_32_x3
	%assign iter 0
	%rep 8
		mov		eax,[esi+FIXUPPIXEL(0+2*iter)]
		mov		ebx,[esi+FIXUPPIXEL(1+2*iter)]
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
	mov		esi,[asmblit_srcdata]
	mov		edi,[asmblit_dstdata]

	;// load the palette pointer
	mov		ecx,[asmblit_srclookup]
SNIPPET_END


;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_yloop_top
	push	esi
	push	edi
SNIPPET_END


;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_middlexloop_header
	;// determine the number of 16-byte chunks to blit
	mov		ebp,FIXUPVALUE(FIXUPVAL_MIDDLEXCOUNT)
SNIPPET_END


;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_middlexloop_top
	;// nothing to do here
SNIPPET_END


;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_middlexloop_bottom
	sub		ebp,1
	lea		esi,[esi+FIXUPVALUE(FIXUPVAL_SRCBYTES16)]
	lea		edi,[edi+FIXUPVALUE(FIXUPVAL_DSTBYTES16)]
	jne		near FIXUPADDRESS(FIXUPADDR_MIDDLEXTOP)
SNIPPET_END


;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_lastxloop_header
	mov		ebp,FIXUPVALUE(FIXUPVAL_LASTXCOUNT)
SNIPPET_END


;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_lastxloop_top
	;// nothing to do here
SNIPPET_END


;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_lastxloop_bottom
	sub		ebp,1
	lea		esi,[esi+FIXUPVALUE(FIXUPVAL_SRCBYTES1)]
	lea		edi,[edi+FIXUPVALUE(FIXUPVAL_DSTBYTES1)]
	jne		near FIXUPADDRESS(FIXUPADDR_LASTXTOP)
SNIPPET_END


;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_yloop_bottom
	sub		dword [asmblit_srcheight],1
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


;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_footer
	popad
	ret
SNIPPET_END



;//---------------------------------------------------------------

SNIPPET_BEGIN asmblit_prefetch_sse
	prefetchnta	[esi+FIXUPVALUE(FIXUPVAL_SRCPREFETCH16)]
SNIPPET_END



;//============================================================
;//	MMX/SSE/SSE2 detection
;//============================================================

CGLOBAL asmblit_cpuid_features
asmblit_cpuid_features:
	push	ebx
	push	ecx
	push	edx

	mov		eax,0

	;// attempt to change the ID flag
	pushfd
	pop		edx
	xor		edx,1<<21
	push	edx
	popfd

	;// if we can't, they definitely don't have any of these things
	pushfd
	pop		ebx
	xor		edx,ebx
	test	edx,1<<21
	jnz		.Return

	;// use CPUID
	mov		eax,1
	cpuid
	mov		eax, edx

.Return:
	pop		edx
	pop		ecx
	pop		ebx
	ret
