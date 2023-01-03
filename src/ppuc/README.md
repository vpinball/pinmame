# PPUC - Pinball Power-Up Controllers

The *Pinball Power-Up Controllers* are a set of hard- and software designed to repair and enhance the capabilities of
classic pinball machines of the 80s and 90s and to drive the hardware of home brew pinball machines.
The project is in ongoing development. Visit the [PPUC Page](http://ppuc.org) for further information.
This directory contains the PinMAME related parts, mainly the building blocks to emulate a pinball CPU that drives
PPUC I/O boards.

## Motivation

We want to enable people to be creative and to modernize old pinball machines using today's technology. Our goal is to
establish an open and affordable platform for that. Ideally people will publish their game-specific PPUs so others could
leverage and potentially improve them. We want to see a growing library of so-called *Pinball Power-Ups* (PPUs) and a
vital homebrew pinball community.

## Licences

The code in this directory and all sub-directories is licenced under GPLv3, except if a different license is mentioned
in a file's header or in a sub-directory. Be aware of the fact that your own enhancements of ppuc need to be licenced
under a compatible licence.

Due to complicated dependency management on different platforms, these libraries are included as source code copy:
* [serialib](https://github.com/imabot2/serialib)
* [cargs](https://github.com/likle/cargs)
* [libdmdcommon](https://github.com/zesinger/libdmdcommon)

PPUC uses
* [libpinmame](https://github.com/vpinball/pinmame)
* [libusb](https://libusb.info/)
* [yaml-cpp](https://github.com/jbeder/yaml-cpp)
* [OpenAL Soft](https://openal-soft.org/)

## Documentation

These components are still in an early development stage and the documentation will grow.

### Command Line Options

* -c path
    * path to config file
    * required
* -r rom name
    * rom to use, overwrites *rom* setting in config file
    * optional
* -s serial device 
    * serial device path to use, overwrites *serialPort* setting in config file
    * optional
* -d
    * enable debug mode, overwrites *debug* setting in config file
    * optional
* -i
    * render display in console
    * optional
* -h
    * help


### Compiling

#### macOS

Install required dependencies via homebrew:
```shell
brew install openal-soft libusb yaml-cpp
```

Compile:
```shell
cp cmake/ppuc/CMakeLists_osx-x64.txt CMakeLists.txt
cmake -DCMAKE_BUILD_TYPE=Release -B build/Release
cmake --build build/Release
```

Run:
```shell
build/Release/ppuc -c src/ppuc/examples/t2.yml
```

#### Linux (debian based)

Install required dependencies via apt:
```shell
apt install cmake zlib1g-dev libopenal-dev libyaml-cpp-dev libusb-1.0-0-dev
```

Compile:
```shell
cp cmake/ppuc/CMakeLists_linux-x64.txt CMakeLists.txt
cmake -DCMAKE_BUILD_TYPE=Release -B build/Release
cmake --build build/Release
```

Run:
```shell
sudo build/Release/ppuc -c src/ppuc/examples/t2.yml
```

### Windows

tbd
