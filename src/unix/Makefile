OBJDIR = ../../$(OBJ)/unix.$(DISPLAY_METHOD)
MY_TARGET = $(OBJDIR)/osdepend.a

##############################################################################
# *** object listings
##############################################################################

# common objs
COMMON_OBJS  =  \
	$(OBJDIR)/main.o $(OBJDIR)/sound.o \
	$(OBJDIR)/keyboard.o $(OBJDIR)/devices.o \
	$(OBJDIR)/video.o $(OBJDIR)/mode.o \
	$(OBJDIR)/fileio.o $(OBJDIR)/dirio.o $(OBJDIR)/config.o \
	$(OBJDIR)/fronthlp.o $(OBJDIR)/ident.o $(OBJDIR)/network.o \
	$(OBJDIR)/snprintf.o $(OBJDIR)/nec765_dummy.o $(OBJDIR)/effect.o \
	$(OBJDIR)/ticker.o $(OBJDIR)/parallel.o

# sysdep objs
SYSDEP_DIR  = $(OBJDIR)/sysdep
SYSDEP_OBJS = $(SYSDEP_DIR)/rc.o $(SYSDEP_DIR)/misc.o \
   $(SYSDEP_DIR)/plugin_manager.o $(SYSDEP_DIR)/sound_stream.o \
   $(SYSDEP_DIR)/sysdep_palette.o $(SYSDEP_DIR)/sysdep_dsp.o \
   $(SYSDEP_DIR)/sysdep_mixer.o
   
# video driver objs per display method
VID_DIR = $(OBJDIR)/video-drivers
VID_OBJS.x11    = $(VID_DIR)/xinput.o $(VID_DIR)/xil.o \
	$(VID_DIR)/x11_window.o \
	$(VID_DIR)/xf86_dga1.o $(VID_DIR)/xf86_dga2.o $(VID_DIR)/xf86_dga.o
VID_OBJS.xgl    = $(VID_DIR)/gltool.o $(VID_DIR)/glxtool.o $(VID_DIR)/glcaps.o \
		  $(VID_DIR)/glvec.o $(VID_DIR)/glgen.o $(VID_DIR)/glexport.o \
		  $(VID_DIR)/glcab.o $(VID_DIR)/gljpg.o \
		  $(VID_DIR)/xinput.o
VID_OBJS.xfx    = $(VID_DIR)/fxgen.o $(VID_DIR)/xinput.o
VID_OBJS.svgalib = $(VID_DIR)/svgainput.o
VID_OBJS.svgafx = $(VID_DIR)/svgainput.o $(VID_DIR)/fxgen.o
VID_OBJS.openstep = $(VID_DIR)/openstep_input.o
VID_OBJS.photon2 = $(VID_DIR)/photon2_input.o \
	$(VID_DIR)/photon2_window.o \
	$(VID_DIR)/photon2_overlay.o
VID_OBJS = $(VID_DIR)/$(DISPLAY_METHOD).o $(VID_OBJS.$(DISPLAY_METHOD))

# sound driver objs per arch
DSP_DIR = $(OBJDIR)/sysdep/dsp-drivers
MIXER_DIR = $(OBJDIR)/sysdep/mixer-drivers
SOUND_OBJS.linux   = $(DSP_DIR)/oss.o $(MIXER_DIR)/oss.o $(DSP_DIR)/alsa.o
SOUND_OBJS.freebsd = $(DSP_DIR)/oss.o $(MIXER_DIR)/oss.o
SOUND_OBJS.netbsd  = $(DSP_DIR)/netbsd.o
#SOUND_OBJS.openbsd = $(DSP_DIR)/oss.o $(MIXER_DIR)/oss.o
SOUND_OBJS.openbsd = $(DSP_DIR)/netbsd.o 
SOUND_OBJS.solaris = $(DSP_DIR)/solaris.o $(MIXER_DIR)/solaris.o
SOUND_OBJS.next    = $(DSP_DIR)/soundkit.o
SOUND_OBJS.macosx  = $(DSP_DIR)/coreaudio.o
SOUND_OBJS.nto     = $(DSP_DIR)/io-audio.o
SOUND_OBJS.irix    = $(DSP_DIR)/irix.o
SOUND_OBJS.irix_al = $(DSP_DIR)/irix_al.o
SOUND_OBJS.beos    =
SOUND_OBJS.generic =
#these need to be converted to plugins first
#SOUND_OBJS.aix     = $(DSP_DIR)/aix.o
SOUND_OBJS = $(SOUND_OBJS.$(ARCH)) $(DSP_DIR)/esound.o $(DSP_DIR)/artssound.o \
    $(DSP_DIR)/arts.o $(DSP_DIR)/sdl.o $(DSP_DIR)/waveout.o

# joystick objs
JOY_DIR  = $(OBJDIR)/joystick-drivers
JOY_OBJS = $(JOY_DIR)/joy_i386.o $(JOY_DIR)/joy_pad.o $(JOY_DIR)/joy_x11.o \
    $(JOY_DIR)/joy_usb.o $(JOY_DIR)/joy_ps2.o $(JOY_DIR)/joy_SDL.o \
    $(JOY_DIR)/XInputDevices.o

# framskip objs
FRAMESKIP_DIR  = $(OBJDIR)/frameskip-drivers
FRAMESKIP_OBJS = $(FRAMESKIP_DIR)/dos.o $(FRAMESKIP_DIR)/barath.o

# all objs
OBJS = $(COMMON_OBJS) $(SYSDEP_OBJS) $(VID_OBJS) $(SOUND_OBJS) $(JOY_OBJS) \
   $(FRAMESKIP_OBJS)

# obj dirs
OBJDIRS = $(OBJDIR) $(SYSDEP_DIR) $(DSP_DIR) $(MIXER_DIR) $(VID_DIR) \
   $(JOY_DIR) $(FRAMESKIP_DIR)


##############################################################################
# *** cflags
##############################################################################

# per arch
CFLAGS.linux      = -DSYSDEP_DSP_OSS -DSYSDEP_MIXER_OSS -DHAVE_SNPRINTF -DHAVE_VSNPRINTF
CFLAGS.freebsd    = -DSYSDEP_DSP_OSS -DSYSDEP_MIXER_OSS -DHAVE_SNPRINTF -DHAVE_VSNPRINTF
CFLAGS.netbsd     = -DSYSDEP_DSP_NETBSD -DHAVE_SNPRINTF -DHAVE_VSNPRINTF
#CFLAGS.openbsd    = -DSYSDEP_DSP_OSS -DSYSDEP_MIXER_OSS -DHAVE_SNPRINTF -DHAVE_VSNPRINTF
CFLAGS.openbsd    = -DSYSDEP_DSP_NETBSD -DHAVE_SNPRINTF -DHAVE_VSNPRINTF
CFLAGS.solaris    = -DSYSDEP_DSP_SOLARIS -DSYSDEP_MIXER_SOLARIS
CFLAGS.next       = -DSYSDEP_DSP_SOUNDKIT -DBSD43
CFLAGS.macosx     = -DSYSDEP_DSP_COREAUDIO -DHAVE_SNPRINTF -DHAVE_VSNPRINTF
CFLAGS.nto        = -DSYSDEP_DSP_ALSA -DSYSDEP_MIXER_ALSA
CFLAGS.irix       = -DSYSDEP_DSP_IRIX -DHAVE_SNPRINTF
CFLAGS.irix_al    = -DSYSDEP_DSP_IRIX -DHAVE_SNPRINTF
CFLAGS.beos       = `sdl-config --cflags` -DSYSDEP_DSP_SDL
CFLAGS.generic    =
#these need to be converted to plugins first
#CFLAGS.aix        = -DSYSDEP_DSP_AIX -I/usr/include/UMS -I/usr/lpp/som/include

#all cflags
MY_CFLAGS = -I. -I.. -I../.. -I../../mess $(CFLAGS) $(CFLAGS.$(ARCH)) 


##############################################################################
# *** begin of the real makefile
##############################################################################
all:	$(OBJDIRS) $(MY_TARGET)

clean:
	rm -f $(MY_TARGET) $(OBJS)

$(MY_TARGET): $(OBJS)
	$(CC_COMMENT) @echo 'Archiving $@ ...'
	$(CC_COMPILE) ar $(AR_OPTS) $@ $(OBJS)
	$(CC_COMPILE) $(RANLIB) $@

$(OBJDIRS):
	-mkdir $@

$(OBJDIR)/%.o: %.c xmame.h
	$(CC_COMMENT) @echo 'Compiling src/unix/$< ...'
	$(CC_COMPILE) $(CC) $(MY_CFLAGS) -o $@ -c $<

$(OBJDIR)/%.o: %.m xmame.h
	$(CC_COMMENT) @echo 'Compiling src/unix/$< ...'
	$(CC_COMPILE) $(CC) $(MY_CFLAGS) -o $@ -c $<

$(OBJDIR)/video.o: video-drivers/glmame.h effect.h
$(OBJDIR)/effect.o: effect.h

$(VID_DIR)/x11_window.o: video-drivers/blit.h video-drivers/blit_core.h effect.h
$(VID_DIR)/svgalib.o: video-drivers/blit.h video-drivers/blit_core.h video-drivers/twkmodes.h video-drivers/twkuser.c effect.h
$(VID_DIR)/xf86_dga1.o: video-drivers/xf86_dga1.c video-drivers/blit.h video-drivers/blit_core.h effect.h
$(VID_DIR)/xf86_dga2.o: video-drivers/xf86_dga2.c video-drivers/blit.h video-drivers/blit_core.h effect.h
$(VID_DIR)/SDL.o: video-drivers/blit.h video-drivers/blit_core.h effect.h
$(VID_DIR)/xinput.o:  video-drivers/xkeyboard.h video-drivers/glmame.h
$(VID_DIR)/xinput.o:  video-drivers/xkeyboard.h video-drivers/glmame.h
$(VID_DIR)/xinput.o:  video-drivers/xkeyboard.h
$(VID_DIR)/glgen.o:   video-drivers/glmame.h video-drivers/gltool.h \
	              video-drivers/glxtool.h 
$(VID_DIR)/glvec.o:   video-drivers/glmame.h video-drivers/gltool.h \
	              video-drivers/glxtool.h
$(VID_DIR)/glcab.o:   video-drivers/glmame.h video-drivers/gltool.h \
	              video-drivers/glxtool.h
$(VID_DIR)/gltool.o:  video-drivers/gltool.h \
	              video-drivers/glxtool.h video-drivers/glcaps.h
$(VID_DIR)/glcaps.o:  video-drivers/gltool.h 
$(VID_DIR)/glxtool.o: video-drivers/gltool.h \
	              video-drivers/glxtool.h video-drivers/glcaps.h
$(VID_DIR)/xgl.o:     video-drivers/glmame.h video-drivers/gltool.h \
	              video-drivers/glxtool.h
$(VID_DIR)/gltool.h:  video-drivers/gl-disp-var.h video-drivers/glu-disp-var.h

