#ifndef DRIVER_RECURSIVE
#  define DRIVER_RECURSIVE
#  include "driver.h"

const struct GameDriver driver_0 = {
 __FILE__, 0, "", 0, 0, 0, 0, 0, 0, 0, NOT_A_DRIVER
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
#else /* DRIVER_RECURSIVE */


// ---------------------
// BALLY GAMES BELOW
// ---------------------
DRIVERNV(freedom )      //BY17-720: 08/76 Freedom
DRIVERNV(nightrdr)      //BY17-721: 01/76 Night Rider (EM release date)
DRIVERNV(blackjck)      //BY17-728: 05/76 Black Jack  (EM release date)
DRIVERNV(evelknie)      //BY17-722: 09/76 Evel Knievel
DRIVERNV(matahari)      //BY17-725: 09/77 Mata Hari
                        //??        10/76 Fireball
                        //??        10/76 Star Ship
DRIVERNV(sst     )      //BY35-741: 10/76 Supersonic
DRIVERNV(eightbll)      //BY17-723: 01/77 Eight Ball
DRIVERNV(pwerplay)      //BY17-724: 02/77 Power Play
DRIVERNV(stk_sprs)      //BY17-740: 08/77 Strikes and Spares
DRIVERNV(lostwrld)      //BY35-729: 02/77 Lost World
DRIVERNV(smman   )      //BY35-742: 08/77 The Six Million Dollar Man
DRIVERNV(playboy )      //BY35-743: 09/76 Playboy
                        //??          /78 Big Foot
                        //??          /78 Galaxy
DRIVERNV(startrek)      //BY35-745: 01/78 Star Trek
DRIVERNV(voltan  )      //BY35-744: 01/78 Voltan Escapes Cosmic Doom
                        //??        02/78 Skateball
DRIVERNV(paragon )      //BY35-748: 12/78 Paragon
DRIVERNV(hglbtrtr)      //BY35-750: 08/78 Harlem Globetrotters On Tour
DRIVERNV(dollyptn)      //BY35-777: 10/78 Dolly Parton
DRIVERNV(kiss    )      //BY35-746: 04/78 Kiss
DRIVERNV(futurspa)      //BY35-781: 03/79 Future Spa
DRIVERNV(spaceinv)      //BY35-792: 05/79 Space Invaders
DRIVERNV(ngndshkr)      //BY35-776: 05/78 Nitro Ground Shaker
DRIVERNV(slbmania)      //BY35-786: 06/78 Silverball Mania
DRIVERNV(rollston)      //BY35-796: 06/79 Rolling Stones
DRIVERNV(mystic  )      //BY35-798: 08/79 Mystic
DRIVERNV(hotdoggn)      //BY35-809: 12/79 Hotdoggin'
DRIVERNV(viking  )      //BY35-802: 12/79 Viking
DRIVERNV(skatebll)      //BY35-823: 04/80 Skateball
DRIVERNV(frontier)      //BY35-819: 05/80 Frontier
DRIVERNV(xenon   )      //BY35-811: 11/79 Xenon
DRIVERNV(xenonf  )      //BY35-811: 11/79 Xenon (French)
DRIVERNV(flashgdn)      //BY35-834: 05/80 Flash Gordon
DRIVERNV(flashgdf)      //BY35-834: 05/80 Flash Gordon (French)
DRIVERNV(eballdlx)      //BY35-838: 09/80 Eight Ball Deluxe
DRIVERNV(fball_ii)      //BY35-839: 09/80 Fireball II
DRIVERNV(embryon )      //BY35-841: 09/80 Embryon
DRIVERNV(fathom  )      //BY35-842: 12/80 Fathom
DRIVERNV(medusa  )      //BY35-845: 02/81 Medusa
DRIVERNV(centaur )      //BY35-848: 02/81 Centaur
DRIVERNV(elektra )      //BY35-857: 03/81 Elektra
DRIVERNV(vector  )      //BY35-858: 03/81 Vector
DRIVERNV(spectrum)      //BY35-868: 04/81 Spectrum
DRIVERNV(spectru4)      //BY35-868: 04/81 Spectrum (rel 4)
DRIVERNV(speakesy)      //BY35-877: 08/82 Speakeasy
DRIVERNV(speakes4)      //BY35-877: 08/82 Speakeasy 4 (4 player)
DRIVERNV(rapidfir)      //BY35-869: 06/81 Rapid Fire
DRIVERNV(m_mpac  )      //BY35-872: 05/82 Mr. & Mrs. Pac-Man
                        //??        10/82 Baby Pac-Man
/*same as eballdlx*/    //BY35      10/82 Eight Ball Deluxe Limited Edition
DRIVERNV(bmx     )      //BY35-888: 11/82 BMX
DRIVERNV(granslam)      //BY35-     01/83 Grand Slam
/*same as cenatur*/     //BY35      06/83 Centaur II
DRIVERNV(goldball)      //BY35-     10/83 Gold Ball
DRIVERNV(xsandos )      //BY35-     12/83 X's & O's
                        //??        ??/84 Mysterian
                        //BY35-     01/84 Granny and the Gators
DRIVERNV(kosteel )      //BY35-     05/84 Kings of Steel
DRIVERNV(spyhuntr)      //BY35-     10/84 Spy Hunter
DRIVERNV(blakpyra)      //BY35-     07/84 Black Pyramid
                        //??        ??/85 Hot Shotz
DRIVERNV(fbclass )      //BY35-     02/85 Fireball Classic
DRIVERNV(cybrnaut)      //BY35-     05/85 Cybernaut
                        //BY??      09/85 Eight Ball Champ
//DRIVERNV(atlantis)    //6803-2006: 03/89 Atlantis

// ---------------------
// GOTTLIEB GAMES BELOW
// ---------------------
DRIVERNV(spidermn)    //System 80: Spiderman
DRIVERNV(panthera)    //System 80: Panthera
DRIVERNV(circus)      //System 80: Circus
DRIVERNV(cntforce)    //System 80: Counterforce
DRIVERNV(starrace)    //System 80: Star Race
DRIVERNV(jamesb)      //System 80: James Bond (Timed Play)
DRIVERNV(jamesb2)     //System 80: James Bond (3/5 Ball)
DRIVERNV(timeline)    //System 80: Time Line
DRIVERNV(forceii)     //System 80: Force II
DRIVERNV(pnkpnthr)    //System 80: Pink Panther
DRIVERNV(mars)        //System 80: Mars God of War
DRIVERNV(vlcno_ax)    //System 80: Volcano (Sound & Speech)
DRIVERNV(vlcno_1b)    //System 80: Volcano (Sound Only)
DRIVERNV(blckhole)    //System 80: Black Hole (Sound & Speech, Rev 4)
DRIVERNV(blkhole2)    //System 80: Black Hole (Sound & Speech, Rev 2)
DRIVERNV(blkholea)    //System 80: Black Hole (Sound Only)
DRIVERNV(hh)          //System 80: Haunted House (Rev 2)
DRIVERNV(hh_1)        //System 80: Haunted House (Rev 1)
DRIVERNV(eclipse)     //System 80: Eclipse
DRIVERNV(s80tst)      //System 80: Text Fixture

DRIVERNV(dvlsdre)     //System 80a: Devils Dare (Sound & Speech)
DRIVERNV(dvlsdre2)    //System 80a: Devils Dare (Sound Only)
DRIVERNV(caveman)     //System 80a: Caveman
DRIVERNV(rocky)       //System 80a: Rocky
DRIVERNV(spirit)      //System 80a: Spirit
DRIVERNV(punk)        //System 80a: Punk
DRIVERNV(striker)     //System 80a: Striker
DRIVERNV(krull)       //System 80a: Krull
DRIVERNV(qbquest)     //System 80a: Q*bert's Quest
DRIVERNV(sorbit)      //System 80a: Super Orbit
DRIVERNV(rflshdlx)    //System 80a: Royal Flush Deluxe
DRIVERNV(goinnuts)    //System 80a: Goin' Nuts
DRIVERNV(amazonh)     //System 80a: Amazon Hunt
DRIVERNV(rackemup)    //System 80a: Rack 'Em Up
DRIVERNV(raimfire)    //System 80a: Ready Aim Fire
DRIVERNV(jack2opn)    //System 80a: Jacks to Open
DRIVERNV(alienstr)    //System 80a: Alien Star
DRIVERNV(thegames)    //System 80a: The Games
DRIVERNV(touchdn)     //System 80a: Touchdown
DRIVERNV(eldorado)    //System 80a: El Dorado
DRIVERNV(icefever)    //System 80a: Ice Fever

DRIVERNV(triplay)     //System 80b: Chicago Cubs Triple Play
DRIVERNV(bountyh)     //System 80b: Bounty Hunter
                      //System 80b: Tag Team
DRIVERNV(rock)        //System 80b: Rock
                      //System 80b: Rock Encore
DRIVERNV(raven)       //System 80b: Raven
DRIVERNV(hlywoodh)    //System 80b: Hollywood Heat
DRIVERNV(genesis)     //System 80b: Genesis
DRIVERNV(goldwing)    //System 80b: Gold Wings
DRIVERNV(mntecrlo)    //System 80b: Monte Carlo
DRIVERNV(sprbreak)    //System 80b: Spring Break
                      //System 80b: Amazon Hunt II
DRIVERNV(arena)       //System 80b: Arena
DRIVERNV(victory)     //System 80b: Victory
DRIVERNV(diamond)     //System 80b: Diamond Lady
DRIVERNV(txsector)    //System 80b: TX Sector
                      //System 80b: Amazon Hunt III
DRIVERNV(robowars)    //System 80b: Robo-War
DRIVERNV(excalibr)    //System 80b: Excalibur
DRIVERNV(badgirls)    //System 80b: Bad Girls
                      //System 80b: Hot Shots
DRIVERNV(bighouse)    //System 80b: Big House
DRIVERNV(bonebstr)    //System 80b: Bone Busters

DRIVERNV(lca)	      //System 3: Lights, Camera, Action 1989
DRIVERNV(bellring)    //System 3: Bell Ringer 1990
DRIVERNV(silvslug)    //System 3: Silver Slugger 1990
DRIVERNV(vegas)       //System 3: Vegas 1990
DRIVERNV(deadweap)    //System 3: Deadly Weapon 1990
DRIVERNV(tfight)      //System 3: Title Fight 1990
DRIVERNV(nudgeit)     //System 3: Nudge It 1990
DRIVERNV(carhop)      //System 3: Car Hop 1991
DRIVERNV(hoops)       //System 3: Hoops 1991
DRIVERNV(cactjack)    //System 3: Cactus Jacks 1991
DRIVERNV(clas1812)    //System 3: Class of 1812 1991
DRIVERNV(surfnsaf)    //System 3: Surf'n Safari 1991
DRIVERNV(opthund)     //System 3: Operation: Thunder
DRIVERNV(smb)         //System 3: Super Mario Brothers
DRIVERNV(smbmush)     //System 3: Super Mario Brothers Mushroom World
DRIVERNV(cueball)     //System 3: Cue Ball Wizard
DRIVERNV(sfight2)     //System 3: Street Fighter II
DRIVERNV(sfight2a)    //System 3: Street Fighter II (V.2)
DRIVERNV(teedoff)     //System 3: Tee'd Off
DRIVERNV(wipeout)     //System 3: Wipe Out
DRIVERNV(gladiatr)    //System 3: Gladiators
DRIVERNV(wcsoccer)    //System 3: World Challenge Soccer
DRIVERNV(rescu911)    //System 3: Rescue 911
DRIVERNV(freddy)      //System 3: Freddy: A Nightmare on Elm Street
DRIVERNV(shaqattq)    //System 3: Shaq Attaq
DRIVERNV(stargate)    //System 3: Stargate
DRIVERNV(stargat2)    //System 3: Stargate (V.2)
DRIVERNV(bighurt)     //System 3: Big Hurt
DRIVERNV(waterwld)    //System 3: Waterworld
DRIVERNV(andretti)    //System 3: Mario Andretti
DRIVERNV(barbwire)    //System 3: Barb Wire

// ---------------------
// STERN GAMES BELOW
// ---------------------
// MPU-100 - Chime Sound

DRIVERNV(stingray)		//MPU-100: 03/77 Stingray
DRIVERNV(pinball)		//MPU-100: 07/77 Pinball
DRIVERNV(stars)			//MPU-100: 03/78 Stars
DRIVERNV(memlane)		//MPU-100: 06/78 Memory Lane
// MPU-100 - Sound Board: SB-100
DRIVERNV(lectrono)		//MPU-100: 08/78 Lectronamo
DRIVERNV(wildfyre)		//MPU-100: 10/78 Wildfyre
DRIVERNV(nugent)		//MPU-100: 11/78 Nugent
DRIVERNV(dracula)		//MPU-100: 01/79 Dracula
DRIVERNV(trident)		//MPU-100: 03/79 Trident
DRIVERNV(hothand)		//MPU-100: 06/79 Hot Hand
DRIVERNV(magic)			//MPU-100: 08/79 Magic
// MPU-200 - Sound Board: SB-300
DRIVERNV(meteor)		//MPU-200: 09/79 Meteor
DRIVERNV(galaxy)		//MPU-200: 01/80 Galaxy
DRIVERNV(ali)			//MPU-200: 03/80 Ali
DRIVERNV(biggame)		//MPU-200: 03/80 Big Game
DRIVERNV(seawitch)		//MPU-200: 05/80 Seawitch
DRIVERNV(cheetah)		//MPU-200: 06/80 Cheetah
DRIVERNV(quicksil)		//MPU-200: 06/80 Quicksilver
DRIVERNV(nineball)		//MPU-200: 12/80 Nineball
DRIVERNV(freefall)		//MPU-200: 01/81 Free Fall
DRIVERNV(splitsec)		//MPU-200: 08/81 Split Second
DRIVERNV(catacomb)		//MPU-200: 10/81 Catacomb
DRIVERNV(ironmaid)      //MPU-200: 10/81 Iron Maiden
DRIVERNV(viper)			//MPU-200: 12/81 Viper
DRIVERNV(dragfist)		//MPU-200: 01/82 Dragonfist
// MPU-200 - Sound Board: SB-300, VS-100
DRIVERNV(flight2k)		//MPU-200: 08/80 Flight 2000
DRIVERNV(stargzr)		//MPU-200: 08/80 Stargazer
DRIVERNV(lightnin)		//MPU-200: 03/81 Lightning
DRIVERNV(orbitor1)		//MPU-200: 04/82 Orbitor One
DRIVERNV(cue)			//MPU-200: ??/82 Cue		(Proto - Never released)
DRIVERNV(lazrlord)		//MPU-200: 10/84 Lazer Lord	(Proto - Never released)

// Whitestar System
DRIVERNV(strikext)      //Whitestar: 03/00 Striker Extreme
DRIVERNV(strxt_uk)      //Whitestar: 03/00 Striker Extreme (UK)
DRIVERNV(strxt_gr)      //Whitestar: 03/00 Striker Extreme (Germany)
DRIVERNV(strxt_fr)      //Whitestar: 03/00 Striker Extreme (France)
DRIVERNV(strxt_it)      //Whitestar: 03/00 Striker Extreme (Italy)
DRIVERNV(strxt_sp)      //Whitestar: 03/00 Striker Extreme (Spain)
DRIVERNV(shrkysht)      //Whitestar: 09/00 Sharky's Shootout
#ifndef VPINMAME
DRIVERNV(hirolcas)      //Whitestar: 01/01 High Roller Casino
DRIVERNV(hirol_gr)      //Whitestar: 01/01 High Roller Casino (Germany)
DRIVERNV(austin)        //Whitestar: 05/01 Austin Powers (3.0)
DRIVERNV(austin2)       //Whitestar: 05/01 Austin Powers (2.0)
DRIVERNV(monopoly)      //Whitestar: 09/01 Monopoly (2.33)
#endif /* VPINMAME */
// ---------------------
// SEGA GAMES BELOW
// ---------------------
//Data East Hardare, DMD 192x64
DRIVERNV(frankst)       //DE/Sega MPU: 12/94 Frankenstein
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
//Whitestar Hardware DMD 128x32
DRIVERNV(apollo13)      //Whitestar: 11/95 Apollo 13
DRIVERNV(gldneye)       //Whitestar: 02/96 Golden Eye
DRIVERNV(twister)       //Whitestar: 04/96 Twister
DRIVERNV(id4)           //Whitestar: 07/96 ID4: Independance Day
DRIVERNV(spacejam)      //Whitestar: 08/96 Space Jam
DRIVERNV(swtril)        //Whitestar: 02/97 Star Wars Trilogy
DRIVERNV(jplstwld)      //Whitestar: 06/97 The Lost World: Jurassic Park
DRIVERNV(xfiles)        //Whitestar: 08/97 X-Files
DRIVERNV(startrp)       //Whitestar: 11/97 Starship Troopers
DRIVERNV(viprsega)	//Whitestar: 02/98 Viper Night Drivin'
DRIVERNV(lostspc)       //Whitestar: 06/98 Lost in Space
DRIVERNV(godzilla)      //Whitestar: 09/98 Godzilla
DRIVERNV(southpk)       //Whitestar: 01/99 South Park
DRIVERNV(harley)        //Whitestar: 08/99 Harley Davidon

// ---------------------
// DATA EAST GAMES BELOW
// ---------------------
DRIVERNV(lwar)          //Data East MPU: 05/87 Laser War
DRIVERNV(ssvc)          //Data East MPU: 03/88 Secret Service
DRIVERNV(torpe)         //Data East MPU: 08/88 Torpedo Alley
DRIVERNV(tmach)         //Data East MPU: 12/88 Time Machine
DRIVERNV(play)          //Data East MPU: 05/89 Playboy 35th Anniversary
//2 x 16 A/N Display
DRIVERNV(mnfb)          //Data East MPU: 09/89 ABC Monday Night Football
DRIVERNV(robo)          //Data East MPU: 11/89 Robo Cop
DRIVERNV(poto)          //Data East MPU: 01/90 Phantom of the Opera
DRIVERNV(bttf)          //Data East MPU: 06/90 Back to the Future
DRIVERNV(simp)          //Data East MPU: 09/90 The Simpsons
//DMD 128 x 16
DRIVERNV(chkpnt)        //Data East MPU: 02/91 Checkpoint
DRIVERNV(tmnt)          //Data East MPU: 05/91 Teenage Mutant Ninja Turtles
//BSMT2000 Sound chip
DRIVERNV(batmn)         //Data East MPU: 07/91 Batman
DRIVERNV(trek)          //Data East MPU: 10/91 Star Trek 25th Anniversary
DRIVERNV(hook)          //Data East MPU: 01/92 Hook
//DMD 128 x 32
DRIVERNV(lw3)           //Data East MPU: 06/92 Lethal Weapon
DRIVERNV(stwarde)       //Data East MPU: 10/92 Star Wars
DRIVERNV(rab)           //Data East MPU: 02/93 Rocky & Bullwinkle
DRIVERNV(jurpark)       //Data East MPU: 04/93 Jurasic Park
DRIVERNV(lah)           //Data East MPU: 08/93 Last Action Hero
DRIVERNV(tftc)          //Data East MPU: 11/93 Tales From the Crypt
DRIVERNV(tommy)         //Data East MPU: 02/94 Tommy
DRIVERNV(wwfrumb)       //Data East MPU: 05/94 WWF Royal Rumble
DRIVERNV(gnr)           //Data East MPU: 07/94 Guns N Roses
//DMD 192 x 64
DRIVERNV(maverick)      //Data East MPU: 09/94 Maverick
//MISC
DRIVERNV(detest)        //Data East MPU: ??/?? DE Test Chip




// -------------------------------------
// WILLIAMS & WILLIAMS/BALLY GAMES BELOW
// -------------------------------------
                    //??-466:   06/76 W Aztec
                    //??-470:   10/77 W Wild Card
DRIVER(httip,l1)    //S3-477:   11/77 W Hot Tip
DRIVER(lucky,l1)    //S3-480:   03/78 W Lucky Seven
DRIVER(cntct,l1)    //S3-482:   05/78 W Contact
DRIVER(wldcp,l1)    //S3-481:   05/78 W World Cup
DRIVER(disco,l1)    //S3-483:   08/78 W Disco Fever
DRIVER(pkrno,l1)    //S4-488:   10/78 W Pokerino
DRIVER(phnix,l1)    //S4-485:   11/78 W Phoenix
DRIVER(flash,l1)    //S4-486:   01/79 W Flash
DRIVER(stlwr,l2)    //S4-490:   03/79 W Stellar Wars
                    //S?-491:   06/79 W Rock'N Roll
DRIVER(trizn,l1)    //S6-487:   07/79 W TriZone
DRIVER(tmwrp,l2)    //S6-489:   09/79 W Time Warp
DRIVER(grgar,l1)    //S6-496:   11/79 W Gorgar
DRIVER(lzbal,l2)    //S6-493:   12/79 W Laser Ball
DRIVER(frpwr,l2)    //S6-497:   02/80 W Firepower
DRIVER(blkou,l1)    //S6-495:   06/80 W Blackout
DRIVER(scrpn,l1)    //S6-494:   07/80 W Scorpion
DRIVER(algar,l1)    //S6-499:   09/80 W Algar
DRIVER(alpok,l2)    //S6-501:   10/80 W Alien Poker
DRIVER(bk,l4)       //S7-500:   11/80 W Black Knight
DRIVER(jngld,l2)    //S7-503:   02/81 W Jungle Lord
DRIVER(pharo,l2)    //S7-504:   05/81 W Pharoah
                    //S7-506:   06/81 W Black Knight Limited Edition
DRIVER(solar,l2)    //S7-507:   07/81 W Solar Fire
DRIVER(barra,l1)    //S7-510:   09/81 W Barracora
DRIVER(hypbl,l4)    //S7-509:   12/81 W HyperBall
                    //S?-508:   05/82 W Thunderball
DRIVER(csmic,l1)    //S7-502:   06/82 W Cosmic Gunfight
DRIVER(vrkon,l1)    //S7-512:   09/82 W Varkon
DRIVER(wrlok,l3)    //S7-516:   10/82 W Warlok
DRIVER(dfndr,l4)    //S7-517:   12/82 W Defender
                    //S?-527:   01/83 W Rat Race (never produced)
DRIVER(tmfnt,l5)    //S7-515:   03/83 W Time Fantasy
DRIVER(jst,l2)      //S7-519:   04/83 W Joust
DRIVER(fpwr2,l2)    //S7-521:   08/83 W Firepower II
DRIVER(lsrcu,l2)    //S7-520:   02/84 W Laser Cue
DRIVER(pfevr,p3)    //S9-526:   05/84 W Pennant Fever (pitch & bat)
DRIVER(strlt,l1)    //S7-530:   06/84 W Star Light
                    //S?-538:   10/84 W Gridiron
DRIVER(sshtl,l7)    //S9-535:   12/84 W Space Shuttle
DRIVER(sorcr,l1)    //S9-532:   03/85 W Sorcerer
DRIVER(comet,l4)    //S9-540:   06/85 W Comet
DRIVER(hs,l4)       //S11-541:  01/86 W High Speed
                    //S11:      02/86 W Alley Cats (Bowler)
DRIVER(grand,l4)    //S11-523:  04/86 W Grand Lizard
DRIVER(rdkng,l5)    //S11-542:  07/86 W Road Kings
DRIVER(pb,l5)       //S11-549:  10/86 W Pin-bot
                    //S11:      10/86 W Strike Force
DRIVER(milln,l3)    //S11-555:  01/87 W Millionaire
DRIVER(f14,l1)      //S11-554:  03/87 W F-14 Tomcat
DRIVER(fire,l3)     //S11-556:  08/87 W Fire!
DRIVER(bguns,l8)    //S11-557:  10/87 W Big Guns
DRIVER(spstn,l5)    //S11-552:  12/87 W Space Station
DRIVER(cycln,l5)    //S11-564:  02/88 W Cyclone
DRIVER(bnzai,l3)    //S11-566:  05/88 W Banzai Run
DRIVER(swrds,l2)    //S11-559:  06/88 W Swords of Fury
DRIVER(taxi,l4)     //S11-553:  08/88 W Taxi
DRIVER(taxi,l3)
DRIVER(jokrz,l6)    //S11-567:  12/88 W Jokerz!
DRIVER(eshak,f1)    //S11-568:  02/89 W Earthshaker
DRIVER(eshak,l3)
DRIVER(bk2k,l4)     //S11-563:  04/89 W Black Knight 2000
DRIVER(tsptr,l3)    //S11-2630: 04/89 B Transporter the Rescue
                    //S11:      05/89 W Pool
DRIVER(polic,l4)    //S11-573:  08/89 W Police Force
DRIVER(eatpm,l4)    //S11-782:  10/89 B Elvira and the Party Monsters
DRIVER(bcats,l5)    //S11-575:  11/89 W Bad Cats
DRIVER(rvrbt,l3)    //S11-1966: 11/89 W Riverboat Gambler
DRIVER(mousn,l4)    //S11-1635: 12/89 B Mousin' Around!
                    //???:      ??/90 B Mazatron
                    //???:      ??/90 B Player's Choice
                    //???:      ??/90 B Ghost Gallery
DRIVER(whirl,l3)    //S11-574:  01/90 W Whirlwind
DRIVER(gs,l3)       //S11-985:  04/90 B Game Show
DRIVER(rollr,l2)    //S11-576:  06/90 W Rollergames
DRIVER(pool,l7)     //S11-1848: 06/90 B Pool Sharks (Shark?)
DRIVER(diner,l4)    //S11-571:  09/90 W Diner
DRIVER(radcl,l1)    //S11-1904: 09/90 B Radical!
DRIVER(dd,l2)       //S11-737:  11/90 B Dr. Dude

DRIVER(dd,p7)       //WPC:      11/90 B Dr. Dude
DRIVER(fh,l9)       //WPC-503:  12/90 W Funhouse
DRIVER(bbnny,l2)    //S11-396:  01/91 B Bugs Bunny's Birthday Ball
DRIVER(bop,l7)      //WPC-502:  02/91 W The Machine: Bride of Pinbot
DRIVER(hd,l3)       //WPC:      02/91 B Harley Davidson
DRIVER(sf,l1)       //WPC-601:  03/91 W SlugFest
DRIVER(t2,l8)       //WPC-513:  07/91 W Terminator 2: Judgement Day
DRIVER(t2,p2f)                        // Profanity Speech version
DRIVER(hurr,l2)     //WPC-512:  08/91 W Hurricane
DRIVER(pz,f4)       //WPC:      08/91 B Party Zone
DRIVER(gi,l9)       //WPC:      11/91 B Gilligan's Island
DRIVER(strik,l4)  //WPC:     05/92 W Strike Master
DRIVER(gw,l5)       //WPC-504:  02/92 W The Getaway: High Speed II
DRIVER(taf,l5)      //WPC:      03/92 B The Addams Family
DRIVER(taf,l6)      //WPC:      03/92 B The Addams Family L-6
DRIVER(br,l4)       //WPC:      07/92 B Black Rose
DRIVER(dw,l2)       //WPC:      09/92 B Doctor Who
DRIVER(ft,l5)       //WPC-505:  10/92 W Fish Tales
DRIVER(cftbl,l4)    //WPC:      12/92 B Creature from the Black Lagoon
DRIVER(ww,l5)       //WPC-518:  01/93 W White Water
DRIVER(drac,l1)     //WPC-501:  04/93 W Bram Stoker's Dracula
DRIVER(tz,92)       //WPC:      04/93 B Twilight Zone
DRIVER(tz,94h)
DRIVER(ij,l7)       //WPC-517:  08/93 W Indiana Jones
DRIVER(jd,l7)       //WPC:      09/93 B Judge Dredd
DRIVER(sttng,l7)    //WPC-523:  11/93 W Star Trek: The Next Generation
DRIVER(dm,lx4)      //WPC-528:  02/94 W Demolition Man
DRIVER(dm,px5)
DRIVER(pop,lx5)     //WPC:      02/94 B Popeye Saves the Earth
DRIVER(wcs,l2)      //WPC-531:  02/94 B World Cup Soccer
                    //WPC-620:  06/94 W Pinball Circus
DRIVER(fs,lx5)      //WPC-529:  07/94 W The Flintstones
DRIVER(corv,21)     //WPC-536:  08/94 B Corvette
                    //WPC-617:  10/94 W Hot Shots
DRIVER(rs,l6)       //WPC-524:  10/94 W Red & Ted's Road Show
DRIVER(rs,lx4)
DRIVER(tafg,lx3)    //WPC:      10/94 B The Addams Family Special Collectors Edition
DRIVER(tafg,h3)
DRIVER(ts,lx5)      //WPC-532:  11/94 B The Shadow
DRIVER(ts,lh6)
DRIVER(dh,lx2)      //WPC-530:  03/95 W Dirty Harry
DRIVER(tom,13)      //WPC-539:  03/95 B Theatre of Magic
DRIVER(tom,12)
DRIVER(nf,23x)      //WPC-525:  05/95 W No Fear: Dangerous Sports
DRIVER(i500,11r)    //WPC-526:  06/95 B Indianapolis 500
DRIVER(jm,12r)      //WPC-542:  08/95 W Johnny Mnemonic
DRIVER(wd,12)       //WPC-544:  09/95 B Who dunnit
DRIVER(wd,12g)      //WPC-544:  09/95 B Who dunnit (Germany)
DRIVER(jb,10r)      //WPC-551:  10/95 W Jack*Bot
DRIVER(jb,10b)		//WPC-551:  10/95 W Jack*Bot (Belgium/Canada)
DRIVER(congo,21)    //WPC-550:  11/95 W Congo
DRIVER(afm,11)      //WPC-541:  12/95 B Attack from Mars
DRIVER(afm,113)
DRIVER(lc,11)       //WPC:      ??/96 B League Champ (Shuffle Alley)
DRIVER(ttt,10)      //WPC-905:  03/96 W Ticket Tac Toe
DRIVER(sc,18)       //WPC-903:  03/96 B Safe Cracker
DRIVER(sc,18n)      //WPC-903:  03/96 B Safe Cracker (New)
DRIVER(totan,14)    //WPC-547:  05/96 W Tales of the Arabian Nights
DRIVER(ss,15)       //WPC-548:  09/96 B Scared Stiff
DRIVER(jy,12)       //WPC-552:  12/96 W Junk Yard
DRIVER(nbaf,31)     //WPC-553:  03/97 B NBA Fastbreak
DRIVER(nbaf,31a)
DRIVER(mm,109)      //WPC-559:  06/97 W Medieval Madness
DRIVER(mm,10)
DRIVER(cv,14)       //WPC-562:  10/97 B Cirqus Voltaire
DRIVER(ngg,13)      //WPC-561:  12/97 W No Good Gofers
DRIVER(cp,16)       //WPC-563:  04/98 B The Champion Pub
DRIVER(mb,106)      //WPC-565:  07/98 W Monster Bash
DRIVER(mb,10)
DRIVER(cc,12)       //WPC-566:  10/98 B Cactus Canyon
DRIVER(cc,13)

DRIVER(tfdmd,l3)    //WPC:              Test fixture DMD
DRIVER(tfs,12)      //WPC-648:          Test fixture Security
DRIVER(tfa,13)      //WPC:              Test fixture Alphanumeric
DRIVER(tf95,12)     //WPC-648:          Test fixture WPC95
#endif /* DRIVER_RECURSIVE */

