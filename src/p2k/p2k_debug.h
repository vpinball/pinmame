// license:BSD-3-Clause

// P2K DEBUG switch
//
// Bringing this board up took a lot of apparatus: instruction and trap traces, backtraces,
// memory and I/O watches, register and stack dumps, frame and program-memory dumps, and a
// stand-in playfield so a standalone build can start a game with nothing feeding its switches.
// All of it is driven by P2K_* environment variables, and none of it belongs in a build endusers
// play on: a variable left set in someone's environment must not be able to change how the
// machine runs, and the probes cost a tiny bit, too
//
// So its all behind this switch:
//
//   cmake           -DPINMAME_P2K_DEBUG=ON
//   makefile.unix   make P2K=1 P2K_DEBUG=1
//   Visual Studio   add P2K_DEBUG=1 to the project's preprocessor definitions
//
// src/p2k/README.md lists what each variable does

#pragma once

#ifndef P2K_DEBUG
#define P2K_DEBUG 0
#endif
