# define CPSMAME
CPSMAME = 1

# tiny compile
COREDEFS = -DCPSMAME

# override executable name
EMULATOR_EXE = cpsmame.exe

# CPUs
CPUS+=Z80@
CPUS+=M68000@
# bug, won't compile with the asm core otherwise
CPUS+=M68020@

# SOUNDs
SOUNDS+=YM2151_ALT@
SOUNDS+=OKIM6295@
SOUNDS+=QSOUND@

DRVLIBS = $(OBJ)/cps.a

$(OBJ)/cps.a: \
	$(OBJ)/machine/kabuki.o \
	$(OBJ)/vidhrdw/cps1.o $(OBJ)/drivers/cps1.o $(OBJ)/drivers/cps2.o \

# MAME specific core objs
COREOBJS += $(OBJ)/driver.o $(OBJ)/cheat.o

