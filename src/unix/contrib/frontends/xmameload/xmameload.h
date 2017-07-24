#ifndef FD_xmame_main_form_h_
#define FD_xmame_main_form_h_
/* Header file generated with fdesign. */

/**** Callback routines ****/



/**** Forms and Objects ****/

typedef struct {
	FL_FORM *xmame_main_form;
	void *vdata;
	long ldata;
	FL_OBJECT *file_menu;
	FL_OBJECT *gamlist_browser;
	FL_OBJECT *video_atributes;
	FL_OBJECT *audio_options;
	FL_OBJECT *io_options;
	FL_OBJECT *mamedir_input;
	FL_OBJECT *spooldir_input;
	FL_OBJECT *dsiplayname_input;
	FL_OBJECT *xscale_input;
	FL_OBJECT *yscale_input;
	FL_OBJECT *frameskip_input;
	FL_OBJECT *use_xsync_input;
	FL_OBJECT *use_mitshm_input;
	FL_OBJECT *use_private_cmap_input;
	FL_OBJECT *use_truecolor_input;
	FL_OBJECT *use_mouse_input;
	FL_OBJECT *use_joystick_input;
	FL_OBJECT *use_trakball_input;
	FL_OBJECT *joyfilter_input;
	FL_OBJECT *x11_joyname_input;
	FL_OBJECT *use_audio_input;
	FL_OBJECT *use_fm_input;
	FL_OBJECT *video_menu;
	FL_OBJECT *audio_menu;
	FL_OBJECT *io_menu;
	FL_OBJECT *audio_device_input;
	FL_OBJECT *sample_freq_input;
	FL_OBJECT *timer_freq_input;
	FL_OBJECT *exe_menu;
} FD_xmame_main_form;

extern FD_xmame_main_form * create_form_xmame_main_form(void);

#endif /* FD_xmame_main_form_h_ */
