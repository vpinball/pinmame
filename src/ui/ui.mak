#####################################################################
# make SUFFIX=32

# don't create gamelist.txt
TEXTS = gamelist.txt

# remove pedantic
$(OBJ)/ui/%.o: src/ui/%.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -c $< -o $@

OBJDIRS += $(OBJ)/ui

# only OS specific output files and rules
OSOBJS += \
        $(OBJ)/ui/m32main.o \
        $(OBJ)/ui/M32Util.o \
        $(OBJ)/ui/DirectInput.o \
        $(OBJ)/ui/DIJoystick.o \
        $(OBJ)/ui/DirectDraw.o \
        $(OBJ)/ui/Directories.o \
        $(OBJ)/ui/audit32.o \
        $(OBJ)/ui/ColumnEdit.o \
        $(OBJ)/ui/Screenshot.o \
        $(OBJ)/ui/TreeView.o \
        $(OBJ)/ui/Splitters.o \
        $(OBJ)/ui/Bitmask.o \
        $(OBJ)/ui/DataMap.o \
        $(OBJ)/ui/dxdecode.o \
        $(OBJ)/ui/Win32ui.o \
        $(OBJ)/ui/Properties.o \
        $(OBJ)/ui/options.o \
        $(OBJ)/ui/help.o \
        $(OBJ)/ui/layout.o \
        $(OBJ)/ui/history.o \
        $(OBJ)/ui/dialogs.o

# add resource file
OSOBJS += $(OBJ)/ui/PinMAME32.res

#####################################################################
# compiler

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
        -UWINNT \
	-DCLIB_DECL=__cdecl \
        -DDECL_SPEC= \
        -DZEXTERN=extern \
        -Isrc/htmlhelp/include

#####################################################################
# Resources

RC = windres --use-temp-file

RCDEFS = -DNDEBUG -D_WIN32_IE=0x0400

RCFLAGS = -O coff --include-dir src/ui

$(OBJ)/ui/%.res: src/ui/%.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) -o $@ -i $<

#####################################################################
# Linker

LIBS += -lkernel32 \
        -lshell32 \
        -lcomctl32 \
        -lcomdlg32 \
        -ladvapi32 \
        -lhtmlhelp

# use -mconsole for romcmp
LDFLAGS += -mwindows -Lsrc/htmlhelp/lib

#####################################################################

