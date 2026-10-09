<#
.SYNOPSIS
  Builds a PinMAME Stern Spike 1 ROM set from Stern's own download.

.DESCRIPTION
  Stern publishes each Spike 1 code version as an SD-card image, <title>-<version>.iso.zip, or
  for some titles (Heavy Metal) only as an update package, <title>-<version>.spk. The image holds
  an MBR with an extended partition; one of its ext3 partitions carries the game folder (the
  program, image.bin, lcdinsert.bin when the title has an LCD insert, and the node board
  firmware). The package holds the same folder as one of its groups. PinMAME's ROM set is that
  folder as a zip, named after the set.

  This script reads the image straight out of the .iso.zip - no 3.7 GB temporary copy - in a few
  forward passes, or the files straight out of the .spk, writes <set>.zip (stored, not
  compressed, so PinMAME maps image.bin quickly) and checks every file against the set's CRCs.

  Without -Image it opens a window. Start it with "Build ROM set.cmd", or drop the .iso.zip or
  .spk onto that file.

.PARAMETER Image
  Stern's .iso.zip, the .iso inside it, or Stern's .spk.

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
			new KnownSet { Set = "wwe_135h", Folder = "WWE_LE", Title = "WWE WrestleMania LE 1.35.0", Files = new KnownFile[] {
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
		};
	}

	// The disk image as a stream read front to back; a pass opens it anew
	public class ImageSource
	{
		readonly string m_path;
		readonly bool m_zipped;
		public long Length;
		public string EntryName;

		public ImageSource(string path)
		{
			m_path = path;
			m_zipped = path.EndsWith(".zip", StringComparison.OrdinalIgnoreCase);
			if (!m_zipped) { Length = new FileInfo(path).Length; EntryName = Path.GetFileName(path); return; }
			using (var zip = new ZipArchive(File.OpenRead(path), ZipArchiveMode.Read))
			{
				ZipArchiveEntry best = null;
				foreach (var e in zip.Entries) if (best == null || e.Length > best.Length) best = e;
				if (best == null) throw new Exception("The zip file is empty.");
				Length = best.Length;
				EntryName = best.FullName;
			}
		}

		public ForwardReader Open()
		{
			if (!m_zipped) return new ForwardReader(File.OpenRead(m_path), null, true);
			var zip = new ZipArchive(File.OpenRead(m_path), ZipArchiveMode.Read);
			return new ForwardReader(zip.GetEntry(EntryName).Open(), zip, false);
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

		// The game folder is the partition's directory that holds "game" and "image.bin"
		void RootListed(Partition p, List<KeyValuePair<string, uint>> entries, List<byte> types)
		{
			for (int i = 0; i < entries.Count; i++)
			{
				string name = entries[i].Key;
				if (name == "." || name == ".." || name == "lost+found" || (types[i] != 0 && types[i] != 2)) continue;
				uint child = entries[i].Value;
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

			var intro = new Label { Text = "Builds the PinMAME ROM set from Stern's SD-card image (.iso.zip) or update package (.spk). Nothing in it is changed.", AutoSize = true, Margin = new Padding(0, 0, 0, 10) };
			table.Controls.Add(intro, 0, 0); table.SetColumnSpan(intro, 3);

			table.Controls.Add(new Label { Text = "Stern image", AutoSize = true, Anchor = AnchorStyles.Left }, 0, 1);
			m_image.Dock = DockStyle.Fill; m_image.Text = image ?? "";
			table.Controls.Add(m_image, 1, 1);
			var pickImage = new Button { Text = "Browse...", AutoSize = true };
			pickImage.Click += delegate {
				using (var d = new OpenFileDialog { Filter = "Stern download (*.iso.zip;*.iso;*.spk)|*.iso.zip;*.iso;*.spk|All files (*.*)|*.*", Title = "Stern image" })
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
			if (!File.Exists(image)) { MessageBox.Show(this, "Choose Stern's .iso.zip or .spk file first.", Text); return; }
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
