// license:BSD-3-Clause

// SPIKE1 DEBUG switch
//
// SPIKE 1 machine debugging used a lot of logging: every system call the game program makes,
// the node bus traffic, the I2C transfers, what the device model does not model yet, the switch,
// coil and LED tables it read, and the driver's sound and timing statistics. Most of it builds its
// text on paths the game runs all the time, and none of it is needed in a build endusers play on.
// It is all behind this switch, compiled out without it, so it costs nothing:
//
//   cmake           -DPINMAME_SPIKE1_DEBUG=ON (cmake/spike1boot, the bring-up harness, always has it)
//   Visual Studio   add SPIKE1_DEBUG=1 to the project's preprocessor definitions
//
// With it, SPIKE1_LOG set in the environment passes the subsystem's whole log, and the driver's
// statistics, to stderr. Without it, only a stop of the game program and the NVRAM's fate are
// reported (spike1_pinmame.cpp): rare, and what a user needs to know why a game is not running.

#pragma once

#ifndef SPIKE1_DEBUG
#define SPIKE1_DEBUG 0
#endif

#if SPIKE1_DEBUG
// a line for the subsystem's log, in a member function of spike1_linux or spike1_devices
#define SPIKE1_LOG(...) log(__VA_ARGS__)
// a statement that only exists for the log: building a note, or the bookkeeping behind one. Without
// SPIKE1_DEBUG it is an empty statement, so a bare if or else around it keeps its own (empty) body
#define SPIKE1_DEBUG_ONLY(...) __VA_ARGS__
#else
#define SPIKE1_LOG(...) do { } while (0)
#define SPIKE1_DEBUG_ONLY(...) ;
#endif
