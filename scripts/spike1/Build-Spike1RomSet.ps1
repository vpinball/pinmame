<#
.SYNOPSIS
  Builds a PinMAME Stern Spike 1 ROM set from Stern's own download.

.DESCRIPTION
  Stern publishes each Spike 1 code version as an SD-card image, <title>-<version>.iso.zip, or
  for some titles (Heavy Metal) only as an update package, <title>-<version>.spk. The image holds
  an MBR with an extended partition; one of its ext3 partitions carries the game folder (the
  program, image.bin, lcdinsert.bin when the title has an LCD insert, and the node board
  firmware). The package holds the same folder as one of its groups. PinMAME's ROM set is that
  folder as a zip, named after the set. The two titles on Stern's earlier ELG framework
  (Transformers The Pin, Avengers The Pin) keep theirs in the root file system instead,
  /usr/local/games/<name>-elg: the programs and their sounds folder, which the set holds as the
  program the machine runs and the sounds' WAV files.

  This script reads the image straight out of the .iso.zip - no 3.7 GB temporary copy - in a few
  forward passes, or the files straight out of the .spk, writes <set>.zip (stored, not
  compressed, so PinMAME maps image.bin quickly) and checks every file against the set's CRCs. A
  card dump in a .7z (Avengers The Pin's) it reads through 7-Zip, which has to be installed.

  Without -Image it opens a window. Start it with "Build ROM set.cmd", or drop the .iso.zip, card
  dump or .spk onto that file.

.PARAMETER Image
  Stern's .iso.zip, the .iso inside it, a raw dump of a card (.bin or .img, as Spider-Man's, or a
  .7z holding one, as Avengers The Pin's), or Stern's .spk.

.PARAMETER OutDir
  Where <set>.zip goes. Default: the roms folder of VPX's PinMAME plugin (PinMAMEPath in
  VPinballX.ini), else the folder of the image.
#>
param(
  [string]$Image,
  [string]$OutDir,
  [switch]$Gui
)

$ErrorActionPreference = 'Stop'

# The C# below targets Windows PowerShell's .NET Framework (part of every Windows 10 and 11);
# PowerShell 7 hands over to it
if ($PSVersionTable.PSEdition -eq 'Core') {
  $relay = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-STA', '-File', $PSCommandPath)
  if ($Image) { $relay += @('-Image', $Image) }
  if ($OutDir) { $relay += @('-OutDir', $OutDir) }
  if ($Gui) { $relay += '-Gui' }
  & "$env:SystemRoot\System32\WindowsPowerShell\v1.0\powershell.exe" @relay
  exit $LASTEXITCODE
}

$source = @'
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace Spike1Rom
{
	// The sets PinMAME knows: the game folder each comes from, and its files
	public class KnownFile { public string Name; public long Size; public uint Crc; public KnownFile(string n, long s, uint c) { Name = n; Size = s; Crc = c; } }
	public class KnownSet
	{
		public string Set, Folder, Title;
		public KnownFile[] Files;
		// The set's LCD clips are encrypted, and the key is not in the set: the user adds <set>.txt
		public bool VideoKey;
		public static readonly KnownSet[] All = new KnownSet[] {
			new KnownSet { Set = "gbust_117h", Folder = "ghostbusters_le", Title = "Ghostbusters LE 1.17.0", Files = new KnownFile[] {
				new KnownFile("game", 0x006c311a, 0x0b6958ba),
				new KnownFile("image.bin", 0x462dcb94, 0x26796bf5),
				new KnownFile("accbridgenode-LPC1313-0_52_0.hex", 0x0000527e, 0xc4a99441),
				new KnownFile("coil4node-LPC1112_101-0_52_0.hex", 0x00008543, 0x98264c96),
				new KnownFile("coil4node-LPC1112_201-0_52_0.hex", 0x00008543, 0x41a33f10),
				new KnownFile("coil4node-LPC1313-0_52_0.hex", 0x0000df1e, 0x42b641b3),
				new KnownFile("lcdinsert.bin", 0x0122a020, 0xe77842db),
				new KnownFile("lcdnode-LPC1113_302-0_52_0.hex", 0x0000b6fa, 0x118be40f),
				new KnownFile("netbridge-LPC1313-0_52_0.hex", 0x0000f53c, 0x29323686),
				new KnownFile("node4-LPC1124_303-0_52_0.hex", 0x000065d4, 0x13217fb3),
				new KnownFile("nodebusanalyzer-LPC1313-0_52_0.hex", 0x0000546d, 0x69c0666c),
				new KnownFile("pinnode-LPC1112_101-0_52_0.hex", 0x00007fc8, 0x9f0f6f3f),
				new KnownFile("pinnode-LPC1112_201-0_52_0.hex", 0x00008005, 0xa2bc8685),
				new KnownFile("pinnode-LPC1313-0_52_0.hex", 0x0000ea4e, 0xac00f191),
				new KnownFile("ws2812node-LPC1313-0_52_0.hex", 0x00008676, 0xd0008f92) } },
			new KnownSet { Set = "wnbjm_155", Folder = "WN", Title = "Whoa Nellie! Big Juicy Melons 1.55.0", Files = new KnownFile[] {
				new KnownFile("game", 0x004115ac, 0x67df0775),
				new KnownFile("image.bin", 0x10756444, 0x9c10415b),
				new KnownFile("coil4node-LPC1112_101-0_28_0.hex", 0x00008487, 0x29d48577),
				new KnownFile("coil4node-LPC1112_201-0_28_0.hex", 0x00008487, 0x19aac926),
				new KnownFile("coil4node-LPC1313-0_28_0.hex", 0x0000c23a, 0xe019995b),
				new KnownFile("lcdnode-LPC1113_302-0_28_0.hex", 0x0000b484, 0xd139cac7),
				new KnownFile("pinnode-LPC1112_101-0_28_0.hex", 0x00007f1c, 0x83ca12dc),
				new KnownFile("pinnode-LPC1112_201-0_28_0.hex", 0x00007f66, 0xcee92fdc),
				new KnownFile("pinnode-LPC1313-0_28_0.hex", 0x0000c859, 0x18c17d1d),
				new KnownFile("ws2812node-LPC1313-0_28_0.hex", 0x00005590, 0x921dc0f7) } },
			new KnownSet { Set = "primus_103", Folder = "primus", Title = "Primus 1.03.0", Files = new KnownFile[] {
				new KnownFile("game", 0x0041372a, 0x017178e5),
				new KnownFile("image.bin", 0x2a06f114, 0xcb499566),
				new KnownFile("coil4node-LPC1112_101-0_28_0.hex", 0x00008487, 0x29d48577),
				new KnownFile("coil4node-LPC1112_201-0_28_0.hex", 0x00008487, 0x19aac926),
				new KnownFile("coil4node-LPC1313-0_28_0.hex", 0x0000c23a, 0xe019995b),
				new KnownFile("lcdnode-LPC1113_302-0_28_0.hex", 0x0000b484, 0xd139cac7),
				new KnownFile("pinnode-LPC1112_101-0_28_0.hex", 0x00007f1c, 0x83ca12dc),
				new KnownFile("pinnode-LPC1112_201-0_28_0.hex", 0x00007f66, 0xcee92fdc),
				new KnownFile("pinnode-LPC1313-0_28_0.hex", 0x0000c859, 0x18c17d1d),
				new KnownFile("ws2812node-LPC1313-0_28_0.hex", 0x00005590, 0x921dc0f7) } },
			new KnownSet { Set = "pabst_101", Folder = "can_crusher", Title = "Pabst Can Crusher 1.01.0", Files = new KnownFile[] {
				new KnownFile("game", 0x004122e1, 0x0a114c42),
				new KnownFile("image.bin", 0x398fd2c4, 0x424bb457),
				new KnownFile("coil4node-LPC1112_101-0_28_0.hex", 0x00008487, 0x29d48577),
				new KnownFile("coil4node-LPC1112_201-0_28_0.hex", 0x00008487, 0x19aac926),
				new KnownFile("coil4node-LPC1313-0_28_0.hex", 0x0000c23a, 0xe019995b),
				new KnownFile("lcdnode-LPC1113_302-0_28_0.hex", 0x0000b484, 0xd139cac7),
				new KnownFile("pinnode-LPC1112_101-0_28_0.hex", 0x00007f1c, 0x83ca12dc),
				new KnownFile("pinnode-LPC1112_201-0_28_0.hex", 0x00007f66, 0xcee92fdc),
				new KnownFile("pinnode-LPC1313-0_28_0.hex", 0x0000c859, 0x18c17d1d),
				new KnownFile("ws2812node-LPC1313-0_28_0.hex", 0x00005590, 0x921dc0f7) } },
			new KnownSet { Set = "got_137h", Folder = "GOT_LE", Title = "Game of Thrones LE 1.37.0", Files = new KnownFile[] {
				new KnownFile("game", 0x005fa263, 0xbd9d74e9),
				new KnownFile("image.bin", 0x2ec129dc, 0x8dbe0aa4),
				new KnownFile("accbridgenode-LPC1313-0_49_0.hex", 0x0000529b, 0x5f3ae3d6),
				new KnownFile("coil4node-LPC1112_101-0_49_0.hex", 0x00008595, 0x6f4a25ae),
				new KnownFile("coil4node-LPC1112_201-0_49_0.hex", 0x00008595, 0x43f9bc24),
				new KnownFile("coil4node-LPC1313-0_49_0.hex", 0x0000d28b, 0x551c8bb3),
				new KnownFile("lcdnode-LPC1113_302-0_49_0.hex", 0x0000b70a, 0x762a4010),
				new KnownFile("netbridge-LPC1313-0_49_0.hex", 0x0000e89c, 0x0179e13b),
				new KnownFile("nodebusanalyzer-LPC1313-0_49_0.hex", 0x0000548a, 0x7925a13c),
				new KnownFile("pinnode-LPC1112_101-0_49_0.hex", 0x0000801a, 0x66b91e74),
				new KnownFile("pinnode-LPC1112_201-0_49_0.hex", 0x00008057, 0x42ca3b82),
				new KnownFile("pinnode-LPC1313-0_49_0.hex", 0x0000ddae, 0xa31b7afa),
				new KnownFile("ws2812node-LPC1313-0_49_0.hex", 0x0000777d, 0x00f38c82) } },
			new KnownSet { Set = "got_137", Folder = "GOT", Title = "Game of Thrones Pro 1.37.0", Files = new KnownFile[] {
				new KnownFile("game", 0x005e6954, 0x8e5d0f49),
				new KnownFile("image.bin", 0x2ec129dc, 0x8dbe0aa4),
				new KnownFile("accbridgenode-LPC1313-0_49_0.hex", 0x0000529b, 0x5f3ae3d6),
				new KnownFile("coil4node-LPC1112_101-0_49_0.hex", 0x00008595, 0x6f4a25ae),
				new KnownFile("coil4node-LPC1112_201-0_49_0.hex", 0x00008595, 0x43f9bc24),
				new KnownFile("coil4node-LPC1313-0_49_0.hex", 0x0000d28b, 0x551c8bb3),
				new KnownFile("lcdnode-LPC1113_302-0_49_0.hex", 0x0000b70a, 0x762a4010),
				new KnownFile("netbridge-LPC1313-0_49_0.hex", 0x0000e89c, 0x0179e13b),
				new KnownFile("nodebusanalyzer-LPC1313-0_49_0.hex", 0x0000548a, 0x7925a13c),
				new KnownFile("pinnode-LPC1112_101-0_49_0.hex", 0x0000801a, 0x66b91e74),
				new KnownFile("pinnode-LPC1112_201-0_49_0.hex", 0x00008057, 0x42ca3b82),
				new KnownFile("pinnode-LPC1313-0_49_0.hex", 0x0000ddae, 0xa31b7afa),
				new KnownFile("ws2812node-LPC1313-0_49_0.hex", 0x0000777d, 0x00f38c82) } },
			new KnownSet { Set = "kiss15_141h", Folder = "KISS_LE", Title = "KISS LE 1.41.0", Files = new KnownFile[] {
				new KnownFile("game", 0x004ee9ca, 0x1c55a059),
				new KnownFile("image.bin", 0x3e8f02ec, 0xd96ccf4c),
				new KnownFile("coil4node-LPC1112_101-0_28_0.hex", 0x00008487, 0x29d48577),
				new KnownFile("coil4node-LPC1112_201-0_28_0.hex", 0x00008487, 0x19aac926),
				new KnownFile("coil4node-LPC1313-0_28_0.hex", 0x0000c23a, 0xe019995b),
				new KnownFile("lcdnode-LPC1113_302-0_28_0.hex", 0x0000b484, 0xd139cac7),
				new KnownFile("pinnode-LPC1112_101-0_28_0.hex", 0x00007f1c, 0x83ca12dc),
				new KnownFile("pinnode-LPC1112_201-0_28_0.hex", 0x00007f66, 0xcee92fdc),
				new KnownFile("pinnode-LPC1313-0_28_0.hex", 0x0000c859, 0x18c17d1d),
				new KnownFile("ws2812node-LPC1313-0_28_0.hex", 0x00005590, 0x921dc0f7) } },
			new KnownSet { Set = "wwe_135h", Folder = "WWE_LE", Title = "WWE WrestleMania LE 1.35.0", VideoKey = true, Files = new KnownFile[] {
				new KnownFile("game", 0x005685c2, 0xedc2c801),
				new KnownFile("image.bin", 0x547598dc, 0xaa528057),
				new KnownFile("AJ.spv", 0x008ca1fc, 0x6fe0fc26),
				new KnownFile("BELLA_TWINS.spv", 0x008ca1fc, 0x5ca08b63),
				new KnownFile("BLANK_FRAME.spv", 0x00025824, 0x19340b39),
				new KnownFile("CENA_BG_FPS20.spv", 0x01588cb4, 0xc516caca),
				new KnownFile("CENA_INTRO_FPS20.spv", 0x059a733c, 0xfefaf73d),
				new KnownFile("coil4node-LPC1112_101-0_18_4.hex", 0x000086b3, 0x0db5fef9),
				new KnownFile("coil4node-LPC1112_201-0_18_4.hex", 0x000086b3, 0x0d97d36b),
				new KnownFile("coil4node-LPC1313-0_18_4.hex", 0x00009910, 0xeda97e7a),
				new KnownFile("DB_BG_FPS20.spv", 0x01588cb4, 0xdadcc332),
				new KnownFile("DB_INTRO_FPS20.spv", 0x0a95843c, 0x1c0764df),
				new KnownFile("FIREWORKS_FPS20.spv", 0x008ca1fc, 0x4a85e806),
				new KnownFile("HBK_BG_FPS20.spv", 0x01588cb4, 0x32e76255),
				new KnownFile("HBK_INTRO_FPS20.spv", 0x0b9c07bc, 0xcf51844f),
				new KnownFile("HHH_BG_FPS20.spv", 0x01588cb4, 0xa535703f),
				new KnownFile("HHH_INTRO_FPS20.spv", 0x064c959c, 0x2ba4c0e4),
				new KnownFile("HOGAN_BG_FPS20.spv", 0x01588cb4, 0x7fd2b05f),
				new KnownFile("HOGAN_INTRO_FPS20.spv", 0x099d10ec, 0xbfffb34e),
				new KnownFile("KANE.spv", 0x005dc15c, 0xd0d04329),
				new KnownFile("lcdnode-LPC1113_302-0_18_4.hex", 0x00009783, 0xb9e6387e),
				new KnownFile("LEGENDS_FPS20.spv", 0x0070819c, 0xd45c415f),
				new KnownFile("LEGION_OF_DOOM.spv", 0x008ca1fc, 0x3031bab7),
				new KnownFile("MAIN_EVENT.spv", 0x008ca1fc, 0x9386f7d6),
				new KnownFile("pinnode-LPC1112_101-0_18_4.hex", 0x00008005, 0x72dd8621),
				new KnownFile("pinnode-LPC1112_201-0_18_4.hex", 0x0000802a, 0x8a0e089c),
				new KnownFile("pinnode-LPC1313-0_18_4.hex", 0x0000a082, 0x0e9790a7),
				new KnownFile("RAW.spv", 0x008ca1fc, 0x7c19cd49),
				new KnownFile("ROCK_BG_FPS20.spv", 0x01588cb4, 0x8ed98eb7),
				new KnownFile("ROCK_INTRO_FPS20.spv", 0x06c6773c, 0x3a667503),
				new KnownFile("ROYAL_RUMBLE.spv", 0x00a4124c, 0xd549883a),
				new KnownFile("SCSA_BG_FPS20.spv", 0x01588cb4, 0x9230922c),
				new KnownFile("SCSA_INTRO_FPS20.spv", 0x0668b5fc, 0x42a33cce),
				new KnownFile("TAG_TEAM.spv", 0x005dc15c, 0xf90ff90f),
				new KnownFile("UNDERTAKER_BG_FPS20.spv", 0x01588cb4, 0x69dd7575),
				new KnownFile("UNDERTAKER_INTRO_FPS20.spv", 0x070f2034, 0xf513bc4f),
				new KnownFile("US_CHAMPIONS.spv", 0x011943dc, 0x7a004855),
				new KnownFile("WORLD_HEAVYWEIGHT.spv", 0x011943dc, 0x4fd67f81),
				new KnownFile("WORLD_INTERCONTINENTAL.spv", 0x0148247c, 0x72ea6cc2),
				new KnownFile("WRESTLEMANIALOGO.spv", 0x00d2f2ec, 0xe387938a),
				new KnownFile("WWE_SHATTERLOGO_FPS20.spv", 0x015f94cc, 0x8be9e928),
				new KnownFile("WWELOGO_FPS20.spv", 0x008efa04, 0x0b72c2aa) } },
			new KnownSet { Set = "heavym20_102", Folder = "heavy_metal", Title = "Heavy Metal 1.02.0", Files = new KnownFile[] {
				new KnownFile("game", 0x00364e7c, 0x89dcf06c),
				new KnownFile("image.bin", 0x71c08cbc, 0x378ea9e2),
				new KnownFile("coil4node-LPC1112_101-0_67_0.hex", 0x00008784, 0x3694f8a6),
				new KnownFile("coil4node-LPC1112_201-0_67_0.hex", 0x00008784, 0x9760b3eb),
				new KnownFile("coil4node-LPC1313-0_67_0.hex", 0x0000ecf5, 0x0795b75b),
				new KnownFile("lcdnode-LPC1113_302-0_67_0.hex", 0x0000b494, 0x5486eb88),
				new KnownFile("node4-LPC1124_303-0_67_0.hex", 0x00009e0c, 0x219e0f03),
				new KnownFile("pinnode-LPC1112_101-0_67_0.hex", 0x00008784, 0x6c0eadc3),
				new KnownFile("pinnode-LPC1112_201-0_67_0.hex", 0x00008784, 0x7b8accae),
				new KnownFile("pinnode-LPC1313-0_67_0.hex", 0x0000f839, 0x0ee1afa2),
				new KnownFile("tmc2590node-LPC1313-0_67_0.hex", 0x0000bfe5, 0xa48f352c),
				new KnownFile("ws2812node-LPC1313-0_67_0.hex", 0x00007c40, 0x514046cf) } },
			new KnownSet { Set = "supreme_101", Folder = "supreme", Title = "Supreme 1.01.0", Files = new KnownFile[] {
				new KnownFile("game", 0x00462e0b, 0x681a3c00),
				new KnownFile("image.bin", 0x06fa11ec, 0x3eb2c19d),
				new KnownFile("coil4node-LPC1112_101-0_22_0.hex", 0x000082cd, 0x22c8518e),
				new KnownFile("coil4node-LPC1112_201-0_22_0.hex", 0x000082cd, 0x01d69520),
				new KnownFile("coil4node-LPC1313-0_22_0.hex", 0x0000bcec, 0x290c5e17),
				new KnownFile("lcdnode-LPC1113_302-0_22_0.hex", 0x0000b484, 0x0e222d2c),
				new KnownFile("pinnode-LPC1112_101-0_22_0.hex", 0x000080fb, 0x0be8403d),
				new KnownFile("pinnode-LPC1112_201-0_22_0.hex", 0x00008138, 0xb3646ae2),
				new KnownFile("pinnode-LPC1313-0_22_0.hex", 0x0000c313, 0xf6416a86),
				new KnownFile("ws2812node-LPC1313-0_22_0.hex", 0x00005590, 0x9a090388) } },
			new KnownSet { Set = "smancp_101", Folder = "spiderman_cp", Title = "Spider-Man CP 1.01", Files = new KnownFile[] {
				new KnownFile("game", 0x00504aef, 0x11a8c4e4),
				new KnownFile("image.bin", 0x105bafbc, 0x10f7f532),
				new KnownFile("coil4node-LPC1112_101-0_19_1.hex", 0x00008705, 0x4f7ee588),
				new KnownFile("coil4node-LPC1112_201-0_19_1.hex", 0x00008705, 0x5dd30f62),
				new KnownFile("coil4node-LPC1313-0_19_1.hex", 0x00009a09, 0x0b2c7089),
				new KnownFile("lcdnode-LPC1113_302-0_19_1.hex", 0x00009be8, 0x8efe9b44),
				new KnownFile("pinnode-LPC1112_101-0_19_1.hex", 0x00008057, 0x221c95fd),
				new KnownFile("pinnode-LPC1112_201-0_19_1.hex", 0x0000807c, 0x2769520c),
				new KnownFile("pinnode-LPC1313-0_19_1.hex", 0x0000a188, 0xfeb117d1) } },
			new KnownSet { Set = "tfcp_1018", Folder = "tf-elg", Title = "Transformers The Pin 1.0.18", Files = new KnownFile[] {
				new KnownFile("gamer", 0x00104148, 0x14587823),
				new KnownFile("Award_02_A.wav", 0x0002f37e, 0x6041b02e),
				new KnownFile("Award_02_A_short.wav", 0x00004f5a, 0xc56c4639),
				new KnownFile("BG_AutoBotBonus_Intro.wav", 0x0005296e, 0x24b9d750),
				new KnownFile("BG_AutoBotBonus_loop.wav", 0x0001f25e, 0xaf67a81f),
				new KnownFile("BG_AutoBotMainPlay_loop.wav", 0x00103978, 0x4149659f),
				new KnownFile("BG_AutoBotMainPlay_Opening.wav", 0x00103b48, 0xf34d0ffe),
				new KnownFile("BG_AutobotSelect_intro.wav", 0x0000aefc, 0x9fcf9ead),
				new KnownFile("BG_AutobotSelect_loop.wav", 0x000567ec, 0xd437e830),
				new KnownFile("BG_Blackout_Loop.wav", 0x00103a82, 0x3c1375d0),
				new KnownFile("BG_BumbleBee_Intro.wav", 0x0000535c, 0x2b7284e1),
				new KnownFile("BG_BumbleBee_Loop.wav", 0x000d96f0, 0xa5db75cc),
				new KnownFile("BG_DecepticonBonus_loop.wav", 0x00016306, 0x0abda797),
				new KnownFile("BG_DecepticonBonus_Start.wav", 0x0003f98e, 0x7ec3a726),
				new KnownFile("BG_DecepticonMainPlay.wav", 0x001780a2, 0x997fbc2a),
				new KnownFile("BG_DecepticonSelect_Intro.wav", 0x0001785e, 0x556b0589),
				new KnownFile("BG_DecepticonSelect_Loop.wav", 0x0005dbb4, 0x5a96c5eb),
				new KnownFile("BG_Devastator_Intro.wav", 0x0006d174, 0xd023a7bf),
				new KnownFile("BG_Devastator_loop.wav", 0x00110312, 0x84048cb0),
				new KnownFile("BG_FastScoring_loop.wav", 0x000f11c4, 0x683c97b2),
				new KnownFile("BG_Ironhide2a Intro.wav", 0x0003ea0e, 0x77f8474d),
				new KnownFile("BG_Ironhide2a loop.wav", 0x000cb29c, 0x1308eb92),
				new KnownFile("BG_MandS_Twins_intro.wav", 0x00029f2a, 0x4df7dadd),
				new KnownFile("BG_MandS_Twins_loop.wav", 0x0014d1b0, 0x4cbaafcd),
				new KnownFile("BG_Megatron_Multiballintro.wav", 0x0005b8ba, 0x92278335),
				new KnownFile("BG_Megatron_Multiballloop.wav", 0x00157586, 0x9c8162ab),
				new KnownFile("BG_MiniWizard_Intro.wav", 0x00025a82, 0x6ac24c7c),
				new KnownFile("BG_MiniWizard_loop.wav", 0x000e1826, 0xfd36f10d),
				new KnownFile("BG_OptimusMultiball3_intro.wav", 0x0006bf2c, 0xac11c6c4),
				new KnownFile("BG_OptimusMultiball3_loop.wav", 0x00103efe, 0x0f530c57),
				new KnownFile("BG_PillarMultiball_Intro.wav", 0x000280d6, 0xba846720),
				new KnownFile("BG_PillarMultiball_loop.wav", 0x00169d28, 0x555490cc),
				new KnownFile("BG_Sam.wav", 0x0016aa48, 0xb491cdae),
				new KnownFile("BG_Shockwave_Intro.wav", 0x00029f1c, 0x115cfdbe),
				new KnownFile("BG_Shockwave_loop.wav", 0x000d204e, 0xdaa94981),
				new KnownFile("BG_Starscream_Intro.wav", 0x0005ccb8, 0xb0679b3f),
				new KnownFile("BG_Starscream_loop.wav", 0x000fce2e, 0xf08fa138),
				new KnownFile("BG_TCT_Heavy_intro.wav", 0x00013164, 0x956184a6),
				new KnownFile("BG_TCT_Heavy_loop.wav", 0x0015a72c, 0x6632f5ee),
				new KnownFile("FX_AllsparkLites.wav", 0x0002f108, 0xd66be657),
				new KnownFile("FX_AllSparkMysteryAward.wav", 0x00030c12, 0x9f433359),
				new KnownFile("FX_AllSparkMysteryAward2_Locked.wav", 0x00022926, 0x01fc45ab),
				new KnownFile("FX_AllSparkMysteryAward2_Rise.wav", 0x0001a29c, 0xe53b0e7f),
				new KnownFile("FX_AutoBotFires_01.wav", 0x0001b308, 0x9cbd2e9f),
				new KnownFile("FX_AutoBotLitLane.wav", 0x0000fa26, 0x48c7ddc7),
				new KnownFile("FX_AutoTalk_01.wav", 0x00017704, 0x928fda25),
				new KnownFile("FX_AutoTalk_02.wav", 0x0000b586, 0x9d3571e2),
				new KnownFile("FX_AutoTalk_03_FX001.wav", 0x000063e0, 0xb460234e),
				new KnownFile("FX_AutoTalk_03_FX002.wav", 0x000076ca, 0xc779b540),
				new KnownFile("FX_Bang01.wav", 0x00018108, 0x1bc4b26a),
				new KnownFile("FX_BeamWeapon_01.wav", 0x0001346c, 0xe3ec2d78),
				new KnownFile("FX_Bung_01.wav", 0x0001307e, 0x49fce7c5),
				new KnownFile("FX_CarRevs_FX001.wav", 0x0000d228, 0xdffb4b49),
				new KnownFile("FX_CarRevs_FX002.wav", 0x0000cb08, 0xbf37df1b),
				new KnownFile("FX_CarRevs_FX003.wav", 0x0001e8c6, 0x803d5924),
				new KnownFile("FX_CarSkidsFX001.wav", 0x00022e0c, 0x7629f7dd),
				new KnownFile("FX_CarSkidsFX002.wav", 0x000134e0, 0x33f07b30),
				new KnownFile("FX_CarSkidsFX003.wav", 0x00039c66, 0x13caa210),
				new KnownFile("FX_CarSkidsFX004.wav", 0x0002f004, 0x07fd25ac),
				new KnownFile("FX_CleanHit_FX001.wav", 0x00009aec, 0xe797a04c),
				new KnownFile("FX_CleanHit_FX002.wav", 0x00009966, 0x09779ad8),
				new KnownFile("FX_CleanHit_FX003.wav", 0x00009966, 0x59f483cd),
				new KnownFile("FX_CleanHit_FX004.wav", 0x00009966, 0x8a5ef724),
				new KnownFile("FX_DecepPops_02.wav", 0x00015d3a, 0x96c45979),
				new KnownFile("FX_DecepPops_03.wav", 0x00016b0a, 0x3dd3a9b5),
				new KnownFile("FX_DecepThing.wav", 0x00017154, 0xddfa434d),
				new KnownFile("FX_Diesel01.wav", 0x0002b490, 0xbbf46d01),
				new KnownFile("FX_Energon_FX001.wav", 0x0000e766, 0x5194210f),
				new KnownFile("FX_Energon_FX002.wav", 0x0000d4ae, 0xae45b77c),
				new KnownFile("FX_EnergyBeam_01.wav", 0x00019d44, 0xb61b9006),
				new KnownFile("FX_EnergyBeam_02.wav", 0x00018814, 0x15427c47),
				new KnownFile("FX_EnergyBeam_03.wav", 0x0001901a, 0xc26352a2),
				new KnownFile("FX_EnergyShot_01.wav", 0x0001ed70, 0x3bc671d1),
				new KnownFile("FX_EnergyShot_02.wav", 0x0001960a, 0x03f1f707),
				new KnownFile("FX_EnergyShot_03.wav", 0x0001f58a, 0xaadce90e),
				new KnownFile("FX_FragmentedRiseFlanged2_SweptDown.wav", 0x00023754, 0x1d04ee64),
				new KnownFile("FX_GoodAward_01_FX001.wav", 0x000104c8, 0x9b73ff1c),
				new KnownFile("FX_GoodAward_01_FX002.wav", 0x000104c8, 0x8f1d4dde),
				new KnownFile("FX_GoodThing_02_FX001.wav", 0x00010ad4, 0x7ad03e17),
				new KnownFile("FX_GoodThing_02_FX003.wav", 0x00010842, 0x87d5295b),
				new KnownFile("FX_HitDecepThing_FX001.wav", 0x0000bf0c, 0xc9dd2c91),
				new KnownFile("FX_HitDecepThing_FX002.wav", 0x0000d376, 0x38a29278),
				new KnownFile("FX_HitDecepThing_FX003.wav", 0x0000b916, 0x9f11a7b8),
				new KnownFile("FX_HitLow_FX001.wav", 0x00011e42, 0x1da41783),
				new KnownFile("FX_HitLow_FX002.wav", 0x000105fc, 0x794954bc),
				new KnownFile("FX_HitLow_FX003.wav", 0x0000f432, 0x6a54b178),
				new KnownFile("FX_HitLow_FX004.wav", 0x0001247c, 0x1c7d2dc5),
				new KnownFile("FX_HitSomethingGood_01.wav", 0x0000f638, 0x05df45c4),
				new KnownFile("FX_ImpactMetal_FX001.wav", 0x0000e936, 0xa549a333),
				new KnownFile("FX_ImpactMetal_FX002.wav", 0x0000e506, 0xa1ba1b08),
				new KnownFile("FX_ImpactMetal_FX003.wav", 0x0000e50e, 0xb5881c61),
				new KnownFile("FX_ImpactMetal_FX004.wav", 0x0000e50e, 0x69a86da0),
				new KnownFile("FX_LeftEjectWindUp_01.wav", 0x00022efc, 0x83972a28),
				new KnownFile("FX_LookingForTheAllSpark.wav", 0x00015c78, 0x24a29ee6),
				new KnownFile("FX_MagicFlyIn_01.wav", 0x000253d8, 0xdbfe8225),
				new KnownFile("FX_MegaAward_01.wav", 0x00023eee, 0x10df610a),
				new KnownFile("FX_MegatronFires_01_FX001.wav", 0x00022724, 0xa68e7b4f),
				new KnownFile("FX_MegatronFires_01_FX002.wav", 0x00022724, 0x07bbf92b),
				new KnownFile("FX_MegatronFires_01_FX003.wav", 0x00022724, 0x8e3fc9df),
				new KnownFile("FX_MegatronLock_FX001.wav", 0x000166e6, 0xfe2346c8),
				new KnownFile("FX_MegatronLock_FX002.wav", 0x00017ef2, 0x11397ede),
				new KnownFile("FX_MegatronLock_FX003.wav", 0x0001f49c, 0x1e90de5c),
				new KnownFile("FX_ModeStartDevastator_01a.wav", 0x000279ce, 0x160426e3),
				new KnownFile("FX_ModeStartDevastator_01b.wav", 0x00022a90, 0x78598d65),
				new KnownFile("FX_ModeStartDevastator_01c.wav", 0x0001d9ee, 0xc8d8b155),
				new KnownFile("FX_ModeStartDevastator_01d.wav", 0x0002ef48, 0x08065027),
				new KnownFile("FX_ModeStartDevastator_01e.wav", 0x0002eee0, 0xe1e3d3d4),
				new KnownFile("FX_ModeStartMandS.wav", 0x0002bf5a, 0x4ea65a46),
				new KnownFile("FX_ModeStartRatchet_01.wav", 0x0003299a, 0xfcd67b69),
				new KnownFile("FX_ModeStartRatchet_02.wav", 0x0002919a, 0x77af4ea1),
				new KnownFile("FX_Modifier_01.wav", 0x0000fa5e, 0x4a7a1f57),
				new KnownFile("FX_Modifier_02.wav", 0x0000f8ea, 0x72dce911),
				new KnownFile("FX_MoreThanMeetsTheEye.wav", 0x000289c6, 0x8007f4cf),
				new KnownFile("FX_Null_02.wav", 0x000129fc, 0x42d581ef),
				new KnownFile("FX_OptimusOrbit.wav", 0x00033b9a, 0x5886349b),
				new KnownFile("FX_OrbitScore_FX002.wav", 0x00018662, 0x56610077),
				new KnownFile("FX_Plunge_01.wav", 0x000271da, 0x479354ea),
				new KnownFile("FX_Plunge_02.wav", 0x000252d6, 0x200a896e),
				new KnownFile("FX_RampEntry_01_FX001.wav", 0x00014c28, 0x49681a5a),
				new KnownFile("FX_RampEntry_01_FX002.wav", 0x000156a4, 0xcfe239f1),
				new KnownFile("FX_Spinner_01_FX001.wav", 0x00014b24, 0xe8455417),
				new KnownFile("FX_Spinner_01_FX004.wav", 0x000133ee, 0x8f420a3e),
				new KnownFile("FX_StingerAutoAward02.wav", 0x000234be, 0xf61cb6a6),
				new KnownFile("FX_StingerDecepticon_01_FX001.wav", 0x00030856, 0x4beb5dc1),
				new KnownFile("FX_StingerDecepticon_01_FX002.wav", 0x00030856, 0x2d0964a8),
				new KnownFile("FX_StingerDecepticon_01_FX003.wav", 0x0002f11c, 0x5c6f8301),
				new KnownFile("FX_StingerScore_01.wav", 0x0001c66e, 0x0fd296ab),
				new KnownFile("FX_StingerScore_a_01.wav", 0x0001ab0c, 0x7866be28),
				new KnownFile("FX_StingerScore_B_01.wav", 0x0002b774, 0x7958681e),
				new KnownFile("FX_StingerScore_Bb_01.wav", 0x0001aa4e, 0x431795c1),
				new KnownFile("FX_StingerScore_C_01.wav", 0x0002b980, 0x15fc30a9),
				new KnownFile("FX_StingerScore_Db_01.wav", 0x0002b96a, 0x0da31496),
				new KnownFile("FX_SuperSkillShot.wav", 0x0001dd96, 0x48909b97),
				new KnownFile("FX_SynthFlyBy.wav", 0x0002d4d0, 0x1b2ccfeb),
				new KnownFile("FX_T1_AutobotEMIPulse.wav", 0x00013a48, 0xc7e7e9d4),
				new KnownFile("FX_T1_BlackOutLands.wav", 0x0002741c, 0xb93dc17c),
				new KnownFile("FX_T1_DecepticonHelicopterFlyover.wav", 0x0003f050, 0xd317a338),
				new KnownFile("FX_T1_DecepticonLanesComplete.wav", 0x0000f29a, 0x467c4a6e),
				new KnownFile("FX_T1_Explo.wav", 0x0000df8e, 0x09d2a519),
				new KnownFile("FX_T1_ExplosionGunsClose.wav", 0x00015094, 0x9333251f),
				new KnownFile("FX_T1_ExplosionGunsClose_02.wav", 0x0000ea5c, 0x44271f64),
				new KnownFile("FX_T1_ExplosionGunsClose_04.wav", 0x000175ee, 0xdbd5e64e),
				new KnownFile("FX_T1_MegatronShouts_01.wav", 0x0000e368, 0xf349b51b),
				new KnownFile("FX_T1_MisslesHitMegatron.wav", 0x00016b8c, 0x7247744f),
				new KnownFile("FX_T1_OptimusDeploysEnergonSword.wav", 0x0000c4e8, 0xdde57461),
				new KnownFile("FX_T1_OptimusHitsBuilding.wav", 0x000290c4, 0xada7be0b),
				new KnownFile("FX_T1_OptimusSheathssEnergonSword.wav", 0x00009df2, 0x70a5e475),
				new KnownFile("FX_T1_OptimusTransforms_FX001.wav", 0x00021d54, 0x0273d4f3),
				new KnownFile("FX_T1_OptimusTransforms_FX002.wav", 0x0001b6e4, 0xec1ee8ae),
				new KnownFile("FX_T1_OptimusTransforms_FX003.wav", 0x0001b69e, 0xc6a5dd46),
				new KnownFile("FX_T1_OptimusTransforms_FX008.wav", 0x000122c8, 0x07026eff),
				new KnownFile("FX_T1_OptimusTransforms_FX010.wav", 0x00018f3e, 0xcc314442),
				new KnownFile("FX_T1_OptimusTransforms_FX011.wav", 0x0001f83e, 0x5ae47165),
				new KnownFile("FX_T1_OptimusTransforms_FX012.wav", 0x0001c318, 0x10cb2d5a),
				new KnownFile("FX_T1_StarscreamFlyBy_01.wav", 0x0001a26c, 0xee66db34),
				new KnownFile("FX_T1_StarscreamFlyBy_02.wav", 0x00029252, 0x46e6be76),
				new KnownFile("FX_T1_StarscreamFlyOver.wav", 0x00015d24, 0x3a3a2dfc),
				new KnownFile("FX_T1_StarscreamFlyOver_02.wav", 0x00024f5a, 0xe969a189),
				new KnownFile("FX_T1_StarscreamFlyOver_03.wav", 0x0002523e, 0xdadbc4a2),
				new KnownFile("FX_T1_WindingUpCubePower.wav", 0x0001bef4, 0x70c68eef),
				new KnownFile("FX_T2_OptimusLandsPunch_02.wav", 0x00014cfa, 0x4bc001bd),
				new KnownFile("FX_T2_OptimusLandsPunch_07.wav", 0x00013530, 0x22af841d),
				new KnownFile("FX_T2_OptimusLandsSloMoPunch_01.wav", 0x000131fe, 0xd41a1bc6),
				new KnownFile("FX_T2_OptimusWacksGrindor_01.wav", 0x00017c3e, 0x7d909730),
				new KnownFile("FX_TCT_CommercialBumper_01.wav", 0x00028fb8, 0x4897c740),
				new KnownFile("FX_TiltWarning_01.wav", 0x000157b8, 0xc3187c2e),
				new KnownFile("FX_UnlitDecepticonLane_01.wav", 0x0000bc60, 0x13507fad),
				new KnownFile("FX_UnlitDecepticonLane_02.wav", 0x0000f4a0, 0xf83bbfb5),
				new KnownFile("FX_UnlitDecepticonLane_03.wav", 0x00010c92, 0x0fdea2c5),
				new KnownFile("FX_Warning_04b.wav", 0x00009cfc, 0xd1864ca2),
				new KnownFile("FXT1SomeThingEnabled.wav", 0x00023806, 0x9a657fc9),
				new KnownFile("VO_IH_001a_It_s_gonna_take_more_than_you_rust_buckets_to_stop_me.wav", 0x00012d32, 0x4af9444a),
				new KnownFile("VO_IH_002a_Your_days_are_numbered_now_decepi_creeps.wav", 0x00010ab2, 0x640165f4),
				new KnownFile("VO_IH_007a_Lets_blast_their_tail_rudders_out_of_there.wav", 0x0000f82e, 0xd24eb5c8),
				new KnownFile("VO_IH_012a_Say_your_prayers_Decepti_crumbs.wav", 0x0000e332, 0xc32e7329),
				new KnownFile("VO_IH_014a_You_Decepti_jerks_fell_lucky.wav", 0x0000f4b2, 0xea8dbc3e),
				new KnownFile("VO_IH_016a_Decepticon_punk.wav", 0x00009364, 0xadbaa25b),
				new KnownFile("VO_IH_Let_s_bust_Decepti_chops_1_add_on.wav", 0x0000c368, 0xb6adf630),
				new KnownFile("VO_IH_Your_Days_Are_Numbered_1_add_on.wav", 0x000107b2, 0x93a63cc9),
				new KnownFile("VO_M_015c_Feel_my_power.wav", 0x0000c50c, 0xeca58d29),
				new KnownFile("VO_M_037a_You_cannot_stop_me_no_one_stops_Megatron.wav", 0x00022132, 0x38dfab2e),
				new KnownFile("VO_M_056a_Optimus.wav", 0x0000cb32, 0x8b837f52),
				new KnownFile("VO_M_076a_You_re_runnin_out_of_time.wav", 0x00011332, 0xeb13870c),
				new KnownFile("VO_Mfx_001a_Wait_I_still_function.wav", 0x00015340, 0x7c1c9510),
				new KnownFile("VO_Mfx_003a_Devastator.wav", 0x0000824a, 0xd5fe14a3),
				new KnownFile("VO_Mfx_004a_Shockwave.wav", 0x00008284, 0xe6e752fd),
				new KnownFile("VO_Mfx_005a_Megatron.wav", 0x00009f26, 0x95fb455d),
				new KnownFile("VO_Mfx_008a_Blackout.wav", 0x00007ba0, 0x171f7760),
				new KnownFile("VO_Mfx_009a_Starscream.wav", 0x0000aa80, 0xea916830),
				new KnownFile("VO_Mfx_013b_This_is_my_planet.wav", 0x0000ec20, 0xda65d252),
				new KnownFile("VO_Mfx_015c_Feel_my_power.wav", 0x0000bd40, 0x863cc746),
				new KnownFile("VO_Mfx_016b_Worthless.wav", 0x00005f80, 0xe9ef7c87),
				new KnownFile("VO_Mfx_017b_Pathetic.wav", 0x00005f80, 0x97d1367b),
				new KnownFile("VO_Mfx_018b_Pitiful.wav", 0x00005620, 0x134f3bbe),
				new KnownFile("VO_Mfx_019a_you_re_so_weak.wav", 0x0000d960, 0x3354af6c),
				new KnownFile("VO_Mfx_032a_Decepticons_Scramble.wav", 0x0000ec20, 0x3bd5a1ed),
				new KnownFile("VO_Mfx_033b_None_shall_stand_in_my_way.wav", 0x00012dc0, 0xd9b2fff4),
				new KnownFile("VO_Mfx_041a_We_are_indestructible.wav", 0x0000fee0, 0x46d77b30),
				new KnownFile("VO_Mfx_045a_I_bring_chaos.wav", 0x0000ec20, 0xe1f0da2e),
				new KnownFile("VO_Mfx_054a_I_ll_crush_you_with_my_bare_hands.wav", 0x00016f60, 0x32bb66f3),
				new KnownFile("VO_Mfx_085_Collected.wav", 0x000056cc, 0x673bf561),
				new KnownFile("VO_Mfx_085c_Collected.wav", 0x00005f00, 0x1085e86e),
				new KnownFile("VO_Mfx_Imbecile_added_line.wav", 0x00007ba0, 0x6a19d1a5),
				new KnownFile("VO_Mfx_Time_is_Running_Out_Prime_3_added_line.wav", 0x000111a0, 0xf1119c8b),
				new KnownFile("VO_OP_001a_Bumblebee.wav", 0x000065f4, 0x5220ad71),
				new KnownFile("VO_OP_001b_Bumblebee.wav", 0x00006a32, 0xaf362403),
				new KnownFile("VO_OP_001c_Bumblebee.wav", 0x0000795a, 0x9ffc87ae),
				new KnownFile("VO_OP_001d_Bumblebee.wav", 0x00007532, 0x3e3cecfa),
				new KnownFile("VO_OP_003a_collected.wav", 0x00007932, 0x16fbf082),
				new KnownFile("VO_OP_004a_Optimus_Prime.wav", 0x00009df2, 0xc6f9b8e3),
				new KnownFile("VO_OP_005a_Ironhide.wav", 0x000070d6, 0x027be7b6),
				new KnownFile("VO_OP_006a_Mudflap_and_Skids.wav", 0x0000c5b2, 0x669165bc),
				new KnownFile("VO_OP_007a_Ratchet.wav", 0x0000562c, 0x85e58fc5),
				new KnownFile("VO_OP_042a_Multiball.wav", 0x00008e8a, 0x15fb3bd1),
				new KnownFile("VO_OP_042b_Multiball.wav", 0x000080f0, 0x825c02b6),
				new KnownFile("VO_OP_042c_Multiball.wav", 0x00007ec0, 0x120b4fcf),
				new KnownFile("VO_OP_048b_Field_is_hot_Move_move.wav", 0x0000eaf4, 0xf013bdd2),
				new KnownFile("VO_OP_050a_It_is_time_for_battle.wav", 0x0000c668, 0x8a0013c5),
				new KnownFile("VO_OP_056a_Keep_it_steady.wav", 0x00006b92, 0x02e00194),
				new KnownFile("VO_OP_088a_Super_Jackpot.wav", 0x00008154, 0xea88f3ac),
				new KnownFile("VO_OP_Optimus_Prime_Multiball_added_line.wav", 0x0000f410, 0xa220921d),
				new KnownFile("VO_SS_001a_AHHHH_you_flesh_creatures_will_pay_for_that.wav", 0x00019a32, 0x20b87e6d),
				new KnownFile("VO_SS_002a_Pathetic_fools_there_s_no_escape.wav", 0x0001b632, 0x97c382e5),
				new KnownFile("VO_SS_Decepticons_Attack_added_line.wav", 0x0000c672, 0x08a0b751),
				new KnownFile("VO_SS_You_re_No_Match_for_Lord_Megatron_added_line.wav", 0x00015d32, 0x854fc2bb),
				new KnownFile("VO_T1_00054_OP_Before_time_began_there_was_the_Cube.wav", 0x00019632, 0xe40c6909),
				new KnownFile("VO_T1_00059_OP_001_WeKnowNotWhereItComesFrom.wav", 0x0000b84a, 0xe9c62e67),
				new KnownFile("VO_T1_00059_OP_002_OnlyThatItHoldsThePowerToCreateWorlds.wav", 0x0001595c, 0xd4dca73a),
				new KnownFile("VO_T1_00059_OP_003_AndFillThem.wav", 0x00007e12, 0x3f2f105a),
				new KnownFile("VO_T1_00059_OP_004_WithLife.wav", 0x00004b28, 0x9fae996e),
				new KnownFile("VO_T1_00111_OP_001_ForATimeWeLivedInHarmony.wav", 0x0000c9ca, 0x4de832a4),
				new KnownFile("VO_T1_00111_OP_002_ButLikeAllGreatPower.wav", 0x0000b12e, 0xfe663028),
				new KnownFile("VO_T1_00111_OP_003_SomeWantedItForGood.wav", 0x0000bef0, 0xb0188d8d),
				new KnownFile("VO_T1_00111_OP_004_OthersForEvil.wav", 0x0000913c, 0x275f3ceb),
				new KnownFile("VO_T1_00121_OP_And_so_began_001_AndSoBeganTheWar.wav", 0x0000ebe8, 0x6437d156),
				new KnownFile("VO_T1_00121_OP_And_so_began_002_AWarThatRavagedOurPlanet.wav", 0x0001d0e8, 0xf0cc42f6),
				new KnownFile("VO_T1_00121_OP_And_so_began_003_AndTheCube.wav", 0x00007534, 0x4c77cec8),
				new KnownFile("VO_T1_00121_OP_And_so_began_004_WasLostToTheFarReachesOfSpace.wav", 0x0000fb7c, 0xf575f4fd),
				new KnownFile("VO_T1_00134_OP_001_weScatteredAcrossTheGalaxy.wav", 0x0000d54c, 0x499f8e1b),
				new KnownFile("VO_T1_00134_OP_002_HopingToFindIt.wav", 0x000080ca, 0x1e7dd828),
				new KnownFile("VO_T1_00134_OP_003_AndRebuildOurHome.wav", 0x00009658, 0xff5eb538),
				new KnownFile("VO_T1_00134_OP_004_SearchingEveryStar.wav", 0x0000b18a, 0xa75aa223),
				new KnownFile("VO_T1_00134_OP_005_EveryWorld.wav", 0x00006f84, 0x83ac252f),
				new KnownFile("VO_T1_00146_OP_001_andJustWhenAllHopeSeemedLost.wav", 0x0000fb08, 0x7e0a475d),
				new KnownFile("VO_T1_00146_OP_002_MessageOfANewDiscovery.wav", 0x0000b02a, 0xd115b0a0),
				new KnownFile("VO_T1_00146_OP_003_DrewUsToAnUnknownPlanetCalled.wav", 0x00010eb0, 0x8eee0da6),
				new KnownFile("VO_T1_00146_OP_004_Earth.wav", 0x0000433a, 0x81b0a5f6),
				new KnownFile("VO_T1_00203_OP_But_we_were_already_too_late.wav", 0x0000d248, 0xf5b94b39),
				new KnownFile("VO_T1_10643_MEG_I_am_Megatron.wav", 0x0001295a, 0x11af8828),
				new KnownFile("VO_T1_12651_MEG_Oh_so_unwise.wav", 0x000109ca, 0xc422c9d3),
				new KnownFile("VO_T2_00631_MUD_This_is_combat_man.wav", 0x00006056, 0x687e2eda),
				new KnownFile("VO_T2_06352_MEG_Its_time_for_the_world_to_know_of_our_presence.wav", 0x0000fba4, 0x9d56ab05),
				new KnownFile("VO_T2_06356_MEG_No_more_disguises_No_mercy.wav", 0x0001295c, 0xea4fc443),
				new KnownFile("VO_T2_10227_MUD_Now_thats_rude.wav", 0x00004d7c, 0x365d7234),
				new KnownFile("VO_T2_11754_MUD_You_aint_getting_me.wav", 0x0000bf3c, 0x9ebe32bc),
				new KnownFile("VO_T2_11844_MUD_Nobody_messes_with_the_twins.wav", 0x0000872e, 0x0a3580e7),
				new KnownFile("VO_T3_opt_get_down_here_sentinel.wav", 0x0000c5ea, 0xd06285ee),
				new KnownFile("VOfx_d_LookingForTheAllSpark.wav", 0x000137de, 0x83f4e78b),
				new KnownFile("VOfx_E_LookingForTheAllSpark.wav", 0x000138f6, 0xf138b24b),
				new KnownFile("VOfx_OP_010a_Extra_ball.wav", 0x00008e06, 0xa2778def),
				new KnownFile("VOfx_OP_013a_Autobots_roll_out.wav", 0x0000c4aa, 0xb285b106),
				new KnownFile("VOfx_OP_017a_Player_2.wav", 0x000074d6, 0x7b831323),
				new KnownFile("VOfx_OP_020a_Megatron_you_re_a_bucket_of_rust.wav", 0x00011cea, 0x01c9eb2e),
				new KnownFile("VOfx_OP_024a_Stick_it_in_neutral_Megatron.wav", 0x0000f314, 0x68dfe30e),
				new KnownFile("VOfx_OP_026b_Megatron_must_be_stopped_no_matter_the_cost.wav", 0x0000b59a, 0xb24f8bc7),
				new KnownFile("VOfx_OP_037a_We_are_one_step_closer_to_stopping_the_Decepticons.wav", 0x00019030, 0x518cc3d3),
				new KnownFile("VOfx_OP_045a_Protect_the_humans.wav", 0x0000b118, 0x5548f8e4),
				new KnownFile("VOfx_OP_046c_Scan_your_perimeter.wav", 0x0000985c, 0xe23f6e59),
				new KnownFile("VOfx_OP_049a_Always_believe_that_good_will_triumph.wav", 0x000121be, 0x9626a295),
				new KnownFile("VOfx_OP_050a_It_is_time_for_battle.wav", 0x0000c7e8, 0x8f071633),
				new KnownFile("VOfx_OP_051f_Autobots_transform_and_roll_out.wav", 0x00010cce, 0xa30bf3d1),
				new KnownFile("VOfx_OP_053a_Trust_your_instincts.wav", 0x00008a44, 0x9e9c6dea),
				new KnownFile("VOfx_OP_056a_Keep_it_steady.wav", 0x00006d08, 0x62a60a34),
				new KnownFile("VOfx_OP_057a_Autobots_This_is_going_to_be_a_very_dangerous_mission.wav", 0x000186a2, 0xe972efd3),
				new KnownFile("VOfx_OP_058b_We_re_counting_on_you_don_t_drop_the_ball.wav", 0x0000fdfc, 0x2ca1b0ed),
				new KnownFile("VOfx_OP_080a_Megatron_it_s_over.wav", 0x0000c91c, 0xe99f0746),
				new KnownFile("VOfx_OP_084b_Shoot_Again.wav", 0x00007a1a, 0xffae0810),
				new KnownFile("VOfx_OP_088d_Super_Jackpot.wav", 0x000092c6, 0x32217901),
				new KnownFile("VOfx_OP_090a_Three.wav", 0x00005d7e, 0x4a899303),
				new KnownFile("VOfx_OP_091a_Two.wav", 0x000055a6, 0xe76eda5b),
				new KnownFile("VOfx_OP_092a_Four.wav", 0x00006534, 0x7732b177),
				new KnownFile("VOfx_OP_102b_Battle_for_Cybertron_is_ready.wav", 0x0000feac, 0x8ecd68f4),
				new KnownFile("VOfx_OP_Face_Me_Megatron_2_added_line.wav", 0x0000be98, 0xfbde0ae1),
				new KnownFile("VOfx_OP_jack_pot_1_added_line.wav", 0x00005f16, 0xbd024f18),
				new KnownFile("VOfx_OP_jack_pot_2_added_line.wav", 0x00005dae, 0x5c8d05da),
				new KnownFile("VOfx_OP_One_added_line.wav", 0x000057c2, 0xb8896ac1),
				new KnownFile("VOfx_OP_Optimus_Prime_Multiball_added_line.wav", 0x0000ec6e, 0x36af267a),
				new KnownFile("VOfx_OP_robot_JackPOT_.wav", 0x00004ef4, 0x4599896b),
				new KnownFile("windDown02.wav", 0x00032c74, 0x3c706081) } },
			new KnownSet { Set = "avscp_103", Folder = "avengers-elg", Title = "Avengers The Pin 1.0.3", Files = new KnownFile[] {
				new KnownFile("game", 0x0010c319, 0x78eecb7e),
				new KnownFile("BG_BattleForEarth.wav", 0x001c202e, 0xd6b9dd97),
				new KnownFile("BG_BlackWidowLoop.wav", 0x00493770, 0x4d8586ff),
				new KnownFile("BG_BlackWidowShooterGroove.wav", 0x000bb82e, 0x0116a36b),
				new KnownFile("BG_BonusStart.wav", 0x00010e2e, 0x540f8393),
				new KnownFile("BG_BonusTotal.wav", 0x00026628, 0x51e87953),
				new KnownFile("BG_CaptainAmericaLoop.wav", 0x002d9f90, 0x94a4c43c),
				new KnownFile("BG_CaptainAmericaShooterGroove.wav", 0x000abc44, 0x86f4259a),
				new KnownFile("BG_HawkeyeLoop.wav", 0x00282de4, 0x014425aa),
				new KnownFile("BG_HawkeyeShooterGroove.wav", 0x000a0b9a, 0x7d2707ca),
				new KnownFile("BG_HulkLoop.wav", 0x0017702e, 0x77835be7),
				new KnownFile("BG_HulkShooterGroove.wav", 0x0009602e, 0x33389b03),
				new KnownFile("BG_IronManLoop.wav", 0x00218f14, 0xbf1c49ae),
				new KnownFile("BG_IronManShooterGroove.wav", 0x0006648a, 0x541db5e1),
				new KnownFile("BG_LokiLoop.wav", 0x0026162e, 0x82474d0f),
				new KnownFile("BG_MainLoop.wav", 0x002b4518, 0x6ccf9865),
				new KnownFile("BG_MainShooterGrooveLoop.wav", 0x000ad168, 0xbbe82027),
				new KnownFile("BG_Tesseract_01.wav", 0x0011942e, 0xdf559de4),
				new KnownFile("BG_Tesseract_Terminate.wav", 0x000639ee, 0x8a8e3004),
				new KnownFile("BG_ThorLoop.wav", 0x0027602c, 0x5bac4b69),
				new KnownFile("BG_ThorShooterGroove.wav", 0x000b402e, 0x156fbac2),
				new KnownFile("FX_Add_A_Player.wav", 0x00020a32, 0x1cb1f8b7),
				new KnownFile("FX_Advance_CAmerica_001.wav", 0x000262ae, 0xf10948d5),
				new KnownFile("FX_Advance_Hawkeye_1.wav", 0x0001413a, 0x4ffd7a75),
				new KnownFile("FX_Advance_Hawkeye_2_3.wav", 0x0002176e, 0xe480145d),
				new KnownFile("FX_Advance_IronMan_1.wav", 0x0002f7f2, 0x2bb92cfc),
				new KnownFile("FX_Advance_IronMan_2_4.wav", 0x0002f434, 0x5766151b),
				new KnownFile("FX_Advance_Thor_001.wav", 0x000271ae, 0xcdadb2e9),
				new KnownFile("FX_Advance_Thor_2_4.wav", 0x00014ff2, 0xe8e54a3f),
				new KnownFile("FX_AutoPlunge.wav", 0x00022c96, 0x59a0ec77),
				new KnownFile("FX_Ball_Save.wav", 0x000098ca, 0xe28f9ac3),
				new KnownFile("FX_Bang01.wav", 0x00018108, 0x1bc4b26a),
				new KnownFile("FX_BonusSeqMult.wav", 0x00011a4c, 0xe6fa7523),
				new KnownFile("FX_Bottom_Lane_Lit.wav", 0x00015314, 0x56df9790),
				new KnownFile("FX_Bottom_Lanes_Complete.wav", 0x00024f72, 0xcd0a8f93),
				new KnownFile("FX_BottomLane_Unlit.wav", 0x0002686c, 0x02b2a928),
				new KnownFile("FX_Bung_01.wav", 0x0001307e, 0x49fce7c5),
				new KnownFile("FX_BWidow_Advance_001.wav", 0x0001c788, 0x80e075f3),
				new KnownFile("FX_BWidow_Advance_002_4.wav", 0x000233b8, 0xa6235ee2),
				new KnownFile("FX_BWidow_Mode_Start.wav", 0x0002b71a, 0x69dfc830),
				new KnownFile("FX_BWidow_Scores_Letter.wav", 0x00022d76, 0xd17ff301),
				new KnownFile("FX_CA_PopBumperHits_001.wav", 0x0000fe76, 0xaa70c195),
				new KnownFile("FX_CA_PopBumperHits_002.wav", 0x0000e176, 0xe76345e9),
				new KnownFile("FX_CA_PopBumperHits_003.wav", 0x0001b136, 0xbda896c6),
				new KnownFile("FX_CA_PopBumperHits_004.wav", 0x0000cc66, 0x4fa8740a),
				new KnownFile("FX_CAFights_001.wav", 0x00008e1e, 0xbc14702e),
				new KnownFile("FX_CAmerica_Mode_Start.wav", 0x00025448, 0xfa7c81f7),
				new KnownFile("FX_Captain_Bonus.wav", 0x0002b140, 0xf1c349fd),
				new KnownFile("FX_Captain_Bonus_2.wav", 0x0002cc40, 0xb69dd80f),
				new KnownFile("FX_DE_BW_FacePunch.wav", 0x0001b506, 0xd59956dc),
				new KnownFile("FX_DE_BW_Guns.wav", 0x0001724c, 0x4914678d),
				new KnownFile("FX_DE_IM_ChestWeapon.wav", 0x00017e90, 0x946c0dd0),
				new KnownFile("FX_DE_IronManFlyBy.wav", 0x00010a72, 0xbff2cb79),
				new KnownFile("FX_DE_SlideBang_01.wav", 0x0000c3cc, 0x078c9ab0),
				new KnownFile("FX_dink.wav", 0x00001598, 0x56fbd13f),
				new KnownFile("FX_Electricity_003.wav", 0x000118dc, 0x4d747d33),
				new KnownFile("FX_Electricity_004.wav", 0x000150ec, 0x4f3a9101),
				new KnownFile("FX_Enter_Initials_completed.wav", 0x0002b9d6, 0xd3542d37),
				new KnownFile("FX_Extra_Ball_Get.wav", 0x0002d4b6, 0x8f23dc8c),
				new KnownFile("FX_Extra_Ball_Lit.wav", 0x0001e606, 0x846700ac),
				new KnownFile("FX_Hawkeye_Bonus_2.wav", 0x000114ec, 0x2926d14d),
				new KnownFile("FX_Hawkeye_Mode_Start.wav", 0x0002cbf8, 0x4099b82e),
				new KnownFile("FX_Hawkeye_Scores_Letter.wav", 0x0001e6ec, 0x022a39c5),
				new KnownFile("FX_Heli_Score.wav", 0x0001dc5c, 0xadb64e0f),
				new KnownFile("FX_Hulk_Bonus_2.wav", 0x0001e9d8, 0xbd299d73),
				new KnownFile("FX_Hulk_Multiball.wav", 0x00049324, 0x98b05a10),
				new KnownFile("FX_Hulk_Not_Avail_001.wav", 0x0001566e, 0xea97de96),
				new KnownFile("FX_Hulk_Not_Avail_002.wav", 0x00015622, 0x08058099),
				new KnownFile("FX_Hulk_Not_Avail_003.wav", 0x00012656, 0x9b0dfd97),
				new KnownFile("FX_Hulk_Scores_Letters_H.wav", 0x0001442a, 0xb526f78c),
				new KnownFile("FX_Hulk_Scores_Letters_L.wav", 0x00014b3a, 0xb3d2156e),
				new KnownFile("FX_Hulk_Scores_Letters_U.wav", 0x00014b3a, 0xefbc230b),
				new KnownFile("FX_Hulk_Smash_001.wav", 0x00013198, 0x320a60fa),
				new KnownFile("FX_Hulk_Smash_002.wav", 0x0001508c, 0x7df7b19c),
				new KnownFile("FX_Hulk_Smash_003.wav", 0x000166ae, 0xa3c179b1),
				new KnownFile("FX_IronMan_Bonus_2.wav", 0x000114ec, 0xc1f69cbe),
				new KnownFile("FX_IronMan_Mode_Start.wav", 0x0001848c, 0xee7b5ffc),
				new KnownFile("FX_IronManDot.wav", 0x0002a862, 0x922d93ac),
				new KnownFile("FX_Loki_Lock_001.wav", 0x000265a8, 0x63b0debd),
				new KnownFile("FX_Loki_Lock_002.wav", 0x0002f8bc, 0x18710cac),
				new KnownFile("FX_Loki_Multiball_Start.wav", 0x0004d3b6, 0xc5e6c58a),
				new KnownFile("FX_Mystery_.wav", 0x000369d8, 0x38544e46),
				new KnownFile("FX_Plunger_Ball_Launch.wav", 0x0003527a, 0x37a80d4d),
				new KnownFile("FX_SHIELD_Completed.wav", 0x0001882a, 0x47f37ed5),
				new KnownFile("FX_SHIELD_null.wav", 0x0000af88, 0xe3737ccf),
				new KnownFile("FX_SHIELD_unlit.wav", 0x0001dfd2, 0x99a97c40),
				new KnownFile("FX_ShootAgain.wav", 0x0002ba6e, 0x8c7c2412),
				new KnownFile("FX_SILENCE_0400_MS.wav", 0x000089fc, 0x1f069d20),
				new KnownFile("FX_Skill_Shot_Award.wav", 0x000248e4, 0x28021d5f),
				new KnownFile("FX_Slingshots_001.wav", 0x00023322, 0xd54b275e),
				new KnownFile("FX_Slingshots_002.wav", 0x00023322, 0x4d7ea891),
				new KnownFile("FX_Slingshots_003.wav", 0x00023322, 0x58d835ae),
				new KnownFile("FX_Tess_Seg_000.wav", 0x0001ce7e, 0xc8f00370),
				new KnownFile("FX_Tess_Seg_001.wav", 0x0001d83e, 0x578b9a86),
				new KnownFile("FX_Thor_Bonus_2.wav", 0x00029cb4, 0x8389edf5),
				new KnownFile("FX_Thor_Hammer_Returns.wav", 0x0000b44e, 0x4842503a),
				new KnownFile("FX_Thor_Hit_Null_001.wav", 0x0000a300, 0x7a5ee36a),
				new KnownFile("FX_Thor_Hit_Null_002.wav", 0x0000a300, 0xdaa186d7),
				new KnownFile("FX_Thor_Mode_Start.wav", 0x00026b9a, 0xe443f5e7),
				new KnownFile("FX_Thor_Score_001.wav", 0x00022252, 0xe6b13f14),
				new KnownFile("FX_Thor_Thunder_001.wav", 0x0003f4fa, 0x8475f5fd),
				new KnownFile("FX_Thor_Thunder_002.wav", 0x00013bf0, 0x41ec94b4),
				new KnownFile("FX_Thor_Thunder_003.wav", 0x00025b9c, 0x9ee0b809),
				new KnownFile("FX_Thor_Thunder_004.wav", 0x0000a666, 0x33eac99e),
				new KnownFile("FX_Tilt_Tilt.wav", 0x00034776, 0xbff9386d),
				new KnownFile("FX_Tilt_Warning.wav", 0x0001668c, 0xeef13fd4),
				new KnownFile("FX_Top_Lanes_Lit.wav", 0x00015cc2, 0x34a8b99c),
				new KnownFile("FX_Top_Lanes_Unlit.wav", 0x00016bd2, 0x02c4fd0e),
				new KnownFile("VO_001_AN_BLACK_WIDOW.wav", 0x00007dea, 0x543fa6d7),
				new KnownFile("VO_002_AN_CAPTAIN_AMERICA.wav", 0x00006e58, 0xc61cae1d),
				new KnownFile("VO_004_AN_HAWKEYE.wav", 0x000046da, 0xd108f6cb),
				new KnownFile("VO_005_AN_HULK.wav", 0x0000458e, 0x5f47d8e1),
				new KnownFile("VO_006_AN_IRONMAN.wav", 0x00005f06, 0xff877b4e),
				new KnownFile("VO_007_AN_THOR.wav", 0x00005488, 0xa1e52eb6),
				new KnownFile("VO_020_AN_BLACK_WIDOW_COLLECTED.wav", 0x00009dd6, 0x2346c7d6),
				new KnownFile("VO_020_AN_BLACK_WIDOW_COMPLETED.wav", 0x0000b1b4, 0xfe68485e),
				new KnownFile("VO_021_AN_CAPTAIN_AMERICA_COLLECTED.wav", 0x0000d6c6, 0x80907aa2),
				new KnownFile("VO_021_AN_CAPTAIN_AMERICA_COMPLETED.wav", 0x0000ca18, 0xbc1d3f8d),
				new KnownFile("VO_022_AN_HAWKEYE_COMPLETED.wav", 0x0000b178, 0xba990573),
				new KnownFile("VO_022_AN_HAWKEYE_COMPLETED_A.wav", 0x0000b39e, 0x90090ea9),
				new KnownFile("VO_023_AN_JACKPOT_1.wav", 0x00004f58, 0x77d5f877),
				new KnownFile("VO_023_AN_JACKPOT_2.wav", 0x000054ea, 0xd28647ca),
				new KnownFile("VO_023_AN_JACKPOT_3.wav", 0x00007540, 0x6df32a2b),
				new KnownFile("VO_023_AN_JACKPOT_4.wav", 0x0000747a, 0x177b1e46),
				new KnownFile("VO_023_AN_JACKPOT_5.wav", 0x00005a48, 0x6fd81945),
				new KnownFile("VO_024_AN_IRONMAN_COLLECTED.wav", 0x0000abfa, 0xe32c8ba3),
				new KnownFile("VO_024_AN_IRONMAN_COMPLETED.wav", 0x00008fa2, 0xd2b73f1f),
				new KnownFile("VO_025_AN_THOR_COLLECTED.wav", 0x0000b17c, 0x8ed1d7fd),
				new KnownFile("VO_025_AN_THOR_COMPLETED.wav", 0x000096ae, 0xdd670932),
				new KnownFile("VO_026_AN_HULK_COLLECTED.wav", 0x00009dda, 0x6a3d2c3d),
				new KnownFile("VO_026_AN_HULK_COMPLETED.wav", 0x0000c6ce, 0x70c933d9),
				new KnownFile("VO_035_BW_NEVER_UNDERESTIMATE_ME.wav", 0x0000a4ac, 0x33dea6e3),
				new KnownFile("VO_036_BW_MY_MAIN_WEAPON_IS_MY_CHARM.wav", 0x0000c8b2, 0x14e525e3),
				new KnownFile("VO_037_BW_MY_NAME_IS_NATASHA_BUT_YOU_CAN_CALL_ME_THE_BLA.wav", 0x00017550, 0xf904fd81),
				new KnownFile("VO_038_BW_CROSS_ME_AND_YOULL_FIND_OUT_HOW_I_GOT_MY_NAMES.wav", 0x000171a2, 0x4cce96c5),
				new KnownFile("VO_039_BW_DID_I_MENTION_IM_AN_EXPERT_GYMNAST.wav", 0x0000e966, 0x7bb13ed8),
				new KnownFile("VO_040_BW_IVE_DONE_THINGS_IM_NOT_PROUD_OF_ID_LIKE_TO_ATO.wav", 0x00014e96, 0xa494ee40),
				new KnownFile("VO_041_BW_AGENT_BARTON_WAS_SENT_TO_KILL_ME_HE_MADE_A_DIF.wav", 0x00013d1c, 0x24827f2e),
				new KnownFile("VO_042_BW_AVENGERS_ASSEMBLE.wav", 0x0000973c, 0xcdfc7dce),
				new KnownFile("VO_043_BW_REGIMES_FALL_EVERYDAY_I_TEND_NOT_TO_WEEP_OVER_.wav", 0x00012ce2, 0x437e1369),
				new KnownFile("VO_080_CA_THEY_CALL_ME_THE_FIRST_AVENGER_NOW_ILL.wav", 0x00014942, 0xb7770311),
				new KnownFile("VO_081_CA_WE_AVENGERS_WILL_ALWAYS_FIGHT_THE_GOOD.wav", 0x0000d8e8, 0xe8d44788),
				new KnownFile("VO_083_CA_AVENGERS_ASSEMBLE_1.wav", 0x0000965e, 0xe2879a36),
				new KnownFile("VO_085_CA_I_DONT_LIKE_BULLIES_NO_MATTER_WHERE_TH.wav", 0x0001227c, 0xe7d8c3bf),
				new KnownFile("VO_086_CA_IM_JUST_A_KID_FROM_BROOKLYN.wav", 0x00008844, 0x602cdafb),
				new KnownFile("VO_087_CA_IF_YOU_CHOOSE_TO_STAND_IN_THE_WAY_OF_F.wav", 0x0002501a, 0x06c66d69),
				new KnownFile("VO_088_CA_WE_HAVE_A_MISSION_TO_CARRY_OUT_PEOPLE_.wav", 0x0001250e, 0xf18f5553),
				new KnownFile("VO_089_CA_YOU_HAVE_YOUR_DEFINITION_OF_A_HERO_AND.wav", 0x0001e4d2, 0x0b2a6797),
				new KnownFile("VO_090_CA_THIS_SOLDIER_FIGHTS_FOR_FREEDOM_AND_JU.wav", 0x0000de52, 0xb8866ec0),
				new KnownFile("VO_091_CA_I_FIGHT_FOR_THE_FLAG.wav", 0x0000854e, 0x11a6342b),
				new KnownFile("VO_092_CA_MISTER_YOU_JUST_MADE_THE_GREATEST_MIST.wav", 0x0000ffa0, 0xdc03e33e),
				new KnownFile("VO_093_CA_YOU_THINK_YOURE_TOUGH_WELL_LETS_SEE_YO.wav", 0x0001249c, 0x5398b53a),
				new KnownFile("VO_095_CA_WE_HAVE_NO_CHOICE_SO_WE_FIGHT_AND_WE_W.wav", 0x0001fd12, 0x744d4b3c),
				new KnownFile("VO_096_CA_NO_ONE_DIES_ON_MY_WATCH.wav", 0x0000a58e, 0x4ad44f3d),
				new KnownFile("VO_098_CA_THE_BATTLE_IS_NEVER_ENDED_THERE_WILL_B.wav", 0x000161c2, 0x9f7db054),
				new KnownFile("VO_099_CA_FORGET_WHAT_SETS_US_APART_AND_REMEMBER.wav", 0x000126ca, 0x977517c1),
				new KnownFile("VO_100_CA_I_WILL_ALWAYS_PROTECT_THOSE_WHO_CANNOT.wav", 0x0001246a, 0x38f16ba7),
				new KnownFile("VO_102_CA_FREEDOM_WILL_PREVAIL.wav", 0x0000a242, 0x5f984848),
				new KnownFile("VO_137_CA_SON_OF_A_GUN.wav", 0x00004fda, 0xefd09e1f),
				new KnownFile("VO_140_CA_GET_BACK_IN_THERE_SOLDIER.wav", 0x00006f18, 0xfd1a4808),
				new KnownFile("VO_141_CA_NEVER_GIVE_UP_FREEDOM_WILL_PREVAIL.wav", 0x0000e648, 0x7c0d6b2d),
				new KnownFile("VO_142_CA_GOOD_WORK_SOLDIER.wav", 0x000076ca, 0xddd220d0),
				new KnownFile("VO_156_HE_PIECE_OF_CAKE.wav", 0x00008470, 0x6806c102),
				new KnownFile("VO_158_HE_AVENGERS_ASSEMBLE.wav", 0x000080dc, 0x2040fa9b),
				new KnownFile("VO_162_HE_I_SEE_BETTER_FROM_A_DISTANCE.wav", 0x0000a3b6, 0x2e827140),
				new KnownFile("VO_163_HE_ITS_NOT_BOASTING_IF_YOU_CAN_BACK_IT_UP.wav", 0x0000cab8, 0x02644939),
				new KnownFile("VO_164_HE_BULLSEYE.wav", 0x00004870, 0x7abafdc1),
				new KnownFile("VO_166_HE_ALWAYS_ON_THE_MARK.wav", 0x000099be, 0x2b697749),
				new KnownFile("VO_167_HE_THIS_TAKES_REAL_SKILL_YA_KNOW.wav", 0x0000a66a, 0xcf63c4ce),
				new KnownFile("VO_169_HE_WORLDS_GREATEST_MARKSMAN_AT_YOUR_SERVICE.wav", 0x0000d516, 0xe8aeef8d),
				new KnownFile("VO_170_HE_THIS_IS_HOW_ITS_DONE.wav", 0x00009164, 0x3f0cc150),
				new KnownFile("VO_171_HE_I_MAKE_ANY_TEAM_LOOK_GOOD.wav", 0x0000a468, 0xb682a615),
				new KnownFile("VO_172_HE_I_GOT_THIS.wav", 0x00007642, 0x37874fa5),
				new KnownFile("VO_173_HE_I_NEVER_MISS.wav", 0x000075b8, 0xdea10b7c),
				new KnownFile("VO_193_HL_WHY_EVERYBODY_ALWAYS_PICK_ON_HULK.wav", 0x0000f736, 0x2db9b92b),
				new KnownFile("VO_195_HL_LEAVE_HULK_ALONE.wav", 0x0000af7c, 0x459750d7),
				new KnownFile("VO_196_HL_WHEN_IT_HULKS_TURN_TO_PLAY.wav", 0x0000c90c, 0xb4e5f6bc),
				new KnownFile("VO_197_HL_DONT_MAKE_HULK_COME_OVER_THERE.wav", 0x0000d4fe, 0x2e9c320d),
				new KnownFile("VO_199_HL_LAST_TIME_HULK_IN_NEW_YORK_HULK_BREAK_HARLEM.wav", 0x0001701c, 0x408a168e),
				new KnownFile("VO_200_HL_WHY_EVERYONE_ALWAYS_LAUGH_WHEN_HULK_TALK.wav", 0x00014852, 0xb1cbe402),
				new KnownFile("VO_208_HL_LEAVE_HULK_ALONE.wav", 0x00012e58, 0xe5cc0a94),
				new KnownFile("VO_223_HL_YOU_WANT_BALL_HULK_GIVE_YOU_BALL.wav", 0x00013d88, 0x73e9889c),
				new KnownFile("VO_227_HL_METAL_BALL_STAND_STILL_SO_HULK_CAN_SMASH.wav", 0x00019182, 0x59f0d05d),
				new KnownFile("VO_230_HL_CRUSH.wav", 0x00006564, 0x414c88a5),
				new KnownFile("VO_239_HL_HULK.wav", 0x000066a0, 0xdbc8a0dc),
				new KnownFile("VO_242_HL_BASH.wav", 0x0000d788, 0xa56c3df7),
				new KnownFile("VO_253_HL_ANGRY.wav", 0x0000b006, 0xde80f7f8),
				new KnownFile("VO_256_HL_SMASH.wav", 0x00009e7a, 0x5fae5883),
				new KnownFile("VO_259_HL_GIVE_HULK_BALL.wav", 0x00015fc2, 0x221708e4),
				new KnownFile("VO_266_HL_DONT_GET_HULK_ANGRY_YOU_NOT_LIKE_WHEN_HULK_ANGRY.wav", 0x0001ce82, 0x2132394a),
				new KnownFile("VO_267_HL_YOU_MAKE_HULK_ANGRY.wav", 0x0000c244, 0xf9024bbb),
				new KnownFile("VO_293_HL_WHAT_NUMBERS_FOR_NUMBERS_MAKE_HULK_DIZZY.wav", 0x00015a60, 0x301189eb),
				new KnownFile("VO_302_HL_HULK_STRONGEST_ONE_THERE_IS.wav", 0x0000e604, 0x5379d50c),
				new KnownFile("VO_303_HL_HULK_SAY_GET_BACK_HERE.wav", 0x0000a2d2, 0x322b348b),
				new KnownFile("VO_323_HL_HULK_SMASH_PUNY_PLAYER.wav", 0x0000e24c, 0xbf04cb6f),
				new KnownFile("VO_325_HL_WHAT_DOES_PUNY_PLAYER_WANT.wav", 0x0000ca8a, 0x49bb4375),
				new KnownFile("VO_341_IM_JARVIS_TRANSFER_40_MORE_POWER_TO_THE_THRUSTERS.wav", 0x0000f17a, 0x590af916),
				new KnownFile("VO_342_IM_OFFICIAL_CONSULTING_HOURS_ARE_BETWEEN_8_AND_5_EVERY_OT.wav", 0x00014c0c, 0xd737568c),
				new KnownFile("VO_343_IM_YOU_HAVE_REACHED_THE_LIFE_MODEL_DECOY_OF_TONY_STARK_PL.wav", 0x00017b90, 0x0279755b),
				new KnownFile("VO_344_IM_GENIUS_TAKES_TIME_UNLESS_YOURE_ME.wav", 0x0000b718, 0x70b554cf),
				new KnownFile("VO_345_IM_HOW_DOES_IT_FEEL_TO_BE_A_GENIUS_ONLY_I_KNOW_THE_ANSWER.wav", 0x00019d58, 0xe1253263),
				new KnownFile("VO_347_IM_JARVIS_FULL_POWER_TO_THE_REPULSERS.wav", 0x0000ba48, 0xfa09b89e),
				new KnownFile("VO_348_IM_AVENGERS_ASSEMBLE.wav", 0x00009740, 0x2e2de3c7),
				new KnownFile("VO_349_IM_IM_MORE_OF_A_LONE_WOLF.wav", 0x00008498, 0x713bf51b),
				new KnownFile("VO_350_IM_SUPERIOR_TECH_SUPERIOR_INTELLIGENCE_WHAT_CHANCE_DOES_A.wav", 0x00019f22, 0xd34d854c),
				new KnownFile("VO_351_IM_WEAPONS_CHARGING.wav", 0x00006532, 0xd4be9e69),
				new KnownFile("VO_352_IM_I_MAKE_ANY_SUIT_LOOK_GOOD_ESPECIALLY_THIS_ONE.wav", 0x00011ed2, 0x81358c90),
				new KnownFile("VO_353_IM_YEAH_APPARENTLY_IM_VOLATILE_SELF_OBSESSED_DONT_PLAY_WE.wav", 0x00019de6, 0x6ee22924),
				new KnownFile("VO_354_IM_JARVIS_CANCEL_MY_AFTERNOON_MEETINGS_THIS_IS_TOO_MUCH_F.wav", 0x00011786, 0x8594279c),
				new KnownFile("VO_356_IM_JARVIS_ACTIVATE_THE_UNIBEAM.wav", 0x00008fb4, 0xc9bb9365),
				new KnownFile("VO_357_IM_I_TOLD_YOU_I_DONT_WANT_TO_JOIN_YOUR_SUPER_SECRET_BOY_B.wav", 0x0000d21c, 0x41c60809),
				new KnownFile("VO_358_IM_THERES_NOTHING_LIKE_FLYING_AT_THE_SPEED_OF_SOUND_EY_JA.wav", 0x0000d3f4, 0x6ffd8975),
				new KnownFile("VO_359_IM_JARVIS_ARC_REACTOR_FULL_POWER.wav", 0x0000c224, 0x3416effd),
				new KnownFile("VO_360_IM_OK_IM_HERE_THE_PARTY_HAS_NOW_OFFICIALLY_STARTED.wav", 0x00010688, 0x39baba0e),
				new KnownFile("VO_417_IM_GENIUS_BILLIONAIRE_PLAYBOY_PHILANTHROPIST.wav", 0x0000dad4, 0x8e65612f),
				new KnownFile("VO_418_LO_YOU_ARE_BUT_AN_INSECT_TO_ME_BEGONE.wav", 0x0000feaa, 0x102a0d0d),
				new KnownFile("VO_426_LO_YOU_WERE_MADE_TO_BE_RULED.wav", 0x000127f6, 0xd903a058),
				new KnownFile("VO_428_LO_I_AM_LOKI_OF_ASGARD_AND_I_AM_BURDENED_WITH_GLORIOUS_PURPOSE.wav", 0x0001b9f4, 0x776cd006),
				new KnownFile("VO_430_LO_AN_ANT_HAS_NO_QUARREL_WITH_A_BOOT.wav", 0x0000eb98, 0xe18cc93a),
				new KnownFile("VO_434_LO_HOW_WILL_YOU_CONTINUE_TO_PLAY_YOUR_SILLY_GAME_WITHOUT_THIS.wav", 0x0001782a, 0xddbcdbac),
				new KnownFile("VO_435_LO_YOU_ARE_BUT_A_HUMAN_WHAT_HOPE_DO_YOU_HAVE_AGAINST_ME.wav", 0x000144b2, 0x33544dbc),
				new KnownFile("VO_437_LO_DO_YOU_POSSES_THE_TESSERACT_IF_NOT_THEN_GET_OUT_OF_MY_S.wav", 0x0001db76, 0xf38c68ae),
				new KnownFile("VO_438_LO_WHY_DONT_YOU_GO_BOTHER_MY_BROTHER_INSTEAD.wav", 0x000137b2, 0xc94b57c4),
				new KnownFile("VO_441_LO_I_WAS_A_KING_THE_RIGHTFUL_KING_OF_ASGARD_BETRAYED.wav", 0x00018da0, 0xda7ee96d),
				new KnownFile("VO_442_LO_IT_IS_I_LOKI_YOUR_NEXT_RULER.wav", 0x00015408, 0x3398c313),
				new KnownFile("VO_446_LO_KNEEL_BEFORE_ME.wav", 0x000079b0, 0x4be08f89),
				new KnownFile("VO_447_LO_IT_IS_MY_WILL_NOT_YOURS_THAT_THIS_PINBALL_MACHINE_OBEYS.wav", 0x000174c8, 0xcaaa38e2),
				new KnownFile("VO_448_LO_YET_ANOTHER_OF_EARTHS_ARTIFACTS_IN_MY_POSITION.wav", 0x00017906, 0x8b4167bc),
				new KnownFile("VO_449_LO_WHAT_MANNER_OF_THING_IS_THIS_IT_IS_NOT_FROM_ASGARD.wav", 0x000155fa, 0xa9cb6c91),
				new KnownFile("VO_450_LO_ANYTHING_I_CAN_TAKE_AWAY_FROM_YOU_BRINGS_A_SMILE_TO_MY_.wav", 0x0001bace, 0x3b6c6e02),
				new KnownFile("VO_451_LO_ILL_TAKE_THAT.wav", 0x00008320, 0x2e8c837d),
				new KnownFile("VO_452_LO_OH_WAS_THAT_YOUR_BALL_MY_APOLOGIES.wav", 0x00017da4, 0xd6c9d501),
				new KnownFile("VO_456_LO_I_GROW_TIRED_OF_YOUR_DRIBBLE_ITS_TIME_TO_END_THIS.wav", 0x00016f10, 0xe97063a2),
				new KnownFile("VO_457_LO_YOU_REALLY_HAVE_NO_IDEA_WITH_WHOM_YOU_SPEAK_DO_YOU_MORTAL.wav", 0x0001732e, 0xa017b341),
				new KnownFile("VO_458_LO_YOU_THINK_TO_THREATEN_ME.wav", 0x000090b6, 0x4171dbd3),
				new KnownFile("VO_466_LO_IM_IMPRESSED_I_DIDNT_THINK_YOU_HUMANS_HAD_THE_MENTAL_CA.wav", 0x00023370, 0xfe6b628d),
				new KnownFile("VO_471_LO_NO_IT_IS_I_WHO_SHOULD_BE_PINBALL_KING.wav", 0x00019958, 0x3517845c),
				new KnownFile("VO_480_LO_YOU_LACK_THE_SKILL_AND_FINESSE_TO_WIN.wav", 0x0000d9c4, 0x493d48d4),
				new KnownFile("VO_481_LO_YOURE_MOST_ASSUREDLY_NO_AVENGER.wav", 0x00012218, 0xb1ca0997),
				new KnownFile("VO_482_LAUGH_7.wav", 0x000058de, 0xafb894b3),
				new KnownFile("VO_483_LO_I_AM_A_GOD_AND_YOU_ARE_BUT_A_HUMAN_WHAT_COULD_YOU_POSSIBLY_HOPE_T.wav", 0x0001b02c, 0x5df9edf3),
				new KnownFile("VO_486_LO_OOPS_I_AM_SORRY_WAS_THAT_IN_YOUR_WAY.wav", 0x0001194a, 0x904af4eb),
				new KnownFile("VO_489_LO_HOW_CAN_YOU_HOPE_TO_DEFEAT_ME.wav", 0x0000d442, 0xbbd17a9d),
				new KnownFile("VO_491_LO_YOU_HAVE_WORN_OUT_YOUR_WELCOME.wav", 0x0000abd4, 0xe70e7528),
				new KnownFile("VO_497_LO_THIS_IS_LAUGHABLE.wav", 0x0000a0fc, 0x05611137),
				new KnownFile("VO_501_LO_I_SAID_KNEEEEEEEEEEELLLLLLLLLLLLLLLLL.wav", 0x0000e8e2, 0x66b704fa),
				new KnownFile("VO_506_LO_NOOOOOOOOOOO.wav", 0x00006fb8, 0xa52c44f1),
				new KnownFile("VO_512_LO_ARE_YOU_EVER_NOT_GOING_TO_FALL_FOR_THAT.wav", 0x0000c8ea, 0x31a33de2),
				new KnownFile("VO_513_LO_OH_TOO_BAD_HOW_AWFUL_IT_MUST_BE_TO_BE_HUMAN.wav", 0x00019e06, 0x5d255d1f),
				new KnownFile("VO_514_LO_THIS_PLEASES_ME.wav", 0x0000c4d8, 0x64e118ec),
				new KnownFile("VO_529_NF_INTELLIGENCE.wav", 0x0000966c, 0x787f7f61),
				new KnownFile("VO_530_NF_PHYSICAL_PROWESS.wav", 0x00009b14, 0xa03d6707),
				new KnownFile("VO_531_NF_POWER_SOURCE.wav", 0x00008114, 0xe7b6706e),
				new KnownFile("VO_532_NF_WEAPONS.wav", 0x00006906, 0x61b1da01),
				new KnownFile("VO_534_NF_EXPERTISE.wav", 0x0000891c, 0xa4c4458a),
				new KnownFile("VO_535_NF_NATASHA_ROMANOFF_CODENAME_BLACK_WIDOW.wav", 0x000123fe, 0x3ba23920),
				new KnownFile("VO_536_NF_SAVATE.wav", 0x00004480, 0x14d0aedf),
				new KnownFile("VO_537_NF_JUDO.wav", 0x000048fe, 0x855240c4),
				new KnownFile("VO_538_NF_NINJUTSU.wav", 0x000068a8, 0x36bd1233),
				new KnownFile("VO_539_NF_AIKIDO.wav", 0x000067de, 0x7555d9cd),
				new KnownFile("VO_540_NF_ESPIONAGE.wav", 0x00008a06, 0x36bf4fd8),
				new KnownFile("VO_541_NF_KARATE.wav", 0x0000719e, 0x80838312),
				new KnownFile("VO_542_NF_BOXING.wav", 0x00004eae, 0x3c25015a),
				new KnownFile("VO_543_NF_KUNG_FU.wav", 0x00006008, 0x33be7c54),
				new KnownFile("VO_544_NF_WIDOWS_LINE.wav", 0x000065de, 0xb7710b21),
				new KnownFile("VO_545_NF_WIDOWS_KISS.wav", 0x00007902, 0x153f5a1c),
				new KnownFile("VO_546_NF_HIGH.wav", 0x0000355c, 0x03cff140),
				new KnownFile("VO_547_NF_ATHLETIC.wav", 0x00006c06, 0x85f3d338),
				new KnownFile("VO_548_NF_WIDOWS_BITE.wav", 0x0000786e, 0x19e8afcf),
				new KnownFile("VO_549_NF_A_TROUBLED_AGENT_WITH_DIVIDED_LOYALTIES.wav", 0x00013e74, 0xafdde698),
				new KnownFile("VO_550_NF_RUSSIAN_SPY_TURNED_SHIELD_AGENT.wav", 0x0000ef86, 0x5f7c5e4f),
				new KnownFile("VO_551_NF_BLACK_WIDOW.wav", 0x000056c6, 0xbcc16eeb),
				new KnownFile("VO_552_NF_THE_BEAUTIFUL_DEADLY_ASSASSIN.wav", 0x0000fc7a, 0x2b6e985c),
				new KnownFile("VO_553_NF_PEAK_HUMAN_STRENGTH.wav", 0x00009234, 0x9c528e77),
				new KnownFile("VO_554_NF_STEVE_ROGERS_CODENAME_CAPTAIN_AMERICA.wav", 0x00015af4, 0x51416f48),
				new KnownFile("VO_556_NF_VAST_MILITARY_KNOWLEDGE.wav", 0x0000c84a, 0x789d53b9),
				new KnownFile("VO_557_NF_COVERT_OPS.wav", 0x000092be, 0x67e657e2),
				new KnownFile("VO_558_NF_SUPERIOR_ENDURANCE.wav", 0x0000c840, 0x863be5b6),
				new KnownFile("VO_559_NF_CUSTOM_MOTORCYCLE.wav", 0x0000a248, 0x5d3ef4bd),
				new KnownFile("VO_561_NF_ABOVE_AVERAGE.wav", 0x0000b008, 0x7d775e82),
				new KnownFile("VO_562_NF_INCREDIBLE_AGILITY.wav", 0x00009f88, 0xf203d582),
				new KnownFile("VO_563_NF_CUSTOM_BATTLE_VAN.wav", 0x0000bc78, 0x1b5c12b9),
				new KnownFile("VO_564_NF_VIBRANIUM_SHIELD.wav", 0x0000a6f0, 0x8026a56f),
				new KnownFile("VO_565_NF_CAPTAIN_AMERICA.wav", 0x00009d6a, 0x01d8d217),
				new KnownFile("VO_566_NF_WORLD_WAR_2_HERO_DISPLACED_IN_TIME.wav", 0x00012e2c, 0xe6d200bd),
				new KnownFile("VO_567_NF_A_BRILLIANT_BATTLEFIELD_TACTICIAN.wav", 0x0000efd4, 0x899b0c48),
				new KnownFile("VO_568_NF_THE_LIVING_LEGEND.wav", 0x00009060, 0x88c5aadc),
				new KnownFile("VO_569_NF_THE_SUPER_SOLDIER.wav", 0x00007f92, 0xa9036c6d),
				new KnownFile("VO_571_NF_CLINT_BARTON_CODENAME_HAWKEYE.wav", 0x00010f1c, 0xb41c2b82),
				new KnownFile("VO_572_NF_NUMEROUS_TRICK_ARROWS.wav", 0x0000c38e, 0x19ae696b),
				new KnownFile("VO_573_NF_GRANDMASTER_MARKSMAN.wav", 0x0000893e, 0x849f82c0),
				new KnownFile("VO_574_NF_DRAW_WEIGHT_BOW.wav", 0x0000788e, 0x3edf6cdf),
				new KnownFile("VO_575_NF_COVERT_OPS.wav", 0x00008a92, 0x28cf7bf8),
				new KnownFile("VO_577_NF_AVERAGE.wav", 0x00006b78, 0x2d6ae9b1),
				new KnownFile("VO_578_NF_NEVER_MISSES.wav", 0x00007eae, 0x3505d7c9),
				new KnownFile("VO_579_NF_A_SOLITARY_AGENT_WHO_HAS_TROUBLE_FOLLOWING_ORDERS.wav", 0x000152c2, 0xda3aca9d),
				new KnownFile("VO_580_NF_THE_EVER_VIGILANT_MARKSMAN.wav", 0x0000ac7e, 0xcd541fbb),
				new KnownFile("VO_581_NF_HAWKEYE.wav", 0x000056a4, 0x01043d73),
				new KnownFile("VO_582_NF_A_SPECIALIST_IN_MARKSMANSHIP_AND_MARTIAL_ARTS.wav", 0x00010bfc, 0xe801b8dc),
				new KnownFile("VO_583_NF_GENIUS_BILLIONAIRE_PLAYBOY_PHILANTHROPIST.wav", 0x00013e14, 0x9f6b0be1),
				new KnownFile("VO_585_NF_TONY_STARK_CODENAME_IRONMAN.wav", 0x00011a90, 0x03e55cff),
				new KnownFile("VO_586_NF_ADVANCED_WEAPONRY.wav", 0x00009950, 0x5253188c),
				new KnownFile("VO_587_NF_ROBOTICS.wav", 0x00006ac8, 0x6cfccbd8),
				new KnownFile("VO_588_NF_ARTIFICIAL_INTELLIGENCE.wav", 0x0000b97a, 0xd825ce1d),
				new KnownFile("VO_589_NF_CHEMISTRY.wav", 0x000053fa, 0x8384dd7d),
				new KnownFile("VO_590_NF_PHYSICS.wav", 0x00005cb0, 0x4119cb68),
				new KnownFile("VO_591_NF_ENGINEERING.wav", 0x00005a5c, 0xec8a341f),
				new KnownFile("VO_592_NF_GENIUS_LEVEL.wav", 0x0000687a, 0x9c235757),
				new KnownFile("VO_593_NF_UNI_BEAM.wav", 0x00007e42, 0x6ef099d1),
				new KnownFile("VO_594_NF_ARC_REACTOR_POWERED.wav", 0x0000ad14, 0xf7487c4b),
				new KnownFile("VO_596_NF_REPULSER_BLASTS.wav", 0x0000852c, 0x4e700bab),
				new KnownFile("VO_597_NF_HE_DOESNT_PLAY_WELL_WITH_OTHERS.wav", 0x0000bbaa, 0x9144201b),
				new KnownFile("VO_598_NF_A_VOLATILE_SELF_ABSORBED_RICH_BOY_WITH_THE_MOST_P.wav", 0x00020bcc, 0x8efdbd0a),
				new KnownFile("VO_599_NF_A_LEADER_IN_THE_FIELD_OF_ARTIFICIAL_ROBOTICS.wav", 0x00012548, 0x392de204),
				new KnownFile("VO_600_NF_A_VISIONARY_IN_WEAPONS.wav", 0x0000a464, 0xcf4ad2a3),
				new KnownFile("VO_601_NF_A_VISIONARY_IN_ENGINEERING.wav", 0x0000b44c, 0x3895bce4),
				new KnownFile("VO_602_NF_A_LEADER_IN_THE_FIELD_OF_ARTIFICIAL_INTELLIGENCE.wav", 0x0000fd64, 0x0d0c1b55),
				new KnownFile("VO_603_NF_TONY_STARK.wav", 0x00006644, 0x7a63f7a7),
				new KnownFile("VO_624_NF_UNARMED_COMBAT.wav", 0x00009344, 0x72315fe5),
				new KnownFile("VO_625_NF_MYSTICAL_ARTS.wav", 0x00009504, 0x86323b19),
				new KnownFile("VO_626_NF_MELEE_WEAPONS.wav", 0x0000819c, 0x369a88c1),
				new KnownFile("VO_626_NF_MYSTICAL_WEAPONS.wav", 0x00009fde, 0x57c12aaa),
				new KnownFile("VO_627_NF_WIND.wav", 0x00004228, 0x728b9014),
				new KnownFile("VO_630_NF_STORMS.wav", 0x000075f2, 0x2e3a8a54),
				new KnownFile("VO_631_NF_LIGHTNING.wav", 0x00007112, 0x032c69ac),
				new KnownFile("VO_632_NF_ENORMOUS_STRENGTH.wav", 0x00009496, 0x99d31948),
				new KnownFile("VO_634_NF_THOR.wav", 0x00003d0a, 0x27a3aee9),
				new KnownFile("VO_636_NF_AN_ASGARDIAN_WITH_UNPARALLELED_STRENGTH.wav", 0x00011298, 0x1796cacb),
				new KnownFile("VO_650_NF_SUPER_JACKPOT.wav", 0x000097b6, 0x69aa3841),
				new KnownFile("VO_667_NF_AVENGERS_ASSEMBLE.wav", 0x00007d5e, 0x6cb99862),
				new KnownFile("VO_687_NF_SKILL_SHOT.wav", 0x00006b32, 0x459d5116),
				new KnownFile("VO_718_TO_THE_CHITAURI_GROW_RESTLESS.wav", 0x00010dde, 0xc941b787),
				new KnownFile("VO_745_TR_THE_TESSERACT_BELONGS_ON_ASGARD.wav", 0x0000b22c, 0x4e11b836),
				new KnownFile("VO_747_TR_YOU_SPEAK_OF_CONTROL_YET_YOU_COURT_CHAOS.wav", 0x000108c4, 0x908a932c),
				new KnownFile("VO_748_TR_I_THOUGHT_HUMANS_WERE_MORE_EVOLVED_THEN_THIS.wav", 0x0000fb04, 0x7132d163),
				new KnownFile("VO_751_TR_FOR_ASGARD_1.wav", 0x00006eda, 0xe4fcdba7),
				new KnownFile("VO_752_TR_AVENGERS_ASSEMBLE.wav", 0x0000d104, 0xbd3fb320),
				new KnownFile("VO_779_TR_WHAT_MANNER_OF_TRICKERY_OR_SORCERY_IS_THIS.wav", 0x000125c8, 0xbaca1eb6),
				new KnownFile("VO_781_TR_BY_ODINS_BEARD_WHAT_IS_THAT_FLYING_CONTRAPTION.wav", 0x00016bf8, 0x733362af),
				new KnownFile("VO_786_TR_I_WILL_DEAL_WITH_THESE_VILLAINS.wav", 0x0000a7c4, 0xb46c3d91),
				new KnownFile("VO_787_TR_I_HAVE_UNFINISHED_BUSINESS_WITH_LOKI.wav", 0x0000d50c, 0xbc3c7ec0),
				new KnownFile("VO_790_TR_MIGHTY_BONUS.wav", 0x000080ca, 0x7f3ab6fc),
				new KnownFile("VO_791_TR_TO_ME_MY_BONUS.wav", 0x000096b4, 0x1ea7c1a6),
				new KnownFile("VO_BW_FX_03.wav", 0x00003b84, 0x56ad352d),
				new KnownFile("VO_HL_FX__05.wav", 0x0000486a, 0x7adb2fee),
				new KnownFile("VO_HL_FX__06.wav", 0x000043f6, 0x8fdfb843),
				new KnownFile("VO_HL_FX__13.wav", 0x00003ffe, 0x2d231500),
				new KnownFile("VO_TO_COSMIC_1.wav", 0x000078bc, 0x0de4296a),
				new KnownFile("VO_TO_COSMIC_CUBE_2.wav", 0x0000a938, 0xec103116),
				new KnownFile("VO_TO_IT_AWAKENS_BUT_HOW.wav", 0x00014ae0, 0x3f46ba96),
				new KnownFile("VO_TO_NOOOOOOO.wav", 0x000079f6, 0xa2720699),
				new KnownFile("VO_TO_PATIENCE_CHITAURI_YOUR_TIME_WILL_COME.wav", 0x0001b874, 0xc7e604e6),
				new KnownFile("VO_TO_SOON_THE_TESSERACTS_POWER_WILL_BE_OURS.wav", 0x0001e06e, 0x75624f35),
				new KnownFile("VO_TO_THAT_WAS_ALL_LUCK_HUMAN.wav", 0x0001602a, 0x5df61b63),
				new KnownFile("VO_TO_THATS_IMPOSSIBLE_NO_HUMAN_CAN_CONTROL_THE_TES.wav", 0x00020cd0, 0xe6ccd122),
				new KnownFile("VO_TO_THE_HUMANS_HAVE_ACTIVATED_THE_TESSERACT.wav", 0x00011d5a, 0x5b16926c),
				new KnownFile("VO_TO_THE_HUMANS_WILL_NEVER_CONTROL_THE_TESSERACTS_.wav", 0x0001a65e, 0xa9a546b9),
				new KnownFile("VO_TO_THE_INVASION_WILL_PROCEED_MY_LORD.wav", 0x00018fb6, 0xe2059dbf),
				new KnownFile("VO_TO_THE_TESSERACT_2.wav", 0x0000a2ec, 0x7a866b20),
				new KnownFile("VO_TO_THE_TESSERACT_IS_ACTIVE.wav", 0x0000ee38, 0xf72b8459),
				new KnownFile("VO_TO_THIS_DOES_NOT_BODE_WELL.wav", 0x00011a24, 0x894d62aa),
				new KnownFile("VO_TO_WE_MUST_STRIKE_NOW_BEFORE_THE_HUMANS_HARNESS_.wav", 0x0001ef06, 0x79778c36),
				new KnownFile("VO_TO_WE_WILL_NOT_ALLOW_YOU_TO_CONTROL_ITS_POWER.wav", 0x00019eb0, 0x0b04c774),
				new KnownFile("VO_TO_WHAT_HOW_DID_YOU_ACTIVATE_IT.wav", 0x0001c44a, 0x135a5019),
				new KnownFile("VO_TO_YOU_ARE_NOT_THE_WEAKLING_WE_EXPECTED.wav", 0x0001f2e6, 0x3628a2c3),
				new KnownFile("VO_TO_YOU_KNOW_NOTHING_OF_THE_TESSERACTS_TRUE_POWER.wav", 0x00018068, 0x3b90c007) } },
		};
	}

	// The disk image as a stream read front to back; a pass opens it anew. In a zip, the largest
	// entry is the image. The zip is read here rather than through .NET's ZipArchive, which refuses
	// some cards' zips: Supreme's lists offset and disk fields in its ZIP64 extra field that its
	// header does not call for. Only the fields the header marks 0xFFFFFFFF are taken from it
	public class ImageSource
	{
		readonly string m_path;
		readonly bool m_zipped;
		long m_dataAt;      // where the entry's data starts in the zip
		int m_method;       // 0 stored, 8 deflated
		readonly string m_7zip; // 7z.exe, for a .7z: it lists the archive and unpacks the largest entry to a pipe, a pass at a time
		public long Length;
		public string EntryName;

		static uint U32(byte[] d, int o) { return BitConverter.ToUInt32(d, o); }
		static ushort U16(byte[] d, int o) { return BitConverter.ToUInt16(d, o); }
		static byte[] ReadAt(Stream s, long at, int count)
		{
			var b = new byte[count];
			s.Seek(at, SeekOrigin.Begin);
			for (int o = 0; o < count; ) { int n = s.Read(b, o, count - o); if (n <= 0) throw new EndOfStreamException("The zip ends early."); o += n; }
			return b;
		}

		public ImageSource(string path)
		{
			m_path = path;
			m_zipped = path.EndsWith(".zip", StringComparison.OrdinalIgnoreCase);
			if (path.EndsWith(".7z", StringComparison.OrdinalIgnoreCase))
			{
				m_7zip = Find7Zip();
				if (m_7zip == null) throw new Exception("A .7z needs 7-Zip to read it, and it is not installed: install it from www.7-zip.org, or unpack the .7z and choose the card dump inside it.");
				List7z();
				return;
			}
			if (!m_zipped) { Length = new FileInfo(path).Length; EntryName = Path.GetFileName(path); return; }
			using (var s = File.OpenRead(path))
			{
				// the end of central directory record, and the ZIP64 one where its fields are full
				int tail = (int)Math.Min(s.Length, 22 + 65535);
				byte[] t = ReadAt(s, s.Length - tail, tail);
				int e = -1;
				for (int i = tail - 22; i >= 0 && e < 0; i--) if (U32(t, i) == 0x06054b50) e = i;
				if (e < 0) throw new Exception("This is not a zip file.");
				long count = U16(t, e + 10), dirAt = U32(t, e + 16);
				if ((count == 0xffff || dirAt == 0xffffffff) && e >= 20 && U32(t, e - 20) == 0x07064b50)
				{
					byte[] z = ReadAt(s, (long)BitConverter.ToUInt64(t, e - 20 + 8), 56);
					count = (long)BitConverter.ToUInt64(z, 32);
					dirAt = (long)BitConverter.ToUInt64(z, 48);
				}
				long bestSize = -1, bestHeader = 0;
				for (long k = 0, at = dirAt; k < count; k++)
				{
					byte[] head = ReadAt(s, at, 46);
					if (U32(head, 0) != 0x02014b50) throw new Exception("The zip's directory is damaged.");
					int nameLen = U16(head, 28), extraLen = U16(head, 30), commentLen = U16(head, 32);
					byte[] rest = ReadAt(s, at + 46, nameLen + extraLen);
					long size = U32(head, 24), packed = U32(head, 20), header = U32(head, 42);
					for (int x = nameLen; x + 4 <= rest.Length; x += 4 + U16(rest, x + 2))
					{
						if (U16(rest, x) != 1) continue; // ZIP64: the full fields, in order, for those the header marks
						int f = x + 4;
						if (size == 0xffffffff) { size = (long)BitConverter.ToUInt64(rest, f); f += 8; }
						if (packed == 0xffffffff) { packed = (long)BitConverter.ToUInt64(rest, f); f += 8; }
						if (header == 0xffffffff) { header = (long)BitConverter.ToUInt64(rest, f); }
					}
					if (size > bestSize) { bestSize = size; bestHeader = header; EntryName = Encoding.UTF8.GetString(rest, 0, nameLen); m_method = U16(head, 10); }
					at += 46 + nameLen + extraLen + commentLen;
				}
				if (bestSize < 0) throw new Exception("The zip file is empty.");
				if (m_method != 0 && m_method != 8) throw new Exception(EntryName + " is compressed in a way this builder does not read (method " + m_method + ").");
				byte[] local = ReadAt(s, bestHeader, 30);
				if (U32(local, 0) != 0x04034b50) throw new Exception("The zip's entry header is damaged.");
				m_dataAt = bestHeader + 30 + U16(local, 26) + U16(local, 28);
				Length = bestSize;
			}
		}

		// 7-Zip's 7z.exe: where its installer records it, in Program Files, or on the PATH
		static string Find7Zip()
		{
			var dirs = new List<string>();
			foreach (var view in new[] { Microsoft.Win32.RegistryView.Registry64, Microsoft.Win32.RegistryView.Registry32 })
				try
				{
					using (var hklm = Microsoft.Win32.RegistryKey.OpenBaseKey(Microsoft.Win32.RegistryHive.LocalMachine, view))
					using (var key = hklm.OpenSubKey(@"SOFTWARE\7-Zip"))
						if (key != null) foreach (var value in new[] { "Path64", "Path" }) { var d = key.GetValue(value) as string; if (!string.IsNullOrEmpty(d)) dirs.Add(d); }
				}
				catch (Exception) { }
			foreach (var env in new[] { "ProgramFiles", "ProgramW6432", "ProgramFiles(x86)" })
			{
				var d = Environment.GetEnvironmentVariable(env);
				if (!string.IsNullOrEmpty(d)) dirs.Add(Path.Combine(d, "7-Zip"));
			}
			dirs.AddRange((Environment.GetEnvironmentVariable("PATH") ?? "").Split(';'));
			foreach (var d in dirs)
				try { var exe = Path.Combine(d.Trim().Trim('"'), "7z.exe"); if (File.Exists(exe)) return exe; }
				catch (ArgumentException) { }
			return null;
		}

		Process Start7z(string arguments)
		{
			var info = new ProcessStartInfo(m_7zip, arguments) { UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true, RedirectStandardError = true };
			var p = Process.Start(info);
			p.ErrorDataReceived += delegate { }; // drained, so a full pipe never stalls it
			p.BeginErrorReadLine();
			return p;
		}

		// The archive's largest entry is the card dump: 7z l -slt lists each entry's Path, Size and Folder
		void List7z()
		{
			string name = null, best = null;
			long size = -1, bestSize = -1;
			bool folder = false, entries = false;
			using (var p = Start7z("l -slt -- \"" + m_path + "\""))
			{
				Action entryDone = delegate { if (name != null && !folder && size > bestSize) { bestSize = size; best = name; } name = null; size = -1; folder = false; };
				for (string line; (line = p.StandardOutput.ReadLine()) != null; )
				{
					if (line.StartsWith("----------")) { entries = true; continue; }
					if (!entries) continue;
					if (line.Length == 0) { entryDone(); continue; }
					if (line.StartsWith("Path = ")) name = line.Substring(7);
					else if (line.StartsWith("Size = ")) long.TryParse(line.Substring(7), out size);
					else if (line.StartsWith("Folder = ")) folder = line.Substring(9) == "+";
				}
				entryDone();
				p.WaitForExit();
				if (p.ExitCode != 0) throw new Exception("7-Zip cannot read " + Path.GetFileName(m_path) + " (exit code " + p.ExitCode + ").");
			}
			if (best == null) throw new Exception(Path.GetFileName(m_path) + " holds no file.");
			EntryName = best;
			Length = bestSize;
		}

		// Ends 7-Zip's unpacking when a pass has read what it needs
		class ProcessOwner : IDisposable
		{
			readonly Process m_process;
			public ProcessOwner(Process p) { m_process = p; }
			public void Dispose()
			{
				try { if (!m_process.HasExited) m_process.Kill(); m_process.WaitForExit(5000); } catch (Exception) { }
				m_process.Dispose();
			}
		}

		public ForwardReader Open()
		{
			if (m_7zip != null)
			{
				var p = Start7z("e -so -bd -- \"" + m_path + "\" \"" + EntryName + "\"");
				return new ForwardReader(p.StandardOutput.BaseStream, new ProcessOwner(p), false);
			}
			if (!m_zipped) return new ForwardReader(File.OpenRead(m_path), null, true);
			var file = File.OpenRead(m_path);
			file.Seek(m_dataAt, SeekOrigin.Begin);
			if (m_method == 0) return new ForwardReader(file, null, false);
			return new ForwardReader(new DeflateStream(file, CompressionMode.Decompress), file, false);
		}
	}

	public class ForwardReader : IDisposable
	{
		readonly Stream m_stream;
		readonly IDisposable m_owner;
		readonly bool m_seekable;
		readonly byte[] m_scratch = new byte[1 << 20];
		public long Position;

		public ForwardReader(Stream s, IDisposable owner, bool seekable) { m_stream = s; m_owner = owner; m_seekable = seekable; }
		public void Dispose() { m_stream.Dispose(); if (m_owner != null) m_owner.Dispose(); }

		public void SkipTo(long offset)
		{
			if (offset < Position) throw new InvalidOperationException("ForwardReader cannot go back");
			if (m_seekable) { m_stream.Seek(offset, SeekOrigin.Begin); Position = offset; return; }
			while (Position < offset)
			{
				int n = m_stream.Read(m_scratch, 0, (int)Math.Min(m_scratch.Length, offset - Position));
				if (n <= 0) throw new EndOfStreamException("The image ends early");
				Position += n;
			}
		}

		public void ReadExactly(byte[] buffer, int offset, int count)
		{
			while (count > 0)
			{
				int n = m_stream.Read(buffer, offset, count);
				if (n <= 0) throw new EndOfStreamException("The image ends early");
				offset += n; count -= n; Position += n;
			}
		}

		public byte[] ReadAt(long offset, int count) { SkipTo(offset); var b = new byte[count]; ReadExactly(b, 0, count); return b; }
	}

	// A file of the game folder: on an image, its ext3 blocks; in a .spk, one stored run at Source
	public class FoundFile { public string Name; public long Size; public uint[] Blocks; public long Source = -1; public long DataOffset; public uint Crc; }

	public class Builder
	{
		public Action<string> Log = delegate { };
		public Action<string, double> Progress = delegate { };
		public volatile bool Cancel;

		class Request { public long Offset; public int Length; public Action<byte[]> Done; }
		class Partition { public long Start; public int BlockSize; public uint FirstDataBlock, InodesPerGroup, BlocksPerGroup, Groups; public int InodeSize; public bool FileType; public byte[] Gdt; }

		readonly List<Request> m_pending = new List<Request>();
		readonly List<Partition> m_partitions = new List<Partition>();
		ImageSource m_image;
		Partition m_gamePart;
		string m_folder;
		readonly List<FoundFile> m_files = new List<FoundFile>();

		static uint U32(byte[] d, int o) { return BitConverter.ToUInt32(d, o); }
		static ushort U16(byte[] d, int o) { return BitConverter.ToUInt16(d, o); }

		void Want(long offset, int length, Action<byte[]> done) { m_pending.Add(new Request { Offset = offset, Length = length, Done = done }); }

		// Runs the queued reads, as many forward passes as their order needs. The image is read in
		// aligned windows, which stay cached: a read that lands in one already seen (the next inode
		// in the same table block, say) is served at once instead of waiting for another pass
		const int Window = 1 << 16;
		readonly Dictionary<long, byte[]> m_windows = new Dictionary<long, byte[]>();

		bool TryServe(Request q)
		{
			long first = q.Offset / Window, last = (q.Offset + q.Length - 1) / Window;
			for (long w = first; w <= last; w++) if (!m_windows.ContainsKey(w)) return false;
			var d = new byte[q.Length];
			for (int done = 0; done < q.Length; )
			{
				long at = q.Offset + done;
				byte[] win = m_windows[at / Window];
				int from = (int)(at % Window), n = Math.Min(q.Length - done, win.Length - from);
				if (n <= 0) return false; // past the end of the image
				Array.Copy(win, from, d, done, n);
				done += n;
			}
			q.Done(d);
			return true;
		}

		void RunMetadataPasses()
		{
			for (int pass = 1; m_pending.Count > 0; pass++)
			{
				if (pass > 12) throw new Exception("The image's file system needs too many passes to read");
				Progress("Reading the image's file system (pass " + pass + ")", -1);
				using (var r = m_image.Open())
				{
					while (true)
					{
						if (Cancel) throw new OperationCanceledException();
						for (bool served = true; served; )
						{
							served = false;
							foreach (var q in m_pending.ToArray())
								if (TryServe(q)) { m_pending.Remove(q); served = true; }
						}
						m_pending.Sort(delegate(Request a, Request b) { return a.Offset.CompareTo(b.Offset); });
						int i = m_pending.FindIndex(delegate(Request q) { return q.Offset >= r.Position; });
						if (i < 0) break;
						long first = m_pending[i].Offset / Window, last = (m_pending[i].Offset + m_pending[i].Length - 1) / Window;
						for (long w = first; w <= last; w++)
						{
							if (m_windows.ContainsKey(w)) continue;
							r.SkipTo(w * Window);
							int n = (int)Math.Min(Window, m_image.Length - w * Window);
							var win = new byte[n];
							r.ReadExactly(win, 0, n);
							m_windows[w] = win;
						}
					}
				}
			}
			m_windows.Clear();
		}

		// ---------------------------------------------------------------- partitions
		void ReadMbr(byte[] mbr)
		{
			if (mbr[510] != 0x55 || mbr[511] != 0xAA) throw new Exception("This is not a Spike 1 SD-card image (no partition table).");
			for (int i = 0; i < 4; i++)
			{
				int e = 446 + 16 * i;
				byte type = mbr[e + 4];
				long lba = U32(mbr, e + 8);
				if (type == 0x83) AddLinux(lba);
				else if (type == 0x05 || type == 0x0F) ReadEbr(lba, lba);
			}
		}

		void ReadEbr(long extStart, long ebr)
		{
			Want(ebr * 512, 512, delegate(byte[] d) {
				if (d[510] != 0x55 || d[511] != 0xAA) return;
				if (d[446 + 4] == 0x83) AddLinux(ebr + U32(d, 446 + 8));
				byte next = d[462 + 4];
				if (next == 0x05 || next == 0x0F) ReadEbr(extStart, extStart + U32(d, 462 + 8));
			});
		}

		void AddLinux(long lba)
		{
			long start = lba * 512;
			Want(start + 1024, 1024, delegate(byte[] sb) {
				if (U16(sb, 56) != 0xEF53) return;
				uint incompat = U32(sb, 96);
				if ((incompat & ~0x6u) != 0) { Log("A partition uses ext4 features this script does not read; skipped."); return; }
				var p = new Partition();
				p.Start = start;
				p.BlockSize = 1024 << (int)U32(sb, 24);
				p.FirstDataBlock = U32(sb, 20);
				p.BlocksPerGroup = U32(sb, 32);
				p.InodesPerGroup = U32(sb, 40);
				p.InodeSize = U32(sb, 76) >= 1 ? U16(sb, 88) : 128;
				p.FileType = (incompat & 0x2) != 0;
				p.Groups = (U32(sb, 4) - p.FirstDataBlock + p.BlocksPerGroup - 1) / p.BlocksPerGroup;
				m_partitions.Add(p);
				int gdtBytes = (int)(p.Groups * 32);
				Want(start + (long)(p.FirstDataBlock + 1) * p.BlockSize, (gdtBytes + p.BlockSize - 1) / p.BlockSize * p.BlockSize, delegate(byte[] gdt) {
					p.Gdt = gdt;
					ReadInode(p, 2, delegate(byte[] root) { ReadDir(p, root, delegate(List<KeyValuePair<string, uint>> entries, List<byte> types) { RootListed(p, entries, types); }); });
				});
			});
		}

		// ---------------------------------------------------------------- ext2/ext3
		void ReadInode(Partition p, uint n, Action<byte[]> done)
		{
			uint g = (n - 1) / p.InodesPerGroup, i = (n - 1) % p.InodesPerGroup;
			long table = U32(p.Gdt, (int)g * 32 + 8);
			Want(p.Start + table * p.BlockSize + (long)i * p.InodeSize, p.InodeSize, done);
		}

		static long SizeOf(byte[] inode) { long s = U32(inode, 4); if ((U16(inode, 0) & 0xF000) == 0x8000) s |= (long)U32(inode, 108) << 32; return s; }

		// The physical blocks of a file, in file order, gathered through its indirect blocks
		void MapBlocks(Partition p, byte[] inode, Action<uint[]> done)
		{
			long size = SizeOf(inode);
			long count = (size + p.BlockSize - 1) / p.BlockSize;
			var blocks = new uint[count];
			int per = p.BlockSize / 4;
			int outstanding = 1;
			Action finish = delegate { if (--outstanding == 0) done(blocks); };
			Action<uint, long, int> indirect = null;
			indirect = delegate(uint block, long first, int depth) {
				if (block == 0 || first >= count) return;
				outstanding++;
				Want(p.Start + (long)block * p.BlockSize, p.BlockSize, delegate(byte[] d) {
					long span = 1; for (int k = 1; k < depth; k++) span *= per;
					for (int k = 0; k < per; k++)
					{
						long at = first + k * span;
						if (at >= count) break;
						uint b = U32(d, 4 * k);
						if (depth == 1) blocks[at] = b; else indirect(b, at, depth - 1);
					}
					finish();
				});
			};
			for (int k = 0; k < 12 && k < count; k++) blocks[k] = U32(inode, 40 + 4 * k);
			indirect(U32(inode, 40 + 48), 12, 1);
			indirect(U32(inode, 40 + 52), 12 + per, 2);
			indirect(U32(inode, 40 + 56), 12 + per + (long)per * per, 3);
			finish();
		}

		void ReadDir(Partition p, byte[] inode, Action<List<KeyValuePair<string, uint>>, List<byte>> done)
		{
			MapBlocks(p, inode, delegate(uint[] blocks) {
				var entries = new List<KeyValuePair<string, uint>>();
				var types = new List<byte>();
				int left = blocks.Length;
				if (left == 0) { done(entries, types); return; }
				var parts = new byte[blocks.Length][];
				for (int b = 0; b < blocks.Length; b++)
				{
					int index = b;
					Want(p.Start + (long)blocks[b] * p.BlockSize, p.BlockSize, delegate(byte[] d) {
						parts[index] = d;
						if (--left > 0) return;
						foreach (var block in parts)
							for (int o = 0; o + 8 <= block.Length; )
							{
								uint child = U32(block, o);
								int rec = U16(block, o + 4);
								int nl = p.FileType ? block[o + 6] : U16(block, o + 6);
								if (rec < 8) break;
								if (child != 0 && nl > 0 && o + 8 + nl <= block.Length)
								{
									entries.Add(new KeyValuePair<string, uint>(Encoding.UTF8.GetString(block, o + 8, nl), child));
									types.Add(p.FileType ? block[o + 7] : (byte)0);
								}
								o += rec;
							}
						done(entries, types);
					});
				}
			});
		}

		// A regular file of the game folder, mapped to its blocks
		void AddFile(Partition p, string fname, uint inode)
		{
			ReadInode(p, inode, delegate(byte[] fi) {
				if ((U16(fi, 0) & 0xF000) != 0x8000) return;
				var found = new FoundFile { Name = fname, Size = SizeOf(fi) };
				m_files.Add(found);
				MapBlocks(p, fi, delegate(uint[] blocks) { found.Blocks = blocks; });
			});
		}

		// The entry so named in a directory, by its inode
		void Lookup(Partition p, uint dir, string name, Action<uint> found)
		{
			ReadInode(p, dir, delegate(byte[] ino) {
				if ((U16(ino, 0) & 0xF000) != 0x4000) return;
				ReadDir(p, ino, delegate(List<KeyValuePair<string, uint>> entries, List<byte> types) {
					foreach (var e in entries) if (e.Key == name) { found(e.Value); return; }
				});
			});
		}

		// An ELG title's game folder (Transformers, Avengers The Pin): /usr/local/games/<name>-elg on
		// the root file system, holding the programs and a sounds directory, whose WAV files the set
		// holds under their own names
		void ElgGames(Partition p, uint games)
		{
			ReadInode(p, games, delegate(byte[] gi) {
				if ((U16(gi, 0) & 0xF000) != 0x4000) return;
				ReadDir(p, gi, delegate(List<KeyValuePair<string, uint>> folders, List<byte> ftypes) {
					for (int k = 0; k < folders.Count; k++)
					{
						string name = folders[k].Key;
						if (!name.EndsWith("-elg") || (ftypes[k] != 0 && ftypes[k] != 2)) continue;
						ReadInode(p, folders[k].Value, delegate(byte[] fi) {
							if ((U16(fi, 0) & 0xF000) != 0x4000) return;
							ReadDir(p, fi, delegate(List<KeyValuePair<string, uint>> files, List<byte> types) {
								int sounds = files.FindIndex(delegate(KeyValuePair<string, uint> f) { return f.Key == "sounds"; });
								if (sounds < 0 || m_folder != null) return;
								m_folder = name;
								m_gamePart = p;
								Log("Game folder: /usr/local/games/" + name + " (" + files.Count + " entries)");
								for (int i = 0; i < files.Count; i++)
									if (files[i].Key != "." && files[i].Key != ".." && i != sounds && (types[i] == 0 || types[i] == 1)) AddFile(p, files[i].Key, files[i].Value);
								ReadInode(p, files[sounds].Value, delegate(byte[] si) {
									if ((U16(si, 0) & 0xF000) != 0x4000) return;
									ReadDir(p, si, delegate(List<KeyValuePair<string, uint>> wavs, List<byte> wtypes) {
										for (int w = 0; w < wavs.Count; w++)
											if (wavs[w].Key != "." && wavs[w].Key != ".." && (wtypes[w] == 0 || wtypes[w] == 1)) AddFile(p, wavs[w].Key, wavs[w].Value);
									});
								});
							});
						});
					}
				});
			});
		}

		// The game folder is the partition's directory that holds "game" and "image.bin"
		void RootListed(Partition p, List<KeyValuePair<string, uint>> entries, List<byte> types)
		{
			for (int i = 0; i < entries.Count; i++)
			{
				string name = entries[i].Key;
				if (name == "." || name == ".." || name == "lost+found" || (types[i] != 0 && types[i] != 2)) continue;
				uint child = entries[i].Value;
				if (name == "usr") Lookup(p, child, "local", delegate(uint local) { Lookup(p, local, "games", delegate(uint games) { ElgGames(p, games); }); });
				ReadInode(p, child, delegate(byte[] ino) {
					if ((U16(ino, 0) & 0xF000) != 0x4000) return;
					ReadDir(p, ino, delegate(List<KeyValuePair<string, uint>> files, List<byte> ftypes) {
						bool hasGame = false, hasImage = false;
						foreach (var f in files) { if (f.Key == "game") hasGame = true; if (f.Key == "image.bin") hasImage = true; }
						if (!hasGame || !hasImage || m_folder != null) return;
						m_folder = name;
						m_gamePart = p;
						Log("Game folder: /" + name + " (" + files.Count + " entries)");
						for (int k = 0; k < files.Count; k++)
						{
							if (files[k].Key == "." || files[k].Key == "..") continue;
							if (files[k].Key == "video" && (ftypes[k] == 0 || ftypes[k] == 2))
							{
								// WWE's LCD clips: the files of the folder's video directory, under their own names
								ReadInode(p, files[k].Value, delegate(byte[] vi) {
									if ((U16(vi, 0) & 0xF000) != 0x4000) return;
									ReadDir(p, vi, delegate(List<KeyValuePair<string, uint>> clips, List<byte> ctypes) {
										for (int c = 0; c < clips.Count; c++)
											if (clips[c].Key != "." && clips[c].Key != ".." && (ctypes[c] == 0 || ctypes[c] == 1)) AddFile(p, clips[c].Key, clips[c].Value);
									});
								});
								continue;
							}
							if (ftypes[k] != 0 && ftypes[k] != 1) continue;
							AddFile(p, files[k].Key, files[k].Value);
						}
					});
				});
			}
		}

		// ---------------------------------------------------------------- a .spk package
		// 'SPKS', a CRC and a version, then groups, each 'SPK0' and its size: an index ('SIDX' and its
		// size: the group's name in 16 bytes, at 0x28 the file count, at 0x30 'STRS', its size and
		// the path table, then per file 'FINF', the record's size, and the path's offset, the size,
		// the data offset, the stored size and the mode), then 'SDAT', a size and the files' bytes.
		// The game's group is the one holding <name>/game and <name>/image.bin
		static void ReadFull(Stream s, byte[] b) { for (int o = 0; o < b.Length; ) { int n = s.Read(b, o, b.Length - o); if (n <= 0) throw new EndOfStreamException("The package ends early"); o += n; } }

		void ReadSpk(string path)
		{
			using (var s = File.OpenRead(path))
			{
				var head = new byte[12];
				ReadFull(s, head);
				if (Encoding.ASCII.GetString(head, 0, 4) != "SPKS") throw new Exception("This is not a Stern .spk package.");
				for (long pos = 12; pos + 16 <= s.Length && m_folder == null; )
				{
					s.Seek(pos, SeekOrigin.Begin);
					var g = new byte[16];
					ReadFull(s, g);
					if (Encoding.ASCII.GetString(g, 0, 4) != "SPK0" || Encoding.ASCII.GetString(g, 8, 4) != "SIDX") break;
					long start = pos + 8, next = start + U32(g, 4);
					var sidx = new byte[U32(g, 12)];
					ReadFull(s, sidx);
					string name = Encoding.ASCII.GetString(sidx, 0, 16).Split('\0')[0];
					if (Encoding.ASCII.GetString(sidx, 0x30, 4) != "STRS") throw new Exception("The package's index is not as expected.");
					int count = (int)U32(sidx, 0x28), strsAt = 0x38, strsSize = (int)U32(sidx, 0x34);
					long data = start + 8 + sidx.Length + 8; // past 'SDAT' and its size
					var files = new List<FoundFile>();
					bool hasGame = false, hasImage = false;
					for (int i = 0, fo = strsAt + strsSize; i < count; i++)
					{
						if (Encoding.ASCII.GetString(sidx, fo, 4) != "FINF") throw new Exception("The package's index is not as expected.");
						int p = fo + 8, nameAt = strsAt + (int)U32(sidx, p);
						long size = U32(sidx, p + 4), offset = U32(sidx, p + 8), stored = U32(sidx, p + 12);
						int nameEnd = Array.IndexOf(sidx, (byte)0, nameAt);
						string file = Encoding.UTF8.GetString(sidx, nameAt, nameEnd - nameAt);
						fo += 8 + (int)U32(sidx, fo + 4);
						if (!file.StartsWith(name + "/")) continue;
						string rel = file.Substring(name.Length + 1);
						if (rel == "game") hasGame = true;
						if (rel == "image.bin") hasImage = true;
						if (rel.StartsWith("video/")) rel = rel.Substring(6); // LCD clips, under their own names
						if (rel.Contains("/")) continue;
						if (stored != size) throw new Exception(rel + " is compressed in the package, which this builder does not read.");
						files.Add(new FoundFile { Name = rel, Size = size, Source = data + offset });
					}
					if (hasGame && hasImage)
					{
						m_folder = name;
						m_files.AddRange(files);
						Log("Game folder: /" + name + " (" + files.Count + " files)");
					}
					if (next <= pos) break;
					pos = next;
				}
			}
		}

		// ---------------------------------------------------------------- the zip
		static readonly uint[] CrcTable = MakeCrcTable();
		static uint[] MakeCrcTable()
		{
			var t = new uint[256];
			for (uint n = 0; n < 256; n++) { uint c = n; for (int k = 0; k < 8; k++) c = (c & 1) != 0 ? 0xEDB88320u ^ (c >> 1) : c >> 1; t[n] = c; }
			return t;
		}
		static uint Crc(uint crc, byte[] b, int o, int n) { crc = ~crc; for (int i = 0; i < n; i++) crc = CrcTable[(crc ^ b[o + i]) & 0xFF] ^ (crc >> 8); return ~crc; }

		static void PutU16(byte[] b, int o, int v) { b[o] = (byte)v; b[o + 1] = (byte)(v >> 8); }
		static void PutU32(byte[] b, int o, uint v) { b[o] = (byte)v; b[o + 1] = (byte)(v >> 8); b[o + 2] = (byte)(v >> 16); b[o + 3] = (byte)(v >> 24); }

		static byte[] LocalHeader(FoundFile f, byte[] name)
		{
			var h = new byte[30 + name.Length];
			PutU32(h, 0, 0x04034b50); PutU16(h, 4, 10); PutU16(h, 6, 0); PutU16(h, 8, 0);
			PutU16(h, 10, 0); PutU16(h, 12, 0x21); // 1980-01-01 00:00
			PutU32(h, 14, f.Crc); PutU32(h, 18, (uint)f.Size); PutU32(h, 22, (uint)f.Size);
			PutU16(h, 26, name.Length); PutU16(h, 28, 0);
			Array.Copy(name, 0, h, 30, name.Length);
			return h;
		}

		// ---------------------------------------------------------------- the whole job
		public string Build(string imagePath, string outDir)
		{
			m_image = new ImageSource(imagePath);
			Log("Image: " + Path.GetFileName(imagePath) + (m_image.EntryName != Path.GetFileName(imagePath) ? " (" + m_image.EntryName + ", " : " (") + (m_image.Length / 1048576) + " MB)");
			bool package = imagePath.EndsWith(".spk", StringComparison.OrdinalIgnoreCase);
			if (package) ReadSpk(imagePath);
			else { Want(0, 512, ReadMbr); RunMetadataPasses(); }
			if (m_folder == null) throw new Exception(package ? "No game in the package. Is this a Stern Spike 1 .spk?" : "No game folder found in the image. Is this a Stern Spike 1 SD-card image?");
			foreach (var f in m_files) if (f.Blocks == null && f.Source < 0) throw new Exception("Could not map " + f.Name);

			// Which set: the known set from this folder whose file sizes match
			KnownSet set = null;
			foreach (var k in KnownSet.All)
			{
				if (k.Folder != m_folder) continue;
				bool all = true;
				foreach (var kf in k.Files) { var f = m_files.Find(delegate(FoundFile x) { return x.Name == kf.Name; }); if (f == null || f.Size != kf.Size) { all = false; break; } }
				if (all) { set = k; break; }
			}
			string setName = set != null ? set.Set : m_folder;
			var files = new List<FoundFile>();
			if (set != null) foreach (var kf in set.Files) files.Add(m_files.Find(delegate(FoundFile x) { return x.Name == kf.Name; }));
			else { files.AddRange(m_files); files.Sort(delegate(FoundFile a, FoundFile b) { return string.CompareOrdinal(a.Name, b.Name); }); }
			Log(set != null ? "Set: " + set.Set + " - " + set.Title : "This code version has no PinMAME set yet; the zip is named after its folder.");

			// Lay the zip out, then copy every file's blocks to their place in one pass
			Directory.CreateDirectory(outDir);
			string target = Path.Combine(outDir, setName + ".zip");
			string partial = target + ".partial";
			long pos = 0;
			foreach (var f in files) { f.DataOffset = pos + 30 + Encoding.UTF8.GetByteCount(f.Name); pos = f.DataOffset + f.Size; }
			long directoryAt = pos;
			if (directoryAt > uint.MaxValue) throw new Exception("The set is too large for a plain zip");

			var runs = new List<long[]>(); // phys offset, length, destination
			foreach (var f in files)
			{
				if (f.Source >= 0) { if (f.Size > 0) runs.Add(new long[] { f.Source, f.Size, f.DataOffset }); continue; }
				int bs = m_gamePart.BlockSize;
				for (long i = 0; i < f.Blocks.Length; )
				{
					long j = i + 1;
					if (f.Blocks[i] == 0) { i = j; continue; } // a hole reads as zeros
					while (j < f.Blocks.Length && f.Blocks[j] == f.Blocks[j - 1] + 1) j++;
					long len = Math.Min((j - i) * bs, f.Size - i * bs);
					runs.Add(new long[] { m_gamePart.Start + (long)f.Blocks[i] * bs, len, f.DataOffset + i * bs });
					i = j;
				}
			}
			runs.Sort(delegate(long[] a, long[] b) { return a[0].CompareTo(b[0]); });
			long total = 0; foreach (var r in runs) total += r[1];

			using (var output = new FileStream(partial, FileMode.Create, FileAccess.ReadWrite, FileShare.None, 1 << 20))
			{
				output.SetLength(directoryAt);
				long copied = 0;
				var buf = new byte[1 << 20];
				using (var reader = m_image.Open())
					foreach (var r in runs)
					{
						reader.SkipTo(r[0]);
						output.Seek(r[2], SeekOrigin.Begin);
						for (long left = r[1]; left > 0; )
						{
							if (Cancel) throw new OperationCanceledException();
							int n = (int)Math.Min(buf.Length, left);
							reader.ReadExactly(buf, 0, n);
							output.Write(buf, 0, n);
							left -= n; copied += n;
							Progress("Copying the game files", (double)copied / total);
						}
					}

				// CRCs from what landed in the zip, checked against the set
				bool ok = true;
				long checkedBytes = 0;
				foreach (var f in files)
				{
					output.Seek(f.DataOffset, SeekOrigin.Begin);
					uint crc = 0;
					for (long left = f.Size; left > 0; )
					{
						if (Cancel) throw new OperationCanceledException();
						int n = output.Read(buf, 0, (int)Math.Min(buf.Length, left));
						if (n <= 0) throw new EndOfStreamException();
						crc = Crc(crc, buf, 0, n);
						left -= n; checkedBytes += n;
						Progress("Checking the files", (double)checkedBytes / total);
					}
					f.Crc = crc;
					if (set != null)
					{
						var kf = Array.Find(set.Files, delegate(KnownFile x) { return x.Name == f.Name; });
						if (kf.Crc != crc) { ok = false; Log("CRC MISMATCH: " + f.Name + " is " + crc.ToString("x8") + ", the set needs " + kf.Crc.ToString("x8")); }
					}
				}

				// Headers and the central directory
				var central = new MemoryStream();
				foreach (var f in files)
				{
					var name = Encoding.UTF8.GetBytes(f.Name);
					var lh = LocalHeader(f, name);
					long headerAt = f.DataOffset - lh.Length;
					output.Seek(headerAt, SeekOrigin.Begin);
					output.Write(lh, 0, lh.Length);
					var ch = new byte[46 + name.Length];
					PutU32(ch, 0, 0x02014b50); PutU16(ch, 4, 20); PutU16(ch, 6, 10);
					Array.Copy(lh, 6, ch, 8, 26 - 6 + 2); // flags .. name length: same fields as the local header
					PutU16(ch, 30, 0); PutU16(ch, 32, 0); PutU16(ch, 34, 0); PutU16(ch, 36, 0); PutU32(ch, 38, 0);
					PutU32(ch, 42, (uint)headerAt);
					Array.Copy(name, 0, ch, 46, name.Length);
					central.Write(ch, 0, ch.Length);
				}
				var end = new byte[22];
				PutU32(end, 0, 0x06054b50); PutU16(end, 8, files.Count); PutU16(end, 10, files.Count);
				PutU32(end, 12, (uint)central.Length); PutU32(end, 16, (uint)directoryAt);
				output.Seek(directoryAt, SeekOrigin.Begin);
				central.WriteTo(output);
				output.Write(end, 0, end.Length);
				if (!ok) throw new Exception("The files do not match the set. Is the download complete?");
			}
			if (File.Exists(target)) File.Delete(target);
			File.Move(partial, target);
			Log("Done: " + target + " (" + (new FileInfo(target).Length / 1048576) + " MB, " + files.Count + " files" + (set != null ? ", all CRCs match" : "") + ")");
			if (set != null && set.VideoKey)
				Log("The LCD videos also need their key: put it (32 hex digits) in " + set.Set + ".txt beside the zip.");
			Progress("Done", 1);
			return target;
		}
	}

	// ---------------------------------------------------------------- the window
	public class App : Form
	{
		readonly TextBox m_image = new TextBox(), m_out = new TextBox(), m_log = new TextBox();
		readonly ProgressBar m_bar = new ProgressBar();
		readonly Label m_stage = new Label();
		readonly Button m_build = new Button(), m_close = new Button();
		Builder m_builder;

		public App(string image, string outDir)
		{
			Text = "Spike 1 ROM set builder";
			Font = new Font("Segoe UI", 9.5f);
			ClientSize = new Size(640, 400);
			MinimumSize = new Size(520, 360);
			StartPosition = FormStartPosition.CenterScreen;

			var table = new TableLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(12), ColumnCount = 3, RowCount = 7 };
			table.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
			table.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
			table.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
			for (int i = 0; i < 5; i++) table.RowStyles.Add(new RowStyle(SizeType.AutoSize));
			table.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
			table.RowStyles.Add(new RowStyle(SizeType.AutoSize));

			var intro = new Label { Text = "Builds the PinMAME ROM set from Stern's SD-card image (.iso.zip, .iso), a card dump (.bin, .img, or a .7z with 7-Zip installed) or update package (.spk). Nothing in it is changed.", AutoSize = true, MaximumSize = new Size(600, 0), Margin = new Padding(0, 0, 0, 10) };
			table.Controls.Add(intro, 0, 0); table.SetColumnSpan(intro, 3);

			table.Controls.Add(new Label { Text = "Stern image", AutoSize = true, Anchor = AnchorStyles.Left }, 0, 1);
			m_image.Dock = DockStyle.Fill; m_image.Text = image ?? "";
			table.Controls.Add(m_image, 1, 1);
			var pickImage = new Button { Text = "Browse...", AutoSize = true };
			pickImage.Click += delegate {
				using (var d = new OpenFileDialog { Filter = "Stern download or card dump (*.iso.zip;*.iso;*.spk;*.bin;*.img;*.7z)|*.iso.zip;*.iso;*.spk;*.bin;*.img;*.7z|All files (*.*)|*.*", Title = "Stern image" })
					if (d.ShowDialog(this) == DialogResult.OK) m_image.Text = d.FileName;
			};
			table.Controls.Add(pickImage, 2, 1);

			table.Controls.Add(new Label { Text = "Save the set in", AutoSize = true, Anchor = AnchorStyles.Left }, 0, 2);
			m_out.Dock = DockStyle.Fill; m_out.Text = outDir ?? "";
			table.Controls.Add(m_out, 1, 2);
			var pickOut = new Button { Text = "Browse...", AutoSize = true };
			pickOut.Click += delegate {
				using (var d = new FolderBrowserDialog { Description = "Folder for the ROM set (PinMAME's roms folder)", SelectedPath = m_out.Text })
					if (d.ShowDialog(this) == DialogResult.OK) m_out.Text = d.SelectedPath;
			};
			table.Controls.Add(pickOut, 2, 2);

			m_stage.AutoSize = true; m_stage.Text = "Ready."; m_stage.Margin = new Padding(0, 10, 0, 2);
			table.Controls.Add(m_stage, 0, 3); table.SetColumnSpan(m_stage, 3);
			m_bar.Dock = DockStyle.Fill; m_bar.Maximum = 1000;
			table.Controls.Add(m_bar, 0, 4); table.SetColumnSpan(m_bar, 3);

			m_log.Multiline = true; m_log.ReadOnly = true; m_log.ScrollBars = ScrollBars.Vertical; m_log.Dock = DockStyle.Fill;
			m_log.Font = new Font("Consolas", 9f);
			table.Controls.Add(m_log, 0, 5); table.SetColumnSpan(m_log, 3);

			var buttons = new FlowLayoutPanel { FlowDirection = FlowDirection.RightToLeft, Dock = DockStyle.Fill, AutoSize = true };
			m_close.Text = "Close"; m_close.AutoSize = true; m_close.Click += delegate { if (m_builder != null) m_builder.Cancel = true; else Close(); };
			m_build.Text = "Build ROM set"; m_build.AutoSize = true; m_build.Click += delegate { Start(); };
			buttons.Controls.Add(m_close); buttons.Controls.Add(m_build);
			table.Controls.Add(buttons, 0, 6); table.SetColumnSpan(buttons, 3);
			Controls.Add(table);
			AcceptButton = m_build;
		}

		void Say(string line) { BeginInvoke((Action)delegate { m_log.AppendText(line + Environment.NewLine); }); }

		void Start()
		{
			string image = m_image.Text.Trim().Trim('"'), outDir = m_out.Text.Trim().Trim('"');
			if (!File.Exists(image)) { MessageBox.Show(this, "Choose Stern's .iso.zip, .iso, card dump (.bin, .img, .7z) or .spk file first.", Text); return; }
			if (outDir.Length == 0) { MessageBox.Show(this, "Choose a folder for the ROM set.", Text); return; }
			m_build.Enabled = false; m_close.Text = "Cancel"; m_log.Clear();
			m_builder = new Builder();
			m_builder.Log = Say;
			string lastStage = null; int lastValue = -1;
			m_builder.Progress = delegate(string stage, double f) {
				int v = f < 0 ? -1 : (int)(f * 1000);
				if (stage == lastStage && v == lastValue) return;
				lastStage = stage; lastValue = v;
				BeginInvoke((Action)delegate {
					m_stage.Text = stage + (f >= 0 && f < 1 ? string.Format(" ({0:0}%)", f * 100) : "");
					m_bar.Style = f < 0 ? ProgressBarStyle.Marquee : ProgressBarStyle.Continuous;
					if (f >= 0) m_bar.Value = Math.Max(0, Math.Min(1000, v));
				});
			};
			var builder = m_builder;
			var t = new Thread(delegate() {
				string error = null;
				try { builder.Build(image, outDir); }
				catch (OperationCanceledException) { error = "Cancelled."; }
				catch (Exception e) { error = e.Message; }
				BeginInvoke((Action)delegate {
					m_builder = null; m_build.Enabled = true; m_close.Text = "Close";
					if (error != null) { m_stage.Text = "Stopped."; m_bar.Style = ProgressBarStyle.Continuous; m_bar.Value = 0; m_log.AppendText("ERROR: " + error + Environment.NewLine); }
					else m_stage.Text = "Done. The ROM set is ready.";
				});
			});
			t.IsBackground = true;
			t.Start();
		}

		public static void Show(string image, string outDir)
		{
			Application.EnableVisualStyles();
			Application.Run(new App(image, outDir));
		}
	}
}
'@

if (-not ('Spike1Rom.Builder' -as [type])) {
  Add-Type -TypeDefinition $source -ReferencedAssemblies System.Windows.Forms.dll, System.Drawing.dll, System.IO.Compression.dll -Language CSharp
}

# The roms folder of VPX's PinMAME plugin, when VPinballX.ini names one
function Get-DefaultRomsDir([string]$image) {
  $ini = Join-Path $env:APPDATA 'VPinballX\10.8\VPinballX.ini'
  if (Test-Path $ini) {
    $section = ''
    foreach ($line in Get-Content $ini) {
      if ($line -match '^\s*\[(.+)\]\s*$') { $section = $Matches[1]; continue }
      if ($section -eq 'Plugin.PinMAME' -and $line -match '^\s*PinMAMEPath\s*=\s*(.+?)\s*$' -and $Matches[1]) {
        return (Join-Path $Matches[1] 'roms')
      }
    }
  }
  if ($image) { return (Split-Path -Parent $image) }
  return [Environment]::GetFolderPath('MyDocuments')
}

if (-not $OutDir) { $OutDir = Get-DefaultRomsDir $Image }

if ($Image -and -not $Gui) {
  $builder = New-Object Spike1Rom.Builder
  $builder.Log = [Action[string]] { param($line) Write-Host $line }
  $last = @{ Stage = ''; Pct = -1 }
  $builder.Progress = [Action[string, double]] {
    param($stage, $f)
    $pct = if ($f -lt 0) { -1 } else { [int]($f * 100) }
    if ($stage -ne $last.Stage -or $pct -ne $last.Pct) {
      $last.Stage = $stage; $last.Pct = $pct
      if ($pct -ge 0) { Write-Progress -Activity 'Spike 1 ROM set' -Status $stage -PercentComplete $pct } else { Write-Progress -Activity 'Spike 1 ROM set' -Status $stage }
    }
  }
  $null = $builder.Build((Resolve-Path $Image).Path, $OutDir)
  Write-Progress -Activity 'Spike 1 ROM set' -Completed
} else {
  [Spike1Rom.App]::Show($Image, $OutDir)
}
