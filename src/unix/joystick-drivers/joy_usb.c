/*
 * X-Mame USB HID joystick driver for NetBSD.
 *
 * Written by Krister Walfridsson <cato@df.lth.se>
 */
#include "xmame.h"
#include "devices.h"

struct rc_option joy_usb_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

#ifdef USB_JOYSTICK

#if !defined(__ARCH_netbsd) && !defined(__ARCH_freebsd)
#error "USB joysticks are only supported under NetBSD and FreeBSD. "
   "Patches to support other archs are welcome ;)"
#endif

#if defined(HAVE_USBHID_H) || defined(HAVE_LIBUSBHID_H)
#	ifdef HAVE_USBHID_H
#		include <usbhid.h>
#	endif
#	ifdef HAVE_LIBUSBHID_H
#		include <libusbhid.h>
#	endif
#else
#	ifdef __ARCH_netbsd
#		include <usb.h>
#	endif
#	ifdef __ARCH_freebsd
#		include <libusb.h>
#	endif
#endif

#ifdef __ARCH_freebsd
#include <osreldate.h>
#include <sys/ioctl.h>
#endif

#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>

struct priv_joydata_struct
{
  struct hid_item *hids;
  int dlen;
  int offset;
  char *data_buf;
} priv_joy_data[JOY];

static int joy_initialize_hid(int i);
static void joy_usb_poll(void);
static int joy_read(int fd, int i);



void joy_usb_init(void)
{
  int i;
  char devname[20];

  fprintf(stderr_file, "USB joystick interface initialization...\n");

  for (i = 0; i < JOY; i++)
    {
      sprintf(devname, "/dev/uhid%d", i);
      if ((joy_data[i].fd = open(devname, O_RDONLY | O_NONBLOCK)) != -1)
	{
	  if (!joy_initialize_hid(i))
	    {
	      close(joy_data[i].fd);
	      joy_data[i].fd = -1;
	    }
	}
    }

  joy_poll_func = joy_usb_poll;
}



static int joy_initialize_hid(int i)
{
  int size, is_joystick, report_id = 0;
  struct hid_data *d;
  struct hid_item h;
  report_desc_t rd;

  if ((rd = hid_get_report_desc(joy_data[i].fd)) == 0)
    {
      fprintf(stderr_file, "error: /dev/uhid%d: %s", i, strerror(errno));
      return FALSE;
    }

  priv_joy_data[i].hids = NULL;

#if defined(HAVE_USBHID_H) || defined(HAVE_LIBUSBHID_H)
#if defined(__ARCH_netbsd) || (defined(__ARCH_freebsd) && __FreeBSD_version > 500000)
  if (ioctl(joy_data[i].fd, USB_GET_REPORT_ID, &report_id) < 0)
    {
      fprintf(stderr_file, "error: /dev/uhid%d: %s", i, strerror(errno));
      return FALSE;
    }
#endif

  size = hid_report_size(rd, hid_input, report_id);
  priv_joy_data[i].offset = 0;
#else
  size = hid_report_size(rd, hid_input, &report_id);
  priv_joy_data[i].offset = (report_id != 0);
#endif
  if ((priv_joy_data[i].data_buf = malloc(size)) == NULL)
    {
      fprintf(stderr_file, "error: couldn't malloc %d bytes\n", size);
      hid_dispose_report_desc(rd);
      return FALSE;
    }
  priv_joy_data[i].dlen = size;

  is_joystick = 0;
#if defined(HAVE_USBHID_H)
  for (d = hid_start_parse(rd, 1 << hid_input, report_id);
       hid_get_item(d, &h); )
#else
  for (d = hid_start_parse(rd, 1 << hid_input); hid_get_item(d, &h); )
#endif
    {
      int axis, usage, page, interesting_hid;

      page = HID_PAGE(h.usage);
      usage = HID_USAGE(h.usage);

      /* This test is somewhat too simplistic, but this is how MicroSoft
       * does, so I guess it works for all joysticks/game pads. */
      is_joystick = is_joystick ||
	(h.kind == hid_collection &&
	 page == HUP_GENERIC_DESKTOP &&
	 (usage == HUG_JOYSTICK || usage == HUG_GAME_PAD));

      if (h.kind != hid_input)
	continue;

      if (!is_joystick)
	continue;

      interesting_hid = TRUE;
      if (page == HUP_GENERIC_DESKTOP)
	{
	  if (usage == HUG_X || usage == HUG_RX)
	    axis = 0;
	  else if (usage == HUG_Y || usage == HUG_RY)
	    axis = 1;
	  else if (usage == HUG_Z || usage == HUG_RZ)
	    axis = 2;
	  else
	    interesting_hid = FALSE;

	  if (interesting_hid)
	    {
	      joy_data[i].axis[axis].min = h.logical_minimum;
	      joy_data[i].axis[axis].max = h.logical_maximum;

	      /* Set the theoretical center. This will be used in case
	       * the heuristic below fails. */
	      joy_data[i].axis[axis].center =
		(h.logical_minimum + h.logical_maximum) / 2;

	      if (joy_data[i].num_axis < (axis + 1))
		joy_data[i].num_axis = axis + 1;
	    }
	}
      else if (page == HUP_BUTTON)
	{
	  interesting_hid = (usage > 0) && (usage <= JOY_BUTTONS);

	  if (interesting_hid && usage > joy_data[i].num_buttons)
	    joy_data[i].num_buttons = usage;
	}

      if (interesting_hid)
	{
	  h.next = priv_joy_data[i].hids;
	  priv_joy_data[i].hids = malloc(sizeof *(priv_joy_data[i].hids));
	  if (priv_joy_data[i].hids == NULL)
	    {
	      fprintf(stderr_file, "error: Not enough memory for joystick. "
		      "Your joystick may fail to work correctly.\n");
	      break;
	    }
	  *(priv_joy_data[i].hids) = h;
	}
    }
  hid_end_parse(d);

  if (priv_joy_data[i].hids != NULL)
    {
      /* We'll approximate the center with the current joystick value if
       * that can be read (some HID devices returns no data if the state
       * has not changed since the last time it was read.) */
      if (joy_read(joy_data[i].fd, i))
	{
	  joy_data[i].axis[0].center = joy_data[i].axis[0].val;
	  joy_data[i].axis[1].center = joy_data[i].axis[1].val;
	  joy_data[i].axis[2].center = joy_data[i].axis[2].val;
	}
      else
	{
	  /* Assume that the joystick is positioned in the center.
	   * This is needed, or else the system will think that the
	   * joystick is in position left/up (or something) until it
	   * is moved the first time. */
	  joy_data[i].axis[0].val = joy_data[i].axis[0].center;
	  joy_data[i].axis[1].val = joy_data[i].axis[1].center;
	  joy_data[i].axis[2].val = joy_data[i].axis[2].center;
	}

      /* Approximate min/max values. Observe that we cannot use the
       * max/min values that the HID reports, since that is theoretical
       * values that may be wrong for analogs joystics (especially if
       * you have a joystick -> USB adaptor.) We cannot use greater delta
       * values than +/- 1, since it is OK for a gamepad (or my USB TAC 2)
       * to reports directions as center +/- 1. */
      joy_data[i].axis[0].min = joy_data[i].axis[0].center - 1;
      joy_data[i].axis[1].min = joy_data[i].axis[1].center - 1;
      joy_data[i].axis[2].min = joy_data[i].axis[2].center - 1;
      joy_data[i].axis[0].max = joy_data[i].axis[0].center + 1;
      joy_data[i].axis[1].max = joy_data[i].axis[1].center + 1;
      joy_data[i].axis[2].max = joy_data[i].axis[2].center + 1;
    }

  return (priv_joy_data[i].hids != NULL);
}



static void joy_usb_poll(void)
{
  int i;

  for (i = 0; i < JOY; i++)
    {
      if (joy_data[i].fd >= 0)
	joy_read(joy_data[i].fd, i);
    }

   /* Evaluate joystick movements. */
   joy_evaluate_moves ();
}



static int joy_read(int fd, int i)
{
  int len, axis, usage, page, d;
  struct hid_item *h;

  len = read(fd, priv_joy_data[i].data_buf, priv_joy_data[i].dlen);
  if (len != priv_joy_data[i].dlen)
    return FALSE;

  for (h = priv_joy_data[i].hids; h; h = h->next)
    {
      d = hid_get_data(priv_joy_data[i].data_buf + priv_joy_data[i].offset, h);

      page = HID_PAGE(h->usage);
      usage = HID_USAGE(h->usage);

      if (page == HUP_GENERIC_DESKTOP)
	{
	  if (usage == HUG_X || usage == HUG_RX)
	    axis = 0;
	  else if (usage == HUG_Y || usage == HUG_RY)
	    axis = 1;
	  else
	    axis = 2;

	  joy_data[i].axis[axis].val = d;
	}
      else if (page == HUP_BUTTON)
	{
	  joy_data[i].buttons[usage - 1] = (d == h->logical_maximum);
	}
    }

  return TRUE;
}
#endif
