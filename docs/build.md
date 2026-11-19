## Building the command line emulator on Linux and macOS (sdl3pinmame)

`sdl3pinmame` is the command line emulator (xpinmame) using SDL3 for video, input, joysticks and sound
instead of X11. SDL3 is downloaded and statically linked during the build, so nothing has to be
installed for it. For more detailed instructions, have a look at the [ci workflow](.github/workflows/sdl3pinmame.yml).

```shell
# Linux: SDL3 needs the development packages of the backends it should support, e.g. on Ubuntu:
sudo apt install libasound2-dev libpulse-dev libpipewire-0.3-dev libx11-dev libxext-dev libxrandr-dev \
  libxcursor-dev libxfixes-dev libxi-dev libxkbcommon-dev libwayland-dev libdecor-0-dev libgl1-mesa-dev libegl1-mesa-dev
cp cmake/sdl3pinmame/CMakeLists.txt CMakeLists.txt
cmake -DPLATFORM=linux -DARCH=x64 -DCMAKE_BUILD_TYPE=Release -B build
# macOS: -DPLATFORM=macos and -DARCH=arm64 or x64
cmake --build build -j
# Run The Addams Family
./build/sdl3pinmame -rompath ~/.pinmame/roms -nvram_directory ~/.pinmame/nvram taf_l5
```

Useful options: `-fullscreen` (or Alt+Enter while running), `-windowscale <n>`, `-linear`, `-vsync`,
`-joytype 7` for SDL joystick support, `-skip_disclaimer -skip_gameinfo`, and `-help` for all the rest.
Add `-DSDL3PINMAME_SYSTEM_SDL=ON` to link against an SDL3 installed on the system instead.

Add `-DSDL3PINMAME_MAME_DEBUG=ON` to compile in the classic MAME debugger. Start with `-debug` to
begin in the debugger, which gets its own window next to the game one, or press the tilde key (left of 1)
while the game runs to break into it. F1 lists the debugger keys.

### Profile-guided builds (optional)

The CMake builds of sdl3pinmame, PinMAME (Windows x64) and libpinmame can be built profile-guided: an
instrumented build runs the games you care about and records which code is hot, and a second build uses that
profile to lay out and inline the code. `PINMAME_PGO` is empty by default, which leaves the build unchanged.
GCC, Clang and MSVC are supported; the profile goes to `PINMAME_PGO_DIR` (default `build/pgo-profile`) and is
never committed. The output of the emulation is the same as without a profile, only faster.

```shell
cp cmake/sdl3pinmame/CMakeLists.txt CMakeLists.txt
P=$PWD/build/pgo-profile
cmake -B build/pgo-gen -DPLATFORM=linux -DARCH=x64 -DCMAKE_BUILD_TYPE=Release -DPINMAME_PGO=GEN -DPINMAME_PGO_DIR=$P
cmake --build build/pgo-gen -j
# training: run each game a few minutes, e.g. attract mode, unthrottled
./build/pgo-gen/sdl3pinmame <game> -rompath ~/.pinmame/roms -nothrottle -frames_to_run 7200
# Clang only: merge the raw profiles (llvm-profdata may carry a version suffix)
llvm-profdata merge -o $P/default.profdata $P/*.profraw
cmake -B build/pgo-use -DPLATFORM=linux -DARCH=x64 -DCMAKE_BUILD_TYPE=Release -DPINMAME_PGO=USE -DPINMAME_PGO_DIR=$P
cmake --build build/pgo-use -j --clean-first
```

With MSVC the same two configurations apply to a Release build (`-DPINMAME_PGO=GEN`, then `USE`); the
instrumented PinMAME.exe needs `pgort140.dll` from the Visual Studio installation next to it or on the `PATH`, and
writes its `.pgc` files when it exits.

## Windows builds

Use the `create_vc2026_from_vc2012.bat` in the `vcproj` folder (or its older cousins) to convert the existing
vcproj/sln(x) files to the recommended Visual Studio 2026.
It should work out-of-the-box (no external dependencies), unless you compile for 32-bit x86, which needs NASM.

Or simply use the respective cmake files in the `cmake` folder.

Note that older Visual Studio versions are also supported, or compiling via MinGW (for slightly outdated info on the latter, see `setup_mingw.txt`).
