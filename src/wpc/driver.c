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

// -----------------
// ATARI GAMES BELOW
// -----------------
DRIVERNV(atarians)      //The Atarians (November 1976)
DRIVERNV(time2000)      //Time 2000 (June 1977)
DRIVERNV(aavenger)      //Airborne Avenger (September 1977)
DRIVERNV(midearth)      //Middle Earth (February 1978)
DRIVERNV(spcrider)      //Space Riders (September 1978)
DRIVERNV(superman)      //Superman (March 1979)
DRIVERNV(hercules)      //Hercules (May 1979)
                                        //Road Runner (1979)
                                        //Monza (1980)
                                        //Neutron Star (1981)
                                        //4x4 (1983)
                                        //Triangle (19??)

// ---------------------
// GAME PLAN GAMES BELOW
// ---------------------
/*Games below are Cocktail #110 Model*/
DRIVERNV(foxylady)      //Foxy Lady (May 1978)
DRIVERNV(blvelvet)      //Black Velvet (May 1978)
DRIVERNV(camlight)      //Camel Lights (May 1978)
DRIVERNV(real)  //Real (to Real) (May 1978)
DRIVERNV(rio)   //Rio (?? / 1978)

                                        //Chuck-A-Luck (October 1978)

/*Games below are Cocktail #120 Model*/
DRIVERNV(startrip)      //Star Trip (April 1979)
DRIVERNV(famlyfun)      //Family Fun! (April 1979)

/*Games below are regular standup pinball games*/
DRIVERNV(sshooter)      //Sharpshooter (May 1979)
DRIVERNV(vegasgp)       //Vegas (August 1979)
DRIVERNV(coneyis)       //Coney Island! (December 1979)
DRIVERNV(lizard)        //Lizard (July 1980)
DRIVERNV(gwarfare)      //Global Warfare (June 1981)
                                        //Mike Bossy (January 1982)
                                        //The Scoring Machine (January 1982)
DRIVERNV(suprnova)      //Super Nova (May 1982)
DRIVERNV(sshootr2)      //Sharp Shooter II (November 1983)
DRIVERNV(atilla)        //Attila the Hun (April 1984)
DRIVERNV(agent777)      //Agents 777 (November 1984)
DRIVERNV(cpthook)       //Captain Hook (April 1985)
DRIVERNV(ladyshot)      //Lady Sharpshooter (May 1985)
DRIVERNV(andromed)      //Andromeda (September 1985)
DRIVERNV(cyclopes)      //Cyclopes (November 1985)
                                        //Loch Ness Monster (November 1985)


// ---------------------
// ZACCARIA GAMES BELOW
// ---------------------
                                        //Other games created earlier by Zaccaria are EM
                                        //10/77 Combat
                                        //01/78 Winter Sports
                                        //07/78 House of Diamonds
                                        //09/78 Strike
                                        //10/78 Ski Jump
                                        //10/78 Future World
                                        //04/79 Shooting the Rapids
                                        //09/79 Hot Wheels
                                        //09/79 Space City
                                        //01/80 Fire Mountain
                                        //05/80 Star God
                                        //09/80 Space Shuttle
                                        //04/81 Earth, Wind & Fire
                                        //09/81 Locomotion
                                        //04/82 Pinball Champ '82 (Is this really different than the '83?)
                                        //09/82 Soccer King
DRIVERNV(pinchamp)      //??/83 Pinball Champ
DRIVERNV(tmachzac)      //04/83 Time Machine
DRIVERNV(farfalla)      //09/83 Farfalla
DRIVERNV(dvlrider)      //04/84 Devil Riders
                                        //09/84 Magic Castle
                                        //01/85 Robot
                                        //07/85 Clown
                                        //12/85 Pool Champion
DRIVERNV(bbeltzac)      //??/86 Blackbelt
                                        //??/86 Mexico
                                        //??/86 Zankor
                                        //??/86 Mystic Star
                                        //??/87 Spooky
                                        //??/87 Star's Phoenix
                                        //??/86 New Star's Phoenix


// ---------------------
// HANKIN GAMES BELOW
// ---------------------
DRIVERNV(fjholden)    //FJ Holden
DRIVERNV(orbit1)      //Orbit 1
DRIVERNV(howzat)      //Howzat
DRIVERNV(shark)       //Shark
DRIVERNV(empsback)    //Star Wars - The Empire Strike Back

// ---------------------
// BALLY GAMES BELOW
// ---------------------
//MPU-17
DRIVERNV(freedom )    //BY17-720: 08/76 Freedom
DRIVERNV(nightrdr)    //BY17-721: 01/76 Night Rider (EM release date)
DRIVERNV(blackjck)    //BY17-728: 05/76 Black Jack  (EM release date)
DRIVERNV(evelknie)    //BY17-722: 09/76 Evel Knievel
DRIVERNV(matahari)    //BY17-725: 09/77 Mata Hari
                      //??        10/76 Fireball
                      //??        10/76 Star Ship
DRIVERNV(sst     )    //BY35-741: 10/76 Supersonic
DRIVERNV(eightbll)    //BY17-723: 01/77 Eight Ball
DRIVERNV(pwerplay)    //BY17-724: 02/77 Power Play
DRIVERNV(stk_sprs)    //BY17-740: 08/77 Strikes and Spares
//MPU-35
DRIVERNV(lostwrld)    //BY35-729: 02/77 Lost World
DRIVERNV(smman   )    //BY35-742: 08/77 The Six Million Dollar Man
DRIVERNV(playboy )    //BY35-743: 09/76 Playboy
                      //??          /78 Big Foot
                      //??          /78 Galaxy
DRIVERNV(startrek)    //BY35-745: 01/78 Star Trek
DRIVERNV(voltan  )    //BY35-744: 01/78 Voltan Escapes Cosmic Doom
                      //??        02/78 Skateball
DRIVERNV(paragon )    //BY35-748: 12/78 Paragon
DRIVERNV(hglbtrtr)    //BY35-750: 08/78 Harlem Globetrotters On Tour
DRIVERNV(dollyptn)    //BY35-777: 10/78 Dolly Parton
DRIVERNV(kiss    )    //BY35-746: 04/78 Kiss
DRIVERNV(futurspa)    //BY35-781: 03/79 Future Spa
DRIVERNV(spaceinv)    //BY35-792: 05/79 Space Invaders
DRIVERNV(ngndshkr)    //BY35-776: 05/78 Nitro Ground Shaker
DRIVERNV(slbmania)    //BY35-786: 06/78 Silverball Mania
DRIVERNV(rollston)    //BY35-796: 06/79 Rolling Stones
DRIVERNV(mystic  )    //BY35-798: 08/79 Mystic
DRIVERNV(hotdoggn)    //BY35-809: 12/79 Hotdoggin'
DRIVERNV(viking  )    //BY35-802: 12/79 Viking
DRIVERNV(skatebll)    //BY35-823: 04/80 Skateball
DRIVERNV(frontier)    //BY35-819: 05/80 Frontier
DRIVERNV(xenon   )    //BY35-811: 11/79 Xenon
DRIVERNV(xenonf  )    //BY35-811: 11/79 Xenon (French)
DRIVERNV(flashgdn)    //BY35-834: 05/80 Flash Gordon
DRIVERNV(flashgdf)    //BY35-834: 05/80 Flash Gordon (French)
DRIVERNV(eballdlx)    //BY35-838: 09/80 Eight Ball Deluxe
DRIVERNV(fball_ii)    //BY35-839: 09/80 Fireball II
DRIVERNV(embryon )    //BY35-841: 09/80 Embryon
DRIVERNV(fathom  )    //BY35-842: 12/80 Fathom
DRIVERNV(medusa  )    //BY35-845: 02/81 Medusa
DRIVERNV(centaur )    //BY35-848: 02/81 Centaur
DRIVERNV(elektra )    //BY35-857: 03/81 Elektra
DRIVERNV(vector  )    //BY35-858: 03/81 Vector
DRIVERNV(spectrum)    //BY35-868: 04/81 Spectrum
DRIVERNV(spectru4)    //BY35-868: 04/81 Spectrum (rel 4)
DRIVERNV(speakesy)    //BY35-877: 08/82 Speakeasy
DRIVERNV(speakes4)    //BY35-877: 08/82 Speakeasy 4 (4 player)
DRIVERNV(rapidfir)    //BY35-869: 06/81 Rapid Fire
DRIVERNV(m_mpac  )    //BY35-872: 05/82 Mr. & Mrs. Pac-Man
DRIVERNV(babypac )    //??        10/82 Baby Pac-Man
/*same as eballdlx*/  //BY35      10/82 Eight Ball Deluxe Limited Edition
DRIVERNV(bmx     )    //BY35-888: 11/82 BMX
DRIVERNV(granslam)    //BY35-     01/83 Grand Slam
/*same as cenatur*/   //BY35      06/83 Centaur II
DRIVERNV(goldball)    //BY35-     10/83 Gold Ball
DRIVERNV(xsandos )    //BY35-     12/83 X's & O's
                      //??        ??/84 Mysterian
DRIVERNV(granny )     //BY35-     01/84 Granny and the Gators
DRIVERNV(kosteel )    //BY35-     05/84 Kings of Steel
DRIVERNV(blakpyra)    //BY35-     07/84 Black Pyramid
DRIVERNV(spyhuntr)    //BY35-     10/84 Spy Hunter
                      //??        ??/85 Hot Shotz
DRIVERNV(fbclass )    //BY35-     02/85 Fireball Classic
DRIVERNV(cybrnaut)    //BY35-     05/85 Cybernaut
//MPU-6803
DRIVERNV(eballchp)    //6803-0B38: 09/85 Eight Ball Champ
DRIVERNV(eballch2)    //                 Eight Ball Champ (cheap squeak)
DRIVERNV(beatclck)    //6803-0C70: 11/85 Beat the Clock
DRIVERNV(ladyluck)    //6803-0E34: 02/86 Lady Luck
DRIVERNV(motrdome)    //6803-0E14: 05/86 MotorDome
                      //6803-????: 06/86 Karate Fight (Prototype for Black Belt?)
DRIVERNV(blackblt)    //6803-0E52: 07/86 Black Belt
DRIVERNV(specforc)    //6803-0E47: 08/86 Special Force
DRIVERNV(strngsci)    //6803-0E35: 10/86 Strange Science
DRIVERNV(cityslck)    //6803-0E79: 03/87 City Slicker
DRIVERNV(hardbody)    //6803-0E94: 03/87 Hardbody
DRIVERNV(prtyanim)    //6803-0H01: 05/87 Party Animal
DRIVERNV(hvymetal)    //6803-0H03: 08/87 Heavy Metal Meltdown
DRIVERNV(dungdrag)    //6803-0H06: 10/87 Dungeons & Dragons
DRIVERNV(esclwrld)    //6803-0H05: 01/88 Escape from the Lost World
DRIVERNV(black100)    //6803-0H07: 03/88 Blackwater 100
                      //??         06/88 Ramp Warrior (Became Truck Stop after Merger)
//Williams Merger begins here..
DRIVERNV(truckstp)    //6803-2001: 12/88 Truck Stop
DRIVERNV(atlantis)    //6803-2006: 03/89 Atlantis
                                                //??         05/89 Ice Castle

// ---------------------
// GOTTLIEB GAMES BELOW
// ---------------------
                      //??-421    02/79 Solar Ride
                      //??-422    05/79 Count-Down
                      //??-428    08/79 Space Walk
                      //??-433    10/79 Incredible Hulk
                      //??-429    10/79 Totem
                      //??-435    11/79 Genie
//System 80
DRIVERNV(spidermn)    //S80-635:  05/80 The Amazing Spider-man
DRIVERNV(circus)      //S80-654:  06/80 Circus
DRIVERNV(panthera)    //S80-652:  06/80 Panthera
DRIVERNV(cntforce)    //S80-656:  08/80 Counterforce
DRIVERNV(starrace)    //S80-657:  10/80 Star Race
DRIVERNV(jamesb)      //S80-658:  10/80 James Bond (Timed Play)
DRIVERNV(jamesb2)     //                James Bond (3/5 Ball)
DRIVERNV(timeline)    //S80-659:  10/80 Time Line
DRIVERNV(forceii)     //S80-661:  02/81 Force II
DRIVERNV(pnkpnthr)    //S80-664:  03/81 Pink Panther
DRIVERNV(mars)        //S80-666:  03/81 Mars God of War
DRIVERNV(vlcno_ax)    //S80-667:  09/81 Volcano (Sound & Speech)
DRIVERNV(vlcno_1b)    //                Volcano (Sound Only)
DRIVERNV(blckhole)    //S80-668:  10/81 Black Hole (Sound & Speech, Rev 4)
DRIVERNV(blkhole2)    //                Black Hole (Sound & Speech, Rev 2)
DRIVERNV(blkholea)    //                Black Hole (Sound Only)
DRIVERNV(eclipse)     //S80-671:  ??/82 Eclipse
DRIVERNV(hh)          //S80-669:  06/82 Haunted House (Rev 2)
DRIVERNV(hh_1)        //                Haunted House (Rev 1)
DRIVERNV(s80tst)      //S80: Text Fixture
//System 80a
DRIVERNV(dvlsdre)     //S80a-670: 08/82 Devils Dare (Sound & Speech)
DRIVERNV(dvlsdre2)    //                Devils Dare (Sound Only)
DRIVERNV(caveman)     //S80a-PV810:09/82 Caveman
DRIVERNV(rocky)       //S80a-672: 09/82 Rocky
DRIVERNV(spirit)      //S80a-673: 11/82 Spirit
DRIVERNV(striker)     //S80a-675: 11/82 Striker
DRIVERNV(punk)        //S80a-674: 12/82 Punk!
DRIVERNV(krull)       //S80a-676: 02/83 Krull
DRIVERNV(goinnuts)    //S80a-682: 02/83 Goin' Nuts
DRIVERNV(qbquest)     //S80a-677: 03/83 Q*bert's Quest
DRIVERNV(sorbit)      //S80a-680: 05/83 Super Orbit
DRIVERNV(rflshdlx)    //S80a-681: 06/83 Royal Flush Deluxe
DRIVERNV(amazonh)     //S80a-684: 09/83 Amazon Hunt
DRIVERNV(rackemup)    //S80a-685: 11/83 Rack 'Em Up
DRIVERNV(raimfire)    //S80a-686: 11/83 Ready Aim Fire
DRIVERNV(jack2opn)    //S80a-687: 05/84 Jacks to Open
DRIVERNV(alienstr)    //S80a-689: 06/84 Alien Star
DRIVERNV(thegames)    //S80a-691: 08/84 The Games
DRIVERNV(eldorado)    //S80a-692: 09/84 El Dorado City of Gold
DRIVERNV(touchdn)     //S80a-688: 10/84 Touchdown
DRIVERNV(icefever)    //S80a-695: 02/85 Ice Fever
//System 80b
DRIVERNV(triplay)     //S80b-696: 05/85 Chicago Cubs Triple Play
DRIVERNV(bountyh)     //S80b-694: 07/85 Bounty Hunter
                      //S80b-698: 09/85 Tag Team Pinball
DRIVERNV(rock)        //S80b-697: 10/85 Rock
DRIVERNV(raven)       //S80b-702: 03/86 Raven
                      //S80b-704: 04/86 Rock Encore
DRIVERNV(hlywoodh)    //S80b-703: 06/86 Hollywood Heat
DRIVERNV(genesis)     //S80b-705: 09/86 Genesis
DRIVERNV(goldwing)    //S80b-707: 10/86 Gold Wings
DRIVERNV(mntecrlo)    //S80b-708: 02/87 Monte Carlo
DRIVERNV(sprbreak)    //S80b-706: 04/87 Spring Break
                      //S80b-???: 05/87 Amazon Hunt II
DRIVERNV(arena)       //S80b-709: 06/87 Arena
DRIVERNV(victory)     //S80b-710: 10/87 Victory
DRIVERNV(diamond)     //S80b-711: 02/88 Diamond Lady
DRIVERNV(txsector)    //S80b-712: 03/88 TX Sector
DRIVERNV(robowars)    //S80b-714: 04/88 Robo-War
DRIVERNV(badgirls)    //S80b-717: 11/88 Bad Girls
DRIVERNV(excalibr)    //S80b-715: 11/88 Excalibur
DRIVERNV(bighouse)    //S80b-713: 04/89 Big House
                      //S80b-718: 04/89 Hot Shots
DRIVERNV(bonebstr)    //S80b-719: 08/89 Bone Busters Inc
//System 3 Alphanumeric
DRIVERNV(lca)         //S3-720:   11/89 Lights, Camera, Action 1989
DRIVERNV(silvslug)    //S3-722:   02/90 Silver Slugger 1990
DRIVERNV(vegas)       //S3-723:   07/90 Vegas 1990
DRIVERNV(deadweap)    //S3-724:   09/90 Deadly Weapon 1990
DRIVERNV(tfight)      //S3-726:   10/90 Title Fight 1990
DRIVERNV(bellring)    //S3-???:   12/90 Bell Ringer 1990
DRIVERNV(nudgeit)     //S3-???:   12/90 Nudge It 1990
                      //??-???:   ??/91 Amazon Hunt III
DRIVERNV(carhop)      //S3-725:   01/91 Car Hop 1991
DRIVERNV(hoops)       //S3-727:   02/91 Hoops 1991
DRIVERNV(cactjack)    //S3-729:   04/91 Cactus Jack's 1991
DRIVERNV(clas1812)    //S3-730:   08/91 Class of 1812 1991
DRIVERNV(surfnsaf)    //S3-731:   11/91 Surf'n Safari 1991
DRIVERNV(opthund)     //S3-732:   02/92 Operation: Thunder
//System 3 128x32 DMD
DRIVERNV(smb)         //S3-733:   04/92 Super Mario Bros.
DRIVERNV(smbmush)     //S3-???:   06/92 Super Mario Bros. Mushroom World
DRIVERNV(cueball)     //S3-734:   10/92 Cue Ball Wizard
DRIVERNV(sfight2)     //S3-735:   03/93 Street Fighter II
DRIVERNV(sfight2a)    //                Street Fighter II (V2)
DRIVERNV(teedoff)     //S3-736:   05/93 Tee'd Off
DRIVERNV(wipeout)     //S3-738:   10/93 Wipe Out
DRIVERNV(gladiatr)    //S3-737:   11/93 Gladiators
DRIVERNV(wcsoccer)    //S3-741:   02/94 World Challenge Soccer
DRIVERNV(rescu911)    //S3-740:   05/94 Rescue 911
DRIVERNV(freddy)      //S3-744:   10/94 Freddy: A Nightmare on Elm Street
DRIVERNV(shaqattq)    //S3-743:   02/95 Shaq Attaq
DRIVERNV(stargate)    //S3-742:   03/95 Stargate
DRIVERNV(stargat2)    //                Starget (V2)
DRIVERNV(bighurt)     //S3-743:   06/95 Big Hurt
                      //S3-???:   10/95 Strikes 'N Spares
DRIVERNV(waterwld)    //S3-746:   10/95 Waterworld
DRIVERNV(andretti)    //S3-747:   12/95 Mario Andretti
DRIVERNV(barbwire)    //S3-748:   04/96 Barb Wire
//Never produced      //S3-???:         Brooks & Dunn
// ---------------------
// STERN GAMES BELOW
// ---------------------
// MPU-100 - Chime Sound
DRIVERNV(stingray)    //MPU-100: 03/77 Stingray
DRIVERNV(pinball)     //MPU-100: 07/77 Pinball
DRIVERNV(stars)       //MPU-100: 03/78 Stars
DRIVERNV(memlane)     //MPU-100: 06/78 Memory Lane
// MPU-100 - Sound Board: SB-100
DRIVERNV(lectrono)    //MPU-100: 08/78 Lectronamo
DRIVERNV(wildfyre)    //MPU-100: 10/78 Wildfyre
DRIVERNV(nugent)      //MPU-100: 11/78 Nugent
DRIVERNV(dracula)     //MPU-100: 01/79 Dracula
DRIVERNV(trident)     //MPU-100: 03/79 Trident
DRIVERNV(hothand)     //MPU-100: 06/79 Hot Hand
DRIVERNV(magic)       //MPU-100: 08/79 Magic
// MPU-200 - Sound Board: SB-300
DRIVERNV(meteor)      //MPU-200: 09/79 Meteor
DRIVERNV(galaxy)      //MPU-200: 01/80 Galaxy
DRIVERNV(ali)         //MPU-200: 03/80 Ali
DRIVERNV(biggame)     //MPU-200: 03/80 Big Game
DRIVERNV(seawitch)    //MPU-200: 05/80 Seawitch
DRIVERNV(cheetah)     //MPU-200: 06/80 Cheetah
DRIVERNV(quicksil)    //MPU-200: 06/80 Quicksilver
DRIVERNV(nineball)    //MPU-200: 12/80 Nineball
DRIVERNV(freefall)    //MPU-200: 01/81 Free Fall
DRIVERNV(splitsec)    //MPU-200: 08/81 Split Second
DRIVERNV(catacomb)    //MPU-200: 10/81 Catacomb
DRIVERNV(ironmaid)    //MPU-200: 10/81 Iron Maiden
DRIVERNV(viper)       //MPU-200: 12/81 Viper
DRIVERNV(dragfist)    //MPU-200: 01/82 Dragonfist
// MPU-200 - Sound Board: SB-300, VS-100
DRIVERNV(flight2k)    //MPU-200: 08/80 Flight 2000
DRIVERNV(stargzr)     //MPU-200: 08/80 Stargazer
DRIVERNV(lightnin)    //MPU-200: 03/81 Lightning
DRIVERNV(orbitor1)    //MPU-200: 04/82 Orbitor One
DRIVERNV(cue)         //MPU-200: ??/82 Cue            (Proto - Never released)
DRIVERNV(lazrlord)    //MPU-200: 10/84 Lazer Lord     (Proto - Never released)

// Whitestar System
DRIVERNV(strikext)    //Whitestar: 03/00 Striker Extreme
DRIVERNV(strxt_uk)    //Whitestar: 03/00 Striker Extreme (UK)
DRIVERNV(strxt_gr)    //Whitestar: 03/00 Striker Extreme (Germany)
DRIVERNV(strxt_fr)    //Whitestar: 03/00 Striker Extreme (France)
DRIVERNV(strxt_it)    //Whitestar: 03/00 Striker Extreme (Italy)
DRIVERNV(strxt_sp)    //Whitestar: 03/00 Striker Extreme (Spain)
DRIVERNV(shrkysht)    //Whitestar: 09/00 Sharky's Shootout
#ifndef VPINMAME
DRIVERNV(hirolcas)    //Whitestar: 01/01 High Roller Casino
DRIVERNV(hirol_gr)    //Whitestar: 01/01 High Roller Casino (Germany)
DRIVERNV(austin)      //Whitestar: 05/01 Austin Powers (3.0)
DRIVERNV(austin2)     //Whitestar: 05/01 Austin Powers (2.0)
DRIVERNV(monopoly)    //Whitestar: 09/01 Monopoly (2.33)
#endif /* VPINMAME */
// ---------------------
// SEGA GAMES BELOW
// ---------------------
//Data East Hardare, DMD 192x64
DRIVERNV(frankst)     //DE/Sega MPU: 12/94 Frankenstein
DRIVERNV(baywatch)    //DE/Sega MPU: 03/95 Baywatch
DRIVERNV(batmanf)     //DE/Sega MPU: 07/95 Batman Forever (4.0)
DRIVERNV(batmanf3)    //DE/Sega MPU: 07/95 Batman Forever (3.0)
DRIVERNV(bmf_uk)      //DE/Sega MPU: 07/95 Batman Forever (UK)
DRIVERNV(bmf_at)      //DE/Sega MPU: 07/95 Batman Forever (Austria)
DRIVERNV(bmf_be)      //DE/Sega MPU: 07/95 Batman Forever (Belgium)
DRIVERNV(bmf_ch)      //DE/Sega MPU: 07/95 Batman Forever (Switzerland)
DRIVERNV(bmf_cn)      //DE/Sega MPU: 07/95 Batman Forever (Canada)
DRIVERNV(bmf_de)      //DE/Sega MPU: 07/95 Batman Forever (Germany)
DRIVERNV(bmf_fr)      //DE/Sega MPU: 07/95 Batman Forever (France)
DRIVERNV(bmf_nl)      //DE/Sega MPU: 07/95 Batman Forever (Holland)
DRIVERNV(bmf_it)      //DE/Sega MPU: 07/95 Batman Forever (Italy)
DRIVERNV(bmf_sp)      //DE/Sega MPU: 07/95 Batman Forever (Spain)
DRIVERNV(bmf_no)      //DE/Sega MPU: 07/95 Batman Forever (Norway)
DRIVERNV(bmf_sv)      //DE/Sega MPU: 07/95 Batman Forever (Sweden)
DRIVERNV(bmf_jp)      //DE/Sega MPU: 07/95 Batman Forever (Japan)
DRIVERNV(bmf_time)    //DE/Sega MPU: 07/95 Batman Forever (Timed Version)
//Whitestar Hardware DMD 128x32
DRIVERNV(apollo13)    //Whitestar: 11/95 Apollo 13
DRIVERNV(gldneye)     //Whitestar: 02/96 Golden Eye
DRIVERNV(twister)     //Whitestar: 04/96 Twister
DRIVERNV(id4)         //Whitestar: 07/96 ID4: Independance Day
DRIVERNV(spacejam)    //Whitestar: 08/96 Space Jam
DRIVERNV(swtril)      //Whitestar: 02/97 Star Wars Trilogy
DRIVERNV(jplstwld)    //Whitestar: 06/97 The Lost World: Jurassic Park
DRIVERNV(xfiles)      //Whitestar: 08/97 X-Files
DRIVERNV(startrp)     //Whitestar: 11/97 Starship Troopers
DRIVERNV(viprsega)    //Whitestar: 02/98 Viper Night Drivin'
DRIVERNV(lostspc)     //Whitestar: 06/98 Lost in Space
DRIVERNV(godzilla)    //Whitestar: 09/98 Godzilla
DRIVERNV(southpk)     //Whitestar: 01/99 South Park
DRIVERNV(harley)      //Whitestar: 08/99 Harley Davidon

// ---------------------
// DATA EAST GAMES BELOW
// ---------------------
DRIVERNV(lwar)        //Data East MPU: 05/87 Laser War
DRIVERNV(ssvc)        //Data East MPU: 03/88 Secret Service
DRIVERNV(torpe)       //Data East MPU: 08/88 Torpedo Alley
DRIVERNV(tmach)       //Data East MPU: 12/88 Time Machine
DRIVERNV(play)        //Data East MPU: 05/89 Playboy 35th Anniversary
//2 x 16 A/N Display
DRIVERNV(mnfb)        //Data East MPU: 09/89 ABC Monday Night Football
DRIVERNV(robo)        //Data East MPU: 11/89 Robo Cop
DRIVERNV(poto)        //Data East MPU: 01/90 Phantom of the Opera
DRIVERNV(bttf)        //Data East MPU: 06/90 Back to the Future
DRIVERNV(simp)        //Data East MPU: 09/90 The Simpsons
//DMD 128 x 16
DRIVERNV(chkpnt)      //Data East MPU: 02/91 Checkpoint
DRIVERNV(tmnt)        //Data East MPU: 05/91 Teenage Mutant Ninja Turtles
//BSMT2000 Sound chip
DRIVERNV(batmn)       //Data East MPU: 07/91 Batman
DRIVERNV(trek)        //Data East MPU: 10/91 Star Trek 25th Anniversary
DRIVERNV(hook)        //Data East MPU: 01/92 Hook
//DMD 128 x 32
DRIVERNV(lw3)         //Data East MPU: 06/92 Lethal Weapon
DRIVERNV(stwarde)     //Data East MPU: 10/92 Star Wars
DRIVERNV(rab)         //Data East MPU: 02/93 Rocky & Bullwinkle
DRIVERNV(jurpark)     //Data East MPU: 04/93 Jurasic Park
DRIVERNV(lah)         //Data East MPU: 08/93 Last Action Hero
DRIVERNV(tftc)        //Data East MPU: 11/93 Tales From the Crypt
DRIVERNV(tommy)       //Data East MPU: 02/94 Tommy
DRIVERNV(wwfrumb)     //Data East MPU: 05/94 WWF Royal Rumble
DRIVERNV(gnr)         //Data East MPU: 07/94 Guns N Roses
//DMD 192 x 64
DRIVERNV(maverick)    //Data East MPU: 09/94 Maverick
//MISC
DRIVERNV(detest)      //Data East MPU: ??/?? DE Test Chip




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
//First Game produced entirely by Williams after Merger to use Bally Name
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
DRIVER(wcs,l2)      //WPC-531:  02/94 B World Cup Soccer (Lx-2)
DRIVER(wcs,p3)      //                  World Cup Soccer (Px-3)
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
DRIVER(jb,10b)          //WPC-551:  10/95 W Jack*Bot (Belgium/Canada)
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

