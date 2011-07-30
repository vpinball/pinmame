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

DRIVERNV(st_game)       //Unknown game running on old Stern hardware
DRIVERNV(mac_zois)      // 05/03 machinaZOIS Virtual Training Center

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
DRIVERNV(agsoccer)    //A.G. Soccer (1992)
//DRIVERNV(agfootbl)  //A.G. Football (1992)
DRIVERNV(wrldtour)    //Al's Garage Band Goes On A World Tour (1992)
DRIVERNV(wrldtou2)    //Al's Garage Band Goes On A World Tour R02b (1992)
DRIVERNV(usafootb)      //U.S.A. Football (1993)
DRIVERNV(dinoeggs)    //Dinosaur Eggs (1993)
//DRIVERNV(dualpool)  //Dual Pool (1993) - 1 Unit
//DRIVERNV(maxbadaz)  //Max Badazz (1993) - 1 Unit
DRIVERNV(mystcast)    //Mystery Castle (1993)
DRIVERNV(pstlpkr)   //Pistol Poker (1993)
DRIVERNV(punchy)        //Punchy the Clown (1993)
            //A-MAZE-ING Baseball (1994) - any units?
//DRIVERNV(slamnjam)  //Slam N Jam (1994) - 2 Units

#ifdef MAME_DEBUG
  DRIVERNV(test8031)    //Test 8031 cpu core
#endif

// ---------------
// ASTRO GAMES
// ---------------
DRIVERNV(blkshpsq)      //Black Sheep Squadron (1979) - using old Stern hardware

// ---------------
// ATARI GAMES
// ---------------
                        //Triangle (Prototype, 1976?)
DRIVERNV(atarians)      //The Atarians (November 1976)
DRIVERNV(atarianb)      //The Atarians (2002 bootleg)
DRIVERNV(time2000)      //Time 2000 (June 1977)
DRIVERNV(aavenger)      //Airborne Avenger (September 1977)
DRIVERNV(midearth)      //Middle Earth (February 1978)
DRIVERNV(spcrider)      //Space Riders (September 1978)
DRIVERNV(superman)      //Superman (March 1979)
DRIVERNV(hercules)      //Hercules (May 1979)
DRIVERNV(roadrunr)      //Road Runner (Prototype, 1979)
                        //Monza (Prototype, 1980)
                        //Neutron Star (Prototype, 1981)
                        //4x4 (Prototype, 1983)

// ---------------
// BALLY GAMES
// ---------------
//S2650 hardware
DRIVERNV(cntinntl)      //          10/80 Continental (Bingo)

//MPU-17
DRIVERNV(bowarrow)      //          08/76 Bow & Arrow (Prototype)
DRIVERNV(freedom )      //BY17-720: 08/76 Freedom
DRIVERNV(nightr20)      //BY17-721: 01/76 Night Rider (rev. 20, EM release date)
DRIVERNV(nightrdr)      //BY17-721: 01/76 Night Rider (rev. 21)
DRIVERNV(nightrdb)      //BY17-721: 10/08 Night Rider (Free Play)
DRIVERNV(evelknie)      //BY17-722: 09/76 Evel Knievel
DRIVERNV(evelknib)      //BY17-722: 10/08 Evel Knievel (Free Play)
DRIVERNV(eightbll)      //BY17-723: 01/77 Eight Ball
DRIVERNV(eightblb)      //BY17-723: 10/08 Eight Ball (Free Play)
DRIVERNV(pwerplay)      //BY17-724: 02/77 Power Play
DRIVERNV(pwerplab)      //BY17-724: 10/08 Power Play (Free Play)
DRIVERNV(stk_sprs)      //BY17-740: 08/77 Strikes and Spares
DRIVERNV(stk_sprb)      //BY17-740: 10/08 Strikes and Spares (Free Play)
DRIVERNV(matahari)      //BY17-725: 09/77 Mata Hari
DRIVERNV(matatest)      //BY17      ??/06 Mata Hari (new game rules)
DRIVERNV(mataharb)      //BY17      10/08 Mata Hari (Free Play)
DRIVERNV(blackjck)      //BY17-728: 05/76 Black Jack  (EM release date)
DRIVERNV(blackjcb)      //BY17-728: 10/08 Black Jack  (Free Play)
//MPU-35
DRIVERNV(lostwrld)      //BY35-729: 02/77 Lost World
DRIVERNV(lostwldb)      //BY35-729: 10/08 Lost World (Free Play)
DRIVERNV(sst     )      //BY35-741: 10/76 Supersonic (EM "Star Ship" release date)
DRIVERNV(sstb    )      //BY35 	    05/05 Supersonic (7-digit conversion)
DRIVERNV(sstc    )      //BY35 	    10/08 Supersonic (7-digit Rev3 Free Play)
DRIVERNV(sstd    )      //BY35 	    10/08 Supersonic (/10 Free Play)
DRIVERNV(smman   )      //BY35-742: 08/77 The Six Million Dollar Man
DRIVERNV(smmanb  )      //BY35      05/05 The Six Million Dollar Man (7-digit conversion)
DRIVERNV(smmanc  )      //BY35      10/08 The Six Million Dollar Man (7-digit Rev3 Free Play)
DRIVERNV(smmand  )      //BY35      10/08 The Six Million Dollar Man (/10 Free Play)
DRIVERNV(playboy )      //BY35-743: 09/76 Playboy (release date is wrong!)
DRIVERNV(playboyb)      //BY35 	    05/05 Playboy (7-digit conversion)
DRIVERNV(playboyc)      //BY35 	    10/08 Playboy (7-digit Rev3 Free Play)
DRIVERNV(playboyd)      //BY35 	    10/08 Playboy (/10 Free Play)
                        //??        ??/78 Big Foot
DRIVERNV(voltan  )      //BY35-744: 01/78 Voltan Escapes Cosmic Doom
DRIVERNV(voltanb )      //BY35      05/05 Voltan Escapes Cosmic Doom (7-digit conversion)
DRIVERNV(voltanc )      //BY35      10/08 Voltan Escapes Cosmic Doom (7-digit Rev3 Free Play)
DRIVERNV(voltand )      //BY35      10/08 Voltan Escapes Cosmic Doom (/10 Free Play)
DRIVERNV(startrek)      //BY35-745: 01/78 Star Trek
DRIVERNV(startreb)      //BY35      05/05 Star Trek (7-digit conversion)
DRIVERNV(startrec)      //BY35      10/08 Star Trek (7-digit Rev3 Free Play)
DRIVERNV(startred)      //BY35      10/08 Star Trek (/10 Free Play)
                        //??        02/78 Skateball (Prototype)
DRIVERNV(kiss    )      //BY35-746: 04/78 Kiss
DRIVERNV(kissb   )      //BY35      05/05 Kiss (7-digit conversion)
DRIVERNV(kissc   )      //BY35      10/08 Kiss (7-digit Rev3 Free Play)
DRIVERNV(kissd   )      //BY35      10/08 Kiss (/10 Free Play)
DRIVERNV(kissp   )      //BY35      ??/?? Kiss (prototype)
DRIVERNV(ngndshkr)      //BY35-776: 05/78 Nitro Ground Shaker
DRIVERNV(ngndshkb)      //BY35      11/02 Nitro Ground Shaker (7-digit conversion)
DRIVERNV(slbmania)      //BY35-786: 06/78 Silverball Mania
DRIVERNV(slbmanib)      //BY35      11/02 Silverball Mania (7-digit conversion)
DRIVERNV(hglbtrtr)      //BY35-750: 08/78 Harlem Globetrotters On Tour
DRIVERNV(hglbtrtb)      //BY35      11/02 Harlem Globetrotters On Tour (7-digit conversion)
DRIVERNV(dollyptn)      //BY35-777: 10/78 Dolly Parton
DRIVERNV(dollyptb)      //BY35      11/02 Dolly Parton (7-digit conversion)
DRIVERNV(paragon )      //BY35-748: 12/78 Paragon
DRIVERNV(paragonb)      //BY35      05/05 Paragon (7-digit conversion)
DRIVERNV(paragonc)      //BY35      10/08 Paragon (7-digit Rev3 Free Play)
DRIVERNV(paragond)      //BY35      10/08 Paragon (/10 Free Play)
DRIVERNV(futurspa)      //BY35-781: 03/79 Future Spa
DRIVERNV(futurspb)      //BY35      11/02 Future Spa (7-digit conversion)
DRIVERNV(spaceinv)      //BY35-792: 05/79 Space Invaders
DRIVERNV(spaceinb)      //BY35      11/02 Space Invaders (7-digit conversion)
DRIVERNV(rollston)      //BY35-796: 06/79 Rolling Stones
DRIVERNV(rollstob)      //BY35      11/02 Rolling Stones (7-digit conversion)
DRIVERNV(mystic  )      //BY35-798: 08/79 Mystic
DRIVERNV(mysticb )      //BY35      11/02 Mystic (7-digit conversion)
DRIVERNV(xenon   )      //BY35-811: 11/79 Xenon
DRIVERNV(xenona  )      //BY35-811: 10/08 Xenon (Free Play)
DRIVERNV(xenonf  )      //BY35      11/79 Xenon (French)
DRIVERNV(xenonfa )      //BY35      10/08 Xenon (French Free Play)
DRIVERNV(hotdoggn)      //BY35-809: 12/79 Hotdoggin'
DRIVERNV(hotdogga)      //BY35      11/02 Hotdoggin' (Free Play)
DRIVERNV(hotdoggb)      //BY35      11/02 Hotdoggin' (7-digit conversion)
DRIVERNV(viking  )      //BY35-802: 12/79 Viking
DRIVERNV(vikingb )      //BY35      11/02 Viking (7-digit conversion)
DRIVERNV(skatebll)      //BY35-823: 04/80 Skateball
DRIVERNV(skatebla)      //BY35-823: 10/08 Skateball (Free Play)
DRIVERNV(skateblb)      //BY35      09/05 Skateball (rev. 3)
DRIVERNV(frontier)      //BY35-819: 05/80 Frontier
DRIVERNV(frontiea)      //BY35-819: 10/08 Frontier (Free Play)
DRIVERNV(flashgdn)      //BY35-834: 05/80 Flash Gordon
DRIVERNV(flashgda)      //BY35      10/08 Flash Gordon (Free Play)
DRIVERNV(flashgdv)      //BY35      05/80 Flash Gordon (Vocalizer sound)
DRIVERNV(flashgva)      //BY35      10/08 Flash Gordon (Vocalizer sound Free Play)
DRIVERNV(flashgdf)      //BY35      05/80 Flash Gordon (French)
DRIVERNV(flashgfa)      //BY35      10/08 Flash Gordon (French Free Play)
DRIVERNV(flashgdp)      //BY35      ??/8? Flash Gordon (68701 hardware prototype)
DRIVERNV(flashgp2)      //BY35      ??/8? Flash Gordon (6801 hardware prototype)
DRIVERNV(eballd14)      //BY35-838: 09/80 Eight Ball Deluxe (rev. 14)
DRIVERNV(eballdlx)      //BY35      02/05 Eight Ball Deluxe (rev. 15)
DRIVERNV(eballdla)      //BY35      10/08 Eight Ball Deluxe (Free Play)
DRIVERNV(eballdlb)      //BY35      02/07 Eight Ball Deluxe custom rom rev.29
DRIVERNV(eballdlc)      //BY35      10/07 Eight Ball Deluxe custom rom rev.32
DRIVERNV(eballdp1)      //BY35      ??/81 Eight Ball Deluxe (68701 hardware prototype, rev. 1)
DRIVERNV(eballdp2)      //BY35      ??/81 Eight Ball Deluxe (68701 hardware prototype, rev. 2)
DRIVERNV(eballdp3)      //BY35      ??/81 Eight Ball Deluxe (68701 hardware prototype, rev. 3)
DRIVERNV(eballdp4)      //BY35      ??/81 Eight Ball Deluxe (68701 hardware prototype, rev. 4)
DRIVERNV(fball_ii)      //BY35-839: 09/80 Fireball II
DRIVERNV(fball_ia)      //BY35      10/08 Fireball II (Free Play)
DRIVERNV(embryon )      //BY35-841: 09/80 Embryon
DRIVERNV(embryona)      //BY35      10/08 Embryon (Free Play)
DRIVERNV(embryonb)      //BY35      10/02 Embryon (7-digit conversion rev.1)
DRIVERNV(embryonc)      //BY35      05/04 Embryon (7-digit conversion rev.8)
DRIVERNV(embryond)      //BY35      10/04 Embryon (7-digit conversion rev.9)
DRIVERNV(fathom  )      //BY35-842: 12/80 Fathom
DRIVERNV(fathoma )      //BY35      10/04 Fathom (Free Play)
DRIVERNV(fathomb )      //BY35      10/04 Fathom (modified rules)
DRIVERNV(medusa  )      //BY35-845: 02/81 Medusa
DRIVERNV(medusaa )      //BY35      10/04 Medusa (Free Play)
#ifdef MAME_DEBUG
DRIVERNV(medusaf )      //BY35      02/81 Medusa (6802 board)
#endif
DRIVERNV(centaur )      //BY35-848: 02/81 Centaur
DRIVERNV(centaura)      //BY35      10/04 Centaur (Free Play)
DRIVERNV(centaurb)      //BY35      10/08 Centaur (Free Play Rev. 27)
DRIVERNV(elektra )      //BY35-857: 03/81 Elektra
DRIVERNV(elektraa)      //BY35      10/04 Elektra (Free Play)
DRIVERNV(vector  )      //BY35-858: 03/81 Vector
DRIVERNV(vectora )      //BY35      10/04 Vector (Free Play)
DRIVERNV(vectorb )      //BY35      10/04 Vector (modified rules)
DRIVERNV(spectrum)      //BY35-868: 04/81 Spectrum
DRIVERNV(spectrua)      //BY35      10/04 Spectrum (Free Play)
DRIVERNV(spectru4)      //BY35      04/81 Spectrum (Rev. 4)
DRIVERNV(spectr4a)      //BY35      10/04 Spectrum (Rev. 4 Free Play)
DRIVERNV(speakesy)      //BY35-877: 08/82 Speakeasy
DRIVERNV(speakesa)      //BY35      10/04 Speakeasy (Free Play)
DRIVERNV(speakes4)      //BY35      08/82 Speakeasy 4 (4 player)
DRIVERNV(speake4a)      //BY35      10/04 Speakeasy 4 (4 player Free Play)
DRIVERNV(rapidfir)      //BY35-869: 06/81 Rapid Fire
DRIVERNV(rapidfia)      //BY35      09/05 Rapid Fire (Free Play)
DRIVERNV(m_mpac  )      //BY35-872: 05/82 Mr. & Mrs. Pac-Man
DRIVERNV(m_mpaca )      //BY35      10/04 Mr. & Mrs. Pac-Man (Free Play)
/*same as eballdlx*/    //BY35      10/82 Eight Ball Deluxe Limited Edition
DRIVERNV(babypac )      //BY35-891  10/82 Baby Pac-Man
DRIVERNV(babypacn)      //BY35      06/06 Baby Pac-Man (home roms)
DRIVERNV(bmx     )      //BY35-888: 11/82 BMX
DRIVERNV(bmxa    )      //BY35      10/04 BMX (Free Play)
DRIVERNV(granslam)      //BY35-1311:01/83 Grand Slam
DRIVERNV(granslaa)      //BY35      12/04 Grand Slam (Free Play)
DRIVERNV(gransla4)      //BY35      01/83 Grand Slam (4 player)
DRIVERNV(gransl4a)      //BY35      12/04 Grand Slam (4 player Free Play)
/*same as centaur*/     //BY35      06/83 Centaur II
DRIVERNV(goldball)      //BY35-1371:10/83 Gold Ball
DRIVERNV(goldbalb)      //BY35      03/04 Gold Ball (7-digit conversion)
DRIVERNV(goldbalc)      //BY35      03/05 Gold Ball (6/7-digit alternate set rev.10)
DRIVERNV(goldbaln)      //BY35      10/83 Gold Ball (alternate)
DRIVERNV(xsandos )      //BY35-1391:12/83 X's & O's
DRIVERNV(xsandosa)      //BY35      10/04 X's & O's (Free Play)
                        //??        ??/84 Mysterian
DRIVERNV(granny )       //BY35-1369:01/84 Granny and the Gators
DRIVERNV(kosteel )      //BY35-1390:05/84 Kings of Steel
DRIVERNV(kosteela)      //BY35      10/04 Kings of Steel (Free Play)
DRIVERNV(blakpyra)      //BY35-0A44 04/84 Black Pyramid
DRIVERNV(blakpyrb)      //BY35      10/04 Black Pyramid (Free Play)
DRIVERNV(mdntmrdr)      //          05/84 Midnight Marauders (gun game)
DRIVERNV(spyhuntr)      //BY35-0A17:07/84 Spy Hunter
DRIVERNV(spyhunta)      //BY35      10/04 Spy Hunter (Free Play)
                        //??        ??/85 Hot Shotz
DRIVERNV(fbclass )      //BY35-0A40 10/84 Fireball Classic
DRIVERNV(fbclassa)      //BY35      10/04 Fireball Classic (Free Play)
DRIVERNV(cybrnaut)      //BY35-0B42 02/85 Cybernaut
DRIVERNV(cybrnaua)      //BY35      10/04 Cybernaut (Free Play)
//MPU-6803
DRIVERNV(eballchp)      //6803-0B38:09/85 Eight Ball Champ
DRIVERNV(eballch2)      //6803      09/85 Eight Ball Champ (cheap squeak)
DRIVERNV(beatclck)      //6803-0C70:11/85 Beat the Clock
DRIVERNV(ladyluck)      //6803-0E34:02/86 Lady Luck
DRIVERNV(motrdome)      //6803-0E14:05/86 MotorDome
                        //6803-????:06/86 Karate Fight (Prototype for Black Belt?)
DRIVERNV(blackblt)      //6803-0E52:07/86 Black Belt
DRIVERNV(specforc)      //6803-0E47:08/86 Special Force
DRIVERNV(strngsci)      //6803-0E35:10/86 Strange Science
DRIVERNV(cityslck)      //6803-0E79:03/87 City Slicker
DRIVERNV(hardbody)      //6803-0E94:02/87 Hardbody
DRIVERNV(hardbdyg)      //6803      03/87 Hardbody (German)
DRIVERNV(prtyanim)      //6803-0H01:05/87 Party Animal
DRIVERNV(hvymetal)      //6803-0H03:08/87 Heavy Metal Meltdown
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

// ------------------
// (NUOVA) BELL GAMES
// ------------------
// Bell Coin Matics
            //      ??/78 The King
            //BY35      ??/79 Sexy Girl (Bally Playboy clone with image projector)
            //      ??/80 The Hunter
            //      ??/80 White Shark
            //      ??/80 Cosmodrome
// Bell Games
            //      01/82 Magic Picture Pin
            //BY35      ??/82 Fantasy (Bally Centaur clone)
            //      02/83 Pinball (Zaccaria Pinball Champ '82 clone)
            //      ??/8? Movie (Zaccaria Pinball Champ clone)
            //BY35      12/83 Pin Ball Pool (Bally Eight Ball Deluxe clone)
DRIVERNV(suprbowl)    //BY35      06/84 Super Bowl (Bally X's & O's clone)
DRIVERNV(tigerrag)    //BY35      ??/84 Tiger Rag (Bally Kings Of Steel clone)
DRIVERNV(cosflash)    //BY35      ??/85 Cosmic Flash (Bally Flash Gordon clone)
DRIVERNV(newwave)     //BY35      04/85 New Wave (Bally Black Pyramid clone)
DRIVERNV(saturn2)     //BY35      08/85 Saturn 2 (Bally Spy Hunter clone)
            //      ??/?? World Cup / World Championship (redemption game)
// Nuova Bell Games
DRIVERNV(worlddef)    //BY35      11/85 World Defender
DRIVERNV(spacehaw)      //BY35      04/86 Space Hawks (Bally Cybernaut clone)
DRIVERNV(darkshad)    //BY35      ??/86 Dark Shadow
DRIVERNV(skflight)    //BY35      09/86 Skill Flight
DRIVERNV(cobra)     //BY35      02/87 Cobra
DRIVERNV(futrquen)      //BY35      07/87 Future Queen
DRIVERNV(f1gp)          //BY35ALPHA 12/87 F1 Grand Prix
DRIVERNV(toppin)    //BY35      01/88 Top Pin (WMS Pin*Bot conversion)
DRIVERNV(uboat65)   //BY35ALPHA 04/88 U-Boat 65

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
DRIVERNV(bsv103)        // 05/96    Breakshot (1.3)
DRIVERNV(bsb105)        // 05/96    Breakshot (Beta, 1.5)
DRIVERNV(ffv101)        // 10/96    Flipper Football (1.01)
DRIVERNV(ffv104)        // 10/96    Flipper Football (1.04)
DRIVERNV(bbb108)        // 11/96    Big Bang Bar (Beta, 1.8)
DRIVERNV(bbb109)        // 11/96    Big Bang Bar (Beta, 1.9)
DRIVERNV(kpv106)        // 12/96    Kingpin

// -------------------
// DATA EAST GAMES
// -------------------
//4 x 2 x 7 (mixed) + credits A/N Display
DRIVER(lwar,a83)        //Data East MPU: 05/87 Laser War (8.3)
DRIVER(lwar,e90)        //Data East MPU: 05/87 Laser War (9.0 Europe)
//4 x 2 x 7 (mixed) A/N Display
DRIVER(ssvc,a26)        //Data East MPU: 03/88 Secret Service
DRIVER(torp,e21)        //Data East MPU: 08/88 Torpedo Alley
DRIVER(tmac,a18)        //Data East MPU: 12/88 Time Machine (1.8)
DRIVER(tmac,a24)        //Data East MPU: 12/88 Time Machine (2.4)
DRIVER(play,a24)        //Data East MPU: 05/89 Playboy 35th Anniversary
//2 x 16 A/N Display
DRIVER(mnfb,c27)        //Data East MPU: 09/89 ABC Monday Night Football
DRIVER(robo,a34)        //Data East MPU: 11/89 Robocop
DRIVER(poto,a32)        //Data East MPU: 01/90 Phantom of the Opera
DRIVER(bttf,a20)        //Data East MPU: 06/90 Back to the Future (2.0)
DRIVER(bttf,a21)        //Data East MPU: ??/90 Back to the Future (2.1)
DRIVER(bttf,a27)        //Data East MPU: 12/90 Back to the Future (2.7)
DRIVER(bttf,g27)        //Data East MPU: ??/9? Back to the Future (2.7 Germany)
DRIVER(simp,a20)        //Data East MPU: 09/90 The Simpsons (2.0)
DRIVER(simp,a27)        //Data East MPU: 09/90 The Simpsons (2.7)
//DMD 128 x 16
DRIVER(ckpt,a17)        //Data East MPU: 02/91 Checkpoint
DRIVER(tmnt,103)        //Data East MPU: 05/91 Teenage Mutant Ninja Turtles (1.03)
DRIVER(tmnt,104)        //Data East MPU: 05/91 Teenage Mutant Ninja Turtles (1.04)
//BSMT2000 Sound chip
DRIVER(btmn,101)        //Data East MPU: 07/91 Batman (1.01)
DRIVER(btmn,103)        //Data East MPU: ??/91 Batman (1.03)
DRIVER(btmn,g13)        //Data East MPU: ??/91 Batman (1.03 Germany)
DRIVER(btmn,106)        //Data East MPU: ??/91 Batman (1.06)
DRIVER(trek,11a)        //Data East MPU: 11/91 Star Trek 25th Anniversary (1.10 Alpha Display)
DRIVER(trek,110)        //Data East MPU: 11/91 Star Trek 25th Anniversary (1.10)
DRIVER(trek,120)        //Data East MPU: 01/92 Star Trek 25th Anniversary (1.20)
DRIVER(trek,200)        //Data East MPU: 04/92 Star Trek 25th Anniversary (2.00)
DRIVER(trek,201)        //Data East MPU: 04/92 Star Trek 25th Anniversary (2.01)
DRIVER(hook,401)        //Data East MPU: 01/92 Hook (4.01)
DRIVER(hook,404)        //Data East MPU: 01/92 Hook (4.04)
DRIVER(hook,408)        //Data East MPU: 01/92 Hook (4.08)
//DMD 128 x 32
DRIVER(lw3,200)         //Data East MPU: 06/92 Lethal Weapon (2.00)
DRIVER(lw3,205)         //Data East MPU: 07/92 Lethal Weapon (2.05)
DRIVER(lw3,207)         //Data East MPU: 08/92 Lethal Weapon (2.07 Canada)
DRIVER(lw3,208)         //Data East MPU: 11/92 Lethal Weapon (2.08)
DRIVER(aar,101)         //Data East MPU: 12/92 Aaron Spelling (1.01)
DRIVER(stwr,103)        //Data East MPU: 10/92 Star Wars (1.03)
DRIVER(stwr,a14)        //Data East MPU: 10/92 Star Wars (Display Rev.1.04)
DRIVER(stwr,g11)        //Data East MPU: 10/92 Star Wars (1.01 Germany)
DRIVER(stwr,102)        //Data East MPU: 11/92 Star Wars (1.02)
DRIVER(stwr,e12)        //Data East MPU: 11/92 Star Wars (1.02 England)
DRIVER(rab,103)         //Data East MPU: 02/93 Rocky & Bullwinkle (1.03 Spain)
DRIVER(rab,130)         //Data East MPU: 04/93 Rocky & Bullwinkle (1.30)
DRIVER(rab,320)         //Data East MPU: 08/93 Rocky & Bullwinkle (3.20)
DRIVER(jupk,501)        //Data East MPU: 09/93 Jurassic Park (5.01)
DRIVER(jupk,g51)        //Data East MPU: 09/93 Jurassic Park (5.01 Germany)
DRIVER(jupk,513)        //Data East MPU: 09/93 Jurassic Park (5.13)
DRIVER(lah,l104)        //Data East MPU: 08/93 Last Action Hero (1.04 Spain)
DRIVER(lah,l108)        //Data East MPU: 08/93 Last Action Hero (1.08 Spain)
DRIVER(lah,110)         //Data East MPU: 08/93 Last Action Hero (1.10)
DRIVER(lah,112)         //Data East MPU: 08/93 Last Action Hero (1.12)
DRIVER(tftc,104)        //Data East MPU: 11/93 Tales From the Crypt (1.04 Spain)
DRIVER(tftc,200)        //Data East MPU: 11/93 Tales From the Crypt (2.00)
DRIVER(tftc,300)	//Data East MPU: 11/93 Tales From the Crypt (3.00)
DRIVER(tftc,302)	//Data East MPU: 11/93 Tales From the Crypt (3.02 Dutch)
DRIVER(tftc,303)        //Data East MPU: 11/93 Tales From the Crypt (3.03)
DRIVER(tomy,h30)        //Data East MPU: 02/94 Tommy (3.00 Holland)
DRIVER(tomy,400)        //Data East MPU: 02/94 Tommy (4.00)
DRIVER(wwfr,103)        //Data East MPU: 05/94 WWF Royal Rumble (1.03)
DRIVER(wwfr,106)        //Data East MPU: 08/94 WWF Royal Rumble (1.06)
DRIVER(gnr,300)         //Data East MPU: 07/94 Guns N Roses
//MISC
DRIVERNV(detest)        //Data East MPU: ??/?? DE Test Chip

// --------------------
// FASCINATION INTERNATIONAL, INC.
// --------------------
DRIVERNV(royclark)      // 09/77 Roy Clark - The Entertainer
DRIVERNV(circa33)       // 02/79 Circa 1933
DRIVERNV(erosone)       // 03/79 Eros One

// ---------------
// GAMATRON GAMES
// ---------------
DRIVERNV(gamatron)      //Pinstar Gamatron (December 85)

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
DRIVERNV(andromed)      //Andromeda (August 1985)
DRIVERNV(andromea)      //Andromeda (alternate set)
DRIVERNV(cyclopes)      //Cyclopes (November 1985)
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
DRIVERNV(spidermn)      //S80-653:  05/80 The Amazing Spider-man
DRIVERNV(spiderm7)      //          01/08 The Amazing Spider-man (7-digit conversion)
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
DRIVERNV(forceii7)      //          01/08 Force II, 7-digit conversion)
DRIVERNV(pnkpnthr)      //S80-664:  03/81 Pink Panther
DRIVERNV(pnkpntr7)      //          01/08 Pink Panther, 7-digit conversion)
DRIVERNV(mars)          //S80-666:  03/81 Mars God of War
DRIVERNV(mars7)         //          01/08 Mars God of War, 7-digit conversion)
DRIVERNV(vlcno_ax)      //S80-667:  09/81 Volcano (Sound & Speech)
DRIVERNV(vlcno_a7)      //          01/08 Volcano (Sound & Speech, 7-digit conversion)
DRIVERNV(vlcno_1b)      //                Volcano (Sound Only)
DRIVERNV(vlcno_b7)      //          01/08 Volcano (Sound Only, 7-digit conversion)
DRIVERNV(vlcno_1a)      //                Volcano (Sound Only, alternate version)
DRIVERNV(blckhole)      //S80-668:  10/81 Black Hole (Sound & Speech, Rev 4)
DRIVERNV(blkhole7)      //          01/08 Black Hole (Sound & Speech, Rev 4, 7-digit conversion)
DRIVERNV(blkhole2)      //                Black Hole (Sound & Speech, Rev 2)
DRIVERNV(blkholea)      //                Black Hole (Sound Only)
DRIVERNV(blkhol7s)      //          01/08 Black Hole (Sound Only, 7-digit conversion)
                        //          ??/81 Night Hawk (never produced, by Premier, before the merger)
DRIVERNV(hh)            //S80-669:  06/82 Haunted House (Rev 2)
DRIVERNV(hh7)           //          01/08 Haunted House (Rev 2, 7-digit conversion)
DRIVERNV(hh_1)          //                Haunted House (Rev 1)
DRIVERNV(eclipse)       //S80-671:  ??/82 Eclipse
DRIVERNV(eclipse7)      //          01/08 Eclipse (7-digit conversion)
DRIVERNV(s80tst)        //S80: Text Fixture
//System 80a
DRIVERNV(dvlsdre)       //S80a-670: 08/82 Devil's Dare (Sound & Speech)
DRIVERNV(dvlsdre2)      //                Devil's Dare (Sound Only)
DRIVERNV(caveman)       //S80a-PV810:09/82 Caveman
DRIVERNV(cavemana)      //                Caveman (set 2)
DRIVERNV(rocky)         //S80a-672: 09/82 Rocky
DRIVERNV(spirit)        //S80a-673: 11/82 Spirit
DRIVERNV(striker)       //S80a-675: 11/82 Striker
DRIVERNV(punk)          //S80a-674: 12/82 Punk!
DRIVERNV(krull)         //S80a-676: 02/83 Krull
DRIVERNV(goinnuts)      //S80a-682: 02/83 Goin' Nuts
DRIVERNV(qbquest)       //S80a-677: 03/83 Q*bert's Quest
DRIVERNV(sorbit)        //S80a-680: 05/83 Super Orbit
DRIVERNV(rflshdlx)      //S80a-681: 06/83 Royal Flush Deluxe
DRIVERNV(amazonh)       //S80a-684: 09/83 Amazon Hunt
DRIVERNV(rackemup)      //S80a-685: 11/83 Rack 'Em Up
DRIVERNV(raimfire)      //S80a-686: 11/83 Ready Aim Fire
DRIVERNV(jack2opn)      //S80a-687: 05/84 Jacks to Open
DRIVERNV(alienstr)      //S80a-689: 06/84 Alien Star
DRIVERNV(thegames)      //S80a-691: 08/84 The Games
DRIVERNV(eldorado)      //S80a-692: 09/84 El Dorado City of Gold
DRIVERNV(touchdn)       //S80a-688: 10/84 Touchdown
DRIVERNV(icefever)      //S80a-695: 02/85 Ice Fever
//System 80b
DRIVERNV(triplay)       //S80b-696: 05/85 Chicago Cubs Triple Play
DRIVERNV(bountyh)       //S80b-694: 07/85 Bounty Hunter
DRIVERNV(tagteam)       //S80b-698: 09/85 Tag Team Wrestling
DRIVERNV(tagteam2)      //                Tag Team Wrestling (rev.2)
DRIVERNV(rock)          //S80b-697: 10/85 Rock
                        //S80b-700: ??/85 Ace High (never produced, playable whitewood exists)
DRIVERNV(raven)         //S80b-702: 03/86 Raven
DRIVERNV(rock_enc)      //S80b-704: 04/86 Rock Encore
DRIVERNV(hlywoodh)      //S80b-703: 06/86 Hollywood Heat
DRIVERNV(genesis)       //S80b-705: 09/86 Genesis
DRIVERNV(goldwing)      //S80b-707: 10/86 Gold Wings
DRIVERNV(mntecrlo)      //S80b-708: 02/87 Monte Carlo
DRIVERNV(sprbreak)      //S80b-706: 04/87 Spring Break
DRIVERNV(sprbrks)       //                Spring Break (single ball game)
                        //S80b-???: 05/87 Amazon Hunt II
DRIVERNV(arena)         //S80b-709: 06/87 Arena
DRIVERNV(victory)       //S80b-710: 10/87 Victory
DRIVERNV(diamond)       //S80b-711: 02/88 Diamond Lady
DRIVERNV(txsector)      //S80b-712: 03/88 TX Sector
DRIVERNV(robowars)      //S80b-714: 04/88 Robo-War
DRIVERNV(badgirls)      //S80b-717: 11/88 Bad Girls
DRIVERNV(excalibr)      //S80b-715: 11/88 Excalibur
DRIVERNV(bighouse)      //S80b-713: 04/89 Big House
DRIVERNV(hotshots)      //S80b-718: 04/89 Hot Shots
DRIVERNV(bonebstr)      //S80b-719: 08/89 Bone Busters Inc
DRIVERNV(nmoves)        //C-103:    11/89 Night Moves (for International Concepts)
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
                        //??-???:   ??/91 Amazon Hunt III
DRIVERNV(carhop)        //S3-725:   01/91 Car Hop
DRIVERNV(hoops)         //S3-727:   02/91 Hoops
DRIVERNV(cactjack)      //S3-729:   04/91 Cactus Jack's
DRIVERNV(clas1812)      //S3-730:   08/91 Class of 1812
DRIVERNV(surfnsaf)      //S3-731:   11/91 Surf'n Safari
DRIVERNV(opthund)       //S3-732:   02/92 Operation: Thunder
//System 3 128x32 DMD
DRIVERNV(smb)           //S3-733:   04/92 Super Mario Bros.
DRIVERNV(smb1)          //                Super Mario Bros. (rev.1)
DRIVERNV(smb2)          //                Super Mario Bros. (rev.2)
DRIVERNV(smb3)          //                Super Mario Bros. (rev.3)
DRIVERNV(smbmush)       //S3-N105:  06/92 Super Mario Bros. Mushroom World
DRIVERNV(cueball)       //S3-734:   10/92 Cue Ball Wizard
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
DRIVERNV(bighurt)       //S3-743:   06/95 Big Hurt (rev.3)
DRIVERNV(snspares)      //S3-N111:  10/95 Strikes 'N Spares (rev.6)
DRIVERNV(snspare1)      //                Strikes 'N Spares (rev.1)
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

// ----------------
// HANKIN GAMES
// ----------------
DRIVERNV(fjholden)      //FJ Holden
DRIVERNV(orbit1)        //Orbit 1
DRIVERNV(howzat)        //Howzat
DRIVERNV(shark)         //Shark
DRIVERNV(empsback)      //Star Wars - The Empire Strike Back

// ----------------
// INDER GAMES
// ----------------
DRIVERNV(brvteam )      //Brave Team (1985)
DRIVERNV(canasta )      //Canasta '86' (1986)
DRIVERNV(lapbylap)      //Lap By Lap (1986)
DRIVERNV(pinclown)      //Pinball Clown (1988)
DRIVERNV(corsario)      //Corsario (1989)
DRIVERNV(atleta  )      //Atleta (1991)
DRIVERNV(ind250cc)      //250 CC (1992)

// ----------------
// JAC VAN HAM
// ----------------
DRIVERNV(escape)        //Escape (10/1987)
DRIVERNV(movmastr)      //Movie Masters (??/19??)

// ----------------
// JEUTEL OF FRANCE
// ----------------
DRIVERNV(leking)        //Le King (??/1983)
DRIVERNV(olympic)       //Olympic Games (??/1984)

// ----------------
// JUEGOS POPULARES
// ----------------
DRIVERNV(petaco  )      //Petaco
DRIVERNV(faeton  )      //Faeton
DRIVERNV(petaco2 )      //Petaco 2
DRIVERNV(olympus )      //Olympus
DRIVERNV(america )      //America 1492
DRIVERNV(aqualand)      //Aqualand
DRIVERNV(lortium )      //Lortium

// ----------------
// LTD
// ----------------
// Sistema III
                        //O Gaucho
                        //Samba
                        //Amazon
                        //Arizona
DRIVERNV(atla_ltd)      //Atlantis
                        //Galaxia
                        //Grand Prix
                        //Hustler
                        //Martian Queen
DRIVERNV(bhol_ltd)      //Black Hole
DRIVERNV(zephy   )      //Zephy (clone of Bally Xenon)
// Sistema IV
DRIVERNV(alcapone)      //Al Capone
                        //Alien Warrior
                        //Carnaval no Rio
DRIVERNV(columbia)      //Columbia
DRIVERNV(cowboy  )      //Cowboy Eight Ball (clone of Bally Eight Ball Deluxe)
                        //Cowboy 2
                        //Disco Dancing
                        //Force
                        //Haunted Hotel
                        //King Kong
                        //Kung Fu
DRIVERNV(pecmen  )      //Mr. & Mrs. Pec-Men (clone of Bally's... guess the game! :))
                        //Space Poker
                        //Time Machine
                        //Trick Shooter
                        //Viking King

// ----------------
// MICROPIN GAMES
// ----------------
DRIVERNV(pentacup)      //Pentacup (rev. 1, 1978)
DRIVERNV(pentacp2)      //Pentacup (rev. 2, 1980)

// ----------------
// MIDWAY GAMES
// ----------------
DRIVERNV(flicker )      //Flicker (Prototype, September 1974)
DRIVERNV(rotation)      //Rotation VIII (September 1978)

// ----------------
// MONROE BOWLING CO.
// ----------------
DRIVERNV(monrobwl)      //Stars & Strikes (1979?)

// ----------------
// MR. GAME
// ----------------
DRIVERNV(dakar)     //Dakar (1988?)
DRIVERNV(motrshow)    //Motor Show (1988?)
DRIVERNV(motrshwa)    //Motor Show (alternate set)
DRIVERNV(macattck)    //Mac Attack (1989?)
DRIVERNV(wcup90)    //World Cup 90 (1990)

// ----------------
// NSM GAMES
// ----------------
                        //Cosmic Flash (10/1985)
DRIVERNV(firebird)      //Hot Fire Birds (12/1985)

// ----------------
// PEYPER
// ----------------
                        // Tally Hoo
                        // Fantastic World (1985)
                        // Odin (1985)
                        // Nemesis (1986)
                        // Wolfman (1987)
DRIVERNV(wolfman)       // Wolf Man (1987)
DRIVERNV(odisea)        // Odisea Paris-Dakar (1987)
                        // Sir Lancelot (1994)

// ----------------
// PLAYMATIC
// ----------------
DRIVERNV(spcgambl) // 03/78 Space Gambler
DRIVERNV(bigtown)  // 04/78 Big Town
DRIVERNV(lastlap)  // 09/78 Last Lap
DRIVERNV(chance)   // 09/78 Chance
DRIVERNV(party)    // 05/79 Party
DRIVERNV(antar)    // 11/79 Antar
DRIVERNV(antar2)   // 11/79 Antar (alternate set)
DRIVERNV(evlfight) // 03/80 Evil Fight
DRIVERNV(attack)   // 10/80 Attack
DRIVERNV(blkfever) // 12/80 Black Fever
                   // ??/80 Zira
DRIVERNV(cerberus) // 03/82 Cerberus
DRIVERNV(spain82)  // 10/82 Spain 82
DRIVERNV(madrace)  // ??/8? Mad Race
DRIVERNV(megaaton) // 04/84 Meg-Aaton
                   // ??/84 Nautilus
                   // ??/84 The Raid
DRIVERNV(ufo_x)    // 11/84 UFO-X
DRIVERNV(kz26)     // ??/85 KZ-26
DRIVERNV(rock2500) // ??/85 Rock 2500
DRIVERNV(starfire) // ??/85 Star Fire
DRIVERNV(trailer)  // ??/85 Trailer
                   // ??/85 Stop Ship
                   // ??/86 Flash Dragon
                   // ??/87 Phantom Ship
                   // ??/87 Skill Flight

// ----------------
// ROWAMET
// ----------------
DRIVERNV(heavymtl)    //Heavy Metal (198?)

// --------------
// SEGA GAMES
// --------------
//Data East Hardware, DMD 192x64
DRIVERNV(mav_100)       //DE/Sega MPU: 09/94 Maverick 1.00
DRIVERNV(mav_400)       //DE/Sega MPU: 09/94 Maverick 4.00
DRIVERNV(mav_401)       //DE/Sega MPU: 09/94 Maverick 4.01 Display
DRIVERNV(mav_402)       //DE/Sega MPU: 09/94 Maverick 4.02 Display
DRIVERNV(frankst)       //DE/Sega MPU: 12/94 Frankenstein
DRIVERNV(frankstg)      //DE/Sega MPU: 01/95 Frankenstein (Germany)
DRIVERNV(baywatch)      //DE/Sega MPU: 03/95 Baywatch
DRIVERNV(batmanf)       //DE/Sega MPU: 07/95 Batman Forever (4.0)
DRIVERNV(batmanf3)      //DE/Sega MPU: 07/95 Batman Forever (3.0)
DRIVERNV(bmf_uk)        //DE/Sega MPU: 07/95 Batman Forever (UK)
DRIVERNV(bmf_at)        //DE/Sega MPU: 07/95 Batman Forever (Austria)
DRIVERNV(bmf_be)        //DE/Sega MPU: 07/95 Batman Forever (Belgium)
DRIVERNV(bmf_ch)        //DE/Sega MPU: 07/95 Batman Forever (Switzerland)
DRIVERNV(bmf_cn)        //DE/Sega MPU: 07/95 Batman Forever (Canada)
DRIVERNV(bmf_de)        //DE/Sega MPU: 07/95 Batman Forever (Germany)
DRIVERNV(bmf_fr)        //DE/Sega MPU: 07/95 Batman Forever (France)
DRIVERNV(bmf_nl)        //DE/Sega MPU: 07/95 Batman Forever (Holland)
DRIVERNV(bmf_it)        //DE/Sega MPU: 07/95 Batman Forever (Italy)
DRIVERNV(bmf_sp)        //DE/Sega MPU: 07/95 Batman Forever (Spain)
DRIVERNV(bmf_no)        //DE/Sega MPU: 07/95 Batman Forever (Norway)
DRIVERNV(bmf_sv)        //DE/Sega MPU: 07/95 Batman Forever (Sweden)
DRIVERNV(bmf_jp)        //DE/Sega MPU: 07/95 Batman Forever (Japan)
DRIVERNV(bmf_time)      //DE/Sega MPU: 07/95 Batman Forever (Timed Version)
DRIVERNV(ctcheese)      //DE/Sega MPU: ??/96 Cut The Cheese (Redemption)
//Whitestar Hardware, DMD 128x32
DRIVERNV(apollo13)      //Whitestar: 11/95 Apollo 13
DRIVERNV(gldneye)       //Whitestar: 02/96 Golden Eye
DRIVER  (twst,300)      //Whitestar: 05/96 Twister (3.00)
DRIVER  (twst,404)      //Whitestar: 05/96 Twister (4.04)
DRIVER  (twst,405)      //Whitestar: 05/96 Twister (4.05)
DRIVERNV(id4)           //Whitestar: 07/96 ID4: Independance Day
DRIVERNV(spacejam)      //Whitestar: 08/96 Space Jam
DRIVERNV(spacejmf)      //Whitestar: 08/96 Space Jam (France)
DRIVERNV(spacejmg)      //Whitestar: 08/96 Space Jam (Germany)
DRIVERNV(spacejmi)      //Whitestar: 08/96 Space Jam (Italy)
DRIVERNV(swtril43)      //Whitestar: 02/97 Star Wars Trilogy (4.03)
DRIVERNV(swtril41)      //Whitestar: 02/97 Star Wars Trilogy (4.01)
DRIVERNV(jplstw22)      //Whitestar: 06/97 The Lost World: Jurassic Park (2.02)
DRIVERNV(jplstw20)      //Whitestar: 06/97 The Lost World: Jurassic Park (2.00)
DRIVERNV(xfiles2)       //Whitestar: 08/97 X-Files (2.04)
DRIVERNV(xfiles)        //Whitestar: 08/97 X-Files (3.03)
DRIVERNV(startrp)       //Whitestar: 11/97 Starship Troopers
DRIVERNV(ctchzdlx)      //Whitestar: ??/98 Cut The Cheese Deluxe (Redemption)
DRIVERNV(wackadoo)      //Whitestar: ??/98 Wack-A-Doodle-Doo (Redemption)
DRIVERNV(titanic)       //Whitestar: ??/98 Titanic Redemption (Coin dropper)
DRIVERNV(viprsega)      //Whitestar: 02/98 Viper Night Drivin'
DRIVERNV(lostspc)       //Whitestar: 06/98 Lost in Space
DRIVERNV(goldcue)       //Whitestar: 06/98 Golden Cue
DRIVERNV(godzilla)      //Whitestar: 09/98 Godzilla
DRIVER  (sprk,090)      //Whitestar: 01/99 South Park (0.90)
DRIVER  (sprk,096)      //Whitestar: 01/99 South Park (0.96)
DRIVER  (sprk,103)      //Whitestar: 01/99 South Park (1.03)
DRIVER  (harl,a10)      //Whitestar: 09/99 Harley Davidson (1.03, Display 1.00)
DRIVER  (harl,a13)      //Whitestar: 10/99 Harley Davidson (1.03, Display 1.04)
DRIVER  (harl,f13)      //Whitestar: 10/99 Harley Davidson (1.03 France)
DRIVER  (harl,g13)      //Whitestar: 10/99 Harley Davidson (1.03 Germany)
DRIVER  (harl,i13)      //Whitestar: 10/99 Harley Davidson (1.03 Italy)
DRIVER  (harl,l13)      //Whitestar: 10/99 Harley Davidson (1.03 Spain)

// ----------------
// SLEIC
// ----------------
                        // Bike Race (1992)
DRIVERNV(sleicpin)      // Pin-Ball (1993)
                        // Io Moon (1994)
                        // Dona Elvira 2 (1996)

// ----------------
// SONIC
// ----------------
            // Night Fever (1979)
            // Storm (1979)
DRIVERNV(odin_dlx)    // Odin De Luxe (1985)
            // Gamatron (1986)
DRIVERNV(solarwar)    // Solar Wars (1986)
DRIVERNV(sonstwar)    // Star Wars (1987)
DRIVERNV(poleposn)    // Pole Position (1987)
            // Hang-On (1988)

// ---------------
// SPINBALL GAMES
// ---------------
DRIVERNV(bushido)   //1993 - Bushido     ( Last game by Inder - before becomming Spinball - but same hardware)
DRIVERNV(bushidoa)    //1993 - Bushido (alternate set)
DRIVERNV(mach2)     //1995 - Mach 2
DRIVERNV(jolypark)      //1996 - Jolly Park
DRIVERNV(vrnwrld)   //1996 - Verne's World

// ---------------
// SPLIN BINGO
// ---------------
DRIVERNV(goldgame)      //19?? - Golden Game
DRIVERNV(goldgam2)      //19?? - Golden Game Stake 6/10

// ---------------
// STARGAME GAMES
// ---------------
DRIVERNV(mephist1)      //1986 - Mephisto (rev. 1.1)
DRIVERNV(mephisto)      //1986 - Mephisto (rev. 1.2)

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
DRIVERNV(tridenfp)      //MPU-100: 03/79 Trident (Free Play)
DRIVERNV(hothand)       //MPU-100: 06/79 Hot Hand
DRIVERNV(hothanfp)      //MPU-100: 06/79 Hot Hand (Free Play)
DRIVERNV(magic)         //MPU-100: 08/79 Magic
DRIVERNV(magicfp)       //MPU-100: 08/79 Magic (Free Play)
DRIVERNV(princess)      //MPU-100: 08/79 Cosmic Princess
DRIVERNV(princefp)      //MPU-100: 08/79 Cosmic Princess (Free Play)
// MPU-200 - Sound Board: SB-300
DRIVERNV(meteor)        //MPU-200: 09/79 Meteor
DRIVERNV(meteorfp)      //MPU-200: 09/79 Meteor (Free Play)
DRIVERNV(meteorb)       //MPU-200: 11/03 Meteor (7-Digit conversion)
DRIVERNV(meteorc)       //MPU-200: 11/03 Meteor (7-Digit conversion Free Play)
DRIVERNV(meteord)       //MPU-200: 05/05 Meteor (/10 Scoring)
DRIVERNV(galaxy)        //MPU-200: 01/80 Galaxy
DRIVERNV(galaxyfp)      //MPU-200: 01/80 Galaxy (Free Play)
DRIVERNV(galaxyb)       //MPU-200: ??/04 Galaxy (7-Digit conversion)
DRIVERNV(ali)           //MPU-200: 03/80 Ali
DRIVERNV(alifp)         //MPU-200: 03/80 Ali (Free Play)
DRIVERNV(biggame)       //MPU-200: 03/80 Big Game
DRIVERNV(biggamfp)      //MPU-200: 03/80 Big Game (Free Play)
DRIVERNV(seawitch)      //MPU-200: 05/80 Seawitch
DRIVERNV(seawitfp)      //MPU-200: 05/80 Seawitch (Free Play)
DRIVERNV(cheetah)       //MPU-200: 06/80 Cheetah
DRIVERNV(cheetafp)      //MPU-200: 06/80 Cheetah (Free Play)
DRIVERNV(cheetah1)      //MPU-200: 06/80 Cheetah (Bonus shot 1/game)
DRIVERNV(cheetah2)      //MPU-200: 06/80 Cheetah (Bonus shot 1/ball)
DRIVERNV(quicksil)      //MPU-200: 06/80 Quicksilver
DRIVERNV(quicksfp)      //MPU-200: 06/80 Quicksilver (Free Play)
DRIVERNV(stargzr)       //MPU-200: 08/80 Star Gazer
DRIVERNV(stargzfp)      //MPU-200: 08/80 Star Gazer (Free Play)
DRIVERNV(stargzrb)      //MPU-200: 03/06 Star Gazer (modified rules rev.9)
DRIVERNV(flight2k)      //MPU-200: 10/80 Flight 2000
DRIVERNV(flightfp)      //MPU-200: 10/80 Flight 2000 (Free Play)
DRIVERNV(nineball)      //MPU-200: 12/80 Nine Ball
DRIVERNV(ninebafp)      //MPU-200: 12/80 Nine Ball (Free Play)
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
DRIVERNV(dragfisb)      //MPU-200: 01/82 Dragonfist (Bootleg - modified to match instruction card/manual)
DRIVERNV(orbitor1)      //MPU-200: 04/82 Orbitor One
DRIVERNV(orbitofp)      //MPU-200: 04/82 Orbitor One (Free Play)
DRIVERNV(orbitora)      //MPU-200: 04/82 Orbitor One (Bootleg)
DRIVERNV(orbitorb)      //MPU-200: 04/82 Orbitor One (Bootleg Free Play)
DRIVERNV(cue)           //MPU-200: ??/82 Cue            (Proto - Never released)
DRIVERNV(blbeauty)      //MPU-200: 09/84 Black Beauty (Shuffle)
DRIVERNV(lazrlord)      //MPU-200: 10/84 Lazer Lord     (Proto - Never released)
// Whitestar System
DRIVERNV(strikext)      //Whitestar: 03/00 Striker Extreme
DRIVERNV(strxt_uk)      //Whitestar: 03/00 Striker Extreme (UK)
DRIVERNV(strxt_gr)      //Whitestar: 03/00 Striker Extreme (Germany)
DRIVERNV(strxt_sp)      //Whitestar: 03/00 Striker Extreme (Spain)
DRIVERNV(strxt_fr)      //Whitestar: 03/00 Striker Extreme (France)
DRIVERNV(strxt_it)      //Whitestar: 03/00 Striker Extreme (Italy)
DRIVERNV(shrkysht)      //Whitestar: 09/00 Sharkey's Shootout (2.11)
DRIVERNV(shrky_gr)      //Whitestar: 09/00 Sharkey's Shootout (2.11 Germany)
DRIVERNV(shrky_fr)      //Whitestar: 09/00 Sharkey's Shootout (2.11 France)
DRIVERNV(shrky_it)      //Whitestar: 09/00 Sharkey's Shootout (2.11 Italy)
DRIVERNV(shrky207)      //Whitestar: 09/00 Sharkey's Shootout (2.07)
DRIVERNV(hirolcas)      //Whitestar: 01/01 High Roller Casino (3.00)
DRIVERNV(hirol_g3)      //Whitestar: 01/01 High Roller Casino (3.00 Germany)
DRIVERNV(hirol_fr)      //Whitestar: 01/01 High Roller Casino (3.00 France)
DRIVERNV(hirol_it)      //Whitestar: 01/01 High Roller Casino (3.00 Italy)
DRIVERNV(hirolcat)      //Whitestar: 01/01 High Roller Casino (2.90 TEST BUILD 1820)
DRIVERNV(hirol210)      //Whitestar: 01/01 High Roller Casino (2.10)
DRIVERNV(hirol_gr)      //Whitestar: 01/01 High Roller Casino (2.10 Germany)
DRIVERNV(austin)        //Whitestar: 05/01 Austin Powers (3.02)
DRIVERNV(austing)       //Whitestar: 05/01 Austin Powers (3.02 German display)
DRIVERNV(austinf)       //Whitestar: 05/01 Austin Powers (3.02 French display)
DRIVERNV(austini)       //Whitestar: 05/01 Austin Powers (3.02 Italian display)
DRIVERNV(aust301)       //Whitestar: 05/01 Austin Powers (3.01)
DRIVERNV(aust300)       //Whitestar: 05/01 Austin Powers (3.00)
DRIVERNV(aust201)       //Whitestar: 05/01 Austin Powers (2.01)
DRIVERNV(monopoly)      //Whitestar: 09/01 Monopoly (3.20)
DRIVERNV(monopolg)      //Whitestar: 09/01 Monopoly (3.20 German display)
DRIVERNV(monopoll)      //Whitestar: 09/01 Monopoly (3.20 Spanish display)
DRIVERNV(monopolf)      //Whitestar: 09/01 Monopoly (3.20 French display)
DRIVERNV(monopoli)      //Whitestar: 09/01 Monopoly (3.20 Italian display)
DRIVERNV(monopole)      //Whitestar: 09/01 Monopoly (3.03)
DRIVERNV(monop301)      //Whitestar: 09/01 Monopoly (3.01)
DRIVERNV(monop251)      //Whitestar: 09/01 Monopoly (2.51)
DRIVERNV(monop233)      //Whitestar: 09/01 Monopoly (2.33)
DRIVERNV(nfl)           //Whitestar: 11/01 NFL
DRIVERNV(monopred)      //Whitestar: ??/02 Monopoly Redemption (Coin dropper)
DRIVERNV(playboys)      //Whitestar: 02/02 Playboy (5.00)
DRIVERNV(playboyg)      //Whitestar: 02/02 Playboy (5.00 Germany)
DRIVERNV(playboyl)      //Whitestar: 02/02 Playboy (5.00 Spain)
DRIVERNV(playboyf)      //Whitestar: 02/02 Playboy (5.00 France)
DRIVERNV(playboyi)      //Whitestar: 02/02 Playboy (5.00 Italy)
DRIVERNV(play401)       //Whitestar: 02/02 Playboy (4.01)
DRIVERNV(play401g)      //Whitestar: 02/02 Playboy (4.01 Germany)
DRIVERNV(play401l)      //Whitestar: 02/02 Playboy (4.01 Spain)
DRIVERNV(play401f)      //Whitestar: 02/02 Playboy (4.01 France)
DRIVERNV(play401i)      //Whitestar: 02/02 Playboy (4.01 Italy)
DRIVERNV(play303)       //Whitestar: 02/02 Playboy (3.03)
DRIVERNV(play303g)      //Whitestar: 02/02 Playboy (3.03 Germany)
DRIVERNV(play303l)      //Whitestar: 02/02 Playboy (3.03 Spain)
DRIVERNV(play303f)      //Whitestar: 02/02 Playboy (3.03 France)
DRIVERNV(play303i)      //Whitestar: 02/02 Playboy (3.03 Italy)
DRIVERNV(play300)       //Whitestar: 02/02 Playboy (3.00)
DRIVERNV(play203)       //Whitestar: 02/02 Playboy (2.03)
DRIVERNV(play203g)      //Whitestar: 02/02 Playboy (2.03 Germany)
DRIVERNV(play203l)      //Whitestar: 02/02 Playboy (2.03 Spain)
DRIVERNV(play203f)      //Whitestar: 02/02 Playboy (2.03 France)
DRIVERNV(play203i)      //Whitestar: 02/02 Playboy (2.03 Italy)
DRIVERNV(rctycn)        //Whitestar: 09/02 Roller Coaster Tycoon (7.02)
DRIVERNV(rctycng)       //Whitestar: 09/02 Roller Coaster Tycoon (7.02 Germany)
DRIVERNV(rctycnl)       //Whitestar: 09/02 Roller Coaster Tycoon (7.02 Spain)
DRIVERNV(rctycnf)       //Whitestar: 09/02 Roller Coaster Tycoon (7.02 France)
DRIVERNV(rctycni)       //Whitestar: 09/02 Roller Coaster Tycoon (7.02 Italy)
DRIVERNV(rct701)        //Whitestar: 09/02 Roller Coaster Tycoon (7.01)
DRIVERNV(rct701g)       //Whitestar: 09/02 Roller Coaster Tycoon (7.01 Germany)
DRIVERNV(rct701l)       //Whitestar: 09/02 Roller Coaster Tycoon (7.01 Spain)
DRIVERNV(rct701f)       //Whitestar: 09/02 Roller Coaster Tycoon (7.01 France)
DRIVERNV(rct701i)       //Whitestar: 09/02 Roller Coaster Tycoon (7.01 Italy)
DRIVERNV(rct600)        //Whitestar: 09/02 Roller Coaster Tycoon (6.00)
DRIVERNV(rct600l)       //Whitestar: 09/02 Roller Coaster Tycoon (6.00 Spain)
DRIVERNV(rct600f)       //Whitestar: 09/02 Roller Coaster Tycoon (6.00 France)
DRIVERNV(rct600i)       //Whitestar: 09/02 Roller Coaster Tycoon (6.00 Italy)
DRIVERNV(rct400)        //Whitestar: 09/02 Roller Coaster Tycoon (4.00)
DRIVERNV(rct400l)       //Whitestar: 09/02 Roller Coaster Tycoon (4.00 Spain)
DRIVERNV(rct400f)       //Whitestar: 09/02 Roller Coaster Tycoon (4.00 France)
DRIVERNV(rct400g)       //Whitestar: 09/02 Roller Coaster Tycoon (4.00 Germany)
DRIVERNV(rct400i)       //Whitestar: 09/02 Roller Coaster Tycoon (4.00 Italy)
DRIVERNV(simpprty)      //Whitestar: 01/03 The Simpsons Pinball Party (5.00)
DRIVERNV(simpprtg)      //Whitestar: 01/03 The Simpsons Pinball Party (5.00 Germany)
DRIVERNV(simpprtl)      //Whitestar: 01/03 The Simpsons Pinball Party (5.00 Spain)
DRIVERNV(simpprtf)      //Whitestar: 01/03 The Simpsons Pinball Party (5.00 France)
DRIVERNV(simpprti)      //Whitestar: 01/03 The Simpsons Pinball Party (5.00 Italy)
DRIVERNV(simp400)       //Whitestar: 01/03 The Simpsons Pinball Party (4.00)
DRIVERNV(simp400g)      //Whitestar: 01/03 The Simpsons Pinball Party (4.00 Germany)
DRIVERNV(simp400l)      //Whitestar: 01/03 The Simpsons Pinball Party (4.00 Spain)
DRIVERNV(simp400f)      //Whitestar: 01/03 The Simpsons Pinball Party (4.00 France)
DRIVERNV(simp400i)      //Whitestar: 01/03 The Simpsons Pinball Party (4.00 Italy)
DRIVERNV(simp204)       //Whitestar: 01/03 The Simpsons Pinball Party (2.04)
DRIVERNV(simp204l)      //Whitestar: 01/03 The Simpsons Pinball Party (2.04 Spain)
DRIVERNV(simp204f)      //Whitestar: 01/03 The Simpsons Pinball Party (2.04 France)
DRIVERNV(simp204i)      //Whitestar: 01/03 The Simpsons Pinball Party (2.04 Italy)
DRIVERNV(term3)         //Whitestar: 06/03 Terminator 3: Rise of the Machines (4.00)
DRIVERNV(term3g)        //Whitestar: 06/03 Terminator 3: Rise of the Machines (4.00 Germany)
DRIVERNV(term3l)        //Whitestar: 06/03 Terminator 3: Rise of the Machines (4.00 Spain)
DRIVERNV(term3f)        //Whitestar: 06/03 Terminator 3: Rise of the Machines (4.00 France)
DRIVERNV(term3i)        //Whitestar: 06/03 Terminator 3: Rise of the Machines (4.00 Italy)
DRIVERNV(term3_2)       //Whitestar: 06/03 Terminator 3: Rise of the Machines (2.05)
DRIVERNV(term3l_2)      //Whitestar: 06/03 Terminator 3: Rise of the Machines (2.05 Spain)
DRIVERNV(term3f_2)      //Whitestar: 06/03 Terminator 3: Rise of the Machines (2.05 France)
DRIVERNV(term3i_2)      //Whitestar: 06/03 Terminator 3: Rise of the Machines (2.05 Italy)
DRIVER  (harl,a18)      //Whitestar: 07/03 Harley Davidson (1.08)
DRIVER  (harl,f18)      //Whitestar: 07/03 Harley Davidson (1.08 France)
DRIVER  (harl,g18)      //Whitestar: 07/03 Harley Davidson (1.08 Germany)
DRIVER  (harl,i18)      //Whitestar: 07/03 Harley Davidson (1.08 Italy)
DRIVER  (harl,l18)      //Whitestar: 07/03 Harley Davidson (1.08 Spain)
// New CPU/Sound Board with ARM7 CPU + Xilinx FPGA controlling sound
DRIVERNV(lotr_le)       //Whitestar: ??/08 Lord Of The Rings (10.02 Limited Edition)
DRIVERNV(lotr)          //Whitestar: 10/05 Lord Of The Rings (10.00)
DRIVERNV(lotr_gr)       //Whitestar: 10/05 Lord Of The Rings (10.00 Germany)
DRIVERNV(lotr_sp)       //Whitestar: 10/05 Lord Of The Rings (10.00 Spain)
DRIVERNV(lotr_fr)       //Whitestar: 10/05 Lord Of The Rings (10.00 France)
DRIVERNV(lotr_it)       //Whitestar: 10/05 Lord Of The Rings (10.00 Italy)
DRIVERNV(lotr9)         //Whitestar: 01/05 Lord Of The Rings (9.00)
DRIVERNV(lotr_gr9)      //Whitestar: 01/05 Lord Of The Rings (9.00 Germany)
DRIVERNV(lotr_sp9)      //Whitestar: 01/05 Lord Of The Rings (9.00 Spain)
DRIVERNV(lotr_fr9)      //Whitestar: 01/05 Lord Of The Rings (9.00 France)
DRIVERNV(lotr_it9)      //Whitestar: 01/05 Lord Of The Rings (9.00 Italy)
DRIVERNV(lotr8)         //Whitestar: 07/04 Lord Of The Rings (8.00)
DRIVERNV(lotr_gr8)      //Whitestar: 07/04 Lord Of The Rings (8.00 Germany)
DRIVERNV(lotr_sp8)      //Whitestar: 07/04 Lord Of The Rings (8.00 Spain)
DRIVERNV(lotr_fr8)      //Whitestar: 07/04 Lord Of The Rings (8.00 France)
DRIVERNV(lotr_it8)      //Whitestar: 07/04 Lord Of The Rings (8.00 Italy)
DRIVERNV(lotr7)         //Whitestar: 04/04 Lord Of The Rings (7.00)
DRIVERNV(lotr_gr7)      //Whitestar: 04/04 Lord Of The Rings (7.00 Germany)
DRIVERNV(lotr_sp7)      //Whitestar: 04/04 Lord Of The Rings (7.00 Spain)
DRIVERNV(lotr_fr7)      //Whitestar: 04/04 Lord Of The Rings (7.00 France)
DRIVERNV(lotr_it7)      //Whitestar: 04/04 Lord Of The Rings (7.00 Italy)
DRIVERNV(lotr4)         //Whitestar: 11/03 Lord Of The Rings (4.01)
DRIVERNV(lotr_gr4)      //Whitestar: 11/03 Lord Of The Rings (4.01 Germany)
DRIVERNV(lotr_sp4)      //Whitestar: 11/03 Lord Of The Rings (4.01 Spain)
DRIVERNV(lotr_fr4)      //Whitestar: 11/03 Lord Of The Rings (4.01 France)
DRIVERNV(lotr_it4)      //Whitestar: 11/03 Lord Of The Rings (4.01 Italy)
DRIVERNV(ripleys)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.20)
DRIVERNV(ripleysg)      //Whitestar: 03/04 Ripley's Believe It or Not! (3.20 Germany)
DRIVERNV(ripleysl)      //Whitestar: 03/04 Ripley's Believe It or Not! (3.20 Spain)
DRIVERNV(ripleysf)      //Whitestar: 03/04 Ripley's Believe It or Not! (3.20 France)
DRIVERNV(ripleysi)      //Whitestar: 03/04 Ripley's Believe It or Not! (3.20 Italy)
DRIVERNV(rip310)        //Whitestar: 03/04 Ripley's Believe It or Not! (3.10)
DRIVERNV(rip310g)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.10 Germany)
DRIVERNV(rip310l)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.10 Spain)
DRIVERNV(rip310f)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.10 France)
DRIVERNV(rip310i)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.10 Italy)
DRIVERNV(rip302)        //Whitestar: 03/04 Ripley's Believe It or Not! (3.02)
DRIVERNV(rip302g)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.02 Germany)
DRIVERNV(rip302l)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.02 Spain)
DRIVERNV(rip302f)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.02 France)
DRIVERNV(rip302i)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.02 Italy)
DRIVERNV(rip300)        //Whitestar: 03/04 Ripley's Believe It or Not! (3.00)
DRIVERNV(rip300g)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.00 Germany)
DRIVERNV(rip300l)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.00 Spain)
DRIVERNV(rip300f)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.00 France)
DRIVERNV(rip300i)       //Whitestar: 03/04 Ripley's Believe It or Not! (3.00 Italy)
DRIVERNV(elvis)         //Whitestar: 08/04 Elvis (5.00)
DRIVERNV(elvisg)        //Whitestar: 08/04 Elvis (5.00 Germany)
DRIVERNV(elvisl)        //Whitestar: 08/04 Elvis (5.00 Spain)
DRIVERNV(elvisf)        //Whitestar: 08/04 Elvis (5.00 France)
DRIVERNV(elvisi)        //Whitestar: 08/04 Elvis (5.00 Italy)
DRIVERNV(elv400)        //Whitestar: 08/04 Elvis (4.00)
DRIVERNV(elv400g)       //Whitestar: 08/04 Elvis (4.00 Germany)
DRIVERNV(elv400l)       //Whitestar: 08/04 Elvis (4.00 Spain)
DRIVERNV(elv400f)       //Whitestar: 08/04 Elvis (4.00 France)
DRIVERNV(elv400i)       //Whitestar: 08/04 Elvis (4.00 Italy)
DRIVERNV(elv303)        //Whitestar: 08/04 Elvis (3.03)
DRIVERNV(elv303g)       //Whitestar: 08/04 Elvis (3.03 Germany)
DRIVERNV(elv303l)       //Whitestar: 08/04 Elvis (3.03 Spain)
DRIVERNV(elv303f)       //Whitestar: 08/04 Elvis (3.03 France)
DRIVERNV(elv303i)       //Whitestar: 08/04 Elvis (3.03 Italy)
DRIVERNV(elv302)        //Whitestar: 08/04 Elvis (3.02)
DRIVERNV(elv302g)       //Whitestar: 08/04 Elvis (3.02 Germany)
DRIVERNV(elv302l)       //Whitestar: 08/04 Elvis (3.02 Spain)
DRIVERNV(elv302f)       //Whitestar: 08/04 Elvis (3.02 France)
DRIVERNV(elv302i)       //Whitestar: 08/04 Elvis (3.02 Italy)
DRIVER  (harl,a40)      //Whitestar: 10/04 Harley Davidson (4.00)
DRIVER  (harl,f40)      //Whitestar: 10/04 Harley Davidson (4.00 France)
DRIVER  (harl,g40)      //Whitestar: 10/04 Harley Davidson (4.00 Germany)
DRIVER  (harl,i40)      //Whitestar: 10/04 Harley Davidson (4.00 Italy)
DRIVER  (harl,l40)      //Whitestar: 10/04 Harley Davidson (4.00 Spain)
DRIVER  (harl,a30)      //Whitestar: 10/04 Harley Davidson (3.00)
DRIVER  (harl,f30)      //Whitestar: 10/04 Harley Davidson (3.00 France)
DRIVER  (harl,g30)      //Whitestar: 10/04 Harley Davidson (3.00 Germany)
DRIVER  (harl,i30)      //Whitestar: 10/04 Harley Davidson (3.00 Italy)
DRIVER  (harl,l30)      //Whitestar: 10/04 Harley Davidson (3.00 Spain)
DRIVERNV(sopranos)      //Whitestar: 02/05 The Sopranos (5.00)
DRIVERNV(sopranog)      //Whitestar: 02/05 The Sopranos (5.00 Germany)
DRIVERNV(sopranol)      //Whitestar: 02/05 The Sopranos (5.00 Spain)
DRIVERNV(sopranof)      //Whitestar: 02/05 The Sopranos (5.00 France)
DRIVERNV(sopranoi)      //Whitestar: 02/05 The Sopranos (5.00 Italy)
DRIVERNV(sopr300)       //Whitestar: 02/05 The Sopranos (3.00)
DRIVERNV(sopr300g)      //Whitestar: 02/05 The Sopranos (3.00 Germany)
DRIVERNV(sopr300l)      //Whitestar: 02/05 The Sopranos (3.00 Spain)
DRIVERNV(sopr300f)      //Whitestar: 02/05 The Sopranos (3.00 France)
DRIVERNV(sopr300i)      //Whitestar: 02/05 The Sopranos (3.00 Italy)
DRIVERNV(sopr204)       //Whitestar: 02/05 The Sopranos (2.04)
DRIVERNV(sopr107l)      //Whitestar: 02/05 The Sopranos (1.07 Spain)
DRIVERNV(sopr107g)      //Whitestar: 02/05 The Sopranos (1.07 Germany)
DRIVERNV(sopr107f)      //Whitestar: 02/05 The Sopranos (1.07 France)
DRIVERNV(sopr107i)      //Whitestar: 02/05 The Sopranos (1.07 Italy)
DRIVERNV(nascar)        //Whitestar: 07/06 Nascar (4.50)
DRIVERNV(nascarl)       //Whitestar: 07/06 Nascar (4.50 Spain)
DRIVERNV(nas400)        //Whitestar: 10/05 Nascar (4.00)
DRIVERNV(nas400l)       //Whitestar: 10/05 Nascar (4.00 Spain)
DRIVERNV(nas350)        //Whitestar: 10/05 Nascar (3.50)
DRIVERNV(nas340)        //Whitestar: 10/05 Nascar (3.40)
DRIVERNV(nas340l)       //Whitestar: 10/05 Nascar (3.40 Spain)
DRIVERNV(nas301)        //Whitestar: 09/05 Nascar (3.01)
DRIVERNV(nas301l)       //Whitestar: 09/05 Nascar (3.01 Spain)
DRIVERNV(gprix)         //Whitestar: 07/06 Grand Prix (4.50)
DRIVERNV(gprixg)        //Whitestar: 07/06 Grand Prix (4.50 Germany)
DRIVERNV(gprixl)        //Whitestar: 07/06 Grand Prix (4.50 Spain)
DRIVERNV(gprixf)        //Whitestar: 07/06 Grand Prix (4.50 France)
DRIVERNV(gprixi)        //Whitestar: 07/06 Grand Prix (4.50 Italy)
DRIVERNV(gpr400)        //Whitestar: 10/05 Grand Prix (4.00)
DRIVERNV(gpr400g)       //Whitestar: 10/05 Grand Prix (4.00 Germany)
DRIVERNV(gpr400l)       //Whitestar: 10/05 Grand Prix (4.00 Spain)
DRIVERNV(gpr400f)       //Whitestar: 10/05 Grand Prix (4.00 France)
DRIVERNV(gpr400i)       //Whitestar: 10/05 Grand Prix (4.00 Italy)
DRIVERNV(gpr340)        //Whitestar: 10/05 Grand Prix (3.40)
DRIVERNV(gpr340g)       //Whitestar: 10/05 Grand Prix (3.40 Germany)
DRIVERNV(gpr340l)       //Whitestar: 10/05 Grand Prix (3.40 Spain)
DRIVERNV(gpr340f)       //Whitestar: 10/05 Grand Prix (3.40 France)
DRIVERNV(gpr340i)       //Whitestar: 10/05 Grand Prix (3.40 Italy)
DRIVERNV(gpr301)        //Whitestar: 09/05 Grand Prix (3.01)
DRIVERNV(gpr301g)       //Whitestar: 09/05 Grand Prix (3.01 Germany)
DRIVERNV(gpr301l)       //Whitestar: 09/05 Grand Prix (3.01 Spain)
DRIVERNV(gpr301f)       //Whitestar: 09/05 Grand Prix (3.01 France)
DRIVERNV(gpr301i)       //Whitestar: 09/05 Grand Prix (3.01 Italy)
                        //Whitestar: ??/06 The Brain (Simpsons Pinball Party conversion)

// Stern S.A.M System
#ifdef INCLUDE_STERN_SAM
DRIVER(sam1_flashb,0102)//S.A.M: 02/06 S.A.M System Flash Boot - V1.02
DRIVER(sam1_flashb,0106)//S.A.M: 08/06 S.A.M System Flash Boot - V1.06
DRIVER(sam1_flashb,0310)//S.A.M: 01/08 S.A.M System Flash Boot - V3.10
DRIVER(wpt,103a)        //S.A.M: ??/06 World Poker Tour - V1.03
DRIVER(wpt,105a)        //S.A.M: 02/06 World Poker Tour - V1.05
DRIVER(wpt,106a)        //S.A.M: 02/06 World Poker Tour - V1.06
DRIVER(wpt,106f)        //S.A.M: 02/06 World Poker Tour - V1.06 (French)
DRIVER(wpt,106g)        //S.A.M: 02/06 World Poker Tour - V1.06 (German)
DRIVER(wpt,106i)        //S.A.M: 02/06 World Poker Tour - V1.06 (Italian)
DRIVER(wpt,106l)        //S.A.M: 02/06 World Poker Tour - V1.06 (Spanish)
DRIVER(wpt,108a)        //S.A.M: 03/06 World Poker Tour - V1.08
DRIVER(wpt,108f)        //S.A.M: 03/06 World Poker Tour - V1.08 (French)
DRIVER(wpt,108g)        //S.A.M: 03/06 World Poker Tour - V1.08 (German)
DRIVER(wpt,108i)        //S.A.M: 03/06 World Poker Tour - V1.08 (Italian)
DRIVER(wpt,108l)        //S.A.M: 03/06 World Poker Tour - V1.08 (Spanish)
DRIVER(wpt,109a)        //S.A.M: 03/06 World Poker Tour - V1.09
DRIVER(wpt,109f)        //S.A.M: 03/06 World Poker Tour - V1.09 (French)
DRIVER(wpt,109g)        //S.A.M: 03/06 World Poker Tour - V1.09 (German)
DRIVER(wpt,109i)        //S.A.M: 03/06 World Poker Tour - V1.09 (Italian)
DRIVER(wpt,109l)        //S.A.M: 03/06 World Poker Tour - V1.09 (Spanish)
DRIVER(wpt,111a)        //S.A.M: 08/06 World Poker Tour - V1.11
DRIVER(wpt,111af)       //S.A.M: 08/06 World Poker Tour - V1.11 (English, French)
DRIVER(wpt,111ai)       //S.A.M: 08/06 World Poker Tour - V1.11 (English, Italian)
DRIVER(wpt,111al)       //S.A.M: 08/06 World Poker Tour - V1.11 (English, Spanish)
DRIVER(wpt,111f)        //S.A.M: 08/06 World Poker Tour - V1.11 (French)
DRIVER(wpt,111g)        //S.A.M: 08/06 World Poker Tour - V1.11 (German)
DRIVER(wpt,111gf)       //S.A.M: 08/06 World Poker Tour - V1.11 (German, French)
DRIVER(wpt,111i)        //S.A.M: 08/06 World Poker Tour - V1.11 (Italian)
DRIVER(wpt,111l)        //S.A.M: 08/06 World Poker Tour - V1.11 (Spanish)
DRIVER(wpt,112a)        //S.A.M: 11/06 World Poker Tour - V1.12
DRIVER(wpt,112af)       //S.A.M: 11/06 World Poker Tour - V1.12 (English, French)
DRIVER(wpt,112ai)       //S.A.M: 11/06 World Poker Tour - V1.12 (English, Italian)
DRIVER(wpt,112al)       //S.A.M: 11/06 World Poker Tour - V1.12 (English, Spanish)
DRIVER(wpt,112f)        //S.A.M: 11/06 World Poker Tour - V1.12 (French)
DRIVER(wpt,112g)        //S.A.M: 11/06 World Poker Tour - V1.12 (German)
DRIVER(wpt,112gf)       //S.A.M: 11/06 World Poker Tour - V1.12 (German, French)
DRIVER(wpt,112i)        //S.A.M: 11/06 World Poker Tour - V1.12 (Italian)
DRIVER(wpt,112l)        //S.A.M: 11/06 World Poker Tour - V1.12 (Spanish)
DRIVER(wpt,140a)        //S.A.M: 01/08 World Poker Tour - V14.0
DRIVER(wpt,140af)       //S.A.M: 01/08 World Poker Tour - V14.0 (English, French)
DRIVER(wpt,140ai)       //S.A.M: 01/08 World Poker Tour - V14.0 (English, Italian)
DRIVER(wpt,140al)       //S.A.M: 01/08 World Poker Tour - V14.0 (English, Spanish)
DRIVER(wpt,140f)        //S.A.M: 01/08 World Poker Tour - V14.0 (French)
DRIVER(wpt,140g)        //S.A.M: 01/08 World Poker Tour - V14.0 (German)
DRIVER(wpt,140gf)       //S.A.M: 01/08 World Poker Tour - V14.0 (German, French)
DRIVER(wpt,140i)        //S.A.M: 01/08 World Poker Tour - V14.0 (Italian)
DRIVER(wpt,140l)        //S.A.M: 01/08 World Poker Tour - V14.0 (Spanish)
DRIVERNV(scarn9nj)      //S.A.M: ??/06 Simpsons Kooky Carnival (Redemption) - V0.9 New Jersey
DRIVERNV(scarn103)      //S.A.M: 04/06 Simpsons Kooky Carnival (Redemption) - V1.03
DRIVERNV(scarn)         //S.A.M: 08/06 Simpsons Kooky Carnival (Redemption) - V1.05
DRIVER(potc,108as)      //S.A.M: 07/06 Pirates of the Caribbean - V1.08 (English, Spanish)
DRIVER(potc,109af)      //S.A.M: 08/06 Pirates of the Caribbean - V1.09 (English, French)
DRIVER(potc,109ai)      //S.A.M: 08/06 Pirates of the Caribbean - V1.09 (English, Italian)
DRIVER(potc,109as)      //S.A.M: 08/06 Pirates of the Caribbean - V1.09 (English, Spanish)
DRIVER(potc,109gf)      //S.A.M: 08/06 Pirates of the Caribbean - V1.09 (German, French)
DRIVER(potc,111as)      //S.A.M: 08/06 Pirates of the Caribbean - V1.11 (English, Spanish)
DRIVER(potc,113af)      //S.A.M: 09/06 Pirates of the Caribbean - V1.13 (English, French)
DRIVER(potc,113ai)      //S.A.M: 09/06 Pirates of the Caribbean - V1.13 (English, Italian)
DRIVER(potc,113as)      //S.A.M: 09/06 Pirates of the Caribbean - V1.13 (English, Spanish)
DRIVER(potc,113gf)      //S.A.M: 09/06 Pirates of the Caribbean - V1.13 (German, French)
DRIVER(potc,115af)      //S.A.M: 11/06 Pirates of the Caribbean - V1.15 (English, French)
DRIVER(potc,115ai)      //S.A.M: 11/06 Pirates of the Caribbean - V1.15 (English, Italian)
DRIVER(potc,115as)      //S.A.M: 11/06 Pirates of the Caribbean - V1.15 (English, Spanish)
DRIVER(potc,115gf)      //S.A.M: 11/06 Pirates of the Caribbean - V1.15 (German, French)
DRIVER(potc,400af)      //S.A.M: 04/07 Pirates of the Caribbean - V4.0  (English, French)
DRIVER(potc,400ai)      //S.A.M: 04/07 Pirates of the Caribbean - V4.0  (English, Italian)
DRIVER(potc,400al)      //S.A.M: 04/07 Pirates of the Caribbean - V4.0  (English, Spanish)
DRIVER(potc,400gf)      //S.A.M: 04/07 Pirates of the Caribbean - V4.0  (German, French)
DRIVER(potc,600af)      //S.A.M: 01/08 Pirates of the Caribbean - V6.0  (English, French)
DRIVER(potc,600ai)      //S.A.M: 01/08 Pirates of the Caribbean - V6.0  (English, Italian)
DRIVER(potc,600as)      //S.A.M: 01/08 Pirates of the Caribbean - V6.0  (English, Spanish)
DRIVER(potc,600gf)      //S.A.M: 01/08 Pirates of the Caribbean - V6.0  (German, French)
DRIVER(fg,200a)         //S.A.M: 02/07 Family Guy - V2.00
DRIVER(fg,300ai)        //S.A.M: 02/07 Family Guy - V3.00  (English, Italian)
DRIVER(fg,800al)        //S.A.M: 03/07 Family Guy - V8.00  (English, Spanish)
DRIVER(fg,1000af)       //S.A.M: 03/07 Family Guy - V10.00 (English, French)
DRIVER(fg,1000ag)       //S.A.M: 03/07 Family Guy - V10.00 (English, German)
DRIVER(fg,1000ai)       //S.A.M: 03/07 Family Guy - V10.00 (English, Italian)
DRIVER(fg,1000al)       //S.A.M: 03/07 Family Guy - V10.00 (English, Spanish)
DRIVER(fg,1100af)       //S.A.M: 06/07 Family Guy - V11.00 (English, French)
DRIVER(fg,1100ag)       //S.A.M: 06/07 Family Guy - V11.00 (English, German)
DRIVER(fg,1100ai)       //S.A.M: 06/07 Family Guy - V11.00 (English, Italian)
DRIVER(fg,1100al)       //S.A.M: 06/07 Family Guy - V11.00 (English, Spanish)
DRIVER(fg,1200af)       //S.A.M: 01/08 Family Guy - V12.00 (English, French)
DRIVER(fg,1200ag)       //S.A.M: 01/08 Family Guy - V12.00 (English, German)
DRIVER(fg,1200ai)       //S.A.M: 01/08 Family Guy - V12.00 (English, Italian)
DRIVER(fg,1200al)       //S.A.M: 01/08 Family Guy - V12.00 (English, Spanish)
DRIVER(sman,130af)      //S.A.M: 06/07 Spider-Man - V1.30 (English, French)
DRIVER(sman,130ai)      //S.A.M: 06/07 Spider-Man - V1.30 (English, Italian)
DRIVER(sman,130al)      //S.A.M: 06/07 Spider-Man - V1.30 (English, Spanish)
DRIVER(sman,130gf)      //S.A.M: 06/07 Spider-Man - V1.30 (German, French)
DRIVER(sman,132)        //S.A.M: ??/07 Spider-Man - V1.32
DRIVER(sman,170)        //S.A.M: ??/07 Spider-Man - V1.70
DRIVER(sman,170af)      //S.A.M: ??/07 Spider-Man - V1.70 (English, French)
DRIVER(sman,170ai)      //S.A.M: ??/07 Spider-Man - V1.70 (English, Italian)
DRIVER(sman,170al)      //S.A.M: ??/07 Spider-Man - V1.70 (English, Spanish)
DRIVER(sman,170gf)      //S.A.M: ??/07 Spider-Man - V1.70 (German, French)
DRIVER(sman,190)        //S.A.M: 11/07 Spider-Man - V1.90
DRIVER(sman,190af)      //S.A.M: 11/07 Spider-Man - V1.90 (English, French)
DRIVER(sman,190ai)      //S.A.M: 11/07 Spider-Man - V1.90 (English, Italian)
DRIVER(sman,190al)      //S.A.M: 11/07 Spider-Man - V1.90  (English, Spanish)
DRIVER(sman,190gf)      //S.A.M: 11/07 Spider-Man - V1.90 (German, French)
DRIVER(sman,192)        //S.A.M: 01/08 Spider-Man - V1.92
DRIVER(sman,192af)      //S.A.M: 01/08 Spider-Man - V1.92 (English, French)
DRIVER(sman,192ai)      //S.A.M: 01/08 Spider-Man - V1.92 (English, Italian)
DRIVER(sman,192al)      //S.A.M: 01/08 Spider-Man - V1.92 (English, Spanish)
DRIVER(sman,192gf)      //S.A.M: 01/08 Spider-Man - V1.92 (German, French)
DRIVER(sman,210)        //S.A.M: 12/08 Spider-Man - V2.1
DRIVER(sman,210af)      //S.A.M: 12/08 Spider-Man - V2.1  (English, French)
DRIVER(sman,210ai)      //S.A.M: 12/08 Spider-Man - V2.1  (English, Italian)
DRIVER(sman,210al)      //S.A.M: 12/08 Spider-Man - V2.1  (English, Spanish)
DRIVER(sman,210gf)      //S.A.M: 12/08 Spider-Man - V2.1  (German, French)
DRIVER(sman,240)        //S.A.M: 11/09 Spider-Man - V2.4
DRIVER(wof,100)         //S.A.M: 11/07 Wheel of Fortune - V1.0
DRIVER(wof,200)         //S.A.M: 11/07 Wheel of Fortune - V2.0
DRIVER(wof,200f)        //S.A.M: 11/07 Wheel of Fortune - V2.0 (French)
DRIVER(wof,200g)        //S.A.M: 11/07 Wheel of Fortune - V2.0 (German)
DRIVER(wof,200i)        //S.A.M: 11/07 Wheel of Fortune - V2.0 (Italian)
DRIVER(wof,300)         //S.A.M: 12/07 Wheel of Fortune - V3.0
DRIVER(wof,300f)        //S.A.M: 12/07 Wheel of Fortune - V3.0 (French)
DRIVER(wof,300g)        //S.A.M: 12/07 Wheel of Fortune - V3.0 (German)
DRIVER(wof,300i)        //S.A.M: 12/07 Wheel of Fortune - V3.0 (Italian)
DRIVER(wof,300l)        //S.A.M: 12/07 Wheel of Fortune - V3.0 (Spanish)
DRIVER(wof,400)         //S.A.M: 12/07 Wheel of Fortune - V4.0
DRIVER(wof,400f)        //S.A.M: 12/07 Wheel of Fortune - V4.0 (French)
DRIVER(wof,400g)        //S.A.M: 12/07 Wheel of Fortune - V4.0 (German)
DRIVER(wof,400i)        //S.A.M: 12/07 Wheel of Fortune - V4.0 (Italian)
DRIVER(wof,400l)        //S.A.M: 12/07 Wheel of Fortune - V4.0 (Spanish)
DRIVER(wof,500)         //S.A.M: 12/07 Wheel of Fortune - V5.0
DRIVER(wof,500f)        //S.A.M: 12/07 Wheel of Fortune - V5.0 (French)
DRIVER(wof,500g)        //S.A.M: 12/07 Wheel of Fortune - V5.0 (German)
DRIVER(wof,500i)        //S.A.M: 12/07 Wheel of Fortune - V5.0 (Italian)
DRIVER(wof,500l)        //S.A.M: 12/07 Wheel of Fortune - V5.0 (Spanish)
DRIVER(shr,130)         //S.A.M: 03/08 Shrek - V1.30
DRIVER(shr,141)         //S.A.M: 04/08 Shrek - V1.41
DRIVER(ij4,113)         //S.A.M: 05/08 Indiana Jones - V1.13
DRIVER(ij4,113f)        //S.A.M: 05/08 Indiana Jones - V1.13 (French)
DRIVER(ij4,113g)        //S.A.M: 05/08 Indiana Jones - V1.13 (German)
DRIVER(ij4,113i)        //S.A.M: 05/08 Indiana Jones - V1.13 (Italian)
DRIVER(ij4,113l)        //S.A.M: 05/08 Indiana Jones - V1.13 (Spanish)
DRIVER(ij4,114)         //S.A.M: 06/08 Indiana Jones - V1.14
DRIVER(ij4,114f)        //S.A.M: 06/08 Indiana Jones - V1.14 (French)
DRIVER(ij4,114g)        //S.A.M: 06/08 Indiana Jones - V1.14 (German)
DRIVER(ij4,114i)        //S.A.M: 06/08 Indiana Jones - V1.14 (Italian)
DRIVER(ij4,114l)        //S.A.M: 06/08 Indiana Jones - V1.14 (Spanish)
DRIVER(ij4,116)         //S.A.M: 09/08 Indiana Jones - V1.16
DRIVER(ij4,116f)        //S.A.M: 09/08 Indiana Jones - V1.16 (French)
DRIVER(ij4,116g)        //S.A.M: 09/08 Indiana Jones - V1.16 (German)
DRIVER(ij4,116i)        //S.A.M: 09/08 Indiana Jones - V1.16 (Italian)
DRIVER(ij4,116l)        //S.A.M: 09/08 Indiana Jones - V1.16 (Spanish)
DRIVER(ij4,210)         //S.A.M: ??/09 Indiana Jones - V2.1
DRIVER(ij4,210f)        //S.A.M: ??/09 Indiana Jones - V2.1 (French)
DRIVER(bdk,130)         //S.A.M: 07/08 Batman - The Dark Knight - V1.3
DRIVER(bdk,200)         //S.A.M: 08/08 Batman - The Dark Knight - V2.0
DRIVER(bdk,201)         //S.A.M: 08/08 Batman - The Dark Knight - V2.01
DRIVER(bdk,202)         //S.A.M: 08/08 Batman - The Dark Knight - V2.02
DRIVER(bdk,240)         //S.A.M: 11/09 Batman - The Dark Knight - V2.4
DRIVER(bdk,290)         //S.A.M: 05/10 Batman - The Dark Knight - V2.9
DRIVER(bdk,294)         //S.A.M: 05/10 Batman - The Dark Knight - V2.94
DRIVER(csi,102)         //S.A.M: 11/08 C.S.I. - V1.02
DRIVER(csi,200)         //S.A.M: 12/08 C.S.I. - V2.0
DRIVER(csi,210)         //S.A.M: 01/09 C.S.I. - V2.1
DRIVER(csi,230)         //S.A.M: 08/09 C.S.I. - V2.3
DRIVER(csi,240)         //S.A.M: 08/09 C.S.I. - V2.4
DRIVER(twenty4,130)     //S.A.M: 03/09 24 - V1.3
DRIVER(twenty4,140)     //S.A.M: 03/09 24 - V1.4
DRIVER(twenty4,144)     //S.A.M: 09/09 24 - V1.44
DRIVER(twenty4,150)     //S.A.M: 05/10 24 - V1.5
DRIVER(nba,600)         //S.A.M: 06/09 NBA - V6.0
DRIVER(nba,700)         //S.A.M: 06/09 NBA - V7.0
DRIVER(nba,801)         //S.A.M: 08/09 NBA - V8.01
DRIVER(nba,802)         //S.A.M: 11/09 NBA - V8.02
DRIVER(bbh,140)         //S.A.M: 02/10 Big Buck Hunter Pro - V1.4
DRIVER(bbh,150)         //S.A.M: 02/10 Big Buck Hunter Pro - V1.5
DRIVER(bbh,160)         //S.A.M: 05/10 Big Buck Hunter Pro - V1.6
DRIVER(im2,100)         //S.A.M: 04/10 Iron Man 2 - V1.0
DRIVER(im2,110)         //S.A.M: 04/10 Iron Man 2 - V1.1
DRIVER(im2,120)         //S.A.M: 04/10 Iron Man 2 - V1.2
DRIVER(im2,140)         //S.A.M: 04/10 Iron Man 2 - V1.4
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
                        //??/?? Football
                        //??/79 Hot Ball (B Eight Ball, 01/77)
DRIVERNV(shock   )      //??/79 Shock (W Flash, 01/79)
                        //??/?? Sultan (G Sinbad, 05/78)
DRIVERNV(obaoba  )      //??/80 Oba Oba (B Playboy, 10/77)
DRIVERNV(obaoba1 )      //??/80 Oba Oba alternate set
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
DRIVERNV(hawkman )    //??/82 Hawkman (B Fathom, 12/80)
DRIVERNV(hawkman1)    //??/82 Hawkman alternate set
DRIVERNV(stest   )      //??/82 Speed Test
DRIVERNV(lunelle )      //??/82 Lunelle (W Alien Poker, 10/80)
DRIVERNV(rally   )      //??/82 Rally (B Skateball, 04/80)
DRIVERNV(snake   )      //??/82 Snake Machine
DRIVERNV(gork    )      //??/82 Gork
DRIVERNV(voleybal)      //??/?? Voley Ball
                        //??/8? Ogar
DRIVERNV(icecold)       //??/83 Ice Cold Beer (Redemption)
DRIVERNV(icecoldf)      //??/83 Ice Cold Beer (Redemption Free Play)
DRIVERNV(zekepeak)      //??/83 Zeke's Peak (Redemption)
DRIVERNV(mrblack )    //??/84 Mr. Black (W Defender, 12/82)
DRIVERNV(mrblack1)    //??/84 Mr. Black alternate set
DRIVERNV(mrblkz80)    //??/8? Mr. Black (Z-80 CPU)
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
            //??/?? Space Team

// ----------------
// UNITED GAMES
// ----------------
DRIVERNV(bbbowlin)      //Big Ball Bowling - using Bally hardware

// -----------------------------------
// VALLEY GAMES
// -----------------------------------
//DRIVERNV(spectra)       //??/79 Spectra IV

// -----------------------------------
// VIDEODENS GAMES
// -----------------------------------
//                      //??/86 Papillon
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
DRIVER(pomp,l1)         //          ??/78 W Pompeii (Shuffle)
DRIVER(topaz,l1)        //          ??/78 W Topaz (Shuffle)
DRIVER(pkrno,l1)        //S4-488:   10/78 W Pokerino
DRIVER(phnix,l1)        //S4-485:   11/78 W Phoenix
DRIVER(flash,l1)        //S4-486:   01/79 W Flash
DRIVER(flash,t1)        //          01/79 W Flash /10 Scoring Ted Estes
DRIVER(stlwr,l2)        //S4-490:   03/79 W Stellar Wars
                        //S?-491:   06/79 W Rock'N Roll
DRIVER(arist,l1)        //          ??/79 W Aristocrat (Shuffle)
DRIVER(taurs,l1)        //          ??/79 W Taurus (Shuffle)
DRIVER(kingt,l1)        //          ??/79 W King Tut (Shuffle)
//System 6
DRIVER(trizn,l1)        //S6-487:   07/79 W TriZone
DRIVER(trizn,t1)        //          07/79 W TriZone /10 Scoring Ted Estes
DRIVER(tmwrp,l2)        //S6-489:   09/79 W Time Warp
DRIVER(tmwrp,t2)        //          09/79 W Time Warp /10 Scoring Ted Estes
DRIVER(grgar,l1)        //S6-496:   11/79 W Gorgar
DRIVER(grgar,t1)        //          11/79 W Gorgar /10 Scoring Ted Estes
DRIVER(lzbal,l2)        //S6-493:   12/79 W Laser Ball
DRIVER(lzbal,t2)        //          12/79 W Laser Ball /10 Scoring Ted Estes
DRIVER(frpwr,l2)        //S6-497:   02/80 W Firepower (L-2)
DRIVER(frpwr,l6)        //S6-497:   02/80 W Firepower (L-6)
DRIVER(frpwr,t6)        //S6-497:   02/80 W Firepower (L-6) /10 Scoring Ted Estes
DRIVER(frpwr,a6)        //S6:       10/05 W Firepower (Sys.6/6-Digit Custom Rev. 31)
DRIVER(frpwr,b6)        //S6:       12/03 W Firepower (Sys.6 7-digit conversion)
DRIVER(frpwr,c6)        //S6:       12/03 W Firepower (Sys.6/7-Digit Custom Rev. 31)
DRIVER(frpwr,d6)        //S7:       10/05 W Firepower (Sys.6/6-digit /10 Scoring Rev. 31)
DRIVER(omni,l1)         //          04/80 W Omni (Shuffle)
DRIVER(blkou,l1)        //S6-495:   06/80 W Blackout
DRIVER(blkou,f1)        //          06/80 W Blackout, French speech
DRIVER(blkou,t1)        //          06/80 W Blackout /10 Scoring Ted Estes
DRIVER(scrpn,l1)        //S6-494:   07/80 W Scorpion
DRIVER(scrpn,t1)        //          07/80 W Scorpion /10 Scoring Ted Estes
DRIVER(algar,l1)        //S6-499:   09/80 W Algar
DRIVER(alpok,l2)        //S6-501:   10/80 W Alien Poker L-2
DRIVER(alpok,l6)        //          10/80 W Alien Poker L-6
DRIVER(alpok,f6)        //          10/80 W Alien Poker L-6, French speech
DRIVER(alpok,b6)        //          11/06 W Alien Poker Multiball mod
//System 7
DRIVER(frpwr,a7)        //S7:       10/05 W Firepower (Sys.7/6-digit Custom Rev. 31)
DRIVER(frpwr,b7)        //S7:       12/03 W Firepower (Sys.7 7-digit conversion)
DRIVER(frpwr,c7)        //S7:       11/06 W Firepower (Sys.7/7-digit Custom Rev. 38)
DRIVER(frpwr,d7)        //S7:       10/05 W Firepower (Sys.7/7-digit Custom Rev. 31)
DRIVER(frpwr,e7)        //S7:       10/05 W Firepower (Sys.7/6-digit /10 Scoring Rev. 31)
DRIVER(bk,l3)           //S7-500:   11/80 W Black Knight (L-3)
DRIVER(bk,l4)           //S7-500:   11/80 W Black Knight (L-4)
DRIVER(bk,f4)           //          11/80 W Black Knight (L-4) French speech
DRIVER(jngld,l2)        //S7-503:   02/81 W Jungle Lord
DRIVER(pharo,l2)        //S7-504:   05/81 W Pharaoh
                        //S7-506:   06/81 W Black Knight Limited Edition
DRIVER(solar,l2)        //S7-507:   07/81 W Solar Fire
DRIVER(barra,l1)        //S7-510:   09/81 W Barracora
DRIVER(hypbl,l4)        //S7-509:   12/81 W HyperBall
DRIVER(hypbl,l5)        //          04/98 W HyperBall (bootleg w/ high score save)
DRIVER(hypbl,l6)        //          05/06 W HyperBall (bootleg w/ high score save)
DRIVER(thund,p1)        //S7-508:   05/82 W Thunderball
DRIVER(csmic,l1)        //S7-502:   06/82 W Cosmic Gunfight
DRIVER(vrkon,l1)        //S7-512:   09/82 W Varkon
DRIVER(wrlok,l3)        //S7-516:   10/82 W Warlok
DRIVER(dfndr,l4)        //S7-517:   12/82 W Defender
DRIVER(ratrc,l1)        //S7-5??:   01/83 W Rat Race (never produced)
DRIVER(tmfnt,l5)        //S7-515:   03/83 W Time Fantasy
DRIVER(jst,l2)          //S7-519:   04/83 W Joust
DRIVER(fpwr2,l2)        //S7-521:   08/83 W Firepower II
DRIVER(bstrk,l1)        //          ??/83 W Big Strike (Bowler)
DRIVER(tstrk,l1)        //          ??/83 W Triple Strike (Bowler)
DRIVER(lsrcu,l2)        //S7-520:   02/84 W Laser Cue
DRIVER(strlt,l1)        //S7-530:   06/84 W Star Light (Came out after System 9 produced)
//System 9
DRIVER(pfevr,p3)        //S9-526:   05/84 W Pennant Fever (pitch & bat)
DRIVER(pfevr,l2)        //          05/84 W Pennant Fever (pitch & bat)
                        //S?-538:   10/84 W Gridiron
DRIVER(sshtl,l7)        //S9-535:   12/84 W Space Shuttle
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
DRIVER(grand,l4)        //S11-523:  04/86 W Grand Lizard
DRIVER(rdkng,l1)        //S11-542:  07/86 W Road Kings (L-1)
DRIVER(rdkng,l2)        //          07/86 W Road Kings (L-2)
DRIVER(rdkng,l3)        //          07/86 W Road Kings (L-3)
DRIVER(rdkng,l4)        //          07/86 W Road Kings (L-4)
DRIVER(pb,l2)           //S11-549:  10/86 W Pin-bot (L-2)
DRIVER(pb,l3)           //          10/86 W Pin-bot (L-3)
DRIVER(pb,l5)           //          10/86 W Pin-bot (L-5)
                        //S11:      10/86 W Strike Force
DRIVER(tts,l1)          //S11-919:  ??/86 W Tic-Tac-Strike (L-1, Shuffle)
DRIVER(tts,l2)          //S11-919:  ??/86 W Tic-Tac-Strike (L-2, Shuffle)
DRIVER(milln,l3)        //S11-555:  01/87 W Millionaire
DRIVER(f14,p3)          //S11-554:  03/87 W F-14 Tomcat (P-3)
DRIVER(f14,p4)          //          03/87 W F-14 Tomcat (P-4)
DRIVER(f14,l1)          //          05/87 W F-14 Tomcat (L-1)
DRIVER(fire,l3)         //S11-556:  08/87 W Fire!
DRIVER(bguns,p1)        //S11-557:  10/87 W Big Guns (P-1)
DRIVER(bguns,la)        //          10/87 W Big Guns (L-A)
DRIVER(bguns,l7)        //          10/87 W Big Guns (L-7)
DRIVER(bguns,l8)        //          10/87 W Big Guns (L-8)
DRIVER(spstn,l5)        //S11-552:  12/87 W Space Station
DRIVER(gmine,l2)        //S11-920:  ??/87 W Gold Mine (Shuffle)
DRIVER(tdawg,l1)        //S11-921:  ??/87 W Top Dawg (Shuffle)
DRIVER(shfin,l1)        //S11-922:  ??/87 W Shuffle Inn (Shuffle)
DRIVER(cycln,l4)        //S11-564:  02/88 W Cyclone (L-4)
DRIVER(cycln,l5)        //          02/88 W Cyclone (L-5)
DRIVER(bnzai,pa)        //S11-566:  05/88 W Banzai Run (P-A)
DRIVER(bnzai,l1)        //          05/88 W Banzai Run (L-1)
DRIVER(bnzai,l3)        //          05/88 W Banzai Run (L-3)
DRIVER(bnzai,g3)        //          05/88 W Banzai Run (L-3 Germany)
DRIVER(swrds,l2)        //S11-559:  06/88 W Swords of Fury
DRIVER(taxi,lg1)        //S11-553:  08/88 W Taxi (Marilyn LG-1) Germany
DRIVER(taxi,l3)         //          08/88 W Taxi (Marilyn)
DRIVER(taxi,l4)         //          08/88 W Taxi (Lola)
DRIVER(jokrz,l3)        //S11-567:  12/88 W Jokerz! (L-3)
DRIVER(jokrz,l6)        //          12/88 W Jokerz! (L-6)
DRIVER(esha,pa1)        //S11-568:  02/89 W Earthshaker Prototype version
DRIVER(esha,la1)        //          02/89 W Earthshaker LA-1
DRIVER(esha,lg1)        //          02/89 W Earthshaker LG-1 (German)
DRIVER(esha,lg2)        //          02/89 W Earthshaker LG-2 (German)
DRIVER(esha,la3)        //          02/89 W Earthshaker LA-3
DRIVER(esha,ma3)        //          02/89 W Earthshaker LA-3 (Metallica)
DRIVER(esha,pr4)        //          02/89 W Earthshaker Family version
DRIVER(bk2k,pu1)        //S11-563:  04/89 W Black Knight 2000 (PU-1)
DRIVER(bk2k,lg1)        //          04/89 W Black Knight 2000 (LG-1)
DRIVER(bk2k,lg3)        //          04/89 W Black Knight 2000 (LG-3)
DRIVER(bk2k,l4)         //          04/89 W Black Knight 2000 (L-4)
                        //S11:      05/89 W Pool
//First Game produced entirely by Williams after Merger to use Bally Name
DRIVER(tsptr,l3)        //S11-2630: 07/89 B Transporter the Rescue
DRIVER(polic,l2)        //S11-573:  08/89 W Police Force (LA-2)
DRIVER(polic,l3)        //          09/89 W Police Force (LA-3)
DRIVER(polic,l4)        //          10/89 W Police Force (LA-4)
DRIVER(eatpm,p7)        //S11-782:  09/89 B Elvira and the Party Monsters (PA-7)
DRIVER(eatpm,l1)        //          09/89 B Elvira and the Party Monsters (L-1)
DRIVER(eatpm,l2)        //          10/89 B Elvira and the Party Monsters (L-2)
DRIVER(eatpm,l4)        //          10/89 B Elvira and the Party Monsters (L-4)
DRIVER(eatpm,4u)        //          10/89 B Elvira and the Party Monsters (L-4 European)
DRIVER(eatpm,4g)        //          10/89 B Elvira and the Party Monsters (L-4 German)
DRIVER(bcats,l2)        //S11-575:  11/89 W Bad Cats (LA-2)
DRIVER(bcats,l5)        //          11/89 W Bad Cats (L-5)
DRIVER(rvrbt,l3)        //S11-1966: 09/90 W Riverboat Gambler
DRIVER(mousn,l1)        //S11-1635: 11/89 B Mousin' Around! (L-1)
DRIVER(mousn,lu)        //          11/89 B Mousin' Around! (Europe)
DRIVER(mousn,l4)        //          03/90 B Mousin' Around! (L-4)
DRIVER(mousn,lx)        //          03/90 B Mousin' Around! (L-X)
                        //???:      ??/90 B Mazatron
                        //???:      ??/90 B Player's Choice
                        //???:      ??/90 B Ghost Gallery
DRIVER(whirl,l2)        //S11-574:  01/90 W Whirlwind (L-2)
DRIVER(whirl,l3)        //          01/90 W Whirlwind (L-3)
DRIVER(whirl,g3)        //          01/90 W Whirlwind (LG-3)
DRIVER(gs,l3)           //S11-985:  02/90 B Game Show (L-3)
DRIVER(gs,l4)           //          02/90 B Game Show (L-4)
DRIVER(rollr,ex)        //S11-576:  01/90 W Rollergames (EXPERIMENTAL)
DRIVER(rollr,e1)        //          01/90 W Rollergames (PU-1) Europe
DRIVER(rollr,p2)        //          01/90 W Rollergames (PA-2, PA-1 Sound)
DRIVER(rollr,l2)        //          04/90 W Rollergames (L-2)
DRIVER(rollr,l3)        //          05/90 W Rollergames (L-3) Europe
DRIVER(rollr,g3)        //          05/90 W Rollergames (L-3) Germany
DRIVER(pool,p7)         //S11-1848: 09/89?B Pool Sharks (PA-7) Proto
DRIVER(pool,le2)        //          03/90?B Pool Sharks (LE-2) Europe
DRIVER(pool,l6)         //          05/90 B Pool Sharks (LA-6)
DRIVER(pool,l7)         //          01/91 B Pool Sharks (LA-7)
DRIVER(diner,l1)        //S11-571:  06/90 W Diner (L-1)
DRIVER(diner,l3)        //          06/90 W Diner (L-3)
DRIVER(diner,l4)        //          09/90 W Diner (L-4)
DRIVER(radcl,p3)        //S11-1904: 06/90 B Radical! (P-3)
DRIVER(radcl,l1)        //          06/90 B Radical!
DRIVER(radcl,g1)        //          06/90 B Radical! (G-1)
DRIVER(strax,p7)        //S11-???:  09/90 W Star Trax (domestic prototype)
DRIVER(dd,p6)           //S11-737:  08/90 B Dr. Dude (P-6)
DRIVER(dd,l2)           //          09/90 B Dr. Dude (L-2)
DRIVER(bbnny,l2)        //S11-396:  12/90 B Bugs Bunny's Birthday Ball (L-2)
DRIVER(bbnny,lu)        //          12/90 B Bugs Bunny's Birthday Ball (LU-2) European
//WPC
DRIVER(dd,p06)          //WPC:      08/90 B Dr. Dude (P-6 WPC)
DRIVER(dd,p7)           //WPC:      09/90 B Dr. Dude (P-7 WPC)
                        //WPC-503:  11/90 W Funhouse (L-1)
DRIVER(fh,l3)           //          12/90 W Funhouse (L-3)
DRIVER(fh,d3)           //                W Funhouse (D-3) LED Ghost Fix
DRIVER(fh,l4)           //          01/91 W Funhouse (L-4)
DRIVER(fh,d4)           //                W Funhouse (D-4) LED Ghost Fix
DRIVER(fh,l5)           //          02/91 W Funhouse (L-5)
DRIVER(fh,d5)           //                W Funhouse (D-5) LED Ghost Fix
DRIVER(fh,l9)           //          12/92 W Funhouse (L-9)
DRIVER(fh,d9)           //                W Funhouse (D-9) LED Ghost Fix
DRIVER(fh,l9b)          //                  bootleg with correct German translation
DRIVER(fh,d9b)          //                  bootleg with correct German translation LED Ghost Fix
DRIVER(fh,905h)         //          04/96 W Funhouse (9.05H)
DRIVER(fh,f91)          //WPC-503:  12/90 W Funhouse (FreeWPC 0.91)
DRIVER(bop,l2)          //WPC-502:  04/91 W The Machine: Bride of Pinbot (L-2)
DRIVER(bop,d2)          //WPC-502:  04/91 W The Machine: Bride of Pinbot (D-2) LED Ghost Fix
DRIVER(bop,l3)          //WPC-502:  04/91 W The Machine: Bride of Pinbot (L-3)
DRIVER(bop,d3)          //WPC-502:  04/91 W The Machine: Bride of Pinbot (D-3) LED Ghost Fix
DRIVER(bop,l4)          //WPC-502:  04/91 W The Machine: Bride of Pinbot (L-4)
DRIVER(bop,d4)          //WPC-502:  04/91 W The Machine: Bride of Pinbot (D-4) LED Ghost Fix
DRIVER(bop,l5)          //WPC-502:  05/91 W The Machine: Bride of Pinbot (L-5)
DRIVER(bop,d5)          //WPC-502:  05/91 W The Machine: Bride of Pinbot (D-5) LED Ghost Fix
DRIVER(bop,l6)          //WPC-502:  05/91 W The Machine: Bride of Pinbot (L-6)
DRIVER(bop,d6)          //WPC-502:  05/91 W The Machine: Bride of Pinbot (D-6) LED Ghost Fix
DRIVER(bop,l7)          //WPC-502:  12/92 W The Machine: Bride of Pinbot (L-7)
DRIVER(bop,d7)          //WPC-502:  12/92 W The Machine: Bride of Pinbot (D-7) LED Ghost Fix
DRIVER(bop,l8)          //WPC-502:  12/92 W The Machine: Bride of Pinbot (L-8) fixes billionaire crash multiplayer bug
DRIVER(bop,d8)          //WPC-502:  12/92 W The Machine: Bride of Pinbot (D-8) LED Ghost Fix
DRIVER(hd,l1)           //WPC:      02/91 B Harley Davidson (L-1)
DRIVER(hd,d1)           //WPC:      02/91 B Harley Davidson (D-1) LED Ghost Fix
DRIVER(hd,l3)           //WPC:      02/91 B Harley Davidson (L-3)
DRIVER(hd,d3)           //WPC:      02/91 B Harley Davidson (D-3) LED Ghost Fix
DRIVER(sf,l1)           //WPC-601:  03/91 W SlugFest (L-1)
DRIVER(sf,d1)           //WPC-601:  03/91 W SlugFest (D-1) LED Ghost Fix
DRIVER(hurr,l2)         //WPC-512:  08/91 W Hurricane (L-2)
DRIVER(hurr,d2)         //WPC-512:  08/91 W Hurricane (D-2) LED Ghost Fix
DRIVER(t2,l2)           //WPC-513:  10/91 W Terminator 2: Judgement Day (L-2)
DRIVER(t2,d2)           //WPC-513:  10/91 W Terminator 2: Judgement Day (D-2) LED Ghost Fix
DRIVER(t2,l3)           //WPC-513:  10/91 W Terminator 2: Judgement Day (L-3)
DRIVER(t2,d3)           //WPC-513:  10/91 W Terminator 2: Judgement Day (D-3) LED Ghost Fix
DRIVER(t2,l4)           //WPC-513:  10/91 W Terminator 2: Judgement Day (L-4)
DRIVER(t2,d4)           //WPC-513:  10/91 W Terminator 2: Judgement Day (D-4) LED Ghost Fix
DRIVER(t2,l6)           //WPC-513:  10/91 W Terminator 2: Judgement Day (L-6)
DRIVER(t2,d6)           //WPC-513:  10/91 W Terminator 2: Judgement Day (D-6) LED Ghost Fix
DRIVER(t2,l8)           //WPC-513:  12/91 W Terminator 2: Judgement Day (L-8)
DRIVER(t2,d8)           //WPC-513:  12/91 W Terminator 2: Judgement Day (D-8) LED Ghost Fix
DRIVER(t2,p2f)          //          12/91 W Terminator 2: Judgement Day (Profanity Speech version)
DRIVER(t2,p2g)          //          12/91 W Terminator 2: Judgement Day (Profanity Speech version) LED Ghost Fix
DRIVER(t2,f32)          //WPC-513:  12/91 W Terminator 2: Judgement Day (FreeWPC 0.32)
DRIVER(gi,l3)           //WPC-203:  07/91 B Gilligan's Island (L-3)
DRIVER(gi,d3)           //WPC-203:  07/91 B Gilligan's Island (D-3) LED Ghost Fix
DRIVER(gi,l4)           //WPC-203:  07/91 B Gilligan's Island (L-4)
DRIVER(gi,d4)           //WPC-203:  07/91 B Gilligan's Island (D-4) LED Ghost Fix
DRIVER(gi,l6)           //WPC-203:  08/91 B Gilligan's Island (L-6)
DRIVER(gi,d6)           //WPC-203:  08/91 B Gilligan's Island (D-6) LED Ghost Fix
DRIVER(gi,l9)           //WPC-203:  12/92 B Gilligan's Island (L-9)
DRIVER(gi,d9)           //WPC-203:  12/92 B Gilligan's Island (D-9) LED Ghost Fix
DRIVER(pz,l1)           //WPC-204:  10/91 B Party Zone (L-1)
DRIVER(pz,d1)           //WPC-204:  10/91 B Party Zone (D-1) LED Ghost Fix
DRIVER(pz,l2)           //WPC-204:  10/91 B Party Zone (L-2)
DRIVER(pz,d2)           //WPC-204:  10/91 B Party Zone (D-2) LED Ghost Fix
DRIVER(pz,l3)           //WPC-204:  10/91 B Party Zone (L-3)
DRIVER(pz,d3)           //WPC-204:  10/91 B Party Zone (D-3) LED Ghost Fix
DRIVER(pz,f4)           //WPC-204:  10/91 B Party Zone (F-4)
DRIVER(pz,f5)           //WPC-204:  10/91 B Party Zone (F-5) LED Ghost Fix
DRIVER(strik,l4)        //WPC:      05/92 W Strike Master (L-4)
DRIVER(strik,d4)        //WPC:      05/92 W Strike Master (D-4) LED Ghost Fix
DRIVER(taf,p2)          //WPC:      01/92 B The Addams Family (P-2)
DRIVER(taf,p3)          //WPC:      01/92 B The Addams Family (P-3) LED Ghost Fix
DRIVER(taf,l1)          //WPC:      01/92 B The Addams Family (L-1)
DRIVER(taf,d1)          //WPC:      01/92 B The Addams Family (D-1) LED Ghost Fix
DRIVER(taf,l2)          //WPC:      03/92 B The Addams Family (L-2)
DRIVER(taf,d2)          //WPC:      03/92 B The Addams Family (D-2) LED Ghost Fix
DRIVER(taf,l3)          //WPC:      05/92 B The Addams Family (L-3)
DRIVER(taf,d3)          //WPC:      05/92 B The Addams Family (D-3) LED Ghost Fix
DRIVER(taf,l4)          //WPC:      05/92 B The Addams Family (L-4)
DRIVER(taf,d4)          //WPC:      05/92 B The Addams Family (D-4) LED Ghost Fix
DRIVER(taf,l5)          //WPC:      12/92 B The Addams Family (L-5)
DRIVER(taf,d5)          //WPC:      12/92 B The Addams Family (D-5) LED Ghost Fix
DRIVER(taf,l6)          //WPC:      03/93 B The Addams Family (L-6)
DRIVER(taf,d6)          //WPC:      03/93 B The Addams Family (D-6) LED Ghost Fix
DRIVER(taf,l7)          //WPC:      10/92 B The Addams Family (L-7) (Prototype L-5)
DRIVER(taf,d7)          //WPC:      10/92 B The Addams Family (D-7) (Prototype L-5) LED Ghost Fix
DRIVER(taf,h4)          //WPC:      05/94 B The Addams Family (H-4)
DRIVER(taf,i4)          //WPC:      05/94 B The Addams Family (I-4) LED Ghost Fix
DRIVER(gw,pc)           //WPC-504:  03/92 W The Getaway: High Speed II (P-C)
DRIVER(gw,pd)           //WPC-504:  03/92 W The Getaway: High Speed II (P-D) LED Ghost Fix
DRIVER(gw,p7)           //WPC-504:  03/92 W The Getaway: High Speed II (P-7)
DRIVER(gw,p8)           //WPC-504:  03/92 W The Getaway: High Speed II (P-8) LED Ghost Fix
DRIVER(gw,l1)           //WPC-504:  04/92 W The Getaway: High Speed II (L-1)
DRIVER(gw,d1)           //WPC-504:  04/92 W The Getaway: High Speed II (D-1) LED Ghost Fix
DRIVER(gw,l2)           //WPC-504:  06/92 W The Getaway: High Speed II (L-2)
DRIVER(gw,d2)           //WPC-504:  06/92 W The Getaway: High Speed II (D-2) LED Ghost Fix
DRIVER(gw,l3)           //WPC-504:  06/92 W The Getaway: High Speed II (L-3)
DRIVER(gw,d3)           //WPC-504:  06/92 W The Getaway: High Speed II (D-3) LED Ghost Fix
DRIVER(gw,l5)           //WPC-504:  12/92 W The Getaway: High Speed II (L-5)
DRIVER(gw,d5)           //WPC-504:  12/92 W The Getaway: High Speed II (D-5) LED Ghost Fix
DRIVER(br,p17)          //WPC:      05/92 B Black Rose (P-17)
DRIVER(br,p18)          //WPC:      05/92 B Black Rose (P-18) LED Ghost Fix
DRIVER(br,l1)           //WPC:      08/92 B Black Rose (L-1)
DRIVER(br,d1)           //WPC:      08/92 B Black Rose (D-1) LED Ghost Fix
DRIVER(br,l3)           //WPC:      01/93 B Black Rose (L-3)
DRIVER(br,d3)           //WPC:      01/93 B Black Rose (D-3) LED Ghost Fix
DRIVER(br,l4)           //WPC:      11/93 B Black Rose (L-4)
DRIVER(br,d4)           //WPC:      11/93 B Black Rose (D-4) LED Ghost Fix
DRIVER(ft,p4)           //WPC-505:  07/92 W Fish Tales (P-4)
DRIVER(ft,p5)           //WPC-505:  07/92 W Fish Tales (P-5) LED Ghost Fix
DRIVER(ft,l3)           //WPC-505:  09/92 W Fish Tales (L-3)
DRIVER(ft,l4)           //WPC-505:  09/92 W Fish Tales (L-4)
DRIVER(ft,l5)           //WPC-505:  12/92 W Fish Tales (L-5)
DRIVER(ft,d5)           //WPC-505:  12/92 W Fish Tales (D-5) LED Ghost Fix
DRIVER(dw,p5)           //WPC-206   10/92 B Doctor Who (P-5)
DRIVER(dw,p6)           //WPC-206   10/92 B Doctor Who (P-6) LED Ghost Fix
DRIVER(dw,l1)           //WPC-206   10/92 B Doctor Who (L-1)
DRIVER(dw,d1)           //WPC-206   10/92 B Doctor Who (D-1) LED Ghost Fix
DRIVER(dw,l2)           //WPC-206:  11/92 B Doctor Who (L-2)
DRIVER(dw,d2)           //WPC-206:  11/92 B Doctor Who (D-2) LED Ghost Fix
DRIVER(cftbl,l3)        //WPC-218:  01/93 B Creature from the Black Lagoon (L-3, Sound P-1)
DRIVER(cftbl,d3)        //WPC:      01/93 B Creature from the Black Lagoon (D-3, Sound P-1) LED Ghost Fix
DRIVER(cftbl,l4)        //          02/93 B Creature from the Black Lagoon (L-4)
DRIVER(cftbl,d4)        //WPC:      02/93 B Creature from the Black Lagoon (D-4) LED Ghost Fix
DRIVER(hshot,p8)        //WPC-617:  11/92 M Hot Shot Basketball (P-8)
DRIVER(hshot,p9)        //WPC-617:  11/92 M Hot Shot Basketball (P-9) LED Ghost Fix
DRIVER(ww,p1)           //WPC-518:  11/92 W White Water (P-8, P-1 sound)
DRIVER(ww,p2)           //WPC-518:  11/92 W White Water (P-9, P-1 sound) LED Ghost Fix
DRIVER(ww,p8)           //WPC-518:  11/92 W White Water (P-8, P-2 sound)
DRIVER(ww,p9)           //WPC-518:  11/92 W White Water (P-9, P-2 sound) LED Ghost Fix
DRIVER(ww,l2)           //WPC-518:  12/92 W White Water (L-2)
DRIVER(ww,d2)           //WPC-518:  12/92 W White Water (D-2) LED Ghost Fix
DRIVER(ww,l3)           //WPC-518:  01/93 W White Water (L-3)
DRIVER(ww,d3)           //WPC-518:  01/93 W White Water (D-3) LED Ghost Fix
DRIVER(ww,l4)           //WPC-518:  02/93 W White Water (L-4)
DRIVER(ww,d4)           //WPC-518:  02/93 W White Water (D-4) LED Ghost Fix
DRIVER(ww,l5)           //WPC-518:  05/93 W White Water (L-5)
DRIVER(ww,d5)           //WPC-518:  05/93 W White Water (D-5) LED Ghost Fix
DRIVER(ww,lh5)          //WPC-518:  10/00 W White Water (LH-5)
DRIVER(ww,lh6)          //WPC-518:  10/00 W White Water (LH-6)
DRIVER(drac,p11)        //WPC-501:  02/93 W Bram Stoker's Dracula (P-11)
DRIVER(drac,p12)        //WPC-501:  02/93 W Bram Stoker's Dracula (P-12) LED Ghost Fix
DRIVER(drac,l1)         //WPC-501:  02/93 W Bram Stoker's Dracula (L-1)
DRIVER(drac,d1)         //WPC-501:  02/93 W Bram Stoker's Dracula (D-1) LED Ghost Fix
DRIVER(tz,pa1)          //WPC-520:  03/93 B Twilight Zone (PA-1)
DRIVER(tz,pa2)          //WPC-520:  03/93 B Twilight Zone (PA-2) LED Ghost Fix
DRIVER(tz,p3)           //WPC-520:  04/93 B Twilight Zone (P-3)
DRIVER(tz,p3d)          //WPC-520:  04/93 B Twilight Zone (P-3) LED Ghost Fix
DRIVER(tz,p4)           //WPC-520:  04/93 B Twilight Zone (P-4)
DRIVER(tz,p5)           //WPC-520:  04/93 B Twilight Zone (P-5) LED Ghost Fix
DRIVER(tz,l1)           //WPC-520:  04/93 B Twilight Zone (L-1)
DRIVER(tz,d1)           //WPC-520:  04/93 B Twilight Zone (D-1) LED Ghost Fix
DRIVER(tz,l2)           //WPC-520:  05/93 B Twilight Zone (L-2)
DRIVER(tz,d2)           //WPC-520:  05/93 B Twilight Zone (D-2) LED Ghost Fix
DRIVER(tz,l3)           //WPC-520:  05/93 B Twilight Zone (L-3)
DRIVER(tz,d3)           //WPC-520:  05/93 B Twilight Zone (D-3) LED Ghost Fix
DRIVER(tz,ifpa)         //WPC-520:  05/93 B Twilight Zone (IFPA)
DRIVER(tz,ifpa2)        //WPC-520:  05/93 B Twilight Zone (IFPA) LED Ghost Fix
DRIVER(tz,l4)           //WPC-520:  06/93 B Twilight Zone (L-4)
DRIVER(tz,d4)           //WPC-520:  06/93 B Twilight Zone (D-4) LED Ghost Fix
DRIVER(tz,h7)           //WPC-520:  10/94 B Twilight Zone (H-7)
DRIVER(tz,i7)           //WPC-520:  10/94 B Twilight Zone (I-7) LED Ghost Fix
DRIVER(tz,h8)           //WPC-520:  11/94 B Twilight Zone (H-8)
DRIVER(tz,i8)           //WPC-520:  11/94 B Twilight Zone (I-8) LED Ghost Fix
DRIVER(tz,92)           //WPC-520:  01/95 B Twilight Zone (9.2)
DRIVER(tz,93)           //WPC-520:  01/95 B Twilight Zone (9.3) LED Ghost Fix
DRIVER(tz,94h)          //WPC-520:  10/98 B Twilight Zone (9.4H - Home version)
DRIVER(tz,94ch)         //WPC-520:  10/98 B Twilight Zone (9.4CH - Home Coin version)
DRIVER(tz,f86)          //WPC-520:  01/95 B Twilight Zone (FreeWPC 0.86)
DRIVER(ij,l3)           //WPC-517:  07/93 W Indiana Jones (L-3)
DRIVER(ij,d3)           //WPC-517:  07/93 W Indiana Jones (D-3) LED Ghost Fix
DRIVER(ij,l4)           //WPC-517:  08/93 W Indiana Jones (L-4)
DRIVER(ij,d4)           //WPC-517:  08/93 W Indiana Jones (D-4) LED Ghost Fix
DRIVER(ij,l5)           //WPC-517:  09/93 W Indiana Jones (L-5)
DRIVER(ij,d5)           //WPC-517:  09/93 W Indiana Jones (D-5) LED Ghost Fix
DRIVER(ij,l6)           //WPC-517:  10/93 W Indiana Jones (L-6)
DRIVER(ij,d6)           //WPC-517:  10/93 W Indiana Jones (D-6) LED Ghost Fix
DRIVER(ij,l7)           //WPC-517:  11/93 W Indiana Jones (L-7)
DRIVER(ij,d7)           //WPC-517:  11/93 W Indiana Jones (D-7) LED Ghost Fix
DRIVER(ij,lg7)          //WPC-517:  11/93 W Indiana Jones (LG-7)
DRIVER(ij,dg7)          //WPC-517:  11/93 W Indiana Jones (DG-7) LED Ghost Fix
DRIVER(jd,l1)           //WPC-220:  10/93 B Judge Dredd (L-1)
DRIVER(jd,d1)           //WPC-220:  10/93 B Judge Dredd (D-1) LED Ghost Fix
DRIVER(jd,l6)           //WPC-220:  10/93 B Judge Dredd (L-6)
DRIVER(jd,d6)           //WPC-220:  10/93 B Judge Dredd (D-6) LED Ghost Fix
DRIVER(jd,l7)           //WPC-220:  12/93 B Judge Dredd (L-7)
DRIVER(jd,d7)           //WPC-220:  12/93 B Judge Dredd (D-7) LED Ghost Fix
DRIVER(ij,h1)           //WPC-517:  11/93 W Indiana Jones (HK-1) Family no hate speech version
DRIVER(ij,i1)           //WPC-517:  11/93 W Indiana Jones (I-1) Family no hate LED Ghost Fix
DRIVER(afv,l4)          //WPC-622:  ??/93 W Addams Family Values (L-4 Redemption)
DRIVER(afv,d4)          //WPC-622:  ??/93 W Addams Family Values (D-4 Redemption) LED Ghost Fix
DRIVER(sttng,p5)        //WPC-523:  11/93 W Star Trek: The Next Generation (P-5)
DRIVER(sttng,p6)        //WPC-523:  11/93 W Star Trek: The Next Generation (P-6) LED Ghost Fix
DRIVER(sttng,l1)        //WPC-523:  11/93 W Star Trek: The Next Generation (LX-1)
DRIVER(sttng,d1)        //WPC-523:  11/93 W Star Trek: The Next Generation (DX-1) LED Ghost Fix
DRIVER(sttng,l2)        //WPC-523:  12/93 W Star Trek: The Next Generation (LX-2)
DRIVER(sttng,d2)        //WPC-523:  12/93 W Star Trek: The Next Generation (DX-2) LED Ghost Fix
DRIVER(sttng,l7)        //WPC-523:  02/94 W Star Trek: The Next Generation (LX-7)
DRIVER(sttng,d7)        //WPC-523:  02/94 W Star Trek: The Next Generation (DX-7) LED Ghost Fix
DRIVER(sttng,x7)        //WPC-523:  02/94 W Star Trek: The Next Generation (LX-7 Special)
DRIVER(sttng,dx)        //WPC-523:  02/94 W Star Trek: The Next Generation (DX-7 Special) LED Ghost Fix
DRIVER(sttng,s7)        //WPC-523:  02/94 W Star Trek: The Next Generation (LX-7) SP1
DRIVER(sttng,ds)        //WPC-523:  02/94 W Star Trek: The Next Generation (DX-7) SP1 LED Ghost Fix
DRIVER(sttng,g7)        //WPC-523:  02/94 W Star Trek: The Next Generation (LG-7)
DRIVER(sttng,h7)        //WPC-523:  02/94 W Star Trek: The Next Generation (HG-7) LED Ghost Fix
DRIVER(pop,pa3)         //WPC-522:  12/93 B Popeye Saves the Earth (PA-3)
DRIVER(pop,pa4)         //WPC-522:  12/93 B Popeye Saves the Earth (PA-4) LED Ghost Fix
DRIVER(pop,lx5)         //WPC:      02/94 B Popeye Saves the Earth (LX-5)
DRIVER(pop,dx5)         //WPC:      02/94 B Popeye Saves the Earth (DX-5) LED Ghost Fix
DRIVER(dm,pa2)          //WPC-528:  03/94 W Demolition Man (PA-2)
DRIVER(dm,pa3)          //WPC-528:  03/94 W Demolition Man (PA-3) LED Ghost Fix
DRIVER(dm,px5)          //WPC-528:  04/94 W Demolition Man (PX-5)
DRIVER(dm,px6)          //WPC-528:  04/94 W Demolition Man (PX-6) LED Ghost Fix
DRIVER(dm,la1)          //WPC-528:  04/94 W Demolition Man (LA-1)
DRIVER(dm,da1)          //WPC-528:  04/94 W Demolition Man (DA-1) LED Ghost Fix
DRIVER(dm,lx3)          //WPC-528:  04/94 W Demolition Man (LX-3)
DRIVER(dm,dx3)          //WPC-528:  04/94 W Demolition Man (DX-3) LED Ghost Fix
DRIVER(dm,lx4)          //WPC-528:  05/94 W Demolition Man (LX-4)
DRIVER(dm,dx4)          //WPC-528:  05/94 W Demolition Man (DX-4) LED Ghost Fix
DRIVER(dm,h5)           //WPC-528:  02/95 W Demolition Man (H-5) with rude speech
DRIVER(dm,h5b)          //WPC-528:  02/95 W Demolition Man (H-5) with rude speech (Coin Play)
DRIVER(dm,dh5)          //WPC-528:  02/95 W Demolition Man (DH-5) with rude speech LED Ghost Fix
DRIVER(dm,dh5b)         //WPC-528:  02/95 W Demolition Man (DH-5) with rude speech LED Ghost Fix (Coin Play)
DRIVER(dm,h6)           //WPC-528:  02/95 W Demolition Man (H-6) with rude speech
DRIVER(dm,h6b)          //WPC-528:  02/95 W Demolition Man (H-6) with rude speech (Coin Play)
DRIVER(tafg,lx3)        //WPC:      10/94 B The Addams Family Special Collectors Edition (LX-3)
DRIVER(tafg,dx3)        //WPC:      10/94 B The Addams Family Special Collectors Edition (DX-3) LED Ghost Fix
DRIVER(tafg,la2)        //WPC:      10/94 B The Addams Family Special Collectors Edition (LA-2)
DRIVER(tafg,da2)        //WPC:      10/94 B The Addams Family Special Collectors Edition (DA-2) LED Ghost Fix
DRIVER(tafg,la3)        //WPC:      10/94 B The Addams Family Special Collectors Edition (LA-3)
DRIVER(tafg,da3)        //WPC:      10/94 B The Addams Family Special Collectors Edition (DA-3) LED Ghost Fix
DRIVER(tafg,h3)         //                  Home version
DRIVER(tafg,i3)         //                  Home version LED Ghost Fix
DRIVER(wcs,l2)          //WPC-531:  05/94 B World Cup Soccer (Lx-2)
DRIVER(wcs,d2)          //WPC-531:  05/94 B World Cup Soccer (Dx-2) LED Ghost Fix
DRIVER(wcs,p2)          //                  (Pa-2)
DRIVER(wcs,p5)          //                  (Pa-5) LED Ghost Fix
DRIVER(wcs,p3)          //                  (Px-3)
DRIVER(wcs,p6)          //                  (Px-6) LED Ghost Fix
DRIVER(wcs,f62)         //                  World Cup Soccer (FreeWPC 0.62)
                        //WPC-620:  06/94 W Pinball Circus
DRIVER(fs,lx2)          //WPC-529:  07/94 W The Flintstones (LX-2)
DRIVER(fs,dx2)          //WPC-529:  07/94 W The Flintstones (DX-2) LED Ghost Fix
DRIVER(fs,sp2)          //WPC-529:  07/94 W The Flintstones (SP-2)
DRIVER(fs,sp2d)         //WPC-529:  07/94 W The Flintstones (SP-2D) LED Ghost Fix
DRIVER(fs,lx4)          //WPC-529:  09/94 W The Flintstones (LX-4)
DRIVER(fs,dx4)          //WPC-529:  09/94 W The Flintstones (DX-4) LED Ghost Fix
DRIVER(fs,lx5)          //WPC-529:  11/94 W The Flintstones (LX-5)
DRIVER(fs,dx5)          //WPC-529:  11/94 W The Flintstones (DX-5) LED Ghost Fix
DRIVER(corv,px4)        //WPC-536:  08/94 B Corvette (PX4)
DRIVER(corv,px5)        //WPC-536:  08/94 B Corvette (PX5) LED Ghost Fix
DRIVER(corv,lx1)        //          09/94 B Corvette (LX1)
DRIVER(corv,dx1)        //WPC-536:  08/94 B Corvette (DX1) LED Ghost Fix
DRIVER(corv,21)         //          01/96 B Corvette (2.1)
DRIVER(rs,l6)           //WPC-524:  10/94 W Red & Ted's Road Show (L_6)
DRIVER(rs,lx2)          //WPC-524:  10/94 W Red & Ted's Road Show (Lx_2)
DRIVER(rs,dx2)          //WPC-524:  10/94 W Red & Ted's Road Show (Dx_2) LED Ghost Fix
DRIVER(rs,lx3)          //WPC-524:  10/94 W Red & Ted's Road Show (Lx_3)
DRIVER(rs,dx3)          //WPC-524:  10/94 W Red & Ted's Road Show (Dx_3) LED Ghost Fix
DRIVER(rs,la4)          //WPC-524:  10/94 W Red & Ted's Road Show (La_4)
DRIVER(rs,da4)          //WPC-524:  10/94 W Red & Ted's Road Show (Da_4) LED Ghost Fix
DRIVER(rs,lx4)          //WPC-524:  10/94 W Red & Ted's Road Show (Lx-4)
DRIVER(rs,dx4)          //WPC-524:  10/94 W Red & Ted's Road Show (Dx-4) LED Ghost Fix
DRIVER(rs,la5)          //WPC-524:  10/94 W Red & Ted's Road Show (La_5)
DRIVER(rs,da5)          //WPC-524:  10/94 W Red & Ted's Road Show (Da_5) LED Ghost Fix
DRIVER(rs,lx5)          //WPC-524:  10/94 W Red & Ted's Road Show (Lx_5)
DRIVER(rs,dx5)          //WPC-524:  10/94 W Red & Ted's Road Show (Dx_5) LED Ghost Fix
DRIVER(ts,pa1)          //WPC-532:  11/94 B The Shadow (PA-1)
DRIVER(ts,pa2)          //WPC-532:  11/94 B The Shadow (PA-2) LED Ghost Fix
DRIVER(ts,la2)          //WPC-532:  12/94 B The Shadow (LA-2)
DRIVER(ts,da2)          //WPC-532:  12/94 B The Shadow (DA-2) LED Ghost Fix
DRIVER(ts,la4)          //WPC-532:  02/95 B The Shadow (LA-4)
DRIVER(ts,da4)          //WPC-532:  02/95 B The Shadow (DA-4) LED Ghost Fix
DRIVER(ts,lx4)          //WPC-532:  02/95 B The Shadow (LX-4)
DRIVER(ts,dx4)          //WPC-532:  02/95 B The Shadow (DX-4) LED Ghost Fix
DRIVER(ts,lx5)          //WPC-532:  05/95 B The Shadow (LX-5)
DRIVER(ts,dx5)          //WPC-532:  05/95 B The Shadow (DX-5) LED Ghost Fix
DRIVER(ts,lh6)          //WPC-532:  05/95 B The Shadow (LH-6, Home version)
DRIVER(ts,dh6)          //WPC-532:  05/95 B The Shadow (DH-6, Home version) LED Ghost Fix
DRIVER(ts,lf6)          //WPC-532:  05/95 B The Shadow (LF-6) French
DRIVER(ts,df6)          //WPC-532:  05/95 B The Shadow (DF-6) French LED Ghost Fix
DRIVER(ts,lm6)          //WPC-532:  05/95 B The Shadow (LM-6) Mild
DRIVER(ts,dm6)          //WPC-532:  05/95 B The Shadow (DM-6) Mild LED Ghost Fix
DRIVER(dh,lx2)          //WPC-530:  01/95 W Dirty Harry (LX-2)
DRIVER(dh,dx2)          //WPC-530:  03/95 W Dirty Harry (DX-2) LED Ghost Fix
DRIVER(tom,06)          //WPC-539:  03/95 B Theatre of Magic (0.6A)
DRIVER(tom,061)         //WPC-539:  03/95 B Theatre of Magic (0.61A) LED Ghost Fix
DRIVER(tom,10f)		//WPC-539:  04/95 B Theatre of Magic (1.0 French)
DRIVER(tom,101f)	//WPC-539:  04/95 B Theatre of Magic (1.01 French) LED Ghost Fix
DRIVER(tom,12)          //WPC-539:  04/95 B Theatre of Magic (1.2X)
DRIVER(tom,121)         //WPC-539:  04/95 B Theatre of Magic (1.21X) LED Ghost Fix
DRIVER(tom,13)          //          08/95 B Theatre of Magic (1.3X)
DRIVER(tom,14h)         //          10/96 B Theatre of Magic (1.4 Home version)
DRIVER(tom,14hb)        //          10/96 B Theatre of Magic (1.4 Home version Coin Play)
DRIVER(nf,20)           //WPC-525:  05/95 W No Fear: Dangerous Sports (2.0)
DRIVER(nf,22)           //WPC-525:  05/95 W No Fear: Dangerous Sports (2.2)
DRIVER(nf,23)           //WPC-525:  05/95 W No Fear: Dangerous Sports (2.3)
DRIVER(nf,23f)          //WPC-525:  05/95 W No Fear: Dangerous Sports (2.3F)
DRIVER(nf,23x)          //WPC-525:  05/95 W No Fear: Dangerous Sports (2.3 Export)
DRIVER(i500,11r)        //WPC-526:  06/95 B Indianapolis 500
DRIVER(i500,11b)        //          06/95 B Indianapolis 500 (Belgium)
DRIVER(jb,10r)          //WPC-551:  10/95 W Jack*Bot
DRIVER(jb,101r)         //WPC-551:  10/95 W Jack*Bot LED Ghost Fix
DRIVER(jb,10b)          //                  Belgium/Canada
DRIVER(jb,101b)         //                  Belgium/Canada LED Ghost Fix
DRIVER(jm,12r)          //WPC-542:  08/95 W Johnny Mnemonic
DRIVER(jm,12b)          //WPC-542:  08/95 W Johnny Mnemonic (Belgium)
DRIVER(wd,03r)          //WPC-544:  09/95 B Who dunnit (0.3 R)
DRIVER(wd,048r)         //          10/95 B Who dunnit (0.48 R)
DRIVER(wd,10r)          //          11/95 B Who dunnit (1.0 R)
DRIVER(wd,10g)          //          11/95 B Who dunnit (1.0 German)
DRIVER(wd,10f)          //          11/95 B Who dunnit (1.0 French)
DRIVER(wd,11)           //         ?04/96 B Who dunnit (1.1)
DRIVER(wd,12)           //         ?05/96 B Who dunnit (1.2)
DRIVER(wd,12g)          //         ?05/96 B Who dunnit (1.2 German)
DRIVER(congo,20)        //WPC-550:  02/96 W Congo (2.0)
DRIVER(congo,21)        //          10/96 W Congo (2.1)
DRIVER(afm,10)          //WPC-541:  12/95 B Attack from Mars (1.0)
DRIVER(afm,11)          //WPC-541:  12/95 B Attack from Mars (1.1)
DRIVER(afm,11u)         //WPC-541:  12/95 B Attack from Mars (1.1 Ultrapin)
DRIVER(afm,113)         //WPC-541:  12/95 B Attack from Mars (1.13 Home version)
DRIVER(afm,113b)        //WPC-541:  12/95 B Attack from Mars (1.13b Coin Play)
DRIVER(afm,f32)         //WPC-541:  12/95 B Attack from Mars (FreeWPC 0.32)
DRIVER(lc,11)           //WPC-107:  03/96 B League Champ (Shuffle Alley)
DRIVER(ttt,10)          //WPC-905:  03/96 W Ticket Tac Toe
DRIVER(sc,14)           //WPC-903:  06/96 B Safe Cracker (1.4)
DRIVER(sc,17)           //          11/96 B Safe Cracker (1.7)
DRIVER(sc,17n)          //          11/96 B Safe Cracker (1.7, alternate version)
DRIVER(sc,18)           //          04/98 B Safe Cracker (1.8)
DRIVER(sc,18n)          //          04/98 B Safe Cracker (1.8, alternate version)
DRIVER(sc,18s2)         //          04/98 B Safe Cracker (1.8, alternate sound)
DRIVER(totan,04)        //WPC-547:  05/96 W Tales of the Arabian Nights (0.4)
DRIVER(totan,12)        //WPC-547:  06/96 W Tales of the Arabian Nights (1.2)
DRIVER(totan,13)        //          07/96 W Tales of the Arabian Nights (1.3)
DRIVER(totan,14)        //          10/96 W Tales of the Arabian Nights (1.4)
DRIVER(ss,01)           //WPC-548:  09/96 B Scared Stiff (D0.1R with sound rev.25)
DRIVER(ss,01b)          //          09/96 B Scared Stiff (D0.1R with sound rev.25 Coin Play)
DRIVER(ss,03)           //          09/96 B Scared Stiff (0.3)
DRIVER(ss,12)           //          10/96 B Scared Stiff (1.2)
DRIVER(ss,14)           //          11/96 B Scared Stiff (1.4)
DRIVER(ss,15)           //          02/97 B Scared Stiff (1.5)
DRIVER(jy,03)           //WPC-552:  10/96 W Junk Yard (0.3)
DRIVER(jy,11)           //          01/97 W Junk Yard (1.1)
DRIVER(jy,12)           //          07/97 W Junk Yard (1.2)
DRIVER(nbaf,11s)        //WPC-553:  03/97 B NBA Fastbreak (1.1 - S0.4)
DRIVER(nbaf,11)         //          03/97 B NBA Fastbreak (1.1)
DRIVER(nbaf,11a)        //          03/97 B NBA Fastbreak (1.1 - S2.0)
DRIVER(nbaf,115)        //          05/97 B NBA Fastbreak (1.15)
DRIVER(nbaf,21)         //          05/97 B NBA Fastbreak (2.1)
DRIVER(nbaf,22)         //          05/97 B NBA Fastbreak (2.2)
DRIVER(nbaf,23)         //          06/97 B NBA Fastbreak (2.3)
DRIVER(nbaf,31)         //          09/97 B NBA Fastbreak (3.1)
DRIVER(nbaf,31a)        //          09/97 B NBA Fastbreak (3.1a)
DRIVER(mm,05)           //WPC-559:  06/97 W Medieval Madness (0.5)
DRIVER(mm,10)           //          07/97 W Medieval Madness (1.0)
DRIVER(mm,10u)          //WPC-559:  07/97 W Medieval Madness (1.0 Ultrapin)
DRIVER(mm,109)          //          06/99 W Medieval Madness (1.09, Home version)
DRIVER(mm,109b)         //          06/99 W Medieval Madness (1.09B, Home version Coin Play)
DRIVER(mm,109c)         //          06/99 W Medieval Madness (1.09C, Home version w/ profanity speech)
DRIVER(cv,10)           //WPC-562:  10/97 B Cirqus Voltaire (1.0)
DRIVER(cv,11)           //          11/97 B Cirqus Voltaire (1.1)
DRIVER(cv,13)           //          04/98 B Cirqus Voltaire (1.3)
DRIVER(cv,14)           //          10/98 B Cirqus Voltaire (1.4)
DRIVER(cv,20h)          //          02/02 B Cirqus Voltaire (Home version)
DRIVER(ngg,p06)         //WPC-561:  10/97 W No Good Gofers (Prototype 0.6)
DRIVER(ngg,10)          //          12/97 W No Good Gofers (1.0)
DRIVER(ngg,13)          //          04/98 W No Good Gofers (1.3)
DRIVER(cp,15)           //WPC-563:  07/98 B The Champion Pub (1.5)
DRIVER(cp,16)           //          07/98 B The Champion Pub (1.6)
DRIVER(mb,10)           //WPC-565:  08/98 W Monster Bash
DRIVER(mb,106)          //          06/99 W Monster Bash (Home version)
DRIVER(mb,106b)         //          06/99 W Monster Bash (1.06b Coin Play)
DRIVER(cc,12)           //WPC-566:  02/99 B Cactus Canyon (1.2)
DRIVER(cc,13)           //          04/99 B Cactus Canyon (1.3)
DRIVER(cc,13k)		//          04/99 B Cactus Canyon (1.3) with real knocker
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
DRIVERNV(sshtlzac)      //09/80 Space Shuttle
DRIVERNV(ewf)           //04/81 Earth, Wind & Fire
DRIVERNV(locomotn)      //09/81 Locomotion
                        //04/82 Pinball Champ '82 (using the same roms as the '83 version)
DRIVERNV(socrking)      //09/82 Soccer Kings
DRIVERNV(socrkngi)      //      Soccer Kings (Italian speech)
DRIVERNV(socrkngg)      //      Soccer Kings (German speech)
DRIVERNV(sockfp)        //09/82 Soccer Kings (Free Play)
DRIVERNV(sockifp)       //      Soccer Kings (Italian speech Free Play)
DRIVERNV(sockgfp)       //      Soccer Kings (German speech Free Play)
DRIVERNV(pinchamp)      //04/83 Pinball Champ
DRIVERNV(pinchamg)      //      Pinball Champ (German speech)
DRIVERNV(pinchami)      //      Pinball Champ (Italian speech)
DRIVERNV(pincham7)      //      Pinball Champ (7 digits)
DRIVERNV(pincha7g)      //      Pinball Champ (7 digits, German speech)
DRIVERNV(pincha7i)      //      Pinball Champ (7 digits, Italian speech)
DRIVERNV(pincfp)        //04/83 Pinball Champ (Free Play)
DRIVERNV(pincgfp)       //      Pinball Champ (German speech Free Play)
DRIVERNV(pincifp)       //      Pinball Champ (Italian speech Free Play)
DRIVERNV(pinc7fp)       //      Pinball Champ (7 digits Free Play)
DRIVERNV(pinc7gfp)      //      Pinball Champ (7 digits, German speech Free Play)
DRIVERNV(pinc7ifp)      //      Pinball Champ (7 digits, Italian speech Free Play)
DRIVERNV(tmachzac)      //04/83 Time Machine
DRIVERNV(tmacgzac)      //      Time Machine (German speech)
DRIVERNV(tmacfzac)      //      Time Machine (French speech)
DRIVERNV(tmachfp)       //04/83 Time Machine (Free Play)
DRIVERNV(tmacgfp)       //      Time Machine (German speech Free Play)
DRIVERNV(tmacffp)       //      Time Machine (French speech Free Play)
DRIVERNV(farfalla)      //09/83 Farfalla
DRIVERNV(farfalli)      //      Farfalla (Italian speech)
DRIVERNV(farfallg)      //      Farfalla (German speech)
DRIVERNV(farffp)        //09/83 Farfalla (Free Play)
DRIVERNV(farfifp)       //      Farfalla (Italian speech Free Play)
DRIVERNV(farfgfp)       //      Farfalla (German speech Free Play)
DRIVERNV(dvlrider)      //04/84 Devil Riders
DRIVERNV(dvlridei)      //      Devil Riders (Italian speech)
DRIVERNV(dvlrideg)      //      Devil Riders (German speech)
DRIVERNV(dvlrdfp)       //04/84 Devil Riders (Free Play)
DRIVERNV(dvlrdifp)      //      Devil Riders (Italian speech Free Play))
DRIVERNV(dvlrdgfp)      //      Devil Riders (German speech Free Play)
DRIVERNV(mcastle)       //09/84 Magic Castle
DRIVERNV(mcastlei)      //      Magic Castle (Italian speech)
DRIVERNV(mcastleg)      //      Magic Castle (German speech)
DRIVERNV(mcastlef)      //      Magic Castle (French speech)
DRIVERNV(mcastfp)       //09/84 Magic Castle (Free Play)
DRIVERNV(mcastifp)      //      Magic Castle (Italian speech Free Play)
DRIVERNV(mcastgfp)      //      Magic Castle (German speech Free Play)
DRIVERNV(mcastffp)      //      Magic Castle (French speech Free Play)
DRIVERNV(robot)         //01/85 Robot
DRIVERNV(roboti)        //      Robot (Italian speech)
DRIVERNV(robotg)        //      Robot (German speech)
DRIVERNV(robotf)        //      Robot (French speech)
DRIVERNV(robotfp)       //01/85 Robot (Free Play)
DRIVERNV(robotifp)      //      Robot (Italian speech Free Play)
DRIVERNV(robotgfp)      //      Robot (German speech Free Play)
DRIVERNV(robotffp)      //      Robot (French speech Free Play)
DRIVERNV(clown)         //07/85 Clown
DRIVERNV(clownfp)       //07/85 Clown (Free Play)
DRIVERNV(poolcham)      //12/85 Pool Champion
DRIVERNV(poolchai)      //      Pool Champion (Italian speech)
DRIVERNV(poolchap)      //      Pool Champion (alternate sound)
DRIVERNV(poolcfp)       //12/85 Pool Champion (Free Play)
DRIVERNV(poolcifp)      //      Pool Champion (Italian speech Free Play)
DRIVERNV(poolcafp)      //      Pool Champion (alternate sound Free Play)
DRIVERNV(myststar)      //??/86 Mystic Star
DRIVERNV(bbeltzac)      //03/86 Blackbelt
DRIVERNV(bbeltzfp)      //03/86 Blackbelt (Free Play)
DRIVERNV(mexico)        //07/86 Mexico '86
DRIVERNV(mexicofp)      //07/86 Mexico '86 (Free Play)
DRIVERNV(zankor)        //12/86 Zankor
DRIVERNV(zankorfp)      //12/86 Zankor (Free Play)
DRIVERNV(spooky)        //04/87 Spooky
DRIVERNV(spookyi)       //      Spooky (Italian speech)
DRIVERNV(spookyfp)      //04/87 Spooky (Free Play)
DRIVERNV(spookifp)      //      Spooky (Italian speech Free Play)
DRIVERNV(strsphnx)      //07/87 Star's Phoenix
DRIVERNV(strsphfp)      //07/87 Star's Phoenix (Free Play)
DRIVERNV(nstrphnx)      //08/87 New Star's Phoenix (same roms as strsphnx)
DRIVERNV(nstrphfp)      //08/87 New Star's Phoenix (same roms as strsphnx Free Play)

#endif /* DRIVER_RECURSIVE */
