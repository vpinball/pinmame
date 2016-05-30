/* Form definition file generated with fdesign. */

#include "forms.h"
#include <stdlib.h>
#include "xmameload.h"

FD_xmame_main_form *create_form_xmame_main_form(void)
{
  FL_OBJECT *obj;
  FD_xmame_main_form *fdui = (FD_xmame_main_form *) fl_calloc(1, sizeof(*fdui));

  fdui->xmame_main_form = fl_bgn_form(FL_NO_BOX, 670, 300);
  obj = fl_add_box(FL_UP_BOX,0,0,670,300,"");
  fdui->file_menu = obj = fl_add_menu(FL_PULLDOWN_MENU,20,10,70,20,"File");
    fl_set_object_boxtype(obj,FL_FRAME_BOX);
  fdui->gamlist_browser = obj = fl_add_browser(FL_SELECT_BROWSER,20,60,160,220,"GameList");
    fl_set_object_lalign(obj,FL_ALIGN_TOP_LEFT);
  fdui->video_atributes = obj = fl_add_labelframe(FL_ENGRAVED_FRAME,190,110,160,170,"Video Options");
  fdui->audio_options = obj = fl_add_labelframe(FL_ENGRAVED_FRAME,360,130,130,150,"Audio Options");
  fdui->io_options = obj = fl_add_labelframe(FL_ENGRAVED_FRAME,500,130,150,150,"Input/Output");
  fdui->mamedir_input = obj = fl_add_input(FL_NORMAL_INPUT,250,40,240,20,"ROMs Dir");
  fdui->spooldir_input = obj = fl_add_input(FL_NORMAL_INPUT,250,70,240,20,"HIScores Dir");
  fdui->dsiplayname_input = obj = fl_add_input(FL_NORMAL_INPUT,240,120,90,20,"Display");
  fdui->xscale_input = obj = fl_add_counter(FL_SIMPLE_COUNTER,200,160,60,20,"X-Scale");
  fdui->yscale_input = obj = fl_add_counter(FL_SIMPLE_COUNTER,200,200,60,20,"Y-Scale");
  fdui->frameskip_input = obj = fl_add_counter(FL_SIMPLE_COUNTER,200,240,60,20,"Skip Frames");
  fdui->use_xsync_input = obj = fl_add_round3dbutton(FL_PUSH_BUTTON,320,150,30,30,"DoXSync");
    fl_set_object_lalign(obj,FL_ALIGN_LEFT);
  fdui->use_mitshm_input = obj = fl_add_round3dbutton(FL_PUSH_BUTTON,320,180,30,30,"MitShmem");
    fl_set_object_lalign(obj,FL_ALIGN_LEFT);
  fdui->use_private_cmap_input = obj = fl_add_round3dbutton(FL_PUSH_BUTTON,320,210,30,30,"PrivCmap");
    fl_set_object_lalign(obj,FL_ALIGN_LEFT);
  fdui->use_truecolor_input = obj = fl_add_round3dbutton(FL_PUSH_BUTTON,320,240,30,30,"TrueColor");
    fl_set_object_lalign(obj,FL_ALIGN_LEFT);
  fdui->use_mouse_input = obj = fl_add_round3dbutton(FL_PUSH_BUTTON,510,140,30,30,"Mouse");
    fl_set_object_lalign(obj,FL_ALIGN_BOTTOM);
  fdui->use_joystick_input = obj = fl_add_round3dbutton(FL_PUSH_BUTTON,560,140,30,30,"JoyStick");
    fl_set_object_lalign(obj,FL_ALIGN_BOTTOM);
  fdui->use_trakball_input = obj = fl_add_round3dbutton(FL_PUSH_BUTTON,610,140,30,30,"TrakBall");
    fl_set_object_lalign(obj,FL_ALIGN_BOTTOM);
  fdui->joyfilter_input = obj = fl_add_counter(FL_SIMPLE_COUNTER,590,220,50,20,"Joy Filter");
    fl_set_object_lalign(obj,FL_ALIGN_LEFT);
  fdui->x11_joyname_input = obj = fl_add_input(FL_NORMAL_INPUT,570,250,70,20,"X11 JoyName");
  fdui->use_audio_input = obj = fl_add_round3dbutton(FL_PUSH_BUTTON,460,130,30,30,"Use Audio");
    fl_set_object_lalign(obj,FL_ALIGN_LEFT);
  fdui->use_fm_input = obj = fl_add_round3dbutton(FL_PUSH_BUTTON,460,160,30,30,"Use FM");
    fl_set_object_lalign(obj,FL_ALIGN_LEFT);
  fdui->video_menu = obj = fl_add_menu(FL_PULLDOWN_MENU,120,10,70,20,"Video");
    fl_set_object_boxtype(obj,FL_FRAME_BOX);
  fdui->audio_menu = obj = fl_add_menu(FL_PULLDOWN_MENU,220,10,70,20,"Audio");
    fl_set_object_boxtype(obj,FL_FRAME_BOX);
  fdui->io_menu = obj = fl_add_menu(FL_PULLDOWN_MENU,320,10,70,20,"Devices");
    fl_set_object_boxtype(obj,FL_FRAME_BOX);
  fdui->audio_device_input = obj = fl_add_input(FL_NORMAL_INPUT,410,250,70,20,"AudioDev");
  fdui->sample_freq_input = obj = fl_add_counter(FL_SIMPLE_COUNTER,420,190,60,20,"SampleFreq");
    fl_set_object_lalign(obj,FL_ALIGN_LEFT);
  fdui->timer_freq_input = obj = fl_add_counter(FL_SIMPLE_COUNTER,420,220,60,20,"TimerFreq");
    fl_set_object_lalign(obj,FL_ALIGN_LEFT);
  fdui->exe_menu = obj = fl_add_menu(FL_PULLDOWN_MENU,420,10,70,20,"Execute");
    fl_set_object_boxtype(obj,FL_FRAME_BOX);
  fl_end_form();

  fdui->xmame_main_form->fdui = fdui;

  return fdui;
}
/*---------------------------------------*/

