# only Windows specific output files and rules
# the first two targets generate the prefix.h header
# note this requires that OSOBJS be the first target
#
OSOBJS = $(OBJ)/windows/winmain.o $(OBJ)/windows/fileio.o $(OBJ)/windows/config.o \
	 $(OBJ)/windows/ticker.o $(OBJ)/windows/fronthlp.o $(OBJ)/windows/video.o \
	 $(OBJ)/windows/input.o $(OBJ)/windows/sound.o $(OBJ)/windows/blit.o \
	 $(OBJ)/windows/rc.o $(OBJ)/windows/misc.o \
	 $(OBJ)/windows/window.o $(OBJ)/windows/wind3d.o $(OBJ)/windows/wind3dfx.o \
	 $(OBJ)/windows/winddraw.o \
	 $(OBJ)/windows/asmblit.o $(OBJ)/windows/asmtile.o
# not necessary anymore, even with older MinGW2/GCC3.2.3 setup
# OSOBJS += $(OBJ)/windows/snprintf.o

# uncomment this line to enable guard pages on all memory allocations
#OSOBJS += $(OBJ)/windows/winalloc.o

# video blitting functions
$(OBJ)/windows/asmblit.o: src/windows/asmblit.asm
	@echo Assembling $<...
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

# tilemap blitting functions
$(OBJ)/windows/asmtile.o: src/windows/asmtile.asm
	@echo Assembling $<...
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

# add our prefix files to the mix
CFLAGS += -mwindows -include src/$(MAMEOS)/winprefix.h
CPPFLAGS += -mwindows -include src/$(MAMEOS)/winprefix.h

# add the windows libraries
LIBS += -luser32 -lgdi32 -lddraw -ldsound -ldinput -ldxguid -lwinmm

# due to quirks of using /bin/sh, we need to explicitly specify the current path
CURPATH = ./

# if building with a UI, set the C flags and include the ui.mak
ifneq ($(WINUI),)
CFLAGS+= -DWINUI=1
CPPFLAGS+= -DWINUI=1
include src/ui/ui.mak
endif
