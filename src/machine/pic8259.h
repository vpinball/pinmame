#include "driver.h"

#ifdef __cplusplus
extern "C" {
#endif
#ifdef PINMAME
extern void pic8259_0_config(int cpuno, int irqno);
extern void pic8259_1_config(int cpuno, int irqno);
#endif /* PINMAME */
extern WRITE_HANDLER(pic8259_0_w);
extern READ_HANDLER(pic8259_0_r);
extern WRITE_HANDLER(pic8259_1_w);
extern READ_HANDLER(pic8259_1_r);

extern void pic8259_0_issue_irq(int irq);
extern void pic8259_1_issue_irq(int irq);

int pic8259_0_irq_pending(int irq);
int pic8259_1_irq_pending(int irq);

#ifdef __cplusplus
}
#endif
