#------------------------------------------------------------------------------
# altsound library
# 07/04/2023 Dave Roscoe
#
# CMake project to build altsound code as a standalone static library.
#------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)

# set initial source root to CMake source root
set(ROOT_DIR ${CMAKE_SOURCE_DIR})

# Load helper functions
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/helper_fns.cmake)

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	project(altsound_test)
	findRootDirectory(.git) # use .git as root search target
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

add_compile_definitions(
   PINMAME
   VPINMAME_ALTSOUND
)

set(STANDALONE_SOURCES
	altsound_data.cpp
	altsound_data.hpp
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
    altsound_file_parser.cpp
    altsound_file_parser.hpp
    altsound_csv_parser.cpp
    altsound_csv_parser.hpp
	gsound_processor.cpp
	gsound_processor.hpp
	${ROOT_DIR}/src/vc/dirent.c # for AltsoundFileParser
	${ROOT_DIR}/src/vc/dirent.h # for AltsoundFileParser
	${ROOT_DIR}/src/vc/unistd.h # for AltsoundCsvParser
)

set(VPINMAME_SOURCES
   snd_alt.cpp
   snd_alt.h
)

set(FULL_SOURCES
  ${STANDALONE_SOURCES}
)

# If not building standalone, add the VPinMAME sources
if (NOT ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	set(FULL_SOURCES ${FULL_SOURCES}
		${VPINMAME_SOURCES}
)
endif()

add_library(altsound STATIC
	${FULL_SOURCES}
)

set(OPT_COMMON /Ob2 /O2 /Oi /arch:SSE2 /fp:fast /fp:except- /Ot /GF /GS- /Gy /GR- /Oy /GT /GL /EHsc /INCREMENTAL:NO)

target_compile_options(altsound PRIVATE
   $<$<CONFIG:RELEASE>:$<$<COMPILE_LANGUAGE:CXX>:${OPT_COMMON}>>
   $<$<CONFIG:DEBUG>:$<$<COMPILE_LANGUAGE:CXX>:/EHsc /INCREMENTAL:NO>>
   $<$<CONFIG:RELEASE>:$<$<COMPILE_LANGUAGE:C>:${OPT_COMMON}>>
   $<$<CONFIG:DEBUG>:$<$<COMPILE_LANGUAGE:C>:/EHsc /INCREMENTAL:NO>>
)

target_include_directories(altsound PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${ROOT_DIR}
   ${ROOT_DIR}/src
   ${ROOT_DIR}/src/wpc
   ${ROOT_DIR}/src/windows
   ${ROOT_DIR}/src/vc #dirent.h
   ${ROOT_DIR}/ext/bass
)

target_link_directories(altsound PRIVATE
   ${ROOT_DIR}/ext/bass
)

target_link_libraries(altsound PRIVATE
   bass.lib
)

target_link_options(altsound PRIVATE
   /SAFESEH:NO
   /INCREMENTAL:NO>
)

set_target_properties(altsound PROPERTIES
   LINKER_LANGUAGE CXX
   MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:DEBUG>:Debug>"
)

# build executable if building standalone
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	add_compile_definitions(ALTSOUND_STANDALONE)

	add_executable(altsound_test
		main.cpp
	)

	target_include_directories(altsound_test PRIVATE
		${ROOT_DIR}/ext/bass
        ${ROOT_DIR}/src/vc #dirent.h
	)

	target_link_libraries(altsound_test PRIVATE
		altsound
	)

	target_link_directories(altsound_test PRIVATE
		${ROOT_DIR}/ext/bass
	)

	set_target_properties(altsound_test PROPERTIES
		LINKER_LANGUAGE CXX
			MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:DEBUG>:Debug>"
	)
endif()

if (MSVC)
    set(CMAKE_BUILD_TOOL msbuild)
else()
    set(CMAKE_BUILD_TOOL make)
endif()

