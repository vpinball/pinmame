# define NEOMAME
NEOMAME = 1

# tiny compile
COREDEFS = -DNEOMAME

# override executable name
EMULATOR_EXE = neomame.exe

# CPUs
CPUS+=Z80@
CPUS+=M68000@

# SOUNDs
SOUNDS+=AY8910@
SOUNDS+=YM2610@

DRVLIBS = $(OBJ)/neogeo.a

$(OBJ)/neogeo.a: \
	$(OBJ)/machine/neogeo.o $(OBJ)/machine/neocrypt.o $(OBJ)/machine/pd4990a.o $(OBJ)/vidhrdw/neogeo.o $(OBJ)/drivers/neogeo.o \

# MAME specific core objs
COREOBJS += $(OBJ)/driver.o $(OBJ)/cheat.o
