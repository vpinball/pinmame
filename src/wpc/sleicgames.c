// license:BSD-3-Clause

#include "driver.h"
#include "gen.h"
#include "sim.h"
#include "sleic.h"

#define INITGAME(name, disptype, balls) \
	SLEIC_INPUT_PORTS_START(name, balls) SLEIC_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_SLEIC,disptype,{FLIP_SW(FLIP_L)}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

/* Io Moon only: same cabinet block, but the real SW40 DIP block instead of S1..S8,
   because it is the one machine here whose Z80 ROM was traced for it (sleic.h) */
#define INITGAME2(name, disptype, balls) \
	SLEIC2_INPUT_PORTS_START(name, balls) SLEIC_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_SLEIC,disptype,{FLIP_SW(FLIP_L)}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

/* Dot-Matrix display (128 x 32) */
static core_tLCDLayout sleic_dispDMD[] = {
  {0,0,32,128,CORE_DMD,NULL,NULL}, {0}
};

/*-------------------------------------------------------------------
/ Bike Race (1992)
/-------------------------------------------------------------------*/
INITGAME(bikerace, sleic_dispDMD, 2)
SLEIC_ROMSTART7(bikerace,"bkdsp01.bin", CRC(9b220fcb) SHA1(54e82705d8ce8a26d9e1b5f0fe382ded1f2070c3),
						 "bksnd02.bin", CRC(d67b3883) SHA1(712022b9b24c6ab559d020ab8e2106f68b4d7896),
						 "bksnd03.bin", CRC(b6d00245) SHA1(f7da6f2ca681fbe62ea9cab7f92d3e501b7e867d),
						 "bkcpu04.bin", CRC(ce745e89) SHA1(04ba97a9ef1e60a7609c87cf6d8fcae2d0e32621),
						 "bkcpu05.bin", CRC(072ce879) SHA1(4f6fb044592feb4c72bbdcbe5f19e063c0e49d0d),
						 "bkcpu06.bin", CRC(9db436d4) SHA1(3869524c0490e0a019d2f8ab46546ff42727665e),
						 "bkio07.bin",  CRC(b52a9d4f) SHA1(726a4d9b354729d7390d2a4f877dc480701ec795))
SLEIC_ROMEND
CORE_GAMEDEFNV(bikerace,"Bike Race",1992,"Sleic (Spain)",gl_mSLEIC3,0)

INITGAME(bikerac2, sleic_dispDMD, 2)
SLEIC_ROMSTART7(bikerac2,"bkdsp01.bin", CRC(9b220fcb) SHA1(54e82705d8ce8a26d9e1b5f0fe382ded1f2070c3),
						 "bksnd02.bin", CRC(d67b3883) SHA1(712022b9b24c6ab559d020ab8e2106f68b4d7896),
						 "bksnd03.bin", CRC(b6d00245) SHA1(f7da6f2ca681fbe62ea9cab7f92d3e501b7e867d),
						 "04.bin",      CRC(aaaa4a8a) SHA1(ff579041575da4060615da2ff634f3aa91537751),
						 "bkcpu05.bin", CRC(072ce879) SHA1(4f6fb044592feb4c72bbdcbe5f19e063c0e49d0d),
						 "bkcpu06.bin", CRC(9db436d4) SHA1(3869524c0490e0a019d2f8ab46546ff42727665e),
						 "07.bin",      CRC(0b763a89) SHA1(8952d7b13674e1599e53cce96e57c2783899a90a))
SLEIC_ROMEND
CORE_CLONEDEFNV(bikerac2,bikerace,"Bike Race (2-ball play)",1992,"Sleic (Spain)",gl_mSLEIC3,0)

/* V4.1 -- the newest of the three known Bike Race sets (dumped by Joerg Amann).
/  ROM 01 was not dumped; 02 and 05 are byte-identical to the parent set and are
/  inherited from it, so only 03/04/06/07 are listed here.  That inheritance is
/  verified for 02 and 05 against a complete six-chip pull off a V4.1 machine,
/  not merely assumed; 01 is still assumed.  Relative to bikerace, this set
/  carries the same F000 code revision as bikerac2 (including the 4-opto trough
/  handler that can report "FALTA 2 BOLAS"), and adds changes of its own to the
/  OKI sample ROM (03), the character/graphics ROM (06) and the game data (04).
/
/  NOT WORKING: it boots and runs, but the artwork comes out as garbage.  The
/  cause is in the ROM set, not in this driver: ROM 06 does not match ROM 04.
/
/  ROM 06 holds the sprite/character table in its first 0x1104 bytes -- the only
/  part of that chip V4.1 changes -- as records of a 6-byte header W,1,H followed
/  by ceil(W/8)*H*3 bytes of plane 0, plane 1 and mask.  ROM 04 reaches them
/  through far pointers held in its own code segment, and those pointers are
/  BYTE-IDENTICAL to the 1992 set's: every one of the 24 seg-0x2000 pointer slots
/  in bk04 holds the same offset as the matching slot in bkcpu04, 0x0000, 0x012C,
/  0x014A, 0x03A2 and 0x05FA among them.  bkcpu06 carries a well-formed record at
/  each of those offsets (eleven 8x8 sprites on a 0x1E stride, then an 18x18 at
/  0x014A).  bk06 carries none: it has 19 well-formed headers where bkcpu06 has
/  32, at different offsets, and the very first record of the chain at 0x0000 is
/  already malformed.  The same code reading the same addresses therefore finds
/  no sprite, and draws the garbage.
/
/  Substituting bkcpu06 for bk06 -- or just its first 0x1104 bytes, leaving the
/  other 124 KB of V4.1's own graphics in place -- makes this set render exactly
/  like the parent: 5131 of 5131 captured DMD frames identical between the two
/  substitutions, seven distinct screens instead of two, the "SLEIC PRESENTA" and
/  "FALTAN n BOLAS" screens clean.  So ROM 03, 04 and 07 of this set are sound
/  and the fault is confined to bk06[0x0000:0x1104].
/
/  bk06 CONTAINS NO NEW ARTWORK, which is what says it is not a genuine V4.1
/  revision of the table.  Classify each byte of bk06[0x0000:0x1200] as either
/  bkcpu06's byte at the same address or its byte 0x200 further on, and all but
/  two are accounted for -- 2099 displaced by +0x200, 2507 correct, 2 left over:
/
/      0x0000-0x002D  +0x200      0x0600-0x07FF  same
/      0x002E-0x002F  NEITHER     0x0800-0x09FF  +0x200
/      0x0030-0x00FF  same        0x0A00-0x0BFF  same
/      0x0100-0x01FF  +0x200      0x0C00-0x0DFF  +0x200
/      0x0200-0x03FF  same        0x0E00-0x0FFF  same
/      0x0400-0x05FF  +0x200      0x1000-0x1104  +0x200
/                                 0x1105-0x11FF  same
/
/  Every run is exact.  A revised sprite table would hold revised sprites; this
/  one holds two bytes that are not already in the 1992 chip, recycled at a
/  0x200 page granularity no build tool produces.  It is NOT a stuck address
/  line -- 0x0030-0x00FF and 0x1105-0x11FF have bit 9 clear and are correct, and
/  the 0x1104 boundary falls mid-block where no address line can change.
/
/  Traced at runtime the divergence is one byte.  Logging every 80188 read of the
/  ROM 06 window, the two sets are identical for 120 accesses -- both fetch the
/  18x18 record at 0x20366 -- and then split at flat 0x2001E, the table's second
/  record: bikerace reads 08 00 01 00 08 00 and draws an 8x8 sprite, bikerac3
/  reads 18 06 26 26 3C 3C, a width of 0x0618 = 1560 pixels, and runs away (3514
/  reads reaching 0x20D5F against 180 reaching 0x203A1).
/
/  THE MACHINE THIS SET CAME FROM RUNS CORRECTLY, per its owner, and that settles
/  what the bytes could not: for it to work, the byte at 0x2001E must be 0x08, and
/  the file says 0x18, so the file is not what the chip holds.  The read is at
/  fault and the physical ROM 06 is fine.  That also fits the damage stopping
/  after 4 KB rather than following an address line -- a marginal contact, or a
/  reader that fetched the opening pages twice, corrupts the start of a read and
/  then settles.
/
/  Nothing on the driver side compensates, which was tested rather than assumed:
/  loading bkcpu05 in the 06 slot, and rotating bk06 0x200 each way, all still
/  fail (2, 8 and 2 distinct DMD frames over 600, against 7 for a working set).
/  The emulation is faithfully reading the byte the file contains.
/
/  Three further things say the chip is simply the 1992 one.  Bike Race has two
/  graphics ROMs, 05 at MCS2 0x40000 and 06 at MCS1 0x20000; 05 is the larger
/  sprite bank, 133 well-formed records against 06's 32, and is byte-identical
/  across every known set (CRC 072ce879 in bikerace, bikerac2 and V4.1).  First,
/  no Bike Race revision has ever changed a graphics ROM -- bikerac2 rebuilds 04
/  and 07 and leaves both alone -- so a V4.1 that revised 06 would be the only
/  exception in the family.  Second, the same dump session read 05 perfectly,
/  128 KB correct with none of the page duplication bk06 shows, so the fault is
/  one chip's read and not the setup.  Third, of the 32 records in bkcpu06, the
/  15 that survive into bk06 all sit in correctly-read windows and the 17 that
/  are lost all sit in mis-read ones, with zero anomalies: bk06's table is not a
/  different 19-record table but the 1992 table with the damaged pages knocked
/  out.
/
/  So the set needs a re-read of ROM 06, checking offset 0x001E first: it must be
/  08 00 01 00 08 00.  The expected outcome is bkcpu06, CRC 9db436d4, which would
/  make V4.1 a three-chip clone -- bk03, bk04 and bk07 over the parent set.
/
/  Either way this is not a driver change.  It is not a missing chip: the V4.1
/  16-bit board carries exactly three 27C010 positions and all three are
/  populated, so the seven-chip complement this driver expects is the whole
/  machine.  Nor is it a mapping question: bk04's chip-select table (MMCS 0x01FF,
/  PACS 0xA03C, MPCS 0xC0FC, at file offset 0x50) is byte-identical to bkcpu04's,
/  so V4.1 decodes ROM 06 at MCS1 0x20000 and ROM 05 at MCS2 0x40000 exactly as
/  the 1992 sets do. */
INITGAME(bikerac3, sleic_dispDMD, 2)
SLEIC_ROMSTART7(bikerac3,"bkdsp01.bin", CRC(9b220fcb) SHA1(54e82705d8ce8a26d9e1b5f0fe382ded1f2070c3),
						 "bksnd02.bin", CRC(d67b3883) SHA1(712022b9b24c6ab559d020ab8e2106f68b4d7896),
						 "bk03.bin",    CRC(74c10536) SHA1(43a2a63494b044fe2326ee09831ef90f37d3b432),
						 "bk04.bin",    CRC(33fd212e) SHA1(9471e34fc4280741816d65f88590febc9e8629a7),
						 "bkcpu05.bin", CRC(072ce879) SHA1(4f6fb044592feb4c72bbdcbe5f19e063c0e49d0d),
						 "bk06.bin",    CRC(ad48a30a) SHA1(183b04699b038811de950bba6b8a067689bdb883),
						 "bk07.bin",    CRC(200ff3fc) SHA1(96fc8561b078c5306b15e260436e3d3ba562c51d))
SLEIC_ROMEND
CORE_CLONEDEFNV(bikerac3,bikerace,"Bike Race (V4.1)",1992,"Sleic (Spain)",gl_mSLEIC3,GAME_NOT_WORKING)

/*-------------------------------------------------------------------
/ Sleic Pin-Ball (1993)
/-------------------------------------------------------------------*/
INITGAME(sleicpin, sleic_dispDMD, 1)
SLEIC_ROMSTART4(sleicpin,"sp01-1_1.rom", CRC(240015bb) SHA1(0e647718173ad59dafbf3b5bc84bef3c33886e23),
						 "sp02-1_1.rom", CRC(0e4851a0) SHA1(0692ee2df0b560e2013db9c03fd27c6eb12e618d),
						 "sp03-1_1.rom", CRC(261b0ae4) SHA1(e7d9d1c2cab7776afb732701b0b8697b62a8d990),
						 "sp04-1_1.rom", CRC(84514cfa) SHA1(6aa87b86892afa534cf963821f08286c126b4245))
SLEIC_ROMEND
CORE_GAMEDEFNV(sleicpin,"Sleic Pin-Ball",1993,"Sleic (Spain)",gl_mSLEIC1,0)

/*-------------------------------------------------------------------
/ Io Moon (1996)
/-------------------------------------------------------------------*/
/* "Balls" defaults to 0, and for Io Moon that is not a ball count -- it is the OFF
   position of the driver's optional internal ball-trough model (SWITCH_UPDATE(SLEIC2) in
   sleic.c).  Off is the PinMAME convention: swMatrix[1] bits 0-3 (switch codes 0x0A-0x0D)
   are ordinary switches, and closing them is the frontend's job -- a VPinMAME table
   script's, or standalone the Q/W/E/R matrix test keys'.  Bike Race does the same and
   sits on "FALTA 1 BOLA" until its trough contacts close.

   Set "Balls" to 3 to turn the model on for standalone desktop play, where nothing else
   is going to close those contacts.  THREE is the firmware's own number rather than a
   guess: the 80188's ball-start path stores 3 into its trough counter [413C:00F9] after a
   successful ball search (DC514, DC587) and its 0xEA reply table does the same for the
   "trough full" answer 0x3A (DC14D), and the Z80's trough test 2C1F only ever clears with
   three adjacent contacts closed.  1 and 2 clamp up to 3; 4-7 also give 3, since the
   trough has only three contacts. */
INITGAME2(iomoon, sleic_dispDMD, 0)
SLEIC_ROMSTART5(iomoon, "v1_3_01.bin", CRC(df80bf4f) SHA1(29547b444cad116c9dc925d6b3112f584df37250),
						"v1_3_02.bin", CRC(2bd589cd) SHA1(87354c76cbef8185d563266230c72a618ce6fcd7),
						"v1_3_03.bin", CRC(334d0e20) SHA1(06b38cc7fcee633c45a9000187fcde8d7e03a51f),
						"v1_3_04.bin", CRC(f3a950bf) SHA1(e0410f8fe9b4efe7d21052c0a19894a563f90a27),
						"v1_3_05.bin", CRC(6bb5e101) SHA1(125412953bbee7ee171c0bd34f7848fde37ace67))
SLEIC_ROMEND
CORE_GAMEDEFNV(iomoon,"Io Moon",1996,"Sleic (Spain)",gl_mSLEIC2,0)

/* An earlier dump of the same 1.3 set.  The stickers on both sets read V1.3 -- the number
   after the dash is the chip position, not a sub-revision -- so the two are told apart by
   content: chip 01 (80188 code + upper graphics) and chip 05 (Z80 I/O code) differ, and
   the graphics and OKI sample ROMs 02/03/04 are byte-identical to the parent set and are
   listed here under the parent's names.  The suffixed file names are this driver's, for
   the same reason bikerac2 renames the two chips it changes: the chips themselves carry
   no label that separates them.

   TESTED to the same depth as the parent and no further: it boots, runs, seeds a blank
   non-volatile store and renders the DMD, and the parts of chip 01 that differ do not
   reach the sound, non-volatile-store or country/pricing code, which is byte-identical
   to the parent's.  What is NOT tested is where the two revisions actually diverge --
   the service-menu dispatch around DD480 in chip 01, and the Z80 trough and port-0x04
   handlers 2BC7/2C1F/2D9D in chip 05.  Those want an interactive play-test -- and the
   trough half of it needs either a frontend driving the trough switches or "Balls" set
   to 3, since the internal model is off by default here as it is on the parent. */
INITGAME2(iomoona, sleic_dispDMD, 0)
SLEIC_ROMSTART5(iomoona,"v1_3_01e.bin", CRC(00a75790) SHA1(3af7a5c10a8c1687a212a01393cc9195a04a73c9),
						"v1_3_02.bin",  CRC(2bd589cd) SHA1(87354c76cbef8185d563266230c72a618ce6fcd7),
						"v1_3_03.bin",  CRC(334d0e20) SHA1(06b38cc7fcee633c45a9000187fcde8d7e03a51f),
						"v1_3_04.bin",  CRC(f3a950bf) SHA1(e0410f8fe9b4efe7d21052c0a19894a563f90a27),
						"v1_3_05e.bin", CRC(dd5145f5) SHA1(7de0b9582e5130cd1eafb1c0038ee7c9ce7b3ec2))
SLEIC_ROMEND
CORE_CLONEDEFNV(iomoona,iomoon,"Io Moon (earlier ROM revision)",1996,"Sleic (Spain)",gl_mSLEIC2,0)

/* Tournament MOD of the parent set: chip 01 patched so the end of a game asks for PRESS
   START instead of dropping straight back to attract, which is what a tournament wants
   between players.  Only chip 01 changes; 02-05 are the parent's.  The patch is 186
   bytes in four regions -- a 168-byte and an 11-byte block of new code at C0010-C00B7
   and C00D0-C00DA, reached by two four-byte hooks planted at D5077 and D5123.  Seeing
   the patch fire means playing a game to its end, so standalone it needs "Balls" = 3;
   the internal trough model is off by default here as it is on the parent. */
INITGAME2(iomoont, sleic_dispDMD, 0)
SLEIC_ROMSTART5(iomoont,"v1_3_01t.bin", CRC(42cafcda) SHA1(0ac3dd882748bc86a3b66aff2d286eecd8d24a4b),
						"v1_3_02.bin",  CRC(2bd589cd) SHA1(87354c76cbef8185d563266230c72a618ce6fcd7),
						"v1_3_03.bin",  CRC(334d0e20) SHA1(06b38cc7fcee633c45a9000187fcde8d7e03a51f),
						"v1_3_04.bin",  CRC(f3a950bf) SHA1(e0410f8fe9b4efe7d21052c0a19894a563f90a27),
						"v1_3_05.bin",  CRC(6bb5e101) SHA1(125412953bbee7ee171c0bd34f7848fde37ace67))
SLEIC_ROMEND
CORE_CLONEDEFNV(iomoont,iomoon,"Io Moon (PRESS START tournament MOD)",1996,"Sleic (Spain)",gl_mSLEIC2,0)
