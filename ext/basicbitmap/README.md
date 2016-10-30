# BasicBitmap
As the purpose of providing a simple compact, high-performance and platform independent Bitmap library, BasicBitmap is created based on a subset of my vector graphic library: https://github.com/skywind3000/pixellib .

# Features
- Highly optimized C++ code which could be compiled and executed in any platform
- Multiple pixel formats from 8 to 32 bits: A8R8G8B8, R8G8B8, A4R4G4B4, R5G6B5, A8, etc.
- Blit in opaque or transparent mode (with a maskcolor/colorkey)
- Converting between different pixel formats
- Blending with different compositors
- Scaling with different filters (nearest, linear, bilinear)
- High quality resampling to different size (Bicubic/Bilinear)
- Loading bmp/tga from memory or file and saving bmp to file
- Loading png/jpg with gdiplus (only in windows xp or above)
- Saving bmp/ppm files
- Self-contained and not rely on other libraries
- Compact and portable, just copy BasicBitmap.h/.cpp into your project
- Core routines can be replaced by external (eg. SSE2 implementations)
- Blitting is 40% faster than DirectDraw FastBlt (with AVX/SSE2 optimizing enabled).
- Blending is 34% faster than GDI's AlphaBlend (with SSE2 enabled).


# Performance

Blit Performance
----------------

Full window (800x600) blitting (both opacity and transparent), compare to GDI/SDL/DirectDraw:

|   32 Bits Blit       |  Opacity  | Transparent |
|----------------------|-----------|-------------|
| BasicBitmap C++      | fps=2325  |   fps=1368  |
| BasicBitmap AVX/SSE2 | fps=2904  |   fps=2531  |
| GDI                  | fps=2333  |   fps=1167  |
| SDL                  | fps=2671  |   fps=1015  |
| DirectDraw           | fps=2695  |   fps=2090  |

Note: use BltFast with DirectDrawSurface7 in System Memory to perform Opacity & Transparent blit. BitBlt and TransparentBlt(msimg32.dll) are used in the GDI testing case. 

|   16 Bits Blit       |  Opacity  | Transparent |
|----------------------|-----------|-------------|
| BasicBitmap C++      | fps=4494  |  fps=1253   |
| BasicBitmap AVX/SSE2 | fps=9852  |  fps=2909   |
| DirectDraw BltFast   | fps=5889  |  fps=861    |

Blitting performance in SDL & GDI are slower than DirectDraw, just compare to ddraw as well.

|    8 Bits Blit       |  Opacity  | Transparent |
|----------------------|-----------|-------------|
| BasicBitmap C++      | fps=11142 |  fps=1503   |
| BasicBitmap AVX/SSE2 | fps=18181 |  fps=5449   |
| DirectDraw BltFast   | fps=14705 |  fps=4832   |

DirectDrawSurface in *Video Memory* takes the benefit of hardware acceleration which is definitely faster than BasicBitmap. If you really need hardware acceleration, use OpenGL/DX as well. 

*BasicBitmap* is a software implementation which aims to achieve the best performance in all other software implementations: like GDI/GDI+, SDL/DirectDraw in System Memory, for examples.

So just compare to DirectDrawSurface in *System Memory*. Use it in the condition that you only need a lightweight software solution: GUI/Cross Platform/hardware unavailable/image processing/video compositing, etc.



Blending Performance
----------------------

|  SRC OVER         |    FPS    |
|-------------------|-----------|
| BasicBitmap C++   |    594    |
| BasicBitmap SSE2  |   1731    |
| GDI (msimg32.dll) |   1137    |

note: 800x600 full window src-over blending vs GDI's AlphaBlend function (in msimg32.dll).
 