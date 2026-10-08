# ---------------------------------------------------------------------------
#  Optional Stern Spike 1 subsystem (src/spike1)
#
#  ON by default; disable with -DPINMAME_SPIKE1=OFF, which leaves neither
#  src/wpc/spike1.c's games nor the Spike 1 CPU registered. The games need a
#  64-bit build: a title's asset image is over 1 GB, and it is a ROM.
#
#  Like the Pinball 2000 subsystem (cmake/p2k.cmake, which explains why in
#  full) it is its own OBJECT library: it needs C++17, RTTI and its include
#  directories searched first, which the rest of PinMAME must not have. It
#  uses the same MAME compatibility layer (src/p2k/shim); when the Pinball 2000
#  subsystem is built too, the layer's two sources come from its library
#  instead of being compiled a second time.
#
#  Usage in a top-level CMakeLists: after pinmame_enable_p2k(), do
#      include(${CMAKE_SOURCE_DIR}/cmake/spike1.cmake)
#      pinmame_enable_spike1(<target>)
# ---------------------------------------------------------------------------

option(PINMAME_SPIKE1 "Build the Stern Spike 1 subsystem (src/spike1)" ON)

set(_PINMAME_SPIKE1_LIST_DIR ${CMAKE_CURRENT_LIST_DIR})

set(_PINMAME_SPIKE1_SOURCES
   src/spike1/spike1_cpu.cpp
   src/spike1/spike1_devices.cpp
   src/spike1/spike1_linux.cpp
   src/spike1/spike1_memory.cpp
   src/spike1/spike1_vfs.cpp
   src/spike1/spike1_pinmame.cpp
   src/spike1/mame/cpu/arm7/arm7.cpp
   src/spike1/mame/cpu/arm7/arm7ops.cpp
   src/spike1/mame/cpu/arm7/arm7thmb.cpp
   src/spike1/mame/cpu/arm7/arm7dasm.cpp
)

# The MAME compatibility layer's own sources, when no Pinball 2000 library carries them
set(_PINMAME_SPIKE1_SHIM_SOURCES
   src/p2k/shim/emu.cpp
   src/p2k/mame/emu/attotime.cpp
)

set(_PINMAME_SPIKE1_INCLUDES
   src/spike1
   src/spike1/mame/cpu/arm7
   src/p2k/shim
   src/p2k/mame
   src/p2k/mame/emu
   src/p2k/mame/osd
)

function(_pinmame_spike1_add_library reference_target)
   set(_srcs "")
   foreach(_s IN LISTS _PINMAME_SPIKE1_SOURCES)
      list(APPEND _srcs ${_PINMAME_SPIKE1_LIST_DIR}/../${_s})
   endforeach()
   if(NOT TARGET p2k)
      foreach(_s IN LISTS _PINMAME_SPIKE1_SHIM_SOURCES)
         list(APPEND _srcs ${_PINMAME_SPIKE1_LIST_DIR}/../${_s})
      endforeach()
   endif()
   add_library(spike1 OBJECT ${_srcs})

   set(_incs "")
   foreach(_i IN LISTS _PINMAME_SPIKE1_INCLUDES)
      list(APPEND _incs ${_PINMAME_SPIKE1_LIST_DIR}/../${_i})
   endforeach()
   target_include_directories(spike1 BEFORE PRIVATE ${_incs})

   set_target_properties(spike1 PROPERTIES
      CXX_STANDARD 17
      CXX_STANDARD_REQUIRED ON
      POSITION_INDEPENDENT_CODE ON
   )

   # The consumer's options, as cmake/p2k.cmake copies them, then RTTI and quiet imported sources
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
      target_compile_options(spike1 PRIVATE ${_consumer_options} /GR /EHsc /w)
      get_target_property(_rt ${reference_target} MSVC_RUNTIME_LIBRARY)
      if(_rt)
         set_target_properties(spike1 PROPERTIES MSVC_RUNTIME_LIBRARY "${_rt}")
      endif()
   else()
      target_compile_options(spike1 PRIVATE ${_consumer_options} -frtti -fno-strict-aliasing -w)
   endif()
endfunction()

# Attach the Spike 1 subsystem to a pinmame target. No-op unless PINMAME_SPIKE1 is ON.
function(pinmame_enable_spike1 target)
   if(NOT PINMAME_SPIKE1)
      return()
   endif()
   if(NOT TARGET spike1)
      _pinmame_spike1_add_library(${target})
   endif()
   # src/wpc/spike1.c is PinMAME's half - machine driver, CPU interface, game data, I/O, displays,
   # sound, NVRAM - built with the consumer's own settings. It is wrapped in #if HAS_SPIKE1, the
   # switch src/wpc/driver.c and src/cpuintrf.c test as well
   target_sources(${target} PRIVATE
      ${_PINMAME_SPIKE1_LIST_DIR}/../src/wpc/spike1.c
      $<TARGET_OBJECTS:spike1>
   )
   target_compile_definitions(${target} PRIVATE HAS_SPIKE1=1)
endfunction()
