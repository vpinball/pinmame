# the core object files (without target specific objects;
# those are added in the target.mak files)
COREOBJS = $(OBJ)/version.o $(OBJ)/mame.o \
	$(OBJ)/drawgfx.o $(OBJ)/common.o $(OBJ)/usrintrf.o $(OBJ)/ui_text.o \
	$(OBJ)/cpuintrf.o $(OBJ)/memory.o $(OBJ)/timer.o $(OBJ)/palette.o \
	$(OBJ)/input.o $(OBJ)/inptport.o $(OBJ)/unzip.o \
	$(OBJ)/audit.o $(OBJ)/info.o $(OBJ)/png.o $(OBJ)/artwork.o \
	$(OBJ)/tilemap.o \
	$(OBJ)/state.o $(OBJ)/datafile.o $(OBJ)/hiscore.o \
	$(sort $(CPUOBJS)) \
	$(OBJ)/sndintrf.o \
	$(OBJ)/sound/streams.o $(OBJ)/sound/mixer.o $(OBJ)/sound/filter.o \
	$(sort $(SOUNDOBJS)) \
	$(OBJ)/sound/votrax.o \
	$(OBJ)/machine/z80fmly.o $(OBJ)/machine/6821pia.o \
	$(OBJ)/machine/8255ppi.o $(OBJ)/machine/7474.o \
	$(OBJ)/vidhrdw/generic.o $(OBJ)/vidhrdw/vector.o \
	$(OBJ)/vidhrdw/avgdvg.o $(OBJ)/machine/mathbox.o \
	$(OBJ)/machine/ticket.o $(OBJ)/machine/eeprom.o \
	$(OBJ)/machine/6522via.o \
	$(OBJ)/machine/mb87078.o \
	$(OBJ)/mamedbg.o $(OBJ)/window.o \
	$(OBJ)/profiler.o \
	$(sort $(DBGOBJS))

TOOLS = romcmp$(EXE)
TEXTS = gamelist.txt

