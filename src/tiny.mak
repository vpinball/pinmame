# a tiny compile is without Neogeo games
COREDEFS += -DTINY_COMPILE=1
COREDEFS += -DTINY_NAME="driver_buckrog,driver_zoom909,driver_zoom909a"
COREDEFS += -DTINY_POINTER="&driver_buckrog,&driver_zoom909,&driver_zoom909a"

# uses these CPUs
CPUS+=CPU_Z80@

# uses these SOUNDs
SOUNDS+=SOUND_SAMPLES@

OBJS = $(OBJ)/drivers/turbo.o $(OBJ)/machine/turbo.o $(OBJ)/vidhrdw/turbo.o $(OBJ)/machine/segacrpt.o

# MAME specific core objs
COREOBJS += $(OBJ)/driver.o $(OBJ)/cheat.o
