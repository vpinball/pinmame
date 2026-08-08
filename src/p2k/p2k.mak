# Pinball 2000 subsystem - build rules.
#
# Self-contained: everything here lives under src/p2k/ and is only linked when P2K=1 is set.
# Included from src/unix/unix.mak.

P2K_OBJ = $(NAME).obj/p2k

P2K_INC = -Isrc/p2k/shim -Isrc/p2k/mame -Isrc/p2k/mame/cpu/i386 -Isrc/p2k/mame/emu \
          -Isrc/p2k/mame/osd -Isrc/p2k/mame/3rdparty -Isrc/p2k/mame/machine -Isrc/p2k/mame/video

# The imported MAME sources need a modern C++ dialect; PinMAME's own C flags do not apply.
P2K_CXXFLAGS = -std=gnu++17 -O2 -fno-strict-aliasing -w $(P2K_INC)

# The bring-up apparatus - traces, watches, dumps, the stand-in playfield, and the fifty-odd
# P2K_* environment variables that drive them. None of it is compiled unless this is set, so
# it goes into DEFS as well: src/wpc/p2k.c and src/wpc/wmssnd.c are on PinMAME's side of the
# build and carry their half of it. See src/p2k/p2k_debug.h
ifdef P2K_DEBUG
P2K_CXXFLAGS += -DP2K_DEBUG=1
DEFS += -DP2K_DEBUG=1
endif

P2K_OBJDIRS = $(P2K_OBJ) $(P2K_OBJ)/mame/cpu/i386 $(P2K_OBJ)/mame/emu \
              $(P2K_OBJ)/mame/3rdparty/softfloat $(P2K_OBJ)/mame/machine \
              $(P2K_OBJ)/mame/video $(P2K_OBJ)/shim $(P2K_OBJ)/shim/machine

P2KOBJS = \
	$(P2K_OBJ)/mame/cpu/i386/i386.o \
	$(P2K_OBJ)/mame/cpu/i386/i386dasm.o \
	$(P2K_OBJ)/mame/emu/divtlb.o \
	$(P2K_OBJ)/mame/emu/attotime.o \
	$(P2K_OBJ)/mame/emu/diserial.o \
	$(P2K_OBJ)/shim/coreutil_bcd.o \
	$(P2K_OBJ)/mame/machine/pic8259.o \
	$(P2K_OBJ)/mame/machine/pit8253.o \
	$(P2K_OBJ)/mame/machine/am9517a.o \
	$(P2K_OBJ)/mame/machine/mc146818.o \
	$(P2K_OBJ)/mame/machine/8042kbdc.o \
	$(P2K_OBJ)/mame/machine/lpci.o \
	$(P2K_OBJ)/mame/machine/nvram.o \
	$(P2K_OBJ)/mame/machine/ins8250.o \
	$(P2K_OBJ)/mame/3rdparty/softfloat/softfloat.o \
	$(P2K_OBJ)/mame/3rdparty/softfloat/fpatan.o \
	$(P2K_OBJ)/mame/3rdparty/softfloat/fsincos.o \
	$(P2K_OBJ)/mame/3rdparty/softfloat/fyl2x.o \
	$(P2K_OBJ)/shim/emu.o \
	$(P2K_OBJ)/shim/machine/pckeybrd.o \
	$(P2K_OBJ)/p2k_machine.o \
	$(P2K_OBJ)/p2k_driver.o \
	$(P2K_OBJ)/p2k_cpuintrf.o \
	$(P2K_OBJ)/p2k_pinmame.o \
	$(P2K_OBJ)/p2k_selftest.o

# The shim headers define the class layout of everything here, and debugger.h decides whether the
# imported cores call back per instruction - rebuild all of it when any of them changes. The
# subsystem's own headers belong here for the same reason: a member added to p2k_state changes
# its size, and a stale object left over from before that will corrupt the heap. Listing them by
# name has gone wrong three times, so this is a wildcard.
P2KHDRS := $(wildcard src/p2k/*.h src/p2k/shim/*.h src/p2k/shim/*/*.h) src/p2k/mame/emu/divtlb.h

$(P2KOBJS): $(P2KHDRS)

$(P2K_OBJ)/%.o: src/p2k/%.cpp
	$(CC_COMMENT) @echo 'Compiling $< ...'
	$(CC_COMPILE) $(CPP) $(P2K_CXXFLAGS) -o $@ -c $<

$(P2K_OBJ)/mame/%.o: src/p2k/mame/%.cpp
	$(CC_COMMENT) @echo 'Compiling $< ...'
	$(CC_COMPILE) $(CPP) $(P2K_CXXFLAGS) -o $@ -c $<

# SoftFloat is C source, but MAME builds it as C++ as well.
$(P2K_OBJ)/mame/3rdparty/softfloat/%.o: src/p2k/mame/3rdparty/softfloat/%.c
	$(CC_COMMENT) @echo 'Compiling $< ...'
	$(CC_COMPILE) $(CPP) $(P2K_CXXFLAGS) -Isrc/p2k/mame/3rdparty/softfloat -o $@ -c $<

# Boot harness: loads ROMs and runs the machine from the reset vector.
p2kboot: $(P2K_OBJDIRS) $(P2KOBJS)
	$(CC_COMPILE) $(CPP) $(P2K_CXXFLAGS) -o $(P2K_OBJ)/p2k_boot.o -c src/p2k/p2k_boot.cpp
	$(CC_COMPILE) $(CPP) -o p2kboot $(P2K_OBJ)/p2k_boot.o \
		$(filter-out $(P2K_OBJ)/p2k_selftest.o,$(P2KOBJS))
	@echo 'built p2kboot'

# Standalone self-test binary: runs the imported CPU core without starting PinMAME.
p2ktest: $(P2K_OBJDIRS) $(P2KOBJS)
	$(CC_COMPILE) $(CPP) $(P2K_CXXFLAGS) -DP2K_SELFTEST_MAIN -o $(P2K_OBJ)/selftest_main.o \
		-c src/p2k/p2k_selftest.cpp
	$(CC_COMPILE) $(CPP) -o p2ktest $(P2K_OBJ)/selftest_main.o \
		$(filter-out $(P2K_OBJ)/p2k_selftest.o,$(P2KOBJS))
	@echo 'built p2ktest'
