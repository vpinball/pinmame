# a tiny compile is without Neogeo games
COREDEFS += -DTINY_COMPILE=1
COREDEFS += -DTINY_NAME="driver_redclash,driver_redclask"
COREDEFS += -DTINY_POINTER="&driver_redclash,&driver_redclask"

# uses these CPUs
CPUS+=CPU_Z80@

# uses these SOUNDs
SOUNDS+=SN76496@

OBJS = $(OBJ)/drivers/redclash.o $(OBJ)/vidhrdw/redclash.o

# MAME specific core objs
COREOBJS += $(OBJ)/driver.o $(OBJ)/cheat.o
