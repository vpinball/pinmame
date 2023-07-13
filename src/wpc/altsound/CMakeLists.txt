#------------------------------------------------------------------------------
# altsound library
# 07/04/2023 Dave Roscoe
#
# CMake project to build altsound code as a standalone static library.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)

#file(READ src/version.c version)
#string(REGEX MATCH "[0-9\\.]+" PROJECT_VERSION ${version})
#
#project(vpinmame VERSION ${PROJECT_VERSION})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

add_compile_definitions(
   MAMEVER=7300
   PINMAME
   VPINMAME_ALTSOUND

   LSB_FIRST
   DIRECTINPUT_VERSION=0x0700
   DIRECTDRAW_VERSION=0x0300
   #WIN32
   _CRT_SECURE_NO_WARNINGS
   #_USRDLL
)

add_library(altsound STATIC
   altsound_data.cpp
   altsound_data.hpp
   altsound_file_parser.cpp
   altsound_file_parser.hpp
   altsound_csv_parser.cpp
   altsound_csv_parser.hpp
   gsound_csv_parser.cpp
   gsound_csv_parser.hpp
   altsound_ini_processor.hpp
   altsound_ini_processor.cpp
   altsound_logger.cpp
   altsound_logger.hpp
   altsound_processor_base.cpp
   altsound_processor_base.hpp
   altsound_processor.cpp
   altsound_processor.hpp
   gsound_processor.cpp
   gsound_processor.hpp
   snd_alt.cpp
   snd_alt.h
)

set(OPT_COMMON /Ob2 /O2 /Oi /arch:SSE2 /fp:fast /fp:except- /Ot /GF /GS- /Gy /GR- /Oy /GT /GL /EHsc)

target_compile_options(altsound PUBLIC
   $<$<CONFIG:RELEASE>:$<$<COMPILE_LANGUAGE:CXX>:${OPT_COMMON}>>
   $<$<CONFIG:DEBUG>:$<$<COMPILE_LANGUAGE:CXX>:/EHsc>>
   $<$<CONFIG:RELEASE>:$<$<COMPILE_LANGUAGE:C>:${OPT_COMMON}>>
   $<$<CONFIG:DEBUG>:$<$<COMPILE_LANGUAGE:C>:/EHsc>>
)

target_include_directories(altsound PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${CMAKE_SOURCE_DIR}/src
   ${CMAKE_SOURCE_DIR}/src/wpc
   ${CMAKE_SOURCE_DIR}/src/windows
   ${CMAKE_SOURCE_DIR}/src/vc #dirent.h
   ${CMAKE_SOURCE_DIR}/ext/bass
   ${CMAKE_CURRENT_BINARY_DIR}/$(IntDir)
)

target_link_directories(altsound PUBLIC
   ext/bass
)

target_link_libraries(altsound PUBLIC
   version.lib
   bass.lib
)

target_link_options(altsound PUBLIC
   /SAFESEH:NO
   $<$<CONFIG:RELEASE>:/INCREMENTAL:NO>
   $<$<CONFIG:RELEASE>:/OPT:REF>
   $<$<CONFIG:RELEASE>:/OPT:ICF>
   $<$<CONFIG:RELEASE>:/LTCG>
   $<$<CONFIG:RELEASE>:/DYNAMICBASE:NO>
)

set_target_properties(altsound PROPERTIES
   LINKER_LANGUAGE CXX
   VS_DPI_AWARE "On"
   MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:DEBUG>:Debug>"
)
