cmake_minimum_required(VERSION 3.25)

set(PLATFORM "win" CACHE STRING "Platform")
set(ARCH "x64" CACHE STRING "Arch")

option(BUILD_SHARED "Option to build shared library" ON)
option(BUILD_STATIC "Option to build static library" ON)

message(STATUS "PLATFORM: ${PLATFORM}")
message(STATUS "ARCH: ${ARCH}")

message(STATUS "BUILD_SHARED: ${BUILD_SHARED}")
message(STATUS "BUILD_STATIC: ${BUILD_STATIC}")

if(PLATFORM STREQUAL "macos")
   set(CMAKE_OSX_DEPLOYMENT_TARGET 14.0)
elseif(PLATFORM STREQUAL "ios" OR PLATFORM STREQUAL "ios-simulator")
   set(CMAKE_SYSTEM_NAME iOS)
   if (PLATFORM STREQUAL "ios-simulator")
      set(CMAKE_OSX_SYSROOT iphonesimulator)
   endif()
   set(CMAKE_OSX_DEPLOYMENT_TARGET 17.0)
elseif(PLATFORM STREQUAL "tvos")
   set(CMAKE_SYSTEM_NAME tvOS)
   set(CMAKE_OSX_DEPLOYMENT_TARGET 17.0)
elseif(PLATFORM STREQUAL "android")
   set(CMAKE_SYSTEM_NAME Android)
   set(CMAKE_SYSTEM_VERSION 30)
   set(CMAKE_ANDROID_ARCH_ABI arm64-v8a)
endif()

file(READ src/version.h version)
string(REGEX MATCH "VERSION_MAJOR[ ]+([0-9]+)" _tmp ${version})
set(VERSION_MAJOR "${CMAKE_MATCH_1}")
string(REGEX MATCH "VERSION_MINOR[ ]+([0-9]+)" _tmp ${version})
set(VERSION_MINOR "${CMAKE_MATCH_1}")
string(REGEX MATCH "VERSION_REV[ ]+([0-9]+)" _tmp ${version})
set(VERSION_REV "${CMAKE_MATCH_1}")

project(pinmame VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REV}"
   DESCRIPTION "Cross-platform PinMAME library")

if(PLATFORM STREQUAL "win")
   if(ARCH STREQUAL "x86")
      add_compile_definitions(WIN32)
   endif()
elseif(PLATFORM STREQUAL "macos")
   if (ARCH STREQUAL "arm64")
      set(CMAKE_OSX_ARCHITECTURES arm64)
   elseif(ARCH STREQUAL "x64")
      set(CMAKE_OSX_ARCHITECTURES x86_64)
   endif()
   set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
   set(CMAKE_INSTALL_RPATH "@executable_path")
elseif(PLATFORM STREQUAL "ios" OR PLATFORM STREQUAL "ios-simulator" OR PLATFORM STREQUAL "tvos")
   set(CMAKE_OSX_ARCHITECTURES arm64)
elseif(PLATFORM STREQUAL "linux" OR PLATFORM STREQUAL "android")
   set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
   set(CMAKE_INSTALL_RPATH "$ORIGIN")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)

add_compile_definitions(
   HAS_M6809=1
   HAS_M6808=1
   HAS_M6800=1
   HAS_M6803=1
   HAS_M6802=1
   HAS_ADSP2101=1
   HAS_ADSP2105=1
   HAS_Z80=1
   HAS_M6502=1
   HAS_M65C02=1
   HAS_M68000=1
   HAS_M68306=1
   HAS_S2650=1
   HAS_8080=1
   HAS_8085A=1
   HAS_I8035=1
   HAS_I8039=1
   HAS_I86=1
   HAS_I88=1
   HAS_I186=1
   HAS_I188=1
   HAS_4004=1
   HAS_PPS4=1
   HAS_SCAMP=1
   HAS_I8051=1
   HAS_I8752=1
   HAS_TMS7000=1
   HAS_AT91=1
   HAS_ARM7=1
   HAS_CDP1802=1
   HAS_TMS9980=1
   HAS_TMS9995=1
   HAS_COP420=1
   HAS_DAC=1
   HAS_YM2151_YMFM=1
   HAS_HC55516=1
   HAS_MC3417=1
   HAS_SAMPLES=1
   HAS_TMS5220=1
   HAS_AY8910=1
   HAS_MSM5205=1
   HAS_CUSTOM=1
   HAS_BSMT2000=1
   HAS_OKIM6295=1
   HAS_ADPCM=1
   HAS_VOTRAXSC01=1
   HAS_SN76477=1
   HAS_SN76496=1
   HAS_DISCRETE=1
   HAS_SP0250=1
   HAS_TMS320AV120=1
   HAS_M114S=1
   HAS_YM3812=1
   HAS_S14001A=1
   HAS_YM2203=1
   HAS_YM3526=1
   HAS_TMS5110=1
   HAS_SP0256=1
   HAS_Y8950=1
   HAS_ASTROCADE=1
   HAS_YMF262=1
   HAS_MEA8000=1
   HAS_SAA1099=1
   HAS_QSOUND=1

   MAMEVER=7300
   PINMAME
   PINMAME_NO_UNUSED
   LIBPINMAME
   SAM_INCLUDE_COLORED

   LSB_FIRST
   PI=M_PI
)

if(PLATFORM STREQUAL "win")
   add_compile_definitions(
      __LP64__
   )
else()
   add_compile_definitions(
      HAVE_UNISTD_H=1
   )
endif()

add_definitions( "-DINLINE=static inline __attribute__((always_inline))" )

set(PINMAME_SOURCES
   src/artwork.c
   src/artwork.h
   src/audit.c
   src/audit.h
   src/cheat.c
   src/cheat.h
   src/common.c
   src/common.h
   src/config.c
   src/config.h
   src/cpu/adsp2100/2100dasm.c
   src/cpu/adsp2100/adsp2100.c
   src/cpu/adsp2100/adsp2100.h
   src/cpu/arm7/arm7.c
   src/cpu/arm7/arm7.h
   src/cpu/arm7/arm7dasm.c
   src/cpu/at91/at91.c
   src/cpu/at91/at91.h
   src/cpu/at91/at91dasm.c
   src/cpu/cdp1802/1802dasm.c
   src/cpu/cdp1802/cdp1802.c
   src/cpu/cdp1802/cdp1802.h
   src/cpu/cop400/410ops.c
   src/cpu/cop400/420ops.c
   src/cpu/cop400/cop400.h
   src/cpu/cop400/cop420.c
   src/cpu/cop400/cop420ds.c
   src/cpu/i86/i86.c
   src/cpu/i86/i86.h
   src/cpu/i86/i86dasm.c
   src/cpu/i4004/4004dasm.c
   src/cpu/i4004/i4004.c
   src/cpu/i4004/i4004.h
   src/cpu/i8039/8039dasm.c
   src/cpu/i8039/i8039.c
   src/cpu/i8039/i8039.h
   src/cpu/i8051/8051dasm.c
   src/cpu/i8051/i8051.c
   src/cpu/i8051/i8051.h
   src/cpu/i8085/8085dasm.c
   src/cpu/i8085/i8085.c
   src/cpu/i8085/i8085.h
   src/cpu/m6502/6502dasm.c
   src/cpu/m6502/m6502.c
   src/cpu/m6502/m6502.h
   src/cpu/m6800/6800dasm.c
   src/cpu/m6800/m6800.c
   src/cpu/m6800/m6800.h
   src/cpu/m6809/6809dasm.c
   src/cpu/m6809/m6809.c
   src/cpu/m6809/m6809.h
   src/cpu/m68000/generated_by_m68kmake/m68kopac.c
   src/cpu/m68000/generated_by_m68kmake/m68kops.c
   src/cpu/m68000/generated_by_m68kmake/m68kopdm.c
   src/cpu/m68000/generated_by_m68kmake/m68kopnz.c
   src/cpu/m68000/generated_by_m68kmake/m68kops.h
   src/cpu/m68000/m68kcpu.c
   src/cpu/m68000/m68kcpu.h
   src/cpu/m68000/m68kdasm.c
   src/cpu/m68000/m68kmame.c
   src/cpu/m68000/m68kmame.h
   src/cpu/pps4/pps4.c
   src/cpu/pps4/pps4.h
   src/cpu/pps4/pps4dasm.c
   src/cpu/s2650/2650dasm.c
   src/cpu/s2650/s2650.c
   src/cpu/s2650/s2650.h
   src/cpu/scamp/scamp.c
   src/cpu/scamp/scamp.h
   src/cpu/scamp/scampdsm.c
   src/cpu/tms7000/7000dasm.c
   src/cpu/tms7000/tms7000.c
   src/cpu/tms7000/tms7000.h
   src/cpu/tms9900/9900dasm.c
   src/cpu/tms9900/tms9900.c
   src/cpu/tms9900/tms9900.h
   src/cpu/tms9900/tms9980a.c
   src/cpu/tms9900/tms9995.c
   src/cpu/z80/z80.c
   src/cpu/z80/z80.h
   src/cpu/z80/z80dasm.c
   src/cpu/z80/z80dasm.h
   src/cpuexec.c
   src/cpuexec.h
   src/cpuint.c
   src/cpuint.h
   src/cpuintrf.c
   src/cpuintrf.h
   src/datafile.c
   src/datafile.h
   src/drawgfx.c
   src/drawgfx.h
   src/driver.h
   src/fileio.c
   src/fileio.h
   src/harddisk.c
   src/harddisk.h
   src/hash.c
   src/hash.h
   src/hiscore.c
   src/hiscore.h
   src/info.c
   src/info.h
   src/inptport.c
   src/inptport.h
   src/input.c
   src/input.h
   src/machine/4094.c
   src/machine/4094.h
   src/machine/6522via.c
   src/machine/6522via.h
   src/machine/6530riot.c
   src/machine/6530riot.h
   src/machine/6532riot.c
   src/machine/6532riot.h
   src/machine/6821pia.c
   src/machine/6821pia.h
   src/machine/8255ppi.c
   src/machine/8255ppi.h
   src/machine/eeprom.c
   src/machine/eeprom.h
   src/machine/i8155.c
   src/machine/i8155.h
   src/machine/mathbox.c
   src/machine/mathbox.h
   src/machine/pic8259.c
   src/machine/pic8259.h
   src/machine/ticket.c
   src/machine/ticket.h
   src/machine/z80fmly.c
   src/machine/z80fmly.h
   src/mame.c
   src/mame.h
   src/mamedbg.c
   src/mamedbg.h
   src/md5.c
   src/md5.h
   src/memory.c
   src/memory.h
   src/osdepend.h
   src/palette.c
   src/palette.h
   src/pinmame.h
   src/png.c
   src/png.h
   src/profiler.c
   src/profiler.h
   src/sha1.c
   src/sha1.h
   src/sndintrf.c
   src/sndintrf.h
   src/sound/262intf.c
   src/sound/262intf.h
   src/sound/2151intf.c
   src/sound/2151intf.h
   src/sound/2203intf.c
   src/sound/2203intf.h
   src/sound/3812intf.c
   src/sound/3812intf.h
   src/sound/5110intf.c
   src/sound/5110intf.h
   src/sound/5220intf.c
   src/sound/5220intf.h
   src/sound/adpcm.c
   src/sound/adpcm.h
   src/sound/astrocde.c
   src/sound/astrocde.h
   src/sound/ay8910.c
   src/sound/ay8910.h
   src/sound/bsmt2000.c
   src/sound/bsmt2000.h
   src/sound/dac.c
   src/sound/dac.h
   src/sound/discrete.c
   src/sound/discrete.h
   src/sound/filter.c
   src/sound/filter.h
   src/sound/fm.c
   src/sound/fm.h
   src/sound/fmopl.c
   src/sound/fmopl.h
   src/sound/hc55516.c
   src/sound/hc55516.h
   src/sound/m114s.c
   src/sound/m114s.h
   src/sound/mc3417.c
   src/sound/mc3417.h
   src/sound/mea8000.c
   src/sound/mea8000.h
   src/sound/mixer.c
   src/sound/mixer.h
   src/sound/msm5205.c
   src/sound/msm5205.h
   src/sound/qsound.c
   src/sound/qsound.h
   src/sound/s14001a.c
   src/sound/s14001a.h
   src/sound/saa1099.c
   src/sound/saa1099.h
   src/sound/samples.c
   src/sound/samples.h
   src/sound/sn76477.c
   src/sound/sn76477.h
   src/sound/sn76496.c
   src/sound/sn76496.h
   src/sound/sp0250.c
   src/sound/sp0250.h
   src/sound/sp0256.c
   src/sound/sp0256.h
   src/sound/streams.c
   src/sound/streams.h
   src/sound/tms320av120.c
   src/sound/tms320av120.h
   src/sound/tms5110.c
   src/sound/tms5110.h
   src/sound/tms5220.c
   src/sound/tms5220.h
   src/sound/votrax.c
   src/sound/votrax.h
   src/sound/ym2151.c
   src/sound/ym2151.h
   src/sound/ymdeltat.c
   src/sound/ymdeltat.h
   src/sound/ymf262.c
   src/sound/ymf262.h
   src/state.c
   src/state.h
   src/tilemap.c
   src/tilemap.h
   src/timer.c
   src/timer.h
   src/ui_text.c
   src/ui_text.h
   src/unzip.c
   src/unzip.h
   src/usrintrf.c
   src/usrintrf.h
   src/version.c
   src/vidhrdw/avgdvg.c
   src/vidhrdw/avgdvg.h
   src/vidhrdw/crtc6845.c
   src/vidhrdw/crtc6845.h
   src/vidhrdw/generic.c
   src/vidhrdw/generic.h
   src/vidhrdw/tms9928a.c
   src/vidhrdw/tms9928a.h
   src/vidhrdw/vector.c
   src/vidhrdw/vector.h
   src/window.c
   src/window.h
   src/wpc/allied.c
   src/wpc/alvg.c
   src/wpc/alvg.h
   src/wpc/alvgdmd.c
   src/wpc/alvgdmd.h
   src/wpc/alvggames.c
   src/wpc/alvgs.c
   src/wpc/alvgs.h
   src/wpc/atari.c
   src/wpc/atari.h
   src/wpc/atarigames.c
   src/wpc/atarisnd.c
   src/wpc/barni.c
   src/wpc/bingo.c
   src/wpc/boomerang.c
   src/wpc/bowarrow.c
   src/wpc/bowlgames.c
   src/wpc/bulb.c
   src/wpc/bulb.h
   src/wpc/by35.c
   src/wpc/by35.h
   src/wpc/by35games.c
   src/wpc/by35snd.c
   src/wpc/by35snd.h
   src/wpc/by6803.c
   src/wpc/by6803.h
   src/wpc/by6803games.c
   src/wpc/by68701.c
   src/wpc/byvidgames.c
   src/wpc/byvidpin.c
   src/wpc/byvidpin.h
   src/wpc/capcom.c
   src/wpc/capcom.h
   src/wpc/capcoms.c
   src/wpc/capcoms.h
   src/wpc/capgames.c
   src/wpc/core.c
   src/wpc/core.h
   src/wpc/dedmd.c
   src/wpc/dedmd.h
   src/wpc/degames.c
   src/wpc/desound.c
   src/wpc/desound.h
   src/wpc/driver.c
   src/wpc/efo.c
   src/wpc/efosnd.c
   src/wpc/flicker.c
   src/wpc/gen.h
   src/wpc/gp.c
   src/wpc/gp.h
   src/wpc/gpgames.c
   src/wpc/gpsnd.c
   src/wpc/gpsnd.h
   src/wpc/gts1.c
   src/wpc/gts1.h
   src/wpc/gts1games.c
   src/wpc/gts3.c
   src/wpc/gts3.h
   src/wpc/gts3dmd.c
   src/wpc/gts3dmd.h
   src/wpc/gts3games.c
   src/wpc/gts80.c
   src/wpc/gts80.h
   src/wpc/gts80games.c
   src/wpc/gts80s.c
   src/wpc/gts80s.h
   src/wpc/hnkgames.c
   src/wpc/hnks.c
   src/wpc/hnks.h
   src/wpc/icecoldbeer.c
   src/wpc/idsa.c
   src/wpc/inder.c
   src/wpc/inder.h
   src/wpc/indergames.c
   src/wpc/inderp.c
   src/wpc/jeutel.c
   src/wpc/joctronic.c
   src/wpc/jp.c
   src/wpc/jp.h
   src/wpc/jpgames.c
   src/wpc/jvh.c
   src/wpc/kissproto.c
   src/wpc/lancelot.c
   src/wpc/ltd.c
   src/wpc/ltd.h
   src/wpc/ltdgames.c
   src/wpc/luckydraw.c
   src/wpc/mac.c
   src/wpc/mech.c
   src/wpc/mech.h
   src/wpc/mephisto.c
   src/wpc/micropin.c
   src/wpc/mrgame.c
   src/wpc/mrgame.h
   src/wpc/mrgamegames.c
   src/wpc/nsm.c
   src/wpc/nuova.c
   src/wpc/peyper.c
   src/wpc/peyper.h
   src/wpc/peypergames.c
   src/wpc/play.c
   src/wpc/play.h
   src/wpc/playgames.c
   src/wpc/playsnd.c
   src/wpc/regama.c
   src/wpc/rotation.c
   src/wpc/rowamet.c
   src/wpc/s3games.c
   src/wpc/s4.c
   src/wpc/s4.h
   src/wpc/s4games.c
   src/wpc/s6.c
   src/wpc/s6.h
   src/wpc/s6games.c
   src/wpc/s7.c
   src/wpc/s7.h
   src/wpc/s7games.c
   src/wpc/s11.c
   src/wpc/s11.h
   src/wpc/s11games.c
   src/wpc/se.c
   src/wpc/se.h
   src/wpc/segames.c
   src/wpc/sim.c
   src/wpc/sim.h
   src/wpc/sims/wpc/full/sttng.c
   src/wpc/sims/wpc/full/fh.c
   src/wpc/sims/wpc/full/jd.c
   src/wpc/sims/wpc/full/bop.c
   src/wpc/sims/wpc/full/ft.c
   src/wpc/sims/wpc/full/gw.c
   src/wpc/sims/wpc/full/afm.c
   src/wpc/sims/wpc/full/tz.c
   src/wpc/sims/wpc/full/taf.c
   src/wpc/sims/wpc/full/pz.c
   src/wpc/sims/wpc/full/t2.c
   src/wpc/sims/wpc/full/ngg.c
   src/wpc/sims/wpc/full/dd_wpc.c
   src/wpc/sims/wpc/full/cftbl.c
   src/wpc/sims/wpc/full/mm.c
   src/wpc/sims/wpc/full/br.c
   src/wpc/sims/wpc/full/wcs.c
   src/wpc/sims/wpc/full/hd.c
   src/wpc/sims/wpc/full/drac.c
   src/wpc/sims/wpc/full/ss.c
   src/wpc/sims/wpc/full/ww.c
   src/wpc/sims/wpc/full/ij.c
   src/wpc/sims/wpc/full/tom.c
   src/wpc/sims/wpc/full/rs.c
   src/wpc/sims/wpc/full/hurr.c
   src/wpc/sims/wpc/full/gi.c
   src/wpc/sims/wpc/full/dm.c
   src/wpc/sims/wpc/prelim/cp.c
   src/wpc/sims/wpc/prelim/ts.c
   src/wpc/sims/wpc/prelim/pop.c
   src/wpc/sims/wpc/prelim/wd.c
   src/wpc/sims/wpc/prelim/jm.c
   src/wpc/sims/wpc/prelim/i500.c
   src/wpc/sims/wpc/full/nbaf.c
   src/wpc/sims/wpc/prelim/corv.c
   src/wpc/sims/wpc/prelim/fs.c
   src/wpc/sims/wpc/prelim/sc.c
   src/wpc/sims/wpc/prelim/mb.c
   src/wpc/sims/wpc/prelim/totan.c
   src/wpc/sims/wpc/prelim/congo.c
   src/wpc/sims/wpc/prelim/nf.c
   src/wpc/sims/wpc/prelim/jb.c
   src/wpc/sims/wpc/prelim/cv.c
   src/wpc/sims/wpc/prelim/dw.c
   src/wpc/sims/wpc/prelim/dh.c
   src/wpc/sims/wpc/prelim/cc.c
   src/wpc/sims/wpc/prelim/jy.c
   src/wpc/sims/se/prelim/monopoly.c
   src/wpc/sims/se/prelim/elvis.c
   src/wpc/sims/se/prelim/harley.c
   src/wpc/sims/s11/full/dd.c
   src/wpc/sims/s11/full/milln.c
   src/wpc/sims/s11/prelim/eatpm.c
   src/wpc/sims/s7/full/bk.c
   src/wpc/sims/s7/full/tmfnt.c
   src/wpc/slalom.c
   src/wpc/sleic.c
   src/wpc/sleic.h
   src/wpc/sleicgames.c
   src/wpc/snd_cmd.c
   src/wpc/snd_cmd.h
   src/wpc/sndbrd.c
   src/wpc/sndbrd.h
   src/wpc/spectra.c
   src/wpc/spinb.c
   src/wpc/spinb.h
   src/wpc/spinbgames.c
   src/wpc/spiritof76.c
   src/wpc/stargame.c
   src/wpc/stgames.c
   src/wpc/stsnd.c
   src/wpc/stsnd.h
   src/wpc/tabart.c
   src/wpc/taito.c
   src/wpc/taito.h
   src/wpc/taitogames.c
   src/wpc/taitos.c
   src/wpc/taitos.h
   src/wpc/techno.c
   src/wpc/vd.c
   src/wpc/vpintf.c
   src/wpc/vpintf.h
   src/wpc/wico.c
   src/wpc/wmssnd.c
   src/wpc/wmssnd.h
   src/wpc/wpc.c
   src/wpc/wpc.h
   src/wpc/wpcgames.c
   src/wpc/wpcsam.c
   src/wpc/wpcsam.h
   src/wpc/zac.c
   src/wpc/zac.h
   src/wpc/zacgames.c
   src/wpc/zacproto.c
   src/wpc/zacsnd.c
   src/wpc/zacsnd.h

   src/libpinmame/video.c
   src/libpinmame/video.h
   src/libpinmame/joystick.c
   src/libpinmame/fileio.c
   src/libpinmame/misc.c
   src/libpinmame/misc.h
   src/libpinmame/libpinmame.cpp
   src/libpinmame/libpinmame.h

   ext/vgm/vgmwrite.c
   ext/vgm/vgmwrite.h

   ext/ymfm/ymfm_opm.cpp
   ext/ymfm/ymfm_opm.h

   ext/zlib/adler32.c
   ext/zlib/compress.c
   ext/zlib/crc32.c
   ext/zlib/deflate.c
   ext/zlib/gzclose.c
   ext/zlib/gzlib.c
   ext/zlib/gzread.c
   ext/zlib/gzwrite.c
   ext/zlib/infback.c
   ext/zlib/inffast.c
   ext/zlib/inflate.c
   ext/zlib/inftrees.c
   ext/zlib/trees.c
   ext/zlib/uncompr.c
   ext/zlib/zutil.c
)

if(PLATFORM STREQUAL "win")
   list(APPEND PINMAME_SOURCES
      src/vc/dirent.c
      src/vc/dirent.h
      src/vc/unistd.h

      src/windows/jit.c
      src/windows/jit.h
      src/windows/jitemit.c
      src/windows/jitemit.h
      src/windows/ticker.c
   )
else()
   list(APPEND PINMAME_SOURCES
      src/libpinmame/ticker.c
   )
endif()
   
set(PINMAME_INCLUDE_DIRS
   src
   src/wpc
   src/cpu/m68000/generated_by_m68kmake
   src/unix
   src/unix/sysdep
)

if(PLATFORM STREQUAL "win")
   list(APPEND PINMAME_INCLUDE_DIRS
      src/vc
      src/windows
      ext/zlib
   )
endif()

if(BUILD_SHARED)
   add_library(pinmame_shared SHARED ${PINMAME_SOURCES})

   target_include_directories(pinmame_shared PUBLIC ${PINMAME_INCLUDE_DIRS})

   if(PLATFORM STREQUAL "win")
      target_link_libraries(pinmame_shared PUBLIC winmm)
   endif()

   if(PLATFORM STREQUAL "win" AND ARCH STREQUAL "x64")
      set(PINMAME_OUTPUT_NAME "pinmame64")
   else()
      set(PINMAME_OUTPUT_NAME "pinmame")
   endif()

   set_target_properties(pinmame_shared PROPERTIES
      OUTPUT_NAME ${PINMAME_OUTPUT_NAME}
      VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REV}
   )

   install(TARGETS pinmame_shared
      LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
   )

   install(FILES src/libpinmame/libpinmame.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

   if(PLATFORM STREQUAL "win" OR PLATFORM STREQUAL "macos" OR PLATFORM STREQUAL "linux")
      add_executable(pinmame_test
         src/libpinmame/test.cpp
      )
      target_link_libraries(pinmame_test PUBLIC pinmame_shared)
   endif()
endif()

if(BUILD_STATIC)
   add_library(pinmame_static STATIC ${PINMAME_SOURCES})

   target_include_directories(pinmame_static PUBLIC ${PINMAME_INCLUDE_DIRS})

   if(PLATFORM STREQUAL "win")
      set_target_properties(pinmame_static PROPERTIES
         OUTPUT_NAME "pinmame_static"
      )
   else()
      set_target_properties(pinmame_static PROPERTIES
         OUTPUT_NAME "pinmame"
      )
   endif()

   install(TARGETS pinmame_static
      LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
   )

   install(FILES src/libpinmame/libpinmame.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

   if(PLATFORM STREQUAL "win" OR PLATFORM STREQUAL "macos" OR PLATFORM STREQUAL "linux")
      add_executable(pinmame_test_s
         src/libpinmame/test.cpp
      )

      if(PLATFORM STREQUAL "win")
         target_link_libraries(pinmame_test_s PUBLIC pinmame_static winmm)
      else()
         target_link_libraries(pinmame_test_s PUBLIC pinmame_static)
      endif()
   endif()
endif()
