MAMEVER=7300
PINMAMESRC=wpc
ISVER$(MAMEVER)=1
#
# PinMAME specific flags
#
PINOBJ=$(OBJ)/$(PINMAMESRC)
CFLAGS += -Isrc/$(PINMAMESRC)
DEFS += -DPINMAME=1 -DMAMEVER=$(MAMEVER)
# Used in GUI version (PinMAME32)
DEFS += -DMAME32NAME=\"PINMAME32\" -DMAMENAME=\"PINMAME\"
TOOLS=

#
# Common stuff
#
DRVLIBS = $(PINOBJ)/sim.o $(PINOBJ)/core.o $(OBJ)/allgames.a
DRVLIBS += $(PINOBJ)/vpintf.o $(PINOBJ)/snd_cmd.o $(PINOBJ)/wpcsam.o
DRVLIBS += $(PINOBJ)/sndbrd.o
DRVLIBS += $(OBJ)/machine/4094.o
DRVLIBS += $(OBJ)/sound/wavwrite.o

COREOBJS += $(PINOBJ)/driver.o $(OBJ)/cheat.o $(PINOBJ)/mech.o

# taken from MESS
DRVLIBS += $(OBJ)/vidhrdw/crtc6845.o $(OBJ)/machine/6530riot.o
DRVLIBS += $(OBJ)/vidhrdw/tms9928a.o $(OBJ)/machine/pic8259.o

#
# Core drivers
#
DRVLIBS += $(PINOBJ)/s4.o $(PINOBJ)/s6.o $(PINOBJ)/s7.o $(PINOBJ)/s11.o
DRVLIBS += $(PINOBJ)/wpc.o $(PINOBJ)/wmssnd.o
DRVLIBS += $(PINOBJ)/dedmd.o $(PINOBJ)/desound.o
DRVLIBS += $(PINOBJ)/gts3.o $(PINOBJ)/gts3dmd.o
DRVLIBS += $(PINOBJ)/se.o
DRVLIBS += $(PINOBJ)/gts80.o $(PINOBJ)/gts80s.o
DRVLIBS += $(PINOBJ)/by35.o $(PINOBJ)/by35snd.o $(PINOBJ)/stsnd.o
DRVLIBS += $(PINOBJ)/by6803.o $(PINOBJ)/by68701.o
DRVLIBS += $(PINOBJ)/byvidpin.o
DRVLIBS += $(PINOBJ)/capcom.o $(PINOBJ)/capcoms.o
DRVLIBS += $(PINOBJ)/hnks.o
DRVLIBS += $(PINOBJ)/zac.o $(PINOBJ)/zacproto.o $(PINOBJ)/zacsnd.o
DRVLIBS += $(PINOBJ)/gp.o $(PINOBJ)/gpsnd.o
DRVLIBS += $(PINOBJ)/atari.o $(PINOBJ)/atarisnd.o
DRVLIBS += $(PINOBJ)/taito.o $(PINOBJ)/taitos.o
DRVLIBS += $(PINOBJ)/gts1.o
DRVLIBS += $(PINOBJ)/alvg.o $(PINOBJ)/alvgdmd.o $(PINOBJ)/alvgs.o
DRVLIBS += $(PINOBJ)/bingo.o
DRVLIBS += $(PINOBJ)/techno.o
DRVLIBS += $(PINOBJ)/spinb.o
DRVLIBS += $(PINOBJ)/mrgame.o
DRVLIBS += $(PINOBJ)/nuova.o
DRVLIBS += $(PINOBJ)/inder.o
DRVLIBS += $(PINOBJ)/jp.o
DRVLIBS += $(PINOBJ)/ltd.o
DRVLIBS += $(PINOBJ)/peyper.o
DRVLIBS += $(PINOBJ)/sleic.o
DRVLIBS += $(PINOBJ)/play.o
DRVLIBS += $(PINOBJ)/bowarrow.o $(PINOBJ)/flicker.o $(PINOBJ)/rotation.o
DRVLIBS += $(PINOBJ)/rowamet.o
DRVLIBS += $(PINOBJ)/wico.o
DRVLIBS += $(PINOBJ)/nsm.o
DRVLIBS += $(PINOBJ)/allied.o
DRVLIBS += $(PINOBJ)/jvh.o
DRVLIBS += $(PINOBJ)/vd.o
DRVLIBS += $(PINOBJ)/kissproto.o
#
# Games
#
PINGAMES  = $(PINOBJ)/by35games.o $(PINOBJ)/stgames.o
PINGAMES += $(PINOBJ)/s3games.o $(PINOBJ)/s4games.o $(PINOBJ)/s6games.o
PINGAMES += $(PINOBJ)/s7games.o $(PINOBJ)/s11games.o
PINGAMES += $(PINOBJ)/bowlgames.o
PINGAMES += $(PINOBJ)/by6803games.o
PINGAMES += $(PINOBJ)/byvidgames.o
PINGAMES += $(PINOBJ)/gts3games.o
PINGAMES += $(PINOBJ)/gts80games.o
PINGAMES += $(PINOBJ)/degames.o
PINGAMES += $(PINOBJ)/segames.o
PINGAMES += $(PINOBJ)/wpcgames.o
PINGAMES += $(PINOBJ)/hnkgames.o
PINGAMES += $(PINOBJ)/zacgames.o
PINGAMES += $(PINOBJ)/gpgames.o
PINGAMES += $(PINOBJ)/atarigames.o
PINGAMES += $(PINOBJ)/taitogames.o
PINGAMES += $(PINOBJ)/capgames.o
PINGAMES += $(PINOBJ)/gts1games.o
PINGAMES += $(PINOBJ)/alvggames.o
PINGAMES += $(PINOBJ)/spinbgames.o
PINGAMES += $(PINOBJ)/mrgamegames.o
PINGAMES += $(PINOBJ)/indergames.o
PINGAMES += $(PINOBJ)/jpgames.o
PINGAMES += $(PINOBJ)/ltdgames.o
PINGAMES += $(PINOBJ)/peypergames.o
PINGAMES += $(PINOBJ)/sleicgames.o
PINGAMES += $(PINOBJ)/playgames.o
#
# Simulators
#
PINGAMES += $(PINOBJ)/sims/s7/full/bk.o
PINGAMES += $(PINOBJ)/sims/s7/full/tmfnt.o
PINGAMES += $(PINOBJ)/sims/s11/prelim/eatpm.o
PINGAMES += $(PINOBJ)/sims/s11/full/milln.o
PINGAMES += $(PINOBJ)/sims/s11/full/dd.o
PINGAMES += $(PINOBJ)/sims/se/prelim/monopoly.o
PINGAMES += $(PINOBJ)/sims/se/prelim/elvis.o
PINGAMES += $(PINOBJ)/sims/wpc/full/afm.o
PINGAMES += $(PINOBJ)/sims/wpc/full/bop.o
PINGAMES += $(PINOBJ)/sims/wpc/full/br.o
PINGAMES += $(PINOBJ)/sims/wpc/full/cftbl.o
PINGAMES += $(PINOBJ)/sims/wpc/full/dd_wpc.o
PINGAMES += $(PINOBJ)/sims/wpc/full/drac.o
PINGAMES += $(PINOBJ)/sims/wpc/full/fh.o
PINGAMES += $(PINOBJ)/sims/wpc/full/ft.o
PINGAMES += $(PINOBJ)/sims/wpc/full/gi.o
PINGAMES += $(PINOBJ)/sims/wpc/full/gw.o
PINGAMES += $(PINOBJ)/sims/wpc/full/hurr.o
PINGAMES += $(PINOBJ)/sims/wpc/full/ij.o
PINGAMES += $(PINOBJ)/sims/wpc/full/jd.o
PINGAMES += $(PINOBJ)/sims/wpc/full/pz.o
PINGAMES += $(PINOBJ)/sims/wpc/full/rs.o
PINGAMES += $(PINOBJ)/sims/wpc/full/sttng.o
PINGAMES += $(PINOBJ)/sims/wpc/full/t2.o
PINGAMES += $(PINOBJ)/sims/wpc/full/taf.o
PINGAMES += $(PINOBJ)/sims/wpc/full/tz.o
PINGAMES += $(PINOBJ)/sims/wpc/full/wcs.o
PINGAMES += $(PINOBJ)/sims/wpc/full/ww.o
PINGAMES += $(PINOBJ)/sims/wpc/full/ss.o
PINGAMES += $(PINOBJ)/sims/wpc/full/tom.o
PINGAMES += $(PINOBJ)/sims/wpc/full/mm.o
PINGAMES += $(PINOBJ)/sims/wpc/full/ngg.o
PINGAMES += $(PINOBJ)/sims/wpc/full/hd.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/cc.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/congo.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/corv.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/cp.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/cv.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/dh.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/dm.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/dw.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/fs.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/i500.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/jb.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/jm.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/jy.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/mb.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/nbaf.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/nf.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/pop.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/sc.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/totan.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/ts.o
PINGAMES += $(PINOBJ)/sims/wpc/prelim/wd.o

CPUS += M6809@
CPUS += M6808@
CPUS += M6800@
CPUS += M6803@
CPUS += M6802@
CPUS += ADSP2105@
ifdef ISVER7300
CPUS += ADSP2101@
endif
CPUS += Z80@
CPUS += M6502@
CPUS += M65C02@
CPUS += M68000@
CPUS += M68306@
CPUS += S2650@
CPUS += 8080@
CPUS += 8085A@
CPUS += I8035@
CPUS += I8039@
CPUS += I86@
CPUS += I186@
CPUS += I188@
CPUS += 4004@
CPUS += PPS4@
CPUS += I8051@
CPUS += I8752@
CPUS += TMS7000@
CPUS += SCAMP@
CPUS += ARM7@
CPUS += AT91@
CPUS += CDP1802@
CPUS += TMS9980@
CPUS += TMS9995@

SOUNDS += DAC@
SOUNDS += YM2151_ALT@
ifdef ISVER6300
SOUNDS += YM2610@ #to avoid compile errors
SOUNDS += YM2610B@
endif
ifdef ISVER6100
SOUNDS += YM2610@ #to avoid compile errors
endif
SOUNDS += HC55516@
SOUNDS += SAMPLES@
SOUNDS += TMS5220@
SOUNDS += AY8910@
SOUNDS += MSM5205@
SOUNDS += CUSTOM@
SOUNDS += BSMT2000@
SOUNDS += VOTRAXSC01@
SOUNDS += OKIM6295@
SOUNDS += ADPCM@
SOUNDS += SN76477@
SOUNDS += SN76496@
SOUNDS += DISCRETE@
SOUNDS += SP0250@
SOUNDS += TMS320AV120@
SOUNDS += M114S@
SOUNDS += YM3812@
SOUNDS += S14001A@

OBJDIRS += $(PINOBJ)
OBJDIRS += $(PINOBJ)/sims
OBJDIRS += $(PINOBJ)/sims/wpc
OBJDIRS += $(PINOBJ)/sims/wpc/prelim
OBJDIRS += $(PINOBJ)/sims/wpc/full
OBJDIRS += $(PINOBJ)/sims/s11
OBJDIRS += $(PINOBJ)/sims/s11/full
OBJDIRS += $(PINOBJ)/sims/s11/prelim
OBJDIRS += $(PINOBJ)/sims/s7
OBJDIRS += $(PINOBJ)/sims/s7/full
OBJDIRS += $(PINOBJ)/sims/se
OBJDIRS += $(PINOBJ)/sims/se/prelim

$(OBJ)/allgames.a: $(PINGAMES)
#
# Preprocessor Definitions
#

DEFS += -DDIRECTSOUND_VERSION=0x0300 \
        -DDIRECTINPUT_VERSION=0x0500 \
        -DDIRECTDRAW_VERSION=0x0300 \
        -DWINVER=0x0400 \
        -D_WIN32_IE=0x0500 \
        -D_WIN32_WINNT=0x0400 \
        -DWIN32 \
        -UWINNT

# generated text files
TEXTS += gamelist.txt

gamelist.txt: $(EMULATOR)
	@echo Generating $@...
	@$(CURPATH)$(EMULATOR) -gamelist -noclones -sortname > gamelist.txt

cleanpinmame:
	@echo Deleting $(target) object tree $(PINOBJ)...
	$(RM) -r $(PINOBJ)
	@echo Deleting $(EMULATOR)...
	$(RM) $(EMULATOR)

ifdef DEBUG
LDFLAGS+= -Xlinker -Map -Xlinker $(TARGET).map -Xlinker --cref
endif

