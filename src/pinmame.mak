#
# MAME source version
# PinMAME requires patches in the following files:
# src/cpu/adsp2100/adsp2100.c
# src/sound/hc55516.h
# src/sound/hc55516.C

# Visual PinMAME requires patches in the following files:
# src/unzip.c

MAMEVER=3716

#MAMEVER > 3716
#DEFS += -DBMTYPE=UINT16
#DEFS += -DM65C02_INT_IRQ=M65C02_IRQ_LINE
#DEFS += -DM65C02_INT_NMI=INTERRUPT_NMI
#else
DEFS += -DBMTYPE=UINT8
DEFS += -Dmame_bitmap=osd_bitmap

PINMAMESRC=wpc
#
# enable PinMAME extension (sound recording, command line switches)
# requires patches in the following MAME files:
# src/osdepend.h
# src/mame.h
# src/common.c
# src/msdos/fileio.c
# src/msdos/config.c
# src/windows/fileio.c
# src/sound/mixer.c
#

PINMAME_EXT=1

#
# enable DCS speedups
# requires patches in
# src/cpu/adsp2100/adsp2100.c
#

WPCDCSSPEEDUP=1

#
# enabled MESS exit handling for PinMAME
# requires patch in
# src/driver.h
# src/cpuintrf.c

PINMAME_EXIT=1

#
# PinMAME specific flags
#
PINOBJ=$(OBJ)/$(PINMAMESRC)
CFLAGS += -Isrc/$(PINMAMESRC)
COREDEFS += -DTINY_COMPILE=1
DEFS += -DMAMEVER=$(MAMEVER) -DPINMAME=1
ifdef PINMAME_EXT
DEFS += -DPINMAME_EXT=1
endif
ifdef WPCDCSSPEEDUP
DEFS += -DWPCDCSSPEEDUP=1
endif
ifdef PINMAME_EXIT
DEFS += -DPINMAME_EXIT
endif

#
# Common stuff
#
DRVLIBS = $(PINOBJ)/sim.o $(PINOBJ)/core.o $(OBJ)/allgames.a
DRVLIBS += $(PINOBJ)/vpintf.o $(PINOBJ)/snd_cmd.o $(PINOBJ)/wpcsam.o

COREOBJS += $(PINOBJ)/driver.o $(OBJ)/cheat.o $(PINOBJ)/mech.o

# why isn't this part of the core
DRVLIBS += $(OBJ)/vidhrdw/crtc6845.o $(OBJ)/machine/6532riot.o

#
# Core drivers
#
DRVLIBS += $(PINOBJ)/s11.o $(PINOBJ)/s11csoun.o
DRVLIBS += $(PINOBJ)/wpc.o $(PINOBJ)/wpcsound.o $(PINOBJ)/dcs.o
DRVLIBS += $(PINOBJ)/s7.o
DRVLIBS += $(PINOBJ)/s6.o $(PINOBJ)/s67s.o
DRVLIBS += $(PINOBJ)/s4.o
DRVLIBS += $(PINOBJ)/de.o $(PINOBJ)/de1sound.o $(PINOBJ)/de2sound.o
DRVLIBS += $(PINOBJ)/de2.o $(PINOBJ)/de3.o $(PINOBJ)/dedmd.o
DRVLIBS += $(PINOBJ)/gts3.o
DRVLIBS += $(PINOBJ)/se.o $(PINOBJ)/sesound.o
DRVLIBS += $(PINOBJ)/s80.o $(PINOBJ)/s80sound0.o $(PINOBJ)/s80sound1.o
DRVLIBS += $(PINOBJ)/by35.o $(PINOBJ)/by35snd.o
#
# Games
#
PINGAMES  = $(PINOBJ)/by35games.o
PINGAMES += $(PINOBJ)/s3games.o $(PINOBJ)/s4games.o $(PINOBJ)/s6games.o
PINGAMES += $(PINOBJ)/s7games.o $(PINOBJ)/s11games.o
PINGAMES += $(PINOBJ)/degames.o $(PINOBJ)/gts3games.o $(PINOBJ)/s80games.o
PINGAMES += $(PINOBJ)/segames.o $(PINOBJ)/wpcgames.o
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
PINGAMES += $(PINOBJ)/sims/wpc/full/dd.o
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

SOUNDS += DAC@
SOUNDS += YM2151@
SOUNDS += HC55516@
SOUNDS += SAMPLES@
SOUNDS += TMS5220@
SOUNDS += AY8910@
SOUNDS += MSM5205@
SOUNDS += CUSTOM@
SOUNDS += BSMT2000@

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

ifdef DEBUG
DEFS += -DDBG_BPR=1
endif

cleanpinmame:
	@echo Deleting $(target) object tree $(PINOBJ)...
	$(RM) -r $(PINOBJ)
	@echo Deleting $(EMULATOR)...
	$(RM) $(EMULATOR)

