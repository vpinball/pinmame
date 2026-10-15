# ---------------------------------------------------------------------------
#  spike1boot - the Spike 1 subsystem's standalone bring-up harness
#
#  Builds src/spike1 alone, without PinMAME around it, the way "make p2kboot"
#  does for the Pinball 2000 subsystem. Use it straight from this directory:
#
#      cmake -S cmake/spike1boot -B build/spike1boot -DCMAKE_BUILD_TYPE=Release
#      cmake --build build/spike1boot
#
#  The subsystem shares src/p2k's MAME compatibility layer (shim), so it needs
#  the same settings as the p2k object library: C++17 and RTTI, and its own
#  include directories searched first. See cmake/p2k.cmake for why.
# ---------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.20)
project(spike1boot CXX)

set(_root ${CMAKE_CURRENT_LIST_DIR}/../..)

add_executable(spike1boot
   ${_root}/src/spike1/spike1_boot.cpp
   ${_root}/src/spike1/spike1_cpu.cpp
   ${_root}/src/spike1/spike1_devices.cpp
   ${_root}/src/spike1/spike1_linux.cpp
   ${_root}/src/spike1/spike1_memory.cpp
   ${_root}/src/spike1/spike1_vfs.cpp
   ${_root}/src/spike1/mame/cpu/arm7/arm7.cpp
   ${_root}/src/spike1/mame/cpu/arm7/arm7ops.cpp
   ${_root}/src/spike1/mame/cpu/arm7/arm7thmb.cpp
   ${_root}/src/spike1/mame/cpu/arm7/arm7dasm.cpp
   ${_root}/src/p2k/shim/emu.cpp
   ${_root}/src/p2k/mame/emu/attotime.cpp
)

target_include_directories(spike1boot BEFORE PRIVATE
   ${_root}/src/spike1
   ${_root}/src/spike1/mame/cpu/arm7
   ${_root}/src/p2k/shim
   ${_root}/src/p2k/mame
   ${_root}/src/p2k/mame/emu
   ${_root}/src/p2k/mame/osd
)

set_target_properties(spike1boot PROPERTIES
   CXX_STANDARD 17
   CXX_STANDARD_REQUIRED ON
)

if(MSVC)
   target_compile_options(spike1boot PRIVATE /GR /EHsc /w)
else()
   target_compile_options(spike1boot PRIVATE -frtti -fno-strict-aliasing -w)
   if(MINGW)
      target_link_options(spike1boot PRIVATE -static)
   endif()
endif()
