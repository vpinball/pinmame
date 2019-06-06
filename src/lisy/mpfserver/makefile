# lisy Makefile for mpfserver
# RTH 04.2018

CC      = /usr/bin/gcc
CFLAGS  = -g -Wunused-variable
LDFLAGS = -lwiringPi -lSDL2 -lSDL2_mixer

OBJ = lisy.o fileio.o utils.o hw_lib.o switches.o displays.o coils.o eeprom.o mpfserver.o sound.o fadecandy.o opc.o linked_list.o

mpfserver: $(OBJ)
	$(CC) $(CFLAGS) -o mpfserver $(OBJ) $(LDFLAGS)

%.o: ../%.c ../%.h
	$(CC) $(CFLAGS) -c $<

