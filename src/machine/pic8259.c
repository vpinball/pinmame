#include "pic8259.h"

#if 0
#define DBG_LOG(level, text, print) \
                if (level>0) { \
                                logerror("%s\t", text); \
                                logerror print; \
                }
#else
#define DBG_LOG(level, text, print)
#endif

typedef struct {
 UINT8 enable;
 UINT8 in_service;
 UINT8 pending;
 UINT8 prio;

 UINT8 icw2;
 UINT8 icw3;
 UINT8 icw4;

 UINT8 special;
 UINT8 input;

 UINT8 level_trig_mode;
 UINT8 vector_size;
 UINT8 cascade;
 UINT8 base;
 UINT8 slave;

 UINT8 nested;
 UINT8 mode;
 UINT8 auto_eoi;
 UINT8 x86;
#ifdef PINMAME
 int   cpuno, irqno;
#endif /* PINMAME */
} PIC8259;

static PIC8259 pic8259[2]= { { 0xff }, { 0xff }};
#ifdef PINMAME
void pic8259_0_config(int cpuno, int irqno) {
  memset(&pic8259[0],0,sizeof(pic8259[0]));
  pic8259[0].enable = 0xff; pic8259[0].cpuno = cpuno; pic8259[0].irqno = irqno;
}
void pic8259_1_config(int cpuno, int irqno) {
  memset(&pic8259[1],0,sizeof(pic8259[1]));
  pic8259[1].enable = 0xff;  pic8259[1].cpuno = cpuno; pic8259[1].irqno = irqno;
}
#endif /* PINMAME */
static void pic8259_issue_irq(PIC8259 *this, int irq) {
  UINT8 mask = 1 << irq;
  DBG_LOG(1,"PIC_issue_irq",("IRQ%d: ", irq));

  /* PIC not initialized? */
  if (this->icw2 || this->icw3 || this->icw4)
    { DBG_LOG(1,0,("PIC not initialized!\n")); return; }

  /* can't we handle it? */
  if (irq < 0 || irq > 7)
    { DBG_LOG(1,0,("out of range!\n")); return; }

  /* interrupt not enabled? */
  if (this->enable & mask) {
    DBG_LOG(1,0,("is not enabled\n"));
    /* this->pending &= ~mask; */
    /* this->in_service &= ~mask; */
    return;
  }

  /* same interrupt not yet acknowledged ? */
  if (this->in_service & mask) {
    DBG_LOG(1,0,("is already in service\n"));
    /* save request mask for later HACK! */
    this->in_service &= ~mask;
    this->pending |= mask;
    return;
  }

  /* higher priority interrupt in service? */
  if (this->in_service & (mask-1)) {
    DBG_LOG(1,0,("is lower priority\n"));
    this->pending |= mask; /* save request mask for later */
    return;
  }

  /* remove from the requested INTs */
  this->pending &= ~mask;

  /* mask interrupt until acknowledged */
  this->in_service |= mask;

  irq += this->base;
  DBG_LOG(1,0,("INT %02X\n", irq));
#ifdef PINMAME
  cpu_irq_line_vector_w(this->cpuno,this->irqno,irq);
  cpu_set_irq_line(this->cpuno,this->irqno,HOLD_LINE);
#else /* PINMAME */
  cpu_irq_line_vector_w(0,0,irq);
  cpu_set_irq_line(0,0,HOLD_LINE);
#endif /* PINMAME */
}

static int pic8259_irq_pending(PIC8259 *this, int irq) {
  UINT8 mask = 1 << irq;
  return (this->pending & mask) ? 1 : 0;
}

static void pic8259_w(PIC8259 *this, offs_t offset, data8_t data ) {
  switch( offset ) {
    case 0: /* PIC acknowledge IRQ */
      if (data & 0x10) { /* write ICW1 ? */
        this->icw2 = 1;
        this->icw3 = 1;
        this->level_trig_mode = (data >> 3) & 1;
        this->vector_size = (data >> 2) & 1;
        this->cascade = ((data >> 1) & 1) ^ 1;
        if (this->cascade == 0) this->icw3 = 0;
        this->icw4 = data & 1;
        DBG_LOG(1,"PIC_ack_w",("$%02x: ICW1, icw4 %d, cascade %d, vec size %d, ltim %d\n",
                data, this->icw4, this->cascade, this->vector_size, this->level_trig_mode));
      }
      else if (data & 0x08) {
        DBG_LOG(1,"PIC_ack_w",("$%02x: OCW3", data));
        switch (data & 0x60) {
          case 0x00: case 0x20: break;
          case 0x40:
            DBG_LOG(1,0,(", reset special mask")); break;
          case 0x60:
            DBG_LOG(1,0,(", set special mask")); break;
        }
        switch (data & 0x03) {
          case 0x00: case 0x01:
            DBG_LOG(1,0,(", no operation")); break;
          case 0x02:
            DBG_LOG(1,0,(", read request register"));
            this->special = 1;
            this->input = this->pending;
            break;
          case 0x03:
            DBG_LOG(1,0,(", read in-service register"));
            this->special = 1;
            this->input = this->in_service & ~this->enable;
            break;
        }
        DBG_LOG(1,0,("\n"));
      }
      else {
        int n = data & 7;
        UINT8 mask = 1 << n;
        DBG_LOG(1,"PIC_ack_w",("$%02x: OCW2", data));
        switch (data & 0xe0) {
          case 0x00:
            DBG_LOG(1,0,(" rotate auto EOI clear\n"));
            this->prio = 0;
            break;
          case 0x20:
            DBG_LOG(1,0,(" nonspecific EOI\n"));
            for (n = 0, mask = 1<<this->prio; n < 8; n++, mask = (mask<<1) | (mask>>7)) {
              if (this->in_service & mask) {
                this->in_service &= ~mask;
                this->pending &= ~mask;
                break;
              }
            } // for
            break;
          case 0x40:
            DBG_LOG(1,0,(" OCW2 NOP\n")); break;
          case 0x60:
            DBG_LOG(1,0,(" OCW2 specific EOI%d\n", n));
            if (this->in_service & mask) {
              this->in_service &= ~mask;
              this->pending &= ~mask;
            }
            break;
          case 0x80:
            DBG_LOG(1,0,(" OCW2 rotate auto EOI set\n"));
            this->prio = ++this->prio & 7;
            break;
          case 0xa0:
            DBG_LOG(1,0,(" OCW2 rotate on nonspecific EOI\n"));
            for (n = 0, mask = 1<<this->prio; n < 8; n++, mask = (mask<<1) | (mask>>7)) {
              if (this->in_service & mask) {
                this->in_service &= ~mask;
                this->pending &= ~mask;
                this->prio = ++this->prio & 7;
                break;
              }
            }
            break;
          case 0xc0:
            DBG_LOG(1,0,(" OCW2 set priority\n"));
            this->prio = n & 7;
            break;
          case 0xe0:
            DBG_LOG(1,0,(" OCW2 rotate on specific EOI%d\n", n));
            if (this->in_service & mask) {
              this->in_service &= ~mask;
              this->pending &= ~mask;
              this->prio = ++this->prio & 7;
            }
            break;
        } // switch
      }
      break;
    case 1: /* PIC ICW2,3,4 or OCW1 */
      if (this->icw2) {
        this->base = data & 0xf8;
        DBG_LOG(1,"PIC_enable_w",("$%02x: ICW2 (base)\n", this->base));
        this->icw2 = 0;
      }
      else if (this->icw3) {
        this->slave = data;
        DBG_LOG(1,"PIC_enable_w",("$%02x: ICW3 (slave)\n", this->slave));
        this->icw3 = 0;
      }
      else if (this->icw4) {
        this->nested = (data >> 4) & 1;
        this->mode = (data >> 2) & 3;
        this->auto_eoi = (data >> 1) & 1;
        this->x86 = data & 1;
        DBG_LOG(1,"PIC_enable_w",("$%02x: ICW4 x86 mode %d, auto EOI %d, mode %d, nested %d\n",
                data, this->x86, this->auto_eoi, this->mode, this->nested));
        this->icw4 = 0;
      }
      else {
        DBG_LOG(1,"PIC_enable_w",("$%02x: OCW1 enable\n", data));
        this->enable = data;
        this->in_service &= data;
        this->pending &= data;
      }
      break;
  } // switch
  if (this->pending & 0x01) pic8259_issue_irq(this, 0);
  if (this->pending & 0x02) pic8259_issue_irq(this, 1);
  if (this->pending & 0x04) pic8259_issue_irq(this, 2);
  if (this->pending & 0x08) pic8259_issue_irq(this, 3);
  if (this->pending & 0x10) pic8259_issue_irq(this, 4);
  if (this->pending & 0x20) pic8259_issue_irq(this, 5);
  if (this->pending & 0x40) pic8259_issue_irq(this, 6);
  if (this->pending & 0x80) pic8259_issue_irq(this, 7);
}

static int pic8259_r(PIC8259 *this, offs_t offset) {
  int data = 0xff;
  switch (offset) {
    case 0: /* PIC acknowledge IRQ */
      if (this->special) {
        this->special = 0;
        data = this->input;
        DBG_LOG(1,"PIC_ack_r",("$%02x read special\n", data));
      }
      else
        DBG_LOG(1,"PIC_ack_r",("$%02x\n", data));
      break;
    case 1: /* PIC mask register */
      data = this->enable;
      DBG_LOG(1,"PIC_enable_r",("$%02x\n", data));
      break;
  }
  return data;
}

READ_HANDLER(pic8259_0_r) { return pic8259_r(pic8259,offset); }
READ_HANDLER(pic8259_1_r) { return pic8259_r(pic8259+1,offset); }
WRITE_HANDLER(pic8259_0_w) { pic8259_w(pic8259, offset, data); }
WRITE_HANDLER(pic8259_1_w) { pic8259_w(pic8259+1, offset, data); }
void pic8259_0_issue_irq(int irq) { pic8259_issue_irq(pic8259, irq); }
void pic8259_1_issue_irq(int irq) { pic8259_issue_irq(pic8259+1, irq); }
int pic8259_0_irq_pending(int irq) { return pic8259_irq_pending(pic8259, irq); }
int pic8259_1_irq_pending(int irq) { return pic8259_irq_pending(pic8259+1, irq); }
