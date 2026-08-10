# ---------------------------------------------------------------------------
#  Optional Pinball 2000 subsystem (src/p2k)
#
#  ON by default; disable with -DPINMAME_P2K=OFF, which leaves neither
#  src/wpc/p2k.c's games nor the MediaGX CPU registered.
#
#  Needs a C++17 compiler: the object
#  library below sets CXX_STANDARD 17 with CXX_STANDARD_REQUIRED. The unix
#  makefile keeps its own opt-in switch (make P2K=1); the hand-kept Visual
#  Studio projects get it from vcproj/p2k.props, which the create_vc2017 and
#  later conversion scripts import because only a v141+ toolset can build it.
#
#  The subsystem is built as its own OBJECT library rather than folded into the
#  consumer's source list, because it needs settings the rest of PinMAME must
#  not have, and two of them bite hard:
#
#    - RTTI. The shim's device_t::subdevice() uses dynamic_cast, and every
#      PinMAME target compiles with -fno-rtti / /GR-.
#    - Include order. PinMAME has its own src/machine/pic8259.h, and with src/
#      searched first the imported MAME sources pick that up instead of their
#      own - which shows as "invalid use of incomplete type pic8259_device",
#      not as a missing file. Hence BEFORE on the include directories.
#
#  Kept in step with src/p2k/p2k.mak and vcproj/p2k.props: a source added to
#  one belongs in all three.
#
#  Usage in a top-level CMakeLists: last, after the target's own
#  target_compile_options() and set_target_properties() - the object library
#  copies both (see COMPILE_OPTIONS and MSVC_RUNTIME_LIBRARY below) - do
#      include(${CMAKE_SOURCE_DIR}/cmake/p2k.cmake)
#      pinmame_enable_p2k(<target>)
# ---------------------------------------------------------------------------

option(PINMAME_P2K "Build the Pinball 2000 subsystem (src/p2k)" ON)

# The bring-up apparatus - traces, watches, dumps, the stand-in playfield, and the P2K_*
# environment variables that drive them. Off, and not compiled at all: see src/p2k/p2k_debug.h.
option(PINMAME_P2K_DEBUG "Build the Pinball 2000 debugging apparatus (P2K_* env vars)" OFF)

# Repo layout anchor, captured at INCLUDE time. Inside a function,
# CMAKE_CURRENT_LIST_DIR resolves to the CALLER's list file directory (the
# top-level CMakeLists), which would point the paths below outside the repo.
set(_PINMAME_P2K_LIST_DIR ${CMAKE_CURRENT_LIST_DIR})

set(_PINMAME_P2K_SOURCES
   src/p2k/p2k_machine.cpp
   src/p2k/p2k_driver.cpp
   src/p2k/p2k_cpuintrf.cpp
   src/p2k/p2k_pinmame.cpp
   src/p2k/p2k_selftest.cpp
   src/p2k/shim/emu.cpp
   src/p2k/shim/coreutil_bcd.cpp
   src/p2k/shim/machine/pckeybrd.cpp
   src/p2k/mame/cpu/i386/i386.cpp
   src/p2k/mame/cpu/i386/i386dasm.cpp
   src/p2k/mame/emu/divtlb.cpp
   src/p2k/mame/emu/attotime.cpp
   src/p2k/mame/emu/diserial.cpp
   src/p2k/mame/machine/pic8259.cpp
   src/p2k/mame/machine/pit8253.cpp
   src/p2k/mame/machine/am9517a.cpp
   src/p2k/mame/machine/mc146818.cpp
   src/p2k/mame/machine/8042kbdc.cpp
   src/p2k/mame/machine/lpci.cpp
   src/p2k/mame/machine/nvram.cpp
   src/p2k/mame/machine/ins8250.cpp
)

# SoftFloat is C source that MAME compiles as C++, and so must we.
set(_PINMAME_P2K_SOURCES_C
   src/p2k/mame/3rdparty/softfloat/softfloat.c
   src/p2k/mame/3rdparty/softfloat/fpatan.c
   src/p2k/mame/3rdparty/softfloat/fsincos.c
   src/p2k/mame/3rdparty/softfloat/fyl2x.c
)

set(_PINMAME_P2K_INCLUDES
   src/p2k/shim
   src/p2k/mame
   src/p2k/mame/cpu/i386
   src/p2k/mame/emu
   src/p2k/mame/osd
   src/p2k/mame/3rdparty
   src/p2k/mame/3rdparty/softfloat
   src/p2k/mame/machine
   src/p2k/mame/video
)

# Build the object library once, on first use.
function(_pinmame_p2k_add_library reference_target)
   set(_srcs "")
   foreach(_s IN LISTS _PINMAME_P2K_SOURCES _PINMAME_P2K_SOURCES_C)
      list(APPEND _srcs ${_PINMAME_P2K_LIST_DIR}/../${_s})
   endforeach()
   add_library(p2k OBJECT ${_srcs})

   set(_c_srcs "")
   foreach(_s IN LISTS _PINMAME_P2K_SOURCES_C)
      list(APPEND _c_srcs ${_PINMAME_P2K_LIST_DIR}/../${_s})
   endforeach()
   set_source_files_properties(${_c_srcs} PROPERTIES LANGUAGE CXX)

   set(_incs "")
   foreach(_i IN LISTS _PINMAME_P2K_INCLUDES)
      list(APPEND _incs ${_PINMAME_P2K_LIST_DIR}/../${_i})
   endforeach()
   target_include_directories(p2k BEFORE PRIVATE ${_incs})

   set_target_properties(p2k PROPERTIES
      CXX_STANDARD 17
      CXX_STANDARD_REQUIRED ON
      POSITION_INDEPENDENT_CODE ON
   )

   # Directory-scope options are inherited at target creation, but the Windows
   # lists set their ${OPT_COMMON} on the target instead - which an OBJECT
   # library does not see, leaving the i386 core and softfloat on CMake's bare
   # /O2 /Ob2 and out of the /LTCG link for want of /GL. So copy them, minus the
   # directory's own (p2k already inherited those), and ahead of p2k's own
   # options below so those still win on /GR- vs /GR.
   get_target_property(_consumer_options ${reference_target} COMPILE_OPTIONS)

   if(NOT _consumer_options)
      set(_consumer_options "")
   else()
      get_directory_property(_dir_options COMPILE_OPTIONS)
      if(_dir_options)
         list(REMOVE_ITEM _consumer_options ${_dir_options})
      endif()
   endif()

   if(MSVC)
      # /GR after the consumer's /GR- wins - MSVC takes the last setting on the
      # command line - and /w and /EHsc keep the imported MAME sources quiet and
      # exception-correct regardless of the consumer's flags.
      target_compile_options(p2k PRIVATE
        ${_consumer_options}
        /GR
        /EHsc
        /w
      )

      # An OBJECT library does not inherit the consuming target's CRT choice,
      # and a mismatch is a link error (RuntimeLibrary LIBCMT vs MSVCRT).
      # This is why pinmame_enable_p2k() has to be called after the consumer's
      # set_target_properties().
      get_target_property(_rt ${reference_target} MSVC_RUNTIME_LIBRARY)
      if(_rt)
         set_target_properties(p2k PROPERTIES MSVC_RUNTIME_LIBRARY "${_rt}")
      endif()
   else()
      target_compile_options(p2k PRIVATE
        ${_consumer_options}
        -frtti
        -fno-strict-aliasing
        -w
      )
   endif()

   if(PINMAME_P2K_DEBUG)
      target_compile_definitions(p2k PRIVATE P2K_DEBUG=1)
   endif()
endfunction()

# Attach the Pinball 2000 subsystem to a pinmame target.
# No-op unless PINMAME_P2K is ON.
function(pinmame_enable_p2k target)
   if(NOT PINMAME_P2K)
      return()
   endif()
   if(NOT TARGET p2k)
      _pinmame_p2k_add_library(${target})
   endif()
   # src/wpc/p2k.c is PinMAME's half - machine driver, game data, switch/lamp/
   # coil mapping, video renderer, NVRAM - and belongs in the consumer, built
   # with the consumer's own settings. It is itself wrapped in #if HAS_MEDIAGX.
   target_sources(${target} PRIVATE
      ${_PINMAME_P2K_LIST_DIR}/../src/wpc/p2k.c
      $<TARGET_OBJECTS:p2k>
   )
   # This is the switch src/wpc/driver.c already tests to list the games, and
   # the one the unix build derives from CPUS += MEDIAGX@. Defining it only for
   # p2k.c would leave the driver compiled but the games unlisted - which is
   # exactly what happened first time round.
   target_compile_definitions(${target} PRIVATE HAS_MEDIAGX=1)
   # src/wpc/p2k.c and src/wpc/wmssnd.c carry their half of the debug apparatus, and they are
   # compiled into the consumer - so the switch has to reach it too, not just the object library.
   if(PINMAME_P2K_DEBUG)
      target_compile_definitions(${target} PRIVATE P2K_DEBUG=1)
   endif()
endfunction()
