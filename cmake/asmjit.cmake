# ---------------------------------------------------------------------------
#  Optional asmjit-based ARM7 JIT backend
#
#  ARM7 JIT -> asmjit migration/generalization
#
#  ON by default; disable with -DPINMAME_JIT_ASMJIT=OFF.
#
#  x86/x64 TARGETS ONLY: the translator emits x86/x64 host code through
#  asmjit's x86 backend (jit_asmjit.cpp uses x86::Assembler throughout, and
#  emit_mem_call #errors on any other host ABI). On ARM64 hosts the vendored
#  asmjit is built with ASMJIT_NO_FOREIGN, which drops the x86 emitter
#  entirely, so the translation unit cannot even compile there. Until an
#  AArch64 backend exists, this file therefore resolves the user-facing
#  option into PINMAME_JIT_ASMJIT_EFFECTIVE, which is ON only when the
#  option is ON AND the target architecture is x86/x64; every other target
#  (win/arm64, macos/arm64, linux/aarch64, android, ios, tvos, ...) builds
#  exactly as if the option were OFF: asmjit is NOT built, no source is
#  added, nothing links against it, and the emulator uses the interpreter.
#
#  Usage in a top-level CMakeLists: after the target is defined, do
#      include(${CMAKE_SOURCE_DIR}/cmake/asmjit.cmake)
#      pinmame_enable_asmjit(<target>)
# ---------------------------------------------------------------------------

option(PINMAME_JIT_ASMJIT "Build the asmjit-based ARM7 JIT backend (x86/x64 targets only)" ON)

# Resolve the effective switch: option ON + x86/x64 target.
set(PINMAME_JIT_ASMJIT_EFFECTIVE FALSE)
if(PINMAME_JIT_ASMJIT)
   # Determine the TARGET architecture. Precedence:
   #   1. ARCH        - the libpinmame CMakeLists' cache variable; the one place
   #                    that builds for non-x86 targets (x86/x64/arm64/aarch64/
   #                    arm64-v8a), so it must win over host-derived guesses.
   #   2. CMAKE_GENERATOR_PLATFORM - Visual Studio -A value (Win32/x64/ARM64);
   #                    covers the win-only pinmame/pinmame32/vpinmame lists,
   #                    which don't define ARCH.
   #   3. CMAKE_SYSTEM_PROCESSOR   - everything else (single-arch generators,
   #                    cross toolchains set it in their toolchain file).
   if(DEFINED ARCH)
      set(_pinmame_asmjit_arch "${ARCH}")
   elseif(CMAKE_GENERATOR_PLATFORM)
      set(_pinmame_asmjit_arch "${CMAKE_GENERATOR_PLATFORM}")
   else()
      set(_pinmame_asmjit_arch "${CMAKE_SYSTEM_PROCESSOR}")
   endif()
   string(TOLOWER "${_pinmame_asmjit_arch}" _pinmame_asmjit_arch)
   # Allowlist of x86-family spellings across the sources above (ARCH, VS -A,
   # uname -m / %PROCESSOR_ARCHITECTURE%). Anything else - notably arm64,
   # aarch64, arm64-v8a, or an empty value from a minimal toolchain - keeps
   # the JIT off; unknown-but-actually-x86 spellings fail SAFE (interpreter).
   if(_pinmame_asmjit_arch MATCHES "^(x86|x64|win32|amd64|x86_64|i[3-6]86)$")
      set(PINMAME_JIT_ASMJIT_EFFECTIVE TRUE)
   else()
      message(STATUS "PINMAME_JIT_ASMJIT: target architecture '${_pinmame_asmjit_arch}' has no JIT backend (x86/x64 only) - building without the asmjit JIT")
   endif()
endif()

# Repo layout anchor, captured at INCLUDE time. Inside a function,
# CMAKE_CURRENT_LIST_DIR resolves to the CALLER's list file directory (the
# top-level CMakeLists), which would point the paths below outside the repo.
set(_PINMAME_ASMJIT_LIST_DIR ${CMAKE_CURRENT_LIST_DIR})

# Add the vendored asmjit sub-project exactly once.
if(PINMAME_JIT_ASMJIT_EFFECTIVE AND NOT TARGET asmjit::asmjit)
   set(ASMJIT_STATIC     ON  CACHE BOOL "" FORCE)  # static lib, link straight in
   set(ASMJIT_EMBED      OFF CACHE BOOL "" FORCE)
   set(ASMJIT_TEST       OFF CACHE BOOL "" FORCE)  # no asmjit tests/benchmarks
   set(ASMJIT_NO_FOREIGN ON  CACHE BOOL "" FORCE)  # build only the host backend
   add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../ext/asmjit ${CMAKE_BINARY_DIR}/ext_asmjit)
endif()

# Attach the asmjit JIT backend + library to a pinmame target.
# No-op unless PINMAME_JIT_ASMJIT resolved to an x86/x64 target (see above).
function(pinmame_enable_asmjit target)
   if(PINMAME_JIT_ASMJIT_EFFECTIVE)
      target_sources(${target} PRIVATE ${_PINMAME_ASMJIT_LIST_DIR}/../src/windows/jit_asmjit.cpp)
      # Link by appending to the LINK_LIBRARIES property rather than calling
      # target_link_libraries: CMake forbids mixing the plain and keyword
      # signatures on one target, and our consumers use BOTH styles (the
      # win-only pinmame/pinmame32/vpinmame lists link plain, libpinmame links
      # with keywords). The property append is signature-neutral and still
      # propagates asmjit's usage requirements (include dirs etc.).
      set_property(TARGET ${target} APPEND PROPERTY LINK_LIBRARIES asmjit::asmjit)
      target_compile_definitions(${target} PRIVATE PINMAME_JIT_ASMJIT)
      # Match the consumer's CRT: the win pinmame/pinmame32/vpinmame targets build
      # with the STATIC runtime (MSVC_RUNTIME_LIBRARY "MultiThreaded..."), while
      # asmjit would default to the DLL runtime -- a guaranteed LNK2038 mismatch
      # at link time. Copy the consumer's setting onto the asmjit target (no-op
      # when the consumer uses the default, e.g. libpinmame).
      if(MSVC)
         get_target_property(_pinmame_asmjit_crt ${target} MSVC_RUNTIME_LIBRARY)
         if(_pinmame_asmjit_crt)
            set_target_properties(asmjit PROPERTIES MSVC_RUNTIME_LIBRARY "${_pinmame_asmjit_crt}")
         endif()
      endif()
   endif()
endfunction()
