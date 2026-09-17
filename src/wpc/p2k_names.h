/* The games' own device tables, read out of their game.roms and kept here by hand. Tables only change when a new
   version turns up with a device the old ones did not have.

   Note to table authors: If just interested in the switch/coil/lamp mappings, skip this wall of text/comments here and go to the tables directly!


   Not all from one version, and the reason is worth knowing. Revenge From Mars is read out of 2.60:
   against the official 1.60 that changes five entries, four of them the expansion switches 53-56
   which 1.60 has no name for, so taking the newest costs nothing and gains those.

   Episode I is not. Its 2.10 differs from the official 1.50 in 28 switches and 19 drivers, nearly
   all of it myPinballs rewording - Bank became Standup, Sling became Slingshot, "fl." became
   Flasher - and one of the rewordings is wrong: 2.10 calls switch 34 Left Standup - Lower, a
   duplicate of 36, where 1.50 and the manual both have Left Bank: Upper. 2.10 also drops 107 and
   108, the upper flipper EOS pair. So Episode I is read out of 1.50, with only the entries 2.10
   genuinely adds taken from it: switches 48 and 52 for the 6 ball trough, and drivers 5, 42, 43
   and 44 - an auto plunger, knocker, shaker and topper, all on drives the manual lists as unused.

   Driver 5 is the one entry here that is different for some sets, and deliberately so. The two
   unofficial lines chose different drives for the same job: myPinballs' 2.x puts an auto plunger
   on 5 and the shaker on 43, while hemtoni's 1.66 puts the shaker on 5 and leaves 42-44 unused.
   Read out of their own tables:

       drv   1.50        1.66                2.00 / 2.10
       5     Not Used    Shaker (Optional)   Auto Plunger
       42    Not Used    Not Used            Knocker (Optional)
       43    Not Used    Not Used            Shaker Motor (Optional)
       44    Not Used    Not Used            Topper (Optional)

   One table cannot be right for both, and this one follows 2.10, so on swep1_166r2 driver 5 reads
   Auto Plunger where the machine means its shaker. Worth knowing before trusting a coil watch on
   that set; fixing it properly means making the coil table version-aware, which nothing else
   needs yet.

   Every version was swept against its game's official one, not just the newest. For Revenge From
   Mars 1.20, 1.40, 1.50, 1.80, 1.90, 1.91 and 1.95 are all identical to 1.60 - the same table for
   eighteen years - and the 2.x sets differ only in 53/54 (55/56 as well from 2.60), the casing of
   94, and drivers 18/19. Episode I's 1.30 and 1.40 likewise match 1.50, and all three 2.x differ
   from it in the same way.

   Every 'Not Used' slot is dropped.

   Each game.rom holds a switch table of 0x30-byte records and a driver table of 0x18-byte ones,
   both ending in the name repeated once per language, and both are flat arrays indexed by device
   number: switch number = 100 + index, driver = index + 1.

   To redo them, anchor on a name whose number is known and step by the record size. game.rom loads
   at 0x100000, so a pointer to a string at file offset N reads 0x100000 + N:

     switches   find the pointer to "Slam Tilt", switch 111 and so index 80, and the table of name
                pointers is that address - 80 * 0x30; walk it and read off
                ((i / 8) + 1) * 10 + (i % 8) + 1 as the PinMAME number
     coils      the same with a known driver: "Left Martian" is 1 on Revenge From Mars, and
                "Right Flipper Power" is 33 on both games, the flipper circuits being fixed by the
                power driver board; step 0x18

   Search the whole file for the string but only inside the table's own region for the pointer -
   names like "Drop Target Down" and "Left Jet" are in the switch, coil and lamp tables alike, and
   the first hit is as likely to be the wrong one. Each name is repeated up to four times in a row
   for the languages, so a hit may be any of four consecutive dwords; anchoring on a known number
   and stepping is what keeps that from mattering.

   Stop at the end of each table, which nothing in the data marks: the tables sit back to back and
   walking past one gives plausible nonsense out of the next. Switches end at 118 and drivers at 48,
   both per the 1999 operations manual (ipdb 4446). Read further and the switch walk starts handing
   out SwitchTable's "Switch_8" placeholders and then internal flags like
   "recent_center_trough_hit", while the driver walk runs into the lamps - which is where the
   entries for coils 53, 56, 57, 59, 60 and 62 in the older revision of this file came from. They
   were lamp names, and they are gone.

   The packages' symbols.rom is worth knowing about for anything beyond this - 'SYMBOL TABLE|', a
   u32 count at 0x10, then count * { u32 name_offset, u32 address } with the name at
   offset + 0x25400. That names every function and table in the image.

   The packages' symbols.rom is worth knowing about for anything beyond this: it names every
   function and table in the image. "SYMBOL TABLE" at 0x00, a u32 count at 0x10, then the entries
   from 0x18 as count * { u32 address, u32 name_offset }, sorted by address, with the names based
   at the end of the entries - 0x18 + count * 8. Checked against rfm_160, rfm_260, swep1_150,
   swep1_166r2 and swep1_210: the first entry is first(void) at 0x100000 in each, and
   wms_pdb_fuse_status(unsigned char &, unsigned char &) resolves in all five.

   An earlier version of this note had the pair the other way round and the names at a fixed
   +0x25400, which parses to nothing but truncated fragments - worth saying, because the wrong
   version is convincing enough to waste an hour on.

   Lamps were taken from the operations manuals, and the image's own lamp table has since been
   found, which confirms them. It sits with the other two: 0x24-byte records of four language
   pointers then five small fields, 128 of them, with one lead-in record before lamp 0. Its order
   is not PinMAME's - matrix A is a block of 64 and matrix B another, each column-major:

       i < 64 ? A : B,  column = (i % 64) / 8 + 1,  row = (i % 64) % 8 + 1

   and the rule below turns that into a lamp number. Read out of Revenge From Mars 1.60 it agrees
   with 107 of the 116 entries here and every position; the nine differences left are all wording,
   the ROM being terser ("Bottom Jet" for "Bottom Jet Bumper", "R. Top." for "R. Top", "Left Of"
   for "Left of"), and the manual's is kept for those. Lamp 109 was the tenth and is not wording:
   the manual has "Between U/R Top Lanes" where the ROM has "Between L/R Top Lanes", and L/R is
   right - 108 and 110 are the left and right top lanes, so a lamp between them is not U/R. That
   one now follows the ROM.

   The manuals give each cell as <column><row><matrix> - 13A,
   44B and so on - and the board turns that into PinMAME's matrix through the row banks: eight
   columns of sixteen lamps, in two banks of eight (p2k_state::lpt_w registers 0x06 and 0x07),
   handed over as bank A at byte 2c and bank B at byte 2c+1. The banks interleave per column, so

       lamp = (column - 1) * 16 + (matrix == B ? 8 : 0) + (row - 1) + 1

   Column 1 row 1 bank A is byte 0 bit 0, and that is lamp 1: these are the numbers a table asks
   for, not the matrix positions underneath. The + 1 is not cosmetic. PinMAME numbers lamps from
   one - vp_getLamp(n) reads matrix position n - 1, and libpinmame's m2lamp arrives at the same -
   so a 0-based table sits one below every Lamp() call in a VPX script, which is what these did
   until it was reported. Coils are 1-based as well, being the manuals' driver numbers, and the
   switch tables carry the game's own column * 10 + row.

   Measured, not derived. Walking swep1_150's own lamp test, which lights one lamp at a time and
   names it on screen, against P2K_LAMPWATCH=1:

       13A -> 3    15A -> 5    16A -> 6    25A -> 21   26A -> 22
       87A -> 119  13B -> 11   24B -> 28   47B -> 63

   all nine agreeing with the rule above. An earlier revision of this file used
   (column-1)*8 + (row-1) - 2, with matrix B at +64, which assumed bytes 0-7 held one whole matrix
   and 8-15 the other. Every lamp in both games was wrong by it. The tables here were remapped from
   it by inverting to (column, row, bank) and re-applying the rule above - note when reading old
   notes that its matrix B began at 62, being 11B, not at 64.

   A second check fell out of that. Ten lamps flicker together on a one to three frame period in
   attract mode; under the old numbering they were an unrelated scatter including Coin Door
   Illumination and a G.I. string, and under this one they are 8, 9, 11, 24, 25, 26, 40, 41, 56 and
   79 - the whole ship, wings, body and tail, pulsing as one object. That is the playfield, not a
   sampling artifact.

   Revenge From Mars was remapped by the same transform, and its table has since been walked whole
   against the machine's own lamp test and matches throughout. Its matrix B had never been
   transcribed past 52B, though - the older revision simply stopped there - so 53B to 88B were read
   off the manual's own grid (page 90 of the February 1999 operations manual, ipdb 4446) and added,
   29 cells. That extraction was checked before being trusted: it
   reproduces every one of the 34 cells the table already held, and independently marks 38B and 48B
   NOT USED, which the table already lacked.

   One disagreement came out of it, and the machine settled it against the manual. Page 90 puts Left
   Slingshot Spotlight at 18B and Right Slingshot Spotlight at 28B; this table has the two the other
   way round, and the lamp test agrees with the table. So that one pair in the manual's grid is
   printed swapped - the only cell in either matrix where it is wrong. Do not "correct" 15 and 31
   back to it when re-deriving.

   What is still unnamed is exactly what the manuals mark NOT USED: twelve cells on Revenge From
   Mars - 11A, 12A, 14A, 22A, 31A to 34A, 58A, 38B, 48B and 72B - and twenty-one on Episode I.
   Neither game drives any of them, so nothing that lights is nameless now.

   Being manual-sourced, the lamp names are the manual's words rather than the game's, unlike the
   switches and coils, which came out of the games' own device tables. Finding the lamp table in the
   image would close that gap, and is easier now than it was: every index below is confirmed against
   the machine, so they are a crib. Search game.rom for a name that is unique to lamps - "Saucer Rim
   1" on Revenge From Mars, "Ship Tail Upper" on Episode I - and check whether the pointer to it
   sits at a stride from the pointer to another whose index is known. That is how the switch and
   coil tables were found; it was not worth attempting while the numbering itself was still wrong.

   ALL SIX TABLES HAVE NOW BEEN WALKED AGAINST THE MACHINES. Both games' switch, coil and lamp
   tests were stepped through with P2K_SWWATCH / P2K_SOLWATCH / P2K_LAMPWATCH and every entry
   matches the name the game puts on screen - the 'E', 'J', 'I', 'D' at 52-55 on Episode I included,
   which are one column of bank A and the insert layout, not a fault. So these are measured against
   the hardware, not transcribed and hoped for, and a mismatch appearing later means something in
   the I/O path changed rather than a bad name.

   Keep it that way when adding entries. Nothing in this file is hard to get plausibly wrong and
   impossible to notice: the lamp numbering was wrong in both games for a while and looked entirely
   coherent, because the names had been fitted to a rule rather than to a playfield. Names grouping
   sensibly is not evidence. The switch test is.

   The part number against each coil is from the same manuals' solenoid tables - the coil wound on
   that driver, or the bulb behind that flasher. It is there for whoever implements the modulated
   outputs, which this machine does not have yet (see the note by p2k_getSol in src/wpc/p2k.c):
   core_set_pwm_output_type() wants exactly this, a physical model per output, and the four families
   present are enough to pick from.

       AE1-xx-yyyy / AE-26-1200   coils, the yyyy being the winding
       SM1-26-600                 the drop target down coils
       FL1-xxxxx                  the flipper-style circuits, power and hold on one part
       A-14406, 20-10197, A-23157 gate, magnet and Episode I's neon
       #906, #89                  flasher bulbs

   Six are marked "kit" instead: Revenge From Mars 18/19 and Episode I 5/42/43/44 are the drives
   myPinballs repurpose, which the factory left unused and so unlisted. None of this hardware is
   stock - no Pinball 2000 shipped with a knocker, a shaker or a topper, whatever the test menu's
   own "Knocker Test" entry suggests. Their instructions recommend a 26-1200 for the knocker; the
   shaker and the topper are a motor and a lamp, not coils. Lamp bulb types are in the manuals too, on the Lamp Locations pages, but are not transcribed here yet */
#ifndef P2K_NAMES_H
#define P2K_NAMES_H

/* What is on the end of a driver output, where the manuals name a part. Only the coil tables carry
   it: it decides which physical model PinMAME's PWM integrator uses for that output, and a coil is
   the default, so nothing else has to say anything. The bulb numbers are the manuals' own - the
   comment after each entry is where they came from */
enum { P2K_DEV_COIL = 0, P2K_DEV_BULB_89, P2K_DEV_BULB_906 };

typedef struct { int num; const char *name; int dev; } p2k_name_t;

/* which game's tables a caller wants */
enum { P2K_GAME_RFM = 0, P2K_GAME_SWEP1 = 1 };

/* switch number = 100 + (column-1)*8 + (row-1); PinMAME numbers it column*10 + row */
static const p2k_name_t p2k_rfm_switch_names[] = {
  {  11, "Right Ramp Entrance" },
  {  12, "Left Ramp Exit" },
  {  13, "Start Button" },
  {  15, "Drop Target Down" },
  {  16, "Left Outlane" },
  {  17, "Right Return Lane" },
  {  18, "Shooter Lane" },
  {  23, "Launch Button" },
  {  25, "Left Loop (Low)" },
  {  26, "Left Return Lane" },
  {  27, "Right Outlane" },
  {  28, "Right Ramp Exit" },
  {  31, "Center Loop Reed (Bottom)" },
  {  32, "Center Loop Reed (Top)" },
  {  33, "Center Target 4" },
  {  34, "Center Target 3" },
  {  35, "Center Target 2" },
  {  36, "Center Target 1" },
  {  37, "Martian Target 4 (Center)" },
  {  38, "Up/Down Ramp Up" },
  {  41, "Trough Jam" },
  {  42, "Trough Ball 1" },
  {  43, "Trough Ball 2" },
  {  44, "Trough Ball 3" },
  {  45, "Trough Ball 4" },
  {  46, "Right Popper" },
  {  47, "Jet Exit" },
  {  51, "Right Lockup 1" },
  {  52, "Left Ramp Entrance" },
  {  53, "Trough Ball 5" },
  {  54, "Trough Ball 6" },
  {  55, "Right Lockup 2" },
  {  56, "Right Lockup 3" },
  {  61, "Left Slingshot" },
  {  62, "Right Slingshot" },
  {  63, "Left Jet" },
  {  64, "Right Jet" },
  {  65, "Bottom Jet" },
  {  67, "Right Loop (Low)" },
  {  68, "Right Loop (High)" },
  {  71, "Martian Target 3 (L. Top.)" },
  {  72, "Martian Target 2 (L. Mid.)" },
  {  73, "Martian Target 1 (L. Bot.)" },
  {  74, "Center Loop Rollover" },
  {  75, "Center Deflector Panel" },
  {  76, "Right Top Lane" },
  {  77, "Left Top Lane" },
  {  78, "Left Loop (High)" },
  {  85, "Martian Target 7 (R. Bot.)" },
  {  86, "Martian Target 6 (R. Mid.)" },
  {  87, "Martian Target 5 (R. Top.)" },
  {  91, "LEFT COIN SLOT" },
  {  92, "CENTER COIN SLOT" },
  {  93, "RIGHT COIN SLOT" },
  {  94, "4th Coin Option" },
  { 101, "'ESCAPE' BUTTON" },
  { 102, "'Down' Button" },
  { 103, "'Up' Button" },
  { 104, "'Enter' Button" },
  { 105, "Right Flipper EOS" },
  { 106, "Left Flipper EOS" },
  { 111, "Slam Tilt" },
  { 112, "Coin Door Closed" },
  { 113, "Plumb Bob Tilt" },
  { 115, "Right Flipper Button" },
  { 116, "Left Flipper Button" },
  { 117, "Right Action Button" },
  { 118, "Left Action Button" },
  { 0, NULL }
};

static const p2k_name_t p2k_swep1_switch_names[] = {
  {  13, "Start Button" },
  {  15, "Left Drop Target" },
  {  16, "Left Outlane" },
  {  17, "Right Inlane" },
  {  18, "Shooter Lane" },
  {  21, "Captive Ball" },
  {  23, "Launch Button" },
  {  24, "Always Closed" },
  {  25, "Right Drop Target" },
  {  26, "Left Inlane" },
  {  27, "Right Outlane" },
  {  28, "Sneaky Lane" },
  {  31, "Right Bank: Upper" },
  {  32, "Right Bank: Middle" },
  {  33, "Right Bank: Lower" },
  {  34, "Left Bank: Upper" },
  {  35, "Left Bank Middle" },
  {  36, "Left Bank Lower" },
  {  37, "Left Saucer" },
  {  38, "Right Saucer" },
  {  41, "Trough Jam" },
  {  42, "Trough Ball 1" },
  {  43, "Trough Ball 2" },
  {  44, "Trough Ball 3" },
  {  45, "Trough Ball 4" },
  {  46, "Left Ramp Enter" },
  {  47, "Right Ramp Enter" },
  {  48, "Trough Ball 5" },
  {  51, "Shield Popper" },
  {  52, "Trough Ball 6" },
  {  53, "Left Shield Target" },
  {  54, "Right Shield Target" },
  {  55, "Ramp Made Left" },
  {  56, "Ramp Made Right" },
  {  57, "Shield Up" },
  {  58, "Shield Hit" },
  {  61, "Left Sling" },
  {  62, "Right Sling" },
  {  63, "Upper Jet" },
  {  64, "Middle Jet" },
  {  65, "Lower Jet" },
  {  66, "Jets Rollover" },
  {  67, "Left Loop Upper" },
  {  68, "Left Loop Rollover" },
  {  91, "Left Coin Slot" },
  {  92, "Center Coin Slot" },
  {  93, "Right Coin Slot" },
  {  94, "4th Coin Option" },
  { 101, "'Escape' Button" },
  { 102, "'Down' Button" },
  { 103, "'Up' Button" },
  { 104, "'Enter' Button" },
  { 105, "Lower/Right flipper EOS" },
  { 106, "Lower/Left flipper EOS" },
  { 107, "Upper/Right flipper EOS" },
  { 108, "Upper/Left flipper EOS" },
  { 111, "Slam Tilt" },
  { 112, "Coin Door Closed" },
  { 113, "Plumb Bob Tilt" },
  { 115, "Right flipper button" },
  { 116, "Left flipper button" },
  { 117, "Right Action Button" },
  { 118, "Left Action Button" },
  { 0, NULL }
};

/* Driver numbers as the power driver board counts them, which is what the games' own coil tests
   print and what the manuals list. 33-40 are the FL1 power/hold pairs, flippers first.

   These are NOT all PinMAME solenoid numbers, and a table script has to know it. Only 1-32 line up;
   above that the core reserves fixed slots and p2k_solIndex() places them accordingly:

       driver 1-32   ->  solenoid 1-32
       driver 33-36  ->  solenoid 45-48   the lower flipper slots, where core_getSol() looks
       driver 37-48  ->  solenoid 51-62   the custom solenoids, answered by p2k_getSol()

   So Solenoid(45) is the right flipper's power coil, not driver 45. Lamps had the same class of
   problem - see the numbering note at the top of this file - and switches do not, the game's own
   column * 10 + row being exactly what core_getSw() takes */
static const p2k_name_t p2k_rfm_coil_names[] = {
  {   1, "Left Martian" },                     /* AE1-26-1500 */
  {   2, "Right Martian" },                    /* AE1-26-1500 */
  {   3, "Jet Exit Post" },                    /* AE1-26-1500 */
  {   4, "Right Gate" },                       /* A-14406 */
  {   5, "Left Gate" },                        /* A-14406 */
  {   6, "Drop Target Down" },                 /* SM1-26-600 */
  {   7, "Drop Target Up" },                   /* AE1-26-1200 */
  {   8, "Right Popper" },                     /* AE1-25-1000 */
  {   9, "Trough Eject" },                     /* AE1-26-1500 */
  {  10, "Left Sling" },                       /* AE1-26-1200 */
  {  11, "Right Sling" },                      /* AE1-26-1200 */
  {  12, "Left Jet" },                         /* AE1-26-1200 */
  {  13, "Right Jet" },                        /* AE1-26-1200 */
  {  14, "Bottom Jet" },                       /* AE1-26-1200 */
  {  15, "Autoplunger" },                      /* AE1-23-800 */
  {  16, "Right Lockup" },                     /* AE1-23-800 */
  {  17, "Center Arrow Flasher", P2K_DEV_BULB_906 }, /* #906 */
  {  18, "Knocker (Optional)" },               /* AE-26-1200, kit */
  {  19, "Shaker (Optional)" },                /* motor, kit */
  {  22, "Right Popper Flasher", P2K_DEV_BULB_906 }, /* #906 */
  {  23, "Left Arch Flasher", P2K_DEV_BULB_89 }, /* #89 */
  {  25, "Right Arch Flasher", P2K_DEV_BULB_89 }, /* #89 */
  {  26, "Left Martian Flasher", P2K_DEV_BULB_89 }, /* #89 */
  {  27, "Right Martian Flasher", P2K_DEV_BULB_89 }, /* #89 */
  {  28, "Attack Mars Flasher", P2K_DEV_BULB_906 }, /* #906 */
  {  33, "Right Flipper Power" },              /* FL1-11629 */
  {  34, "Right Flipper Hold" },               /* FL1-11629 */
  {  35, "Left Flipper Power" },               /* FL1-11629 */
  {  36, "Left Flipper Hold" },                /* FL1-11629 */
  {  37, "Lock Diverter Power" },              /* FL1-22241 */
  {  38, "Lock Diverter Hold" },               /* FL1-22241 */
  {  39, "Up/Down Ramp Power" },               /* FL1-11753 */
  {  40, "Up/Down Ramp Hold" },                /* FL1-11753 */
  {  48, "Ticket Dispenser" },
  { 0, NULL }
};

static const p2k_name_t p2k_swep1_coil_names[] = {
  {   1, "Left Saucer" },                      /* AE1-27-1200 */
  {   2, "Left Drop Target Up" },              /* AE1-26-1200 */
  {   3, "Left Drop Target Down" },            /* SM1-26-600 */
  {   4, "Magnet" },                           /* 20-10197 */
  {   5, "Auto Plunger" },                     /* kit */
  {   6, "Right Drop Target Down" },           /* SM1-26-600 */
  {   7, "Right Drop Target Up" },             /* AE1-26-1200 */
  {   8, "Shield Popper" },                    /* AE1-26-1500 */
  {   9, "Trough Eject" },                     /* AE1-26-1500 */
  {  10, "Left Sling" },                       /* AE1-27-1200 */
  {  11, "Right Sling" },                      /* AE1-27-1200 */
  {  12, "Upper Jet" },                        /* AE1-26-1200 */
  {  13, "Middle Jet" },                       /* AE1-26-1200 */
  {  14, "Lower Jet" },                        /* AE1-26-1200 */
  {  15, "Upper Hotdog Flashers", P2K_DEV_BULB_906 }, /* #906 (2) */
  {  16, "Right Saucer" },                     /* AE1-27-1200 */
  {  17, "Lower Left Hotdog Fl.", P2K_DEV_BULB_906 }, /* #906 */
  {  18, "Lower Right Hotdog Fl.", P2K_DEV_BULB_906 }, /* #906 */
  {  19, "Back Panel right/upper fl.", P2K_DEV_BULB_906 }, /* #906 */
  {  20, "Back Panel right/middle fl.", P2K_DEV_BULB_906 }, /* #906 */
  {  21, "Jet Flasher", P2K_DEV_BULB_906 }, /* #906 */
  {  22, "Left Inlanes Flasher", P2K_DEV_BULB_89 }, /* #89 */
  {  23, "Right Inlanes Flasher", P2K_DEV_BULB_89 }, /* #89 */
  {  24, "Back Panel Middle fl.", P2K_DEV_BULB_906 }, /* #906 */
  {  25, "Back Panel right/lower fl.", P2K_DEV_BULB_906 }, /* #906 */
  {  26, "Back Panel left/upper fl.", P2K_DEV_BULB_906 }, /* #906 */
  {  27, "Back Panel left/middle fl.", P2K_DEV_BULB_906 }, /* #906 */
  {  28, "Back Panel left/lower fl.", P2K_DEV_BULB_906 }, /* #906 */
  {  33, "Right Flipper Power" },              /* FL1-11722 */
  {  34, "Right Flipper Hold" },               /* FL1-11722 */
  {  35, "Left Flipper Power" },               /* FL1-11722 */
  {  36, "Left Flipper Hold" },                /* FL1-11722 */
  {  37, "Shield Power" },                     /* FL1-15411 */
  {  38, "Shield Hold" },                      /* FL1-15411 */
  {  39, "Left Laser Flasher", P2K_DEV_BULB_89 }, /* #89 */
  {  40, "Right Laser Flasher", P2K_DEV_BULB_89 }, /* #89 */
  {  41, "Neon" },                             /* A-23157 */
  {  42, "Knocker (Optional)" },               /* AE-26-1200, kit */
  {  43, "Shaker Motor (Optional)" },          /* motor, kit */
  {  44, "Topper (Optional)" },                /* kit */
  {  48, "Ticket Dispenser" },
  { 0, NULL }
};

static const p2k_name_t p2k_rfm_lamp_names[] = {
  {   3, "Start Button" },
  {   5, "Right Top Lane" },
  {   6, "Left Top Lane" },
  {   7, "Martian Target 4 (Center)" },
  {   8, "Center Loop Arrow" },
  {   9, "Secret Weapon" },
  {   10, "Tower Struggle" },
  {  11, "Center Saucer Beam (Left)" },
  {  12, "Question Mark" },
  {  13, "Center Saucer Beam (Right)" },
  {  14, "Drive-In Demolition" },
  {  15, "Paris In Peril" },
  {  16, "Right Slingshot Spotlight" },
  {  17, "Tickets Low" },
  {  19, "Launch Button" },
  {  20, "Coin Door Illumination" },
  {  21, "Mothership Multiball (Right)" },
  {  22, "Mothership Multiball (Left)" },
  {  23, "Left Return Lane" },
  {  24, "Left Outlane" },
  {  25, "Big-O-Beam" },
  {  26, "Right Saucer Beam (Left)" },
  {  27, "Weapons" },
  {  28, "Saucer" },
  {  29, "Fuel" },
  {  30, "Left Saucer Beam (Right)" },
  {  31, "Center Saucer Beam (Center)" },
  {  32, "Left Slingshot Spotlight" },
  {  37, "Left Drain To Trough" },
  {  38, "Right Drain To Trough" },
  {  39, "Right Return Lane" },
  {  40, "Right Outlane" },
  {  41, "Mars Kneads Women" },
  {  42, "Right Saucer Beam (Right)" },
  {  43, "Saucer Rim 9 (Right)" },
  {  44, "Saucer Rim 8" },
  {  45, "Saucer Rim 7" },
  {  46, "Saucer Rim 6" },
  {  47, "Saucer Rim 5" },
  {  49, "Right Popper Arrow" },
  {  50, "Extra Ball" },
  {  51, "Martian Attack" },
  {  52, "Stroke Of Luck" },
  {  53, "Left Side Spotlight" },
  {  54, "Center Arrow" },
  {  55, "Right Martian (High)" },
  {  56, "Right Martian (Low)" },
  {  57, "Martian Happy Hour" },
  {  58, "Alien Abduction" },
  {  59, "Left Saucer Beam (Left)" },
  {  60, "Saucer Rim 1 (Left)" },
  {  61, "Saucer Rim 2" },
  {  62, "Saucer Rim 3" },
  {  63, "Saucer Rim 4" },
  {  65, "Multiball" },
  {  66, "Capture 2" },
  {  67, "Capture 1" },
  {  68, "Capture Zone Active" },
  {  69, "Shoot Again" },
  {  70, "Behind Center Targets" },
  {  71, "Upper R. Corner (Middle)" },
  {  73, "Right Loop Arrow" },
  {  74, "Right Loop Circle" },
  {  75, "Right Ramp Arrow" },
  {  76, "Right Ramp Circle" },
  {  77, "Left Loop Arrow" },
  {  78, "Left Ramp Arrow" },
  {  79, "Left Loop Circle" },
  {  80, "Left Ramp Circle" },
  {  81, "Shooter Lane 9 (Top)" },
  {  82, "Under R. Ramp (Low)" },
  {  83, "Under R. Ramp (High)" },
  {  84, "Upper R. Corner (Low)" },
  {  85, "Right Arch (Right)" },
  {  86, "Right Arch (Left)" },
  {  87, "Left Arch (Right)" },
  {  88, "Left Arch (Left)" },
  {  89, "Martian Target 5 (R. Top)" },
  {  90, "Martian Target 6 (R. Mid.)" },
  {  91, "Martian Target 7 (R. Bot.)" },
  {  92, "Martian Target 3 (Left Top)" },
  {  93, "Martian Target 2 (Left Mid.)" },
  {  94, "Martian Target 1 (Left Bot.)" },
  {  95, "Right Martian Eye" },
  {  96, "Left Martian Eye" },
  {  97, "Left Side 1 (Bottom)" },
  {  98, "Left Side 2" },
  {  99, "Left Side 3" },
  {  100, "Left Side 4 (Top)" },
  { 101, "Under Left Ramp (Bottom)" },
  { 102, "Under Left Ramp (Top)" },
  { 103, "Between L/B Jets" },
  { 104, "Upper Left Corner" },
  { 105, "Bottom Jet Bumper" },
  { 107, "Left Jet Bumper" },
  { 108, "Left of Left Top Lane" },
  { 109, "Between L/R Top Lanes" },
  { 110, "Right of Right Top Lane" },
  { 111, "Top of Center Loop" },
  { 112, "Upper R. Corner (High)" },
  { 113, "Right Slingshot (Bottom)" },
  { 114, "Right Slingshot (Saucer)" },
  { 115, "Right Return Lane (Right)" },
  { 116, "Right Return Lane (Left)" },
  { 117, "Left Return Lane (Right)" },
  { 118, "Left Return Lane (Left)" },
  { 119, "Left Slingshot (Saucer)" },
  { 120, "Left Slingshot (Bottom)" },
  { 121, "Shooter Lane 1 (Bottom)" },
  { 122, "Shooter Lane 2" },
  { 123, "Shooter Lane 3" },
  { 124, "Shooter Lane 4" },
  { 125, "Shooter Lane 5" },
  { 126, "Shooter Lane 6" },
  { 127, "Shooter Lane 7" },
  { 128, "Shooter Lane 8" },
  { 0, NULL }
};

static const p2k_name_t p2k_swep1_lamp_names[] = {
  {   3, "Start Button" },
  {   5, "Shield Lower Right" },
  {   6, "Shield Lower 4" },
  {   7, "Shield Lower 3" },
  {   8, "Shield Lower 2" },
  {   9, "Ship Right Wing Upper" },
  {   10, "Ship Right Wing Lower" },
  {  11, "Bonus X5" },
  {  12, "Ship Tail Upper" },
  {  13, "Jedi Spirit" },
  {  14, "Right Hotdog Left" },
  {  15, "Jets Rollover" },
  {  16, "Right Laser End" },
  {  17, "Tickets Low" },
  {  20, "Coin Door Illumination" },
  {  21, "Shield Middle Right" },
  {  22, "Shield Middle 3" },
  {  23, "Shield Middle 2" },
  {  24, "Shield Lower Left" },
  {  25, "Ship Body Upper Right" },
  {  26, "Ship Body Middle" },
  {  27, "Ship Body Lower" },
  {  28, "Bonus X4" },
  {  29, "Jedi Master" },
  {  30, "Fire Lasers Right" },
  {  31, "Right Saucer" },
  {  32, "Extra Ball" },
  {  37, "Shield Upper Right" },
  {  38, "Shield Upper Middle" },
  {  39, "Shield Middle Left" },
  {  40, "Shield Upper Left" },
  {  41, "Ship Left Wing Upper" },
  {  42, "Ship Left Wing Lower" },
  {  43, "Bonus X2" },
  {  44, "Bonus X3" },
  {  45, "Jedi Youth" },
  {  46, "Left Hotdog Right" },
  {  47, "Shooter" },
  {  48, "Bottom Arch Right/Left" },
  {  49, "Left Loop Right Leg" },
  {  50, "Left Loop Right Foot" },
  {  51, "Left Loop Left Foot" },
  {  52, "Left Loop Left Leg" },
  {  53, "Jedi 'E'" },
  {  54, "Jedi 'J'" },
  {  55, "Jedi 'I'" },
  {  56, "Jedi 'D'" },
  {  57, "Ship Body Upper Left" },
  {  60, "Spotlight Right" },
  {  61, "Fire Lasers Left" },
  {  62, "Jedi Knight" },
  {  63, "Shoot Again" },
  {  64, "Left Flipper" },
  {  65, "Left Loop Body Middle" },
  {  66, "Left Loop Body Upper" },
  {  67, "Left Loop Head" },
  {  68, "Left Loop Body Lower" },
  {  69, "Right Ramp G.I." },
  {  70, "Scoop Lower Right G.I." },
  {  71, "Left Loop Rollover" },
  {  72, "Left Saucer" },
  {  73, "Left Laser End" },
  {  74, "Left Saucer Insert" },
  {  75, "Right Saucer Insert" },
  {  76, "Spotlight Left" },
  {  77, "Left Hotdog Left" },
  {  78, "Right Hotdog Right" },
  {  79, "Right Flipper" },
  {  80, "Ship Tail Lower" },
  {  81, "Right Standup Upper" },
  {  82, "Right Standup Middle" },
  {  83, "Right Standup Lower" },
  {  84, "Left Standup Lower" },
  {  85, "Left Standup Middle" },
  {  86, "Left Standup Upper" },
  {  97, "Bottom Arch Left/Left" },
  {  98, "Left Inlane G.I. Right" },
  {  99, "Left Sling G.I. Upper" },
  {  100, "Left Loop Lower G.I." },
  { 101, "Captive Ball G.I." },
  { 102, "Scoop Lower Left G.I." },
  { 103, "Scoop Upper Left G.I." },
  { 104, "Jets Top G.I." },
  { 105, "Bottom Arch Left/Right" },
  { 106, "Left Inlane G.I. Left" },
  { 107, "Left Sling G.I. Lower" },
  { 108, "Left Outlane G.I." },
  { 109, "Left Standup G.I." },
  { 110, "Left Loop Middle G.I." },
  { 111, "Upper Left Corner G.I." },
  { 112, "Left Ramp G.I." },
  { 113, "Bottom Arch Right/Right" },
  { 114, "Right Inlane G.I. Left" },
  { 115, "Right Sling G.I. Upper" },
  { 116, "Shooter Ramp G.I. Lower" },
  { 117, "Shoot Ramp G.I. Middle" },
  { 118, "Right Standup G.I." },
  { 119, "Middle Jet" },
  { 120, "Jet Middle G.I." },
  { 121, "Scoop Upper Right G.I." },
  { 122, "Upper Right Corner G.I." },
  { 123, "Upper Jet" },
  { 124, "Lower Jet" },
  { 125, "Shooter Ramp G.I. Upper" },
  { 126, "Right Outlane G.I." },
  { 127, "Right Sling G.I. Lower" },
  { 128, "Right Inlane G.I. Right" },
  { 0, NULL }
};

/* Which game's tables to use - the driver knows from its set name (p2k_romPrefix) */
static const p2k_name_t *p2k_switch_names(int game) {
  return game ? p2k_swep1_switch_names : p2k_rfm_switch_names;
}
static const p2k_name_t *p2k_coil_names(int game) {
  return game ? p2k_swep1_coil_names : p2k_rfm_coil_names;
}
static const p2k_name_t *p2k_lamp_names(int game) {
  return game ? p2k_swep1_lamp_names : p2k_rfm_lamp_names;
}

static const char *p2k_lookup(const p2k_name_t * const t, int num) {
  int i; for (i = 0; t[i].name; i++) if (t[i].num == num) return t[i].name;
  return NULL;
}

#endif /* P2K_NAMES_H */
