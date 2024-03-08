# PPUC - Pinball Power-Up Controllers

The *Pinball Power-Up Controllers* are a set of hard- and software designed to repair and enhance the capabilities of
classic pinball machines of the 80s and 90s and to drive the hardware of home brew pinball machines.
The project is in ongoing development. Visit the [PPUC Page](https://github.com/PPUC) for further information,
hardware and firmwares.

The project started as part of pinmame. But in oposite to comparable systems, PPUC now leverages libpinamme and doesn't
consist of a lot of compile switches across the pinmame source code, except a few. For example in `wpc.c`.

The code that has been in in this folder is now moved to dedicated projects, mainly libriaries that could be re-used by
others (for example VPX Standlaone):

* [libppuc](https://github.com/PPUC/libppuc)
* [libdmdutil](https://github.com/vpinball/libdmdutil)
* [libzedmd](https://github.com/PPUC/libuedmd)

There's now a new main project to create the [ppuc executable](https://github.com/PPUC/ppuc).
The github actions to build everything have been moved from pinmame to that repository.
