#####################################################################

ZLIB_LIBPATH = zlib
DEFS += -DMAME32NAME='"PINMAME32"' -DMAMENAME='"PINMAME"'

# set this to mame, mess or the destination you want to build
TARGET = pinmame
# TARGET = mame
# TARGET = mess
# TARGET = neomame
# TARGET = cpsmame
# TARGET = tiny

MAME_VERSION = -DMAME_VERSION=37
BETA_VERSION = -DBETA_VERSION=13

# uncomment next line to make a debug version
# DEBUG = 1

# build a version for debugging games
# MAME_DEBUG = -DMAME_DEBUG

# uncomment next line to use Assembler 68000 engine
# X86_ASM_68000 = 1

# uncomment next line to use Assembler 68020 engine
# X86_ASM_68020 = 1

# if MAME_NET is defined, network support will be compiled in
# MAME_NET = -DMAME_NET

# if MAME_MMX is defined, MMX will be compiled in
# MAME_MMX = -DMAME_MMX

# use fast (register) calling convention
# USE_FASTCALL = -DFASTCALL

# Define this to enable MIDAS sound
# MIDAS = 1

# uncomment next line to generate help files
# HELP = 1

# set this the operating system you're building for
OS = win32

# extension for executables
EXE = .exe

# CPU core include paths
VPATH=src $(wildcard src/cpu/*)

# compiler, linker and utilities
AR = @ar
CC = @gcc
LD = @gcc
RC = @windres
ASM = @nasmw
ASMFLAGS = -f win32
RM = @rm -f
RMDIR = $(RM) -r
MD = -mkdir

WINDOWS_PROGRAM = -mwindows
CONSOLE_PROGRAM = -mconsole

ifdef DEBUG
NAME = $(TARGET)32d
else
NAME = $(TARGET)32
endif

ifdef DEBUG
NAME = $(TARGET)32d
else
ifdef K6
NAME = $(TARGET)32k6
ARCH = -march=k6
else
ifdef I686
NAME = $(TARGET)32ppro
ARCH = -march=pentiumpro
else
NAME = $(TARGET)32
ARCH = -march=pentium
endif
endif
endif

# build the targets in different object dirs, since mess changes
# some structures and thus they can't be linked against each other.
# cleantiny isn't needed anymore, because the tiny build has its
# own object directory too.
OBJ = obj/$(NAME)

EMULATOR = $(NAME)$(EXE)

#####################################################################
# compiler

#
# Preprocessor Definitions
#

DEFS = \
        -D_WIN32_IE=0x0400 \
        -DWINVER=0x0400 \
        -D_WIN32_WINNT=0x0400 \
        -DDIRECTSOUND_VERSION=0x0300 \
        -DDIRECTDRAW_VERSION=0x0300 \
        -DDIRECTINPUT_VERSION=0x0500 \
        -D__int64='long long' \
        -DWIN32 \
        -D_WINDOWS \
        -UWINNT \
        -DHMONITOR_DECLARED \
        -D_LPCWAVEFORMATEX_DEFINED \
        -DM_PI=3.1415926534 \
        -DPI=3.1415926534 \
        -DINLINE='static __inline__' \
        -DCLIB_DECL=__cdecl \
        -DDECL_SPEC=__cdecl \
        -DPNG_SAVE_SUPPORT \
        -DHAS_CPUS \
        -DHAS_SOUND \
        -DLSB_FIRST=1 \
        -DZLIB_DLL \
        -DX86_ASM \
        $(MAME_VERSION) \
        $(BETA_VERSION) \
        $(RELEASE_CANDIDATE) \
        $(MAME_NET) \
        $(MAME_MMX) \
        $(MAME_DEBUG) \
        $(USE_FASTCALL) \

ifndef DEBUG
DEFS += -DNDEBUG
endif

ifeq "$(TARGET)" "mess"
DEFS += -DMESS=1 -DMAME32NAME='"MESS32"' -DMAMENAME='"MESS"'
endif

ifdef MIDAS
DEFS += -DMIDAS
endif

#
# Include path
#

INCLUDES = \
        -I. \
        -Isrc \
        -Isrc/Win32 \
        -Isrc/cpu/m68000 \
        -I$(OBJ)/cpu/m68000 \

ifeq "$(TARGET)" "mess"
INCLUDES += \
        -Imess \
        -Imess/includes \
        -Imess/Win32
endif

ifdef DX_INCPATH
INCLUDES += -idirafter $(DX_INCPATH)
endif

ifdef ZLIB_INCPATH
INCLUDES += -I $(ZLIB_INCPATH)
endif

ifdef MIDAS
ifdef MIDAS_INCPATH
INCLUDES += -I $(MIDAS_INCPATH)
endif
endif

#
# C Compiler flags
#

CFLAGS = $(INCLUDES)

# multi threaded
CFLAGS +=

ifdef DEBUG
CFLAGS += -d
else
CFLAGS += -O3
endif

CLAGS += $(ARCH) \
        -fomit-frame-pointer \
        -fstrict-aliasing \
        -Werror \
        -Wall \
        -Wno-sign-compare \
        -Wunused \
        -Wpointer-arith \
        -Wbad-function-cast \
        -Wcast-align \
        -Waggregate-return \
        -pedantic \
        -Wshadow \
        -Wstrict-prototypes \
        -mno-cygwin

#####################################################################
# Resources

RCDEFS = $(MAME_NET) $(MAME_MMX) -DNDEBUG $(MAME_VERSION) -D_WIN32_IE=0x0400

RCFLAGS = -O coff --include-dir src/Win32 --include-dir $(INCLUDE)

ifeq "$(TARGET)" "mess"
RCFLAGS += -DMESS --include-dir mess/Win32
endif

#####################################################################
# Linker

LIBS = \
        -lkernel32 \
        -luser32 \
        -lgdi32 \
        -lshell32 \
        -lcomctl32 \
        -lcomdlg32 \
        -ladvapi32 \
        -lwinmm \
        -ldxguid \
        -ldinput \
        -lzlib

ifdef MAME_NET
LIBS += -lwsock32
endif

ifdef MAME_AVI
LIBS += -lvfw32
endif

ifdef MIDAS
LIBS    += -lmidas
ifdef MIDAS_LIBPATH
LDFLAGS += -L $(MIDAS_LIBPATH)
endif
endif

ifdef DX_LIBPATH
LDFLAGS += -L $(DX_LIBPATH)
endif

ifdef ZLIB_LIBPATH
LDFLAGS += -L $(ZLIB_LIBPATH)
endif

ifdef DEBUG
LDFLAGS +=
else
LDFLAGS +=
endif

#####################################################################

OBJDIRS = \
        $(OBJ) \
        $(OBJ)/cpu \
        $(OBJ)/sound \
        $(OBJ)/drivers \
        $(OBJ)/machine \
        $(OBJ)/vidhrdw \
        $(OBJ)/sndhrdw \
        $(OBJ)/Win32 \
        $(OBJ)/Win32/hlp \

ifeq "$(TARGET)" "mess"
# MESS object directories
OBJDIRS += \
        $(OBJ)/mess \
        $(OBJ)/mess/systems \
        $(OBJ)/mess/machine \
        $(OBJ)/mess/vidhrdw \
        $(OBJ)/mess/sndhrdw \
        $(OBJ)/mess/tools \
        $(OBJ)/mess/formats \
        $(OBJ)/mess/messroms \
        $(OBJ)/mess/Win32
endif

#####################################################################

all: maketree $(EMULATOR) extra

# include the various .mak files
include src/core.mak
include src/$(TARGET).mak
include src/rules.mak
include src/$(OS)/$(OS).mak

ifdef MAME_DEBUG
DBGDEFS = -DMAME_DEBUG
else
DBGDEFS =
DBGOBJS =
endif

extra:	romcmp$(EXE) $(TOOLS) $(HELPFILES)

# combine the various definitions to one
CDEFS = $(DEFS) $(COREDEFS) $(CPUDEFS) $(SOUNDDEFS) $(ASMDEFS) $(DBGDEFS)

$(EMULATOR): $(OBJS) $(COREOBJS) $(OSOBJS) $(DRVLIBS) $(RES)
# always recompile the version string
	$(CC) -o $(OBJ)/version.o -c src/version.c
	@echo Linking $@...
	$(LD) $(LDFLAGS) $(WINDOWS_PROGRAM) $(OBJS) $(COREOBJS) $(OSOBJS) $(LIBS) $(DRVLIBS) $(RES) -o $@
ifndef DEBUG
#	upx $(EMULATOR)
endif

romcmp$(EXE): $(OBJ)/romcmp.o $(OBJ)/unzip.o
	@echo Linking $@...
	$(LD) $(LDFLAGS) $(CONSOLE_PROGRAM) $^ $(LIBS) -o $@

ifdef PERL
$(OBJ)/cpuintrf.o: src/cpuintrf.c rules.mak
	$(PERL) src/makelist.pl
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -c $< -o $@
endif

$(OBJ)/%.o: src/%.asm
	@echo Assembling $<...
	$(ASM) -o $@ $(ASMFLAGS) $(ASMDEFS) $<

$(OBJ)/%.o: src/%.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -o $@ -c $<

$(OBJ)/mess/%.o: mess/%.c
	@echo [MESS] Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -o $@ -c $<

$(OBJ)/Win32/%.res: src/Win32/%.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) -o $@ -i $<

# compile generated C files for the 68000 emulator
$(M68000_GENERATED_OBJS): $(OBJ)/cpu/m68000/m68kmake$(EXE)
	@echo Compiling $(subst .o,.c,$@)...
	$(CC) $(CDEFS) $(CFLAGS) -o $@ -c $*.c

# additional rule, because m68kcpu.c includes the generated m68kops.h :-/
$(OBJ)/cpu/m68000/m68kcpu.o: $(OBJ)/cpu/m68000/m68kmake$(EXE)

# generate C source files for the 68000 emulator
$(OBJ)/cpu/m68000/m68kmake$(EXE): src/cpu/m68000/m68kmake.c
	@echo M68K make $<...
	$(CC) $(CDEFS) $(CFLAGS) -o $(OBJ)/cpu/m68000/m68kmake$(EXE) $<
	@echo Generating M68K source files...
	$(OBJ)/cpu/m68000/m68kmake$(EXE) $(OBJ)/cpu/m68000 src/cpu/m68000/m68k_in.c

# generate asm source files for the 68000 emulator
$(OBJ)/cpu/m68000/68000.asm:  src/cpu/m68000/make68k.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -o $(OBJ)/cpu/m68000/make68k$(EXE) $<
	@echo Generating $@...
	$(OBJ)/cpu/m68000/make68k$(EXE) $@ $(OBJ)/cpu/m68000/68000tab.asm 00

$(OBJ)/cpu/m68000/68020.asm:  src/cpu/m68000/make68k.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -o $(OBJ)/cpu/m68000/make68k$(EXE) $<
	@echo Generating $@...
	$(OBJ)/cpu/m68000/make68k$(EXE) $@ $(OBJ)/cpu/m68000/68020tab.asm 20

# generated asm files for the 68000 emulator
$(OBJ)/cpu/m68000/68000.o:  $(OBJ)/cpu/m68000/68000.asm
	@echo Assembling $<...
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

$(OBJ)/cpu/m68000/68020.o:  $(OBJ)/cpu/m68000/68020.asm
	@echo Assembling $<...
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

$(OBJ)/%.a:
	@echo Archiving $@...
	$(RM) $@
	$(AR) cr $@ $^

makedir:
	@echo make makedir is no longer necessary, just type make

$(sort $(OBJDIRS)):
	$(MD) $@

maketree: $(sort $(OBJDIRS))

clean:
	@echo Deleting object tree $(OBJ)...
	$(RMDIR) $(OBJ)
	@echo Deleting $(EMULATOR)...
	$(RM) $(EMULATOR)

