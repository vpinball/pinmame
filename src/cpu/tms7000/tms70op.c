/*****************************************************************************
 *
 *	 tms70op.c (Op code functions)
 *	 Portable TMS7000 emulator (Texas Instruments 7000)
 *
 *	 Copyright (c) 2001 tim lindner, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   tlindner@ix.netcom.com
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/

//SJE: Changed all references to ICount to icount (to match MAME requirements)

#include "cpuexec.h"

void illegal( void );
void illegal( void )
{
	/* This is a guess */
	tms7000_icount -= 4;
}

void adc_imp( void );
void adc_imp( void )
{
	UINT16	t;
	
	t = RDA + RDB + GET_C;
	WRA(t);
	
	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 5;
}

void adc_r2a( void );
void adc_r2a( void )
{
	UINT16	t;
	UINT8	v;
	
	IMMBYTE(v);
	
	t = RM(v) + RDA + GET_C;
	WRA(t);
	
	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 8;
}

void adc_r2b( void );
void adc_r2b( void )
{
	UINT16	t;
	UINT8	v;
	
	IMMBYTE(v);
	
	t = RM(v) + RDB + GET_C;
	WRB(t);
	
	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 8;
}

void adc_r2r( void );
void adc_r2r( void )
{
	UINT16	t;
	UINT8	i,j;
	
	IMMBYTE(i);
	IMMBYTE(j);
	
	t = RM(i)+RM(j) + GET_C;
	WM(j,t);
	
	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 10;
}

void adc_i2a( void );
void adc_i2a( void )
{
	UINT16	t;
	UINT8	v;
	
	IMMBYTE(v);
	
	t = v + RDA + GET_C;
	WRA(t);
	
	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 7;
}

void adc_i2b( void );
void adc_i2b( void )
{
	UINT16	t;
	UINT8	v;
	
	IMMBYTE(v);
	
	t = v + RDB + GET_C;
	WRB(t);
	
	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 7;
}

void adc_i2r( void );
void adc_i2r( void )
{
	UINT16	t;
	UINT8	i,j;
	
	IMMBYTE(i);
	IMMBYTE(j);
	
	t = i+RM(j) + GET_C;
	WM(j,t);
	
	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 9;
}

void add_imp( void );
void add_imp( void )
{
	UINT16	t;
	
	t = RDA + RDB;
	WRA(t);
	
	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 5;
}

void add_r2a( void );
void add_r2a( void )
{
	UINT16	t;
	UINT8	v;
	
	IMMBYTE(v);
	
	t = RM(v) + RDA;
	WRA(t);
	
	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 8;
}

void add_r2b( void );
void add_r2b( void )
{
	UINT16	t;
	UINT8	v;
	
	IMMBYTE(v);
	
	t = RM(v) + RDB;
	WRB(t);
	
	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 8;
}

void add_r2r( void );
void add_r2r( void )
{
	UINT16	t;
	UINT8	i,j;
	
	IMMBYTE(i);
	IMMBYTE(j);
	
	t = RM(i)+RM(j);
	WM(j,t);
	
	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 10;
}

void add_i2a( void );
void add_i2a( void )
{
	UINT16	t;
	UINT8	v;
	
	IMMBYTE(v);
	
	t = v + RDA;
	WRA(t);
	
	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 7;
}

void add_i2b( void );
void add_i2b( void )
{
	UINT16	t;
	UINT8	v;
	
	IMMBYTE(v);
	
	t = v + RDB;
	WRB(t);
	
	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 7;
}

void add_i2r( void );
void add_i2r( void )
{
	UINT16	t;
	UINT8	i,j;
	
	IMMBYTE(i);
	IMMBYTE(j);
	
	t = i+RM(j);
	WM(j,t);
	
	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 9;
}

void and_imp( void );
void and_imp( void )
{
	UINT8	t;
	
	t = RDA & RDB;
	WRA(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 5;
}

void and_r2a( void );
void and_r2a( void )
{
	UINT8	t;
	UINT8	v;
	
	IMMBYTE(v);
	
	t = RM(v) & RDA;
	WRA(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 8;
}

void and_r2b( void );
void and_r2b( void )
{
	UINT8	t;
	UINT8	v;
	
	IMMBYTE(v);
	
	t = RM(v) & RDB;
	WRB(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 8;
}

void and_r2r( void );
void and_r2r( void )
{
	UINT8	t;
	UINT8	i,j;
	
	IMMBYTE(i);
	IMMBYTE(j);
	
	t = RM(i) & RM(j);
	WM(j,t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 10;
}

void and_i2a( void );
void and_i2a( void )
{
	UINT8	t;
	UINT8	v;
	
	IMMBYTE(v);
	
	t = v & RDA;
	WRA(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 7;
}

void and_i2b( void );
void and_i2b( void )
{
	UINT8	t;
	UINT8	v;
	
	IMMBYTE(v);
	
	t = v & RDB;
	WRB(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 7;
}

void and_i2r( void );
void and_i2r( void )
{
	UINT8	t;
	UINT8	i,j;
	
	IMMBYTE(i);
	IMMBYTE(j);
	
	t = i & RM(j);
	WM(j,t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 9;
}

void andp_a2p( void );
void andp_a2p( void )
{
	UINT8	t;
	UINT8	v;
	
	IMMBYTE(v);
	t = RDA & RM( 0x0100 + v);
	WM( 0x0100+v, t);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 10;
}	

void andp_b2p( void );
void andp_b2p( void )
{
	UINT8	t;
	UINT8	v;
	
	IMMBYTE(v);
	t = RDB & RM( 0x0100 + v);
	WM( 0x0100+v, t);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 9;
}	


void movp_i2p( void );
void movp_i2p( void )
{
	UINT8	i,v;
	
	IMMBYTE(i);
	IMMBYTE(v);
	WM( 0x0100+v, i);

	CLR_NZC;
	SET_N8(i);
	SET_Z8(i);
	
	tms7000_icount -= 11;
}	

void andp_i2p( void );
void andp_i2p( void )
{
	UINT8	t;
	UINT8	i,v;
	
	IMMBYTE(i);
	IMMBYTE(v);
	t = i & RM( 0x0100 + v);
	WM( 0x0100+v, t);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 11;
}	

void br_dir( void );
void br_dir( void )
{
	PAIR p;
	
	IMMWORD( p );
	pPC = p.d;
	CHANGE_PC;
	tms7000_icount -= 10;
}

void br_ind( void );
void br_ind( void )
{
	UINT8	v;
	
	IMMBYTE( v );
	PC.w.l = RRF16(v);
	
	tms7000_icount -= 9;
}

void br_inx( void );
void br_inx( void )
{
	PAIR p;
	
	IMMWORD( p );
	pPC = p.w.l + RDB;
	CHANGE_PC;
	tms7000_icount -= 12;
}

void btjo_imp( void );
void btjo_imp( void )
{
	UINT8	t;
		
	t = RDB & RDA;

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if(t != 0)
	{
		INT8	j;
		
		SIMMBYTE( j );
		pPC += j;
		tms7000_icount -= 9;
	}
	else
	{
		pPC++;
		tms7000_icount -= 7;
	}
	CHANGE_PC;
}

void btjo_r2a( void );
void btjo_r2a( void )
{
	UINT8	t,r;
	
	IMMBYTE( r );
	t = RM( r ) & RDA;

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if(t != 0)
	{
		INT8	j;
		
		SIMMBYTE( j );
		pPC += j;
		tms7000_icount -= 9;
	}
	else
	{
		pPC++;
		tms7000_icount -= 7;
	}
	CHANGE_PC;
}

void btjo_r2b( void );
void btjo_r2b( void )
{
	UINT8	t,r;
	
	IMMBYTE(r);
	t = RM(r) & RDB;

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if(t != 0)
	{
		INT8	j;
		
		SIMMBYTE(j);
		pPC += j;
		tms7000_icount -= 12;
	}
	else
	{
		pPC++;
		tms7000_icount -= 10;
	}
	CHANGE_PC;
}

void btjo_r2r( void );
void btjo_r2r( void )
{
	UINT8	t,r,s;
	
	IMMBYTE(r);
	IMMBYTE(s);
	t = RM(r) & RM(s);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if(t != 0)
	{
		INT8	j;
		
		SIMMBYTE(j);
		pPC += j;
		tms7000_icount -= 14;
	}
	else
	{
		pPC++;
		tms7000_icount -= 12;
	}
	CHANGE_PC;
}

void btjo_i2a( void );
void btjo_i2a( void )
{
	UINT8	t,r;
	
	IMMBYTE(r);
	t = r & RDA;

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if(t != 0)
	{
		INT8	j;
		
		SIMMBYTE(j);
		pPC += j;
		tms7000_icount -= 11;
	}
	else
	{
		pPC++;
		tms7000_icount -= 9;
	}
	CHANGE_PC;
}

void btjo_i2b( void );
void btjo_i2b( void )
{
	UINT8	t,i;
	
	IMMBYTE(i);
	t = i & RDB;

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if(t != 0)
	{
		INT8	j;
		
		SIMMBYTE(j);
		pPC += j;
		tms7000_icount -= 11;
	}
	else
	{
		pPC++;
		tms7000_icount -= 9;
	}
	CHANGE_PC;
}

void btjo_i2r( void );
void btjo_i2r( void )
{
	UINT8	t,i,r;
	
	IMMBYTE(i);
	IMMBYTE(r);
	t = i & RM(r);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if(t != 0)
	{
		INT8	j;
		
		SIMMBYTE(j);
		pPC += j;
		tms7000_icount -= 13;
	}
	else
	{
		pPC++;
		tms7000_icount -= 11;
	}
	CHANGE_PC;
}

void btjop_a( void );
void btjop_a( void )
{
	UINT8	t,p;
	
	IMMBYTE(p);
	
	t = RM(0x100+p) & RDA;
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if(t != 0)
	{
		INT8	j;
		
		SIMMBYTE(j);
		pPC += j;
		tms7000_icount -= 13;
	}
	else
	{
		pPC++;
		tms7000_icount -= 11;
	}
	CHANGE_PC;
}
	
void btjop_b( void );
void btjop_b( void )
{
	UINT8	t,p;
	
	IMMBYTE(p);
	
	t = RM(0x100+p) & RDB;
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if(t != 0)
	{
		INT8	j;
		
		SIMMBYTE(j);
		pPC += j;
		tms7000_icount -= 12;
	}
	else
	{
		pPC++;
		tms7000_icount -= 10;
	}
	CHANGE_PC;
}
	
void btjop_im( void );
void btjop_im( void )
{
	UINT8	t,p,i;
	
	IMMBYTE(i);
	IMMBYTE(p);
	
	t = RM(0x100+p) & i;
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if(t != 0)
	{
		INT8	j;
		
		SIMMBYTE(j);
		pPC += j;
		tms7000_icount -= 14;
	}
	else
	{
		pPC++;
		tms7000_icount -= 12;
	}
	CHANGE_PC;
}
	
void btjz_imp( void );
void btjz_imp( void )
{
	UINT8	t;

	t = RDB & ~RDA;

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if(t != 0)
	{
		INT8	j;
		
		SIMMBYTE( j );
		pPC += j;
		tms7000_icount -= 9;
	}
	else
	{
		pPC++;
		tms7000_icount -= 7;
	}
	CHANGE_PC;
}

void btjz_r2a( void );
void btjz_r2a( void )
{
	UINT8	t,r;
	
	IMMBYTE( r );
	t = RM( r ) & ~RDA;

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if(t != 0)
	{
		INT8	j;
		
		SIMMBYTE( j );
		pPC += j;
		tms7000_icount -= 9;
	}
	else
	{
		pPC++;
		tms7000_icount -= 7;
	}
	CHANGE_PC;
}

void btjz_r2b( void );
void btjz_r2b( void )
{
	UINT8	t,r;
	
	IMMBYTE(r);
	t = RM(r) & ~RDB;

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if(t != 0)
	{
		INT8	j;
		
		SIMMBYTE(j);
		pPC += j;
		tms7000_icount -= 12;
	}
	else
	{
		pPC++;
		tms7000_icount -= 10;
	}
	CHANGE_PC;
}

void btjz_r2r( void );
void btjz_r2r( void )
{
	UINT8	t,r,s;
	
	IMMBYTE(r);
	IMMBYTE(s);
	t = RM(r) & ~RM(s);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if(t != 0)
	{
		INT8	j;
		
		SIMMBYTE(j);
		pPC += j;
		tms7000_icount -= 14;
	}
	else
	{
		pPC++;
		tms7000_icount -= 12;
	}
	CHANGE_PC;
}

void btjz_i2a( void );
void btjz_i2a( void )
{
	UINT8	t,r;
	
	IMMBYTE(r);
	t = r & ~RDA;

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if(t != 0)
	{
		INT8	j;
		
		SIMMBYTE(j);
		pPC += j;
		tms7000_icount -= 11;
	}
	else
	{
		pPC++;
		tms7000_icount -= 9;
	}
	CHANGE_PC;
}

void btjz_i2b( void );
void btjz_i2b( void )
{
	UINT8	t,i;
	
	IMMBYTE(i);
	t = i & ~RDB;

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if(t != 0)
	{
		INT8	j;
		
		SIMMBYTE(j);
		pPC += j;
		tms7000_icount -= 11;
	}
	else
	{
		pPC++;
		tms7000_icount -= 9;
	}
	CHANGE_PC;
}

void btjz_i2r( void );
void btjz_i2r( void )
{
	UINT8	t,i,r;
	
	IMMBYTE(i);
	IMMBYTE(r);
	t = i & ~RM(r);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if(t != 0)
	{
		INT8	j;
		
		SIMMBYTE(j);
		pPC += j;
		tms7000_icount -= 13;
	}
	else
	{
		pPC++;
		tms7000_icount -= 11;
	}
	CHANGE_PC;
}

void btjzp_a( void );
void btjzp_a( void )
{
	UINT8	t,p;
	
	IMMBYTE(p);
	
	t = RM(0x100+p) & ~RDA;
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if(t != 0)
	{
		INT8	j;
		
		SIMMBYTE(j);
		pPC += j;
		tms7000_icount -= 13;
	}
	else
	{
		pPC++;
		tms7000_icount -= 11;
	}
	CHANGE_PC;
}
	
void btjzp_b( void );
void btjzp_b( void )
{
	UINT8	t,p;
	
	IMMBYTE(p);
	
	t = RM(0x100+p) & ~RDB;
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if(t != 0)
	{
		INT8	j;
		
		SIMMBYTE(j);
		pPC += j;
		tms7000_icount -= 12;
	}
	else
	{
		pPC++;
		tms7000_icount -= 10;
	}
	CHANGE_PC;
}
	
void btjzp_im( void );
void btjzp_im( void )
{
	UINT8	t,p,i;
	
	IMMBYTE(i);
	IMMBYTE(p);
	
	t = RM(0x100+p) & ~i;
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if(t != 0)
	{
		INT8	j;
		
		SIMMBYTE(j);
		pPC += j;
		tms7000_icount -= 14;
	}
	else
	{
		pPC++;
		tms7000_icount -= 12;
	}
	CHANGE_PC;
}
	
void call_dir( void );
void call_dir( void )
{
	PAIR	tPC;
	
	IMMWORD( tPC );
	PUSHWORD( PC );
	pPC = tPC.d;
	CHANGE_PC;
	
	tms7000_icount -= 14;
}

void call_ind( void );
void call_ind( void )
{
	UINT8	v;
	
	IMMBYTE( v );
	PUSHWORD( PC );
	PC.w.l = RRF16(v);
	
	tms7000_icount -= 13;
}

void call_inx( void );
void call_inx( void )
{
	PAIR	tPC;
	
	IMMWORD( tPC );
	PUSHWORD( PC );
	pPC = tPC.w.l + RDB;
	CHANGE_PC;
	tms7000_icount -= 16;
}

void clr_a( void );
void clr_a( void )
{
	WRA(0);
	tms7000_icount -= 5;
}

void clr_b( void );
void clr_b( void )
{
	WRB(0);
	tms7000_icount -= 5;
}

void clr_r( void );
void clr_r( void )
{
	UINT8	r;
	
	IMMBYTE(r);
	WM(r,0);
	tms7000_icount -= 7;
}

void clrc( void );
void clrc( void )
{
	UINT8	a;
	
	a = RDA;

	CLR_NZC;
	SET_N8(a);
	SET_Z8(a);
	
	tms7000_icount -= 6;
}

void cmp_imp( void );
void cmp_imp( void )
{
	UINT16 t;
	
	t = RDA - RDB;
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	if( t==0 )
		SETC;
	else
		SET_C8( ~t );
	
	tms7000_icount -= 5;	
}	

void cmp_ra( void );
void cmp_ra( void )
{
	UINT16	t;
	UINT8	r;
	
	IMMBYTE(r);
	t = RDA - RM(r);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	if( t==0 )
		SETC;
	else
		SET_C8( ~t );
	
	tms7000_icount -= 8;	
}	

void cmp_rb( void );
void cmp_rb( void )
{
	UINT16	t;
	UINT8	r;
	
	IMMBYTE(r);
	t = RDB - RM(r);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	if( t==0 )
		SETC;
	else
		SET_C8( ~t );
	
	tms7000_icount -= 8;	
}	

void cmp_rr( void );
void cmp_rr( void )
{
	UINT16	t;
	UINT8	r,s;
	
	IMMBYTE(r);
	IMMBYTE(s);
	t = RM(s) - RM(r);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	if( t==0 )
		SETC;
	else
		SET_C8( ~t );
	
	tms7000_icount -= 10;	
}	

void cmp_ia( void );
void cmp_ia( void )
{
	UINT16	t;
	UINT8	i;
	
	IMMBYTE(i);
	t = RDA - i;
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	if( t==0 )
		SETC;
	else
		SET_C8( ~t );
	
	tms7000_icount -= 7;	
}	

void cmp_ib( void );
void cmp_ib( void )
{
	UINT16	t;
	UINT8	i;
	
	IMMBYTE(i);
	t = RDB - i;
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	if( t==0 )
		SETC;
	else
		SET_C8( ~t );
	
	tms7000_icount -= 7;	
}	

void cmp_ir( void );
void cmp_ir( void )
{
	UINT16	t;
	UINT8	i,r;
	
	IMMBYTE(i);
	IMMBYTE(r);
	t = RM(r) - i;
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	if( t==0 )
		SETC;
	else
		SET_C8( ~t );
	
	tms7000_icount -= 9;	
}	

void cmpa_dir( void );
void cmpa_dir( void )
{
	UINT16	t;
	PAIR	i;
	
	IMMWORD( i );
	t = RDA - RM(i.w.l);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	if( t==0 )
		SETC;
	else
		SET_C8( ~t );
	
	tms7000_icount -= 12;	
}	

void cmpa_ind( void );
void cmpa_ind( void )
{
	UINT16	t;
	PAIR	p;
	INT8	i;
	
	IMMBYTE(i);
	p.w.l = RRF16(i);
	t = RDA - RM(p.w.l);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	if( t==0 )
		SETC;
	else
		SET_C8( ~t );
	
	tms7000_icount -= 11;	
}	

void cmpa_inx( void );
void cmpa_inx( void )
{
	UINT16	t;
	PAIR	i;
	
	IMMWORD( i );
	t = RDA - RM(i.w.l + RDB);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	if( t==0 )
		SETC;
	else
		SET_C8( ~t );
	
	tms7000_icount -= 14;
}	

void dac_b2a( void );
void dac_b2a( void )
{
	UINT16	t;
	
	t = bcd_add( RDA, RDB );
	
	if (pSR & SR_C)
		t = bcd_add( t, 1 );

	WRA(t);

	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 7;
}
	
void dac_r2a( void );
void dac_r2a( void )
{
	UINT8	r;
	UINT16	t;
	
	IMMBYTE(r);
	
	t = bcd_add( RDA, RM(r) );
	
	if (pSR & SR_C)
		t = bcd_add( t, 1 );

	WRA(t);

	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 10;
}
	
void dac_r2b( void );
void dac_r2b( void )
{
	UINT8	r;
	UINT16	t;
	
	IMMBYTE(r);
	
	t = bcd_add( RDB, RM(r) );
	
	if (pSR & SR_C)
		t = bcd_add( t, 1 );

	WRB(t);

	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 10;
}
	
void dac_r2r( void );
void dac_r2r( void )
{
	UINT8	r,s;
	UINT16	t;
	
	IMMBYTE(s);
	IMMBYTE(r);
	
	t = bcd_add( RM(s), RM(r) );
	
	if (pSR & SR_C)
		t = bcd_add( t, 1 );

	WM(r,t);

	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 12;
}
	
void dac_i2a( void );
void dac_i2a( void )
{
	UINT8	i;
	UINT16	t;
	
	IMMBYTE(i);
	
	t = bcd_add( i, RDA );
	
	if (pSR & SR_C)
		t = bcd_add( t, 1 );

	WRA(t);

	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 9;
}
	
void dac_i2b( void );
void dac_i2b( void )
{
	UINT8	i;
	UINT16	t;
	
	IMMBYTE(i);
	
	t = bcd_add( i, RDB );
	
	if (pSR & SR_C)
		t = bcd_add( t, 1 );

	WRB(t);

	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 9;
}
	
void dac_i2r( void );
void dac_i2r( void )
{
	UINT8	i,r;
	UINT16	t;
	
	IMMBYTE(i);
	IMMBYTE(r);
	
	t = bcd_add( i, RM(r) );
	
	if (pSR & SR_C)
		t = bcd_add( t, 1 );

	WM(r,t);

	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 11;
}
	
void dec_a( void );
void dec_a( void )
{
	UINT16 t;
	
	t = RDA - 1;
	
	WRA( t );
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	SET_C8(~t);

	tms7000_icount -= 5;
}

void dec_b( void );
void dec_b( void )
{
	UINT16 t;
	
	t = RDB - 1;
	
	WRB( t );
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	SET_C8(~t);

	tms7000_icount -= 5;
}

void dec_r( void );
void dec_r( void )
{
	UINT16	t;
	UINT8	r;
	
	IMMBYTE(r);
	
	t = RM(r) - 1;
	
	WM( r, t );
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	SET_C8(~t);

	tms7000_icount -= 7;
}

void decd_a( void );
void decd_a( void )
{
	/* Not implemented */
	tms7000_icount -= 9;
}

void decd_b( void );
void decd_b( void )
{
	/* Not implemented */
	tms7000_icount -= 9;
}

void decd_r( void );
void decd_r( void )
{
	UINT8	r;
	PAIR	t;
	
	IMMBYTE(r);
	t.w.l = RRF16(r);
	t.d -= 1;
	WRF16(r,t);

	CLR_NZC;
	SET_N8(t.b.h);
	SET_Z8(t.b.h);

	SET_C16(~(t.d));

	tms7000_icount -= 11;
}

void dint( void );
void dint( void )
{
	CLR_NZCI;
	tms7000_icount -= 5;
}

void djnz_a( void );
void djnz_a( void )
{
	UINT16 t;
	
	t = RDA - 1;
	
	WRA( t );
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if( t != 0 )
	{
		INT8	s;
		
		SIMMBYTE(s);
		pPC += s;
		tms7000_icount -= 7;
	}
	else
	{
		pPC++;
		tms7000_icount -= 2;
	}
	CHANGE_PC;
}

void djnz_b( void );
void djnz_b( void )
{
	UINT16 t;
	
	t = RDB - 1;
	
	WRB( t );
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if( t != 0 )
	{
		INT8	s;
		
		SIMMBYTE(s);
		pPC += s;
		tms7000_icount -= 7;
	}
	else
	{
		pPC++;
		tms7000_icount -= 2;
	}
	CHANGE_PC;
}

void djnz_r( void );
void djnz_r( void )
{
	UINT16	t;
	UINT8	r;
	
	IMMBYTE(r);
	
	t = RM(r) - 1;
	
	WM(r,t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	if( t != 0 )
	{
		INT8	s;
		
		SIMMBYTE(s);
		pPC += s;
		tms7000_icount -= 9;
	}
	else
	{
		pPC++;
		tms7000_icount -= 3;
	}
	CHANGE_PC;
}

void dsb_b2a( void );
void dsb_b2a( void )
{
	UINT16	t;

	t = bcd_sub( RDA, RDB );
	
	if( !(pSR & SR_C) )
		t = bcd_sub( t, 1 );

	WRA(t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 7;
}
	
void dsb_r2a( void );
void dsb_r2a( void )
{
	UINT8	r;
	UINT16	t;
	
	IMMBYTE(r);

	t = bcd_sub( RDA, RM(r) );
	
	if( !(pSR & SR_C) )
		t = bcd_sub( t, 1 );

	WRA(t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 10;
}
	
void dsb_r2b( void );
void dsb_r2b( void )
{
	UINT8	r;
	UINT16	t;
	
	IMMBYTE(r);

	t = bcd_sub( RDB, RM(r) );
	
	if( !(pSR & SR_C) )
		t = bcd_sub( t, 1 );

	WRB(t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 10;
}
	
void dsb_r2r( void );
void dsb_r2r( void )
{
	UINT8	r,s;
	UINT16	t;

	IMMBYTE(s);
	IMMBYTE(r);

	t = bcd_sub( RM(s), RM(r) );
	
	if( !(pSR & SR_C) )
		t = bcd_sub( t, 1 );

	WM(r,t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 12;
}
	
void dsb_i2a( void );
void dsb_i2a( void )
{
	UINT8	i;
	UINT16	t;
	
	IMMBYTE(i);

	t = bcd_sub( RDA, i );
	
	if( !(pSR & SR_C) )
		t = bcd_sub( t, 1 );

	WRA(t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 9;
}
	
void dsb_i2b( void );
void dsb_i2b( void )
{
	UINT8	i;
	UINT16	t;
	
	IMMBYTE(i);

	t = bcd_sub( RDB, i );
	
	if( !(pSR & SR_C) )
		t = bcd_sub( t, 1 );

	WRB(t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 9;
}
	
void dsb_i2r( void );
void dsb_i2r( void )
{
	UINT8	r,i;
	UINT16	t;

	IMMBYTE(i);
	IMMBYTE(r);

	t = bcd_sub( RM(r), i );
	
	if( !(pSR & SR_C) )
		t = bcd_sub( t, 1 );

	WM(r,t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 11;
}
	
void eint( void );
void eint( void )
{
	pSR |= (SR_N|SR_Z|SR_C|SR_I);
	tms7000_icount -= 5;
}

void idle( void );
void idle( void )
{
	tms7000.idle_state = 1;
	cpu_yielduntil_int();
	tms7000_icount -= 6;
}

void inc_a( void );
void inc_a( void )
{
	UINT16	t;
	
	t = RDA + 1;
	
	WRA( t );

	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 5;
}
	
void inc_b( void );
void inc_b( void )
{
	UINT16	t;
	
	t = RDB + 1;
	
	WRB( t );

	CLR_NZC;
	SET_C8(t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 5;
}

void inc_r( void );
void inc_r( void )
{
	UINT16	t;
	UINT8	r;
	
	IMMBYTE(r);
	
	t = RM(r) + 1;
	
	WM( r, t );
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	SET_C8(t);

	tms7000_icount -= 7;
}

void inv_a( void );
void inv_a( void )
{
	UINT16 t;
	
	t = ~(RDA);
	WRA(t);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 5;
}

void inv_b( void );
void inv_b( void )
{
	UINT16 t;
	
	t = ~(RDA);
	WRA(t);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 5;
}

void inv_r( void );
void inv_r( void )
{
	UINT16	t;
	UINT8	r;
	
	IMMBYTE(r);
	
	t = ~(RM(r));
	
	WM( r, t );
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 7;
}

void jc( void );
void jc( void )
{
	if( pSR & SR_C )
	{
		INT8 s;
		
		SIMMBYTE( s );
		pPC += s;
		CHANGE_PC;
		tms7000_icount -= 7;
	}
	else
	{
		pPC++;
		tms7000_icount -= 5;
	}
}
		
void jeq( void );
void jeq( void )
{
	if( pSR & SR_Z )
	{
		INT8 s;
		
		SIMMBYTE( s );
		pPC += s;
		CHANGE_PC;
		tms7000_icount -= 7;
	}
	else
	{
		pPC++;
		tms7000_icount -= 5;
	}
}

void jl( void );
void jl( void )
{
	if( pSR & SR_C )
	{
		pPC++;
		tms7000_icount -= 5;
	}
	else
	{
		INT8 s;
		
		SIMMBYTE( s );
		pPC += s;
		CHANGE_PC;
		tms7000_icount -= 7;
	}
}
		
void jmp( void );
void jmp( void )
{
	INT8 s;
	
	SIMMBYTE( s );
	pPC += s;
	CHANGE_PC;
	tms7000_icount -= 7;
}

void jn( void );
void jn( void )
{
	if( pSR & SR_N )
	{
		INT8 s;
		
		SIMMBYTE( s );
		pPC += s;
		CHANGE_PC;
		tms7000_icount -= 7;
	}
	else
	{
		pPC++;
		tms7000_icount -= 5;
	}
	
}

void jne( void );
void jne( void )
{
	if( pSR & SR_Z )
	{
		pPC++;
		tms7000_icount -= 5;
	}
	else
	{
		INT8 s;
		
		SIMMBYTE( s );
		pPC += s;
		CHANGE_PC;
		tms7000_icount -= 7;
	}
}

void jp( void );
void jp( void )
{
	if( pSR & (SR_Z|SR_N) )
	{
		pPC++;
		tms7000_icount -= 5;
	}
	else
	{
		INT8 s;
		
		SIMMBYTE( s );
		pPC += s;
		CHANGE_PC;
		tms7000_icount -= 7;
	}
}

void jpz( void );
void jpz( void )
{
	if ((pSR & SR_N) == 0 && (pSR & SR_Z) != 0)
	{
		INT8 s;
		
		SIMMBYTE( s );
		pPC += s;
		CHANGE_PC;
		tms7000_icount -= 7;
	}
	else
	{
		pPC++;
		tms7000_icount -= 5;
	}
}

void lda_dir( void );
void lda_dir( void )
{
	UINT16	t;
	PAIR	i;
	
	IMMWORD( i );
	t = RM(i.w.l);
	WRA(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 11;	
}	

void lda_ind( void );
void lda_ind( void )
{
	UINT16	t;
	PAIR	p;
	INT8	i;
	
	IMMBYTE(i);
	p.w.l=RRF16(i);
	t = RM(p.w.l);
	WRA(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 10;	
}	

void lda_inx( void );
void lda_inx( void )
{
	UINT16	t;
	PAIR	i;
	
	IMMWORD( i );
	t = RM(i.w.l + RDB);
	WRA(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 13;
}	

void ldsp( void );
void ldsp( void )
{
	pSP = RDB;
	tms7000_icount -= 5;
}

void mov_a2b( void );
void mov_a2b( void )
{
	UINT16	t;
	
	t = RDA;
	WRB(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 6;
}

void mov_b2a( void );
void mov_b2a( void )
{
	UINT16	t;
	
	t = RDB;
	WRA(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 5;
}


void mov_a2r( void );
void mov_a2r( void )
{
	UINT8	r;
	UINT16	t;
	
	IMMBYTE(r);
	
	t = RDA;
	WM(r,t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 8;
}

void mov_b2r( void );
void mov_b2r( void )
{
	UINT8	r;
	UINT16	t;
	
	IMMBYTE(r);
	
	t = RDB;
	WM(r,t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 7;
}

void mov_r2a( void );
void mov_r2a( void )
{
	UINT8	r;
	UINT16	t;
	
	IMMBYTE(r);
	t = RM(r);
	WRA(t);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 8;
}

void mov_r2b( void );
void mov_r2b( void )
{
	UINT8	r;
	UINT16	t;
	
	IMMBYTE(r);
	t = RM(r);
	WRB(t);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 8;
}

void mov_r2r( void );
void mov_r2r( void )
{
	UINT8	r,s;
	UINT16	t;
	
	IMMBYTE(r);
	IMMBYTE(s);
	t = RM(r);
	WM(s,t);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 10;
}

void mov_i2a( void );
void mov_i2a( void )
{
	UINT16	t;
	
	IMMBYTE(t);
	WRA(t);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 7;
}

void mov_i2b( void );
void mov_i2b( void )
{
	UINT16	t;
	
	IMMBYTE(t);
	WRB(t);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 7;
}

void mov_i2r( void );
void mov_i2r( void )
{
	UINT16	t;
	UINT8	r;
	
	IMMBYTE(t);
	IMMBYTE(r);
	WM(r,t);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 9;
}

void movd_imm( void );
void movd_imm( void )
{
	PAIR	t;
	UINT8	r;
	
	IMMWORD(t);
	IMMBYTE(r);
	WRF16(r,t);
	
	CLR_NZC;
	SET_N8(t.b.h);
	SET_Z8(t.b.h);
	
	tms7000_icount -= 15;
	
}

void movd_r( void );
void movd_r( void )
{
	PAIR	t;
	UINT8	r,s;
	
	IMMBYTE(r);
	IMMBYTE(s);
	t.w.l = RRF16(r);
	WRF16(s,t);
	
	CLR_NZC;
	SET_N8(t.b.h);
	SET_Z8(t.b.h);
	
	tms7000_icount -= 14;
	
}

void movd_ind( void );
void movd_ind( void )
{
	PAIR	t;
	UINT8	r;
	
	IMMWORD(t);
	t.w.l += RDB;
	IMMBYTE(r);
	WRF16(r,t);
	
	CLR_NZC;
	SET_N8(t.b.h);
	SET_Z8(t.b.h);
	
	tms7000_icount -= 17;
}

void movp_a2p( void );
void movp_a2p( void )
{
	UINT8	p;
	UINT16	t;
	
	IMMBYTE(p);
	t=RDA;
	WM( 0x0100+p,t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 10;
}

void movp_b2p( void );
void movp_b2p( void )
{
	UINT8	p;
	UINT16	t;
	
	IMMBYTE(p);
	t=RDB;
	WM( 0x0100+p,t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 10;
}

void movp_r2p( void );
void movp_r2p( void )
{
	UINT8	p,r;
	UINT16	t;
	
	IMMBYTE(r);
	IMMBYTE(p);
	t=RM(r);
	WM( 0x0100+p,t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 11;
}

void movp_p2a( void );
void movp_p2a( void )
{
	UINT8	p;
	UINT16	t;
	
	IMMBYTE(p);
	t=RM(0x0100+p);
	WRA(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 9;
}

void movp_p2b( void );
void movp_p2b( void )
{
	UINT8	p;
	UINT16	t;
	
	IMMBYTE(p);
	t=RM(0x0100+p);
	WRB(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 8;
}

void mpy_ba( void );
void mpy_ba( void )
{
	PAIR t;
	
	t.w.l = RDA * RDB;
	
	WRF16(0x01,t);

	CLR_NZC;
	SET_N8(t.b.h);
	SET_Z8(t.b.h);
	
	tms7000_icount -= 43;
	
}
	
void mpy_ra( void );
void mpy_ra( void )
{
	PAIR	t;
	UINT8	r;
	
	IMMBYTE(r);
	
	t.w.l = RDA * RM(r);
	
	WRF16(0x01,t);

	CLR_NZC;
	SET_N8(t.b.h);
	SET_Z8(t.b.h);
	
	tms7000_icount -= 46;
	
}
	
void mpy_rb( void );
void mpy_rb( void )
{
	PAIR	t;
	UINT8	r;
	
	IMMBYTE(r);
	
	t.w.l = RDB * RM(r);
	
	WRF16(0x01,t);

	CLR_NZC;
	SET_N8(t.b.h);
	SET_Z8(t.b.h);
	
	tms7000_icount -= 46;
	
}
	
void mpy_rr( void );
void mpy_rr( void )
{
	PAIR	t;
	UINT8	r,s;
	
	IMMBYTE(r);
	IMMBYTE(s);
	
	t.w.l = RM(s) * RM(r);
	
	WRF16(0x01,t);

	CLR_NZC;
	SET_N8(t.b.h);
	SET_Z8(t.b.h);
	
	tms7000_icount -= 48;
	
}
	
void mpy_ia( void );
void mpy_ia( void )
{
	PAIR	t;
	UINT8	i;
	
	IMMBYTE(i);
	
	t.w.l = RDA * i;
	
	WRF16(0x01,t);

	CLR_NZC;
	SET_N8(t.b.h);
	SET_Z8(t.b.h);
	
	tms7000_icount -= 45;
	
}
	
void mpy_ib( void );
void mpy_ib( void )
{
	PAIR	t;
	UINT8	i;
	
	IMMBYTE(i);
	
	t.w.l = RDB * i;
	
	WRF16(0x01,t);

	CLR_NZC;
	SET_N8(t.b.h);
	SET_Z8(t.b.h);
	
	tms7000_icount -= 45;
	
}
	
void mpy_ir( void );
void mpy_ir( void )
{
	PAIR	t;
	UINT8	i,r;
	
	IMMBYTE(i);
	IMMBYTE(r);
	
	t.w.l = RM(r) * i;
	
	WRF16(0x01,t);

	CLR_NZC;
	SET_N8(t.b.h);
	SET_Z8(t.b.h);
	
	tms7000_icount -= 47;
	
}
	
void nop( void );
void nop( void )
{
	tms7000_icount -= 4;
}

void or_imp( void );
void or_imp( void )
{
	UINT8	t;
	
	t = RDA | RDB;
	WRA(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 5;
}

void or_r2a( void );
void or_r2a( void )
{
	UINT8	t;
	UINT8	v;
	
	IMMBYTE(v);
	
	t = RM(v) | RDA;
	WRA(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 8;
}

void or_r2b( void );
void or_r2b( void )
{
	UINT8	t;
	UINT8	v;
	
	IMMBYTE(v);
	
	t = RM(v) | RDB;
	WRB(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 8;
}

void or_r2r( void );
void or_r2r( void )
{
	UINT8	t;
	UINT8	i,j;
	
	IMMBYTE(i);
	IMMBYTE(j);
	
	t = RM(i) | RM(j);
	WM(j,t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 10;
}

void or_i2a( void );
void or_i2a( void )
{
	UINT8	t;
	UINT8	v;
	
	IMMBYTE(v);
	
	t = v | RDA;
	WRA(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 7;
}

void or_i2b( void );
void or_i2b( void )
{
	UINT8	t;
	UINT8	v;
	
	IMMBYTE(v);
	
	t = v | RDB;
	WRB(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 7;
}

void or_i2r( void );
void or_i2r( void )
{
	UINT8	t;
	UINT8	i,j;
	
	IMMBYTE(i);
	IMMBYTE(j);
	
	t = i | RM(j);
	WM(j,t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 9;
}

void orp_a2p( void );
void orp_a2p( void )
{
	UINT8	t;
	UINT8	v;
	
	IMMBYTE(v);
	t = RDA | RM( 0x0100 + v);
	WM( 0x0100+v, t);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 10;
}	

void orp_b2p( void );
void orp_b2p( void )
{
	UINT8	t;
	UINT8	v;
	
	IMMBYTE(v);
	t = RDB | RM( 0x0100 + v);
	WM( 0x0100+v, t);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 9;
}	

void orp_i2p( void );
void orp_i2p( void )
{
	UINT8	t;
	UINT8	i,v;
	
	IMMBYTE(i);
	IMMBYTE(v);
	t = i | RM( 0x0100 + v);
	WM( 0x0100+v, t);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 11;
}	

void pop_a( void );
void pop_a( void )
{
	UINT16	t;
	
	PULLBYTE(t);
	WRA(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 6;
}	

void pop_b( void );
void pop_b( void )
{
	UINT16	t;
	
	PULLBYTE(t);
	WRB(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 6;
}	

void pop_r( void );
void pop_r( void )
{
	UINT16	t;
	UINT8	r;
	
	IMMBYTE(r);
	PULLBYTE(t);
	WM(r,t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 8;
}	

void pop_st( void );
void pop_st( void )
{
	UINT16	t;
	
	PULLBYTE(t);
	pSR = t;

	tms7000_icount -= 6;
}	

void push_a( void );
void push_a( void )
{
	UINT16	t;
	
	t = RDA;
	PUSHBYTE(t);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 6;
}

void push_b( void );
void push_b( void )
{
	UINT16	t;
	
	t = RDB;
	PUSHBYTE(t);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 6;
}

void push_r( void );
void push_r( void )
{
	UINT16	t;
	INT8	r;
	
	IMMBYTE(r);
	t = RM(r);
	PUSHBYTE(t);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 8;
}

void push_st( void );
void push_st( void )
{
	UINT16	t;
	t = pSR;
	PUSHBYTE(t);

	tms7000_icount -= 6;
}

void reti( void );
void reti( void )
{
	PULLWORD( PC );
	PULLBYTE( pSR );
	
	tms7000_icount -= 9;
}

void rets_imp( void );
void rets_imp( void )
{
	PULLWORD( PC );
	tms7000_icount -= 7;
}

void rl_a( void );
void rl_a( void )
{
	UINT16	t;
	
	t = RDA << 1;
	
	CLR_NZC;
	SET_C8(t);
	
	if( pSR & SR_C )
		t |= 0x01;

	SET_N8(t);
	SET_Z8(t);
	WRA(t);
	
	tms7000_icount -= 5;
}

void rl_b( void );
void rl_b( void )
{
	UINT16	t;
	
	t = RDB << 1;
	
	CLR_NZC;
	SET_C8(t);
	
	if( pSR & SR_C )
		t |= 0x01;

	SET_N8(t);
	SET_Z8(t);
	WRB(t);
	
	tms7000_icount -= 5;
}

void rl_r( void );
void rl_r( void )
{
	UINT16	t;
	UINT8	r;
	
	IMMBYTE(r);
	t = RM(r) << 1;
	
	CLR_NZC;
	SET_C8(t);
	
	if( pSR & SR_C )
		t |= 0x01;

	SET_N8(t);
	SET_Z8(t);
	WM(r,t);
	
	tms7000_icount -= 7;
}

void rlc_a( void );
void rlc_a( void )
{
	UINT16	t;
	int		old_carry;
	
	old_carry = (pSR & SR_C);
	
	t = RDA << 1;
	
	CLR_NZC;
	SET_C8(t);
	
	if( old_carry )
		t |= 0x01;

	SET_N8(t);
	SET_Z8(t);
	WRA(t);
	
	tms7000_icount -= 5;
}

void rlc_b( void );
void rlc_b( void )
{
	UINT16	t;
	int		old_carry;
	
	old_carry = (pSR & SR_C);
	
	t = RDB << 1;
	
	CLR_NZC;
	SET_C8(t);
	
	if( old_carry )
		t |= 0x01;

	SET_N8(t);
	SET_Z8(t);
	WRB(t);
	
	tms7000_icount -= 5;
}

void rlc_r( void );
void rlc_r( void )
{
	UINT16	t;
	UINT8	r;
	int		old_carry;
	
	old_carry = (pSR & SR_C);
	
	IMMBYTE(r);
	t = RM(r) << 1;
	
	CLR_NZC;
	SET_C8(t);
	
	if( old_carry )
		t |= 0x01;

	SET_N8(t);
	SET_Z8(t);
	WM(r,t);
	
	tms7000_icount -= 7;
}

void rr_a( void );
void rr_a( void )
{
	UINT16	t;
	int		old_bit0;
	
	t = RDA;
	
	old_bit0 = t & 0x0001;
	t = t >> 1;
	
	CLR_NZC;
	
	if( old_bit0 )
	{
		SETC;
		t |= 0x80;
	}
	
	SET_N8(t);
	SET_Z8(t);
	
	WRA(t);
	
	tms7000_icount -= 5;
}

void rr_b( void );
void rr_b( void )
{
	UINT16	t;
	int		old_bit0;
	
	t = RDB;
	
	old_bit0 = t & 0x0001;
	t = t >> 1;
	
	CLR_NZC;
	
	if( old_bit0 )
	{
		SETC;
		t |= 0x80;
	}
	
	SET_N8(t);
	SET_Z8(t);
	
	WRB(t);
	
	tms7000_icount -= 5;
}

void rr_r( void );
void rr_r( void )
{
	UINT16	t;
	UINT8	r;
	
	int		old_bit0;
	
	IMMBYTE(r);
	t = RM(r);
	
	old_bit0 = t & 0x0001;
	t = t >> 1;
	
	CLR_NZC;
	
	if( old_bit0 )
	{
		SETC;
		t |= 0x80;
	}
	
	SET_N8(t);
	SET_Z8(t);
	
	WM(r,t);
	
	tms7000_icount -= 7;
}

void rrc_a( void );
void rrc_a( void )
{
	UINT16	t;
	int		old_bit0;
	
	t = RDA;
	
	old_bit0 = t & 0x0001;
	/* Place carry bit in 9th position */
	t |= ((pSR & SR_C) << 1);
	t = t >> 1;
	
	CLR_NZC;
	
	if( old_bit0 )
		SETC;
	SET_N8(t);
	SET_Z8(t);
	
	WRA(t);
	
	tms7000_icount -= 5;
}

void rrc_b( void );
void rrc_b( void )
{
	UINT16	t;
	int		old_bit0;
	
	t = RDB;
	
	old_bit0 = t & 0x0001;
	/* Place carry bit in 9th position */
	t |= ((pSR & SR_C) << 1);
	t = t >> 1;
	
	CLR_NZC;
	
	if( old_bit0 )
		SETC;
	SET_N8(t);
	SET_Z8(t);
	
	WRB(t);
	
	tms7000_icount -= 5;
}

void rrc_r( void );
void rrc_r( void )
{
	UINT16	t;
	UINT8	r;
	int		old_bit0;
	
	IMMBYTE(r);
	t = RM(r);
	
	old_bit0 = t & 0x0001;
	/* Place carry bit in 9th position */
	t |= ((pSR & SR_C) << 1);
	t = t >> 1;
	
	CLR_NZC;
	
	if( old_bit0 )
		SETC;
	SET_N8(t);
	SET_Z8(t);
	
	WM(r,t);
	
	tms7000_icount -= 7;
}

void sbb_ba( void );
void sbb_ba( void )
{
	UINT16	t;
	
	t = RDB - RDA - (pSR & SR_C ? 1 : 0);
	WRA(t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 5;
}

void sbb_ra( void );
void sbb_ra( void )
{
	UINT16	t;
	UINT8	r;
	
	IMMBYTE(r);
	t = RM(r) - RDA - (pSR & SR_C ? 1 : 0);
	WRA(t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 8;
}

void sbb_rb( void );
void sbb_rb( void )
{
	UINT16	t;
	UINT8	r;
	
	IMMBYTE(r);
	t = RM(r) - RDB - (pSR & SR_C ? 1 : 0);
	WRB(t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 8;
}

void sbb_rr( void );
void sbb_rr( void )
{
	UINT16	t;
	UINT8	r,s;
	
	IMMBYTE(s);
	IMMBYTE(r);
	t = RM(s) - RM(r) - (pSR & SR_C ? 1 : 0);
	WM(r,t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 10;
}

void sbb_ia( void );
void sbb_ia( void )
{
	UINT16	t;
	UINT8	i;
	
	IMMBYTE(i);
	t = i - RDA - (pSR & SR_C ? 1 : 0);
	WRA(t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 7;
}

void sbb_ib( void );
void sbb_ib( void )
{
	UINT16	t;
	UINT8	i;
	
	IMMBYTE(i);
	t = i - RDB - (pSR & SR_C ? 1 : 0);
	WRB(t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 7;
}

void sbb_ir( void );
void sbb_ir( void )
{
	UINT16	t;
	UINT8	r,i;
	
	IMMBYTE(i);
	IMMBYTE(r);
	t = i - RM(r) - (pSR & SR_C ? 1 : 0);
	WM(r,t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 9;
}

void setc( void );
void setc( void )
{
	CLR_NZC;
	pSR |= (SR_C|SR_Z);
	
	tms7000_icount -= 5;
}
	
void sta_dir( void );
void sta_dir( void )
{
	UINT16	t;
	PAIR	i;
	
	t = RDA;
	IMMWORD( i );
	
	WM(i.w.l,t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 11;	
}	

void sta_ind( void );
void sta_ind( void )
{
	UINT16	t;
	PAIR	p;
	INT8	r;
	
	IMMBYTE(r);
	p.w.l = RRF16(r);
	t = RDA;
	WM(p.w.l,t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 10;	
}	

void sta_inx( void );
void sta_inx( void )
{
	UINT16	t;
	PAIR	i;
	
	IMMWORD( i );
	t = RDA;
	WM(i.w.l+RDB,t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);

	tms7000_icount -= 13;
}	

void stsp( void );
void stsp( void )
{
	WRB(pSP);

	tms7000_icount -= 6;
}	

void sub_ba( void );
void sub_ba( void )
{
	UINT16	t;
	
	t = RDB - RDA;
	WRA(t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 5;
}

void sub_ra( void );
void sub_ra( void )
{
	UINT16	t;
	UINT8	r;
	
	IMMBYTE(r);
	t = RM(r) - RDA;
	WRA(t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 8;
}

void sub_rb( void );
void sub_rb( void )
{
	UINT16	t;
	UINT8	r;
	
	IMMBYTE(r);
	t = RM(r) - RDB;
	WRB(t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 8;
}

void sub_rr( void );
void sub_rr( void )
{
	UINT16	t;
	UINT8	r,s;
	
	IMMBYTE(s);
	IMMBYTE(r);
	t = RM(s) - RM(r);
	WM(r,t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 10;
}

void sub_ia( void );
void sub_ia( void )
{
	UINT16	t;
	UINT8	i;
	
	IMMBYTE(i);
	t = i - RDA;
	WRA(t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 7;
}

void sub_ib( void );
void sub_ib( void )
{
	UINT16	t;
	UINT8	i;
	
	IMMBYTE(i);
	t = i - RDB;
	WRB(t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 7;
}

void sub_ir( void );
void sub_ir( void )
{
	UINT16	t;
	UINT8	r,i;
	
	IMMBYTE(i);
	IMMBYTE(r);
	t = i - RM(r);
	WM(r,t);

	CLR_NZC;
	SET_C8(~t);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 9;
}

void trap_0( void );
void trap_0( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xfffe);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_1( void );
void trap_1( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xfffc);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_2( void );
void trap_2( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xfffa);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_3( void );
void trap_3( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xfff8);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_4( void );
void trap_4( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xfff6);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_5( void );
void trap_5( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xfff4);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_6( void );
void trap_6( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xfff2);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_7( void );
void trap_7( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xfff0);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_8( void );
void trap_8( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xffee);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_9( void );
void trap_9( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xffec);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_10( void );
void trap_10( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xffea);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_11( void );
void trap_11( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xffe8);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_12( void );
void trap_12( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xffe6);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_13( void );
void trap_13( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xffe4);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_14( void );
void trap_14( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xffe2);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_15( void );
void trap_15( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xffe0);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_16( void );
void trap_16( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xffde);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_17( void );
void trap_17( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xffdc);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_18( void );
void trap_18( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xffda);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_19( void );
void trap_19( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xffd8);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_20( void );
void trap_20( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xffd6);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_21( void );
void trap_21( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xffd4);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_22( void );
void trap_22( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xffd2);
	CHANGE_PC;
	tms7000_icount -= 14;
}
	
void trap_23( void );
void trap_23( void )
{
	PUSHWORD( PC );
	pPC = RM16(0xffd0);
	CHANGE_PC;
	tms7000_icount -= 14;
}

void swap_a( void );
void swap_a( void )
{
	UINT8	a,b;
	UINT16	t;
	
	a = b = RDA;
	
	a <<= 4;
	b >>= 4;
	t = a+b;
	
	WRA(t);
	
	CLR_NZC;
	
	pSR|=((t&0x0001)<<7);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -=8;
}

void swap_b( void );
void swap_b( void )
{
	UINT8	a,b;
	UINT16	t;
	
	a = b = RDB;
	
	a <<= 4;
	b >>= 4;
	t = a+b;
	
	WRB(t);
	
	CLR_NZC;
	
	pSR|=((t&0x0001)<<7);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -=8;
}

void swap_r( void );
void swap_r( void )
{
	UINT8	a,b,r;
	UINT16	t;
	
	IMMBYTE(r);
	a = b = RM(r);
	
	a <<= 4;
	b >>= 4;
	t = a+b;
	
	WM(r,t);
	
	CLR_NZC;
	
	pSR|=((t&0x0001)<<7);
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -=8;
}

void tstb( void );
void tstb( void )
{
	UINT16	t;
	
	t=RDB;

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 6;
}

void xchb_a( void );
void xchb_a( void )
{
	UINT16	t,u;
	
	t = RDB;
	u = RDA;
	
	WRA(t);
	WRB(u);	
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 6;
}	

void xchb_b( void );
void xchb_b( void )
{
	UINT16	t;
/*	UINT16	u;	*/
	
	t = RDB;
/*	u = RDB;	*/
	
/*	WRB(t);		*/
/*	WRB(u);		*/
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 6;
}	

void xchb_r( void );
void xchb_r( void )
{
	UINT16	t,u;
	UINT8	r;
	
	IMMBYTE(r);
	
	t = RDB;
	u = RM(r);
	
	WRA(t);
	WRB(u);	
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 8;
}	

void xor_imp( void );
void xor_imp( void )
{
	UINT8	t;
	
	t = RDA ^ RDB;
	WRA(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 5;
}

void xor_r2a( void );
void xor_r2a( void )
{
	UINT8	t;
	UINT8	v;
	
	IMMBYTE(v);
	
	t = RM(v) ^ RDA;
	WRA(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 8;
}

void xor_r2b( void );
void xor_r2b( void )
{
	UINT8	t;
	UINT8	v;
	
	IMMBYTE(v);
	
	t = RM(v) ^ RDB;
	WRB(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 8;
}

void xor_r2r( void );
void xor_r2r( void )
{
	UINT8	t;
	UINT8	i,j;
	
	IMMBYTE(i);
	IMMBYTE(j);
	
	t = RM(i) ^ RM(j);
	WM(j,t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 10;
}

void xor_i2a( void );
void xor_i2a( void )
{
	UINT8	t;
	UINT8	v;
	
	IMMBYTE(v);
	
	t = v ^ RDA;
	WRA(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 7;
}

void xor_i2b( void );
void xor_i2b( void )
{
	UINT8	t;
	UINT8	v;
	
	IMMBYTE(v);
	
	t = v ^ RDB;
	WRB(t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 7;
}

void xor_i2r( void );
void xor_i2r( void )
{
	UINT8	t;
	UINT8	i,j;
	
	IMMBYTE(i);
	IMMBYTE(j);
	
	t = i ^ RM(j);
	WM(j,t);
	
	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 9;
}

void xorp_a2p( void );
void xorp_a2p( void )
{
	UINT8	t;
	UINT8	v;
	
	IMMBYTE(v);
	t = RDA ^ RM( 0x0100 + v);
	WM( 0x0100+v, t);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 10;
}	

void xorp_b2p( void );
void xorp_b2p( void )
{
	UINT8	t;
	UINT8	v;
	
	IMMBYTE(v);
	t = RDB ^ RM( 0x0100 + v);
	WM( 0x0100+v, t);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 9;
}	

void xorp_i2p( void );
void xorp_i2p( void )
{
	UINT8	t;
	UINT8	i,v;
	
	IMMBYTE(i);
	IMMBYTE(v);
	t = i ^ RM( 0x0100 + v);
	WM( 0x0100+v, t);

	CLR_NZC;
	SET_N8(t);
	SET_Z8(t);
	
	tms7000_icount -= 11;
}	






