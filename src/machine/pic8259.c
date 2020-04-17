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
static void pic8259_issue_irq(PIC8259 *self, int irq) {
  UINT8 mask = 1 << irq;
  DBG_LOG(1,"PIC_issue_irq",("IRQ%d: ", irq));

  /* PIC not initialized? */
  if (self->icw2 || self->icw3 || self->icw4)
    { DBG_LOG(1,0,("PIC not initialized!\n")); return; }

  /* can't we handle it? */
  if (irq < 0 || irq > 7)
    { DBG_LOG(1,0,("out of range!\n")); return; }

  /* interrupt not enabled? */
  if (self->enable & mask) {
    DBG_LOG(1,0,("is not enabled\n"));
    /* self->pending &= ~mask; */
    /* self->in_service &= ~mask; */
    return;
  }

  /* same interrupt not yet acknowledged ? */
  if (self->in_service & mask) {
    DBG_LOG(1,0,("is already in service\n"));
    /* save request mask for later HACK! */
    self->in_service &= ~mask;
    self->pending |= mask;
    return;
  }

  /* higher priority interrupt in service? */
  if (self->in_service & (mask-1)) {
    DBG_LOG(1,0,("is lower priority\n"));
    self->pending |= mask; /* save request mask for later */
    return;
  }

  /* remove from the requested INTs */
  self->pending &= ~mask;

  /* mask interrupt until acknowledged */
  self->in_service |= mask;

  irq += self->base;
  DBG_LOG(1,0,("INT %02X\n", irq));
#ifdef PINMAME
  cpu_irq_line_vector_w(self->cpuno,self->irqno,irq);
  cpu_set_irq_line(self->cpuno,self->irqno,HOLD_LINE);
#else /* PINMAME */
  cpu_irq_line_vector_w(0,0,irq);
  cpu_set_irq_line(0,0,HOLD_LINE);
#endif /* PINMAME */
}

static int pic8259_irq_pending(PIC8259 *self, int irq) {
  UINT8 mask = 1 << irq;
  return (self->pending & mask) ? 1 : 0;
}

static void pic8259_w(PIC8259 *self, offs_t offset, data8_t data ) {
  switch( offset ) {
    case 0: /* PIC acknowledge IRQ */
      if (data & 0x10) { /* write ICW1 ? */
        self->icw2 = 1;
        self->icw3 = 1;
        self->level_trig_mode = (data >> 3) & 1;
        self->vector_size = (data >> 2) & 1;
        self->cascade = ((data >> 1) & 1) ^ 1;
        if (self->cascade == 0) self->icw3 = 0;
        self->icw4 = data & 1;
        DBG_LOG(1,"PIC_ack_w",("$%02x: ICW1, icw4 %d, cascade %d, vec size %d, ltim %d\n",
                data, self->icw4, self->cascade, self->vector_size, self->level_trig_mode));
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
            self->special = 1;
            self->input = self->pending;
            break;
          case 0x03:
            DBG_LOG(1,0,(", read in-service register"));
            self->special = 1;
            self->input = self->in_service & ~self->enable;
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
            self->prio = 0;
            break;
          case 0x20:
            DBG_LOG(1,0,(" nonspecific EOI\n"));
            for (n = 0, mask = 1<<self->prio; n < 8; n++, mask = (mask<<1) | (mask>>7)) {
              if (self->in_service & mask) {
                self->in_service &= ~mask;
                self->pending &= ~mask;
                break;
              }
            } // for
            break;
          case 0x40:
            DBG_LOG(1,0,(" OCW2 NOP\n")); break;
          case 0x60:
            DBG_LOG(1,0,(" OCW2 specific EOI%d\n", n));
            if (self->in_service & mask) {
              self->in_service &= ~mask;
              self->pending &= ~mask;
            }
            break;
          case 0x80:
            DBG_LOG(1,0,(" OCW2 rotate auto EOI set\n"));
            self->prio = (self->prio + 1) & 7;
            break;
          case 0xa0:
            DBG_LOG(1,0,(" OCW2 rotate on nonspecific EOI\n"));
            for (n = 0, mask = 1<<self->prio; n < 8; n++, mask = (mask<<1) | (mask>>7)) {
              if (self->in_service & mask) {
                self->in_service &= ~mask;
                self->pending &= ~mask;
                self->prio = (self->prio + 1) & 7;
                break;
              }
            }
            break;
          case 0xc0:
            DBG_LOG(1,0,(" OCW2 set priority\n"));
            self->prio = n & 7;
            break;
          case 0xe0:
            DBG_LOG(1,0,(" OCW2 rotate on specific EOI%d\n", n));
            if (self->in_service & mask) {
              self->in_service &= ~mask;
              self->pending &= ~mask;
              self->prio = (self->prio + 1) & 7;
            }
            break;
        } // switch
      }
      break;
    case 1: /* PIC ICW2,3,4 or OCW1 */
      if (self->icw2) {
        self->base = data & 0xf8;
        DBG_LOG(1,"PIC_enable_w",("$%02x: ICW2 (base)\n", self->base));
        self->icw2 = 0;
      }
      else if (self->icw3) {
        self->slave = data;
        DBG_LOG(1,"PIC_enable_w",("$%02x: ICW3 (slave)\n", self->slave));
        self->icw3 = 0;
      }
      else if (self->icw4) {
        self->nested = (data >> 4) & 1;
        self->mode = (data >> 2) & 3;
        self->auto_eoi = (data >> 1) & 1;
        self->x86 = data & 1;
        DBG_LOG(1,"PIC_enable_w",("$%02x: ICW4 x86 mode %d, auto EOI %d, mode %d, nested %d\n",
                data, self->x86, self->auto_eoi, self->mode, self->nested));
        self->icw4 = 0;
      }
      else {
        DBG_LOG(1,"PIC_enable_w",("$%02x: OCW1 enable\n", data));
        self->enable = data;
        self->in_service &= data;
        self->pending &= data;
      }
      break;
  } // switch
  if (self->pending & 0x01) pic8259_issue_irq(self, 0);
  if (self->pending & 0x02) pic8259_issue_irq(self, 1);
  if (self->pending & 0x04) pic8259_issue_irq(self, 2);
  if (self->pending & 0x08) pic8259_issue_irq(self, 3);
  if (self->pending & 0x10) pic8259_issue_irq(self, 4);
  if (self->pending & 0x20) pic8259_issue_irq(self, 5);
  if (self->pending & 0x40) pic8259_issue_irq(self, 6);
  if (self->pending & 0x80) pic8259_issue_irq(self, 7);
}

static int pic8259_r(PIC8259 *self, offs_t offset) {
  int data = 0xff;
  switch (offset) {
    case 0: /* PIC acknowledge IRQ */
      if (self->special) {
        self->special = 0;
        data = self->input;
        DBG_LOG(1,"PIC_ack_r",("$%02x read special\n", data));
      }
      else
        DBG_LOG(1,"PIC_ack_r",("$%02x\n", data));
      break;
    case 1: /* PIC mask register */
      data = self->enable;
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
