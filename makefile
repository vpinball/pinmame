# set this to mame, mess or the destination you want to build
# TARGET = mame
# TARGET = mess
# TARGET = neomame
# TARGET = cpmame
# TARGET = pinmame
# TARGET = mmsnd
# example for a tiny compile
# TARGET = tiny
ifeq ($(TARGET),)
TARGET = pinmame
endif

# uncomment next line to include running older stern whitestar games in new at91 cpu board for testing
# TEST_NEW_SOUND = 1

# uncomment next line to include the debugger
# DEBUG = 1

# uncomment next line to include the symbols for symify
# SYMBOLS = 1

# uncomment next line to generate a link map for exception handling in windows
# MAP = 1

# uncomment next line to use Assembler 68000 engine
# X86_ASM_68000 = 1

# uncomment next line to use Assembler 68020 engine
# X86_ASM_68020 = 1

# uncomment next line to use DRC MIPS3 engine
X86_MIPS3_DRC = 1

# uncomment next line to use cygwin compiler
# COMPILESYSTEM_CYGWIN	= 1


# set this the operating system you're building for
# MAMEOS = msdos
# MAMEOS = windows
ifeq ($(MAMEOS),)
MAMEOS = windows
endif

# extension for executables
EXE = .exe

# CPU core include paths
VPATH=src $(wildcard src/cpu/*)

# compiler, linker and utilities
AR = @ar
CC = @gcc
LD = @gcc
ASM = @nasm
ASMFLAGS = -f coff
MD = -mkdir
RM = @rm -f
#PERL = @perl -w


ifeq ($(MAMEOS),msdos)
PREFIX = d
else
PREFIX =
endif

ifdef DEBUG
NAME = $(PREFIX)$(TARGET)$(SUFFIX)d
else
ifdef ATHLON
NAME = $(PREFIX)$(TARGET)$(SUFFIX)at
ARCH = -march=athlon
else
ifdef K6
NAME = $(PREFIX)$(TARGET)$(SUFFIX)k6
ARCH = -march=k6
else
ifdef I686
NAME = $(PREFIX)$(TARGET)$(SUFFIX)pp
ARCH = -march=pentiumpro
else
ifdef P4
NAME = $(PREFIX)$(TARGET)$(SUFFIX)p4
ARCH = -march=pentium4
else
NAME = $(PREFIX)$(TARGET)$(SUFFIX)
ARCH = -march=pentium
endif
endif
endif
endif
endif

# build the targets in different object dirs, since mess changes
# some structures and thus they can't be linked against each other.
OBJ = obj/gcc/$(NAME)

EMULATOR = $(NAME)$(EXE)

DEFS = -DX86_ASM -DLSB_FIRST -DINLINE="static __inline__" -Dasm=__asm__

CFLAGS = -std=gnu99 -Isrc -Isrc/includes -Isrc/$(MAMEOS) -I$(OBJ)/cpu/m68000 -Isrc/cpu/m68000

ifdef SYMBOLS
CFLAGS += -O0 -Wall -Werror -Wno-unused -g
else
CFLAGS += -DNDEBUG \
	$(ARCH) -O3 -fomit-frame-pointer -fstrict-aliasing \
	-Werror -Wall -Wno-sign-compare -Wunused \
	-Wpointer-arith -Wbad-function-cast -Wcast-align -Waggregate-return \
	-Wshadow -Wstrict-prototypes -Wundef \
	-Wformat-security -Wwrite-strings \
	-Wdisabled-optimization \
#	-Wredundant-decls
#	-Wfloat-equal
#	-Wunreachable-code -Wpadded
#	-W had to remove because of the "missing initializer" warning
#	-Wlarger-than-262144  \
#	-Wcast-qual \
#	-Wwrite-strings \
#	-Wconversion \
#	-Wmissing-prototypes \
#	-Wmissing-declarations
endif

CFLAGSPEDANTIC = $(CFLAGS) -pedantic

ifdef SYMBOLS
LDFLAGS =
else
#LDFLAGS = -s -Wl,--warn-common
LDFLAGS = -s
endif

ifdef MAP
MAPFLAGS = -Wl,-M >$(NAME).map
else
MAPFLAGS =
endif

# platform .mak files will want to add to this
LIBS = -lz

OBJDIRS = obj obj/gcc $(OBJ) $(OBJ)/cpu $(OBJ)/sound $(OBJ)/$(MAMEOS) \
	$(OBJ)/machine $(OBJ)/vidhrdw
ifneq ($(TARGET),pinmame)
OBJDIRS += $(OBJ)/drivers $(OBJ)/sndhrdw
endif
ifdef MESS
OBJDIRS += $(OBJ)/mess $(OBJ)/mess/systems $(OBJ)/mess/machine \
	$(OBJ)/mess/vidhrdw $(OBJ)/mess/sndhrdw $(OBJ)/mess/tools
endif

ifeq ($(TARGET),mmsnd)
OBJDIRS	+= $(OBJ)/mmsnd $(OBJ)/mmsnd/machine $(OBJ)/mmsnd/drivers $(OBJ)/mmsnd/sndhrdw
endif

all:	maketree $(EMULATOR) extra

# include the various .mak files
include src/core.mak
include src/$(TARGET).mak
include src/rules.mak
include src/$(MAMEOS)/$(MAMEOS).mak

ifdef DEBUG
DBGDEFS = -DMAME_DEBUG
else
DBGDEFS =
DBGOBJS =
endif

ifdef TEST_NEW_SOUND
DBGDEFS += -DTEST_NEW_SOUND
endif

ifdef COMPILESYSTEM_CYGWIN
CFLAGS	+= -mno-cygwin
LDFLAGS	+= -mno-cygwin
endif

extra:	$(TOOLS) $(TEXTS)

# combine the various definitions to one
CDEFS = $(DEFS) $(COREDEFS) $(CPUDEFS) $(SOUNDDEFS) $(ASMDEFS) $(DBGDEFS)

# primary target
$(EMULATOR): $(OBJS) $(COREOBJS) $(OSOBJS) $(DRVLIBS)
# always recompile the version string
	$(CC) $(CDEFS) $(CFLAGS) -c src/version.c -o $(OBJ)/version.o
	@echo Linking $@...
	$(LD) $(LDFLAGS) $(OBJS) $(COREOBJS) $(OSOBJS) $(LIBS) $(DRVLIBS) -o $@ $(MAPFLAGS)

romcmp$(EXE): $(OBJ)/romcmp.o $(OBJ)/unzip.o
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ -lz -o $@

hdcomp$(EXE): $(OBJ)/hdcomp.o $(OBJ)/harddisk.o $(OBJ)/md5.o
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ -lz -o $@

xml2info$(EXE): src/xml2info/xml2info.c
	@echo Compiling $@...
	$(CC) -O1 -o xml2info$(EXE) $<

ifdef PERL
$(OBJ)/cpuintrf.o: src/cpuintrf.c rules.mak
	$(PERL) src/makelist.pl
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGSPEDANTIC) -c $< -o $@
endif

# for Windows at least, we can't compile OS-specific code with -pedantic
$(OBJ)/$(MAMEOS)/%.o: src/$(MAMEOS)/%.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -c $< -o $@

$(OBJ)/%.o: src/%.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGSPEDANTIC) -c $< -o $@

# compile generated C files for the 68000 emulator
$(M68000_GENERATED_OBJS): $(OBJ)/cpu/m68000/m68kmake$(EXE)
	@echo Compiling $(subst .o,.c,$@)...
	$(CC) $(CDEFS) $(CFLAGSPEDANTIC) -c $*.c -o $@

# additional rule, because m68kcpu.c includes the generated m68kops.h :-/
$(OBJ)/cpu/m68000/m68kcpu.o: $(OBJ)/cpu/m68000/m68kmake$(EXE)

# generate C source files for the 68000 emulator
$(OBJ)/cpu/m68000/m68kmake$(EXE): src/cpu/m68000/m68kmake.c
	@echo M68K make $<...
	$(CC) $(CDEFS) $(CFLAGSPEDANTIC) -DDOS -o $(OBJ)/cpu/m68000/m68kmake$(EXE) $<
	@echo Generating M68K source files...
	$(OBJ)/cpu/m68000/m68kmake$(EXE) $(OBJ)/cpu/m68000 src/cpu/m68000/m68k_in.c

# generate asm source files for the 68000/68020 emulators
$(OBJ)/cpu/m68000/68000.asm:  src/cpu/m68000/make68k.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGSPEDANTIC) -O0 -DDOS -o $(OBJ)/cpu/m68000/make68k$(EXE) $<
	@echo Generating $@...
	@$(OBJ)/cpu/m68000/make68k$(EXE) $@ $(OBJ)/cpu/m68000/68000tab.asm 00

$(OBJ)/cpu/m68000/68020.asm:  src/cpu/m68000/make68k.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGSPEDANTIC) -O0 -DDOS -o $(OBJ)/cpu/m68000/make68k$(EXE) $<
	@echo Generating $@...
	@$(OBJ)/cpu/m68000/make68k$(EXE) $@ $(OBJ)/cpu/m68000/68020tab.asm 20

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
	$(RM) -r $(OBJ)
	@echo Deleting $(EMULATOR)...
	$(RM) $(EMULATOR)

clean68k:
	@echo Deleting 68k files...
	$(RM) -r $(OBJ)/cpuintrf.o
	$(RM) -r $(OBJ)/drivers/cps2.o
	$(RM) -r $(OBJ)/cpu/m68000

check: $(EMULATOR) xml2info$(EXE)
	./$(EMULATOR) -listxml > $(NAME).xml
	./xml2info < $(NAME).xml > $(NAME).lst
	./xmllint --valid --noout $(NAME).xml


