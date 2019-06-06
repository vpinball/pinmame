#ifndef LISY_USBSER_H
#define LISY_USBSER_H

int lisy_usb_init(void);
int lisy_usb_send_str_to_disp(unsigned char num, char *str);
unsigned char lisy_usb_ask_for_changed_switch(void);
unsigned char lisy_usb_get_switch_status( unsigned char number);
void lisy_usb_lamp_ctrl(int lamp_no,unsigned char action);
void lisy_usb_sol_ctrl(int sol_no,unsigned char action);

#endif  /* LISY_USBSER_H */

