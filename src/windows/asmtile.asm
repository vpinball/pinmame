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
;//	LOCAL VARIABLES
;//============================================================

[SECTION .data]
mmx_end_mask:
	db	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
	db	0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00
	db	0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00
	db	0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00
	db	0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00
	db	0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00
	db	0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00
	db	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00
	db	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff

mmx_8to64_map:
	%assign iter 0
	%rep 256
		db	iter,iter,iter,iter,iter,iter,iter,iter
		%assign iter iter+1
	%endrep


;//============================================================
;//	MMX cleanup
;//============================================================

CGLOBAL osd_pend
osd_pend:
	emms
	ret



;//============================================================
;//	16-bit opaque blitter
;//============================================================

;// parameters:
;//		esp		(pushad)
;//		esp+32	(return addr)
;//		esp+36	UINT16 *		dest
;//		esp+40	const UINT16 *	source
;//		esp+44	int				count
;//		esp+48	UINT8 *			pri
;//		esp+52	UINT32 			pcode

CGLOBAL osd_pdo16
osd_pdo16:

	pushad

	movzx	eax,byte [esp+52]
	mov		edx,[esp+44]
	movq	mm5,[mmx_8to64_map + eax*8]	;// mm5 = expanded code
	mov		ecx,edx
	shr		edx,3						;// edx = counter / 8

	mov		esi,[esp+40]				;// esi = source
	mov		edi,[esp+36]				;// edi = dest
	mov		ebx,[esp+48]				;// ebx = pri
	jz		.loop1end					;// skip if nothing to do

	align	4
.loop1:
	movq	mm0,[ebx]					;// mm1 = priority
	movq	mm1,[esi]					;// mm0 = source
	movq	mm2,[esi + 8]				;// mm0 = source
	por		mm0,mm5						;// or in priority
	movq	[edi],mm1					;// write dest
	movq	[edi + 8],mm2				;// write dest
	movq	[ebx],mm0					;// write priority

	add		ebx,8
	add		esi,16
	add		edi,16
	dec		edx
	jnz		.loop1
.loop1end:

	and		ecx,7						;// one more to finish?
	jz		.nomore						;// skip if not
	movq	mm7,[mmx_end_mask + ecx*8]	;// load end of line mask
	pand	mm5,mm7						;// and the priority data with it

	movq	mm6,mm7						;// mm6 = copy of byte mask
	movq	mm1,[esi]					;// mm1 = source
	movq	mm2,[edi]					;// mm2 = dest
	punpcklbw mm6,mm6					;// mm6 = pixel-doubled mask (now word mask)
	movq	mm0,[ebx]					;// mm0 = priority
	pand	mm1,mm6						;// mm1 = source & mask
	pandn	mm6,mm2						;// mm6 = dest & ~mask
	por		mm1,mm6						;// mm1 = (source & mask) | (dest & ~mask)
	movq	[edi],mm1					;// write dest

	por		mm0,mm5						;// or in priority
	movq	mm1,[esi + 8]				;// mm1 = source
	movq	mm2,[edi + 8]				;// mm2 = dest
	punpckhbw mm7,mm7					;// mm7 = pixel-doubled mask (now word mask)
	movq	[ebx],mm0					;// write priority
	pand	mm1,mm7						;// mm1 = source & mask
	pandn	mm7,mm2						;// mm7 = dest & ~mask
	por		mm1,mm7						;// mm1 = (source & mask) | (dest & ~mask)
	movq	[edi + 8],mm1				;// write dest

.nomore:
	popad
	ret


;//============================================================
;//	16-bit transparent blitter
;//============================================================

;// parameters:
;//		esp		(pushad)
;//		esp+32	(return addr)
;//		esp+36	UINT16 *		dest
;//		esp+40	const UINT16 *	source
;//		esp+44	const UINT8 *	pMask
;//		esp+48	int				mask
;//		esp+52	int				value
;//		esp+56	int				count
;//		esp+60	UINT8 *			pri
;//		esp+64	UINT32 			pcode

CGLOBAL osd_pdt16
osd_pdt16:

	pushad

	movzx	eax,byte [esp+64]
	movzx	ebx,byte [esp+48]
	movzx	ecx,byte [esp+52]
	mov		edx,[esp+56]
	movq	mm5,[mmx_8to64_map + eax*8]	;// mm5 = expanded code
	movq	mm6,[mmx_8to64_map + ebx*8]	;// mm6 = expanded mask
	movq	mm7,[mmx_8to64_map + ecx*8]	;// mm7 = expanded value
	mov		ecx,edx
	shr		edx,3						;// edx = counter / 8

	mov		esi,[esp+40]				;// esi = source
	mov		edi,[esp+36]				;// edi = dest
	mov		ebp,[esp+44]				;// ebp = pMask
	mov		ebx,[esp+60]				;// ebx = pri
	jz		.loop1end					;// skip if nothing to do

	align	4
.loop1:
	movq	mm0,[ebp]					;// mm0 = *pMask
	pand	mm0,mm6						;// mm0 = *pMask & mask
	pcmpeqb	mm0,mm7						;// mm0 = byte mask where (*pMask & mask) == value

.loop1alt:
	movq	mm1,mm0						;// mm1 = byte mask
	movq	mm3,[esi]					;// mm3 = source 16-bit data
	punpcklbw mm1,mm1					;// mm1 = pixel-doubled mask (now word mask)
	movq	mm2,[edi]					;// mm2 = destination 16-bit data
	pand	mm3,mm1						;// mm3 = source & word mask
	movq	mm4,mm0						;// mm4 = byte mask
	pandn	mm1,mm2						;// mm1 = dest & ~word mask
	por		mm1,mm3						;// mm1 = (source & mask) | (dest & ~mask)
	movq	[edi],mm1					;// write destination data

	movq	mm1,mm0						;// mm1 = byte mask
	movq	mm3,[esi + 8]				;// mm3 = source 16-bit data
	punpckhbw mm1,mm1					;// mm1 = pixel-doubled mask (now word mask)
	movq	mm2,[edi + 8]				;// mm2 = destination 16-bit data
	pand	mm3,mm1						;// mm3 = source & word mask
	pandn	mm1,mm2						;// mm1 = dest & ~word mask
	pand	mm0,mm5						;// mm0 = original byte mask & expanded priority code
	por		mm1,mm3						;// mm1 = (source & mask) | (dest & ~mask)
	por		mm0,[ebx]					;// mm0 = priority | masked priority data
	movq	[edi + 8],mm1				;// write destination data
	movq	[ebx],mm0					;// write back priority data

	add		ebx,8
	add		ebp,8
	add		esi,16
	add		edi,16
	dec		edx
	jnz		.loop1
.loop1end:

	and		ecx,7						;// one more to finish?
	jz		.nomore						;// skip if not
	movq	mm0,[ebp]					;// mm0 = *pMask
	inc		edx							;// set edx for one more loop
	pand	mm0,mm6						;// mm0 = *pMask & mask
	pcmpeqb	mm0,mm7						;// mm0 = byte mask where (*pMask & mask) == value
	pand	mm0,[mmx_end_mask + ecx*8]	;// and with the end-of-row mask
	mov		ecx,0						;// clear ecx
	jmp		.loop1alt					;// do one more rep

.nomore:
	popad
	ret


;//============================================================
;//	16-bit transparent blitter, no priority
;//============================================================

;// parameters:
;//		esp		(pushad)
;//		esp+32	(return addr)
;//		esp+36	UINT16 *		dest
;//		esp+40	const UINT16 *	source
;//		esp+44	const UINT8 *	pMask
;//		esp+48	int				mask
;//		esp+52	int				value
;//		esp+56	int				count
;//		esp+60	UINT8 *			pri
;//		esp+64	UINT32 			pcode

CGLOBAL osd_pdt16np
osd_pdt16np:

	pushad

	movzx	ebx,byte [esp+48]
	movzx	ecx,byte [esp+52]
	mov		edx,[esp+56]
	movq	mm6,[mmx_8to64_map + ebx*8]	;// mm6 = expanded mask
	movq	mm7,[mmx_8to64_map + ecx*8]	;// mm7 = expanded value
	mov		ecx,edx
	shr		edx,3						;// edx = counter / 8

	mov		esi,[esp+40]				;// esi = source
	mov		edi,[esp+36]				;// edi = dest
	mov		ebp,[esp+44]				;// ebp = pMask
	jz		.loop1end					;// skip if nothing to do

	align	4
.loop1:
	movq	mm0,[ebp]					;// mm0 = *pMask
	pand	mm0,mm6						;// mm0 = *pMask & mask
	pcmpeqb	mm0,mm7						;// mm0 = byte mask where (*pMask & mask) == value

.loop1alt:
	movq	mm1,mm0						;// mm1 = byte mask
	movq	mm3,[esi]					;// mm3 = source 16-bit data
	punpcklbw mm1,mm1					;// mm1 = pixel-doubled mask (now word mask)
	movq	mm2,[edi]					;// mm2 = destination 16-bit data
	pand	mm3,mm1						;// mm3 = source & word mask
	movq	mm4,mm0						;// mm4 = byte mask
	pandn	mm1,mm2						;// mm1 = dest & ~word mask
	por		mm1,mm3						;// mm1 = (source & mask) | (dest & ~mask)
	movq	[edi],mm1					;// write destination data

	movq	mm3,[esi + 8]				;// mm3 = source 16-bit data
	punpckhbw mm0,mm0					;// mm1 = pixel-doubled mask (now word mask)
	movq	mm2,[edi + 8]				;// mm2 = destination 16-bit data
	pand	mm3,mm0						;// mm3 = source & word mask
	pandn	mm0,mm2						;// mm1 = dest & ~word mask
	por		mm0,mm3						;// mm1 = (source & mask) | (dest & ~mask)
	movq	[edi + 8],mm0				;// write destination data

	add		ebp,8
	add		esi,16
	add		edi,16
	dec		edx
	jnz		.loop1
.loop1end:

	and		ecx,7						;// one more to finish?
	jz		.nomore						;// skip if not
	movq	mm0,[ebp]					;// mm0 = *pMask
	inc		edx							;// set edx for one more loop
	pand	mm0,mm6						;// mm0 = *pMask & mask
	pcmpeqb	mm0,mm7						;// mm0 = byte mask where (*pMask & mask) == value
	pand	mm0,[mmx_end_mask + ecx*8]	;// and with the end-of-row mask
	mov		ecx,0						;// clear ecx
	jmp		.loop1alt					;// do one more rep

.nomore:
	popad
	ret
