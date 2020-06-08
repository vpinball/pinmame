#ifndef DRIVER_RECURSIVE
#  define DRIVER_RECURSIVE
#  include "driver.h"

const struct GameDriver driver_0 = {
 __FILE__, 0, "", 0, 0, 0, 0, 0, 0, 0, 0, NOT_A_DRIVER
};
#  define DRIVER(name, ver) extern struct GameDriver driver_##name##_##ver;
#  define DRIVERNV(name) extern struct GameDriver driver_##name;
#  include "driver.c"
#  undef DRIVER
#  undef DRIVERNV
#  define DRIVER(name, ver) &driver_##name##_##ver,
#  define DRIVERNV(name) &driver_##name,
const struct GameDriver *drivers[] = {
#  include "driver.c"
0 /* end of array */
};
#if MAMEVER >= 6300
const struct GameDriver *test_drivers[] = { 0 };
#endif
#else /* DRIVER_RECURSIVE */

/* A comment on the "LED Ghost Fix" MODs from https://emmytech.com/arcade/led_ghost_busting/index.html :
What is Ghosting ?
When talking about pinball machines and LED's, the term 'ghosting' is frequently used to describe the behavior of an LED that should be off glowing dimly when another LED is turned on.

What causes ghosting ?
Anything that can cause an LED or incandescent lamp to briefly receive current when it should not can cause ghosting.The effect is much more pronounced with LED's because of their quick turn on time relative to incandescent lamps.
The predominant cause for Williams / Bally WPC Era pins is a mixture of a lamp matrix software driver timing issue and what appears to be an issue with the WPC ASIC.

How can LED ghosting be eliminated ?
In the 1995 timeframe, the Williams software designers modified the lamp matrix device driver code to work around the WPC ASIC
issue and to modify the timing with how matrix is controlled.
For 'most' cases this appears to have eliminated the ghosting issue.
I say most because I have read a number of posts where people say they still have LED ghosting issues even when they are running
ROM's with the updated driver code.
I have also read posts stating that the ghosting issue is more prevalent when Rotten Dog power/driver boards are used.
*/

DRIVERNV(st_game)       // Unknown game running on old Stern hardware
DRIVERNV(mac_zois)      // 05/03 machinaZOIS Virtual Training Center
DRIVERNV(wldtexas)      // Wild Texas (Firepower II clone)

// --------------------
// ALLIED LEISURE INC.
// --------------------
// early Allied solid-state games don't use a CPU!
DRIVERNV(suprpick)      // 01/77 Super Picker
DRIVERNV(thndbolt)      // 11/77 Thunderbolt
DRIVERNV(hoedown)       // 03/78 Hoe Down
DRIVERNV(takefive)      // 04/78 Take Five
DRIVERNV(heartspd)      // 12/78 Hearts & Spades
DRIVERNV(foathens)      // 12/78 Flame of Athens
DRIVERNV(disco79)       // 06/79 Disco '79
DRIVERNV(starshot)      // 12/79 Star Shooter

// --------------------
// ALVIN G. AND CO.
// --------------------
DRIVERNV(agsocc07)      // AG01:   07/92  A.G. Soccer-Ball (R07u)
DRIVERNV(agsoccer)      //         04/93  A.G. Soccer-Ball (R18u)
DRIVERNV(agfoot07)      // AG01:   07/92  A.G. Football (R07u)
DRIVERNV(agfootbl)      //         04/93  A.G. Football (R18u)
DRIVERNV(wrldtou3)      // AG03:   02/93  Al's Garage Band Goes On a World Tour (R06a)
DRIVERNV(wrldtou2)      //         02/93  Al's Garage Band Goes On a World Tour (R02b)
DRIVERNV(wrldtour)      //         03/93  Al's Garage Band Goes On a World Tour (R01c)
DRIVERNV(usafoota)      // AG05:   09/92  U.S.A. Football (R01u)
DRIVERNV(usafootb)      //         02/93  U.S.A. Football (R06u)
DRIVERNV(punchy)        // EPC061: 08/93  Punchy the Clown (R02)
DRIVERNV(punchy3)       //         10/93  Punchy the Clown (R03)
DRIVERNV(dinoeggs)      // EPC071: 09/93  Dinosaur Eggs (R02)
DRIVERNV(mystcast)      // AG08:   06/93  Mystery Castle (R02)
DRIVERNV(mystcasa)      // EPC081: ??/9?  Mystery Castle (R03)
DRIVERNV(pstlpkr1)      // AG10:   11/93  Pistol Poker (R01)
DRIVERNV(pstlpkr)       //         11/93  Pistol Poker (R02)
                        //         ??/93  Dual-Pool - proto
                        //         ??/93  Slam 'N Jam - proto
                        //         ??/94  A-MAZE-ING Baseball - proto
#ifdef MAME_DEBUG
DRIVERNV(test8031)      //Test 8031 cpu core
#endif

// --------------------
// APPLE TIME
// --------------------
DRIVERNV(thndrman)      //Thunder Man (Zaccaria hardware)

// ---------------
// ASTILL ENTERTAINMENT
// ---------------
DRIVER(rush,10)         //Rush (custom pin, based on WPC fliptronics hardware)

// ---------------
// ASTRO GAMES
// ---------------
DRIVERNV(blkshpsq)      //Black Sheep Squadron (1979) - using old Stern hardware

// ---------------
// ATARI GAMES
// ---------------
                        //Triangle (Prototype, 1976?)
DRIVERNV(atarians)      //Atarians, The (November 1976)
DRIVERNV(atarianb)      //Atarians, The (2002 bootleg)
DRIVERNV(time2000)      //Time 2000 (June 1977)
DRIVERNV(aavenger)      //Airborne Avenger (September 1977)
DRIVERNV(midearth)      //Middle Earth (February 1978)
DRIVERNV(spcrider)      //Space Riders (September 1978)
DRIVERNV(superman)      //Superman (March 1979)
DRIVERNV(hercules)      //Hercules (May 1979)
DRIVERNV(roadrunr)      //Road Runner (Prototype, 1979)
                        //Monza (Prototype, 1980)
                        //Neutron Star (Prototype, 1981)
DRIVERNV(fourx4)        //4x4 (Prototype, 1982)

// ---------------
// BALLY GAMES
// ---------------
//S2650 hardware
DRIVERNV(cntinntl)      //          10/80 Continental (Bingo)
DRIVERNV(cntintl2)      //          10/80 Continental (Bingo, alternate version)

//Boomerang prototype
DRIVERNV(boomrang)      //          11/75 Boomerang (Engineering prototype)
DRIVERNV(boomranb)      //          04/17 Boomerang (MOD with code fixes)

//MPU-17
DRIVERNV(bowarrow)      //          08/76 Bow & Arrow (Prototype, rev. 23)
DRIVERNV(bowarroa)      //          08/76 Bow & Arrow (Prototype, rev. 22)
DRIVERNV(freedom )      //BY17-720: 12/76 Freedom
DRIVERNV(freedoma)      //BY17      02/19 Freedom (Free Play+ rev. 07)
DRIVERNV(freedomb)      //BY17      01/19 Freedom (Free Play+ rev. 20)
DRIVERNV(nightr20)      //BY17-721: 02/77 Night Rider (rev. 20)
DRIVERNV(nightrdr)      //BY17      02/77 Night Rider (rev. 21)
DRIVERNV(nightrdb)      //BY17      10/08 Night Rider (Free Play)
DRIVERNV(nightrdc)      //BY17      02/19 Night Rider (Free Play+)
DRIVERNV(evelknie)      //BY17-722: 06/77 Evel Knievel
DRIVERNV(evelknib)      //BY17      10/08 Evel Knievel (Free Play)
DRIVERNV(evelknic)      //BY17      01/19 Evel Knievel (Free Play+)
DRIVERNV(eightbll)      //BY17-723: 09/77 Eight Ball
DRIVERNV(eightblo)      //BY17      09/77 Eight Ball (Old)
DRIVERNV(eightblb)      //BY17      10/08 Eight Ball (Free Play)
DRIVERNV(eightblc)      //BY17      12/18 Eight Ball (Free Play+)
DRIVERNV(pwerplay)      //BY17-724: 01/78 Power Play
DRIVERNV(pwerplab)      //BY17      10/08 Power Play (Free Play)
DRIVERNV(pwerplac)      //BY17      12/18 Power Play (Free Play+)
DRIVERNV(matahari)      //BY17-725: 04/78 Mata Hari
DRIVERNV(matatest)      //BY17      ??/06 Mata Hari (New game rules)
DRIVERNV(mataharb)      //BY17      10/08 Mata Hari (Free Play)
DRIVERNV(mataharc)      //BY17      02/19 Mata Hari (Free Play+)
DRIVERNV(blackjck)      //BY17-728: 06/78 Black Jack
DRIVERNV(blackjcb)      //BY17      10/08 Black Jack (Free Play)
DRIVERNV(blackjcc)      //BY17      04/18 Black Jack (Saucer points modification)
DRIVERNV(blackjcd)      //BY17      12/18 Black Jack (Free Play+)
DRIVERNV(stk_sprs)      //BY17-740: 06/78 Strikes and Spares
DRIVERNV(stk_sprb)      //BY17      10/08 Strikes and Spares (Free Play)
DRIVERNV(stk_sprc)      //BY17      12/18 Strikes and Spares (Free Play+)
//MPU-35
DRIVERNV(lostwrld)      //BY35-729: 08/78 Lost World
DRIVERNV(lostwldb)      //BY35      10/08 Lost World (Free Play)
DRIVERNV(smman   )      //BY35-742: 10/78 Six Million Dollar Man, The
DRIVERNV(smmanb  )      //BY35      05/05 Six Million Dollar Man, The (7-digit conversion)
DRIVERNV(smmanc  )      //BY35      10/08 Six Million Dollar Man, The (7-digit Rev3 Free Play)
DRIVERNV(smmand  )      //BY35      10/08 Six Million Dollar Man, The (/10 Free Play)
DRIVERNV(playboy )      //BY35-743: 12/78 Playboy
DRIVERNV(playboyb)      //BY35      05/05 Playboy (7-digit conversion)
DRIVERNV(playboyc)      //BY35      10/08 Playboy (7-digit Rev3 Free Play)
DRIVERNV(playboyd)      //BY35      10/08 Playboy (/10 Free Play)
DRIVERNV(voltan  )      //BY35-744: 02/79 Voltan Escapes Cosmic Doom
DRIVERNV(voltanb )      //BY35      05/05 Voltan Escapes Cosmic Doom (7-digit conversion)
DRIVERNV(voltanc )      //BY35      10/08 Voltan Escapes Cosmic Doom (7-digit Rev3 Free Play)
DRIVERNV(voltand )      //BY35      10/08 Voltan Escapes Cosmic Doom (/10 Free Play)
DRIVERNV(sst     )      //BY35-741: 03/79 Supersonic
DRIVERNV(sstb    )      //BY35      05/05 Supersonic (7-digit conversion)
DRIVERNV(sstc    )      //BY35      10/08 Supersonic (7-digit Rev3 Free Play)
DRIVERNV(sstd    )      //BY35      10/08 Supersonic (/10 Free Play)
DRIVERNV(startrek)      //BY35-745: 04/79 Star Trek
DRIVERNV(startreb)      //BY35      05/05 Star Trek (7-digit conversion)
DRIVERNV(startrec)      //BY35      10/08 Star Trek (7-digit Rev3 Free Play)
DRIVERNV(startred)      //BY35      10/08 Star Trek (/10 Free Play)
DRIVERNV(kiss    )      //BY35-746: 06/79 Kiss
DRIVERNV(kissb   )      //BY35      05/05 Kiss (7-digit conversion)
DRIVERNV(kissc   )      //BY35      10/08 Kiss (7-digit Rev3 Free Play)
DRIVERNV(kissd   )      //BY35      10/08 Kiss (/10 Free Play)
DRIVERNV(kissp   )      //BY35      ??/?? Kiss (prototype)
DRIVERNV(kissp2  )      //BY35      ??/?? Kiss (prototype v.2)
DRIVERNV(paragon )      //BY35-748: 06/79 Paragon
DRIVERNV(paragonb)      //BY35      05/05 Paragon (7-digit conversion)
DRIVERNV(paragonc)      //BY35      10/08 Paragon (7-digit Rev3 Free Play)
DRIVERNV(paragond)      //BY35      10/08 Paragon (/10 Free Play)
DRIVERNV(hglbtrtr)      //BY35-750: 09/79 Harlem Globetrotters On Tour
DRIVERNV(hglbtrtb)      //BY35      11/02 Harlem Globetrotters On Tour (7-digit conversion)
DRIVERNV(dollyptn)      //BY35-777: 11/79 Dolly Parton
DRIVERNV(dollyptb)      //BY35      11/02 Dolly Parton (7-digit conversion)
DRIVERNV(futurspa)      //BY35-781: 12/79 Future Spa
DRIVERNV(futurspb)      //BY35      11/02 Future Spa (7-digit conversion)
DRIVERNV(ngndshkr)      //BY35-776: 01/80 Nitro Ground Shaker
DRIVERNV(ngndshkb)      //BY35      11/02 Nitro Ground Shaker (7-digit conversion)
DRIVERNV(slbmania)      //BY35-786: 02/80 Silverball Mania
DRIVERNV(slbmanib)      //BY35      11/02 Silverball Mania (7-digit conversion)
DRIVERNV(spaceinv)      //BY35-792: 04/80 Space Invaders
DRIVERNV(spaceinb)      //BY35      11/02 Space Invaders (7-digit conversion)
DRIVERNV(rollston)      //BY35-796: 05/80 Rolling Stones
DRIVERNV(rollstob)      //BY35      11/02 Rolling Stones (7-digit conversion)
DRIVERNV(mystic  )      //BY35-798: 06/80 Mystic
DRIVERNV(mysticb )      //BY35      11/02 Mystic (7-digit conversion)
DRIVERNV(viking  )      //BY35-802: 07/80 Viking
DRIVERNV(vikingb )      //BY35      11/02 Viking (7-digit conversion)
DRIVERNV(hotdoggn)      //BY35-809: 07/80 Hotdoggin'
DRIVERNV(hotdogga)      //BY35      11/02 Hotdoggin' (Free Play)
DRIVERNV(hotdoggb)      //BY35      11/02 Hotdoggin' (7-digit conversion)
DRIVERNV(skatebll)      //BY35-823: 09/80 Skateball
DRIVERNV(skatebla)      //BY35      10/08 Skateball (Free Play)
DRIVERNV(skateblb)      //BY35      09/05 Skateball (rev. 3)
DRIVERNV(frontier)      //BY35-819: 11/80 Frontier
DRIVERNV(frontiea)      //BY35      10/08 Frontier (Free Play)
DRIVERNV(frontieg)      //BY35      08/11 Frontier (Gate Fix)
DRIVERNV(xenon   )      //BY35-811: 11/80 Xenon
DRIVERNV(xenonf  )      //BY35      11/80 Xenon (French)
DRIVERNV(xenona  )      //BY35      10/08 Xenon (Free Play)
DRIVERNV(xenonfa )      //BY35      10/08 Xenon (French Free Play)
DRIVERNV(flashgdn)      //BY35-834: 02/81 Flash Gordon
DRIVERNV(flashgdv)      //BY35      02/81 Flash Gordon (Vocalizer sound)
DRIVERNV(flashgdf)      //BY35      02/81 Flash Gordon (French)
DRIVERNV(flashgvf)      //BY35      02/81 Flash Gordon (French Vocalizer sound)
DRIVERNV(flashgda)      //BY35      10/08 Flash Gordon (Free Play)
DRIVERNV(flashgva)      //BY35      10/08 Flash Gordon (Vocalizer sound Free Play)
DRIVERNV(flashgfa)      //BY35      10/08 Flash Gordon (French Free Play)
DRIVERNV(flashgvffp)    //BY35      10/08 Flash Gordon (French Vocalizer sound Free Play)
DRIVERNV(flashgdp)      //BY35      ??/8? Flash Gordon (68701 hardware prototype)
DRIVERNV(flashgp2)      //BY35      ??/8? Flash Gordon (6801 hardware prototype)
DRIVERNV(eballd14)      //BY35-838: 04/81 Eight Ball Deluxe (rev. 14)
DRIVERNV(eballdlx)      //BY35      02/05 Eight Ball Deluxe (rev. 15)
DRIVERNV(eballdla)      //BY35      10/08 Eight Ball Deluxe (Free Play)
DRIVERNV(eballdlb)      //BY35      02/07 Eight Ball Deluxe (modified rules rev.29)
DRIVERNV(eballdlc)      //BY35      10/07 Eight Ball Deluxe (modified rules rev.32)
DRIVERNV(eballdld)      //BY35      02/19 Eight Ball Deluxe (P2/4 Bonus Bugfix)
DRIVERNV(eballdp1)      //BY35      ??/81 Eight Ball Deluxe (68701 hardware prototype, rev. 1)
DRIVERNV(eballdp2)      //BY35      ??/81 Eight Ball Deluxe (68701 hardware prototype, rev. 2)
DRIVERNV(eballdp3)      //BY35      ??/81 Eight Ball Deluxe (68701 hardware prototype, rev. 3)
DRIVERNV(eballdp4)      //BY35      ??/81 Eight Ball Deluxe (68701 hardware prototype, rev. 4)
DRIVERNV(fball_ii)      //BY35-839: 06/81 Fireball II
DRIVERNV(fball_ia)      //BY35      10/08 Fireball II (Free Play)
DRIVERNV(embryon )      //BY35-841: 06/81 Embryon
DRIVERNV(embryona)      //BY35      10/08 Embryon (Free Play)
DRIVERNV(embryonb)      //BY35      10/02 Embryon (7-digit conversion rev.1)
DRIVERNV(embryonc)      //BY35      05/04 Embryon (7-digit conversion rev.8)
DRIVERNV(embryond)      //BY35      10/04 Embryon (7-digit conversion rev.9)
DRIVERNV(embryone)      //BY35      08/11 Embryon (7-digit conversion rev.92)
DRIVERNV(fathom  )      //BY35-842: 08/81 Fathom
DRIVERNV(fathoma )      //BY35      10/04 Fathom (Free Play)
DRIVERNV(fathomb )      //BY35      10/04 Fathom (modified rules)
DRIVERNV(medusa  )      //BY35-845: 09/81 Medusa
DRIVERNV(medusaa )      //BY35      10/04 Medusa (Free Play)
#ifdef MAME_DEBUG
DRIVERNV(medusaf )      //BY35      02/81 Medusa (6802 board)
#endif
DRIVERNV(centaur )      //BY35-848: 10/81 Centaur
DRIVERNV(centaura)      //BY35      10/04 Centaur (Free Play)
DRIVERNV(centaurb)      //BY35      10/08 Centaur (Free Play Rev. 27)
DRIVERNV(elektra )      //BY35-857: 12/81 Elektra
DRIVERNV(elektraa)      //BY35      10/04 Elektra (Free Play)
DRIVERNV(vector  )      //BY35-858: 02/82 Vector
DRIVERNV(vectora )      //BY35      10/04 Vector (Free Play)
DRIVERNV(vector4 )      //BY35      04/04 Vector (modified rules rev. 4)
DRIVERNV(vectorb )      //BY35      10/04 Vector (modified rules rev. 5)
DRIVERNV(vectorc )      //BY35      02/08 Vector (modified rules rev. 10)
DRIVERNV(rapidfir)      //BY35-869: 04/82 Rapid Fire
DRIVERNV(rapidfia)      //BY35      09/05 Rapid Fire (Free Play)
DRIVERNV(m_mpac  )      //BY35-872: 04/82 Mr. & Mrs. Pac-Man Pinball
DRIVERNV(m_mpaca )      //BY35      10/04 Mr. & Mrs. Pac-Man Pinball (Free Play)
DRIVERNV(spectrum)      //BY35-868: 06/82 Spectrum
DRIVERNV(spectru4)      //BY35      06/82 Spectrum (Rev. 4)
DRIVERNV(spectrua)      //BY35      10/04 Spectrum (Free Play)
DRIVERNV(spectr4a)      //BY35      10/04 Spectrum (Rev. 4 Free Play)
/*same as eballdlx*/    //BY35      08/82 Eight Ball Deluxe Limited Edition
DRIVERNV(speakesy)      //BY35-877: 08/82 Speakeasy
DRIVERNV(speakes4)      //BY35      09/82 Speakeasy 4 (4 player)
DRIVERNV(speakesa)      //BY35      10/04 Speakeasy (Free Play)
DRIVERNV(speake4a)      //BY35      10/04 Speakeasy 4 (4 player Free Play)
DRIVERNV(babypac )      //BY35-891: 10/82 Baby Pac-Man
DRIVERNV(babypaca)      //BY35      07/83 Baby Pac-Man (Vidiot U9 ROM Update 891-16 per Service-Bulletin dated July 11 1983)
DRIVERNV(babypacn)      //BY35      06/06 Baby Pac-Man (Home Rom)
DRIVERNV(bmx     )      //BY35-888: 01/83 BMX
DRIVERNV(bmxa    )      //BY35      10/04 BMX (Free Play)
DRIVERNV(granslam)      //BY35-1311:01/83 Grand Slam
DRIVERNV(gransla4)      //BY35      03/83 Grand Slam (4 player)
DRIVERNV(granslaa)      //BY35      12/04 Grand Slam (Free Play)
DRIVERNV(gransl4a)      //BY35      12/04 Grand Slam (4 player Free Play)
/*same as centaur*/     //BY35      06/83 Centaur II
DRIVERNV(goldball)      //BY35-1371:10/83 Gold Ball
DRIVERNV(goldbaln)      //BY35      10/83 Gold Ball (alternate)
DRIVERNV(goldbalb)      //BY35      03/04 Gold Ball (7-digit conversion)
DRIVERNV(goldbalc)      //BY35      03/05 Gold Ball (6/7-digit alternate set rev.10)
DRIVERNV(granny )       //BY35-1369:01/84 Granny and the Gators
DRIVERNV(xsandos )      //BY35-1391:02/84 X's & O's
DRIVERNV(xsandosa)      //BY35      10/04 X's & O's (Free Play)
                        //??        ??/84 Mysterian
DRIVERNV(kosteel )      //BY35-1390:03/84 Kings of Steel
DRIVERNV(kosteela)      //BY35      10/04 Kings of Steel (Free Play)
DRIVERNV(mdntmrdr)      //          05/84 Midnight Marauders (gun game)
DRIVERNV(blakpyra)      //BY35-0A44 07/84 Black Pyramid
DRIVERNV(blakpyrb)      //BY35      10/04 Black Pyramid (Free Play)
DRIVERNV(spyhuntr)      //BY35-0A17:10/84 Spy Hunter
DRIVERNV(spyhunta)      //BY35      10/04 Spy Hunter (Free Play)
                        //??        ??/85 Hot Shotz
DRIVERNV(fbclass )      //BY35-0A40 02/85 Fireball Classic
DRIVERNV(fbclassa)      //BY35      10/04 Fireball Classic (Free Play)
DRIVERNV(cybrnaut)      //BY35-0B42 05/85 Cybernaut
DRIVERNV(cybrnaua)      //BY35      10/04 Cybernaut (Free Play)
DRIVERNV(bigbat)        //BY35-848: ??/84 Big Bat
//MPU-6803
DRIVERNV(eballchp)      //6803-0B38:08/85 Eight Ball Champ
DRIVERNV(eballch2)      //6803      08/85 Eight Ball Champ (cheap squeak)
DRIVERNV(beatclck)      //6803-0C70:10/85 Beat the Clock
DRIVERNV(beatclc2)      //          11/85 Beat the Clock (with flasher support)
DRIVERNV(ladyluck)      //6803-0E34:02/86 Lady Luck
DRIVERNV(motrdome)      //6803-0E14:05/86 MotorDome
DRIVERNV(motrdomg)      //6803-0E69:07/86 MotorDome (German)
                        //6803-????:06/86 Karate Fight (Prototype for Black Belt?)
DRIVERNV(blackblt)      //6803-0E52:07/86 Black Belt
DRIVERNV(blackbl2)      //6803      07/86 Black Belt (Squawk & Talk)
DRIVERNV(specforc)      //6803-0E47:08/86 Special Force
DRIVERNV(strngsci)      //6803-0E35:10/86 Strange Science
DRIVERNV(strngscg)      //6803      10/86 Strange Science (German)
DRIVERNV(cityslck)      //6803-0E79:03/87 City Slicker
DRIVERNV(hardbody)      //6803-0E94:02/87 Hardbody
DRIVERNV(hardbdyg)      //6803      03/87 Hardbody (German)
DRIVERNV(prtyanim)      //6803-0H01:05/87 Party Animal
DRIVERNV(prtyanig)      //6803      05/87 Party Animal (German)
DRIVERNV(hvymetal)      //6803-0H03:08/87 Heavy Metal Meltdown
DRIVERNV(hvymetag)      //6803      08/87 Heavy Metal Meltdown (German)
DRIVERNV(dungdrag)      //6803-0H06:10/87 Dungeons & Dragons
DRIVERNV(esclwrld)      //6803-0H05:12/87 Escape from the Lost World
DRIVERNV(esclwrlg)      //6803      12/87 Escape from the Lost World (German)
DRIVERNV(black100)      //6803-0H07:03/88 Blackwater 100
DRIVERNV(black10s)      //6803      03/88 Blackwater 100 (Single Ball Play)
                        //??        06/88 Ramp Warrior (Became Truck Stop after Merger)
//Williams Merger begins here.. but these are still under the Bally name
DRIVERNV(trucksp2)      //6803-2001:11/88 Truck Stop (P-2)
DRIVERNV(trucksp3)      //6803      12/88 Truck Stop (P-3)
DRIVERNV(atlantis)      //6803-2006:03/89 Atlantis
                        //??        05/89 Ice Castle

// ---------------
// BARNI
// ---------------
                        // Shield (1985)
DRIVERNV(champion)      // Champion (1985)
DRIVERNV(redbaron)      // Red Baron (1985)
                        // Comando (1986)
                        // Saturno (1986)
                        // Fighter (1986)

// ------------------
// (NUOVA) BELL GAMES
// ------------------
// Bell Coin Matics
                        //          ??/78 The King
                        //BY35      ??/79 Sexy Girl (Bally Playboy clone with image projector)
                        //          ??/80 The Hunter (uses Bally Strikes & Spares ROMs) 
                        //          ??/80 White Shark
                        //          ??/80 Cosmodrome
// Bell Games
                        //          01/82 Magic Picture Pin
                        //BY35      ??/82 Fantasy (Bally Centaur clone)
                        //          02/83 Pinball (Zaccaria Pinball Champ '82 clone)
                        //          ??/8? Movie (Zaccaria Pinball Champ clone)
                        //BY35      12/83 Pin Ball Pool (Bally Eight Ball Deluxe clone)
DRIVERNV(suprbowl)      //BY35      06/84 Super Bowl (Bally X's & O's clone but no-clone driver)
DRIVERNV(sprbwlfp)      //BY35      03/18 Super Bowl (Free Play)
DRIVERNV(tigerrag)      //BY35      ??/84 Tiger Rag (Bally Kings Of Steel clone)
DRIVERNV(cosflash)      //BY35      ??/85 Cosmic Flash (ROMs almost the same as Bally Flash Gordon)
DRIVERNV(newwave)       //BY35      04/85 New Wave (Bally Black Pyramid clone)
DRIVERNV(saturn2)       //BY35      08/85 Saturn 2 (Bally Spy Hunter clone)
                        //          ??/?? World Cup / World Championship (redemption game)
// Nuova Bell Games
DRIVERNV(worlddef)      //BY35      11/85 World Defender (Laser Cue clone but no-clone driver)
DRIVERNV(worlddfp)      //BY35      11/85 World Defender (Free Play)
DRIVERNV(spacehaw)      //BY35      04/86 Space Hawks (Bally Cybernaut clone but no-clone driver)
DRIVERNV(spchawfp)      //BY35      04/86 Space Hawks (Free Play - dips 25 & 26 must be on!)
DRIVERNV(darkshad)      //BY35      ??/86 Dark Shadow
DRIVERNV(skflight)      //BY35      09/86 Skill Flight
DRIVERNV(cobra)         //BY35      02/87 Cobra
DRIVERNV(futrquen)      //BY35      07/87 Future Queen
DRIVERNV(f1gp)          //BY35ALPHA 12/87 F1 Grand Prix
DRIVERNV(toppin)        //BY35      01/88 Top Pin (WMS Pin*Bot conversion but no-clone driver)
DRIVERNV(uboat65)       //BY35ALPHA 04/88 U-Boat 65

// ----------------
// CAPCOM GAMES
// ----------------
DRIVERNV(ghv101)        // ??/95    Goofy Hoops (Romstar license)
DRIVERNV(pmv112)        // 10/95    Pinball Magic
DRIVERNV(pmv112r)       // 10/95    Pinball Magic (Redemption)
DRIVERNV(abv106)        // 03/96    Airborne
DRIVERNV(abv106r)       // 03/96    Airborne (Redemption)
DRIVERNV(bsv100r)       // 05/96    Breakshot (Redemption, 1.0)
DRIVERNV(bsv102r)       // 05/96    Breakshot (Redemption, 1.2)
DRIVERNV(bsv102)        // 05/96    Breakshot (1.2)
DRIVERNV(bsv103)        // 05/96    Breakshot (1.3)
DRIVERNV(bsb105)        // 05/96    Breakshot (Beta, 1.5)
DRIVERNV(ffv101)        // 10/96    Flipper Football (1.01)
DRIVERNV(ffv104)        // 10/96    Flipper Football (1.04)
DRIVERNV(bbb108)        // 11/96    Big Bang Bar (Beta 1.8)
DRIVERNV(bbb109)        // 11/96    Big Bang Bar (Beta 1.9)
DRIVERNV(kpb105)        // 12/96    Kingpin (Beta 1.05)

// ----------------
// CICPLAY GAMES
// ----------------
DRIVERNV(glxplay)       //Galaxy Play (1985)
DRIVERNV(kidnap)        //Kidnap (1986)
DRIVERNV(glxplay2)      //Galaxy Play 2 (1987)

// ---------------
// CIRSA GAMES
// ---------------
DRIVERNV(sport2k)       //Sport 2000 (1988)

// -------------------
// DATA EAST GAMES
// -------------------
//4 x 2 x 7 (mixed) + credits A/N Display
DRIVER(lwar,a81)        //Data East MPU: 05/87 Laser War (8.1)
DRIVER(lwar,a83)        //Data East MPU: 05/87 Laser War (8.3)
DRIVER(lwar,e90)        //Data East MPU: 05/87 Laser War (9.0 Europe)
//4 x 2 x 7 (mixed) A/N Display
DRIVER(ssvc,a26)        //Data East MPU: 03/88 Secret Service (2.6)
DRIVER(ssvc,b26)        //Data East MPU: 03/88 Secret Service (2.6 Alternate Sound)
DRIVER(ssvc,a42)        //Data East MPU: 03/88 Secret Service (4.2 Alternate Sound)
DRIVER(torp,e21)        //Data East MPU: 08/88 Torpedo Alley (2.1 Europe)
DRIVER(torp,a16)        //Data East MPU: 08/88 Torpedo Alley (1.6)
DRIVER(tmac,a18)        //Data East MPU: 12/88 Time Machine (1.8)
DRIVER(tmac,g18)        //Data East MPU: 12/88 Time Machine (1.8 German)
DRIVER(tmac,a24)        //Data East MPU: 12/88 Time Machine (2.4)
DRIVER(play,a24)        //Data East MPU: 05/89 Playboy 35th Anniversary
//2 x 16 A/N Display
DRIVER(mnfb,c27)        //Data East MPU: 09/89 ABC Monday Night Football (2.7, 50cts)
DRIVER(mnfb,c29)        //Data East MPU: 09/89 ABC Monday Night Football (2.9, 50cts)
DRIVER(robo,a29)        //Data East MPU: 11/89 Robocop (2.9)
DRIVER(robo,a30)        //Data East MPU: 11/89 Robocop (3.0)
DRIVER(robo,a34)        //Data East MPU: 11/89 Robocop (3.4)
DRIVER(poto,a32)        //Data East MPU: 01/90 Phantom of the Opera (3.2)
DRIVER(poto,a31)        //Data East MPU: 01/90 Phantom of the Opera (3.1)
DRIVER(poto,a29)        //Data East MPU: 01/90 Phantom of the Opera (2.9)
DRIVER(kiko,a10)        //Data East MPU: 01/90 King Kong (1.0)
DRIVER(bttf,a20)        //Data East MPU: 06/90 Back to the Future (2.0)
DRIVER(bttf,a21)        //Data East MPU: ??/90 Back to the Future (2.1)
DRIVER(bttf,a27)        //Data East MPU: 12/90 Back to the Future (2.7)
DRIVER(bttf,g27)        //Data East MPU: ??/9? Back to the Future (2.7 German)
DRIVER(bttf,a28)        //Data East MPU: 12/90 Back to the Future (2.8)
DRIVER(simp,a20)        //Data East MPU: 09/90 Simpsons, The (2.0)
DRIVER(simp,a27)        //Data East MPU: 09/90 Simpsons, The (2.7)
//DMD 128 x 16
DRIVER(ckpt,a17)        //Data East MPU: 02/91 Checkpoint
DRIVER(tmnt,103)        //Data East MPU: 05/91 Teenage Mutant Ninja Turtles (1.03)
DRIVER(tmnt,104)        //Data East MPU: 05/91 Teenage Mutant Ninja Turtles (1.04)
DRIVER(tmnt,104g)       //Data East MPU: 05/91 Teenage Mutant Ninja Turtles (1.04 German)
//BSMT2000 Sound chip
DRIVER(btmn,101)        //Data East MPU: 07/91 Batman (1.01)
DRIVER(btmn,103)        //Data East MPU: ??/91 Batman (1.03)
DRIVER(btmn,f13)        //Data East MPU: 09/91 Batman (1.03 French)
DRIVER(btmn,g13)        //Data East MPU: ??/91 Batman (1.03 German)
DRIVER(btmn,106)        //Data East MPU: ??/91 Batman (1.06)
DRIVER(trek,11a)        //Data East MPU: 11/91 Star Trek 25th Anniversary (1.10 Alpha Display)
DRIVER(trek,110)        //Data East MPU: 11/91 Star Trek 25th Anniversary (1.10)
DRIVER(trek,117)        //Data East MPU: 01/92 Star Trek 25th Anniversary (1.17)
DRIVER(trek,120)        //Data East MPU: 01/92 Star Trek 25th Anniversary (1.20)
DRIVER(trek,200)        //Data East MPU: 04/92 Star Trek 25th Anniversary (2.00)
DRIVER(trek,201)        //Data East MPU: 04/92 Star Trek 25th Anniversary (2.01)
DRIVER(trek,300)        //Data East MPU: 04/20 Star Trek 25th Anniversary (3.00 unofficial MOD)
DRIVER(hook,401p)       //Data East MPU: 11/91 Hook (4.01 with Prototype Sound)
DRIVER(hook,401)        //Data East MPU: 01/92 Hook (4.01)
DRIVER(hook,404)        //Data East MPU: 01/92 Hook (4.04)
DRIVER(hook,408)        //Data East MPU: 01/92 Hook (4.08)
DRIVER(hook,500)        //Data East MPU: 12/16 Hook (5.00 unofficial MOD)
DRIVER(hook,501)        //Data East MPU: 03/18 Hook (5.01 unofficial MOD)
//DMD 128 x 32
DRIVER(lw3,200)         //Data East MPU: 06/92 Lethal Weapon (2.00)
DRIVER(lw3,203)         //Data East MPU: 07/92 Lethal Weapon (2.03)
DRIVER(lw3,205)         //Data East MPU: 07/92 Lethal Weapon (2.05)
DRIVER(lw3,207)         //Data East MPU: 08/92 Lethal Weapon (2.07 Canadian)
DRIVER(lw3,208)         //Data East MPU: 11/92 Lethal Weapon (2.08)
DRIVER(lw3,208p)        //Data East MPU: 04/13 Lethal Weapon (2.08p, Voices Mod)
DRIVER(aar,101)         //Data East MPU: 12/92 Aaron Spelling (1.01)
DRIVER(stwr,101)        //Data East MPU: 10/92 Star Wars (1.01)
DRIVER(stwr,g11)        //Data East MPU: 10/92 Star Wars (1.01 German)
DRIVER(stwr,102)        //Data East MPU: 11/92 Star Wars (1.02)
DRIVER(stwr,e12)        //Data East MPU: 11/92 Star Wars (1.02 English)
DRIVER(stwr,a046)       //Data East MPU: 10/92 Star Wars (1.03, Display A0.46)
DRIVER(stwr,a14)        //Data East MPU: 12/92 Star Wars (1.03, Display 1.04)
DRIVER(stwr,103)        //Data East MPU: 01/93 Star Wars (1.03, Display 1.05)
DRIVER(stwr,104)        //Data East MPU: 12/12 Star Wars (1.04)
DRIVER(stwr,106)        //Data East MPU: 05/16 Star Wars (1.06)
DRIVER(stwr,107)        //Data East MPU: 12/16 Star Wars (1.07)
DRIVER(stwr,107s)       //Data East MPU: 12/16 Star Wars (1.07 Spanish)
DRIVER(rab,103)         //Data East MPU: 02/93 Adventures of Rocky and Bullwinkle and Friends, The (1.03 Spanish)
DRIVER(rab,130)         //Data East MPU: 04/93 Adventures of Rocky and Bullwinkle and Friends, The (1.30)
DRIVER(rab,320)         //Data East MPU: 08/93 Adventures of Rocky and Bullwinkle and Friends, The (3.20)
DRIVER(jupk,501)        //Data East MPU: 09/93 Jurassic Park (5.01)
DRIVER(jupk,g51)        //Data East MPU: 09/93 Jurassic Park (5.01 German)
DRIVER(jupk,513)        //Data East MPU: 09/93 Jurassic Park (5.13)
DRIVER(jupk,600)        //Data East MPU: 10/15 Jurassic Park (6.00 unofficial MOD)
DRIVER(lah,l104)        //Data East MPU: 09/93 Last Action Hero (1.04 Spanish)
DRIVER(lah,c106)        //Data East MPU: 09/93 Last Action Hero (1.06 Canadian)
DRIVER(lah,l108)        //Data East MPU: 09/93 Last Action Hero (1.08 Spanish)
DRIVER(lah,110)         //Data East MPU: 10/93 Last Action Hero (1.10)
DRIVER(lah,112)         //Data East MPU: 11/93 Last Action Hero (1.12)
DRIVER(lah,113)         //Data East MPU: 11/14 Last Action Hero (1.13 unofficial MOD)
DRIVER(tftc,104)        //Data East MPU: 11/93 Tales From the Crypt (1.04 Spanish)
DRIVER(tftc,200)        //Data East MPU: 11/93 Tales From the Crypt (2.00)
DRIVER(tftc,300)        //Data East MPU: 11/93 Tales From the Crypt (3.00)
DRIVER(tftc,302)        //Data East MPU: 11/93 Tales From the Crypt (3.02 Dutch)
DRIVER(tftc,303)        //Data East MPU: 11/93 Tales From the Crypt (3.03)
DRIVER(tftc,400)        //Data East MPU: 10/15 Tales From the Crypt (4.00 unofficial MOD)
DRIVER(tomy,102)        //Data East MPU: 02/94 Tommy (1.02)
DRIVER(tomy,h30)        //Data East MPU: 02/94 Tommy (3.00 Dutch)
DRIVER(tomy,400)        //Data East MPU: 02/94 Tommy (4.00)
DRIVER(tomy,500)        //Data East MPU: 05/16 Tommy (5.00 unofficial MOD)
DRIVER(wwfr,103)        //Data East MPU: 05/94 WWF Royal Rumble (1.03)
DRIVER(wwfr,103f)       //Data East MPU: 05/94 WWF Royal Rumble (1.03 French)
DRIVER(wwfr,106)        //Data East MPU: 08/94 WWF Royal Rumble (1.06)
DRIVER(gnr,300)         //Data East MPU: 07/94 Guns N' Roses (3.00)
DRIVER(gnr,300f)        //Data East MPU: 07/94 Guns N' Roses (3.00 French)
DRIVER(gnr,300d)        //Data East MPU: 07/94 Guns N' Roses (3.00 Dutch)
//MISC
DRIVERNV(detest)        //Data East MPU: ??/?? DE Test Chip

// --------------------
// FASCINATION INTERNATIONAL, INC.
// --------------------
DRIVERNV(royclark)      // 09/77 Roy Clark - The Entertainer
DRIVERNV(circa33)       // 02/79 Circa 1933
DRIVERNV(erosone)       // 03/79 Eros One

// -------------------
// GAME PLAN GAMES
// -------------------
/*Games below are Cocktail #110 Model*/
DRIVERNV(foxylady)      //Foxy Lady (May 1978)
DRIVERNV(blvelvet)      //Black Velvet (May 1978)
DRIVERNV(camlight)      //Camel Lights (May 1978)
DRIVERNV(real)          //Real (May 1978)
DRIVERNV(rio)           //Rio (?? / 1978)
DRIVERNV(chucklck)      //Chuck-A-Luck (October 1978)
/*Games below are Cocktail #120 Model*/
DRIVERNV(startrip)      //Star Trip (April 1979)
DRIVERNV(famlyfun)      //Family Fun! (April 1979)
/*Games below are regular standup pinball games*/
DRIVERNV(sshooter)      //Sharpshooter (May 1979)
DRIVERNV(vegasgp)       //Vegas (August 1979)
DRIVERNV(coneyis)       //Coney Island! (December 1979)
DRIVERNV(lizard)        //Lizard (July 1980)
DRIVERNV(gwarfare)      //Global Warfare (June 1981)
DRIVERNV(mbossy)        //Mike Bossy - The Scoring Machine (January 1982)
DRIVERNV(suprnova)      //Super Nova (May 1982)
DRIVERNV(sshootr2)      //Sharp Shooter II (November 1983)
DRIVERNV(attila)        //Attila the Hun (April 1984)
DRIVERNV(agent777)      //Agents 777 (November 1984)
DRIVERNV(cpthook)       //Captain Hook (April 1985)
DRIVERNV(ladyshot)      //Lady Sharpshooter (May 1985)
DRIVERNV(ldyshot2)      //Lady Sharpshooter (May 1985, alternate set)
DRIVERNV(andromed)      //Andromeda (August 1985)
DRIVERNV(andromea)      //Andromeda (alternate set)
DRIVERNV(cyclope1)      //Cyclopes (November 1985)
DRIVERNV(cyclopes)      //Cyclopes (December 1985)
                        //Loch Ness Monster (November 1985)

// ------------------
// GOTTLIEB GAMES
// ------------------
//System 1
DRIVERNV(cleoptra)      //S1-409    11/77 Cleopatra
DRIVERNV(sinbad)        //S1-412    05/78 Sinbad
DRIVERNV(sinbadn)       //S1-412NO1 05/78 Sinbad (Norway)
DRIVERNV(jokrpokr)      //S1-417    08/78 Joker Poker
DRIVERNV(dragon)        //S1-419    10/78 Dragon
DRIVERNV(closeenc)      //S1-424    10/78 Close Encounters of the Third Kind
DRIVERNV(charlies)      //S1-425    11/78 Charlie's Angels
DRIVERNV(solaride)      //S1-421    02/79 Solar Ride
DRIVERNV(countdwn)      //S1-422    05/79 Count-Down
DRIVERNV(pinpool)       //S1-427    08/79 Pinball Pool
DRIVERNV(totem)         //S1-429    10/79 Totem
DRIVERNV(hulk)          //S1-433    10/79 Incredible Hulk
DRIVERNV(genie)         //S1-435    11/79 Genie
DRIVERNV(buckrgrs)      //S1-437    01/80 Buck Rogers
DRIVERNV(torch)         //S1-438    02/80 Torch
DRIVERNV(roldisco)      //S1-440    02/80 Roller Disco
DRIVERNV(astannie)      //S1-442    12/80 Asteroid Annie and the Aliens
DRIVERNV(sys1test)      //S1-T      ??    System1 test prom
//System 80
DRIVERNV(spidermn)      //S80-653:  05/80 Amazing Spider-Man, The
DRIVERNV(spiderm7)      //          01/08 Amazing Spider-Man, The (7-digit conversion)
DRIVERNV(panthera)      //S80-652:  06/80 Panthera
DRIVERNV(panther7)      //          01/08 Panthera (7-digit conversion)
DRIVERNV(circus)        //S80-654:  06/80 Circus
DRIVERNV(circus7)       //          01/08 Circus (7-digit conversion)
DRIVERNV(cntforce)      //S80-656:  08/80 Counterforce
DRIVERNV(cntforc7)      //          01/08 Counterforce (7-digit conversion)
DRIVERNV(starrace)      //S80-657:  10/80 Star Race
DRIVERNV(starrac7)      //          01/08 Star Race (7-digit conversion)
DRIVERNV(jamesb)        //S80-658:  10/80 James Bond (Timed Play)
DRIVERNV(jamesb7)       //          01/08 James Bond (Timed Play, 7-digit conversion)
DRIVERNV(jamesb2)       //                James Bond (3/5 Ball)
DRIVERNV(jamesb7b)      //          01/08 James Bond (3/5 Ball, 7-digit conversion)
DRIVERNV(timeline)      //S80-659:  10/80 Time Line
DRIVERNV(timelin7)      //          01/08 Time Line (7-digit conversion)
DRIVERNV(forceii)       //S80-661:  02/81 Force II
DRIVERNV(forceii7)      //          01/08 Force II (7-digit conversion)
DRIVERNV(pnkpnthr)      //S80-664:  03/81 Pink Panther
DRIVERNV(pnkpntr7)      //          01/08 Pink Panther (7-digit conversion)
DRIVERNV(pnkpntrs)      //          03/81 Pink Panther (sound correction fix)
DRIVERNV(mars)          //S80-666:  03/81 Mars God of War
DRIVERNV(marsp)         //          ??/?? Mars God of War (Prototype)
DRIVERNV(marsf)         //          03/81 Mars God of War (French Speech)
DRIVERNV(mars7)         //          01/08 Mars God of War (7-digit conversion)
DRIVERNV(mars_2)        //          03/81 Mars God of War (rev. 2 unofficial MOD) // fixes a potential startup problem
DRIVERNV(vlcno_ax)      //S80-667:  09/81 Volcano (Sound & Speech)
DRIVERNV(vlcno_a7)      //          01/08 Volcano (Sound & Speech, 7-digit conversion)
DRIVERNV(vlcno_1b)      //                Volcano (Sound Only)
DRIVERNV(vlcno_b7)      //          01/08 Volcano (Sound Only, 7-digit conversion)
DRIVERNV(vlcno_1a)      //                Volcano (Sound Only, alternate version)
DRIVERNV(vlcno_1c)      //                Volcano (Sound Only, alternate version 2)
DRIVERNV(blckhole)      //S80-668:  10/81 Black Hole (Sound & Speech, Rev 4)
DRIVERNV(blkhole7)      //          01/08 Black Hole (Sound & Speech, Rev 4, 7-digit conversion)
DRIVERNV(blkhole2)      //                Black Hole (Sound & Speech, Rev 2)
DRIVERNV(blkholea)      //                Black Hole (Sound Only)
DRIVERNV(blkhol7s)      //          01/08 Black Hole (Sound Only, 7-digit conversion)
DRIVERNV(hh)            //S80-669:  06/82 Haunted House (rev. 2)
DRIVERNV(hh7)           //          01/08 Haunted House (rev. 2, 7-digit conversion)
DRIVERNV(hh_1)          //                Haunted House (rev. 1)
DRIVERNV(hh_3)          //                Haunted House (rev. 3 unofficial MOD)
DRIVERNV(hh_3a)         //                Haunted House (rev. 3 unofficial MOD, LED)
DRIVERNV(hh_3b)         //                Haunted House (rev. 3 unofficial MOD, LED+Secret Tunnel)
DRIVERNV(hh_4)          //                Haunted House (rev. 4 unofficial Votrax speech MOD)
DRIVERNV(hh_4a)         //                Haunted House (rev. 4 unofficial Votrax speech MOD, LED)
DRIVERNV(hh_4b)         //                Haunted House (rev. 4 unofficial Votrax speech MOD, LED+Secret Tunnel)
DRIVERNV(eclipse)       //S80-671:  ??/82 Eclipse
DRIVERNV(eclipse7)      //          01/08 Eclipse (7-digit conversion)
DRIVERNV(s80tst)        //S80: Text Fixture
//System 80a
DRIVERNV(dvlsdre)       //S80a-670: 08/82 Devil's Dare (Sound & Speech)
DRIVERNV(dvlsdre2)      //                Devil's Dare (Sound Only)
DRIVERNV(caveman)       //S80a-PV810:09/82 Caveman
DRIVERNV(cavemana)      //                Caveman (set 2)
DRIVERNV(cavemane)      //                Caveman (Evolution, unofficial MOD)
DRIVERNV(rocky)         //S80a-672: 09/82 Rocky
DRIVERNV(rockyf)        //                Rocky (French Speech)
DRIVERNV(spirit)        //S80a-673: 11/82 Spirit
DRIVERNV(striker)       //S80a-675: 11/82 Striker
DRIVERNV(punk)          //S80a-674: 12/82 Punk!
DRIVERNV(krull)         //S80a-676: 02/83 Krull
DRIVERNV(goinnuts)      //S80a-682: 02/83 Goin' Nuts
DRIVERNV(qbquest)       //S80a-677: 03/83 Q*bert's Quest
DRIVERNV(sorbit)        //S80a-680: 05/83 Super Orbit
DRIVERNV(rflshdlx)      //S80a-681: 06/83 Royal Flush Deluxe
DRIVERNV(amazonh)       //S80a-684: 09/83 Amazon Hunt
DRIVERNV(amazonha)      //                Amazon Hunt (alternate set)
DRIVERNV(rackemup)      //S80a-685: 11/83 Rack 'Em Up
DRIVERNV(raimfire)      //S80a-686: 11/83 Ready Aim Fire
DRIVERNV(jack2opn)      //S80a-687: 05/84 Jacks to Open
DRIVERNV(alienstr)      //S80a-689: 06/84 Alien Star
DRIVERNV(thegames)      //S80a-691: 08/84 Games, The
DRIVERNV(eldorado)      //S80a-692: 09/84 El Dorado City of Gold
DRIVERNV(touchdn)       //S80a-688: 10/84 Touchdown
DRIVERNV(icefever)      //S80a-695: 02/85 Ice Fever
//System 80b
DRIVERNV(triplay)       //S80b-696: 05/85 Chicago Cubs Triple Play
DRIVERNV(triplyfp)      //                Chicago Cubs Triple Play (Free Play)
DRIVERNV(triplaya)      //                Chicago Cubs Triple Play (rev. 1)
DRIVERNV(triplyf1)      //                Chicago Cubs Triple Play (rev. 1 Free Play)
DRIVERNV(triplayg)      //                Chicago Cubs Triple Play (German)
DRIVERNV(triplgfp)      //                Chicago Cubs Triple Play (German Free Play)
DRIVERNV(bountyh)       //S80b-694: 07/85 Bounty Hunter
DRIVERNV(bounthfp)      //                Bounty Hunter (Free Play)
DRIVERNV(bountyhg)      //                Bounty Hunter (German)
DRIVERNV(bountgfp)      //                Bounty Hunter (German Free Play)
DRIVERNV(tagteam)       //S80b-698: 09/85 Tag-Team Pinball
DRIVERNV(tagtemfp)      //                Tag-Team Pinball (Free Play)
DRIVERNV(tagteamg)      //                Tag-Team Pinball (German)
DRIVERNV(tagtmgfp)      //                Tag-Team Pinball (German Free Play)
DRIVERNV(tagteam2)      //                Tag-Team Pinball (rev.2)
DRIVERNV(tagtem2f)      //                Tag-Team Pinball (rev.2 Free Play)
DRIVERNV(rock)          //S80b-697: 10/85 Rock
DRIVERNV(rockfp)        //                Rock (Free Play)
DRIVERNV(rockg)         //                Rock (German)
DRIVERNV(rockgfp)       //                Rock (German Free Play)
                        //S80b-700: ??/85 Ace High (never produced, playable whitewood exists)
DRIVERNV(s80btest)      //S80B: Text Fixture
DRIVERNV(raven)         //S80b-702: 03/86 Raven
DRIVERNV(ravenfp)       //                Raven (Free Play)
DRIVERNV(raveng)        //                Raven (German)
DRIVERNV(ravengfp)      //                Raven (German Free Play)
DRIVERNV(ravena)        //                Raven (alternate set)
DRIVERNV(ravenafp)      //                Raven (rev. 1 Free Play)
DRIVERNV(rambo)         //                Rambo (Raven unofficial MOD)
DRIVERNV(rock_enc)      //S80b-704: 04/86 Rock Encore
DRIVERNV(rock_efp)      //                Rock Encore (Free Play)
DRIVERNV(rock_eg)       //                Rock Encore (German)
DRIVERNV(rockegfp)      //                Rock Encore (German Free Play)
DRIVERNV(clash)         //                Clash, The (Rock Encore unofficial MOD)
DRIVERNV(hlywoodh)      //S80b-703: 06/86 Hollywood Heat
DRIVERNV(hlywdhfp)      //                Hollywood Heat (Free Play)
DRIVERNV(hlywodhg)      //                Hollywood Heat (German)
DRIVERNV(hlywhgfp)      //                Hollywood Heat (German Free Play)
DRIVERNV(hlywodhf)      //                Hollywood Heat (French)
DRIVERNV(hlywhffp)      //                Hollywood Heat (French Free Play)
DRIVERNV(bubba)         //                Bubba the Redneck Werewolf (Hollywood Heat unofficial MOD)
DRIVERNV(beachbms)      //                Beach Bums (Hollywood Heat unofficial MOD)
DRIVERNV(tomjerry)      //          01/19 Tom & Jerry (Hollywood Heat unofficial MOD)
DRIVERNV(genesis)       //S80b-705: 09/86 Genesis
DRIVERNV(genesifp)      //                Genesis (Free Play)
DRIVERNV(genesisg)      //                Genesis (German)
DRIVERNV(genesgfp)      //                Genesis (German Free Play)
DRIVERNV(genesisf)      //                Genesis (French)
DRIVERNV(genesffp)      //                Genesis (French Free Play)
DRIVERNV(goldwing)      //S80b-707: 10/86 Gold Wings
DRIVERNV(goldwgfp)      //                Gold Wings (Free Play)
DRIVERNV(gldwingg)      //                Gold Wings (German)
DRIVERNV(gldwggfp)      //                Gold Wings (German Free Play)
DRIVERNV(gldwingf)      //                Gold Wings (French)
DRIVERNV(gldwgffp)      //                Gold Wings (French Free Play)
DRIVERNV(mntecrlo)      //S80b-708: 02/87 Monte Carlo
DRIVERNV(mntecrfp)      //                Monte Carlo (Free Play)
DRIVERNV(mntecrlg)      //                Monte Carlo (German)
DRIVERNV(mntcrgfp)      //                Monte Carlo (German Free Play)
DRIVERNV(mntecrlf)      //                Monte Carlo (French)
DRIVERNV(mntcrffp)      //                Monte Carlo (French Free Play)
DRIVERNV(mntecrla)      //                Monte Carlo (alternate set)
DRIVERNV(mntcrafp)      //                Monte Carlo (rev.1 Free Play)
DRIVERNV(mntecrl2)      //                Monte Carlo (rev.2)
DRIVERNV(mntcr2fp)      //                Monte Carlo (rev.2 Free Play)
DRIVERNV(sprbreak)      //S80b-706: 04/87 Spring Break
DRIVERNV(sprbrkfp)      //                Spring Break (Free Play)
DRIVERNV(sprbrkg)       //                Spring Break (German)
DRIVERNV(sprbrgfp)      //                Spring Break (German Free Play)
DRIVERNV(sprbrkf)       //                Spring Break (French)
DRIVERNV(sprbrffp)      //                Spring Break (French Free Play)
DRIVERNV(sprbrka)       //                Spring Break (alternate set)
DRIVERNV(sprbrafp)      //                Spring Break (rev.1 Free Play)
DRIVERNV(sprbrks)       //                Spring Break (single ball game)
DRIVERNV(sprbrsfp)      //                Spring Break (single ball game, Free Play)
DRIVERNV(amazonh2)      //S80b-684C:05/87 Amazon Hunt II (French)
DRIVERNV(amazn2fp)      //                Amazon Hunt II (French Free Play)
DRIVERNV(arena)         //S80b-709: 06/87 Arena
DRIVERNV(arena_fp)      //                Arena (Free Play)
DRIVERNV(arenag)        //                Arena (German)
DRIVERNV(arenagfp)      //                Arena (German Free Play)
DRIVERNV(arenaf)        //                Arena (French)
DRIVERNV(arenaffp)      //                Arena (French Free Play)
DRIVERNV(arenaa)        //                Arena (alternate set)
DRIVERNV(arenaafp)      //                Arena (rev.1 Free Play)
DRIVERNV(victory)       //S80b-710: 10/87 Victory
DRIVERNV(victryfp)      //                Victory (Free Play)
DRIVERNV(victoryg)      //                Victory (German)
DRIVERNV(victrgfp)      //                Victory (German Free Play)
DRIVERNV(victoryf)      //                Victory (French)
DRIVERNV(victrffp)      //                Victory (French Free Play)
DRIVERNV(diamond)       //S80b-711: 02/88 Diamond Lady
DRIVERNV(diamonfp)      //                Diamond Lady (Free Play)
DRIVERNV(diamondg)      //                Diamond Lady (German)
DRIVERNV(diamngfp)      //                Diamond Lady (German Free Play)
DRIVERNV(diamondf)      //                Diamond Lady (French)
DRIVERNV(diamnffp)      //                Diamond Lady (French Free Play)
DRIVERNV(txsector)      //S80b-712: 03/88 TX-Sector
DRIVERNV(txsectfp)      //                TX-Sector (Free Play)
DRIVERNV(txsectrg)      //                TX-Sector (German)
DRIVERNV(txsecgfp)      //                TX-Sector (German Free Play)
DRIVERNV(txsectrf)      //                TX-Sector (French)
DRIVERNV(txsecffp)      //                TX-Sector (French Free Play)
DRIVERNV(robowars)      //S80b-714: 04/88 Robo-War
DRIVERNV(robowrfp)      //                Robo-War (Free Play)
DRIVERNV(robowarf)      //                Robo-War (French)
DRIVERNV(robowffp)      //                Robo-War (French Free Play)
DRIVERNV(badgirls)      //S80b-717: 11/88 Bad Girls
DRIVERNV(badgrlfp)      //                Bad Girls (Free Play)
DRIVERNV(badgirlg)      //                Bad Girls (German)
DRIVERNV(badgrgfp)      //                Bad Girls (German Free Play)
DRIVERNV(badgirlf)      //                Bad Girls (French)
DRIVERNV(badgrffp)      //                Bad Girls (French Free Play)
DRIVERNV(excaliba)      //S80b-715: 11/88 Excalibur
DRIVERNV(excalbfp)      //                Excalibur (Free Play)
DRIVERNV(excalibg)      //                Excalibur (German)
DRIVERNV(excalgfp)      //                Excalibur (German Free Play)
DRIVERNV(excalibr)      //                Excalibur (French)
DRIVERNV(excalffp)      //                Excalibur (French Free Play)
DRIVERNV(bighouse)      //S80b-713: 04/89 Big House
DRIVERNV(bighosfp)      //                Big House (Free Play)
DRIVERNV(bighousg)      //                Big House (German)
DRIVERNV(bighsgfp)      //                Big House (German Free Play)
DRIVERNV(bighousf)      //                Big House (French)
DRIVERNV(bighsffp)      //                Big House (French Free Play)
DRIVERNV(hotshots)      //S80b-718: 04/89 Hot Shots
DRIVERNV(hotshtfp)      //                Hot Shots (Free Play)
DRIVERNV(hotshotg)      //                Hot Shots (German)
DRIVERNV(hotshgfp)      //                Hot Shots (German Free Play)
DRIVERNV(hotshotf)      //                Hot Shots (French)
DRIVERNV(hotshffp)      //                Hot Shots (French Free Play)
DRIVERNV(bonebstr)      //S80b-719: 08/89 Bone Busters Inc.
DRIVERNV(bonebsfp)      //                Bone Busters Inc. (Free Play)
DRIVERNV(bonebstg)      //                Bone Busters Inc. (German)
DRIVERNV(bonebgfp)      //                Bone Busters Inc. (German Free Play)
DRIVERNV(bonebstf)      //                Bone Busters Inc. (French)
DRIVERNV(bonebffp)      //                Bone Busters Inc. (French Free Play)
DRIVERNV(nmoves)        //C-101:    11/89 Night Moves (for International Concepts)
DRIVERNV(nmovesfp)      //                Night Moves (Free Play) (for International Concepts)
DRIVERNV(amazonh3)      //S80b-684D:09/91 Amazon Hunt III (French)
DRIVERNV(amazn3fp)      //                Amazon Hunt III (French Free Play)
DRIVERNV(amazon3a)      //                Amazon Hunt III (rev.1 French)
DRIVERNV(amaz3afp)      //                Amazon Hunt III (rev.1 French Free Play)
//System 3 Alphanumeric
DRIVERNV(tt_game)       //S3-7xx    ??/?? Unnamed game (for Toptronic)
DRIVERNV(ccruise)       //C-102:    ??/89 Caribbean Cruise (for International Concepts)
DRIVERNV(lca)           //S3-720:   11/89 Lights, Camera, Action
DRIVERNV(lca2)          //                Lights, Camera, Action (rev.2)
DRIVERNV(silvslug)      //S3-722:   02/90 Silver Slugger
DRIVERNV(vegas)         //S3-723:   07/90 Vegas
DRIVERNV(deadweap)      //S3-724:   09/90 Deadly Weapon
DRIVERNV(tfight)        //S3-726:   10/90 Title Fight
DRIVERNV(bellring)      //S3-N103:  12/90 Bell Ringer
DRIVERNV(nudgeit)       //S3-N102:  12/90 Nudge It
DRIVERNV(carhop)        //S3-725:   01/91 Car Hop
DRIVERNV(hoops)         //S3-727:   02/91 Hoops
DRIVERNV(cactjack)      //S3-729:   04/91 Cactus Jack's
DRIVERNV(clas1812)      //S3-730:   08/91 Class of 1812
DRIVERNV(surfnsaf)      //S3-731:   11/91 Surf 'n Safari
DRIVERNV(opthund)       //S3-732:   02/92 Operation: Thunder
//System 3 128x32 DMD
DRIVERNV(smb)           //S3-733:   04/92 Super Mario Bros.
DRIVERNV(smb1)          //                Super Mario Bros. (rev.1)
DRIVERNV(smb2)          //                Super Mario Bros. (rev.2)
DRIVERNV(smb3)          //                Super Mario Bros. (rev.3)
DRIVERNV(smbmush)       //S3-N105:  06/92 Super Mario Bros. Mushroom World
DRIVERNV(cueball)       //S3-734:   10/92 Cue Ball Wizard
DRIVERNV(cueball2)      //S3-734:   10/92 Cue Ball Wizard (rev.2)
DRIVERNV(cueball3)      //S3-734:   10/92 Cue Ball Wizard (rev.3)
DRIVERNV(sfight2)       //S3-735:   03/93 Street Fighter II
DRIVERNV(sfight2a)      //                Street Fighter II (rev.1)
DRIVERNV(sfight2b)      //                Street Fighter II (rev.2)
DRIVERNV(teedoff)       //S3-736:   05/93 Tee'd Off
DRIVERNV(teedoff1)      //                Tee'd Off (rev.1)
DRIVERNV(teedoff3)      //                Tee'd Off (rev.3)
DRIVERNV(wipeout)       //S3-738:   10/93 Wipe Out (rev.2)
DRIVERNV(gladiatr)      //S3-737:   11/93 Gladiators
DRIVERNV(wcsoccer)      //S3-741:   02/94 World Challenge Soccer (rev.1)
DRIVERNV(wcsoccd2)      //                World Challenge Soccer (disp.rev.2)
                        //S3-N???:  04/94 Bullseye (redemption game)
DRIVERNV(rescu911)      //S3-740:   05/94 Rescue 911 (rev.1)
DRIVERNV(freddy)        //S3-744:   10/94 Freddy: A Nightmare on Elm Street (rev.3)
DRIVERNV(freddy4)       //S3-744:   10/94 Freddy: A Nightmare on Elm Street (rev.4)
DRIVERNV(shaqattq)      //S3-743:   02/95 Shaq Attaq (rev.5)
DRIVERNV(shaqatt2)      //                Shaq Attaq (rev.2)
DRIVERNV(stargate)      //S3-742:   03/95 Stargate
DRIVERNV(stargat1)      //                Stargate (rev.1)
DRIVERNV(stargat2)      //                Stargate (rev.2)
DRIVERNV(stargat3)      //                Stargate (rev.3)
DRIVERNV(stargat4)      //                Stargate (rev.4)
DRIVERNV(stargat5)      //                Stargate (rev.5)
DRIVERNV(bighurt)       //S3-743:   06/95 Big Hurt (rev.3)
DRIVERNV(snspares)      //S3-N111:  10/95 Strikes N' Spares (rev.6)
DRIVERNV(snspare1)      //                Strikes N' Spares (rev.1)
DRIVERNV(snspare2)      //                Strikes N' Spares (rev.2)
DRIVERNV(waterwld)      //S3-746:   10/95 Waterworld (rev.3)
DRIVERNV(waterwl2)      //                Waterworld (rev.2)
DRIVERNV(andretti)      //S3-747:   12/95 Mario Andretti
DRIVERNV(andrett4)      //                Mario Andretti (rev.T4)
DRIVERNV(barbwire)      //S3-748:   04/96 Barb Wire
DRIVERNV(brooks)        //S3-749:   08/96 Brooks & Dunn (rev.T1, never produced)

// --------------------
// GRAND PRODUCTS, INC.
// --------------------
DRIVERNV(bullseye)      //BY35:     04/86 301/Bullseye
DRIVERNV(bullseyn)      //BY35:     ??/?? 301/Bullseye (normal pinball scoring)

// ----------------
// HANKIN GAMES
// ----------------
DRIVERNV(fjholden)      //FJ Holden
DRIVERNV(orbit1)        //Orbit 1
DRIVERNV(howzat)        //Howzat
DRIVERNV(shark)         //Shark
DRIVERNV(empsback)      //(Star Wars -) Empire Strikes Back, The

// ----------------
// IDSA GAMES
// ----------------
DRIVERNV(v1)            //V-1 (198?)
DRIVERNV(bsktball)      //Basket Ball (04/1987)

// ----------------
// INDER GAMES
// ----------------
                        // Hot and Cold (1978)
                        // Screech (1978)
DRIVERNV(centauri)      //Centaur (1979)
DRIVERNV(centaurj)      //Centaur (1979, alternate set)
DRIVERNV(topazi)        //Topaz (1979)
DRIVERNV(skatebrd)      //Skate Board (1980)
DRIVERNV(brvteam )      //Brave Team (1985)
DRIVERNV(brvteafp)      //Brave Team (Free Play)
DRIVERNV(canasta )      //Canasta '86' (1986)
DRIVERNV(canastfp)      //Canasta '86' (Free Play)
DRIVERNV(lapbylap)      //Lap By Lap (1986)
DRIVERNV(lapbylfp)      //Lap By Lap (Free Play)
DRIVERNV(moonlght)      //Moon Light (1987)
DRIVERNV(moonlifp)      //Moon Light (Free Play)
DRIVERNV(pinclown)      //Clown (1988)
DRIVERNV(pinclofp)      //Clown (Free Play)
DRIVERNV(corsario)      //Corsario (1989)
DRIVERNV(corsarfp)      //Corsario (Free Play)
DRIVERNV(mundial )      //Mundial 90 (1990)
DRIVERNV(mundiafp)      //Mundial 90 (Free Play)
DRIVERNV(atleta  )      //Atleta (1991)
DRIVERNV(ind250cc)      //250 CC (1992)
DRIVERNV(metalman)      //Metal Man (1992)

// ----------------
// JAC VAN HAM
// ----------------
DRIVERNV(icemania)      //Ice Mania (??/1986)
DRIVERNV(escape)        //Escape (10/1987)
DRIVERNV(movmastr)      //Movie Masters (??/19??)
DRIVERNV(formula1)      //Formula 1 (??/1988)

// ----------------
// JEUTEL OF FRANCE
// ----------------
DRIVERNV(leking)        //Le King (??/1983)
DRIVERNV(olympic)       //Olympic Games (??/1984)

// ----------------
// JOCMATIC
// ----------------
DRIVERNV(ridersrf)      //Rider's Surf (1986)

// ----------------
// JOCTRONIC
// ----------------
DRIVERNV(punkywil)      //Punky Willy (1986)
DRIVERNV(walkyria)      //Walkyria (198?)

// ----------------
// JUEGOS POPULARES
// ----------------
DRIVERNV(petaco  )      //1101  - Petaco (1984)
DRIVERNV(petacon )      //1102? - Petaco (1985, using the new hardware)
DRIVERNV(petacona)      //        Petaco (new hardware, alternate version)
DRIVERNV(faeton  )      //1103  - Faeton (1985)
DRIVERNV(halley  )      //1104  - Halley Comet (1986)
DRIVERNV(halleya )      //        Halley Comet (alternate version)
DRIVERNV(aqualand)      //1105  - Aqualand (1986)
DRIVERNV(petaco2 )      //1106  - Petaco 2 (1986)
DRIVERNV(america )      //1107  - America 1492 (1986)
DRIVERNV(olympus )      //1108  - Olympus (1986)
                        //1109  - Unknown game, maybe never produced?
DRIVERNV(lortium )      //1110  - Lortium (1987)
DRIVERNV(pimbal  )      //????  - Pimbal (Pinball 3000, 19??)

// ----------------
// LTD
// ----------------
// Sistema III
                        //Amazon
DRIVERNV(arizona )      //Arizona
DRIVERNV(atla_ltd)      //Atlantis
                        //Big Flush
                        //Carnaval no Rio
DRIVERNV(discodan)      //Disco Dancing
                        //Galaxia
                        //Grand Prix
DRIVERNV(hustler )      //Hustler
DRIVERNV(kkongltd)      //King Kong
                        //Kung Fu
DRIVERNV(marqueen)      //Martian Queen
                        //O Gaucho
                        //Samba
DRIVERNV(spcpoker)      //Space Poker
DRIVERNV(vikngkng)      //Viking King
DRIVERNV(force   )      //Force
DRIVERNV(bhol_ltd)      //Black Hole
DRIVERNV(cowboy3p)      //Cowboy Eight Ball (clone of Bally Eight Ball Deluxe)
DRIVERNV(cowboy3a)      //Cowboy Eight Ball (alternate set)
DRIVERNV(zephy   )      //Zephy (clone of Bally Xenon)
DRIVERNV(zephya  )      //Zephy (alternate set)
// Sistema IV
DRIVERNV(cowboy  )      //Cowboy Eight Ball 2 (clone of Bally Eight Ball Deluxe)
DRIVERNV(hhotel  )      //Haunted Hotel
DRIVERNV(pecmen  )      //Mr. & Mrs. Pec-Men (clone of Bally's... guess the game! :))
DRIVERNV(alcapone)      //Al Capone
DRIVERNV(alienwar)      //Alien Warrior
DRIVERNV(columbia)      //Columbia
DRIVERNV(tmacltd2)      //Time Machine (2 players)
DRIVERNV(tmacltd4)      //Time Machine (4 players)
DRIVERNV(tricksht)      //Trick Shooter

// ----------------
// MAC GAMES
// ----------------
DRIVERNV(macgalxy)      //MAC Galaxy (1986)
DRIVERNV(macjungl)      //MAC Jungle (1986)
DRIVERNV(spctrai0)      //Space Train (1987, old hardware)
DRIVERNV(spctrain)      //Space Train (1987, new hardware)
DRIVERNV(spcpnthr)      //Space Panther (1988)
DRIVERNV(mac_1808)      //Unknown MAC game #1808 (19??)
DRIVERNV(macjungn)      //New MAC Jungle (1995)
DRIVERNV(nbamac)        //NBA MAC (1996)

// ----------------
// MAIBESA
// ----------------
DRIVERNV(ebalchmb)      //Eight Ball Champ (198?)

// ----------------
// MANILAMATIC
// ----------------
DRIVERNV(topsound)      //Top Sound (1988)
DRIVERNV(mmmaster)      //Master (1988)

// ----------------
// MICROPIN GAMES
// ----------------
DRIVERNV(pentacup)      //Pentacup (rev. 1, 1978)
DRIVERNV(pentacp2)      //Pentacup (rev. 2, 1980)

// ----------------
// MIDWAY GAMES
// ----------------
DRIVERNV(flicker )      //Flicker (Prototype, September 1974)
DRIVERNV(rota_101)      //Rotation VIII (v. 1.01, September 1978)
DRIVERNV(rota_115)      //Rotation VIII (v. 1.15, 1978?)
DRIVERNV(rotation)      //Rotation VIII (v. 1.17, 1978?)

// ----------------
// MIRCO GAMES
// ----------------
DRIVERNV(spirit76)      //Spirit of 76 (1975)
DRIVERNV(lckydraw)      //Lucky Draw (1978)

// ----------------
// MONROE BOWLING CO.
// ----------------
DRIVERNV(monrobwl)      //Stars & Strikes (1979?)

// ----------------
// MR. GAME
// ----------------
DRIVERNV(dakar)         //Dakar (1988?)
DRIVERNV(motrshow)      //Motor Show (1988?)
DRIVERNV(motrshwa)      //Motor Show (alternate set)
DRIVERNV(macattck)      //Mac Attack (1989?)
DRIVERNV(wcup90)        //World Cup '90 (1990)

// ----------------
// NONDUM / CIFA
// ----------------
DRIVERNV(comeback)      //Come Back (198?)

// ----------------
// NSM GAMES
// ----------------
                        //Cosmic Flash (10/1985)
DRIVERNV(firebird)      //Hot Fire Birds (12/1985)

// ----------------
// PEYPER
// ----------------
DRIVERNV(odin)          // Odin (1985)
DRIVERNV(nemesis)       // Nemesis (1986)
DRIVERNV(wolfman)       // Wolf Man (1987)
DRIVERNV(odisea)        // Odisea Paris-Dakar (1987)
DRIVERNV(lancelot)      // Sir Lancelot (1994)

// ---------------
// PINSTAR GAMES
// ---------------
DRIVERNV(gamatron)      // Gamatron (December 85)

// ----------------
// PLAYBAR
// ----------------
DRIVERNV(blroller)      // Bloody Roller (1987)
DRIVERNV(cobrapb)       // Cobra (1987)

// ----------------
// PLAYMATIC
// ----------------
DRIVERNV(msdisco)       // ??/?? Miss Disco (bingo machine)
DRIVERNV(spcgambl)      // 03/78 Space Gambler
DRIVERNV(bigtown)       // 04/78 Big Town
DRIVERNV(lastlap)       // 09/78 Last Lap
DRIVERNV(chance)        // 09/78 Chance
DRIVERNV(party)         // 05/79 Party
DRIVERNV(antar)         // 11/79 Antar
DRIVERNV(antar2)        //       Antar (alternate set)
DRIVERNV(evlfight)      // 03/80 Evil Fight
DRIVERNV(attack)        // 10/80 Attack
DRIVERNV(blkfever)      // 12/80 Black Fever
DRIVERNV(zira)          // ??/80 Zira
DRIVERNV(cerberus)      // 03/82 Cerberus
DRIVERNV(spain82)       // 10/82 Spain 82
DRIVERNV(madrace)       // ??/8? Mad Race
DRIVERNV(megaaton)      // 04/84 Meg-Aaton
DRIVERNV(megaatoa)      //       Meg-Aaton (alternate set)
DRIVERNV(nautilus)      // ??/84 Nautilus
DRIVERNV(theraid)       // ??/84 The Raid
DRIVERNV(theraida)      // ??/84 The Raid (alternate set)
DRIVERNV(ufo_x)         // 11/84 UFO-X
DRIVERNV(kz26)          // ??/85 KZ-26
DRIVERNV(rock2500)      // ??/85 Rock 2500
DRIVERNV(starfire)      // ??/85 Star Fire
DRIVERNV(starfira)      //       Star Fire (alternate set)
DRIVERNV(trailer)       // ??/85 Trailer
                        // ??/85 Stop Ship
DRIVERNV(fldragon)      // ??/86 Flash Dragon
DRIVERNV(phntmshp)      // ??/87 Phantom Ship
DRIVERNV(sklflite)      // ??/87 Skill Flight

// ----------------
// REGAMA
// ----------------
DRIVERNV(trebol)        //Trebol (1985)

// ----------------
// ROWAMET
// ----------------
DRIVERNV(heavymtl)      //Heavy Metal (198?)

// --------------
// SEGA GAMES
// --------------
//Data East Hardware, DMD 192x64
DRIVER  (mav,100)       //DE/Sega MPU: 09/94 Maverick 1.00
DRIVER  (mav,400)       //DE/Sega MPU: 09/94 Maverick 4.00
DRIVER  (mav,401)       //DE/Sega MPU: 09/94 Maverick 4.01 Display
DRIVER  (mav,402)       //DE/Sega MPU: 09/94 Maverick 4.02 Display
DRIVERNV(frankst)       //DE/Sega MPU: 12/94 Frankenstein, Mary Shelley's
DRIVERNV(frankstg)      //DE/Sega MPU: 01/95 Frankenstein, Mary Shelley's (German)
DRIVERNV(baywatch)      //DE/Sega MPU: 03/95 Baywatch 4.00
DRIVER  (bay,401)       //DE/Sega MPU: 12/17 Baywatch 4.01 (unofficial MOD)
DRIVER  (bay,d400)      //DE/Sega MPU: 03/95 Baywatch 4.00 (Dutch)
DRIVER  (bay,e400)      //DE/Sega MPU: 03/95 Baywatch 4.00 (English)
DRIVER  (bay,d300)      //DE/Sega MPU: 03/95 Baywatch 3.00 (Dutch)
DRIVERNV(batmanf)       //DE/Sega MPU: 07/95 Batman Forever (4.0)
DRIVERNV(batmanf3)      //DE/Sega MPU: 07/95 Batman Forever (3.0)
DRIVERNV(batmanf2)      //DE/Sega MPU: 07/95 Batman Forever (2.02)
DRIVERNV(batmanf1)      //DE/Sega MPU: 07/95 Batman Forever (1.02)
DRIVER  (bmf,uk)        //DE/Sega MPU: 07/95 Batman Forever (English)
DRIVER  (bmf,at)        //DE/Sega MPU: 07/95 Batman Forever (Austrian)
DRIVER  (bmf,be)        //DE/Sega MPU: 07/95 Batman Forever (Belgian)
DRIVER  (bmf,ch)        //DE/Sega MPU: 07/95 Batman Forever (Swiss)
DRIVER  (bmf,cn)        //DE/Sega MPU: 07/95 Batman Forever (Canadian)
DRIVER  (bmf,de)        //DE/Sega MPU: 07/95 Batman Forever (German)
DRIVER  (bmf,fr)        //DE/Sega MPU: 07/95 Batman Forever (French)
DRIVER  (bmf,nl)        //DE/Sega MPU: 07/95 Batman Forever (Dutch)
DRIVER  (bmf,it)        //DE/Sega MPU: 07/95 Batman Forever (Italian)
DRIVER  (bmf,sp)        //DE/Sega MPU: 07/95 Batman Forever (Spanish)
DRIVER  (bmf,no)        //DE/Sega MPU: 07/95 Batman Forever (Norwegian)
DRIVER  (bmf,sv)        //DE/Sega MPU: 07/95 Batman Forever (Swedish)
DRIVER  (bmf,jp)        //DE/Sega MPU: 07/95 Batman Forever (Japanese)
DRIVER  (bmf,time)      //DE/Sega MPU: 07/95 Batman Forever (Timed Version)
DRIVERNV(ctcheese)      //DE/Sega MPU: ??/96 Cut The Cheese (Redemption)
//Whitestar Hardware, DMD 128x32
DRIVERNV(apollo13)      //Whitestar: 11/95 Apollo 13 (5.01)
DRIVERNV(apollo1)       //Whitestar: 11/95 Apollo 13 (1.00)
DRIVERNV(apollo2)       //Whitestar: 11/95 Apollo 13 (2.03)
DRIVERNV(apollo14)      //Whitestar: 11/95 Apollo 13 (5.01, Display 4.01)
DRIVERNV(gldneye)       //Whitestar: 02/96 Golden Eye
DRIVER  (twst,300)      //Whitestar: 05/96 Twister (3.00)
DRIVER  (twst,404)      //Whitestar: 05/96 Twister (4.04)
DRIVER  (twst,405)      //Whitestar: 05/96 Twister (4.05)
DRIVERNV(id4)           //Whitestar: 07/96 ID4: Independence Day (2.02)
DRIVERNV(id4f)          //Whitestar: 07/96 ID4: Independence Day (2.02 French)
DRIVER  (id4,201)       //Whitestar: 07/96 ID4: Independence Day (2.01)
DRIVER  (id4,201f)      //Whitestar: 07/96 ID4: Independence Day (2.01 French)
DRIVERNV(spacejam)      //Whitestar: 08/96 Space Jam (3.00)
DRIVERNV(spacejm2)      //Whitestar: 08/96 Space Jam (2.00)
DRIVERNV(spacejmf)      //Whitestar: 08/96 Space Jam (3.00 French)
DRIVERNV(spacejmg)      //Whitestar: 08/96 Space Jam (3.00 German)
DRIVERNV(spacejmi)      //Whitestar: 08/96 Space Jam (3.00 Italian)
DRIVERNV(swtril43)      //Whitestar: 02/97 Star Wars Trilogy (4.03)
DRIVERNV(swtril41)      //Whitestar: 02/97 Star Wars Trilogy (4.01)
DRIVERNV(jplstw22)      //Whitestar: 06/97 Lost World: Jurassic Park, The (2.02)
DRIVERNV(jplstw20)      //Whitestar: 06/97 Lost World: Jurassic Park, The (2.00)
DRIVERNV(xfiles20)      //Whitestar: 08/97 X-Files (2.00)
DRIVERNV(xfiles2)       //Whitestar: 08/97 X-Files (2.04)
DRIVERNV(xfilesf)       //Whitestar: 08/97 X-Files (3.03 French)
DRIVERNV(xfiles)        //Whitestar: 08/97 X-Files (3.03)
DRIVERNV(startrp)       //Whitestar: 11/97 Starship Troopers (2.01)
DRIVERNV(startrp2)      //Whitestar: 11/97 Starship Troopers (2.00)
DRIVERNV(ctchzdlx)      //Whitestar: ??/98 Cut The Cheese Deluxe (Redemption)
DRIVERNV(wackadoo)      //Whitestar: ??/98 Wack-A-Doodle-Doo (Redemption)
DRIVERNV(titanic)       //Whitestar: ??/98 Titanic Redemption (Coin dropper)
DRIVERNV(viprsega)      //Whitestar: 02/98 Viper Night Drivin' (2.01)
DRIVER  (vipr,102)      //Whitestar: 02/98 Viper Night Drivin' (1.02)
DRIVERNV(lostspc)       //Whitestar: 06/98 Lost in Space (1.01, Display 1.02)
DRIVERNV(lostspc1)      //Whitestar: 06/98 Lost in Space (1.01, Display 1.01)
DRIVERNV(lostspcg)      //Whitestar: 06/98 Lost in Space (1.01 German)
DRIVERNV(lostspcf)      //Whitestar: 06/98 Lost in Space (1.01 French)
DRIVERNV(goldcue)       //Whitestar: 06/98 Golden Cue
DRIVERNV(godzilla)      //Whitestar: 09/98 Godzilla (2.05)
DRIVER  (godz,100)      //Whitestar: 09/98 Godzilla (1.00)
DRIVER  (godz,090)      //Whitestar: 09/98 Godzilla (0.90 Prototype)
DRIVER  (sprk,090)      //Whitestar: 01/99 South Park (0.90 Prototype)
DRIVER  (sprk,096)      //Whitestar: 01/99 South Park (0.96 Prototype)
DRIVER  (sprk,103)      //Whitestar: 01/99 South Park (1.03)
DRIVER  (harl,a10)      //Whitestar: 09/99 Harley-Davidson (Sega, 1.03, Display 1.00)
DRIVER  (harl,a13)      //Whitestar: 10/99 Harley-Davidson (Sega, 1.03, Display 1.04)
DRIVER  (harl,u13)      //Whitestar: 10/99 Harley-Davidson (Sega, 1.03 English, Display 1.04)
DRIVER  (harl,f13)      //Whitestar: 10/99 Harley-Davidson (Sega, 1.03 French)
DRIVER  (harl,g13)      //Whitestar: 10/99 Harley-Davidson (Sega, 1.03 German)
DRIVER  (harl,i13)      //Whitestar: 10/99 Harley-Davidson (Sega, 1.03 Italian)
DRIVER  (harl,l13)      //Whitestar: 10/99 Harley-Davidson (Sega, 1.03 Spanish)

// ----------------
// SLEIC
// ----------------
DRIVERNV(bikerace)      // Bike Race (1992)
DRIVERNV(bikerac2)      // Bike Race (2-ball play, 1992)
DRIVERNV(sleicpin)      // Pin-Ball (1993)
DRIVERNV(iomoon)        // Io Moon (1994)
                        // Dona Elvira 2 (1996)

// ----------------
// SONIC
// ----------------
DRIVERNV(thrdwrld)      // Third World (1978)
DRIVERNV(ngtfever)      // Night Fever (1979)
                        // Storm (1979) - Williams "Flash" clone
DRIVERNV(odin_dlx)      // Odin De Luxe (1985)
DRIVERNV(gamatros)      // Gamatron (1986)
DRIVERNV(solarwar)      // Solar Wars (1986)
DRIVERNV(sonstwar)      // Star Wars (1987)
DRIVERNV(sonstwr2)      // Star Wars (1987, alternate set)
DRIVERNV(poleposn)      // Pole Position (1987)
DRIVERNV(hangon)        // Hang-On (1988)

// ---------------
// SPINBALL GAMES
// ---------------
DRIVERNV(bushido)       //1993 - Bushido (Last game by Inder before becoming Spinball - but same hardware)
DRIVERNV(bushidoa)      //1993 - Bushido (alternate set)
DRIVERNV(mach2)         //1995 - Mach 2
DRIVERNV(jolypark)      //1996 - Jolly Park
DRIVERNV(vrnwrld)       //1996 - Verne's World

// ---------------
// SPLIN BINGO
// ---------------
DRIVERNV(goldgame)      //19?? - Golden Game
DRIVERNV(goldgam2)      //19?? - Golden Game Stake 6/10

// ---------------
// SPORT MATIC
// ---------------
DRIVERNV(flashman)      //1984 - Flashman
DRIVERNV(terrlake)      //1987 - Terrific Lake

// ---------------
// STARGAME GAMES
// ---------------
DRIVERNV(spcship)       //1986 - Space Ship
DRIVERNV(mephist1)      //1986 - Mephisto (rev. 1.1)
DRIVERNV(mephisto)      //1986 - Mephisto (rev. 1.2)
DRIVERNV(whtforce)      //1987 - White Force
DRIVERNV(ironball)      //1987 - Iron Balls
DRIVERNV(slalom03)      //1988 - Slalom Code 0.3

// ---------------
// STERN GAMES
// ---------------
// MPU-100 - Chime Sound
DRIVERNV(stingray)      //MPU-100: 03/77 Stingray
DRIVERNV(stingrfp)      //MPU-100: 03/77 Stingray (Free Play)
DRIVERNV(pinball)       //MPU-100: 07/77 Pinball
DRIVERNV(pinbalfp)      //MPU-100: 07/77 Pinball (Free Play)
DRIVERNV(stars)         //MPU-100: 03/78 Stars
DRIVERNV(starsfp)       //MPU-100: 03/78 Stars (Free Play)
DRIVERNV(memlane)       //MPU-100: 06/78 Memory Lane
DRIVERNV(memlanfp)      //MPU-100: 06/78 Memory Lane (Free Play)
// MPU-100 - Sound Board: SB-100
DRIVERNV(lectrono)      //MPU-100: 08/78 Lectronamo
DRIVERNV(lectrofp)      //MPU-100: 08/78 Lectronamo (Free Play)
DRIVERNV(wildfyre)      //MPU-100: 10/78 Wildfyre
DRIVERNV(wildfyfp)      //MPU-100: 10/78 Wildfyre (Free Play)
DRIVERNV(nugent)        //MPU-100: 11/78 Nugent
DRIVERNV(nugentfp)      //MPU-100: 11/78 Nugent (Free Play)
DRIVERNV(dracula)       //MPU-100: 01/79 Dracula
DRIVERNV(draculfp)      //MPU-100: 01/79 Dracula (Free Play)
DRIVERNV(trident)       //MPU-100: 03/79 Trident
DRIVERNV(tridenta)      //MPU-100: 03/79 Trident (Old)
DRIVERNV(tridenfp)      //MPU-100: 03/79 Trident (Old Free Play)
DRIVERNV(tridentb)      //MPU-100: 01/09 Trident (MOD rev. 23c)
DRIVERNV(hothand)       //MPU-100: 06/79 Hot Hand
DRIVERNV(hothanfp)      //MPU-100: 06/79 Hot Hand (Free Play)
DRIVERNV(magic)         //MPU-100: 08/79 Magic
DRIVERNV(magicfp)       //MPU-100: 08/79 Magic (Free Play)
DRIVERNV(princess)      //MPU-100: 08/79 Cosmic Princess
DRIVERNV(princefp)      //MPU-100: 08/79 Cosmic Princess (Free Play)
// MPU-200 - Sound Board: SB-300
DRIVERNV(meteor)        //MPU-200: 09/79 Meteor
DRIVERNV(meteora)       //MPU-200: ??/?? Meteor (Bonus Count Official Fix)
DRIVERNV(meteorbf)      //MPU-200: 05/09 Meteor (Bonus Count Fix)
DRIVERNV(meteorfp)      //MPU-200: 09/79 Meteor (Free Play)
DRIVERNV(meteorns)      //MPU-200: 08/11 Meteor (No Background Sound)
DRIVERNV(meteorb)       //MPU-200: 11/03 Meteor (7-Digit conversion)
DRIVERNV(meteorc)       //MPU-200: 11/03 Meteor (7-Digit conversion Free Play)
DRIVERNV(meteord)       //MPU-200: 05/05 Meteor (/10 Scoring)
DRIVERNV(meteora2)      //MPU-200: 08/19 Meteor (Bonus Count and Sound Fix)
DRIVERNV(meteore)       //MPU-200: 12/19 Meteor (Bonus Count Fix, MOD, Free Play rev. 64)
DRIVERNV(meteore7)      //MPU-200: 12/19 Meteor (Bonus Count Fix, MOD, 7-Digit, Free Play rev. 74)
DRIVERNV(galaxy)        //MPU-200: 01/80 Galaxy
DRIVERNV(galaxyps)      //MPU-200: 08/11 Galaxy (Planet Skillshot)
DRIVERNV(galaxyfp)      //MPU-200: 01/80 Galaxy (Free Play)
DRIVERNV(galaxyb)       //MPU-200: ??/04 Galaxy (7-Digit conversion)
DRIVERNV(ali)           //MPU-200: 03/80 Ali
DRIVERNV(alifp)         //MPU-200: 03/80 Ali (Free Play)
DRIVERNV(biggame)       //MPU-200: 03/80 Big Game
DRIVERNV(biggamfp)      //MPU-200: 03/80 Big Game (Free Play)
DRIVERNV(biggameb)      //MPU-200: 11/19 Big Game (MOD rev. 7)
DRIVERNV(seawitch)      //MPU-200: 05/80 Seawitch
DRIVERNV(seawitfp)      //MPU-200: 05/80 Seawitch (Free Play)
DRIVERNV(cheetah)       //MPU-200: 06/80 Cheetah (Black cabinet)
DRIVERNV(cheetahb)      //MPU-200: 06/80 Cheetah (Blue cabinet)
DRIVERNV(cheetafp)      //MPU-200: 06/80 Cheetah (Free Play)
DRIVERNV(cheetah1)      //MPU-200: 06/80 Cheetah (Bonus shot 1/game)
DRIVERNV(cheetah2)      //MPU-200: 06/80 Cheetah (Bonus shot 1/ball)
DRIVERNV(quicksil)      //MPU-200: 06/80 Quicksilver
DRIVERNV(quicksfp)      //MPU-200: 06/80 Quicksilver (Free Play)
DRIVERNV(stargzr)       //MPU-200: 08/80 Star Gazer
DRIVERNV(stargzfp)      //MPU-200: 08/80 Star Gazer (Free Play)
DRIVERNV(stargzrb)      //MPU-200: 03/06 Star Gazer (modified rules rev. 9)
DRIVERNV(flight2k)      //MPU-200: 10/80 Flight 2000
DRIVERNV(flightfp)      //MPU-200: 10/80 Flight 2000 (Free Play)
DRIVERNV(flight2m)      //MPU-200: 04/20 Flight 2000 (modified rules rev. 3335)
DRIVERNV(nineball)      //MPU-200: 12/80 Nine Ball
DRIVERNV(ninebafp)      //MPU-200: 12/80 Nine Ball (Free Play)
DRIVERNV(ninebala)      //MPU-200: 04/20 Nine Ball (Ball handling MOD beta18)
DRIVERNV(ninebalb)      //MPU-200: 01/07 Nine Ball (modified rules rev. 85)
DRIVERNV(freefall)      //MPU-200: 01/81 Freefall
DRIVERNV(freefafp)      //MPU-200: 01/81 Freefall (Free Play)
DRIVERNV(lightnin)      //MPU-200: 03/81 Lightning
DRIVERNV(lightnfp)      //MPU-200: 03/81 Lightning (Free Play)
DRIVERNV(splitsec)      //MPU-200: 08/81 Split Second
DRIVERNV(splitsfp)      //MPU-200: 08/81 Split Second (Free Play)
DRIVERNV(catacomb)      //MPU-200: 10/81 Catacomb
DRIVERNV(catacofp)      //MPU-200: 10/81 Catacomb (Free Play)
DRIVERNV(ironmaid)      //MPU-200: 10/81 Iron Maiden
DRIVERNV(ironmafp)      //MPU-200: 10/81 Iron Maiden (Free Play)
DRIVERNV(viper)         //MPU-200: 12/81 Viper
DRIVERNV(viperfp)       //MPU-200: 12/81 Viper (Free Play)
DRIVERNV(dragfist)      //MPU-200: 01/82 Dragonfist
DRIVERNV(dragfifp)      //MPU-200: 01/82 Dragonfist (Free Play)
DRIVERNV(dragfisb)      //MPU-200: 01/82 Dragonfist (MOD - modified to match instruction card/manual)
DRIVERNV(dragfib2)      //MPU-200: 01/82 Dragonfist (MOD 2 - modified to match instruction card/manual)
DRIVERNV(dragfis3)      //MPU-200: 03/20 Dragonfist (MOD 3 rev. 1105)
DRIVERNV(orbitor1)      //MPU-200: 04/82 Orbitor 1
DRIVERNV(orbitofp)      //MPU-200: 04/82 Orbitor 1 (Free Play)
DRIVERNV(orbitora)      //MPU-200: 04/82 Orbitor 1 (MOD)
DRIVERNV(orbitorb)      //MPU-200: 04/82 Orbitor 1 (MOD Free Play)
DRIVERNV(orbitorc)      //MPU-200: 04/82 Orbitor 1 (MOD No Timed Game)
DRIVERNV(cue)           //MPU-200: ??/82 Cue (Proto - Never released)
DRIVERNV(blbeauty)      //MPU-200: 09/84 Black Beauty (Shuffle)
DRIVERNV(lazrlord)      //MPU-200: 10/84 Lazer Lord (Proto - Never released)
// Whitestar System
DRIVERNV(strikext)      //Whitestar: 05/00 Striker Extreme (1.02)
DRIVERNV(strxt_uk)      //Whitestar: 03/00 Striker Extreme (1.01 English)
DRIVERNV(strxt_gr)      //Whitestar: 07/00 Striker Extreme (1.03 German)
DRIVERNV(strxt_sp)      //Whitestar: 05/00 Striker Extreme (1.02 Spanish)
DRIVERNV(strxt_fr)      //Whitestar: 05/00 Striker Extreme (1.02 French)
DRIVERNV(strxt_it_100)  //Whitestar: 03/00 Striker Extreme (1.00 Italian)
DRIVERNV(strxt_it_101)  //Whitestar: 03/00 Striker Extreme (1.01 Italian)
DRIVERNV(strxt_it)      //Whitestar: 05/00 Striker Extreme (1.02 Italian)
DRIVERNV(shrkysht)      //Whitestar: 09/00 Sharkey's Shootout (2.11)
DRIVERNV(shrky_gr)      //Whitestar: 09/00 Sharkey's Shootout (2.11 German)
DRIVERNV(shrky_fr)      //Whitestar: 09/00 Sharkey's Shootout (2.11 French)
DRIVERNV(shrky_it)      //Whitestar: 09/00 Sharkey's Shootout (2.11 Italian)
DRIVERNV(shrky207)      //Whitestar: 09/00 Sharkey's Shootout (2.07)
DRIVERNV(hirolcas)      //Whitestar: 01/01 High Roller Casino (3.00)
DRIVERNV(hirol_g3)      //Whitestar: 01/01 High Roller Casino (3.00 German)
DRIVERNV(hirol_fr)      //Whitestar: 01/01 High Roller Casino (3.00 French)
DRIVERNV(hirol_it)      //Whitestar: 01/01 High Roller Casino (3.00 Italian)
DRIVERNV(hirolcat)      //Whitestar: 01/01 High Roller Casino (3.00 TEST BUILD 1820)
DRIVERNV(hirol210)      //Whitestar: 01/01 High Roller Casino (2.10)
DRIVERNV(hirol_gr)      //Whitestar: 01/01 High Roller Casino (2.10 German)
DRIVERNV(austin)        //Whitestar: 05/01 Austin Powers (3.02)
DRIVERNV(austing)       //Whitestar: 05/01 Austin Powers (3.02 German)
DRIVERNV(austinf)       //Whitestar: 05/01 Austin Powers (3.02 French)
DRIVERNV(austini)       //Whitestar: 05/01 Austin Powers (3.02 Italian)
DRIVERNV(aust301)       //Whitestar: 05/01 Austin Powers (3.01)
DRIVERNV(aust300)       //Whitestar: 05/01 Austin Powers (3.00)
DRIVERNV(aust201)       //Whitestar: 05/01 Austin Powers (2.01)
DRIVERNV(monopoly)      //Whitestar: 09/01 Monopoly (3.20) // 3.20 has the broken dice
DRIVERNV(monopolg)      //Whitestar: 09/01 Monopoly (3.20 German)
DRIVERNV(monopoll)      //Whitestar: 09/01 Monopoly (3.20 Spanish)
DRIVERNV(monopolf)      //Whitestar: 09/01 Monopoly (3.20 French)
DRIVERNV(monopoli)      //Whitestar: 09/01 Monopoly (3.20 Italian)
DRIVERNV(monopole)      //Whitestar: 09/01 Monopoly (3.03)
DRIVERNV(monop301)      //Whitestar: 09/01 Monopoly (3.01)
DRIVERNV(monop251)      //Whitestar: 09/01 Monopoly (2.51)
DRIVERNV(monoi251)      //Whitestar: 09/01 Monopoly (2.51 Italian)
DRIVERNV(monop233)      //Whitestar: 09/01 Monopoly (2.33)
DRIVERNV(nfl)           //Whitestar: 11/01 NFL
DRIVERNV(monopred)      //Whitestar: ??/02 Monopoly Redemption (Coin dropper)
DRIVERNV(playboys)      //Whitestar: 02/02 Playboy (Stern, 5.00)
DRIVERNV(playboyg)      //Whitestar: 02/02 Playboy (Stern, 5.00 German)
DRIVERNV(playboyl)      //Whitestar: 02/02 Playboy (Stern, 5.00 Spanish)
DRIVERNV(playboyf)      //Whitestar: 02/02 Playboy (Stern, 5.00 French)
DRIVERNV(playboyi)      //Whitestar: 02/02 Playboy (Stern, 5.00 Italian)
DRIVERNV(play401)       //Whitestar: 02/02 Playboy (Stern, 4.01)
DRIVERNV(play401g)      //Whitestar: 02/02 Playboy (Stern, 4.01 German)
DRIVERNV(play401l)      //Whitestar: 02/02 Playboy (Stern, 4.01 Spanish)
DRIVERNV(play401f)      //Whitestar: 02/02 Playboy (Stern, 4.01 French)
DRIVERNV(play401i)      //Whitestar: 02/02 Playboy (Stern, 4.01 Italian)
DRIVERNV(play303)       //Whitestar: 02/02 Playboy (Stern, 3.03)
DRIVERNV(play303g)      //Whitestar: 02/02 Playboy (Stern, 3.03 German)
DRIVERNV(play303l)      //Whitestar: 02/02 Playboy (Stern, 3.03 Spanish)
DRIVERNV(play303f)      //Whitestar: 02/02 Playboy (Stern, 3.03 French)
DRIVERNV(play303i)      //Whitestar: 02/02 Playboy (Stern, 3.03 Italian)
DRIVERNV(play302)       //Whitestar: 02/02 Playboy (Stern, 3.02)
DRIVERNV(play302g)      //Whitestar: 02/02 Playboy (Stern, 3.02 German)
DRIVERNV(play302l)      //Whitestar: 02/02 Playboy (Stern, 3.02 Spanish)
DRIVERNV(play302f)      //Whitestar: 02/02 Playboy (Stern, 3.02 French)
DRIVERNV(play302i)      //Whitestar: 02/02 Playboy (Stern, 3.02 Italian)
DRIVERNV(play300)       //Whitestar: 02/02 Playboy (Stern, 3.00)
DRIVERNV(play203)       //Whitestar: 02/02 Playboy (Stern, 2.03)
DRIVERNV(play203g)      //Whitestar: 02/02 Playboy (Stern, 2.03 German)
DRIVERNV(play203l)      //Whitestar: 02/02 Playboy (Stern, 2.03 Spanish)
DRIVERNV(play203f)      //Whitestar: 02/02 Playboy (Stern, 2.03 French)
DRIVERNV(play203i)      //Whitestar: 02/02 Playboy (Stern, 2.03 Italian)
DRIVERNV(rctycn)        //Whitestar: 09/02 RollerCoaster Tycoon (7.02)
DRIVERNV(rctycng)       //Whitestar: 09/02 RollerCoaster Tycoon (7.02 German)
DRIVERNV(rctycnl)       //Whitestar: 09/02 RollerCoaster Tycoon (7.02 Spanish)
DRIVERNV(rctycnf)       //Whitestar: 09/02 RollerCoaster Tycoon (7.02 French)
DRIVERNV(rctycni)       //Whitestar: 09/02 RollerCoaster Tycoon (7.02 Italian)
DRIVERNV(rct701)        //Whitestar: 09/02 RollerCoaster Tycoon (7.01)
DRIVERNV(rct701g)       //Whitestar: 09/02 RollerCoaster Tycoon (7.01 German)
DRIVERNV(rct701l)       //Whitestar: 09/02 RollerCoaster Tycoon (7.01 Spanish)
DRIVERNV(rct701f)       //Whitestar: 09/02 RollerCoaster Tycoon (7.01 French)
DRIVERNV(rct701i)       //Whitestar: 09/02 RollerCoaster Tycoon (7.01 Italian)
DRIVERNV(rct600)        //Whitestar: 09/02 RollerCoaster Tycoon (6.00)
DRIVERNV(rct600l)       //Whitestar: 09/02 RollerCoaster Tycoon (6.00 Spanish)
DRIVERNV(rct600f)       //Whitestar: 09/02 RollerCoaster Tycoon (6.00 French)
DRIVERNV(rct600i)       //Whitestar: 09/02 RollerCoaster Tycoon (6.00 Italian)
DRIVERNV(rct400)        //Whitestar: 09/02 RollerCoaster Tycoon (4.00)
DRIVERNV(rct400l)       //Whitestar: 09/02 RollerCoaster Tycoon (4.00 Spanish)
DRIVERNV(rct400f)       //Whitestar: 09/02 RollerCoaster Tycoon (4.00 French)
DRIVERNV(rct400g)       //Whitestar: 09/02 RollerCoaster Tycoon (4.00 German)
DRIVERNV(rct400i)       //Whitestar: 09/02 RollerCoaster Tycoon (4.00 Italian)
DRIVERNV(simpprty)      //Whitestar: 01/03 Simpsons Pinball Party, The (5.00)
DRIVERNV(simpprtg)      //Whitestar: 01/03 Simpsons Pinball Party, The (5.00 German)
DRIVERNV(simpprtl)      //Whitestar: 01/03 Simpsons Pinball Party, The (5.00 Spanish)
DRIVERNV(simpprtf)      //Whitestar: 01/03 Simpsons Pinball Party, The (5.00 French)
DRIVERNV(simpprti)      //Whitestar: 01/03 Simpsons Pinball Party, The (5.00 Italian)
DRIVERNV(simp400)       //Whitestar: 01/03 Simpsons Pinball Party, The (4.00)
DRIVERNV(simp400g)      //Whitestar: 01/03 Simpsons Pinball Party, The (4.00 German)
DRIVERNV(simp400l)      //Whitestar: 01/03 Simpsons Pinball Party, The (4.00 Spanish)
DRIVERNV(simp400f)      //Whitestar: 01/03 Simpsons Pinball Party, The (4.00 French)
DRIVERNV(simp400i)      //Whitestar: 01/03 Simpsons Pinball Party, The (4.00 Italian)
DRIVERNV(simp300)       //Whitestar: 01/03 Simpsons Pinball Party, The (3.00)
DRIVERNV(simp300l)      //Whitestar: 01/03 Simpsons Pinball Party, The (3.00 Spanish)
DRIVERNV(simp300f)      //Whitestar: 01/03 Simpsons Pinball Party, The (3.00 French)
DRIVERNV(simp300i)      //Whitestar: 01/03 Simpsons Pinball Party, The (3.00 Italian)
DRIVERNV(simp204)       //Whitestar: 01/03 Simpsons Pinball Party, The (2.04)
DRIVERNV(simp204l)      //Whitestar: 01/03 Simpsons Pinball Party, The (2.04 Spanish)
DRIVERNV(simp204f)      //Whitestar: 01/03 Simpsons Pinball Party, The (2.04 French)
DRIVERNV(simp204i)      //Whitestar: 01/03 Simpsons Pinball Party, The (2.04 Italian)
DRIVERNV(term3)         //Whitestar: 06/03 Terminator 3: Rise of the Machines (4.00)
DRIVERNV(term3g)        //Whitestar: 06/03 Terminator 3: Rise of the Machines (4.00 German)
DRIVERNV(term3l)        //Whitestar: 06/03 Terminator 3: Rise of the Machines (4.00 Spanish)
DRIVERNV(term3f)        //Whitestar: 06/03 Terminator 3: Rise of the Machines (4.00 French)
DRIVERNV(term3i)        //Whitestar: 06/03 Terminator 3: Rise of the Machines (4.00 Italian)
DRIVERNV(term3_3)       //Whitestar: 06/03 Terminator 3: Rise of the Machines (3.01)
DRIVERNV(term3g_3)      //Whitestar: 06/03 Terminator 3: Rise of the Machines (3.01 German)
DRIVERNV(term3l_3)      //Whitestar: 06/03 Terminator 3: Rise of the Machines (3.01 Spanish)
DRIVERNV(term3f_3)      //Whitestar: 06/03 Terminator 3: Rise of the Machines (3.01 French)
DRIVERNV(term3i_3)      //Whitestar: 06/03 Terminator 3: Rise of the Machines (3.01 Italian)
DRIVERNV(term3_2)       //Whitestar: 06/03 Terminator 3: Rise of the Machines (2.05)
DRIVERNV(term3l_2)      //Whitestar: 06/03 Terminator 3: Rise of the Machines (2.05 Spanish)
DRIVERNV(term3f_2)      //Whitestar: 06/03 Terminator 3: Rise of the Machines (2.05 French)
DRIVERNV(term3i_2)      //Whitestar: 06/03 Terminator 3: Rise of the Machines (2.05 Italian)
DRIVER  (harl,a18)      //Whitestar: 07/03 Harley-Davidson (Stern, 1.08)
DRIVER  (harl,f18)      //Whitestar: 07/03 Harley-Davidson (Stern, 1.08 French)
DRIVER  (harl,g18)      //Whitestar: 07/03 Harley-Davidson (Stern, 1.08 German)
DRIVER  (harl,i18)      //Whitestar: 07/03 Harley-Davidson (Stern, 1.08 Italian)
DRIVER  (harl,l18)      //Whitestar: 07/03 Harley-Davidson (Stern, 1.08 Spanish)
// New CPU/Sound Board with ARM7 CPU + Xilinx FPGA controlling sound
DRIVERNV(lotr_le)       //Whitestar: ??/08 Lord of the Rings (10.02 Limited Edition)
DRIVERNV(lotr)          //Whitestar: 10/05 Lord of the Rings (10.00)
DRIVERNV(lotr_gr)       //Whitestar: 10/05 Lord of the Rings (10.00 German)
DRIVERNV(lotr_sp)       //Whitestar: 10/05 Lord of the Rings (10.00 Spanish)
DRIVERNV(lotr_fr)       //Whitestar: 10/05 Lord of the Rings (10.00 French)
DRIVERNV(lotr_it)       //Whitestar: 10/05 Lord of the Rings (10.00 Italian)
DRIVERNV(lotr9)         //Whitestar: 01/05 Lord of the Rings (9.00)
DRIVERNV(lotr_gr9)      //Whitestar: 01/05 Lord of the Rings (9.00 German)
DRIVERNV(lotr_sp9)      //Whitestar: 01/05 Lord of the Rings (9.00 Spanish)
DRIVERNV(lotr_fr9)      //Whitestar: 01/05 Lord of the Rings (9.00 French)
DRIVERNV(lotr_it9)      //Whitestar: 01/05 Lord of the Rings (9.00 Italian)
DRIVERNV(lotr8)         //Whitestar: 07/04 Lord of the Rings (8.00)
DRIVERNV(lotr_gr8)      //Whitestar: 07/04 Lord of the Rings (8.00 German)
DRIVERNV(lotr_sp8)      //Whitestar: 07/04 Lord of the Rings (8.00 Spanish)
DRIVERNV(lotr_fr8)      //Whitestar: 07/04 Lord of the Rings (8.00 French)
DRIVERNV(lotr_it8)      //Whitestar: 07/04 Lord of the Rings (8.00 Italian)
DRIVERNV(lotr7)         //Whitestar: 04/04 Lord of the Rings (7.00)
DRIVERNV(lotr_gr7)      //Whitestar: 04/04 Lord of the Rings (7.00 German)
DRIVERNV(lotr_sp7)      //Whitestar: 04/04 Lord of the Rings (7.00 Spanish)
DRIVERNV(lotr_fr7)      //Whitestar: 04/04 Lord of the Rings (7.00 French)
DRIVERNV(lotr_it7)      //Whitestar: 04/04 Lord of the Rings (7.00 Italian)
DRIVERNV(lotr6)         //Whitestar: 04/04 Lord of the Rings (6.00)
DRIVERNV(lotr_gr6)      //Whitestar: 04/04 Lord of the Rings (6.00 German)
DRIVERNV(lotr_sp6)      //Whitestar: 04/04 Lord of the Rings (6.00 Spanish)
DRIVERNV(lotr_fr6)      //Whitestar: 04/04 Lord of the Rings (6.00 French)
DRIVERNV(lotr_it6)      //Whitestar: 04/04 Lord of the Rings (6.00 Italian)
DRIVERNV(lotr51)        //Whitestar: 04/04 Lord of the Rings (5.01)
DRIVERNV(lotr_g51)      //Whitestar: 04/04 Lord of the Rings (5.01 German)
DRIVERNV(lotr_s51)      //Whitestar: 04/04 Lord of the Rings (5.01 Spanish)
DRIVERNV(lotr_f51)      //Whitestar: 04/04 Lord of the Rings (5.01 French)
DRIVERNV(lotr_i51)      //Whitestar: 04/04 Lord of the Rings (5.01 Italian)
DRIVERNV(lotr5)         //Whitestar: 04/04 Lord of the Rings (5.00)
DRIVERNV(lotr_gr5)      //Whitestar: 04/04 Lord of the Rings (5.00 German)
DRIVERNV(lotr_sp5)      //Whitestar: 04/04 Lord of the Rings (5.00 Spanish)
DRIVERNV(lotr_fr5)      //Whitestar: 04/04 Lord of the Rings (5.00 French)
DRIVERNV(lotr_it5)      //Whitestar: 04/04 Lord of the Rings (5.00 Italian)
DRIVERNV(lotr41)        //Whitestar: 11/03 Lord of the Rings (4.10)
DRIVERNV(lotr_g41)      //Whitestar: 11/03 Lord of the Rings (4.10 German)
DRIVERNV(lotr_f41)      //Whitestar: 11/03 Lord of the Rings (4.10 French)
DRIVERNV(lotr_i41)      //Whitestar: 11/03 Lord of the Rings (4.10 Italian)
DRIVERNV(lotr4)         //Whitestar: 11/03 Lord of the Rings (4.01)
DRIVERNV(lotr_gr4)      //Whitestar: 11/03 Lord of the Rings (4.01 German)
DRIVERNV(lotr_sp4)      //Whitestar: 11/03 Lord of the Rings (4.01 Spanish)
DRIVERNV(lotr_fr4)      //Whitestar: 11/03 Lord of the Rings (4.01 French)
DRIVERNV(lotr_it4)      //Whitestar: 11/03 Lord of the Rings (4.01 Italian)
DRIVERNV(lotr3)         //Whitestar: 11/03 Lord of the Rings (3.00)
DRIVERNV(ripleys)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.20)
DRIVERNV(ripleysg)      //Whitestar: 03/04 Ripley's Believe It or Not! (3.20 German)
DRIVERNV(ripleysl)      //Whitestar: 03/04 Ripley's Believe It or Not! (3.20 Spanish)
DRIVERNV(ripleysf)      //Whitestar: 03/04 Ripley's Believe It or Not! (3.20 French)
DRIVERNV(ripleysi)      //Whitestar: 03/04 Ripley's Believe It or Not! (3.20 Italian)
DRIVERNV(rip310)        //Whitestar: 03/04 Ripley's Believe It or Not! (3.10)
DRIVERNV(rip310g)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.10 German)
DRIVERNV(rip310l)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.10 Spanish)
DRIVERNV(rip310f)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.10 French)
DRIVERNV(rip310i)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.10 Italian)
DRIVERNV(rip302)        //Whitestar: 03/04 Ripley's Believe It or Not! (3.02)
DRIVERNV(rip302g)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.02 German)
DRIVERNV(rip302l)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.02 Spanish)
DRIVERNV(rip302f)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.02 French)
DRIVERNV(rip302i)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.02 Italian)
DRIVERNV(rip301)        //Whitestar: 03/04 Ripley's Believe It or Not! (3.01)
DRIVERNV(rip301g)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.01 German)
DRIVERNV(rip301l)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.01 Spanish)
DRIVERNV(rip301f)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.01 French)
DRIVERNV(rip301i)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.01 Italian)
DRIVERNV(rip300)        //Whitestar: 03/04 Ripley's Believe It or Not! (3.00)
DRIVERNV(rip300g)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.00 German)
DRIVERNV(rip300l)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.00 Spanish)
DRIVERNV(rip300f)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.00 French)
DRIVERNV(rip300i)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.00 Italian)
DRIVERNV(elvis)         //Whitestar: 08/04 Elvis (5.00)
DRIVERNV(elvisg)        //Whitestar: 08/04 Elvis (5.00 German)
DRIVERNV(elvisl)        //Whitestar: 08/04 Elvis (5.00 Spanish)
DRIVERNV(elvisf)        //Whitestar: 08/04 Elvis (5.00 French)
DRIVERNV(elvisi)        //Whitestar: 08/04 Elvis (5.00 Italian)
DRIVERNV(elv400)        //Whitestar: 08/04 Elvis (4.00)
DRIVERNV(elv400g)       //Whitestar: 08/04 Elvis (4.00 German)
DRIVERNV(elv400l)       //Whitestar: 08/04 Elvis (4.00 Spanish)
DRIVERNV(elv400f)       //Whitestar: 08/04 Elvis (4.00 French)
DRIVERNV(elv400i)       //Whitestar: 08/04 Elvis (4.00 Italian)
DRIVERNV(elv303)        //Whitestar: 08/04 Elvis (3.03)
DRIVERNV(elv303g)       //Whitestar: 08/04 Elvis (3.03 German)
DRIVERNV(elv303l)       //Whitestar: 08/04 Elvis (3.03 Spanish)
DRIVERNV(elv303f)       //Whitestar: 08/04 Elvis (3.03 French)
DRIVERNV(elv303i)       //Whitestar: 08/04 Elvis (3.03 Italian)
DRIVERNV(elv302)        //Whitestar: 08/04 Elvis (3.02)
DRIVERNV(elv302g)       //Whitestar: 08/04 Elvis (3.02 German)
DRIVERNV(elv302l)       //Whitestar: 08/04 Elvis (3.02 Spanish)
DRIVERNV(elv302f)       //Whitestar: 08/04 Elvis (3.02 French)
DRIVERNV(elv302i)       //Whitestar: 08/04 Elvis (3.02 Italian)
DRIVER  (harl,a40)      //Whitestar: 10/04 Harley-Davidson (Stern, 4.00)
DRIVER  (harl,f40)      //Whitestar: 10/04 Harley-Davidson (Stern, 4.00 French)
DRIVER  (harl,g40)      //Whitestar: 10/04 Harley-Davidson (Stern, 4.00 German)
DRIVER  (harl,i40)      //Whitestar: 10/04 Harley-Davidson (Stern, 4.00 Italian)
DRIVER  (harl,l40)      //Whitestar: 10/04 Harley-Davidson (Stern, 4.00 Spanish)
DRIVER  (harl,a30)      //Whitestar: 10/04 Harley-Davidson (Stern, 3.00)
DRIVER  (harl,f30)      //Whitestar: 10/04 Harley-Davidson (Stern, 3.00 French)
DRIVER  (harl,g30)      //Whitestar: 10/04 Harley-Davidson (Stern, 3.00 German)
DRIVER  (harl,i30)      //Whitestar: 10/04 Harley-Davidson (Stern, 3.00 Italian)
DRIVER  (harl,l30)      //Whitestar: 10/04 Harley-Davidson (Stern, 3.00 Spanish)
DRIVERNV(sopranos)      //Whitestar: 02/05 Sopranos, The (5.00)
DRIVERNV(sopranog)      //Whitestar: 02/05 Sopranos, The (5.00 German)
DRIVERNV(sopranol)      //Whitestar: 02/05 Sopranos, The (5.00 Spanish)
DRIVERNV(sopranof)      //Whitestar: 02/05 Sopranos, The (5.00 French)
DRIVERNV(sopranoi)      //Whitestar: 02/05 Sopranos, The (5.00 Italian)
DRIVERNV(sopr400)       //Whitestar: 02/05 Sopranos, The (4.00)
DRIVERNV(sopr400g)      //Whitestar: 02/05 Sopranos, The (4.00 German)
DRIVERNV(sopr400l)      //Whitestar: 02/05 Sopranos, The (4.00 Spanish)
DRIVERNV(sopr400f)      //Whitestar: 02/05 Sopranos, The (4.00 French)
DRIVERNV(sopr400i)      //Whitestar: 02/05 Sopranos, The (4.00 Italian)
DRIVERNV(sopr300)       //Whitestar: 02/05 Sopranos, The (3.00)
DRIVERNV(soprano3)      //Whitestar: 02/05 Sopranos, The (3.00 alternative)
DRIVERNV(sopr300g)      //Whitestar: 02/05 Sopranos, The (3.00 German)
DRIVERNV(sopr300l)      //Whitestar: 02/05 Sopranos, The (3.00 Spanish)
DRIVERNV(sopr300f)      //Whitestar: 02/05 Sopranos, The (3.00 French)
DRIVERNV(sopr300i)      //Whitestar: 02/05 Sopranos, The (3.00 Italian)
DRIVERNV(sopr204)       //Whitestar: 02/05 Sopranos, The (2.04)
DRIVERNV(sopr107l)      //Whitestar: 02/05 Sopranos, The (1.07 Spanish)
DRIVERNV(sopr107g)      //Whitestar: 02/05 Sopranos, The (1.07 German)
DRIVERNV(sopr107f)      //Whitestar: 02/05 Sopranos, The (1.07 French)
DRIVERNV(sopr107i)      //Whitestar: 02/05 Sopranos, The (1.07 Italian)
DRIVERNV(nascar)        //Whitestar: 07/06 NASCAR (4.50)
DRIVERNV(nascarl)       //Whitestar: 07/06 NASCAR (4.50 Spanish)
DRIVERNV(nas400)        //Whitestar: 10/05 NASCAR (4.00)
DRIVERNV(nas400l)       //Whitestar: 10/05 NASCAR (4.00 Spanish)
DRIVERNV(nas352)        //Whitestar: 10/05 NASCAR (3.52)
DRIVERNV(nas352l)       //Whitestar: 10/05 NASCAR (3.52 Spanish)
DRIVERNV(nas350)        //Whitestar: 10/05 NASCAR (3.50)
DRIVERNV(nas350l)       //Whitestar: 10/05 NASCAR (3.50 Spanish)
DRIVERNV(nas340)        //Whitestar: 10/05 NASCAR (3.40)
DRIVERNV(nas340l)       //Whitestar: 10/05 NASCAR (3.40 Spanish)
DRIVERNV(nas301)        //Whitestar: 09/05 NASCAR (3.01)
DRIVERNV(nas301l)       //Whitestar: 09/05 NASCAR (3.01 Spanish)
DRIVERNV(gprix)         //Whitestar: 07/06 Grand Prix (4.50)
DRIVERNV(gprixg)        //Whitestar: 07/06 Grand Prix (4.50 German)
DRIVERNV(gprixl)        //Whitestar: 07/06 Grand Prix (4.50 Spanish)
DRIVERNV(gprixf)        //Whitestar: 07/06 Grand Prix (4.50 French)
DRIVERNV(gprixi)        //Whitestar: 07/06 Grand Prix (4.50 Italian)
DRIVERNV(gpr400)        //Whitestar: 10/05 Grand Prix (4.00)
DRIVERNV(gpr400g)       //Whitestar: 10/05 Grand Prix (4.00 German)
DRIVERNV(gpr400l)       //Whitestar: 10/05 Grand Prix (4.00 Spanish)
DRIVERNV(gpr400f)       //Whitestar: 10/05 Grand Prix (4.00 French)
DRIVERNV(gpr400i)       //Whitestar: 10/05 Grand Prix (4.00 Italian)
DRIVERNV(gpr352)        //Whitestar: 10/05 Grand Prix (3.52)
DRIVERNV(gpr352g)       //Whitestar: 10/05 Grand Prix (3.52 German)
DRIVERNV(gpr352l)       //Whitestar: 10/05 Grand Prix (3.52 Spanish)
DRIVERNV(gpr352f)       //Whitestar: 10/05 Grand Prix (3.52 French)
DRIVERNV(gpr352i)       //Whitestar: 10/05 Grand Prix (3.52 Italian)
DRIVERNV(gpr350)        //Whitestar: 10/05 Grand Prix (3.50)
DRIVERNV(gpr350g)       //Whitestar: 10/05 Grand Prix (3.50 German)
DRIVERNV(gpr350l)       //Whitestar: 10/05 Grand Prix (3.50 Spanish)
DRIVERNV(gpr350f)       //Whitestar: 10/05 Grand Prix (3.50 French)
DRIVERNV(gpr350i)       //Whitestar: 10/05 Grand Prix (3.50 Italian)
DRIVERNV(gpr340)        //Whitestar: 10/05 Grand Prix (3.40)
DRIVERNV(gpr340g)       //Whitestar: 10/05 Grand Prix (3.40 German)
DRIVERNV(gpr340l)       //Whitestar: 10/05 Grand Prix (3.40 Spanish)
DRIVERNV(gpr340f)       //Whitestar: 10/05 Grand Prix (3.40 French)
DRIVERNV(gpr340i)       //Whitestar: 10/05 Grand Prix (3.40 Italian)
DRIVERNV(gpr301)        //Whitestar: 09/05 Grand Prix (3.01)
DRIVERNV(gpr301g)       //Whitestar: 09/05 Grand Prix (3.01 German)
DRIVERNV(gpr301l)       //Whitestar: 09/05 Grand Prix (3.01 Spanish)
DRIVERNV(gpr301f)       //Whitestar: 09/05 Grand Prix (3.01 French)
DRIVERNV(gpr301i)       //Whitestar: 09/05 Grand Prix (3.01 Italian)
DRIVERNV(dalejr)        //Whitestar: 07/06 Dale Jr. (NASCAR 5.00)
                        //Whitestar: ??/06 The Brain (Simpsons Pinball Party conversion)
// S.A.M. System
DRIVER(sam1_flashb,0102)//S.A.M.: 02/06 S.A.M. System Flash Boot - V1.02
DRIVER(sam1_flashb,0106)//S.A.M.: 08/06 S.A.M. System Flash Boot - V1.06
DRIVER(sam1_flashb,0210)//S.A.M.: ??/07 S.A.M. System Flash Boot - V2.10
DRIVER(sam1_flashb,0230)//S.A.M.: 06/07 S.A.M. System Flash Boot - V2.3
DRIVER(sam1_flashb,0310)//S.A.M.: 01/08 S.A.M. System Flash Boot - V3.1
DRIVER(wpt,103a)        //S.A.M.: 02/06 World Poker Tour - V1.03 (English)
DRIVER(wpt,105a)        //S.A.M.: 02/06 World Poker Tour - V1.05 (English)
DRIVER(wpt,106a)        //S.A.M.: 02/06 World Poker Tour - V1.06 (English)
DRIVER(wpt,106f)        //S.A.M.: 02/06 World Poker Tour - V1.06 (French)
DRIVER(wpt,106g)        //S.A.M.: 02/06 World Poker Tour - V1.06 (German)
DRIVER(wpt,106i)        //S.A.M.: 02/06 World Poker Tour - V1.06 (Italian)
DRIVER(wpt,106l)        //S.A.M.: 02/06 World Poker Tour - V1.06 (Spanish)
//DRIVER(wpt,107a)      //S.A.M.: 02/06 World Poker Tour - V1.07 (English)
DRIVER(wpt,108a)        //S.A.M.: 03/06 World Poker Tour - V1.08 (English)
DRIVER(wpt,108f)        //S.A.M.: 03/06 World Poker Tour - V1.08 (French)
DRIVER(wpt,108g)        //S.A.M.: 03/06 World Poker Tour - V1.08 (German)
DRIVER(wpt,108i)        //S.A.M.: 03/06 World Poker Tour - V1.08 (Italian)
DRIVER(wpt,108l)        //S.A.M.: 03/06 World Poker Tour - V1.08 (Spanish)
DRIVER(wpt,109a)        //S.A.M.: 03/06 World Poker Tour - V1.09 (English)
DRIVER(wpt,109f)        //S.A.M.: 03/06 World Poker Tour - V1.09 (French)
DRIVER(wpt,109f2)       //S.A.M.: 03/06 World Poker Tour - V1.09.2 (French)
DRIVER(wpt,109g)        //S.A.M.: 03/06 World Poker Tour - V1.09 (German)
DRIVER(wpt,109i)        //S.A.M.: 03/06 World Poker Tour - V1.09 (Italian)
DRIVER(wpt,109l)        //S.A.M.: 03/06 World Poker Tour - V1.09 (Spanish)
DRIVER(wpt,111a)        //S.A.M.: 08/06 World Poker Tour - V1.11 (English)
DRIVER(wpt,111af)       //S.A.M.: 08/06 World Poker Tour - V1.11 (English, French)
DRIVER(wpt,111ai)       //S.A.M.: 08/06 World Poker Tour - V1.11 (English, Italian)
DRIVER(wpt,111al)       //S.A.M.: 08/06 World Poker Tour - V1.11 (English, Spanish)
DRIVER(wpt,111f)        //S.A.M.: 08/06 World Poker Tour - V1.11 (French)
DRIVER(wpt,111g)        //S.A.M.: 08/06 World Poker Tour - V1.11 (German)
DRIVER(wpt,111gf)       //S.A.M.: 08/06 World Poker Tour - V1.11 (German, French)
DRIVER(wpt,111i)        //S.A.M.: 08/06 World Poker Tour - V1.11 (Italian)
DRIVER(wpt,111l)        //S.A.M.: 08/06 World Poker Tour - V1.11 (Spanish)
DRIVER(wpt,112a)        //S.A.M.: 11/06 World Poker Tour - V1.12 (English)
DRIVER(wpt,112af)       //S.A.M.: 11/06 World Poker Tour - V1.12 (English, French)
DRIVER(wpt,112ai)       //S.A.M.: 11/06 World Poker Tour - V1.12 (English, Italian)
DRIVER(wpt,112al)       //S.A.M.: 11/06 World Poker Tour - V1.12 (English, Spanish)
DRIVER(wpt,112f)        //S.A.M.: 11/06 World Poker Tour - V1.12 (French)
DRIVER(wpt,112g)        //S.A.M.: 11/06 World Poker Tour - V1.12 (German)
DRIVER(wpt,112gf)       //S.A.M.: 11/06 World Poker Tour - V1.12 (German, French)
DRIVER(wpt,112i)        //S.A.M.: 11/06 World Poker Tour - V1.12 (Italian)
DRIVER(wpt,112l)        //S.A.M.: 11/06 World Poker Tour - V1.12 (Spanish)
DRIVER(wpt,1129af)      //S.A.M.: 09/06 World Poker Tour - V1.129 (English, French) (might be a beta of the 1.12 build?)
DRIVER(wpt,140a)        //S.A.M.: 01/08 World Poker Tour - V14.0 (English)
DRIVER(wpt,140af)       //S.A.M.: 01/08 World Poker Tour - V14.0 (English, French)
DRIVER(wpt,140ai)       //S.A.M.: 01/08 World Poker Tour - V14.0 (English, Italian)
DRIVER(wpt,140al)       //S.A.M.: 01/08 World Poker Tour - V14.0 (English, Spanish)
DRIVER(wpt,140f)        //S.A.M.: 01/08 World Poker Tour - V14.0 (French)
DRIVER(wpt,140g)        //S.A.M.: 01/08 World Poker Tour - V14.0 (German)
DRIVER(wpt,140gf)       //S.A.M.: 01/08 World Poker Tour - V14.0 (German, French)
DRIVER(wpt,140i)        //S.A.M.: 01/08 World Poker Tour - V14.0 (Italian)
DRIVER(wpt,140l)        //S.A.M.: 01/08 World Poker Tour - V14.0 (Spanish)
DRIVERNV(scarn9nj)      //S.A.M.: ??/06 Simpsons Kooky Carnival (Redemption) - V0.90 New Jersey
//DRIVERNV(scarn100)      //S.A.M.: ??/06 Simpsons Kooky Carnival (Redemption) - V1.00
//DRIVERNV(scarn101)      //S.A.M.: 04/06 Simpsons Kooky Carnival (Redemption) - V1.01
//DRIVERNV(scarn102)      //S.A.M.: 04/06 Simpsons Kooky Carnival (Redemption) - V1.02
DRIVERNV(scarn103)      //S.A.M.: 04/06 Simpsons Kooky Carnival (Redemption) - V1.03
//DRIVERNV(scarn104)      //S.A.M.: 05/06 Simpsons Kooky Carnival (Redemption) - V1.04
DRIVERNV(scarn105)      //S.A.M.: 08/06 Simpsons Kooky Carnival (Redemption) - V1.05
DRIVERNV(scarn200)      //S.A.M.: 02/08 Simpsons Kooky Carnival (Redemption) - V2.0
//DRIVER(potc,103as)      //S.A.M.: 07/06 Pirates of the Caribbean - V1.03 (English, Spanish)
//DRIVER(potc,104as)      //S.A.M.: 07/06 Pirates of the Caribbean - V1.04 (English, Spanish)
//DRIVER(potc,105as)      //S.A.M.: 07/06 Pirates of the Caribbean - V1.05 (English, Spanish)
//DRIVER(potc,106as)      //S.A.M.: 07/06 Pirates of the Caribbean - V1.06 (English, Spanish)
//DRIVER(potc,107as)      //S.A.M.: 07/06 Pirates of the Caribbean - V1.07 (English, Spanish)
DRIVER(potc,108as)      //S.A.M.: 07/06 Pirates of the Caribbean - V1.08 (English, Spanish)
DRIVER(potc,109ai)      //S.A.M.: 08/06 Pirates of the Caribbean - V1.09 (English, Italian)
DRIVER(potc,109as)      //S.A.M.: 08/06 Pirates of the Caribbean - V1.09 (English, Spanish)
DRIVER(potc,109gf)      //S.A.M.: 08/06 Pirates of the Caribbean - V1.09 (German, French)
DRIVER(potc,110af)      //S.A.M.: 08/06 Pirates of the Caribbean - V1.10 (English, French)
DRIVER(potc,110ai)      //S.A.M.: 08/06 Pirates of the Caribbean - V1.10 (English, Italian)
DRIVER(potc,110gf)      //S.A.M.: 08/06 Pirates of the Caribbean - V1.10 (German, French)
DRIVER(potc,111as)      //S.A.M.: 08/06 Pirates of the Caribbean - V1.11 (English, Spanish)
//DRIVER(potc,112as)      //S.A.M.: 08/06 Pirates of the Caribbean - V1.12 (English, Spanish)
DRIVER(potc,113af)      //S.A.M.: 09/06 Pirates of the Caribbean - V1.13 (English, French)
DRIVER(potc,113ai)      //S.A.M.: 09/06 Pirates of the Caribbean - V1.13 (English, Italian)
DRIVER(potc,113as)      //S.A.M.: 09/06 Pirates of the Caribbean - V1.13 (English, Spanish)
DRIVER(potc,113gf)      //S.A.M.: 09/06 Pirates of the Caribbean - V1.13 (German, French)
//DRIVER(potc,114as)      //S.A.M.: 10/06 Pirates of the Caribbean - V1.14 (English, Spanish)
DRIVER(potc,115af)      //S.A.M.: 11/06 Pirates of the Caribbean - V1.15 (English, French)
DRIVER(potc,115ai)      //S.A.M.: 11/06 Pirates of the Caribbean - V1.15 (English, Italian)
DRIVER(potc,115as)      //S.A.M.: 11/06 Pirates of the Caribbean - V1.15 (English, Spanish)
DRIVER(potc,115gf)      //S.A.M.: 11/06 Pirates of the Caribbean - V1.15 (German, French)
//DRIVER(potc,200as)      //S.A.M.: 04/07 Pirates of the Caribbean - V2.00 (English, Spanish)
DRIVER(potc,300af)      //S.A.M.: 04/07 Pirates of the Caribbean - V3.00 (English, French)
DRIVER(potc,300ai)      //S.A.M.: 04/07 Pirates of the Caribbean - V3.00 (English, Italian)
DRIVER(potc,300al)      //S.A.M.: 04/07 Pirates of the Caribbean - V3.00 (English, Spanish)
DRIVER(potc,300gf)      //S.A.M.: 04/07 Pirates of the Caribbean - V3.00 (German, French)
DRIVER(potc,400af)      //S.A.M.: 04/07 Pirates of the Caribbean - V4.00 (English, French)
DRIVER(potc,400ai)      //S.A.M.: 04/07 Pirates of the Caribbean - V4.00 (English, Italian)
DRIVER(potc,400al)      //S.A.M.: 04/07 Pirates of the Caribbean - V4.00 (English, Spanish)
DRIVER(potc,400gf)      //S.A.M.: 04/07 Pirates of the Caribbean - V4.00 (German, French)
//DRIVER(potc,500as)      //S.A.M.: 09/07 Pirates of the Caribbean - V5.00 (English, Spanish)
DRIVER(potc,600af)      //S.A.M.: 01/08 Pirates of the Caribbean - V6.0  (English, French)
DRIVER(potc,600ai)      //S.A.M.: 01/08 Pirates of the Caribbean - V6.0  (English, Italian)
DRIVER(potc,600as)      //S.A.M.: 01/08 Pirates of the Caribbean - V6.0  (English, Spanish)
DRIVER(potc,600gf)      //S.A.M.: 01/08 Pirates of the Caribbean - V6.0  (German, French)
//DRIVER(fg,100a)         //S.A.M.: 02/07 Family Guy - V1.00
//DRIVER(fg,101a)         //S.A.M.: 02/07 Family Guy - V1.01
DRIVER(fg,200a)         //S.A.M.: 02/07 Family Guy - V2.00  (English)
DRIVER(fg,300ai)        //S.A.M.: 02/07 Family Guy - V3.00  (English, Italian)
DRIVER(fg,400a)         //S.A.M.: 02/07 Family Guy - V4.00  (English)
DRIVER(fg,400ag)        //S.A.M.: 02/07 Family Guy - V4.00  (English, German)
//DRIVER(fg,500al)        //S.A.M.: 02/07 Family Guy - V5.00  (English, Spanish)
//DRIVER(fg,600al)        //S.A.M.: 02/07 Family Guy - V6.00  (English, Spanish)
DRIVER(fg,700af)        //S.A.M.: 02/07 Family Guy - V7.00  (English, French)
DRIVER(fg,700al)        //S.A.M.: 02/07 Family Guy - V7.00  (English, Spanish)
DRIVER(fg,800al)        //S.A.M.: 03/07 Family Guy - V8.00  (English, Spanish)
//DRIVER(fg,900al)        //S.A.M.: 03/07 Family Guy - V9.00  (English, Spanish)
DRIVER(fg,1000af)       //S.A.M.: 03/07 Family Guy - V10.00 (English, French)
DRIVER(fg,1000ag)       //S.A.M.: 03/07 Family Guy - V10.00 (English, German)
DRIVER(fg,1000ai)       //S.A.M.: 03/07 Family Guy - V10.00 (English, Italian)
DRIVER(fg,1000al)       //S.A.M.: 03/07 Family Guy - V10.00 (English, Spanish)
DRIVER(fg,1100af)       //S.A.M.: 06/07 Family Guy - V11.0 (English, French)
DRIVER(fg,1100ag)       //S.A.M.: 06/07 Family Guy - V11.0 (English, German)
DRIVER(fg,1100ai)       //S.A.M.: 06/07 Family Guy - V11.0 (English, Italian)
DRIVER(fg,1100al)       //S.A.M.: 06/07 Family Guy - V11.0 (English, Spanish)
DRIVER(fg,1200af)       //S.A.M.: 01/08 Family Guy - V12.0 (English, French)
DRIVER(fg,1200ag)       //S.A.M.: 01/08 Family Guy - V12.0 (English, German)
DRIVER(fg,1200ai)       //S.A.M.: 01/08 Family Guy - V12.0 (English, Italian)
DRIVER(fg,1200al)       //S.A.M.: 01/08 Family Guy - V12.0 (English, Spanish)
DRIVER(sman,102af)      //S.A.M.: 05/07 Spider-Man - V1.02 (English, French)
DRIVER(sman,130af)      //S.A.M.: 06/07 Spider-Man - V1.30 (English, French)
DRIVER(sman,130ai)      //S.A.M.: 06/07 Spider-Man - V1.30 (English, Italian)
DRIVER(sman,130al)      //S.A.M.: 06/07 Spider-Man - V1.30 (English, Spanish)
DRIVER(sman,130gf)      //S.A.M.: 06/07 Spider-Man - V1.30 (German, French)
DRIVER(sman,132)        //S.A.M.: 06/07 Spider-Man - V1.32
DRIVER(sman,140)        //S.A.M.: ??/07 Spider-Man - V1.4
DRIVER(sman,140af)      //S.A.M.: ??/07 Spider-Man - V1.4 (English, French)
DRIVER(sman,140ai)      //S.A.M.: ??/07 Spider-Man - V1.4 (English, Italian)
DRIVER(sman,140al)      //S.A.M.: ??/07 Spider-Man - V1.4 (English, Spanish)
DRIVER(sman,140gf)      //S.A.M.: ??/07 Spider-Man - V1.4 (German, French)
DRIVER(sman,142)        //S.A.M.: ??/07 Spider-Man - V1.42 BETA
DRIVER(sman,160)        //S.A.M.: ??/07 Spider-Man - V1.6
DRIVER(sman,160af)      //S.A.M.: ??/07 Spider-Man - V1.6 (English, French)
DRIVER(sman,160ai)      //S.A.M.: ??/07 Spider-Man - V1.6 (English, Italian)
DRIVER(sman,160al)      //S.A.M.: ??/07 Spider-Man - V1.6 (English, Spanish)
DRIVER(sman,160gf)      //S.A.M.: ??/07 Spider-Man - V1.6 (German, French)
DRIVER(sman,170)        //S.A.M.: ??/07 Spider-Man - V1.7
DRIVER(sman,170af)      //S.A.M.: ??/07 Spider-Man - V1.7 (English, French)
DRIVER(sman,170ai)      //S.A.M.: ??/07 Spider-Man - V1.7 (English, Italian)
DRIVER(sman,170al)      //S.A.M.: ??/07 Spider-Man - V1.7 (English, Spanish)
DRIVER(sman,170gf)      //S.A.M.: ??/07 Spider-Man - V1.7 (German, French)
DRIVER(sman,190)        //S.A.M.: 11/07 Spider-Man - V1.9
DRIVER(sman,190af)      //S.A.M.: 11/07 Spider-Man - V1.9 (English, French)
DRIVER(sman,190ai)      //S.A.M.: 11/07 Spider-Man - V1.9 (English, Italian)
DRIVER(sman,190al)      //S.A.M.: 11/07 Spider-Man - V1.9 (English, Spanish)
DRIVER(sman,190gf)      //S.A.M.: 11/07 Spider-Man - V1.9 (German, French)
DRIVER(sman,192)        //S.A.M.: 01/08 Spider-Man - V1.92
DRIVER(sman,192af)      //S.A.M.: 01/08 Spider-Man - V1.92 (English, French)
DRIVER(sman,192ai)      //S.A.M.: 01/08 Spider-Man - V1.92 (English, Italian)
DRIVER(sman,192al)      //S.A.M.: 01/08 Spider-Man - V1.92 (English, Spanish)
DRIVER(sman,192gf)      //S.A.M.: 01/08 Spider-Man - V1.92 (German, French)
DRIVER(sman,200)        //S.A.M.: 12/08 Spider-Man - V2.0
DRIVER(sman,210)        //S.A.M.: 12/08 Spider-Man - V2.1
DRIVER(sman,210af)      //S.A.M.: 12/08 Spider-Man - V2.1  (English, French)
DRIVER(sman,210ai)      //S.A.M.: 12/08 Spider-Man - V2.1  (English, Italian)
DRIVER(sman,210al)      //S.A.M.: 12/08 Spider-Man - V2.1  (English, Spanish)
DRIVER(sman,210gf)      //S.A.M.: 12/08 Spider-Man - V2.1  (German, French)
DRIVER(sman,220)        //S.A.M.: 04/09 Spider-Man - V2.2
DRIVER(sman,230)        //S.A.M.: 08/09 Spider-Man - V2.3
DRIVER(sman,240)        //S.A.M.: 11/09 Spider-Man - V2.4
DRIVER(sman,250)        //S.A.M.: 08/10 Spider-Man - V2.5
DRIVER(sman,260)        //S.A.M.: 11/10 Spider-Man - V2.6
DRIVER(sman,261)        //S.A.M.: 12/12 Spider-Man - V2.61 (actually released to the public much later though: 05/14)
DRIVER(sman,262)        //S.A.M.: 04/14 Spider-Man - V2.62 (bootleg with replaced music) I was always disappointed that Stern Spiderman didn't use Danny Elfman's theme music. So I hacked it into this rom revision. It plays during the game for one of the sandman modes now.
#ifdef SAM_ORIGINAL
DRIVER(sman,100ve)      //S.A.M.: 02/16 Spider-Man - Vault Edition V1.00
DRIVER(sman,101ve)      //S.A.M.: 05/16 Spider-Man - Vault Edition V1.01
#endif
DRIVER(wof,100)         //S.A.M.: 11/07 Wheel of Fortune - V1.0
DRIVER(wof,200)         //S.A.M.: 11/07 Wheel of Fortune - V2.0
DRIVER(wof,200f)        //S.A.M.: 11/07 Wheel of Fortune - V2.0 (French)
DRIVER(wof,200g)        //S.A.M.: 11/07 Wheel of Fortune - V2.0 (German)
DRIVER(wof,200i)        //S.A.M.: 11/07 Wheel of Fortune - V2.0 (Italian)
DRIVER(wof,300)         //S.A.M.: 12/07 Wheel of Fortune - V3.0
DRIVER(wof,300f)        //S.A.M.: 12/07 Wheel of Fortune - V3.0 (French)
DRIVER(wof,300g)        //S.A.M.: 12/07 Wheel of Fortune - V3.0 (German)
DRIVER(wof,300i)        //S.A.M.: 12/07 Wheel of Fortune - V3.0 (Italian)
DRIVER(wof,300l)        //S.A.M.: 12/07 Wheel of Fortune - V3.0 (Spanish)
DRIVER(wof,400)         //S.A.M.: 12/07 Wheel of Fortune - V4.0
DRIVER(wof,400f)        //S.A.M.: 12/07 Wheel of Fortune - V4.0 (French)
DRIVER(wof,400g)        //S.A.M.: 12/07 Wheel of Fortune - V4.0 (German)
DRIVER(wof,400i)        //S.A.M.: 12/07 Wheel of Fortune - V4.0 (Italian)
DRIVER(wof,401l)        //S.A.M.: 12/07 Wheel of Fortune - V4.01 (Spanish)
DRIVER(wof,500)         //S.A.M.: 12/07 Wheel of Fortune - V5.0
DRIVER(wof,500f)        //S.A.M.: 12/07 Wheel of Fortune - V5.0 (French)
DRIVER(wof,500g)        //S.A.M.: 12/07 Wheel of Fortune - V5.0 (German)
DRIVER(wof,500i)        //S.A.M.: 12/07 Wheel of Fortune - V5.0 (Italian)
DRIVER(wof,500l)        //S.A.M.: 12/07 Wheel of Fortune - V5.0 (Spanish)
DRIVER(wof,602h)        //S.A.M.: 03/09 Wheel of Fortune - V6.02 (Home Rom)
DRIVER(shr,130)         //S.A.M.: 03/08 Shrek - V1.3
//DRIVER(shr,140)         //S.A.M.: 04/08 Shrek - V1.40
DRIVER(shr,141)         //S.A.M.: 04/08 Shrek - V1.41
//DRIVER(ij4,100)         //S.A.M.: 04/08 Indiana Jones - V1.00
//DRIVER(ij4,101)         //S.A.M.: 04/08 Indiana Jones - V1.01
//DRIVER(ij4,102)         //S.A.M.: 04/08 Indiana Jones - V1.02
//DRIVER(ij4,103)         //S.A.M.: 04/08 Indiana Jones - V1.03
//DRIVER(ij4,104)         //S.A.M.: 04/08 Indiana Jones - V1.04
//DRIVER(ij4,105)         //S.A.M.: 04/08 Indiana Jones - V1.05
//DRIVER(ij4,106)         //S.A.M.: 05/08 Indiana Jones - V1.06
//DRIVER(ij4,107)         //S.A.M.: 05/08 Indiana Jones - V1.07
//DRIVER(ij4,108)         //S.A.M.: 05/08 Indiana Jones - V1.08
//DRIVER(ij4,109)         //S.A.M.: 05/08 Indiana Jones - V1.09
//DRIVER(ij4,110)         //S.A.M.: 05/08 Indiana Jones - V1.10
//DRIVER(ij4,111)         //S.A.M.: 05/08 Indiana Jones - V1.11
//DRIVER(ij4,112)         //S.A.M.: 05/08 Indiana Jones - V1.12
DRIVER(ij4,113)         //S.A.M.: 05/08 Indiana Jones - V1.13
DRIVER(ij4,113f)        //S.A.M.: 05/08 Indiana Jones - V1.13 (French)
DRIVER(ij4,113g)        //S.A.M.: 05/08 Indiana Jones - V1.13 (German)
DRIVER(ij4,113i)        //S.A.M.: 05/08 Indiana Jones - V1.13 (Italian)
DRIVER(ij4,113l)        //S.A.M.: 05/08 Indiana Jones - V1.13 (Spanish)
DRIVER(ij4,114)         //S.A.M.: 06/08 Indiana Jones - V1.14
DRIVER(ij4,114f)        //S.A.M.: 06/08 Indiana Jones - V1.14 (French)
DRIVER(ij4,114g)        //S.A.M.: 06/08 Indiana Jones - V1.14 (German)
DRIVER(ij4,114i)        //S.A.M.: 06/08 Indiana Jones - V1.14 (Italian)
DRIVER(ij4,114l)        //S.A.M.: 06/08 Indiana Jones - V1.14 (Spanish)
//DRIVER(ij4,115)         //S.A.M.: 09/08 Indiana Jones - V1.15
DRIVER(ij4,116)         //S.A.M.: 09/08 Indiana Jones - V1.16
DRIVER(ij4,116f)        //S.A.M.: 09/08 Indiana Jones - V1.16 (French)
DRIVER(ij4,116g)        //S.A.M.: 09/08 Indiana Jones - V1.16 (German)
DRIVER(ij4,116i)        //S.A.M.: 09/08 Indiana Jones - V1.16 (Italian)
DRIVER(ij4,116l)        //S.A.M.: 09/08 Indiana Jones - V1.16 (Spanish)
//DRIVER(ij4,200)         //S.A.M.: 12/08 Indiana Jones - V2.00
DRIVER(ij4,210)         //S.A.M.: 01/09 Indiana Jones - V2.1
DRIVER(ij4,210f)        //S.A.M.: 01/09 Indiana Jones - V2.1 (French)
DRIVER(bdk,130)         //S.A.M.: 07/08 Batman - The Dark Knight - V1.3
//DRIVER(bdk,140)         //S.A.M.: 07/08 Batman - The Dark Knight - V1.4
DRIVER(bdk,150)         //S.A.M.: 07/08 Batman - The Dark Knight - V1.5
DRIVER(bdk,160)         //S.A.M.: 07/08 Batman - The Dark Knight - V1.6
DRIVER(bdk,200)         //S.A.M.: 08/08 Batman - The Dark Knight - V2.0
DRIVER(bdk,210)         //S.A.M.: 08/08 Batman - The Dark Knight - V2.1
DRIVER(bdk,220)         //S.A.M.: 08/08 Batman - The Dark Knight - V2.2
//DRIVER(bdk,230)         //S.A.M.: 10/08 Batman - The Dark Knight - V2.3
DRIVER(bdk,240)         //S.A.M.: 11/09 Batman - The Dark Knight - V2.4  (scarecrow sometimes 2 balls, sometimes 3 balls)
DRIVER(bdk,290)         //S.A.M.: 05/10 Batman - The Dark Knight - V2.9  (with 3 ball scarecrow multiball)
DRIVER(bdk,294)         //S.A.M.: 05/10 Batman - The Dark Knight - V2.94 (back to 2 ball scarecrow multiball)
#ifdef SAM_INCLUDE_COLORED
DRIVER(bdk,294c)        //S.A.M.: 05/10 Batman - The Dark Knight - V2.94 (back to 2 ball scarecrow multiball)
#endif
DRIVER(bdk,300)         //S.A.M.: ??/10 Batman - The Dark Knight - V3.00 Home Edition/Costco
//DRIVER(csi,100)         //S.A.M.: 11/08 CSI: Crime Scene Investigation - V1.00
//DRIVER(csi,101)         //S.A.M.: 11/08 CSI: Crime Scene Investigation - V1.01
DRIVER(csi,102)         //S.A.M.: 11/08 CSI: Crime Scene Investigation - V1.02
DRIVER(csi,103)         //S.A.M.: 11/08 CSI: Crime Scene Investigation - V1.03
DRIVER(csi,104)         //S.A.M.: 11/08 CSI: Crime Scene Investigation - V1.04
DRIVER(csi,200)         //S.A.M.: 12/08 CSI: Crime Scene Investigation - V2.0
DRIVER(csi,210)         //S.A.M.: 01/09 CSI: Crime Scene Investigation - V2.1
DRIVER(csi,230)         //S.A.M.: 08/09 CSI: Crime Scene Investigation - V2.3
DRIVER(csi,240)         //S.A.M.: 08/09 CSI: Crime Scene Investigation - V2.4
//DRIVER(twenty4,100)     //S.A.M.: 02/09 24 - V1.0
//DRIVER(twenty4,110)     //S.A.M.: 02/09 24 - V1.1
//DRIVER(twenty4,120)     //S.A.M.: 02/09 24 - V1.2
DRIVER(twenty4,130)     //S.A.M.: 03/09 24 - V1.3
DRIVER(twenty4,140)     //S.A.M.: 03/09 24 - V1.4
DRIVER(twenty4,144)     //S.A.M.: 09/09 24 - V1.44
DRIVER(twenty4,150)     //S.A.M.: 05/10 24 - V1.5
//DRIVER(nba,100)         //S.A.M.: 05/09 NBA - V1.00
//DRIVER(nba,200)         //S.A.M.: 05/09 NBA - V2.00
//DRIVER(nba,300)         //S.A.M.: 05/09 NBA - V3.00
//DRIVER(nba,400)         //S.A.M.: 05/09 NBA - V4.00
DRIVER(nba,500)         //S.A.M.: 05/09 NBA - V5.0
DRIVER(nba,600)         //S.A.M.: 06/09 NBA - V6.0
DRIVER(nba,700)         //S.A.M.: 06/09 NBA - V7.0
DRIVER(nba,801)         //S.A.M.: 08/09 NBA - V8.01
DRIVER(nba,802)         //S.A.M.: 11/09 NBA - V8.02
//DRIVER(bbh,100)         //S.A.M.: 01/10 Big Buck Hunter Pro - V1.00
//DRIVER(bbh,110)         //S.A.M.: 01/10 Big Buck Hunter Pro - V1.10
//DRIVER(bbh,120)         //S.A.M.: 01/10 Big Buck Hunter Pro - V1.20
//DRIVER(bbh,130)         //S.A.M.: 01/10 Big Buck Hunter Pro - V1.30
DRIVER(bbh,140)         //S.A.M.: 02/10 Big Buck Hunter Pro - V1.4
DRIVER(bbh,150)         //S.A.M.: 02/10 Big Buck Hunter Pro - V1.5
DRIVER(bbh,160)         //S.A.M.: 05/10 Big Buck Hunter Pro - V1.6
DRIVER(bbh,170)         //S.A.M.: 11/10 Big Buck Hunter Pro - V1.7
DRIVER(im,100)          //S.A.M.: 04/10 Iron Man - V1.0
DRIVER(im,110)          //S.A.M.: 04/10 Iron Man - V1.1
DRIVER(im,120)          //S.A.M.: 04/10 Iron Man - V1.2
//DRIVER(im,130)          //S.A.M.: 04/10 Iron Man - V1.3
DRIVER(im,140)          //S.A.M.: 04/10 Iron Man - V1.4
//DRIVER(im,150)          //S.A.M.: 09/10 Iron Man - V1.5
DRIVER(im,160)          //S.A.M.: 11/11 Iron Man - V1.6
//DRIVER(im,180)          //S.A.M.: 07/14 Iron Man - V1.8
DRIVER(im,181)          //S.A.M.: 07/14 Iron Man - V1.81
DRIVER(im,182)          //S.A.M.: 07/14 Iron Man - V1.82
DRIVER(im,183)          //S.A.M.: 08/14 Iron Man - V1.83
DRIVER(im,183ve)        //S.A.M.: 08/14 Iron Man - V1.83 Vault Edition
DRIVER(im,185)          //S.A.M.: 03/20 Iron Man - V1.85
DRIVER(im,185ve)        //S.A.M.: 03/20 Iron Man - V1.85 Vault Edition
DRIVER(im,186)          //S.A.M.: 04/20 Iron Man - V1.86
DRIVER(im,186ve)        //S.A.M.: 04/20 Iron Man - V1.86 Vault Edition
//DRIVER(avr,100)         //S.A.M.: 08/10 Avatar - V1.00
//DRIVER(avr,100h)        //S.A.M.: 12/10 Avatar - V1.00 Limited Edition
//DRIVER(avr,101)         //S.A.M.: 08/10 Avatar - V1.01
DRIVER(avr,101h)        //S.A.M.: 12/10 Avatar - V1.01 Limited Edition
//DRIVER(avr,102)         //S.A.M.: 08/10 Avatar - V1.02
//DRIVER(avr,103)         //S.A.M.: 09/10 Avatar - V1.03
//DRIVER(avr,104)         //S.A.M.: 09/10 Avatar - V1.04
//DRIVER(avr,105)         //S.A.M.: 09/10 Avatar - V1.05
DRIVER(avr,106)         //S.A.M.: 10/10 Avatar - V1.06
DRIVER(avr,110)         //S.A.M.: 11/11 Avatar - V1.1
DRIVER(avr,120h)        //S.A.M.: 11/11 Avatar - V1.2 Limited Edition
DRIVER(avr,200)         //S.A.M.: 01/13 Avatar - V2.0 (New CPU)
//DRIVER(rsn,100)         //S.A.M.: 02/11 Rolling Stones - V1.0
DRIVER(rsn,100h)        //S.A.M.: 04/11 Rolling Stones - V1.0 Limited Edition
//DRIVER(rsn,101)         //S.A.M.: 02/11 Rolling Stones - V1.01
//DRIVER(rsn,102)         //S.A.M.: 03/11 Rolling Stones - V1.02
DRIVER(rsn,103)         //S.A.M.: 03/11 Rolling Stones - V1.03
//DRIVER(rsn,104)         //S.A.M.: 03/11 Rolling Stones - V1.04
DRIVER(rsn,105)         //S.A.M.: 03/11 Rolling Stones - V1.05
DRIVER(rsn,110)         //S.A.M.: 11/11 Rolling Stones - V1.1
DRIVER(rsn,110h)        //S.A.M.: 11/11 Rolling Stones - V1.1 Limited Edition
//DRIVER(trn,100)         //S.A.M.: 05/11 TRON: Legacy - V1.0
DRIVER(trn,100h)        //S.A.M.: 06/11 TRON: Legacy - V1.0 Limited Edition
DRIVER(trn,110)         //S.A.M.: 05/11 TRON: Legacy - V1.10
DRIVER(trn,110h)        //S.A.M.: 07/11 TRON: Legacy - V1.1 Limited Edition
DRIVER(trn,120)         //S.A.M.: 06/11 TRON: Legacy - V1.2
DRIVER(trn,130h)        //S.A.M.: 07/11 TRON: Legacy - V1.3 Limited Edition (Stern skipped over TRON: Legacy LE 1.2)
DRIVER(trn,140)         //S.A.M.: 06/11 TRON: Legacy - V1.4 (Stern skipped over TRON: Legacy 1.3)
DRIVER(trn,140h)        //S.A.M.: 11/11 TRON: Legacy - V1.4 Limited Edition
DRIVER(trn,150)         //S.A.M.: 06/11 TRON: Legacy - V1.5
DRIVER(trn,160)         //S.A.M.: 08/11 TRON: Legacy - V1.6
DRIVER(trn,170)         //S.A.M.: 11/11 TRON: Legacy - V1.7
DRIVER(trn,174)         //S.A.M.: 02/13 TRON: Legacy - V1.74
DRIVER(trn,174h)        //S.A.M.: 11/13 TRON: Legacy - V1.74 Limited Edition
DRIVER(trn,17402)       //S.A.M.: 02/13 TRON: Legacy - V1.7402 (New CPU)
DRIVER(tf,088h)         //S.A.M.: ??/11 Transformers - V0.88 Limited Edition
//DRIVER(tf,100)          //S.A.M.: 10/11 Transformers - V1.0
DRIVER(tf,100h)         //S.A.M.: 11/11 Transformers - V1.0 Limited Edition
//DRIVER(tf,110)          //S.A.M.: 10/11 Transformers - V1.1
//DRIVER(tf,110h)         //S.A.M.: 12/11 Transformers - V1.1 Limited Edition
DRIVER(tf,120)          //S.A.M.: 10/11 Transformers - V1.2
DRIVER(tf,120h)         //S.A.M.: 12/11 Transformers - V1.2 Limited Edition
//DRIVER(tf,121h)         //S.A.M.: 01/12 Transformers - V1.21 Limited Edition
//DRIVER(tf,130)          //S.A.M.: 12/11 Transformers - V1.3
DRIVER(tf,130h)         //S.A.M.: 01/12 Transformers - V1.3 Limited Edition
DRIVER(tf,140)          //S.A.M.: 12/11 Transformers - V1.4
DRIVER(tf,140h)         //S.A.M.: 01/12 Transformers - V1.4 Limited Edition
DRIVER(tf,150)          //S.A.M.: 01/12 Transformers - V1.5
DRIVER(tf,150h)         //S.A.M.: 03/12 Transformers - V1.5 Limited Edition
DRIVER(tf,160)          //S.A.M.: 01/12 Transformers - V1.6
DRIVER(tf,170)          //S.A.M.: 03/12 Transformers - V1.7
DRIVER(tf,180)          //S.A.M.: 03/13 Transformers - V1.8
DRIVER(tf,180h)         //S.A.M.: 03/13 Transformers - V1.8 Limited Edition
// Note that AC/DC vault edition has no special ROM like the previous VEs
DRIVER(acd,121)         //S.A.M.: 02/12 AC/DC - V1.21
//DRIVER(acd,121h)        //S.A.M.: 02/12 AC/DC - V1.21 Limited Edition
DRIVER(acd,125)         //S.A.M.: 0?/12 AC/DC - V1.25
DRIVER(acd,130)         //S.A.M.: 0?/12 AC/DC - V1.3
DRIVER(acd,140)         //S.A.M.: 03/12 AC/DC - V1.4
//DRIVER(acd,140h)        //S.A.M.: 03/12 AC/DC - V1.4 Limited Edition
DRIVER(acd,150)         //S.A.M.: 04/12 AC/DC - V1.5
DRIVER(acd,150h)        //S.A.M.: 04/12 AC/DC - V1.5 Limited Edition
DRIVER(acd,152)         //S.A.M.: 05/12 AC/DC - V1.52
DRIVER(acd,152h)        //S.A.M.: 05/12 AC/DC - V1.52 Limited Edition
DRIVER(acd,160)         //S.A.M.: 09/12 AC/DC - V1.6
DRIVER(acd,160h)        //S.A.M.: 09/12 AC/DC - V1.6 Limited Edition
DRIVER(acd,161)         //S.A.M.: 10/12 AC/DC - V1.61
DRIVER(acd,161h)        //S.A.M.: 10/12 AC/DC - V1.61 Limited Edition
DRIVER(acd,163)         //S.A.M.: 01/13 AC/DC - V1.63
DRIVER(acd,163h)        //S.A.M.: 01/13 AC/DC - V1.63 Limited Edition
DRIVER(acd,165)         //S.A.M.: 03/13 AC/DC - V1.65
DRIVER(acd,165h)        //S.A.M.: 03/13 AC/DC - V1.65 Limited Edition
DRIVER(acd,168)         //S.A.M.: 06/14 AC/DC - V1.68
DRIVER(acd,168h)        //S.A.M.: 06/14 AC/DC - V1.68 Limited Edition
DRIVER(acd,170)         //S.A.M.: 02/18 AC/DC - V1.70.0
DRIVER(acd,170h)        //S.A.M.: 02/18 AC/DC - V1.70.0 Limited Edition
#ifdef SAM_INCLUDE_COLORED
DRIVER(acd,168c)        // pinball browser colorized using extend memory
DRIVER(acd,168hc)       // pinball browser colorized using extend memory
DRIVER(acd,170c)        // pinball browser colorized using extend memory
DRIVER(acd,170hc)       // pinball browser colorized using extend memory
#endif
DRIVER(xmn,100)         //S.A.M.: 0?/12 X-Men - V1.0
DRIVER(xmn,102)         //S.A.M.: 09/12 X-Men - V1.02
DRIVER(xmn,104)         //S.A.M.: 12/12 X-Men - V1.04
DRIVER(xmn,105)         //S.A.M.: 03/13 X-Men - V1.05
DRIVER(xmn,120h)        //S.A.M.: 08/12 X-Men - V1.2 Limited Edition
DRIVER(xmn,121h)        //S.A.M.: 09/12 X-Men - V1.21 Limited Edition
DRIVER(xmn,122h)        //S.A.M.: ??/12 X-Men - V1.22 Limited Edition
DRIVER(xmn,123h)        //S.A.M.: 12/12 X-Men - V1.23 Limited Edition
DRIVER(xmn,124h)        //S.A.M.: 03/13 X-Men - V1.24 Limited Edition
DRIVER(xmn,130)         //S.A.M.: 06/13 X-Men - V1.3
DRIVER(xmn,130h)        //S.A.M.: 06/13 X-Men - V1.3 Limited Edition
DRIVER(xmn,150)         //S.A.M.: 02/14 X-Men - V1.5
DRIVER(xmn,150h)        //S.A.M.: 02/14 X-Men - V1.5 Limited Edition
DRIVER(xmn,151)         //S.A.M.: 02/14 X-Men - V1.51
DRIVER(xmn,151h)        //S.A.M.: 02/14 X-Men - V1.51 Limited Edition
#ifdef SAM_INCLUDE_COLORED
DRIVER(xmn,151c)        // pinball browser colorized using extend memory
DRIVER(xmn,151hc)       // pinball browser colorized using extend memory
#endif
//DRIVER(avs,100)         //S.A.M.: 11/12 Avengers - V1.0
//DRIVER(avs,100h)        //S.A.M.: 12/12 Avengers - V1.0 Limited Edition
DRIVER(avs,110)         //S.A.M.: 11/12 Avengers - V1.1
//DRIVER(avs,110h)        //S.A.M.: 12/12 Avengers - V1.1 Limited Edition
//DRIVER(avs,111)         //S.A.M.: 12/12 Avengers - V1.11
//DRIVER(avs,112)         //S.A.M.: 12/12 Avengers - V1.12
DRIVER(avs,120h)        //S.A.M.: 12/12 Avengers - V1.2 Limited Edition
DRIVER(avs,140)         //S.A.M.: 02/13 Avengers - V1.4
DRIVER(avs,140h)        //S.A.M.: 02/13 Avengers - V1.4 Limited Edition
DRIVER(avs,170)         //S.A.M.: 01/16 Avengers - V1.7
DRIVER(avs,170h)        //S.A.M.: 01/16 Avengers - V1.7 Limited Edition
#ifdef SAM_INCLUDE_COLORED
DRIVER(avs,170c)        // pinball browser colorized using extend memory
DRIVER(avs,170hc)       // pinball browser colorized using extend memory
#endif
//DRIVER(mtl,100)         //S.A.M.: 04/13 Metallica - V1.0
//DRIVER(mtl,102)         //S.A.M.: 05/13 Metallica - V1.02
DRIVER(mtl,103)         //S.A.M.: 05/13 Metallica - V1.03
//DRIVER(mtl,104)         //S.A.M.: 05/13 Metallica - V1.04
DRIVER(mtl,105)         //S.A.M.: 05/13 Metallica - V1.05
DRIVER(mtl,106)         //S.A.M.: 05/13 Metallica - V1.06
//DRIVER(mtl,110h)        //S.A.M.: 06/13 Metallica - V1.1 Limited Edition
//DRIVER(mtl,111h)        //S.A.M.: 06/13 Metallica - V1.11 Limited Edition
DRIVER(mtl,112)         //S.A.M.: 0?/13 Metallica - V1.12
DRIVER(mtl,113)         //S.A.M.: 06/13 Metallica - V1.13
DRIVER(mtl,113h)        //S.A.M.: 06/13 Metallica - V1.13 Limited Edition
DRIVER(mtl,116)         //S.A.M.: 06/13 Metallica - V1.16
DRIVER(mtl,116h)        //S.A.M.: 06/13 Metallica - V1.16 Limited Edition
DRIVER(mtl,120)         //S.A.M.: 07/13 Metallica - V1.2
DRIVER(mtl,120h)        //S.A.M.: 07/13 Metallica - V1.2 Limited Edition
DRIVER(mtl,122)         //S.A.M.: 08/13 Metallica - V1.22
DRIVER(mtl,122h)        //S.A.M.: 08/13 Metallica - V1.22 Limited Edition
DRIVER(mtl,150)         //S.A.M.: 03/14 Metallica - V1.5
DRIVER(mtl,150h)        //S.A.M.: 03/14 Metallica - V1.5 Limited Edition
DRIVER(mtl,151)         //S.A.M.: 03/14 Metallica - V1.51
DRIVER(mtl,151h)        //S.A.M.: 03/14 Metallica - V1.51 Limited Edition
DRIVER(mtl,160)         //S.A.M.: 06/14 Metallica - V1.6
DRIVER(mtl,160h)        //S.A.M.: 06/14 Metallica - V1.6 Limited Edition
DRIVER(mtl,163)         //S.A.M.: 09/14 Metallica - V1.63
DRIVER(mtl,163d)        //                                    LED version
DRIVER(mtl,163h)        //S.A.M.: 09/14 Metallica - V1.63 Limited Edition
DRIVER(mtl,164)         //S.A.M.: 04/15 Metallica - V1.64
DRIVER(mtl,164h)        //S.A.M.: 04/15 Metallica - V1.64 Limited Edition
DRIVER(mtl,170)         //S.A.M.: 06/16 Metallica - V1.7
DRIVER(mtl,170h)        //S.A.M.: 06/16 Metallica - V1.7 Limited Edition
DRIVER(mtl,180)         //S.A.M.: 11/18 Metallica - V1.80.0
DRIVER(mtl,180h)        //S.A.M.: 11/18 Metallica - V1.80.0 Limited Edition
#ifdef SAM_INCLUDE_COLORED
DRIVER(mtl,164c)        // pinball browser colorized using extend memory
DRIVER(mtl,164hc)       // pinball browser colorized using extend memory
DRIVER(mtl,170c)        // pinball browser colorized using extend memory
DRIVER(mtl,170hc)       // pinball browser colorized using extend memory
DRIVER(mtl,180c)        // pinball browser colorized using extend memory
DRIVER(mtl,180hc)       // pinball browser colorized using extend memory
#endif
//DRIVER(st,100)          //S.A.M.: 09/13 Star Trek - V1.0
//DRIVER(st,101)          //S.A.M.: 09/13 Star Trek - V1.01
DRIVER(st,120)          //S.A.M.: 09/13 Star Trek - V1.2
DRIVER(st,130)          //S.A.M.: 10/13 Star Trek - V1.3
//DRIVER(st,130h)         //S.A.M.: 11/13 Star Trek - V1.3 Limited Edition
DRIVER(st,140)          //S.A.M.: 12/13 Star Trek - V1.4
DRIVER(st,140h)         //S.A.M.: 12/13 Star Trek - V1.4 Limited Edition
DRIVER(st,141h)         //S.A.M.: 12/13 Star Trek - V1.41 Limited Edition
DRIVER(st,142h)         //S.A.M.: 02/14 Star Trek - V1.42 Limited Edition
DRIVER(st,150)          //S.A.M.: 08/14 Star Trek - V1.5
DRIVER(st,150h)         //S.A.M.: 08/14 Star Trek - V1.5 Limited Edition
//DRIVER(st,152)          //S.A.M.: 03/15 Star Trek - V1.52
//DRIVER(st,152h)         //S.A.M.: 03/15 Star Trek - V1.52 Limited Edition
//DRIVER(st,153)          //S.A.M.: 03/15 Star Trek - V1.53
//DRIVER(st,153h)         //S.A.M.: 03/15 Star Trek - V1.53 Limited Edition
//DRIVER(st,154)          //S.A.M.: 03/15 Star Trek - V1.54
//DRIVER(st,154h)         //S.A.M.: 03/15 Star Trek - V1.54 Limited Edition
DRIVER(st,160)          //S.A.M.: 03/15 Star Trek - V1.6
DRIVER(st,160h)         //S.A.M.: 03/15 Star Trek - V1.6 Limited Edition
DRIVER(st,161)          //S.A.M.: 03/15 Star Trek - V1.61
DRIVER(st,161h)         //S.A.M.: 03/15 Star Trek - V1.61 Limited Edition
DRIVER(st,162)          //S.A.M.: 06/18 Star Trek - V1.62
DRIVER(st,162h)         //S.A.M.: 06/18 Star Trek - V1.62 Limited Edition
#ifdef SAM_INCLUDE_COLORED
DRIVER(st,161c)         // pinball browser colorized using extend memory
DRIVER(st,161hc)        // pinball browser colorized using extend memory
DRIVER(st,162c)         // pinball browser colorized using extend memory
DRIVER(st,162hc)        // pinball browser colorized using extend memory
#endif
//DRIVER(mt,100)          //S.A.M.: 03/14 Mustang - V1.0
//DRIVER(mt,101)          //S.A.M.: 03/14 Mustang - V1.01
//DRIVER(mt,102)          //S.A.M.: 03/14 Mustang - V1.02
//DRIVER(mt,103)          //S.A.M.: 03/14 Mustang - V1.03
//DRIVER(mt,104)          //S.A.M.: 03/14 Mustang - V1.04
//DRIVER(mt,110)          //S.A.M.: 03/14 Mustang - V1.10
//DRIVER(mt,111)          //S.A.M.: 03/14 Mustang - V1.11
//DRIVER(mt,112)          //S.A.M.: 03/14 Mustang - V1.12
//DRIVER(mt,113)          //S.A.M.: 04/14 Mustang - V1.13
DRIVER(mt,120)          //S.A.M.: 04/14 Mustang - V1.2
DRIVER(mt,130)          //S.A.M.: 05/14 Mustang - V1.3
DRIVER(mt,130h)         //S.A.M.: 05/14 Mustang - V1.3 Limited Edition
//DRIVER(mt,130hb)         //S.A.M.: 05/14 Mustang - V1.3 Boss
DRIVER(mt,140)          //S.A.M.: 10/14 Mustang - V1.4
DRIVER(mt,140h)         //S.A.M.: 10/14 Mustang - V1.4 Limited Edition
DRIVER(mt,140hb)        //S.A.M.: 10/14 Mustang - V1.4 Boss
DRIVER(mt,145)          //S.A.M.: 02/16 Mustang - V1.45
DRIVER(mt,145h)         //S.A.M.: 02/16 Mustang - V1.45 Limited Edition
DRIVER(mt,145hb)        //S.A.M.: 02/16 Mustang - V1.45 Boss
#ifdef SAM_INCLUDE_COLORED
DRIVER(mt,145c)         // pinball browser colorized using extend memory
DRIVER(mt,145hc)        // pinball browser colorized using extend memory
#endif
//DRIVER(twd,100)         //S.A.M.: 09/14 Walking Dead, The - V1.0
//DRIVER(twd,101)         //S.A.M.: 09/14 Walking Dead, The - V1.01
//DRIVER(twd,103)         //S.A.M.: 09/14 Walking Dead, The - V1.03
DRIVER(twd,105)         //S.A.M.: 10/14 Walking Dead, The - V1.05
//DRIVER(twd,107)         //S.A.M.: 10/14 Walking Dead, The - V1.07
DRIVER(twd,111)         //S.A.M.: 10/14 Walking Dead, The - V1.11
DRIVER(twd,111h)        //S.A.M.: 10/14 Walking Dead, The - V1.11 Limited Edition
DRIVER(twd,119)         //S.A.M.: 11/14 Walking Dead, The - V1.19
DRIVER(twd,119h)        //S.A.M.: 11/14 Walking Dead, The - V1.19 Limited Edition
DRIVER(twd,124)         //S.A.M.: 05/15 Walking Dead, The - V1.24
DRIVER(twd,124h)        //S.A.M.: 05/15 Walking Dead, The - V1.24 Limited Edition
DRIVER(twd,125)         //S.A.M.: 05/15 Walking Dead, The - V1.25
DRIVER(twd,125h)        //S.A.M.: 05/15 Walking Dead, The - V1.25 Limited Edition
DRIVER(twd,128)         //S.A.M.: 07/15 Walking Dead, The - V1.28
DRIVER(twd,128h)        //S.A.M.: 07/15 Walking Dead, The - V1.28 Limited Edition
DRIVER(twd,141)         //S.A.M.: 08/15 Walking Dead, The - V1.41
DRIVER(twd,141h)        //S.A.M.: 08/15 Walking Dead, The - V1.41 Limited Edition
DRIVER(twd,153)         //S.A.M.: 11/15 Walking Dead, The - V1.53
DRIVER(twd,153h)        //S.A.M.: 11/15 Walking Dead, The - V1.53 Limited Edition
DRIVER(twd,156)         //S.A.M.: 12/15 Walking Dead, The - V1.56
DRIVER(twd,156h)        //S.A.M.: 12/15 Walking Dead, The - V1.56 Limited Edition
DRIVER(twd,160)         //S.A.M.: 11/17 Walking Dead, The - V1.60.0
DRIVER(twd,160h)        //S.A.M.: 11/17 Walking Dead, The - V1.60.0 Limited Edition
#ifdef SAM_INCLUDE_COLORED
DRIVER(twd,156c)        // pinball browser colorized using extend memory
DRIVER(twd,156hc)       // pinball browser colorized using extend memory
DRIVER(twd,160c)        // pinball browser colorized using extend memory
DRIVER(twd,160hc)       // pinball browser colorized using extend memory
#endif
#ifndef SAM_ORIGINAL
DRIVER(smanve,100)      //S.A.M.: 02/16 Spider-Man - V1.0 Vault Edition
DRIVER(smanve,101)      //S.A.M.: 05/16 Spider-Man - V1.01 Vault Edition
#ifdef SAM_INCLUDE_COLORED
DRIVER(smanve,100c)     // pinball browser colorized using extend memory
DRIVER(smanve,101c)     // pinball browser colorized using extend memory
#endif
#endif

// ---------------
// TABART GAMES
// ---------------
                        //??/84 Sahara Love
                        //??/85 Le Grand 8
DRIVERNV(hexagone)      //04/86 L'Hexagone

// ---------------
// TAITO GAMES
// ---------------
                        //??/?? Apache!
                        //??/?? Football (W World Cup'78, 05/78)
                        //??/79 Hot Ball (B Eight Ball, 01/77)
DRIVERNV(shock   )      //??/79 Shock (W Flash, 01/79)
                        //??/?? Sultan (G Sinbad, 05/78)
                        //??/78 Roman Victory (Redemption)
                        //??/78 Space Patrol
DRIVERNV(obaoba  )      //??/80 Oba-Oba (B Playboy, 10/77)
DRIVERNV(obaoba1 )      //??/80 Oba-Oba alternate set
DRIVERNV(obaobao )      //??/80 Oba-Oba (old hardware)
DRIVERNV(drakor  )      //??/80 Drakor (W Gorgar, 11/79)
DRIVERNV(meteort )      //??/80 Meteor (S Meteor, 09/79)
DRIVERNV(fireact )      //??/81 Fire Action (W Firepower, 02/80)
DRIVERNV(cavnegro)      //??/81 Cavaleiro Negro (W Black Knight, 11/80)
DRIVERNV(cavnegr1)      //??/81 Cavaleiro Negro alternate set 1
DRIVERNV(cavnegr2)      //??/81 Cavaleiro Negro alternate set 2
DRIVERNV(sureshot)      //??/81 Sure Shot (B Eight Ball Deluxe, 09/80)
DRIVERNV(vegast  )      //??/81 Vegas (B Mata Hari, 09/77)
DRIVERNV(ladylukt)      //??/81 Lady Luck (B Mata Hari, 09/77)
DRIVERNV(cosmic  )      //??/81 Cosmic (S Galaxy, 01/80)
DRIVERNV(gemini  )      //??/82 Gemini 2000 (B Centaur, 02/81)
DRIVERNV(gemini1 )      //??/82 Gemini 2000 alternate set
DRIVERNV(vortex  )      //??/82 Vortex (W Blackout, 06/80)
DRIVERNV(titan   )      //??/82 Titan (W Barracora, 09/81)
DRIVERNV(titan1  )      //??/82 Titan alternate set
DRIVERNV(zarza   )      //??/82 Zarza (B Xenon, 11/79)
DRIVERNV(zarza1  )      //??/82 Zarza alternate set
DRIVERNV(sharkt  )      //??/82 Shark
DRIVERNV(hawkman )      //??/82 Hawkman (B Fathom, 12/80)
DRIVERNV(hawkman1)      //??/82 Hawkman alternate set
DRIVERNV(stest   )      //??/82 Speed Test
DRIVERNV(lunelle )      //??/82 Lunelle (W Alien Poker, 10/80)
DRIVERNV(rally   )      //??/82 Rally (B Skateball, 04/80)
DRIVERNV(snake   )      //??/82 Snake Machine
DRIVERNV(gork    )      //??/82 Gork
DRIVERNV(voleybal)      //??/?? Volley
                        //??/8? Ogar
DRIVERNV(icecold)       //??/83 Ice Cold Beer (Redemption)
DRIVERNV(icecoldf)      //??/83 Ice Cold Beer (Redemption Free Play)
DRIVERNV(zekepeak)      //??/83 Zeke's Peak (Redemption)
DRIVERNV(mrblack )      //??/84 Mr. Black (W Defender, 12/82)
DRIVERNV(mrblack1)      //??/84 Mr. Black alternate set
DRIVERNV(mrblkz80)      //??/8? Mr. Black (Z-80 CPU)
DRIVERNV(fireactd)      //??/8? Fire Action De Luxe (W Firepower II, 08/83)
DRIVERNV(sshuttle)      //??/85 Space Shuttle (W Space Shuttle, 12/84)
DRIVERNV(sshuttl1)      //??/85 Space Shuttle alternate set
DRIVERNV(polar   )      //??/8? Polar Explorer
DRIVERNV(taitest )      //??/8? Test Fixture

// ----------------
// TECNOPLAY GAMES
// ----------------
DRIVERNV(scram_tp)      //03/87 Scramble
DRIVERNV(xforce)        //??/87 X Force
DRIVERNV(spcteam)       //??/88 Space Team

// ----------------
// UNITED GAMES
// ----------------
DRIVERNV(bbbowlin)      //Big Ball Bowling - using Bally hardware

// -----------------------------------
// VALLEY GAMES
// -----------------------------------
DRIVERNV(spectra)       //??/79 Spectra IV

// -----------------------------------
// VIDEODENS GAMES
// -----------------------------------
DRIVERNV(ator)          //??/?? Ator
DRIVERNV(papillon)      //??/86 Papillon
DRIVERNV(break)         //??/86 Break

// -----------------------------------
// WICO GAMES
// -----------------------------------
//                      //11/77 Big Top (home model)
DRIVERNV(aftor)         //12/84 Af-Tor

// -----------------------------------
// WILLIAMS & WILLIAMS/BALLY GAMES
// -----------------------------------
//System 1
                        //S1-468:   12/76 W Grand Prix
//System 2
                        //S2-466:   07/76 W Aztec
//System 3
DRIVER(httip,l1)        //S3-477:   11/77 W Hot Tip
DRIVER(lucky,l1)        //S3-480:   03/78 W Lucky Seven
DRIVER(cntct,l1)        //S3-482:   05/78 W Contact
DRIVER(wldcp,l1)        //S3-481:   05/78 W World Cup
DRIVER(disco,l1)        //S3-483:   08/78 W Disco Fever
//System 4
DRIVER(pkrno,l1)        //S4-488:   10/78 W Pokerino
DRIVER(phnix,l1)        //S4-485:   11/78 W Phoenix
DRIVER(flash,l1)        //S4-486:   01/79 W Flash (L-1, green flipper ROMs)
DRIVER(flash,l2)        //          01/79 W Flash (L-2, yellow flipper ROMs)
DRIVER(flash,t1)        //                  Flash /10 Scoring Ted Estes
DRIVER(stlwr,l2)        //S4-490:   03/79 W Stellar Wars
                        //S?-491:   06/79 W Rock'N Roll
//System 5
DRIVER(pomp,l1)         //S5-???:   ??/78 W Pompeii (Shuffle)
DRIVER(topaz,l1)        //S5-???:   ??/78 W Topaz (Shuffle)
DRIVER(arist,l1)        //S5-???:   ??/79 W Aristocrat (Shuffle)
DRIVER(taurs,l1)        //S5-???:   ??/79 W Taurus (Shuffle)
DRIVER(kingt,l1)        //S5-???:   ??/79 W King Tut (Shuffle)
//System 6
DRIVER(trizn,l1)        //S6-487:   07/79 W TriZone
DRIVER(trizn,t1)        //                  TriZone /10 Scoring Ted Estes
DRIVER(tmwrp,l2)        //S6-489:   09/79 W Time Warp (L-2)
DRIVER(tmwrp,t2)        //                  Time Warp (L-2) /10 Scoring Ted Estes
DRIVER(tmwrp,l3)        //                  Time Warp (L-3)
DRIVER(grgar,l1)        //S6-496:   11/79 W Gorgar
DRIVER(grgar,t1)        //                  Gorgar /10 Scoring Ted Estes
DRIVER(grgar,c1)        //                  Gorgar (Lane Change MOD)
DRIVER(lzbal,l2)        //S6-493:   12/79 W Laser Ball
DRIVER(lzbal,t2)        //                  Laser Ball /10 Scoring Ted Estes
DRIVER(frpwr,l2)        //S6-497:   02/80 W Firepower (L-2)
DRIVER(frpwr,l6)        //          02/80 W Firepower (L-6)
DRIVER(frpwr,t6)        //                  Firepower (L-6) /10 Scoring Ted Estes
DRIVER(frpwr,a6)        //          10/05   Firepower (Sys.6/6-Digit Custom Rev. 31)
DRIVER(frpwr,b6)        //          12/03   Firepower (Sys.6 7-digit conversion)
DRIVER(frpwr,c6)        //          10/05   Firepower (Sys.6/7-Digit Custom Rev. 31)
DRIVER(frpwr,d6)        //                  Firepower (Sys.6/6-digit /10 Scoring Rev. 31)
DRIVER(omni,l1)         //          04/80 W Omni (Shuffle)
DRIVER(blkou,l1)        //S6-495:   06/80 W Blackout
DRIVER(blkou,f1)        //          06/80 W Blackout, French speech
DRIVER(blkou,t1)        //                  Blackout /10 Scoring Ted Estes
DRIVER(scrpn,l1)        //S6-494:   07/80 W Scorpion
DRIVER(scrpn,t1)        //                  Scorpion /10 Scoring Ted Estes
DRIVER(algar,l1)        //S6-499:   09/80 W Algar
DRIVER(alpok,l2)        //S6-501:   10/80 W Alien Poker L-2
DRIVER(alpok,l6)        //          10/80 W Alien Poker L-6
DRIVER(alpok,f6)        //          10/80 W Alien Poker (L-6, French Speech)
DRIVER(alpok,b6)        //          11/06   Alien Poker Multiball mod
//System 7
DRIVER(frpwr,a7)        //S7-497:   10/05   Firepower (Sys.7/6-digit Custom Rev. 31)
DRIVER(frpwr,b7)        //          12/03   Firepower (Sys.7 7-digit conversion)
DRIVER(frpwr,c7)        //          11/06   Firepower (Sys.7/7-digit Custom Rev. 38)
DRIVER(frpwr,d7)        //          10/05   Firepower (Sys.7/7-digit Custom Rev. 31)
DRIVER(frpwr,e7)        //          10/05   Firepower (Sys.7/6-digit /10 Scoring Rev. 31)
DRIVER(bk,l3)           //S7-500:   11/80 W Black Knight (L-3)
DRIVER(bk,l4)           //          11/80 W Black Knight (L-4)
DRIVER(bk,f4)           //          11/80 W Black Knight (L-4 French Speech)
DRIVER(jngld,l1)        //S7-503:   02/81 W Jungle Lord (L-1)
DRIVER(jngld,l2)        //          02/81 W Jungle Lord (L-2)
DRIVER(jngld,nt)        //          09/13 W Jungle Lord New Tricks
DRIVER(pharo,l2)        //S7-504:   05/81 W Pharaoh (L-2)
DRIVER(pharo,l2b)       //          05/19 W Pharaoh (L-2 'Tomb' Sample Sound Fix MOD)
                        //S7-506:   06/81 W Black Knight Limited Edition
DRIVER(solar,l2)        //S7-507:   07/81 W Solar Fire
DRIVER(barra,l1)        //S7-510:   09/81 W Barracora
DRIVER(hypbl,l2)        //S7-509:   12/81 W HyperBall (L-2)
DRIVER(hypbl,l3)        //          12/81 W HyperBall (L-3)
DRIVER(hypbl,l4)        //          12/81 W HyperBall (L-4)
DRIVER(hypbl,l5)        //          04/98   HyperBall (Jess Askey bootleg w/ high score save)
DRIVER(hypbl,l6)        //          05/06   HyperBall (Jess Askey bootleg w/ high score save, fixed)
DRIVER(csmic,l1)        //S7-502:   06/82 W Cosmic Gunfight
DRIVER(thund,p1)        //S7-508:   06/82 W Thunderball (P-1)
DRIVER(thund,p2)        //          08/82 W Thunderball (P-2)
DRIVER(thund,p3)        //          09/82 W Thunderball (P-3)
DRIVER(vrkon,l1)        //S7-512:   09/82 W Varkon
DRIVER(wrlok,l3)        //S7-516:   10/82 W Warlok
DRIVER(dfndr,l4)        //S7-517:   12/82 W Defender
DRIVER(ratrc,l1)        //S7-5??:   01/83 W Rat Race (never produced)
DRIVER(tmfnt,l5)        //S7-515:   03/83 W Time Fantasy
DRIVER(jst,l1)          //S7-519:   04/83 W Joust (L-1)
DRIVER(jst,l2)          //          04/83 W Joust (L-2)
DRIVER(fpwr2,l2)        //S7-521:   08/83 W Firepower II
DRIVER(lsrcu,l2)        //S7-520:   02/84 W Laser Cue (L-2)
DRIVER(lsrcu,l3)        //          06/16 W Laser Cue (Timmo bootleg, fixes neverending bell)
DRIVER(strlt,l1)        //S7-530:   06/84 W Star Light (Came out after System 9 produced)
//System 8
DRIVER(bstrk,l1)        //S8-???:   ??/83 W Big Strike (Bowler)
DRIVER(tstrk,l1)        //S8-???:   ??/83 W Triple Strike (Bowler)
DRIVER(pfevr,p3)        //S8-526:   05/84 W Pennant Fever (pitch & bat)
DRIVER(pfevr,l2)        //          05/84 W Pennant Fever (pitch & bat)
DRIVER(scrzy,l1)        //S8-543:   ??/84 W Still Crazy
//System 9
                        //S?-538:   10/84 W Gridiron
DRIVER(sshtl,l3)        //S9-535:   12/84 W Space Shuttle (L-3)
DRIVER(sshtl,l7)        //S9-535:   12/84 W Space Shuttle (L-7)
DRIVER(szone,l2)        //S9-916:   ??/84 W Strike Zone (L-2) (Shuffle)
DRIVER(szone,l5)        //          ??/84 W Strike Zone (L-5) (Shuffle)
DRIVER(sorcr,l1)        //S9-532:   03/85 W Sorcerer (L-1)
DRIVER(sorcr,l2)        //          03/85 W Sorcerer (L-2)
DRIVER(comet,l4)        //S9-540:   06/85 W Comet (L-4)
DRIVER(comet,l5)        //          06/85 W Comet (L-5)
//System 11
DRIVER(alcat,l7)        //S11-918:  ??/85 W Alley Cats (Shuffle)
DRIVER(hs,l3)           //S11-541:  01/86 W High Speed (L-3)
DRIVER(hs,l4)           //          01/86 W High Speed (L-4)
DRIVER(hs,l4c)          //          05/18 W High Speed (L-4C Competition MOD (f43c))
DRIVER(grand,l3)        //S11-523:  04/86 W Grand Lizard (L-3)
DRIVER(grand,l4)        //          04/86 W Grand Lizard (L-4)
DRIVER(rdkng,l1)        //S11-542:  07/86 W Road Kings (L-1)
DRIVER(rdkng,l2)        //          07/86 W Road Kings (L-2)
DRIVER(rdkng,l3)        //          07/86 W Road Kings (L-3)
DRIVER(rdkng,l4)        //          07/86 W Road Kings (L-4)
                        //S11-546:  10/86 W Strike Force
DRIVER(pb,p4)           //S11-549:  10/86 W Pin-Bot (P-4)
DRIVER(pb,l1)           //          10/86 W Pin-Bot (L-1)
DRIVER(pb,l2)           //          10/86 W Pin-Bot (L-2)
DRIVER(pb,l3)           //          10/86 W Pin-Bot (L-3)
DRIVER(pb,l5)           //          10/86 W Pin-Bot (L-5)
DRIVER(pb,l5h)          //          08/12   Pin-Bot (L-5, Freeplay / solar value mod)
DRIVER(tts,l1)          //S11-919:  ??/86 W Tic-Tac-Strike (L-1, Shuffle)
DRIVER(tts,l2)          //          ??/86 W Tic-Tac-Strike (L-2, Shuffle)
DRIVER(milln,l3)        //S11-555:  01/87 W Millionaire
DRIVER(f14,p3)          //S11-554:  03/87 W F-14 Tomcat (P-3)
DRIVER(f14,p4)          //          03/87 W F-14 Tomcat (P-4)
DRIVER(f14,p5)          //          03/87 W F-14 Tomcat (P-5)
DRIVER(f14,l1)          //          05/87 W F-14 Tomcat (L-1)
DRIVER(fire,l2)         //S11-556:  08/87 W Fire! (L-2)
DRIVER(fire,l3)         //S11-556:  08/87 W Fire! (L-3)
DRIVER(bguns,p1)        //S11-557:  10/87 W Big Guns (P-1)
DRIVER(bguns,la)        //          10/87 W Big Guns (L-A)
DRIVER(bguns,lac)       //          07/19 W Big Guns (L-AC Competition MOD) // patch 7472
DRIVER(bguns,l7)        //          10/87 W Big Guns (L-7)
DRIVER(bguns,l8)        //          10/87 W Big Guns (L-8)
DRIVER(spstn,l5)        //S11-552:  12/87 W Space Station
DRIVER(gmine,l2)        //S11-920:  ??/87 W Gold Mine (Shuffle)
DRIVER(tdawg,l1)        //S11-921:  ??/87 W Top Dawg (Shuffle)
DRIVER(shfin,l1)        //S11-922:  ??/87 W Shuffle Inn (Shuffle)
DRIVER(cycln,l1)        //S11-564:  02/88 W Cyclone (L-1)
DRIVER(cycln,l4)        //S11-564:  02/88 W Cyclone (L-4)
DRIVER(cycln,l5)        //          02/88 W Cyclone (L-5)
DRIVER(bnzai,pa)        //S11-566:  05/88 W Banzai Run (P-A)
DRIVER(bnzai,l1)        //          05/88 W Banzai Run (L-1)
DRIVER(bnzai,l3)        //          05/88 W Banzai Run (L-3)
DRIVER(bnzai,g3)        //          05/88 W Banzai Run (G-3 German)
DRIVER(bnzai,t3)        //          10/11 W Banzai Run (L-3 Target sound fix)
DRIVER(swrds,l1)        //S11-559:  06/88 W Swords of Fury (L-1)
DRIVER(swrds,l2)        //S11-559:  06/88 W Swords of Fury (L-2)
DRIVER(taxi,p5)         //S11-553:  08/88 W Taxi (P-5)
DRIVER(taxi,lu1)        //S11-553:  08/88 W Taxi (Marilyn LU-1 Europe)
DRIVER(taxi,lg1)        //S11-553:  08/88 W Taxi (Marilyn LG-1 German)
DRIVER(taxi,l3)         //          08/88 W Taxi (Marilyn)
DRIVER(taxi,l4)         //          08/88 W Taxi (Lola)
DRIVER(taxi,l5cm)       //          04/16 W Taxi (Marilyn L-5C Competition MOD)
DRIVER(taxi,l5c)        //          04/16 W Taxi (Lola L-5C Competition MOD)
DRIVER(jokrz,l3)        //S11-567:  12/88 W Jokerz! (L-3)
DRIVER(jokrz,g4)        //          12/88 W Jokerz! (G-4 German)
DRIVER(jokrz,l6)        //          12/88 W Jokerz! (L-6)
DRIVER(esha,pa1)        //S11-568:  02/89 W Earthshaker PA-1 Prototype
DRIVER(esha,pa4)        //          02/89 W Earthshaker PA-4 Prototype
DRIVER(esha,la1)        //          02/89 W Earthshaker LA-1
DRIVER(esha,lg1)        //          02/89 W Earthshaker LG-1 (German)
DRIVER(esha,lg2)        //          02/89 W Earthshaker LG-2 (German)
DRIVER(esha,la3)        //          02/89 W Earthshaker LA-3
DRIVER(esha,l4c)        //          06/16 W Earthshaker LA-4C Competition MOD
DRIVER(esha,ma3)        //                  Earthshaker LA-3 (Metallica)
DRIVER(esha,pr4)        //          02/89 W Earthshaker Family version
DRIVER(bk2k,pu1)        //S11-563:  04/89 W Black Knight 2000 (PU-1 Prototype Europe)
DRIVER(bk2k,pf1)        //          04/89 W Black Knight 2000 (PF-1 Prototype French)
DRIVER(bk2k,lg1)        //          04/89 W Black Knight 2000 (LG-1 German)
DRIVER(bk2k,lg3)        //          04/89 W Black Knight 2000 (LG-3 German)
DRIVER(bk2k,l4)         //          04/89 W Black Knight 2000 (L-4)
DRIVER(bk2k,la2)        //          04/89 W Black Knight 2000 (LA-2)
DRIVER(bk2k,pa7)        //          04/89 W Black Knight 2000 (PA-7 Prototype)
DRIVER(bk2k,pa5)        //          04/89 W Black Knight 2000 (PA-5 Prototype)
                        //S11:      05/89 W Pool
//First Game produced entirely by Williams after Merger to use Bally Name
DRIVER(tsptr,l3)        //S11-2630: 07/89 B Transporter the Rescue
DRIVER(polic,l2)        //S11-573:  08/89 W Police Force (LA-2)
DRIVER(polic,l3)        //          09/89 W Police Force (LA-3)
DRIVER(polic,l4)        //          10/89 W Police Force (LA-4)
DRIVER(polic,g4)        //          10/89 W Police Force (LG-4 German)
DRIVER(eatpm,p7)        //S11-782:  09/89 B Elvira and the Party Monsters (PA-7 Prototype)
DRIVER(eatpm,l1)        //          09/89 B Elvira and the Party Monsters (LA-1)
DRIVER(eatpm,f1)        //          09/89 B Elvira and the Party Monsters (LF-1 French)
DRIVER(eatpm,l2)        //          10/89 B Elvira and the Party Monsters (LA-2)
DRIVER(eatpm,l4)        //          10/89 B Elvira and the Party Monsters (LA-4)
DRIVER(eatpm,4u)        //          10/89 B Elvira and the Party Monsters (LU-4 Europe)
DRIVER(eatpm,4g)        //          10/89 B Elvira and the Party Monsters (LG-4 German)
DRIVER(bcats,l2)        //S11-575:  11/89 W Bad Cats (LA-2)
DRIVER(bcats,l5)        //          11/89 W Bad Cats (L-5)
DRIVER(rvrbt,l3)        //S11-1966: 11/89 W Riverboat Gambler (L-3)
DRIVER(rvrbt,p7)        //S11-1966: 11/89 W Riverboat Gambler (PA-7 Prototype)
DRIVER(mousn,l1)        //S11-1635: 11/89 B Mousin' Around! (LA-1)
DRIVER(mousn,lu)        //          11/89 B Mousin' Around! (LU-1 Europe)
DRIVER(mousn,l4)        //          03/90 B Mousin' Around! (LA-4)
DRIVER(mousn,l4c)       //          01/19 B Mousin' Around! (LA-4C Competition MOD)
                        //S11-???:  ??/90 B Mazatron
                        //S11-???:  ??/90 B Player's Choice
                        //S11-???:  ??/90 B Ghost Gallery
DRIVER(whirl,l2)        //S11-574:  01/90 W Whirlwind (L-2)
DRIVER(whirl,l3)        //          01/90 W Whirlwind (L-3)
DRIVER(whirl,g3)        //          01/90 W Whirlwind (LG-3 German)
DRIVER(gs,la3)          //S11-985:  02/90 B Game Show (LA-3)
DRIVER(gs,lu3)          //          ??/90 B Game Show (LU-3 Europe)
DRIVER(gs,lu4)          //          02/90 B Game Show (LU-4 Europe)
DRIVER(gs,lg6)          //          ??/90 B Game Show (LG-6 German)
DRIVER(rollr,ex)        //S11-576:  01/90 W Rollergames (EXPERIMENTAL)
DRIVER(rollr,e1)        //          01/90 W Rollergames (PU-1 Prototype Europe)
DRIVER(rollr,p2)        //          01/90 W Rollergames (PA-2, PA-1 Sound)
DRIVER(rollr,l2)        //          04/90 W Rollergames (L-2)
DRIVER(rollr,l2c)       //          04/19 W Rollergames (L-2C Competition MOD)
DRIVER(rollr,l3)        //          05/90 W Rollergames (LU-3 Europe)
DRIVER(rollr,g3)        //          05/90 W Rollergames (LG-3 German)
DRIVER(rollr,f2)        //          ??/90 W Rollergames (LF-2 French)
DRIVER(rollr,f3)        //          ??/90 W Rollergames (LF-3 French)
DRIVER(rollr,d2)        //          ??/?? W Rollergames (AD-2 Prototype)
DRIVER(pool,p7)         //S11-1848: 09/89?B Pool Sharks (PA-7 Prototype)
DRIVER(pool,le2)        //          03/90?B Pool Sharks (LE-2 Europe)
DRIVER(pool,l5)         //          05/90 B Pool Sharks (LA-5)
DRIVER(pool,l6)         //          05/90 B Pool Sharks (LA-6)
DRIVER(pool,l7)         //          01/91 B Pool Sharks (LA-7)
DRIVER(pool,l7c)        //          10/19 B Pool Sharks (LA-7C Competition MOD)
DRIVER(diner,p0)        //S11-571:  11/89 W Diner (PA-0 Prototype)
DRIVER(diner,l1)        //          06/90 W Diner (LU-1 Europe)
DRIVER(diner,l2)        //          06/90 W Diner (LU-2 Europe)
DRIVER(diner,f2)        //          06/90 W Diner (LF-2 French)
DRIVER(diner,l3)        //          06/90 W Diner (LA-3)
DRIVER(diner,l4)        //          09/90 W Diner (LA-4)
DRIVER(radcl,p3)        //S11-1904: 06/90 B Radical! (P-3)
DRIVER(radcl,l1)        //          06/90 B Radical! (L-1)
DRIVER(radcl,g1)        //          06/90 B Radical! (LG-1)
DRIVER(radcl,l1c)       //          11/18 B Radical! (L-1C Competition MOD)
DRIVER(strax,p7)        //S11-???:  09/90 W Star Trax (Domestic Prototype)
DRIVER(dd,p6)           //S11-2016: 08/90 B Dr. Dude (PA-6 Prototype)
DRIVER(dd,lu1)          //          08/90 B Dr. Dude (LU-1 Europe)
DRIVER(dd,l2)           //          09/90 B Dr. Dude (LA-2)
DRIVER(dd,l3c)          //          04/16 B Dr. Dude (LA-3C Competition MOD)
DRIVER(bbnny,l2)        //S11-209:  12/90 B Bugs Bunny's Birthday Ball (L-2)
DRIVER(bbnny,lu)        //          12/90 B Bugs Bunny's Birthday Ball (LU-2 Europe)
//WPC
DRIVER(dd,p06)          //WPC-2016: 08/90 B Dr. Dude (P-6 WPC)
DRIVER(dd,p7)           //          09/90 B Dr. Dude (P-7 WPC)
                        //WPC-503:  11/90 W Funhouse (L-1)
DRIVER(fh,l2)           //          12/90 W Funhouse (L-2)
DRIVER(fh,l3)           //          12/90 W Funhouse (L-3)
DRIVER(fh,d3)           //                  Funhouse (D-3) LED Ghost Fix
DRIVER(fh,l4)           //          01/91 W Funhouse (L-4)
DRIVER(fh,d4)           //                  Funhouse (D-4) LED Ghost Fix
DRIVER(fh,l5)           //          02/91 W Funhouse (L-5)
DRIVER(fh,d5)           //                  Funhouse (D-5) LED Ghost Fix
DRIVER(fh,l9)           //          12/92 W Funhouse (L-9)
DRIVER(fh,d9)           //                  Funhouse (D-9) LED Ghost Fix
DRIVER(fh,l9b)          //                  Funhouse bootleg with correct German translation
DRIVER(fh,d9b)          //                  Funhouse bootleg with correct German translation LED Ghost Fix
DRIVER(fh,905h)         //          04/96 W Funhouse (9.05H)
DRIVER(fh,906h)         //                  Funhouse (9.06H Coin Play)
DRIVER(fh,f91)          //          ??/??   Funhouse (FreeWPC 0.91)
DRIVER(bop,l2)          //WPC-502:  04/91 W Machine: Bride of Pinbot, The (L-2)
DRIVER(bop,d2)          //                  Machine: Bride of Pinbot, The (D-2) LED Ghost Fix
DRIVER(bop,l3)          //          04/91 W Machine: Bride of Pinbot, The (L-3)
DRIVER(bop,d3)          //                  Machine: Bride of Pinbot, The (D-3) LED Ghost Fix
DRIVER(bop,l4)          //          04/91 W Machine: Bride of Pinbot, The (L-4)
DRIVER(bop,d4)          //                  Machine: Bride of Pinbot, The (D-4) LED Ghost Fix
DRIVER(bop,l5)          //          05/91 W Machine: Bride of Pinbot, The (L-5)
DRIVER(bop,d5)          //                  Machine: Bride of Pinbot, The (D-5) LED Ghost Fix
DRIVER(bop,l6)          //          05/91 W Machine: Bride of Pinbot, The (L-6)
DRIVER(bop,d6)          //                  Machine: Bride of Pinbot, The (D-6) LED Ghost Fix
DRIVER(bop,l7)          //          12/92 W Machine: Bride of Pinbot, The (L-7) introduced billionaire crash multiplayer bug
DRIVER(bop,d7)          //                  Machine: Bride of Pinbot, The (D-7) LED Ghost Fix
DRIVER(bop,l8)          //                  Machine: Bride of Pinbot, The (L-8) fixes billionaire crash multiplayer bug
DRIVER(bop,d8)          //                  Machine: Bride of Pinbot, The (D-8) LED Ghost Fix
DRIVER(hd,l1)           //WPC-201:  02/91 B Harley Davidson (L-1)
DRIVER(hd,d1)           //                  Harley Davidson (D-1) LED Ghost Fix
DRIVER(hd,l2)           //          02/91 B Harley Davidson (L-2)
DRIVER(hd,d2)           //                  Harley Davidson (D-2) LED Ghost Fix
DRIVER(hd,l3)           //          04/91 B Harley Davidson (L-3)
DRIVER(hd,d3)           //                  Harley Davidson (D-3) LED Ghost Fix
DRIVER(sf,l1)           //WPC-601:  03/91 W SlugFest (L-1)
DRIVER(sf,d1)           //                  SlugFest (D-1) LED Ghost Fix
DRIVER(hurr,l2)         //WPC-512:  08/91 W Hurricane (L-2)
DRIVER(hurr,d2)         //                  Hurricane (D-2) LED Ghost Fix
DRIVER(t2,l2)           //WPC-513:  10/91 W Terminator 2: Judgement Day (L-2)
DRIVER(t2,d2)           //                  Terminator 2: Judgement Day (D-2) LED Ghost Fix
DRIVER(t2,l3)           //          10/91 W Terminator 2: Judgement Day (L-3)
DRIVER(t2,d3)           //                  Terminator 2: Judgement Day (D-3) LED Ghost Fix
DRIVER(t2,l4)           //          10/91 W Terminator 2: Judgement Day (L-4)
DRIVER(t2,d4)           //                  Terminator 2: Judgement Day (D-4) LED Ghost Fix
DRIVER(t2,l6)           //          10/91 W Terminator 2: Judgement Day (L-6)
DRIVER(t2,d6)           //                  Terminator 2: Judgement Day (D-6) LED Ghost Fix
DRIVER(t2,l8)           //          12/91 W Terminator 2: Judgement Day (L-8)
DRIVER(t2,d8)           //                  Terminator 2: Judgement Day (D-8) LED Ghost Fix
DRIVER(t2,l81)          //                  Terminator 2: Judgement Day (L-81) Attract Sound Fix
DRIVER(t2,l82)          //                  Terminator 2: Judgement Day (L-82) Hacked attract routines
DRIVER(t2,p2f)          //          12/91 W Terminator 2: Judgement Day (Profanity Speech version)
DRIVER(t2,p2g)          //                  Terminator 2: Judgement Day (Profanity Speech version) LED Ghost Fix
DRIVER(t2,f19)          //          ??/??   Terminator 2: Judgement Day (FreeWPC 0.19)
DRIVER(t2,f20)          //          ??/??   Terminator 2: Judgement Day (FreeWPC 0.20)
DRIVER(t2,f32)          //          ??/??   Terminator 2: Judgement Day (FreeWPC 0.32)
DRIVER(gi,l3)           //WPC-203:  07/91 B Gilligan's Island (L-3)
DRIVER(gi,d3)           //                  Gilligan's Island (D-3) LED Ghost Fix
DRIVER(gi,l4)           //          07/91 B Gilligan's Island (L-4)
DRIVER(gi,d4)           //                  Gilligan's Island (D-4) LED Ghost Fix
DRIVER(gi,l6)           //          08/91 B Gilligan's Island (L-6)
DRIVER(gi,d6)           //                  Gilligan's Island (D-6) LED Ghost Fix
DRIVER(gi,l8)           //          ??/9? B Gilligan's Island (L-8)
DRIVER(gi,l9)           //          12/92 B Gilligan's Island (L-9) // Revision history says 11/94 ??
DRIVER(gi,d9)           //                  Gilligan's Island (D-9) LED Ghost Fix
DRIVER(pz,l1)           //WPC-204:  10/91 B Party Zone, The (L-1)
DRIVER(pz,d1)           //                  Party Zone, The (D-1 LED Ghost Fix)
DRIVER(pz,l2)           //          10/91 B Party Zone, The (L-2)
DRIVER(pz,d2)           //                  Party Zone, The (D-2 LED Ghost Fix)
DRIVER(pz,l3)           //          10/91 B Party Zone, The (L-3)
DRIVER(pz,d3)           //                  Party Zone, The (D-3 LED Ghost Fix)
DRIVER(pz,f4)           //          10/91 B Party Zone, The (F-4 Fliptronic)
DRIVER(pz,f5)           //                  Party Zone, The (F-5 LED Ghost Fix)
DRIVER(pz,f4pfx)        //          12/18 B Party Zone, The (F-4 Pinball FX)
DRIVER(strik,l4)        //WPC-102:  05/92 W Strike Master (L-4)
DRIVER(strik,d4)        //                  Strike Master (D-4) LED Ghost Fix
DRIVER(taf,p2)          //WPC-217:  01/92 B Addams Family, The (P-2)
DRIVER(taf,p3)          //                  Addams Family, The (P-3) LED Ghost Fix
DRIVER(taf,l1)          //          01/92 B Addams Family, The (L-1)
DRIVER(taf,d1)          //                  Addams Family, The (D-1) LED Ghost Fix
DRIVER(taf,l2)          //          03/92 B Addams Family, The (L-2)
DRIVER(taf,d2)          //                  Addams Family, The (D-2) LED Ghost Fix
DRIVER(taf,l3)          //          05/92 B Addams Family, The (L-3)
DRIVER(taf,d3)          //                  Addams Family, The (D-3) LED Ghost Fix
DRIVER(taf,l4)          //          05/92 B Addams Family, The (L-4)
DRIVER(taf,d4)          //                  Addams Family, The (D-4) LED Ghost Fix
DRIVER(taf,l5)          //          12/92 B Addams Family, The (L-5)
DRIVER(taf,d5)          //                  Addams Family, The (D-5) LED Ghost Fix
DRIVER(taf,l5c)         //          02/20 B Addams Family, The (L-5C Competition MOD)
DRIVER(taf,l6)          //          03/93 B Addams Family, The (L-6)
DRIVER(taf,d6)          //                  Addams Family, The (D-6) LED Ghost Fix
DRIVER(taf,l7)          //          10/92 B Addams Family, The (L-7) (Prototype L-5)
DRIVER(taf,d7)          //                  Addams Family, The (D-7) (Prototype L-5) LED Ghost Fix
DRIVER(taf,h4)          //          05/94 B Addams Family, The (H-4)
DRIVER(taf,i4)          //                  Addams Family, The (I-4) LED Ghost Fix
DRIVER(gw,pb)           //WPC-504:  03/92 W Getaway: High Speed II, The (P-B)
DRIVER(gw,pc)           //          03/92 W Getaway: High Speed II, The (P-C)
DRIVER(gw,pd)           //                  Getaway: High Speed II, The (P-D) LED Ghost Fix
DRIVER(gw,p7)           //          03/92 W Getaway: High Speed II, The (P-7)
DRIVER(gw,p8)           //                  Getaway: High Speed II, The (P-8) LED Ghost Fix
DRIVER(gw,l1)           //          04/92 W Getaway: High Speed II, The (L-1)
DRIVER(gw,d1)           //                  Getaway: High Speed II, The (D-1) LED Ghost Fix
DRIVER(gw,l2)           //          06/92 W Getaway: High Speed II, The (L-2)
DRIVER(gw,d2)           //                  Getaway: High Speed II, The (D-2) LED Ghost Fix
DRIVER(gw,l3)           //          06/92 W Getaway: High Speed II, The (L-3)
DRIVER(gw,d3)           //                  Getaway: High Speed II, The (D-3) LED Ghost Fix
DRIVER(gw,l5)           //          12/92 W Getaway: High Speed II, The (L-5)
DRIVER(gw,d5)           //                  Getaway: High Speed II, The (D-5) LED Ghost Fix
DRIVER(gw,l5c)          //          08/17   Getaway: High Speed II, The (L-5C Competition MOD)
DRIVER(br,p17)          //WPC-213:  05/92 B Black Rose (P-17)
DRIVER(br,p18)          //                  Black Rose (P-18) LED Ghost Fix
DRIVER(br,l1)           //          08/92 B Black Rose (L-1)
DRIVER(br,d1)           //                  Black Rose (D-1) LED Ghost Fix
DRIVER(br,l3)           //          01/93 B Black Rose (L-3)
DRIVER(br,d3)           //                  Black Rose (D-3) LED Ghost Fix
DRIVER(br,l4)           //          11/93 B Black Rose (L-4)
DRIVER(br,d4)           //                  Black Rose (D-4) LED Ghost Fix
DRIVER(ft,p2)           //WPC-505:  06/92 W Fish Tales (P-2)
DRIVER(ft,p4)           //          07/92 W Fish Tales (P-4)
DRIVER(ft,p5)           //                  Fish Tales (P-5) LED Ghost Fix
DRIVER(ft,l3)           //          09/92 W Fish Tales (L-3)
DRIVER(ft,l4)           //          09/92 W Fish Tales (L-4)
DRIVER(ft,l5)           //          12/92 W Fish Tales (L-5)
DRIVER(ft,d5)           //                  Fish Tales (D-5) LED Ghost Fix
DRIVER(ft,l5p)          //                  Fish Tales (L-5) Text size patch bugfix
DRIVER(ft,d6)           //          08/15   Fish Tales (D-6) Text size patch bugfix LED Ghost Fix
DRIVER(dw,p5)           //WPC-206:  10/92 B Doctor Who (P-5)
DRIVER(dw,p6)           //                  Doctor Who (P-6) LED Ghost Fix
DRIVER(dw,l1)           //          10/92 B Doctor Who (L-1)
DRIVER(dw,d1)           //                  Doctor Who (D-1) LED Ghost Fix
DRIVER(dw,l2)           //          11/92 B Doctor Who (L-2)
DRIVER(dw,d2)           //                  Doctor Who (D-2) LED Ghost Fix
DRIVER(cftbl,p3)        //WPC-218:  12/92 B Creature from the Black Lagoon (P-3) - game itself says 1991!
DRIVER(cftbl,l2)        //          01/93 B Creature from the Black Lagoon (L-2)
DRIVER(cftbl,d2)        //          01/93 B Creature from the Black Lagoon (D-2) LED Ghost Fix
DRIVER(cftbl,l3)        //          01/93 B Creature from the Black Lagoon (L-3)
DRIVER(cftbl,d3)        //                  Creature from the Black Lagoon (D-3) LED Ghost Fix
DRIVER(cftbl,l4)        //          02/93 B Creature from the Black Lagoon (L-4)
DRIVER(cftbl,d4)        //                  Creature from the Black Lagoon (D-4) LED Ghost Fix
//DRIVER(cftbl,l5c)     //          04/16 B Creature from the Black Lagoon (L-5C) //outdated patch
DRIVER(cftbl,l4c)       //          02/20 B Creature from the Black Lagoon (L-4C) //patch eccc
DRIVER(hshot,p8)        //WPC-617:  11/92 M Hot Shot Basketball (P-8)
DRIVER(hshot,p9)        //                  Hot Shot Basketball (P-9) LED Ghost Fix
DRIVER(ww,p1)           //WPC-518:  11/92 W White Water (P-8, P-1 sound)
DRIVER(ww,p2)           //                  White Water (P-9, P-1 sound) LED Ghost Fix
DRIVER(ww,p8)           //          11/92 W White Water (P-8, P-2 sound)
DRIVER(ww,p9)           //                  White Water (P-9, P-2 sound) LED Ghost Fix
DRIVER(ww,l2)           //          12/92 W White Water (L-2)
DRIVER(ww,d2)           //                  White Water (D-2) LED Ghost Fix
DRIVER(ww,l3)           //          01/93 W White Water (L-3)
DRIVER(ww,d3)           //                  White Water (D-3) LED Ghost Fix
DRIVER(ww,l4)           //          02/93 W White Water (L-4)
DRIVER(ww,d4)           //                  White Water (D-4) LED Ghost Fix
DRIVER(ww,l5)           //          05/93 W White Water (L-5)
DRIVER(ww,d5)           //                  White Water (D-5) LED Ghost Fix
DRIVER(ww,lh5)          //          10/00 W White Water (LH-5)
DRIVER(ww,lh6)          //          12/05 W White Water (LH-6)
DRIVER(ww,lh6c)         //                  White Water (LH-6 Coin Play)
DRIVER(ww,bfr01)        //          07/16 W White Water (FreeWPC/Bigfoot R0.1)
DRIVER(ww,bfr01b)       //          03/18 W White Water (FreeWPC/Bigfoot R0.1b)
DRIVER(ww,bfr01c)       //          04/18 W White Water (FreeWPC/Bigfoot R0.1c)
DRIVER(ww,bfr01d)       //          05/18 W White Water (FreeWPC/Bigfoot R0.1d)
DRIVER(drac,p11)        //WPC-501:  02/93 W Bram Stoker's Dracula (P-11)
DRIVER(drac,p12)        //                  Bram Stoker's Dracula (P-12) LED Ghost Fix
DRIVER(drac,l1)         //          02/93 W Bram Stoker's Dracula (L-1)
DRIVER(drac,d1)         //                  Bram Stoker's Dracula (D-1) LED Ghost Fix
DRIVER(drac,l2c)        //          04/16 W Bram Stoker's Dracula (L-2C)
DRIVER(tz,pa1)          //WPC-520:  03/93 B Twilight Zone (PA-1 Prototype)
DRIVER(tz,pa2)          //                  Twilight Zone (PA-2) LED Ghost Fix
DRIVER(tz,p3)           //          04/93 B Twilight Zone (P-3)
DRIVER(tz,p3d)          //                  Twilight Zone (P-3) LED Ghost Fix
DRIVER(tz,p4)           //          04/93 B Twilight Zone (P-4)
DRIVER(tz,p5)           //                  Twilight Zone (P-5) LED Ghost Fix
DRIVER(tz,l1)           //          04/93 B Twilight Zone (L-1)
DRIVER(tz,d1)           //                  Twilight Zone (D-1) LED Ghost Fix
DRIVER(tz,l2)           //          05/93 B Twilight Zone (L-2)
DRIVER(tz,d2)           //                  Twilight Zone (D-2) LED Ghost Fix
DRIVER(tz,l3)           //          05/93 B Twilight Zone (L-3)
DRIVER(tz,d3)           //                  Twilight Zone (D-3) LED Ghost Fix
DRIVER(tz,ifpa)         //          05/93 B Twilight Zone (IFPA)
DRIVER(tz,ifpa2)        //                  Twilight Zone (IFPA) LED Ghost Fix
DRIVER(tz,l4)           //          06/93 B Twilight Zone (L-4)
DRIVER(tz,d4)           //                  Twilight Zone (D-4) LED Ghost Fix
DRIVER(tz,l5)           //          08/93 B Twilight Zone (L-5)
DRIVER(tz,h7)           //          10/94 B Twilight Zone (H-7)
DRIVER(tz,i7)           //                  Twilight Zone (I-7) LED Ghost Fix
DRIVER(tz,h8)           //          11/94 B Twilight Zone (H-8)
DRIVER(tz,i8)           //                  Twilight Zone (I-8) LED Ghost Fix
DRIVER(tz,la9)          //          01/95 B Twilight Zone (LA-9, PAPA Tournament Version 9.0)
DRIVER(tz,92)           //          01/95 B Twilight Zone (9.2)
DRIVER(tz,93)           //                  Twilight Zone (9.3) LED Ghost Fix
DRIVER(tz,94h)          //          10/98 B Twilight Zone (9.4H - Home version)
DRIVER(tz,94ch)         //                  Twilight Zone (9.4CH - Home Coin version)
DRIVER(tz,f10)          //          11/06   Twilight Zone (FreeWPC 0.10)
DRIVER(tz,f19)          //          ??/??   Twilight Zone (FreeWPC 0.19)
DRIVER(tz,f50)          //          ??/??   Twilight Zone (FreeWPC 0.50)
DRIVER(tz,f86)          //          ??/??   Twilight Zone (FreeWPC 0.86)
DRIVER(tz,f97)          //          05/10   Twilight Zone (FreeWPC 0.97)
DRIVER(tz,f100)         //          08/11   Twilight Zone (FreeWPC 1.00)
DRIVER(ij,p2)           //WPC-517:  07/93 W Indiana Jones: The Pinball Adventure (P-2)
DRIVER(ij,l3)           //WPC-517:  07/93 W Indiana Jones: The Pinball Adventure (L-3)
DRIVER(ij,d3)           //                  Indiana Jones: The Pinball Adventure (D-3) LED Ghost Fix
DRIVER(ij,l4)           //          08/93 W Indiana Jones: The Pinball Adventure (L-4)
DRIVER(ij,d4)           //                  Indiana Jones: The Pinball Adventure (D-4) LED Ghost Fix
DRIVER(ij,l5)           //          09/93 W Indiana Jones: The Pinball Adventure (L-5)
DRIVER(ij,d5)           //                  Indiana Jones: The Pinball Adventure (D-5) LED Ghost Fix
DRIVER(ij,l6)           //          10/93 W Indiana Jones: The Pinball Adventure (L-6)
DRIVER(ij,d6)           //                  Indiana Jones: The Pinball Adventure (D-6) LED Ghost Fix
DRIVER(ij,l7)           //          11/93 W Indiana Jones: The Pinball Adventure (L-7)
DRIVER(ij,d7)           //                  Indiana Jones: The Pinball Adventure (D-7) LED Ghost Fix
DRIVER(ij,lg7)          //          11/93 W Indiana Jones: The Pinball Adventure (LG-7 German)
DRIVER(ij,dg7)          //                  Indiana Jones: The Pinball Adventure (DG-7) LED Ghost Fix
DRIVER(ij,h1)           //                  Indiana Jones: The Pinball Adventure (HK-1) Family no hate speech version
DRIVER(ij,i1)           //                  Indiana Jones: The Pinball Adventure (I-1) Family no hate LED Ghost Fix
DRIVER(jd,l1)           //WPC-220:  10/93 B Judge Dredd (L-1)
DRIVER(jd,d1)           //                  Judge Dredd (D-1) LED Ghost Fix
DRIVER(jd,l4)           //          10/93 B Judge Dredd (L-4)
DRIVER(jd,d4)           //                  Judge Dredd (D-4) LED Ghost Fix
DRIVER(jd,l5)           //          10/93 B Judge Dredd (L-5)
DRIVER(jd,d5)           //                  Judge Dredd (D-5) LED Ghost Fix
DRIVER(jd,l6)           //          10/93 B Judge Dredd (L-6)
DRIVER(jd,d6)           //                  Judge Dredd (D-6) LED Ghost Fix
DRIVER(jd,l7)           //          12/93 B Judge Dredd (L-7)
DRIVER(jd,d7)           //                  Judge Dredd (D-7) LED Ghost Fix
DRIVER(afv,l4)          //WPC-622:  ??/93 W Addams Family Values (L-4 Redemption)
DRIVER(afv,d4)          //                  Addams Family Values (D-4 Redemption) LED Ghost Fix
DRIVER(sttng,p4)        //WPC-523:  11/93 W Star Trek: The Next Generation (P-4)
DRIVER(sttng,p5)        //          11/93 W Star Trek: The Next Generation (P-5)
DRIVER(sttng,p6)        //                  Star Trek: The Next Generation (P-6) LED Ghost Fix
DRIVER(sttng,p8)        //          11/93 W Star Trek: The Next Generation (P-8)
DRIVER(sttng,l1)        //          11/93 W Star Trek: The Next Generation (LX-1)
DRIVER(sttng,d1)        //                  Star Trek: The Next Generation (DX-1) LED Ghost Fix
DRIVER(sttng,l2)        //          12/93 W Star Trek: The Next Generation (LX-2)
DRIVER(sttng,d2)        //                  Star Trek: The Next Generation (DX-2) LED Ghost Fix
DRIVER(sttng,l3)        //          12/93 W Star Trek: The Next Generation (LX-3)
DRIVER(sttng,l5)        //          12/93 W Star Trek: The Next Generation (LX-5)
DRIVER(sttng,l7)        //          02/94 W Star Trek: The Next Generation (LX-7)
DRIVER(sttng,d7)        //                  Star Trek: The Next Generation (DX-7) LED Ghost Fix
DRIVER(sttng,l7c)       //          08/17 W Star Trek: The Next Generation (LX-7C Competition MOD)
DRIVER(sttng,x7)        //          02/94 W Star Trek: The Next Generation (LX-7 Special)
DRIVER(sttng,dx)        //                  Star Trek: The Next Generation (DX-7 Special) LED Ghost Fix
DRIVER(sttng,s7)        //          02/94 W Star Trek: The Next Generation (LX-7 SP1)
DRIVER(sttng,ds)        //                  Star Trek: The Next Generation (DX-7 SP1) LED Ghost Fix
DRIVER(sttng,g7)        //          02/94 W Star Trek: The Next Generation (LG-7 German)
DRIVER(sttng,h7)        //                  Star Trek: The Next Generation (HG-7) LED Ghost Fix
DRIVER(pop,pa3)         //WPC-522:  12/93 B Popeye Saves the Earth (PA-3 Prototype)
DRIVER(pop,pa4)         //                  Popeye Saves the Earth (PA-4) LED Ghost Fix
DRIVER(pop,la4)         //          02/94 B Popeye Saves the Earth (LA-4)
DRIVER(pop,lx4)         //          02/94 B Popeye Saves the Earth (LX-4)
DRIVER(pop,lx5)         //          02/94 B Popeye Saves the Earth (LX-5)
DRIVER(pop,dx5)         //                  Popeye Saves the Earth (DX-5) LED Ghost Fix
DRIVER(dm,pa2)          //WPC-528:  03/94 W Demolition Man (PA-2 Prototype)
DRIVER(dm,pa3)          //                  Demolition Man (PA-3) LED Ghost Fix
DRIVER(dm,px5)          //          04/94 W Demolition Man (PX-5 Prototype)
DRIVER(dm,px6)          //                  Demolition Man (PX-6) LED Ghost Fix
DRIVER(dm,la1)          //          04/94 W Demolition Man (LA-1)
DRIVER(dm,da1)          //                  Demolition Man (DA-1) LED Ghost Fix
DRIVER(dm,lx3)          //          04/94 W Demolition Man (LX-3)
DRIVER(dm,dx3)          //                  Demolition Man (DX-3) LED Ghost Fix
DRIVER(dm,lx4)          //          05/94 W Demolition Man (LX-4)
DRIVER(dm,dx4)          //                  Demolition Man (DX-4) LED Ghost Fix
DRIVER(dm,lx4c)         //          02/20   Demolition Man (LX-4C Competition MOD)
DRIVER(dm,h5)           //          02/95 W Demolition Man (H-5) with rude speech
DRIVER(dm,h5b)          //                  Demolition Man (H-5) with rude speech (Coin Play)
//DRIVER(dm,h5c)          //          08/17   Demolition Man (H-5C Competition MOD) with rude speech
DRIVER(dm,dh5)          //                  Demolition Man (DH-5) with rude speech LED Ghost Fix
DRIVER(dm,dh5b)         //                  Demolition Man (DH-5) with rude speech LED Ghost Fix (Coin Play)
DRIVER(dm,h6)           //          08/95 W Demolition Man (H-6) with rude speech
DRIVER(dm,h6b)          //                  Demolition Man (H-6) with rude speech (Coin Play)
DRIVER(dm,h6c)          //          11/19   Demolition Man (H-6C Competition MOD) with rude speech
DRIVER(dm,dt099)        //          04/14   Demolition Man (FreeWPC/Demolition Time 0.99)
DRIVER(dm,dt101)        //          09/14   Demolition Man (FreeWPC/Demolition Time 1.01)
DRIVER(tafg,h3)         //WPC-538:  08/94 B Addams Family Special Collectors Edition, The (Home version)
DRIVER(tafg,i3)         //                  Addams Family Special Collectors Edition, The (Home version, LED Ghost Fix)
DRIVER(tafg,lx3)        //          10/94 B Addams Family Special Collectors Edition, The (LX-3)
DRIVER(tafg,dx3)        //                  Addams Family Special Collectors Edition, The (DX-3) LED Ghost Fix
DRIVER(tafg,la2)        //          10/94 B Addams Family Special Collectors Edition, The (LA-2)
DRIVER(tafg,da2)        //                  Addams Family Special Collectors Edition, The (DA-2) LED Ghost Fix
DRIVER(tafg,la3)        //          10/94 B Addams Family Special Collectors Edition, The (LA-3)
DRIVER(tafg,da3)        //                  Addams Family Special Collectors Edition, The (DA-3) LED Ghost Fix
DRIVER(wcs,l2)          //WPC-531:  05/94 B World Cup Soccer (LX-2)
DRIVER(wcs,d2)          //                  World Cup Soccer (DX-2) LED Ghost Fix
DRIVER(wcs,l3c)         //          06/16 B World Cup Soccer (LX-3C)
DRIVER(wcs,la2)         //          02/94 B World Cup Soccer (LA-2)
DRIVER(wcs,p2)          //          ??/?? B World Cup Soccer (PA-2 Prototype)
DRIVER(wcs,p5)          //                  World Cup Soccer (PA-5) LED Ghost Fix
DRIVER(wcs,p3)          //          ??/?? B World Cup Soccer (PX-3 Prototype)
DRIVER(wcs,p6)          //                  World Cup Soccer (PX-6) LED Ghost Fix
DRIVER(wcs,f10)         //          ??/??   World Cup Soccer (FreeWPC 0.10)
DRIVER(wcs,f50)         //          ??/??   World Cup Soccer (FreeWPC 0.50)
DRIVER(wcs,f62)         //          ??/??   World Cup Soccer (FreeWPC 0.62)
                        //WPC-620:  06/94 W Pinball Circus
DRIVER(fs,lx2)          //WPC-529:  07/94 W Flintstones, The (LX-2)
DRIVER(fs,dx2)          //                  Flintstones, The (DX-2) LED Ghost Fix
DRIVER(fs,sp2)          //          07/94 W Flintstones, The (SP-2)
DRIVER(fs,sp2d)         //                  Flintstones, The (SP-2D) LED Ghost Fix
DRIVER(fs,lx3)          //          08/94 W Flintstones, The (LX-3)
DRIVER(fs,lx4)          //          09/94 W Flintstones, The (LX-4)
DRIVER(fs,dx4)          //                  Flintstones, The (DX-4) LED Ghost Fix
DRIVER(fs,lx5)          //          11/94 W Flintstones, The (LX-5)
DRIVER(fs,dx5)          //                  Flintstones, The (DX-5) LED Ghost Fix
DRIVER(corv,px3)        //WPC-536:  08/94 B Corvette (PX-3 Prototype)
DRIVER(corv,px4)        //          09/94 B Corvette (PX-4 Prototype)
DRIVER(corv,px5)        //                  Corvette (PX-5) LED Ghost Fix
DRIVER(corv,la1)        //          09/94 B Corvette (LA-1)
DRIVER(corv,lx1)        //          09/94 B Corvette (LX-1)
DRIVER(corv,dx1)        //                  Corvette (DX-1) LED Ghost Fix
DRIVER(corv,lx2)        //          10/94 B Corvette (LX-2)
DRIVER(corv,21)         //          01/96 B Corvette (2.1)
DRIVER(corv,f61)        //          ??/??   Corvette (FreeWPC 0.61)
DRIVER(rs,l6)           //WPC-524:  10/94 W Red & Ted's Road Show (L-6)
DRIVER(rs,lx2p3)        //          10/94 W Red & Ted's Road Show (LX-2, Sound P-3)
DRIVER(rs,lx2)          //          10/94 W Red & Ted's Road Show (LX-2)
DRIVER(rs,dx2)          //                  Red & Ted's Road Show (DX-2) LED Ghost Fix
DRIVER(rs,lx3)          //          10/94 W Red & Ted's Road Show (LX-3)
DRIVER(rs,dx3)          //                  Red & Ted's Road Show (DX-3) LED Ghost Fix
DRIVER(rs,la4)          //          10/94 W Red & Ted's Road Show (LA-4)
DRIVER(rs,da4)          //                  Red & Ted's Road Show (DA-4) LED Ghost Fix
DRIVER(rs,lx4)          //          10/94 W Red & Ted's Road Show (LX-4)
DRIVER(rs,dx4)          //                  Red & Ted's Road Show (DX-4) LED Ghost Fix
DRIVER(rs,la5)          //          10/94 W Red & Ted's Road Show (LA-5)
DRIVER(rs,da5)          //                  Red & Ted's Road Show (DA-5) LED Ghost Fix
DRIVER(rs,lx5)          //          10/94 W Red & Ted's Road Show (LX-5)
DRIVER(rs,dx5)          //                  Red & Ted's Road Show (DX-5) LED Ghost Fix
DRIVER(rs,l6c)          //          05/19 W Red & Ted's Road Show (L6-C Competition MOD)
DRIVER(ts,pa1)          //WPC-532:  11/94 B Shadow, The (PA-1 Prototype)
DRIVER(ts,pa2)          //                  Shadow, The (PA-2) LED Ghost Fix
DRIVER(ts,la2)          //          12/94 B Shadow, The (LA-2)
DRIVER(ts,da2)          //                  Shadow, The (DA-2) LED Ghost Fix
DRIVER(ts,la4)          //          02/95 B Shadow, The (LA-4)
DRIVER(ts,da4)          //                  Shadow, The (DA-4) LED Ghost Fix
DRIVER(ts,lx4)          //          02/95 B Shadow, The (LX-4)
DRIVER(ts,dx4)          //                  Shadow, The (DX-4) LED Ghost Fix
DRIVER(ts,lx5)          //          05/95 B Shadow, The (LX-5)
DRIVER(ts,dx5)          //                  Shadow, The (DX-5) LED Ghost Fix
DRIVER(ts,la6)          //          05/95 B Shadow, The (LA-6)
DRIVER(ts,da6)          //                  Shadow, The (DA-6) LED Ghost Fix
DRIVER(ts,lh6)          //          05/95 B Shadow, The (LH-6, Home version)
DRIVER(ts,lh6p)         //          05/95 B Shadow, The (LH-6, Home version) wrong text index bugfix patch
DRIVER(ts,dh6)          //                  Shadow, The (DH-6, Home version) LED Ghost Fix
DRIVER(ts,lf6)          //          05/95 B Shadow, The (LF-6) French
DRIVER(ts,df6)          //                  Shadow, The (DF-6) French LED Ghost Fix
DRIVER(ts,lm6)          //          05/95 B Shadow, The (LM-6) Mild
DRIVER(ts,dm6)          //                  Shadow, The (DM-6) Mild LED Ghost Fix
DRIVER(dh,lx2)          //WPC-530:  01/95 W Dirty Harry (LX-2)
DRIVER(dh,dx2)          //                  Dirty Harry (DX-2) LED Ghost Fix
DRIVER(dh,lf2)          //                  Dirty Harry (LF-2 French)
DRIVER(tom,06)          //WPC-539:  03/95 B Theatre of Magic (0.6A)
DRIVER(tom,061)         //                  Theatre of Magic (0.61A) LED Ghost Fix
DRIVER(tom,10f)         //          04/95 B Theatre of Magic (1.0 French)
DRIVER(tom,101f)        //                  Theatre of Magic (1.01 French) LED Ghost Fix
DRIVER(tom,12)          //          04/95 B Theatre of Magic (1.2X)
DRIVER(tom,121)         //                  Theatre of Magic (1.21X) LED Ghost Fix
DRIVER(tom,13)          //          08/95 B Theatre of Magic (1.3X)
DRIVER(tom,13f)         //          08/95 B Theatre of Magic (1.3 French)
DRIVER(tom,14h)         //          10/96 B Theatre of Magic (1.4 Home version)
DRIVER(tom,14hb)        //                  Theatre of Magic (1.4 Home version Coin Play)
//DRIVER(tom,15c)       //          06/16 B Theatre of Magic (1.5C Competition MOD)  //outdated patch
DRIVER(tom,13c)         //          10/19 B Theatre of Magic (1.3XC Competition MOD) //patch f0f8
DRIVER(nf,10)           //WPC-525:  05/95 W No Fear: Dangerous Sports (1.0)
DRIVER(nf,101)          //                  No Fear: Dangerous Sports (1.01) LED Ghost Fix
DRIVER(nf,11x)          //WPC-525:  05/95 W No Fear: Dangerous Sports (1.1 Export)
DRIVER(nf,20)           //          05/95 W No Fear: Dangerous Sports (2.0)
DRIVER(nf,22)           //          05/95 W No Fear: Dangerous Sports (2.2)
DRIVER(nf,23)           //          05/95 W No Fear: Dangerous Sports (2.3)
DRIVER(nf,23f)          //          05/95 W No Fear: Dangerous Sports (2.3F)
DRIVER(nf,23x)          //          05/95 W No Fear: Dangerous Sports (2.3 Export)
DRIVER(i500,11r)        //WPC-526:  06/95 B Indianapolis 500 (1.1)
DRIVER(i500,11b)        //WPC-526:  06/95 B Indianapolis 500 (1.1 Belgian)
DRIVER(i500,10r)        //WPC-526:  06/95 B Indianapolis 500 (1.0)
DRIVER(jb,10r)          //WPC-551:  10/95 W Jack*Bot (1.0R)
DRIVER(jb,101r)         //                  Jack*Bot (1.01R LED Ghost Fix)
DRIVER(jb,10b)          //          10/95 W Jack*Bot (1.0B Belgish/Canadian)
DRIVER(jb,101b)         //                  Jack*Bot (1.01B Belgish/Canadian LED Ghost Fix)
DRIVER(jb,04a)          //                  Jack*Bot (0.4A Prototype)
DRIVER(jm,05r)          //WPC-542:  09/95 W Johnny Mnemonic (0.5R)
DRIVER(jm,12r)          //          10/95 W Johnny Mnemonic (1.2R)
DRIVER(jm,12b)          //          10/95 W Johnny Mnemonic (1.2 Belgian)
DRIVER(wd,03r)          //WPC-544:  09/95 B Who dunnit (0.3 R)
DRIVER(wd,048r)         //          10/95 B Who dunnit (0.48 R)
DRIVER(wd,10r)          //          11/95 B Who dunnit (1.0 R)
DRIVER(wd,10g)          //          11/95 B Who dunnit (1.0 German)
DRIVER(wd,10f)          //          11/95 B Who dunnit (1.0 French)
DRIVER(wd,11)           //         ?04/96 B Who dunnit (1.1)
DRIVER(wd,12)           //         ?05/96 B Who dunnit (1.2)
DRIVER(wd,12g)          //         ?05/96 B Who dunnit (1.2 German)
DRIVER(congo,11s10)     //WPC-550:  11/95 W Congo (1.1, DCS-Sound 1.0)
DRIVER(congo,11)        //          11/95 W Congo (1.1)
DRIVER(congo,13)        //          11/95 W Congo (1.3)
DRIVER(congo,20)        //          02/96 W Congo (2.0)
DRIVER(congo,21)        //          10/96 W Congo (2.1)
DRIVER(afm,10)          //WPC-541:  12/95 B Attack From Mars (1.0)
DRIVER(afm,11)          //          12/95 B Attack From Mars (1.1)
DRIVER(afm,11u)         //          ??/06 B Attack From Mars (1.1 Ultrapin)
DRIVER(afm,11pfx)       //          12/18 B Attack From Mars (1.1 Pinball FX)
DRIVER(afm,113)         //          12/95 B Attack From Mars (1.13 Home version)
DRIVER(afm,113b)        //          12/95 B Attack From Mars (1.13b Coin Play)
DRIVER(afm,f10)         //          ??/??   Attack From Mars (FreeWPC 0.10)
DRIVER(afm,f20)         //          ??/??   Attack From Mars (FreeWPC 0.20)
DRIVER(afm,f32)         //          ??/??   Attack From Mars (FreeWPC 0.32)
DRIVER(lc,11)           //WPC-107:  03/96 B League Champ (Shuffle Alley)
DRIVER(ttt,10)          //WPC-905:  03/96 W Ticket Tac Toe
DRIVER(sc,091)          //WPC-903:  06/96 B Safe Cracker (0.91)
DRIVER(sc,10)           //          06/96 B Safe Cracker (1.0)
DRIVER(sc,14)           //          06/96 B Safe Cracker (1.4)
DRIVER(sc,17)           //          11/96 B Safe Cracker (1.7)
DRIVER(sc,17n)          //          11/96 B Safe Cracker (1.7 No Percentaging)
DRIVER(sc,18)           //          04/98 B Safe Cracker (1.8, Sound S1.0)
DRIVER(sc,18n)          //          04/98 B Safe Cracker (1.8 No Percentaging, Sound S1.0)
DRIVER(sc,18s11)        //          04/98 B Safe Cracker (1.8, Sound S1.1)
DRIVER(sc,18n11)        //          04/98 B Safe Cracker (1.8 No Percentaging, Sound S1.1)
DRIVER(sc,18s2)         //          04/98 B Safe Cracker (1.8, German Sound S2.4)
DRIVER(sc,18ns2)        //          04/98 B Safe Cracker (1.8 No Percentaging, German Sound S2.4)
DRIVER(sc,18pfx)        //          03/19 B Safe Cracker (1.8 Pinball FX, Sound S1.0)
DRIVER(totan,04)        //WPC-547:  05/96 W Tales of the Arabian Nights (0.4)
DRIVER(totan,12)        //          06/96 W Tales of the Arabian Nights (1.2)
DRIVER(totan,13)        //          07/96 W Tales of the Arabian Nights (1.3)
DRIVER(totan,14)        //          10/96 W Tales of the Arabian Nights (1.4)
DRIVER(totan,15c)       //          04/16 W Tales of the Arabian Nights (1.5C)
DRIVER(ss,01)           //WPC-548:  09/96 B Scared Stiff (D0.1R with sound rev.25)
DRIVER(ss,01b)          //          09/96 B Scared Stiff (D0.1R with sound rev.25 Coin Play)
DRIVER(ss,03)           //          09/96 B Scared Stiff (0.3)
DRIVER(ss,12)           //          10/96 B Scared Stiff (1.2)
DRIVER(ss,14)           //          11/96 B Scared Stiff (1.4)
DRIVER(ss,15)           //          02/97 B Scared Stiff (1.5)
DRIVER(jy,03)           //WPC-552:  10/96 W Junk Yard (0.3)
DRIVER(jy,11)           //          01/97 W Junk Yard (1.1)
DRIVER(jy,12)           //          07/97 W Junk Yard (1.2)
DRIVER(jy,12c)          //          12/19 W Junk Yard (1.2C Competition MOD)
DRIVER(nbaf,11s)        //WPC-553:  03/97 B NBA Fastbreak (1.1, Sound S0.4)
DRIVER(nbaf,11)         //          03/97 B NBA Fastbreak (1.1)
DRIVER(nbaf,11a)        //          03/97 B NBA Fastbreak (1.1, Sound S2.0)
DRIVER(nbaf,115)        //          05/97 B NBA Fastbreak (1.15)
DRIVER(nbaf,21)         //          05/97 B NBA Fastbreak (2.1)
DRIVER(nbaf,22)         //          05/97 B NBA Fastbreak (2.2)
DRIVER(nbaf,23)         //          06/97 B NBA Fastbreak (2.3)
DRIVER(nbaf,31)         //          09/97 B NBA Fastbreak (3.1, Sound S3.0)
DRIVER(nbaf,31a)        //          09/97 B NBA Fastbreak (3.1, Sound S1.0)
DRIVER(mm,05)           //WPC-559:  06/97 W Medieval Madness (0.5 Prototype)
DRIVER(mm,10)           //          07/97 W Medieval Madness (1.0)
DRIVER(mm,10u)          //          ??/06 W Medieval Madness (1.0 Ultrapin)
DRIVER(mm,10pfx)        //          10/18 W Medieval Madness (1.0 Pinball FX)
DRIVER(mm,109)          //          06/99 W Medieval Madness (1.09 Home version)
DRIVER(mm,109b)         //                  Medieval Madness (1.09B Home version Coin Play)
DRIVER(mm,109c)         //                  Medieval Madness (1.09C Home version w/ profanity speech)
DRIVER(cv,d52)          //WPC-562:  08/97 B Cirqus Voltaire (D.52 Prototype w/ support for old Ringmaster voice)
DRIVER(cv,10)           //          10/97 B Cirqus Voltaire (1.0)
DRIVER(cv,11)           //          11/97 B Cirqus Voltaire (1.1)
DRIVER(cv,13)           //          04/98 B Cirqus Voltaire (1.3)
DRIVER(cv,14)           //          10/98 B Cirqus Voltaire (1.4)
DRIVER(cv,20h)          //          02/02 B Cirqus Voltaire (Home version)
DRIVER(cv,20hc)         //                  Cirqus Voltaire (Home version Coin Play)
DRIVER(ngg,p06)         //WPC-561:  10/97 W No Good Gofers (Prototype 0.6)
DRIVER(ngg,10)          //          12/97 W No Good Gofers (1.0)
DRIVER(ngg,12)          //WPC-561:  12/97 W No Good Gofers (1.2)
DRIVER(ngg,13)          //          04/98 W No Good Gofers (1.3)
DRIVER(cp,15)           //WPC-563:  07/98 B Champion Pub, The (1.5)
DRIVER(cp,16)           //          07/98 B Champion Pub, The (1.6)
DRIVER(cp,16pfx)        //          03/19 B Champion Pub, The (1.6 Pinball FX)
DRIVER(mb,05)           //WPC-565:  07/98 W Monster Bash (0.5)
DRIVER(mb,10)           //WPC-565:  07/98 W Monster Bash (1.0)
DRIVER(mb,106)                            // (1.06 Home version)
DRIVER(mb,106b)                           // (1.06b Coin Play)
DRIVER(cc,10)           //WPC-566:  10/98 B Cactus Canyon
DRIVER(cc,12)           //          02/99 B Cactus Canyon (1.2)
DRIVER(cc,13)           //          04/99 B Cactus Canyon (1.3)
DRIVER(cc,13k)          //                  Cactus Canyon (1.3) with real knocker
DRIVER(cc,104)          //          11/06 B Cactus Canyon (1.04 test 0.2 (The Pinball Factory) version)
//Test Fixtures
DRIVER(tfa,13)          //WPC-584T: 09/91   Test fixture Alphanumeric
DRIVER(tfdmd,l3)        //WPC-584T: 09/91   Test fixture DMD
DRIVER(tfs,12)          //WPC-584S: 01/95   Test fixture Security
DRIVER(tf95,12)         //WPC-648:  01/95   Test fixture WPC95

// ------------------
// ZACCARIA GAMES
// ------------------
                        //10/77 Combat (in all probability never produced in a SS version)
DRIVERNV(wsports)       //01/78 Winter Sports
DRIVERNV(hod)           //07/78 House of Diamonds
DRIVERNV(strike)        //09/78 Strike
DRIVERNV(skijump)       //10/78 Ski Jump
DRIVERNV(futurwld)      //10/78 Future World
DRIVERNV(strapids)      //04/79 Shooting the Rapids
DRIVERNV(hotwheel)      //09/79 Hot Wheels
DRIVERNV(spacecty)      //09/79 Space City
DRIVERNV(firemntn)      //01/80 Fire Mountain
DRIVERNV(stargod)       //05/80 Star God
DRIVERNV(stargoda)      //      Star God (alternate sound)
DRIVERNV(stargodb)      //      Star God (alternate version)
DRIVERNV(sshtlzac)      //09/80 Space Shuttle
DRIVERNV(ewf)           //04/81 Earth, Wind & Fire
DRIVERNV(locomotn)      //09/81 Locomotion
                        //04/82 Pinball Champ '82 (using the same roms as the '83 version)
DRIVERNV(socrking)      //09/82 Soccer Kings
DRIVERNV(socrkina)      //      Soccer Kings (alternate set)
DRIVERNV(socrkngi)      //      Soccer Kings (Italian Speech)
DRIVERNV(socrkngg)      //      Soccer Kings (German Speech)
DRIVERNV(sockfp)        //      Soccer Kings (Free Play)
DRIVERNV(sockifp)       //      Soccer Kings (Italian Speech Free Play)
DRIVERNV(sockgfp)       //      Soccer Kings (German Speech Free Play)
DRIVERNV(pinchamp)      //04/83 Pinball Champ
DRIVERNV(pinchamg)      //      Pinball Champ (German Speech)
DRIVERNV(pinchami)      //      Pinball Champ (Italian Speech)
DRIVERNV(pincham7)      //      Pinball Champ (7 digits)
DRIVERNV(pincha7g)      //      Pinball Champ (7 digits, German Speech)
DRIVERNV(pincha7i)      //      Pinball Champ (7 digits, Italian Speech)
DRIVERNV(pincfp)        //      Pinball Champ (Free Play)
DRIVERNV(pincgfp)       //      Pinball Champ (German Speech Free Play)
DRIVERNV(pincifp)       //      Pinball Champ (Italian Speech Free Play)
DRIVERNV(pinc7fp)       //      Pinball Champ (7 digits Free Play)
DRIVERNV(pinc7gfp)      //      Pinball Champ (7 digits, German Speech Free Play)
DRIVERNV(pinc7ifp)      //      Pinball Champ (7 digits, Italian Speech Free Play)
DRIVERNV(tmachzac)      //04/83 Time Machine
DRIVERNV(tmacgzac)      //      Time Machine (German Speech)
DRIVERNV(tmacfzac)      //      Time Machine (French Speech)
DRIVERNV(tmachfp)       //      Time Machine (Free Play)
DRIVERNV(tmacgfp)       //      Time Machine (German Speech Free Play)
DRIVERNV(tmacffp)       //      Time Machine (French Speech Free Play)
DRIVERNV(farfalla)      //09/83 Farfalla
DRIVERNV(farfalli)      //      Farfalla (Italian Speech)
DRIVERNV(farfallg)      //      Farfalla (German Speech)
DRIVERNV(farffp)        //      Farfalla (Free Play)
DRIVERNV(farfifp)       //      Farfalla (Italian Speech Free Play)
DRIVERNV(farfgfp)       //      Farfalla (German Speech Free Play)
DRIVERNV(dvlrider)      //04/84 Devil Riders
DRIVERNV(dvlridei)      //      Devil Riders (Italian Speech)
DRIVERNV(dvlrideg)      //      Devil Riders (German Speech)
DRIVERNV(dvlridef)      //      Devil Riders (French Speech)
DRIVERNV(dvlrdfp)       //      Devil Riders (Free Play)
DRIVERNV(dvlrdifp)      //      Devil Riders (Italian Speech Free Play))
DRIVERNV(dvlrdgfp)      //      Devil Riders (German Speech Free Play)
DRIVERNV(mcastle)       //09/84 Magic Castle
DRIVERNV(mcastlei)      //      Magic Castle (Italian Speech)
DRIVERNV(mcastleg)      //      Magic Castle (German Speech)
DRIVERNV(mcastlef)      //      Magic Castle (French Speech)
DRIVERNV(mcastfp)       //      Magic Castle (Free Play)
DRIVERNV(mcastifp)      //      Magic Castle (Italian Speech Free Play)
DRIVERNV(mcastgfp)      //      Magic Castle (German Speech Free Play)
DRIVERNV(mcastffp)      //      Magic Castle (French Speech Free Play)
DRIVERNV(robot)         //01/85 Robot
DRIVERNV(roboti)        //      Robot (Italian Speech)
DRIVERNV(robotg)        //      Robot (German Speech)
DRIVERNV(robotf)        //      Robot (French Speech)
DRIVERNV(robotfp)       //      Robot (Free Play)
DRIVERNV(robotifp)      //      Robot (Italian Speech Free Play)
DRIVERNV(robotgfp)      //      Robot (German Speech Free Play)
DRIVERNV(robotffp)      //      Robot (French Speech Free Play)
DRIVERNV(clown)         //07/85 Clown
DRIVERNV(clownfp)       //      Clown (Free Play)
DRIVERNV(poolcham)      //12/85 Pool Champion
DRIVERNV(poolchai)      //      Pool Champion (Italian Speech)
DRIVERNV(poolcfp)       //      Pool Champion (Free Play)
DRIVERNV(myststar)      //??/86 Mystic Star
DRIVERNV(bbeltzac)      //03/86 Black Belt
DRIVERNV(bbeltzai)      //      Black Belt (Italian Speech)
DRIVERNV(bbeltzag)      //      Black Belt (German Speech)
DRIVERNV(bbeltzaf)      //      Black Belt (French Speech)
DRIVERNV(bbeltzfp)      //      Black Belt (Free Play)
DRIVERNV(bbeltifp)      //      Black Belt (Italian Speech Free Play)
DRIVERNV(bbeltgfp)      //      Black Belt (German Speech Free Play)
DRIVERNV(bbeltffp)      //      Black Belt (French Speech Free Play)
DRIVERNV(mexico)        //07/86 Mexico 86
DRIVERNV(mexicofp)      //      Mexico 86 (Free Play)
DRIVERNV(zankor)        //12/86 Zankor
DRIVERNV(zankorfp)      //      Zankor (Free Play)
DRIVERNV(spooky)        //04/87 Spooky
DRIVERNV(spookyi)       //      Spooky (Italian Speech)
DRIVERNV(spookyfp)      //      Spooky (Free Play)
DRIVERNV(spookifp)      //      Spooky (Italian Speech Free Play)
DRIVERNV(strsphnx)      //07/87 Star's Phoenix
DRIVERNV(strsphnf)      //      Star's Phoenix (French Speech)
DRIVERNV(strsphfp)      //      Star's Phoenix (Free Play)
DRIVERNV(strspffp)      //      Star's Phoenix (French Speech Free Play)
DRIVERNV(nstrphnx)      //08/87 New Star's Phoenix (same roms as strsphnx)
DRIVERNV(nstrphnf)      //      New Star's Phoenix (French Speech)
DRIVERNV(nstrphfp)      //      New Star's Phoenix (Free Play)
DRIVERNV(nstrpffp)      //      New Star's Phoenix (French Speech Free Play)

#endif /* DRIVER_RECURSIVE */
