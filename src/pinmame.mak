MAMEVER=5900
PINMAMESRC=wpc

#
# PinMAME specific flags
#
PINOBJ=$(OBJ)/$(PINMAMESRC)
CFLAGS += -Isrc/$(PINMAMESRC)
DEFS += -DPINMAME=1 -DMAMEVER=$(MAMEVER)
# Used in GUI version (PinMAME32)
DEFS += -DMAME32NAME=\"PINMAME32\" -DMAMENAME=\"PINMAME\"

#
# Common stuff
#
DRVLIBS = $(PINOBJ)/sim.o $(PINOBJ)/core.o $(OBJ)/allgames.a
DRVLIBS += $(PINOBJ)/vpintf.o $(PINOBJ)/snd_cmd.o $(PINOBJ)/wpcsam.o
DRVLIBS += $(PINOBJ)/sndbrd.o

COREOBJS += $(PINOBJ)/driver.o $(OBJ)/cheat.o $(PINOBJ)/mech.o

# taken from MESS
DRVLIBS += $(OBJ)/vidhrdw/crtc6845.o $(OBJ)/machine/6532riot.o $(OBJ)/machine/6530riot.o
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
DRVLIBS += $(PINOBJ)/by35.o $(PINOBJ)/by35snd.o
DRVLIBS += $(PINOBJ)/by6803.o
DRVLIBS += $(PINOBJ)/byvidpin.o
DRVLIBS += $(PINOBJ)/capcom.o
DRVLIBS += $(PINOBJ)/hnk.o $(PINOBJ)/hnks.o
DRVLIBS += $(PINOBJ)/zac.o $(PINOBJ)/zacsnd.o
DRVLIBS += $(PINOBJ)/gp.o
DRVLIBS += $(PINOBJ)/atari.o $(PINOBJ)/atarisnd.o
DRVLIBS += $(PINOBJ)/taito.o $(PINOBJ)/taitos.o
DRVLIBS += $(PINOBJ)/midway.o
DRVLIBS += $(PINOBJ)/gts1.o
#
# Games
#
PINGAMES  = $(PINOBJ)/by35games.o $(PINOBJ)/stgames.o
PINGAMES += $(PINOBJ)/s3games.o $(PINOBJ)/s4games.o $(PINOBJ)/s6games.o
PINGAMES += $(PINOBJ)/s7games.o $(PINOBJ)/s11games.o
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
PINGAMES += $(PINOBJ)/midgames.o
PINGAMES += $(PINOBJ)/gts1games.o
#
# Simulators
#
PINGAMES += $(PINOBJ)/sims/s7/full/tmfnt.o
PINGAMES += $(PINOBJ)/sims/s11/prelim/eatpm.o
PINGAMES += $(PINOBJ)/sims/s11/full/milln.o
PINGAMES += $(PINOBJ)/sims/s11/full/dd.o
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
CPUS += Z80@
CPUS += M6502@
CPUS += M65C02@
CPUS += M68000@
CPUS += M68306@
CPUS += S2650@
CPUS += 8080@
CPUS += I86@
CPUS += 4004@
CPUS += PPS4@

SOUNDS += DAC@
SOUNDS += YM2151_ALT@
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

$(OBJ)/allgames.a: $(PINGAMES)

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

