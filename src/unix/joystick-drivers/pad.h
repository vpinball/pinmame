/*
 * TOWNS PAD
 *    By S.Nomura
 */
#ifndef TRUE
# define TRUE (1)
# define FALSE (0)
#endif

#define PAD_DEV_MASK    0x07
#define PAD_MODE_MASK   0x38
#define PAD_MODE_SHIFT  3

  /* modes */
#define PAD_NORMAL      0
#define PAD_MARTY       1
#define PAD_6BUTTON     2

struct pad_event {
  unsigned long buttons;
  unsigned long stamp;
};

#define PAD_FWD    0x0001
#define PAD_BACK   0x0002
#define PAD_LEFT   0x0004
#define PAD_RIGHT  0x0008
#define PAD_RUN    0x0010
#define PAD_SEL    0x0020
#define PAD_ZOOM   0x0040
#define PAD_A	   0x0100
#define PAD_B	   0x0200
#define PAD_C	   0x0400
#define PAD_X	   0x0800
#define PAD_Y	   0x1000
#define PAD_Z	   0x2000

#define PADIOCSETPARAM  0x5001

#define PAD_QUE_SIZE 50

struct pad_status {
  int active;
  int port;
  int trig, com, mode;
  unsigned long button, last_button;
  struct inode *inode;

  /* Queue */
  struct wait_queue *proc;
  struct pad_event ev[PAD_QUE_SIZE];
  struct pad_event *put, *get, *limit;
  int count, ovf;
};

/* I/O ports */
#define PAD_CTRL_PORT  0x04d6
#define PAD1_PORT      0x04d0
#define PAD2_PORT      0x04d2

 /* control */
#define PAD1_TRIG      0x03
#define PAD1_COM       0x10
#define PAD2_TRIG      0x0c
#define PAD2_COM       0x20

 /* value */
#define _PAD_FWD       0x01
#define _PAD_BACK      0x02
#define _PAD_LEFT      0x04
#define _PAD_RIGHT     0x08
#define _PAD_A         0x10
#define _PAD_B         0x20
#define _PAD_COM       0x40
#define _PAD_C         0x08
#define _PAD_X         0x04
#define _PAD_Y         0x02
#define _PAD_Z         0x01
