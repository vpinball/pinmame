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

   One table cannot be right for both, and this one follows 2.10, so on swep1_166 driver 5 reads
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
   swep1_166 and swep1_210: the first entry is first(void) at 0x100000 in each, and
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
   for "Left of"), and the manual's is kept for those. Lamp 108 was the tenth and is not wording:
   the manual has "Between U/R Top Lanes" where the ROM has "Between L/R Top Lanes", and L/R is
   right - 107 and 109 are the left and right top lanes, so a lamp between them is not U/R. That
   one now follows the ROM.

   The manuals give each cell as <column><row><matrix> - 13A,
   44B and so on - and the board turns that into PinMAME's matrix through the row banks: eight
   columns of sixteen lamps, in two banks of eight (p2k_state::lpt_w registers 0x06 and 0x07),
   handed over as bank A at byte 2c and bank B at byte 2c+1. The banks interleave per column, so

       lamp = (column - 1) * 16 + (matrix == B ? 8 : 0) + (row - 1)

   There is no offset: column 1 row 1 bank A is byte 0 bit 0, so 11A is lamp 0.

   Measured, not derived. Walking swep1_150's own lamp test, which lights one lamp at a time and
   names it on screen, against P2K_LAMPWATCH=1:

       13A -> 2    15A -> 4    16A -> 5    25A -> 20   26A -> 21
       87A -> 118  13B -> 10   24B -> 27   47B -> 62

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

typedef struct { int num; const char *name; } p2k_name_t;

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

/* driver/coil numbers as the power driver board counts them; 33-40 are the flipper circuits */
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
  {  17, "Center Arrow Flasher" },             /* #906 */
  {  18, "Knocker (Optional)" },               /* AE-26-1200, kit */
  {  19, "Shaker (Optional)" },                /* motor, kit */
  {  22, "Right Popper Flasher" },             /* #906 */
  {  23, "Left Arch Flasher" },                /* #89 */
  {  25, "Right Arch Flasher" },               /* #89 */
  {  26, "Left Martian Flasher" },             /* #89 */
  {  27, "Right Martian Flasher" },            /* #89 */
  {  28, "Attack Mars Flasher" },              /* #906 */
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
  {  15, "Upper Hotdog Flashers" },            /* #906 (2) */
  {  16, "Right Saucer" },                     /* AE1-27-1200 */
  {  17, "Lower Left Hotdog Fl." },            /* #906 */
  {  18, "Lower Right Hotdog Fl." },           /* #906 */
  {  19, "Back Panel right/upper fl." },       /* #906 */
  {  20, "Back Panel right/middle fl." },      /* #906 */
  {  21, "Jet Flasher" },                      /* #906 */
  {  22, "Left Inlanes Flasher" },             /* #89 */
  {  23, "Right Inlanes Flasher" },            /* #89 */
  {  24, "Back Panel Middle fl." },            /* #906 */
  {  25, "Back Panel right/lower fl." },       /* #906 */
  {  26, "Back Panel left/upper fl." },        /* #906 */
  {  27, "Back Panel left/middle fl." },       /* #906 */
  {  28, "Back Panel left/lower fl." },        /* #906 */
  {  33, "Right Flipper Power" },              /* FL1-11722 */
  {  34, "Right Flipper Hold" },               /* FL1-11722 */
  {  35, "Left Flipper Power" },               /* FL1-11722 */
  {  36, "Left Flipper Hold" },                /* FL1-11722 */
  {  37, "Shield Power" },                     /* FL1-15411 */
  {  38, "Shield Hold" },                      /* FL1-15411 */
  {  39, "Left Laser Flasher" },               /* #89 */
  {  40, "Right Laser Flasher" },              /* #89 */
  {  41, "Neon" },                             /* A-23157 */
  {  42, "Knocker (Optional)" },               /* AE-26-1200, kit */
  {  43, "Shaker Motor (Optional)" },          /* motor, kit */
  {  44, "Topper (Optional)" },                /* kit */
  {  48, "Ticket Dispenser" },
  { 0, NULL }
};

static const p2k_name_t p2k_rfm_lamp_names[] = {
  {   2, "Start Button" },
  {   4, "Right Top Lane" },
  {   5, "Left Top Lane" },
  {   6, "Martian Target 4 (Center)" },
  {   7, "Center Loop Arrow" },
  {   8, "Secret Weapon" },
  {   9, "Tower Struggle" },
  {  10, "Center Saucer Beam (Left)" },
  {  11, "Question Mark" },
  {  12, "Center Saucer Beam (Right)" },
  {  13, "Drive-In Demolition" },
  {  14, "Paris In Peril" },
  {  15, "Right Slingshot Spotlight" },
  {  16, "Tickets Low" },
  {  18, "Launch Button" },
  {  19, "Coin Door Illumination" },
  {  20, "Mothership Multiball (Right)" },
  {  21, "Mothership Multiball (Left)" },
  {  22, "Left Return Lane" },
  {  23, "Left Outlane" },
  {  24, "Big-O-Beam" },
  {  25, "Right Saucer Beam (Left)" },
  {  26, "Weapons" },
  {  27, "Saucer" },
  {  28, "Fuel" },
  {  29, "Left Saucer Beam (Right)" },
  {  30, "Center Saucer Beam (Center)" },
  {  31, "Left Slingshot Spotlight" },
  {  36, "Left Drain To Trough" },
  {  37, "Right Drain To Trough" },
  {  38, "Right Return Lane" },
  {  39, "Right Outlane" },
  {  40, "Mars Kneads Women" },
  {  41, "Right Saucer Beam (Right)" },
  {  42, "Saucer Rim 9 (Right)" },
  {  43, "Saucer Rim 8" },
  {  44, "Saucer Rim 7" },
  {  45, "Saucer Rim 6" },
  {  46, "Saucer Rim 5" },
  {  48, "Right Popper Arrow" },
  {  49, "Extra Ball" },
  {  50, "Martian Attack" },
  {  51, "Stroke Of Luck" },
  {  52, "Left Side Spotlight" },
  {  53, "Center Arrow" },
  {  54, "Right Martian (High)" },
  {  55, "Right Martian (Low)" },
  {  56, "Martian Happy Hour" },
  {  57, "Alien Abduction" },
  {  58, "Left Saucer Beam (Left)" },
  {  59, "Saucer Rim 1 (Left)" },
  {  60, "Saucer Rim 2" },
  {  61, "Saucer Rim 3" },
  {  62, "Saucer Rim 4" },
  {  64, "Multiball" },
  {  65, "Capture 2" },
  {  66, "Capture 1" },
  {  67, "Capture Zone Active" },
  {  68, "Shoot Again" },
  {  69, "Behind Center Targets" },
  {  70, "Upper R. Corner (Middle)" },
  {  72, "Right Loop Arrow" },
  {  73, "Right Loop Circle" },
  {  74, "Right Ramp Arrow" },
  {  75, "Right Ramp Circle" },
  {  76, "Left Loop Arrow" },
  {  77, "Left Ramp Arrow" },
  {  78, "Left Loop Circle" },
  {  79, "Left Ramp Circle" },
  {  80, "Shooter Lane 9 (Top)" },
  {  81, "Under R. Ramp (Low)" },
  {  82, "Under R. Ramp (High)" },
  {  83, "Upper R. Corner (Low)" },
  {  84, "Right Arch (Right)" },
  {  85, "Right Arch (Left)" },
  {  86, "Left Arch (Right)" },
  {  87, "Left Arch (Left)" },
  {  88, "Martian Target 5 (R. Top)" },
  {  89, "Martian Target 6 (R. Mid.)" },
  {  90, "Martian Target 7 (R. Bot.)" },
  {  91, "Martian Target 3 (Left Top)" },
  {  92, "Martian Target 2 (Left Mid.)" },
  {  93, "Martian Target 1 (Left Bot.)" },
  {  94, "Right Martian Eye" },
  {  95, "Left Martian Eye" },
  {  96, "Left Side 1 (Bottom)" },
  {  97, "Left Side 2" },
  {  98, "Left Side 3" },
  {  99, "Left Side 4 (Top)" },
  { 100, "Under Left Ramp (Bottom)" },
  { 101, "Under Left Ramp (Top)" },
  { 102, "Between L/B Jets" },
  { 103, "Upper Left Corner" },
  { 104, "Bottom Jet Bumper" },
  { 106, "Left Jet Bumper" },
  { 107, "Left of Left Top Lane" },
  { 108, "Between L/R Top Lanes" },
  { 109, "Right of Right Top Lane" },
  { 110, "Top of Center Loop" },
  { 111, "Upper R. Corner (High)" },
  { 112, "Right Slingshot (Bottom)" },
  { 113, "Right Slingshot (Saucer)" },
  { 114, "Right Return Lane (Right)" },
  { 115, "Right Return Lane (Left)" },
  { 116, "Left Return Lane (Right)" },
  { 117, "Left Return Lane (Left)" },
  { 118, "Left Slingshot (Saucer)" },
  { 119, "Left Slingshot (Bottom)" },
  { 120, "Shooter Lane 1 (Bottom)" },
  { 121, "Shooter Lane 2" },
  { 122, "Shooter Lane 3" },
  { 123, "Shooter Lane 4" },
  { 124, "Shooter Lane 5" },
  { 125, "Shooter Lane 6" },
  { 126, "Shooter Lane 7" },
  { 127, "Shooter Lane 8" },
  { 0, NULL }
};

static const p2k_name_t p2k_swep1_lamp_names[] = {
  {   2, "Start Button" },
  {   4, "Shield Lower Right" },
  {   5, "Shield Lower 4" },
  {   6, "Shield Lower 3" },
  {   7, "Shield Lower 2" },
  {   8, "Ship Right Wing Upper" },
  {   9, "Ship Right Wing Lower" },
  {  10, "Bonus X5" },
  {  11, "Ship Tail Upper" },
  {  12, "Jedi Spirit" },
  {  13, "Right Hotdog Left" },
  {  14, "Jets Rollover" },
  {  15, "Right Laser End" },
  {  16, "Tickets Low" },
  {  19, "Coin Door Illumination" },
  {  20, "Shield Middle Right" },
  {  21, "Shield Middle 3" },
  {  22, "Shield Middle 2" },
  {  23, "Shield Lower Left" },
  {  24, "Ship Body Upper Right" },
  {  25, "Ship Body Middle" },
  {  26, "Ship Body Lower" },
  {  27, "Bonus X4" },
  {  28, "Jedi Master" },
  {  29, "Fire Lasers Right" },
  {  30, "Right Saucer" },
  {  31, "Extra Ball" },
  {  36, "Shield Upper Right" },
  {  37, "Shield Upper Middle" },
  {  38, "Shield Middle Left" },
  {  39, "Shield Upper Left" },
  {  40, "Ship Left Wing Upper" },
  {  41, "Ship Left Wing Lower" },
  {  42, "Bonus X2" },
  {  43, "Bonus X3" },
  {  44, "Jedi Youth" },
  {  45, "Left Hotdog Right" },
  {  46, "Shooter" },
  {  47, "Bottom Arch Right/Left" },
  {  48, "Left Loop Right Leg" },
  {  49, "Left Loop Right Foot" },
  {  50, "Left Loop Left Foot" },
  {  51, "Left Loop Left Leg" },
  {  52, "Jedi 'E'" },
  {  53, "Jedi 'J'" },
  {  54, "Jedi 'I'" },
  {  55, "Jedi 'D'" },
  {  56, "Ship Body Upper Left" },
  {  59, "Spotlight Right" },
  {  60, "Fire Lasers Left" },
  {  61, "Jedi Knight" },
  {  62, "Shoot Again" },
  {  63, "Left Flipper" },
  {  64, "Left Loop Body Middle" },
  {  65, "Left Loop Body Upper" },
  {  66, "Left Loop Head" },
  {  67, "Left Loop Body Lower" },
  {  68, "Right Ramp G.I." },
  {  69, "Scoop Lower Right G.I." },
  {  70, "Left Loop Rollover" },
  {  71, "Left Saucer" },
  {  72, "Left Laser End" },
  {  73, "Left Saucer Insert" },
  {  74, "Right Saucer Insert" },
  {  75, "Spotlight Left" },
  {  76, "Left Hotdog Left" },
  {  77, "Right Hotdog Right" },
  {  78, "Right Flipper" },
  {  79, "Ship Tail Lower" },
  {  80, "Right Standup Upper" },
  {  81, "Right Standup Middle" },
  {  82, "Right Standup Lower" },
  {  83, "Left Standup Lower" },
  {  84, "Left Standup Middle" },
  {  85, "Left Standup Upper" },
  {  96, "Bottom Arch Left/Left" },
  {  97, "Left Inlane G.I. Right" },
  {  98, "Left Sling G.I. Upper" },
  {  99, "Left Loop Lower G.I." },
  { 100, "Captive Ball G.I." },
  { 101, "Scoop Lower Left G.I." },
  { 102, "Scoop Upper Left G.I." },
  { 103, "Jets Top G.I." },
  { 104, "Bottom Arch Left/Right" },
  { 105, "Left Inlane G.I. Left" },
  { 106, "Left Sling G.I. Lower" },
  { 107, "Left Outlane G.I." },
  { 108, "Left Standup G.I." },
  { 109, "Left Loop Middle G.I." },
  { 110, "Upper Left Corner G.I." },
  { 111, "Left Ramp G.I." },
  { 112, "Bottom Arch Right/Right" },
  { 113, "Right Inlane G.I. Left" },
  { 114, "Right Sling G.I. Upper" },
  { 115, "Shooter Ramp G.I. Lower" },
  { 116, "Shoot Ramp G.I. Middle" },
  { 117, "Right Standup G.I." },
  { 118, "Middle Jet" },
  { 119, "Jet Middle G.I." },
  { 120, "Scoop Upper Right G.I." },
  { 121, "Upper Right Corner G.I." },
  { 122, "Upper Jet" },
  { 123, "Lower Jet" },
  { 124, "Shooter Ramp G.I. Upper" },
  { 125, "Right Outlane G.I." },
  { 126, "Right Sling G.I. Lower" },
  { 127, "Right Inlane G.I. Right" },
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

static const char *p2k_lookup(const p2k_name_t *t, int num) {
  int i; for (i = 0; t[i].name; i++) if (t[i].num == num) return t[i].name;
  return NULL;
}

#endif /* P2K_NAMES_H */
