# check that the required libraries are available
ifeq ($(wildcard $(DJDIR)/lib/liballeg.a),)
noallegro:
	@echo Missing Allegro library! Get it from http://www.talula.demon.co.uk/allegro/
endif
ifeq ($(wildcard $(DJDIR)/lib/libaudio.a),)
noseal:
	@echo Missing SEAL library! Get it from http://www.egerter.com/
endif
ifeq ($(wildcard $(DJDIR)/lib/libz.a),)
nozlib:
	@echo Missing zlib library! Get it from http://www.cdrom.com/pub/infozip/zlib/
endif

# add allegro and audio libs
LIBS += -lalleg -laudio

# only MS-DOS specific output files and rules
OSOBJS = $(OBJ)/msdos/msdos.o $(OBJ)/msdos/video.o $(OBJ)/msdos/blit.o $(OBJ)/msdos/asmblit.o \
	$(OBJ)/msdos/gen15khz.o $(OBJ)/msdos/ati15khz.o \
	$(OBJ)/msdos/sound.o $(OBJ)/msdos/input.o \
	$(OBJ)/msdos/ticker.o $(OBJ)/msdos/config.o $(OBJ)/msdos/fronthlp.o

# video blitting functions
$(OBJ)/msdos/asmblit.o: src/msdos/asmblit.asm
	@echo Assembling $<...
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

# check if this is a MESS build
ifdef MESS
# additional OS specific object files
OSOBJS += $(OBJ)/mess/msdos.o $(OBJ)/mess/msdos/fileio.o \
	$(OBJ)/mess/msdos/dirio.o $(OBJ)/mess/msdos/nec765.o \
	$(OBJ)/mess/snprintf.o
else
OSOBJS += $(OBJ)/msdos/fileio.o
endif
