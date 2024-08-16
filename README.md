# PinMAME

*Pinball Multiple Arcade Machine Emulator*

## What is it?

PinMAME emulates the hardware found in almost every solid state pinball machine created from
the earliest days of CPU-controlled machines (mid 1970's) through 2014 (Stern SAM),
with around 770 emulated unique Pinball machines and many more clones/revisions (overall more than 2700 sets).
It is available in various forms:

Standalone Emulator (PinMAME (command line), PinMAME32 (UI))

COM library (VPinMAME) to steer simulators like [Visual Pinball 8/9/X](https://github.com/vpinball/vpinball)

library (libPinMAME) to steer simulators like [VPE](https://github.com/freezy/VisualPinball.Engine), VPX Standalone,
or hardware replacement solutions like [PPUC](https://github.com/PPUC)

Supported platforms: Windows (x86), Linux (x86/Arm, incl. RaspberryPi and RK3588), macOS, iOS/tvOS, Android

Currently, the following pinball hardware is emulated:

Williams/Bally WPC, Williams/Bally System 11, Williams System 9, Williams System 7,
Williams System 6, Williams System 4, Williams System 3,
Data East AlphaNumeric System, Data East 128x16 DMD, Data East 128x32 DMD,
Data East/Sega 192x64 DMD, Sega/Stern Whitestar System, Stern S.A.M., Stern MPU-100, Stern MPU-200,
Bally MPU-17 & MPU-35, Bally Video/Pinball, Bally 6803,
Gottlieb System 1, 80, 80a, 80b, System 3, Hankin, Gameplan MPU-1 & MPU-2, Atari,
Zaccaria, Taito of Brazil, Midway, Capcom, Alvin G. and Co., Technoplay, Mr.Game, Spinball,
Nuova Bell, Inder, Juegos Populares, LTD, Peyper, Sonic, Allied Leisure, Fascination, Int.,
Sleic, Playmatic, NSM, Grand Products, Jac van Ham, Videodens, Astro, Micropin,
Christian Tabart, Jeutel, Valley Manufacturing, MAC / CICPlay, Stargame, Barni,
Seeben/Sirmo, Splin Bingo, Playbar, Cirsa, Nondum / CIFA, Maibesa, ManilaMatic, Joctronic, Mirco,
Sport Matic, Regama, Illinois Pinball.

*Note: Emulation is not 100% working and correct for all hardware, but very close for the vast majority.*

PinMAME is built as an add-on to the historic [MAME 0.76 Source Code](https://github.com/mamedev/historic-mame). Some of the original code was altered, and fixes from later MAME versions were applied. In addition, it can be compiled on unix platforms (including macOS) and is (hopefully) 100% compatible with 64bit CPU architectures/compiles by now.

All the historic MAME readme files with all disclaimers, credits and instructions are included for
info on using MAME related functions.

All standard MAME "functions" do work the same way in PinMAME (profiler, debugger, cheats,
record/playback, command line switches etc.).

In addition, there is special compile time support for the [P-ROC](http://www.pinballcontrollers.com),
to drive (at least) real WPC machines with PinMAME/P-ROC, [PPUC](https://github.com/PPUC) and LISY
(Linux for Gottlieb System1 & System80, Bally, Atari, Williams and 'HOme' Pinballs, to drive real pinball machines via
PinMAME and special hardware, see http://www.lisy80.com & http://www.lisy.dev, also README.lisy)
platforms (see PROC and LISY_X defines in makefile.unix).

## What does it do?

Before you start to add the ROMs from your favorite pinball machine please note:

- The PinMAME pinball emulator/simulator itself is not 100% playable. It only emulates the electronic circuit boards and the display(s) found in the pinball machine backbox. There is no playfield and no balls that you will see 'emulated' and displayed!
- This part can optionally be added by using separate independent program packages, like Visual Pinball or Unit3D Pinball, which take care of simulating physics and the 3D rendering of the playfield and all its parts.
- Note however, that you can still activate switches with your keyboard, see the display animations,
and listen to/record the pinball game sounds with the pure PinMAME package itself.

## Games supported (incomplete)

- *Williams/Bally WPC* - All games from Dr. Dude (1990) to Cactus Canyon (1998)
- *Williams/Bally System 11* - All games from High Speed (1986) to Dr.Dude (1990)
- *Williams System 9* - All games from Space Shuttle (1984) to Comet (1985)
- *Williams System 7* - All games from Black Knight (1980) to Star Light (1984)
- *Williams System 6* - All games from Blackout (1979) to Alien Poker (1980)
- *Williams System 4* - All games from Phoenix (1978) to Stellar Wars (1979)
- *Williams System 3* - All games from HotTip (1977) to Disco Fever (1978)
- *Data East AlphaNumeric System* - All games from Laser War (1987) to The Simpsons (1990)
- *Data East 128x16 DMD* - All games from Checkpoint (1991) to Hook (1992)
- *Data East 128x32 DMD* - All games from Lethal Weapon 3 (1992) to Guns 'n Roses (1994)
- *Sega 192x64 DMD* - All games from Maverick (1994) to Batman Forever (1995)
- *Sega 256x64 DMD* - Flipper Football (1996)
- *Sega/Stern Whitestar* - All games from Apollo 13 (1995) to Nascar (2005)
- *Stern S.A.M.* - All games from World Poker Tour (2006) to Walking Dead (2014)
- *Stern MPU-100* - All games from Stingray (1977) to Magic (1979)
- *Stern MPU-200* - All games from Meteor (1979) to Lazer Lord (1984)
- *Bally MPU-17* - All games from Freedom (1977) to Black Jack (1978)
- *Bally MPU-35* - All games from Lost World (1978) to Cybernaut (1985)
- *Bally Video/Pinball* - Baby Pacman (1982) and Granny & The Gators (1984)
- *Gottlieb System 1* - All games from Cleopatra (1977) to Asteroid Annie and the Aliens (1980)
- *Gottlieb System 80* - All games from Spiderman (1980) to Haunted House (1982)
- *Gottlieb System 80a* - All games from Devil's Dare (1982) to Ice Fever (1985)
- *Gottlieb System 80b* - All games from Triple Play (1985) to BoneBusters (1989)
- *Gottlieb System 3* - All games from Lights,Camera,Action (1989) to Barb Wire (1996)
- *Hankin Pinball* - All games from FJ Holden (1978) to Orbit 1 (1981)
- *Game Plan Pinball* - All games from Rio (1978) to Cyclopes (1985)
- *Atari Pinball* - All games from Atarians (1976) to Road Runner (1979)
- *Zaccaria Pinball* - All games from Winter Sports (1978) to New Star's Phoenix (1987)
- *Taito Pinball* - All that's available... games between 1980 and 1985
- *Midway Pinball* - Rotation VIII (1978)
- *Capcom Pinball* - All games from Pinball Magic (1995) to Kingpin (1996)
- *Alvin G. and Co* - All games from Soccer Ball (1991) to Pistol Poker (1993)
- *Tecnoplay* - Scramble, X-Force (both 1987)
- *Mr. Game* - Dakar, Motor Show (1988), World Cup '90 (1990)
- *Spinball* - Mach 2 (1995), Jolly Park (1996)
- *Nuova Bell* - all available Bally clones, also Future Queen (1987), F1 Grand Prix (1987), U-Boat 65 (1988)
- *Inder* - Brave Team (1985), Canasta 86 (1986), Clown (1988), Corsario (1989), Atleta (1991), 250cc (1992), Bushido (1993)
- *Juegos Populares* - Petaco (1984), Faeton (1985), America 1492, Aqualand (both 1987)
- *LTD* - Atlantis, Black Hole, Zephy, Cowboy Eight Ball, Mr. & Mrs. Pec-Men, Al Capone (1980-1983)
- *Peyper* - Odisea Paris-Dakar (1987)
- *Sonic* - Odin DeLuxe (1985), Pole Position (1987), Star Wars (1987)
- *Allied Leisure* - All games from Super Picker (1977) to Star Shooter (1979)
- *Fascination, Int.* - Roy Clark - The Entertainer (1977), Eros One, and Circa 1933 (both 1979)
- *Sleic*
- *Playmatic* - Last Lap (1978), Antar (1979), Evil Fight (1980), Mad Race (1982), Meg-Aaton (1983), KZ-26 (1984) (*)
- *NSM*
- *Grand Products* - 300/Bullseye (1986)
- *Jac van Ham* - Escape (1987), Movie Masters
- *Videodens* - Break (1986)
- *Astro* - Black Sheep Squadron (1978)
- *Micropin* - Pentacup (1979)
- *Christian Tabart* - L'Hexagone (1986)
- *Jeutel* - Le King (1983), Olympic Games (1984)
- *Valley Manufacturing*
- *MAC / CICPlay* - MAC Galaxy (1986), Space Train (1987), Space Panther (1988), New MAC Jungle (1995), NBA MAC (1996), Kidnap (1986), Galaxy Play (1986), Galaxy Play 2 (1987)
- *Stargame* - Space Ship (1986), Mephisto (1986), White Force (1987), Iron Balls (1987), Slalom Code 0.3 (1988)
- *Barni* - Red Baron (1985)
- *Splin Bingo* -  Golden Game
- *Playbar* - Bloody Roller
- *Cirsa*
- *Nondum / CIFA*
- *Maibesa*
- *ManilaMatic*
- *Joctronic*
- *Mirco*
- *Sport Matic*
- *Regama* - Trebol
- **Prototype games and modifications**
  - Dave Nutting's Flicker (Sep 1974)
  - Bally's Bow & Arrow (Jan 1976)
  - Williams Rat Race (Jan 1983)
  - Wild Texas (Firepower II Modification)
  - machinaZOIS
  - and many more...

"Supported" usually means that the game loads and the display(s) start up along with lamps etc. All games
enter attract mode and you can use the Coin Door switches to enter the menus.

### Notes

- Sound may not be supported, or may not work properly for all listed games.
- A FEW games may not work or be supported correctly due to customizations of the original hardware.
- Some games may not be fully supported, simply because we could not find full rom sets, so some chips remain undumped.

## Simulation?

For many games there is a ball simulator which you can use to simulate "playing" the game. The simulator allows you to use the keyboard to make shots/hit targets with your virtual/invisible pinball..

Essentially, it triggers the correct switches depending on where the balls are located. You can program a simulator for your own favourite game if you know a bit of programming and a lot about the game. If you are intersted let us know and we'll try to explain how to do it.

For more information, please refer to [simulation.txt](release/simulation.txt) for instructions on using the pinball simulator built into PinMAME! Many games are not fully simulated, but some have at least preliminary simulator support!

# Note from the PinMAME Development team

We're working hard to improve this great emulator, and welcome your feedback!! Please do not hesitate to contact us with questions, bug reports, suggestions, code patches, whatever!