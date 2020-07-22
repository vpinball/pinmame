###########################################################################
#
#   makefile
#
#   Core makefile for building MAME and derivatives
#
#   Copyright (c) Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
#   Adopted for PinMAME, which is based on MAME 0.76, so several parts of
#   this makefile are still like in MAME 0.76
#
###########################################################################



###########################################################################
#################   BEGIN USER-CONFIGURABLE OPTIONS   #####################
###########################################################################


#-------------------------------------------------
# specify core target: mame, mess, etc.
# build rules will be included from
# src/$(TARGET).mak
#-------------------------------------------------

# set this to mame, mess or the destination you want to build
# TARGET = mame
# TARGET = mess
# TARGET = neomame
# TARGET = cpmame
# TARGET = pinmame
# TARGET = mmsnd
# example for a tiny compile
# TARGET = tiny
ifndef TARGET
# [PinMAME] default to building PinMAME
#TARGET = mame
TARGET = pinmame
endif



#-------------------------------------------------
# specify OS target, which further differentiates
# the underlying OS;
#-------------------------------------------------

# set this to the operating system you're building for
# MAMEOS = msdos
# MAMEOS = windows
ifeq ($(MAMEOS),)
MAMEOS = windows
endif



#-------------------------------------------------
# configure name of final executable
#-------------------------------------------------

# uncomment and specify prefix to be added to the name
# PREFIX =

# uncomment and specify suffix to be added to the name
# SUFFIX =



#-------------------------------------------------
# specify architecture-specific optimizations
#-------------------------------------------------

# uncomment and specify architecture-specific optimizations here
# some examples:
#   optimize for I686:   ARCHOPTS = -march=pentiumpro
#   optimize for Core 2: ARCHOPTS = -march=pentium-m -msse3
#   optimize for G4:     ARCHOPTS = -mcpu=G4
# note that we leave this commented by default so that you can
# configure this in your environment and never have to think about it
# ARCHOPTS =



#-------------------------------------------------
# specify PinMAME options; see each option below
# for details
#-------------------------------------------------

# [PinMAME] uncomment next line to include running older stern whitestar games in new at91 cpu board for testing
# TEST_NEW_SOUND = 1



#-------------------------------------------------
# specify program options; see each option below
# for details
#-------------------------------------------------

# uncomment next line to include the MAME ROM debugger
# DEBUG = 1

# uncomment next line to include P-ROC support
# see http://www.pinballcontrollers.com/
# PROC = 1



#-------------------------------------------------
# specify build options; see each option below
# for details
#-------------------------------------------------

# uncomment next line to include the symbols (for debugging)
# [PinMAME] always create symbols file (separated from executable)
#SYMBOLS = 1

# uncomment next line to generate a link map for exception handling in windows
# [PinMAME] always create map file
#MAP = 1

# uncomment next line to generate verbose build information
# VERBOSE = 1

# specify optimization level or leave commented to use the default
# (default is OPTIMIZE = 3 normally, or OPTIMIZE = 0 with symbols)
# [PinMAME] always defaults to 3
# OPTIMIZE = 3

# uncomment next line to use Assembler 68000 engine
# X86_ASM_68000 = 1

# uncomment next line to use Assembler 68020 engine
# X86_ASM_68020 = 1

# uncomment next line to use DRC MIPS3 engine
X86_MIPS3_DRC = 1

# uncomment next line to use cygwin compiler
# COMPILESYSTEM_CYGWIN = 1


###########################################################################
##################   END USER-CONFIGURABLE OPTIONS   ######################
###########################################################################


#-------------------------------------------------
# macros for arithemtics
# taken from http://www.cmcrossroads.com/ask-mr-make/6504-learning-gnu-make-functions-with-arithmetic
#-------------------------------------------------

# decode turns a number in x's representation into a integer for human
# consumption
decode = $(words $1)

# encode takes an integer and returns the appropriate x's representation
# of the number by chopping $1 x's from the start of input_int
16 = x x x x x x x x x x x x x x x x
max_int := $(foreach a,$(16),$(foreach b,$(16),$(foreach c,$(16),$(16)))))
encode = $(wordlist 1,$1,$(max_int))

# max returns the maximum of its arguments and min the minimum
max = $(subst xx,x,$(join $1,$2))
min = $(subst xx,x,$(filter xx,$(join $1,$2)))

# The following operators return a non-empty string if their result
# is true:
#
# gt   First argument greater than second argument
# gte  First argument greater than or equal to second argument
# lt   First argument less than second argument
# lte  First argument less than or equal to second argument
# eq   First argument is numerically equal to the second argument
# ne   First argument is not numerically equal to the second argument
gt = $(filter-out $(words $2),$(words $(call max,$1,$2)))
lt = $(filter-out $(words $1),$(words $(call max,$1,$2)))
eq = $(filter $(words $1),$(words $2))
ne = $(filter-out $(words $1),$(words $2))
gte = $(call gt,$1,$2)$(call eq,$1,$2)
lte = $(call lt,$1,$2)$(call eq,$1,$2)

#-------------------------------------------------
# sanity check the configuration
#-------------------------------------------------

# specify a default optimization level if none explicitly stated
ifndef OPTIMIZE
# [PinMAME] always defaults to 3
#ifndef SYMBOLS
OPTIMIZE = 3
#else
#OPTIMIZE = 0
#endif
endif


#-------------------------------------------------
# platform-specific definitions
#-------------------------------------------------

# extension for executables
EXE =

ifeq ($(MAMEOS),windows)
EXE = .exe
endif
ifeq ($(MAMEOS),os2)
EXE = .exe
endif

ifndef BUILD_EXE
BUILD_EXE = $(EXE)
endif

# compiler, linker and utilities
AR = @ar
CC = @gcc
CPP = @g++
LD = @g++
ASM = @nasm
ASMFLAGS = -f coff
MD = -mkdir$(EXE)
RM = @rm -f
#PERL = @perl -w
OBJCOPY = @objcopy

CC_VERSION := $(shell $(subst @,,$(CC)) -dumpversion)
CC_MAJOR := $(word 1,$(subst ., ,$(CC_VERSION)))
CC_MINOR := $(word 2,$(subst ., ,$(CC_VERSION)))
CC_PATCH := $(word 3,$(subst ., ,$(CC_VERSION)))

CPP_VERSION := $(shell $(subst @,,$(CPP)) -dumpversion)
CPP_MAJOR := $(word 1,$(subst ., ,$(CPP_VERSION)))
CPP_MINOR := $(word 2,$(subst ., ,$(CPP_VERSION)))
CPP_PATCH := $(word 3,$(subst ., ,$(CPP_VERSION)))


#-------------------------------------------------
# form the name of the executable
#-------------------------------------------------

# P-ROC builds just get the 'p' suffix and nothing more
ifdef PROC
PROCSUFFIX = p
endif

# debug builds just get the 'd' suffix and nothing more
ifdef DEBUG
DEBUGSUFFIX = d
endif

# the main name is just 'target'
NAME = $(TARGET)

# fullname is prefix+name+suffix+debugsuffix
FULLNAME = $(PREFIX)$(NAME)$(SUFFIX)$(PROCSUFFIX)$(DEBUGSUFFIX)

# add an EXE suffix to get the final emulator name
EMULATOR = $(FULLNAME)$(EXE)



#-------------------------------------------------
# source and object locations
#-------------------------------------------------

# CPU core include paths
VPATH = src $(wildcard src/cpu/*)

# build the targets in different object dirs, since mess changes
# some structures and thus they can't be linked against each other.
OBJ = obj/gcc/$(MAMEOS)/$(FULLNAME)



#-------------------------------------------------
# compile-time definitions
#-------------------------------------------------

# CR/LF setup: use both on win32/os2, CR only on everything else
DEFS = -DCRLF=2

ifeq ($(MAMEOS),windows)
DEFS = -DCRLF=3
endif
ifeq ($(MAMEOS),os2)
DEFS = -DCRLF=3
endif

# map the INLINE to something digestible by GCC
DEFS += -DINLINE="static __inline__"

# define LSB_FIRST if we are a little-endian target
ifndef BIGENDIAN
DEFS += -DLSB_FIRST
endif

# define MAME_DEBUG if we are a debugging build
ifdef DEBUG
DEFS += -DMAME_DEBUG
endif

# define DEBUG if we are a non-optimized build
ifeq ($(OPTIMIZE),0)
DEFS += -DDEBUG -D_DEBUG
else
DEFS += -DNDEBUG
endif

DEFS += -DX86_ASM -Dasm=__asm__

# [PinMAME] running older stern whitestar games in new at91 cpu board for testing
ifdef TEST_NEW_SOUND
DBGDEFS += -DTEST_NEW_SOUND
endif



#-------------------------------------------------
# compile flags
#-------------------------------------------------

CFLAGS =
CPPFLAGS =

CFLAGS += -std=gnu99
CPPFLAGS += -std=gnu++11

# add -g if we need symbols, and ensure we have frame pointers
# [PinMAME] not omiting frame pointers is very helpful for stack traces, and there's hardly a performance gain if you do omit
ifdef SYMBOLS
CFLAGS += -g
CPPFLAGS += -g
endif
CFLAGS += -fno-omit-frame-pointer
CPPFLAGS += -fno-omit-frame-pointer

# add -v if we need verbose build information
ifdef VERBOSE
CFLAGS += -v
CPPFLAGS += -v
endif

# add the optimization flag
CFLAGS += -O$(OPTIMIZE)
CPPFLAGS += -O$(OPTIMIZE)

# if we are optimizing, include optimization options
ifneq ($(OPTIMIZE),0)
CFLAGS += $(ARCHOPTS)
CPPFLAGS += $(ARCHOPTS)
endif

# add MAME 0.76 basic set of warnings
CFLAGS += \
	-fstrict-aliasing \
	-Wall -Wno-sign-compare -Wunused \
	-Wpointer-arith -Wbad-function-cast -Wcast-align -Waggregate-return \
	-Wshadow -Wstrict-prototypes -Wundef \
	-Wformat-security -Wwrite-strings \
	-Wdisabled-optimization \
#	-Wredundant-decls
#	-Wfloat-equal
#	-Wunreachable-code -Wpadded
#	-W had to remove because of the "missing initializer" warning
#	-Wlarger-than-262144 \
#	-Wcast-qual \
#	-Wwrite-strings \
#	-Wconversion \
#	-Wmissing-prototypes \
#	-Wmissing-declarations

CPPFLAGS += \
	-fstrict-aliasing \
	-Wall -Wno-sign-compare -Wunused \
	-Wpointer-arith -Wcast-align \
	-Wshadow -Wundef \
	-Wformat-security -Wwrite-strings \
	-Wdisabled-optimization \
#	-Waggregate-return (removed to eliminate warnings from p-roc and
#	                    yaml-cpp functions returning structures, which
#	                    is not a problem.)
#	-Wredundant-decls
#	-Wfloat-equal
#	-Wunreachable-code -Wpadded
#	-W had to remove because of the "missing initializer" warning
#	-Wlarger-than-262144 \
#	-Wcast-qual \
#	-Wwrite-strings \
#	-Wconversion

CFLAGSPEDANTIC = $(CFLAGS) -pedantic
CPPFLAGSPEDANTIC = $(CPPFLAGS) -pedantic


#-------------------------------------------------
# include paths
#-------------------------------------------------

CFLAGS += \
	-Isrc \
	-Isrc/includes \
	-Isrc/$(MAMEOS) \
	-I$(OBJ)/cpu/m68000 \
	-Isrc/cpu/m68000

CPPFLAGS += \
	-Isrc \
	-Isrc/includes \
	-Isrc/$(MAMEOS) \
	-I$(OBJ)/cpu/m68000 \
	-Isrc/cpu/m68000



#-------------------------------------------------
# linking flags
#-------------------------------------------------

# general
# [PinMAME] avoid GCC 4.4 warnings (ToDo: real fix better than disabling warning)
#LDFLAGS = -Wl,--warn-common
LDFLAGS =
MAPFLAGS =

# Allow duplicate symbol definitions (e.g., non-extern entry in a header file)
LDFLAGS += -Wl,--allow-multiple-definition

# strip symbols and other metadata in non-symbols builds
ifndef SYMBOLS
LDFLAGS += -s
endif

# output a map file
ifdef MAP
# [PinMAME] include cross reference
MAPFLAGS += -Wl,-Map,$(FULLNAME).map,--cref
endif

# [PinMAME] avoid dynamic dependencies on Windows, so link statically
ifeq ($(MAMEOS),windows)
LDFLAGS += -static -static-libgcc
endif



#-------------------------------------------------
# define the standard object directory; other
# projects can add their object directories to
# this variable
#-------------------------------------------------

OBJDIRS = $(OBJ) $(OBJ)/cpu $(OBJ)/sound $(OBJ)/$(MAMEOS) \
	$(OBJ)/machine $(OBJ)/vidhrdw
# [PinMANE] some dirs are not necessary for PinMAME
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



#-------------------------------------------------
# either build or link against the included
# libraries
#-------------------------------------------------

# platform .mak files will want to add to this
LIBS = -lz



#-------------------------------------------------
# 'all' target needs to go here, before the
# include files which define additional targets
#-------------------------------------------------

all: maketree $(EMULATOR) extra



#-------------------------------------------------
# include the various .mak files
#-------------------------------------------------

include src/core.mak
include src/$(TARGET).mak
include src/rules.mak
include src/$(MAMEOS)/$(MAMEOS).mak
ifdef PROC
# add include directories for libpinproc and yaml-cpp
CFLAGS += -Iext/pinproc/include -Iext/yaml-cpp/include
CPPFLAGS += -Iext/pinproc/include -Iext/yaml-cpp/include
# add library directories
LDFLAGS += -Lext/pinproc/lib -Lext/yaml-cpp/lib
include src/p-roc/p-roc.mak
endif

# if the MAME ROM debugger is not included, then remove objects for it
ifndef DEBUG
DBGOBJS =
endif

# combine the various definitions to one
CDEFS = $(DEFS) $(COREDEFS) $(CPUDEFS) $(SOUNDDEFS) $(ASMDEFS) $(DBGDEFS)



#-------------------------------------------------
# primary targets
#-------------------------------------------------

extra:	$(TOOLS) $(TEXTS)

# primary target(s)
$(EMULATOR).full: $(OBJS) $(COREOBJS) $(OSOBJS) $(DRVLIBS) $(PROCOBJS)
# always recompile the version string
	$(CC) $(CDEFS) $(CFLAGS) -c src/version.c -o $(OBJ)/version.o
	@echo Linking $@...
	$(LD) $(LDFLAGS) $(OBJS) $(COREOBJS) $(OSOBJS) $(PROCOBJS) $(LIBS) $(DRVLIBS) $(PROCLIBS) -o $@ $(MAPFLAGS)

ifdef SYMBOLS
# [PinMAME] extract debug information into separate file, then strip executable and add a debug link to it
#           see http://sourceware.org/gdb/current/onlinedocs/gdb_19.html#SEC170
#               http://stackoverflow.com/questions/866721/
$(EMULATOR).debug: $(EMULATOR).full
	@echo Extracting debug symbols to $@...
	$(OBJCOPY) -p --only-keep-debug "$(EMULATOR).full" "$(EMULATOR).debug"

$(EMULATOR): $(EMULATOR).debug
	@echo Stripping unneeded symbols from $@...
	$(RM) "$(EMULATOR).strip"
	$(OBJCOPY) -p --strip-unneeded "$(EMULATOR).full" "$(EMULATOR).strip"
	@echo Adding GNU debuglink to $@...
	$(RM) "$(EMULATOR)"
	$(OBJCOPY) -p --add-gnu-debuglink="$(EMULATOR).debug" "$(EMULATOR).strip" "$(EMULATOR)"
	$(RM) "$(EMULATOR).strip"
else
$(EMULATOR): $(EMULATOR).full
	@echo Stripping unneeded symbols from $@...
	$(OBJCOPY) -p --strip-unneeded "$(EMULATOR).full" "$(EMULATOR)"
endif

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
$(OBJ)/cpu/m68000/68000.asm: src/cpu/m68000/make68k.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGSPEDANTIC) -O0 -DDOS -o $(OBJ)/cpu/m68000/make68k$(EXE) $<
	@echo Generating $@...
	@$(OBJ)/cpu/m68000/make68k$(EXE) $@ $(OBJ)/cpu/m68000/68000tab.asm 00

$(OBJ)/cpu/m68000/68020.asm: src/cpu/m68000/make68k.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGSPEDANTIC) -O0 -DDOS -o $(OBJ)/cpu/m68000/make68k$(EXE) $<
	@echo Generating $@...
	@$(OBJ)/cpu/m68000/make68k$(EXE) $@ $(OBJ)/cpu/m68000/68020tab.asm 20

# generated asm files for the 68000 emulator
$(OBJ)/cpu/m68000/68000.o: $(OBJ)/cpu/m68000/68000.asm
	@echo Assembling $<...
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

$(OBJ)/cpu/m68000/68020.o: $(OBJ)/cpu/m68000/68020.asm
	@echo Assembling $<...
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

$(OBJ)/%.a:
	@echo Archiving $@...
	$(RM) $@
	$(AR) cr $@ $^

makedir:
	@echo make makedir is no longer necessary, just type make

$(sort $(OBJDIRS)):
	$(MD) -p $@

maketree: $(sort $(OBJDIRS))

clean:
	@echo Deleting object tree $(OBJ)...
	$(RM) -r $(OBJ)
	@echo Deleting $(EMULATOR)...
	$(RM) $(EMULATOR).full
	$(RM) $(EMULATOR)
	@echo Deleting $(EMULATOR).debug...
	$(RM) $(EMULATOR).debug
	$(RM) $(EMULATOR).strip
	@echo Deleting $(FULLNAME).map...
	$(RM) $(FULLNAME).map

clean68k:
	@echo Deleting 68k files...
	$(RM) -r $(OBJ)/cpuintrf.o
	$(RM) -r $(OBJ)/drivers/cps2.o
	$(RM) -r $(OBJ)/cpu/m68000

check: $(EMULATOR) xml2info$(EXE)
	./$(EMULATOR) -listxml > $(FULLNAME).xml
	./xml2info < $(FULLNAME).xml > $(FULLNAME).lst
	./xmllint --valid --noout $(FULLNAME).xml
