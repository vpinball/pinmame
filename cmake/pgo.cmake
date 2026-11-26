# ---------------------------------------------------------------------------
#  Profile-guided optimisation, off by default
#
#  PINMAME_PGO=GEN builds the target instrumented: running it writes a profile
#  to PINMAME_PGO_DIR. PINMAME_PGO=USE builds it with that profile.
#    GCC, MinGW: -fprofile-generate / -fprofile-use; with -fprofile-prefix-path
#                (GCC 12 and later) the profile is keyed by object paths
#                relative to the build directory, so another build directory
#                of the same tree can use it; a stale profile warns
#    Clang, clang-cl: -fprofile-generate /
#                -fprofile-use=PINMAME_PGO_DIR/default.profdata
#                (merge the .profraw files with llvm-profdata first)
#    MSVC:       /GENPROFILE / /USEPROFILE with PINMAME_PGO_DIR/<target>.pgd,
#                Release only; the instrumented binary needs pgort140.dll and
#                writes <target>!N.pgc when it exits or unloads
#  A static library's link options go to the programs that link it.
#  The profile is local and never committed.
#
#  Usage, after the target is defined and before cmake/asmjit.cmake and
#  cmake/p2k.cmake (which copy the target's options):
#      include(${CMAKE_SOURCE_DIR}/cmake/pgo.cmake)
#      pinmame_enable_pgo(<target>)
# ---------------------------------------------------------------------------

include(CheckCCompilerFlag)

set(PINMAME_PGO "" CACHE STRING "Profile-guided optimisation: empty, GEN or USE")
set(PINMAME_PGO_DIR "${CMAKE_BINARY_DIR}/pgo-profile" CACHE PATH "Directory of the PGO profile")
if(NOT PINMAME_PGO MATCHES "^(|GEN|USE)$")
   message(FATAL_ERROR "PINMAME_PGO must be empty, GEN or USE")
endif()

function(pinmame_enable_pgo target)
   if(PINMAME_PGO STREQUAL "")
      return()
   endif()
   get_target_property(type ${target} TYPE)
   if(type STREQUAL "STATIC_LIBRARY")
      set(link INTERFACE)
   else()
      set(link PRIVATE)
   endif()
   if(MSVC AND NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
      file(MAKE_DIRECTORY ${PINMAME_PGO_DIR})
      if(PINMAME_PGO STREQUAL "GEN")
         target_link_options(${target} ${link} $<$<CONFIG:RELEASE>:/GENPROFILE:PGD=${PINMAME_PGO_DIR}/${target}.pgd>)
      else()
         target_link_options(${target} ${link} $<$<CONFIG:RELEASE>:/USEPROFILE:PGD=${PINMAME_PGO_DIR}/${target}.pgd>)
      endif()
   elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
      check_c_compiler_flag(-fprofile-prefix-path=${CMAKE_BINARY_DIR} PINMAME_PGO_HAVE_PREFIX_PATH)
      check_c_compiler_flag(-fprofile-partial-training PINMAME_PGO_HAVE_PARTIAL_TRAINING)
      set(opts)
      if(PINMAME_PGO_HAVE_PREFIX_PATH)
         list(APPEND opts -fprofile-prefix-path=${CMAKE_BINARY_DIR})
      else()
         message(WARNING "PINMAME_PGO: no -fprofile-prefix-path, so only the build directory that wrote the profile can use it")
      endif()
      if(PINMAME_PGO STREQUAL "GEN")
         target_compile_options(${target} PRIVATE -fprofile-generate=${PINMAME_PGO_DIR} -fprofile-update=atomic ${opts})
         target_link_options(${target} ${link} -fprofile-generate=${PINMAME_PGO_DIR})
      else()
         if(PINMAME_PGO_HAVE_PARTIAL_TRAINING)
            list(APPEND opts -fprofile-partial-training)
         endif()
         target_compile_options(${target} PRIVATE -fprofile-use=${PINMAME_PGO_DIR} ${opts} -Wno-missing-profile -Wno-error=coverage-mismatch)
      endif()
   else()
      if(PINMAME_PGO STREQUAL "GEN")
         target_compile_options(${target} PRIVATE -fprofile-generate=${PINMAME_PGO_DIR})
         if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
            # clang-cl: the objects name clang_rt.profile.lib, the linker needs its directory
            execute_process(COMMAND ${CMAKE_C_COMPILER} -print-resource-dir OUTPUT_VARIABLE res OUTPUT_STRIP_TRAILING_WHITESPACE)
            target_link_options(${target} ${link} /LIBPATH:${res}/lib/windows)
         else()
            target_link_options(${target} ${link} -fprofile-generate=${PINMAME_PGO_DIR})
         endif()
      else()
         target_compile_options(${target} PRIVATE -fprofile-use=${PINMAME_PGO_DIR}/default.profdata -Wno-profile-instr-unprofiled -Wno-profile-instr-out-of-date)
      endif()
   endif()
endfunction()
