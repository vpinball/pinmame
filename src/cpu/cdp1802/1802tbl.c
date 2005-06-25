//#define LOG_ICODE

#ifdef LOG_ICODE
static void cdp1802_log_icode(void)
{
	// logs the i-code
	UINT16 adr=cdp1802.reg[5].w.l;
	UINT8 u=cpu_readmem16(adr),u2=cpu_readmem16(adr+1);
	UINT16 i=(u<<8)|u2;

	switch (u&0xf0) {
	case 0:
		logerror("chip 8 icode %.4x: %.4x call 1802 %.3x\n",adr, i, i&0xfff);
		break;
	case 0x10:
		logerror("chip 8 icode %.4x: %.4x jmp %.3x\n",adr, i, i&0xfff);
		break;
	case 0x20:
		logerror("chip 8 icode %.4x: %.4x call %.3x\n",adr, i, i&0xfff);
		break;
	case 0x30:
		logerror("chip 8 icode %.4x: %.4x jump if r%x !=0  %.2x\n",adr, i,
				 (i&0xf00)>>8,i&0xff);
		break;
	case 0x40:
		logerror("chip 8 icode %.4x: %.4x jump if r%x ==0  %.2x\n",adr, i,
				 (i&0xf00)>>8,i&0xff);
		break;
	case 0x50:
		logerror("chip 8 icode %.4x: %.4x skip if r%x!=%.2x\n",adr, i,
				 (i&0xf00)>>8,i&0xff);
		break;
	case 0x60:
		logerror("chip 8 icode %.4x: %.4x load r%x,%.2x\n",adr, i,
				 (i&0xf00)>>8,i&0xff);
		break;
	case 0x70:
		if (u!=0x70)
			logerror("chip 8 icode %.4x: %.4x add r%x,%.2x\n",adr, i,
					 (i&0xf00)>>8,i&0xff);
		else
			logerror("chip 8 icode %.4x: %.4x dec r0, jump if not zero %.2x\n",adr, i,
					 i&0xff);
		break;
	case 0x80:
		switch (u2&0xf) {
		case 0:
			logerror("chip 8 icode %.4x: %.4x r%x:=r%x\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
			break;
		case 1:
			logerror("chip 8 icode %.4x: %.4x r%x|=r%x\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
			break;
		case 2:
			logerror("chip 8 icode %.4x: %.4x r%x&=r%x\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
			break;
		case 3:
			logerror("chip 8 icode %.4x: %.4x r%x^=r%x\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
			break;
		case 4:
			logerror("chip 8 icode %.4x: %.4x r%x+=r%x, rb carry\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
				break;
		case 5:
			logerror("chip 8 icode %.4x: %.4x r%x=r%x-r%x, rb carry\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4,(i&0xf00)>>8);
			break;
		case 6:
			logerror("chip 8 icode %.4x: %.4x r%x>>=1, rb LSB\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
			break;
		case 7:
			logerror("chip 8 icode %.4x: %.4x r%x-=r%x, rb carry\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
			break;
		case 0xe:
			logerror("chip 8 icode %.4x: %.4x r%x<<=1, rb MSB\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
			break;
		default:
			logerror("chip 8 i-code %.4x %.2x %.2x\n",cdp1802.reg[5].w.l,
					 cpu_readmem16(cdp1802.reg[5].w.l),
					 cpu_readmem16(cdp1802.reg[5].w.l+1));
		}
		break;
	case 0x90:
		switch (u2&0xf) {
		case 0:
			logerror("chip 8 icode %.4x: %.4x skip if r%x!=r%x\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
			break;
		case 1:case 3: case 5: case 7: case 9: case 0xb: case 0xd: case 0xf:
			logerror("chip 8 icode %.4x: %.4x r%x:=r%x\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
			break;
		case 2:case 6: case 0xa: case 0xe:
			logerror("chip 8 icode %.4x: %.4x r%x:=Memory[r%x]\n",adr, i,
					 (i&0xf00)>>8,(i&0xf0)>>4);
			break;
		case 4:case 0xc:
			logerror("chip 8 icode %.4x: %.4x Memory[r%x]\n:=r%x",adr, i,
					 (i&0xf0)>>4,(i&0xf00)>>8);
			break;
		case 8:
			logerror("chip 8 icode %.4x: %.4x Memory[r%x]\n:=BCD(r%x), r%x+=3",adr, i,
					 (i&0xf0)>>4,(i&0xf00)>>8,(i&0xf0)>>4);
			break;
		default:
			logerror("chip 8 i-code %.4x %.2x %.2x\n",cdp1802.reg[5].w.l,
					 cpu_readmem16(cdp1802.reg[5].w.l),
					 cpu_readmem16(cdp1802.reg[5].w.l+1));
		}
		break;
	case 0xa0:
		logerror("chip 8 icode %.4x: %.4x index register:=%.3x\n",adr, i,
				 i&0xfff);
		break;
	case 0xb0:
		logerror("chip 8 icode %.4x: %.4x store %.2x at index register,  add %x to index register\n",adr, i,
				 i&0xff,(i&0xf00)>>8);
		break;
	case 0xc0:
		if (u==0xc0)
			logerror("chip 8 icode %.4x: %.4x return from subroutine\n",adr, i);
		else
			logerror("chip 8 icode %.4x: %.4x r%x:=random&%.2x\n",adr, i,
					 (i&0xf00)>>8,i&0xff);
		break;
	case 0xd0:
		logerror("chip 8 icode %.4x: %.4x if key %x goto %.2x\n",adr, i,
				 (i&0xf00)>>8,i&0xff);
		break;
	default:
		logerror("chip 8 i-code %.4x %.2x %.2x\n",cdp1802.reg[5].w.l,
				 cpu_readmem16(cdp1802.reg[5].w.l),
				 cpu_readmem16(cdp1802.reg[5].w.l+1));
	}
}
#endif

INLINE void cdp1802_add(UINT8 data)
{
	int i;
	i=cdp1802.d+data;
	cdp1802.d=i&0xff;cdp1802.df=i&0x100;
}

INLINE void cdp1802_add_carry(UINT8 data)
{
	int i;
	if (cdp1802.df) i=cdp1802.d+data+1;
	else i=cdp1802.d+data;
	cdp1802.d=i&0xff;cdp1802.df=i&0x100;
}

INLINE void cdp1802_sub(UINT8 left,UINT8 right)
{
	int i;
	i=left-right;
	cdp1802.d=i&0xff;cdp1802.df=i>=0;
}

INLINE void cdp1802_sub_carry(UINT8 left,UINT8 right)
{
	int i;
	if (cdp1802.df) i=left-right-1;  //?
	else i=left-right;
	cdp1802.d=i&0xff;cdp1802.df=i>=0;
}

INLINE UINT16 cdp1802_read_operand_word(void)
{
	UINT16 a=cpu_readop(PC++)<<8;
	a|=cpu_readop(PC++);
	return a;
}

INLINE void cdp1802_long_branch(bool flag)
{
	UINT16 new=cdp1802_read_operand_word();
	if (flag) PC=new;
}

INLINE void cdp1802_long_skip(bool flag)
{
	if (flag) PC+=2;
}

INLINE void cdp1802_short_branch(bool flag)
{
	UINT8 new=cpu_readop(PC++);
	if (flag) cdp1802.reg[cdp1802.p].b.l=new;
}

INLINE void cdp1802_short_branch_ef(bool flag, int mask)
{
	bool b=0;
	UINT8 new=cpu_readop(PC++);

	if (cdp1802.config&&cdp1802.config->in_ef) b=cdp1802.config->in_ef()&mask?1:0;
	if (flag==b) cdp1802.reg[cdp1802.p].b.l=new;
}

INLINE void cdp1802_q(bool level)
{
	cdp1802.q=level;
	if (cdp1802.config&&cdp1802.config->out_q) cdp1802.config->out_q(level);
}

INLINE void cdp1802_read_px(void)
{
	UINT8 i=cpu_readmem16(X++);
	cdp1802.p=i&0xf;
	cdp1802.x=i>>4;
}

INLINE void cdp1802_out_n(int n)
{
	UINT8 i=cpu_readmem16(X++);
	if (cdp1802.config&&cdp1802.config->out_n) cdp1802.config->out_n(i, n);
}

INLINE void cdp1802_in_n(int n)
{
	UINT8 i=0;
	if (cdp1802.config&&cdp1802.config->in_n) i=cdp1802.config->in_n(n);
	cdp1802.d=i;
	cpu_writemem16(cpu_readmem16(X),i);
}

static void cdp1802_instruction(void)
{
	int oper;
	bool b;

#ifdef LOG_ICODE
	if (PC==0x6b) cdp1802_log_icode(); // if you want to have the icodes in the debuglog
#endif

	oper=cpu_readop(PC++);
	switch(oper&0xf0) {
	case 0:
		if (oper==0) {
			cdp1802.idle=1;
		} else {
			cdp1802.d=cpu_readmem16(cdp1802.reg[oper&0xf].w.l);
		}
		break;
	case 0x10: cdp1802.reg[oper&0xf].w.l++;break;
	case 0x20: cdp1802.reg[oper&0xf].w.l--;break;
	case 0x40: cdp1802.d=cpu_readmem16(cdp1802.reg[oper&0xf].w.l++);break;
	case 0x50: cpu_writemem16(cdp1802.reg[oper&0xf].w.l,cdp1802.d);break;
	case 0x80: cdp1802.d=cdp1802.reg[oper&0xf].b.l;break;
	case 0x90: cdp1802.d=cdp1802.reg[oper&0xf].b.h;break;
	case 0xa0: cdp1802.reg[oper&0xf].b.l=cdp1802.d;break;
	case 0xb0: cdp1802.reg[oper&0xf].b.h=cdp1802.d;break;
	case 0xd0: cdp1802.p=oper&0xf;change_pc16(PC);break;
	case 0xe0: cdp1802.x=oper&0xf;break;
		break;
	default:
		switch(oper&0xf8) {
		case 0x60:
			if (oper==0x60) {
				X++;
			} else {
				cdp1802_out_n(oper&7);break;
			}
			break;
		case 0x68:
			if (oper==0x68) {
			} else {
				cdp1802_in_n(oper&7);break;
			}
			break;
		default:
			switch (oper) {
			case 0x30: cdp1802_short_branch(1);break;
			case 0x31: cdp1802_short_branch(cdp1802.q);break;
			case 0x32: cdp1802_short_branch(cdp1802.d==0);break;
			case 0x33: cdp1802_short_branch(cdp1802.df);break;
			case 0x34: cdp1802_short_branch_ef(1,1);break;
			case 0x35: cdp1802_short_branch_ef(1,2);break;
			case 0x36: cdp1802_short_branch_ef(1,4);break;
			case 0x37: cdp1802_short_branch_ef(1,8);break;
			case 0x38: cdp1802_short_branch(0);break;
			case 0x39: cdp1802_short_branch(!cdp1802.q);break;
			case 0x3a: cdp1802_short_branch(cdp1802.d!=0);break;
			case 0x3b: cdp1802_short_branch(!cdp1802.df);break;
			case 0x3c: cdp1802_short_branch_ef(0,1);break;
			case 0x3d: cdp1802_short_branch_ef(0,2);break;
			case 0x3e: cdp1802_short_branch_ef(0,4);break;
			case 0x3f: cdp1802_short_branch_ef(0,8);break;
			case 0x70: cdp1802_read_px();cdp1802.ie=1;break;
			case 0x71: cdp1802_read_px();cdp1802.ie=0;break;
			case 0x72: cdp1802.d=cpu_readmem16(X++);break;
			case 0x73: cpu_writemem16(X--,cdp1802.d);break;
			case 0x74: cdp1802_add_carry(cpu_readmem16(X));break;
			case 0x75: cdp1802_sub_carry(cpu_readmem16(X),cdp1802.d);break;
			case 0x76:
				b=cdp1802.df;cdp1802.df=cdp1802.d&1;cdp1802.d>>=1;
				if (b) cdp1802.d|=0x80;
				break;
			case 0x77: cdp1802_sub_carry(cdp1802.d, cpu_readmem16(X));break;
			case 0x78: cpu_writemem16(X, cdp1802.t);break;
			case 0x79:
				cdp1802.t=cdp1802.x<<4|cdp1802.p;
				cpu_writemem16(cdp1802.reg[2].w.l, cdp1802.t);
				cdp1802.x=cdp1802.p;
				cdp1802.reg[2].w.l--;
				logerror("cpu cdp1802 unsure mark(0x79) at %.4x PC=%x\n",oper, PC-1);
				break;
			case 0x7a: cdp1802_q(0);break;
			case 0x7b: cdp1802_q(1);break;
			case 0x7c: cdp1802_add_carry(cpu_readmem16(PC++));break;
			case 0x7d: cdp1802_sub_carry(cpu_readmem16(PC++),cdp1802.d);break;
			case 0x7e:
				b=cdp1802.df;cdp1802.df=cdp1802.d&0x80;cdp1802.d<<=1;
				if (b) cdp1802.d|=1;
				break;
			case 0x7f: cdp1802_sub_carry(cdp1802.d,cpu_readmem16(PC++));break;
			case 0xc0: cdp1802_long_branch(1);break;
			case 0xc1: cdp1802_long_branch(cdp1802.q);break;
			case 0xc2: cdp1802_long_branch(cdp1802.d==0);break;
			case 0xc3: cdp1802_long_branch(cdp1802.df);break;
			case 0xc4: /*nop*/break;
			case 0xc5: cdp1802_long_skip(!cdp1802.q);cdp1802_icount-=1;break;
			case 0xc6: cdp1802_long_skip(cdp1802.d!=0);cdp1802_icount-=1;break;
			case 0xc7: cdp1802_long_skip(!cdp1802.df);cdp1802_icount-=1;break;
			case 0xc8: cdp1802_long_skip(1);cdp1802_icount-=1;break;
			case 0xc9: cdp1802_long_branch(!cdp1802.q);cdp1802_icount-=1;break;
			case 0xca: cdp1802_long_branch(cdp1802.d!=0);cdp1802_icount-=1;break;
			case 0xcb: cdp1802_long_branch(!cdp1802.df);cdp1802_icount-=1;break;
			case 0xcc: cdp1802_long_skip(cdp1802.ie);cdp1802_icount-=1;break;
			case 0xcd: cdp1802_long_skip(cdp1802.q);cdp1802_icount-=1;break;
			case 0xce: cdp1802_long_skip(cdp1802.d==0);cdp1802_icount-=1;break;
			case 0xcf: cdp1802_long_skip(cdp1802.df);cdp1802_icount-=1;break;
			case 0xf0: cdp1802.d=cpu_readmem16(X);break;
			case 0xf1: cdp1802.d|=cpu_readmem16(X);break;
			case 0xf2: cdp1802.d&=cpu_readmem16(X);break;
			case 0xf3: cdp1802.d^=cpu_readmem16(X);break;
			case 0xf4: cdp1802_add(cpu_readmem16(X));break;
			case 0xf5: cdp1802_sub(cpu_readmem16(X),cdp1802.d);break;
			case 0xf6: cdp1802.df=cdp1802.d&1;cdp1802.d>>=1;break;
			case 0xf7: cdp1802_sub(cdp1802.d,cpu_readmem16(X));break;
			case 0xf8: cdp1802.d=cpu_readmem16(PC++);break;
			case 0xf9: cdp1802.d|=cpu_readmem16(PC++);break;
			case 0xfa: cdp1802.d&=cpu_readmem16(PC++);break;
			case 0xfb: cdp1802.d^=cpu_readmem16(PC++);break;
			case 0xfc: cdp1802_add(cpu_readmem16(PC++));break;
			case 0xfd: cdp1802_sub(cpu_readmem16(PC++),cdp1802.d);break;
			case 0xfe: cdp1802.df=cdp1802.d&0x80;cdp1802.d<<=1;break;
			case 0xff: cdp1802_sub(cdp1802.d,cpu_readmem16(PC++));break;
			default:
				logerror("cpu cdp1802 unknown opcode %.2x at %.4x\n",oper, PC-1);
				break;
			}
			break;
		}
		break;
	}
	cdp1802_icount-=2;
}

