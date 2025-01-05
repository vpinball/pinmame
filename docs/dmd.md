# DMD

Emulating the DMD pursues the following purposes:
- providing emulators (PinMame, VPX,...) with informations allowing rendering of a virtual DMD as accurately as possible,
- driving modern DMD hardware boards which implement their own shading capabilities,
- providing frames that can be uniquely identified to perform colorization or trigger events (f.e. PinUp display events).

These aims lead the DMD emulation to evaluate 2 different things:
- luminance frames, which are suited for accurate rendering, but may vary as the luminance model improves,
- raw frames, which can be used to uniquely identify frames for colorization, and can also be used for rendering but less 
accurately (mainly for backward compatibility, since this was the only thing available until PinMame 3.6).

All DMD control boards use Pulse Width Modulation (PWM) to create shades: they quickly toggle the dots on and 
off, and the observer eyes perceive a shade between fully off and fully on. The limit between perceiving flicker vs perceiving
a stable shade is called the [flicker/fusion threshold](https://en.wikipedia.org/wiki/Flicker_fusion_threshold). It depends mainly on 2 factors:
- the speed at which the dots switch on/off (do they ramp up/down quickly or do they fade more slowly),
- the device luminance level, which impacts how the human eye perceive light (Ferry/Porter law).

For neon plasma DMDs, the flicker/fusion threshold is estimated to be around **25Hz**, based on the analysis of existing
hardware that feature PWM frequencies down to 30Hz, but avoid to get lower (See Phantom Haus). This means that the perceived 
luminance is influenced by what was displayed during the last 1/25 seconds (40ms). A consequence of this is that correct 
luminance may only be evaluated by considering the frames that makes up the PWM pattern as well as the ones displayed before.

The flicker/fusion limit for LED DMDs is unknown, perhaps higher as display is usually less bright which impacts the Ferry/Porter
law, and LEDs have a faster on/off switching time.


## State of DMD emulation

The table below gives the main information (PWM FPS / Display FPS / PWM pattern) for all emulated hardware, as well as the state of emulation as of writing (2024/09/18):

| Name                                                        | PWM FPS / Disp.FPS     | PWM pattern | Emulation comments                                             |
|-------------------------------------------------------------|------------------------|-------------|----------------------------------------------------------------|
|[WPC](#wpc)                                                  | **122.1** / 61.1-40.7  | 2/3 frames  |                                                                |
|[WPC Phantom Haus](#wpc)                                     | 61.05 / 30.1           | 2 frames    |                                                                |
|[Data East 128x16](#data-east-128x16)                        | 177.5 / **59.2**       | 2u row      |                                                                |
|[Data East 128x32](#data-east-128x32-segastern-whitestar)    | 234.2 / **78.1**       | 2u row      |Some machines exhibit slow startup                              |
|Sega 192x64                                                  | 224.2 / 74.73          | 2u row      |Title screen shows sometimes, likely due to timing issues       |
|Gottlieb GTS3                                                | **375.9** / 125.3-37.6 | 3/6/8/10 frames |                                                            |
|[Alvin G. 1](#alvin-g)                                       | 332.4 / 83.1           | 4 row       |Still a little flicker on the title screen                      |
|[Alvin G. 2](#alvin-g)                                       | 298.6 / **74.6**       | 4 row       |                                                                |
|[Sega/Stern Whitestar](#data-east-128x32-segastern-whitestar)| 233.3 / **77.8**       | 2u row      |                                                                |
|Inder                                                        | **134.8** / ?          | ?           |Review in progress                                              |
|Sleic                                                        | ?                      | ?           |Review in progress                                              |
|Spinball                                                     | **132.9**              | ?           |Review in progress                                              |
|Capcom                                                       | **508.6** / 127.2 ?    | 4 ?         |Review in progress                                              |
|[Stern SAM](#stern-sam)                                      | 751.2 / **62.6**       | 4u row      |Needs overall emulation timing fixes, interframe emulation, shade validation, back/front mix validation|
|[Stern Spike 1](#stern-spike-1)                              | 952.4 / **63.5**       | 4u frames   |Unsupported hardware                                            |

- 'u' stands for 'unbalanced': each row/frame has a different display length.
- All FPS are expressed in Hz (same as frame per second), the ones in **bold** have been verified on real hardware.
- PWM can be performed per row or per frame (PWM FPS is computed considering equivalent per frame FPS). When done per frame, the game code may dynamically change the PWM pattern length.


## WPC

WPC continuously renders frames at 122.1Hz. The game code uses this to render sequences of 
either 1 (0/100), 2 (0/50/100), or 3 (0/33/66/100) frames, expecting the viewer eye to 
merge these frames into shades. For example, Terminator 2 uses a 2 frame PWM pattern while
Creature from the Black Lagoon uses a 3 frame PWM pattern.

Phantom Haus uses a double height DMD which is rasterized by the same hardware at half frequency: 61.5Hz.
As a consequence, the game only uses a 2 frame PWM pattern, likely to avoid flicker.


## Alvin G

This hardware is made of discrete circuits with available datasheets, allowing to directly read & perform digital simulation. There have been
2 different board revisions:
- PCA020 for "Al's Garage Band Goes On a World Tour" which was modified after production by adding a clock divider to the rasterizer (existing schematics are incomplete),
- PCA020A which integrates this clock divider as well as other slight improvments for later games.

This is a simple rasterizer which rasterizes each row 4 times, either from the same data source (no shading) or from 4 contiguous RAM locations allowing 0 / 25 / 50 / 75 / 100 brightness levels.


## Data East 128x16

The hardware uses a Z80 that performs frame preparation as well as direct rasterization to the DMD. It rasterizes each row twice with different lengths: 0.13ms and 0.37ms.
This allows the hardware to render 0 / 33 / 66 / 100 brightness levels.


## Data East 128x32 & Sega/Stern Whitestar

These systems share nearly identical hardware: board 520-5055-00 for Data East, 520-5055-01 to 520-5055-03 for Whitestar.
These boards are built around a CRTC 6845 rasterizer, a 6809 CPU and 2 PAL chips in charge of address decoding (U16) and custom rasterization (U2).

Precise signal recordings are available from [RGB DMD project](https://github.com/ecurtz/RGB_DMD/tree/master/recordings).
They show that each row is rasterized twice, once at 500kHz and then at 1MHz.
They also show that the rasterizer setup changed between the initial revision of the board (Data East) and later ones (Sega/Stern). Since the wiring
did not change and the software seems similar, this leads to the guess that the U2 chip setup was modified.

U16 is the main address decoder. It is a PAL16L8, which is a logic combiner (no flip flop). These are the guessed logics:

|Input|Output                           |Ty|Guessed output logic                                                        |
|-----|---------------------------------|--|----------------------------------------------------------------------------|
|BA15 |/RAMWR                           |O |Write (/BWR) & RAMCS                                                        |
|BA14 |/RAMCS                           |IO|BA = 0xxx &#124; BA = 1xxx &#124; BA = 2xxx                                 |
|BA13 |/PORTIN => read latch from CPU   |IO|Read (/BWR) & BA = 3xxx & BA1                                               |
|BA12 |/CRTCCS                          |IO|BA = 3xxx & not BA1                                                         |
|XA0  |CORE => Enable XA ROM/RAM banking|IO|BA = 2xxx &#124; BA = 4xxx                                                  |
|/BWR |/BSE => latch XA0..7             |IO|Write (/BWR) & BA = 3xxx & BA1                                              |
|BA1  |/ROMCS => Read ROM / Write Status|IO|BA = 4xxx to Fxxx                                                           |
|XA5  |ZA0 => ROM A14 / RAM MA12        |O |XA0 or XA5 depending on ROMCS/RAMCS and on BA12..15 for static ROM/RAM area |
|/E   |
|/Q   |

U2 drives the rasterizer by toggling the 6845 clock signal divider. It is a PAL16R4, which is a logic combiner with additional flip flops.
Its behavior is still largely unknown.

|Input           |Output         |Ty|Guessed output logic                                       |
|----------------|---------------|--|-----------------------------------------------------------|
|CURSOR from 6845|/FIRQ          |IO|                                                           |
|HSYNC from 6845 |/DE            |IO|                                                           |
|DDATA           |/SDATA         |Q |                                                           |
|RA0             |/RDATA         |Q |                                                           |
|DE from 6845    |/DOTCLK to 6845|Q |Derived from (RA0 ? Clock/2 : Clock/4)                     |
|/CRTCCS         |N.C.           |Q |Internal Clock/2                                           |
|CA0 from 6845   |/PCLOCK        |IO|                                                           |
|CA13 from 6845  |CLATCH         |IO|Derived from HSYNC                                         |


## Stern SAM

Precise signal recordings are available from [RGB DMD project](https://github.com/ecurtz/RGB_DMD/tree/master/recordings). They show that 
on the SAM system, each row is rasterized 4 times with different lengths: 2 / 4 / 1 / 5 times 41.6µs. These timings lead to 12x41.6µs per 
row, and 32x12x41.6µs = 15.97ms per frame which match the 62.6 FPS measured on the real hardware.

This design allows the hardware to display 13 regularly spread shades from 0 to 100%, but when rasteriring, RAM only uses 12 of these.
The mapping between these is guessed by the emulation and maybe incorrect.


## Stern Spike 1

Precise signal recordings are available from [RGB DMD project](https://github.com/ecurtz/RGB_DMD/tree/master/recordings). They show that 
the Spike 1 system uses a PWM pattern made of 4 frames with different lengths: 1 / 2 / 4 / 8 times 1.05ms. These timings lead to 15.75ms for 
a complete PWM pattern, that is to say 63.5 FPS, and allows to create 16 regularly spread shades.
