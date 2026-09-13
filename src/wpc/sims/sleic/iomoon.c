// license:BSD-3-Clause
/*******************************************************************************
 Io Moon (Sleic, 1996) -- game definition and ball simulator.

 PinMAME's convention is that a simulator file owns its game: the input ports,
 the game data, the ROM sets and the CORE_GAMEDEF lines live here rather than in
 sleicgames.c, which keeps Bike Race and Sleic Pin-Ball.
 ******************************************************************************/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "sleic.h"

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
   trough has only three contacts */
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
   to 3, since the internal model is off by default here as it is on the parent */
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
   the internal trough model is off by default here as it is on the parent */
INITGAME2(iomoont, sleic_dispDMD, 0)
SLEIC_ROMSTART5(iomoont,"v1_3_01t.bin", CRC(42cafcda) SHA1(0ac3dd882748bc86a3b66aff2d286eecd8d24a4b),
						"v1_3_02.bin",  CRC(2bd589cd) SHA1(87354c76cbef8185d563266230c72a618ce6fcd7),
						"v1_3_03.bin",  CRC(334d0e20) SHA1(06b38cc7fcee633c45a9000187fcde8d7e03a51f),
						"v1_3_04.bin",  CRC(f3a950bf) SHA1(e0410f8fe9b4efe7d21052c0a19894a563f90a27),
						"v1_3_05.bin",  CRC(6bb5e101) SHA1(125412953bbee7ee171c0bd34f7848fde37ace67))
SLEIC_ROMEND
CORE_CLONEDEFNV(iomoont,iomoon,"Io Moon (PRESS START tournament MOD)",1996,"Sleic (Spain)",gl_mSLEIC2,0)
