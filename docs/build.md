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

## Windows builds

Use the `create_vc2026_from_vc2012.bat` in the `vcproj` folder (or its older cousins) to convert the existing
vcproj/sln(x) files to the recommended Visual Studio 2026.
It should work out-of-the-box (no external dependencies), unless you compile for 32-bit x86, which needs NASM.

Or simply use the respective cmake files in the `cmake` folder.

Note that older Visual Studio versions are also supported, or compiling via MinGW (for slightly outdated info on the latter, see `setup_mingw.txt`).
