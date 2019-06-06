# only LISY specific output files and rules
#

OBJDIRS += $(OBJ)/lisy

# add precompiler defines for LISY80
DEFS += -DLISY_SUPPORT

# add objects for LISY
LISYOBJS = \
 $(OBJ)/lisy/lisy.o \
 $(OBJ)/lisy/lisy1.o \
 $(OBJ)/lisy/lisy35.o \
 $(OBJ)/lisy/lisy80.o \
 $(OBJ)/lisy/lisy_w.o \
 $(OBJ)/lisy/hw_lib.o \
 $(OBJ)/lisy/utils.o \
 $(OBJ)/lisy/switches.o \
 $(OBJ)/lisy/coils.o \
 $(OBJ)/lisy/displays.o \
 $(OBJ)/lisy/fileio.o \
 $(OBJ)/lisy/sound.o \
 $(OBJ)/lisy/eeprom.o \
 $(OBJ)/lisy/opc.o \
 $(OBJ)/lisy/fadecandy.o \
 $(OBJ)/lisy/usbserial.o \
 $(OBJ)/lisy/lisy_mame.o \
 $(OBJ)/lisy/lisy_home.o


# LISY functions
$(OBJ)/lisy/%.o: src/lisy/%.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CCFLAGS) -c $< -o $@
