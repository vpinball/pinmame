<#
.SYNOPSIS
  Writes the PinMAME source lines for a Stern Spike 1 ROM set.

.DESCRIPTION
  Reads a set's files - the zip Build-Spike1RomSet.ps1 writes (named after the game folder for a
  code version PinMAME has no set for yet), or the game folder itself - hashes each (CRC32, SHA1)
  and prints what a new set needs:

    - its ROM definition for src/wpc/spike1.c, with its CORE_GAMEDEF, CORE_CLONEDEF (an older code
      version, -Parent) or SPIKE1_CLONEDEF line (another edition with its own game data, -Parent
      and -Data);
    - its line for src/wpc/driver.c;
    - its entry for the KnownSet table of Build-Spike1RomSet.ps1.

  The files are laid out as spike1.c lays them out: the program first ("game", or an ELG title's
  "gamer"), then image.bin, then the rest by name, ignoring case; each at a 4 KB boundary, the
  region the whole span rounded up to one. The game data (SPIKE1_INIT*: folder, display, lamps,
  switch numbers) is the title's own: spike1boot --list-switches prints the game's tables.

.PARAMETER Path
  The set's zip, or the game folder (its video directory's clips are taken as the set holds them).

.PARAMETER Set
  The set's name, <name>_<version> (primus_104). Default: the zip's or folder's name, when it has
  that form.

.PARAMETER Folder
  The game folder's name on the machine (primus), for the KnownSet entry. Default: the folder's
  name, or the zip's when the zip is not named after the set.

.PARAMETER LongName
  The game's name as PinMAME lists it, with its version: "Primus (1.04.0)". Without it, a
  placeholder that does not compile.

.PARAMETER Title
  The title the builder shows for the set ("Primus 1.04.0", "KISS Pro 1.41.0"). Default: LongName
  without the parentheses.

.PARAMETER Year
  The title's year. Without it, a placeholder that does not compile.

.PARAMETER Parent
  The set this one is a clone of: an older code version's parent (primus_104 for primus_103), or
  with -Data the edition it sits beside (wwe_135h for wwe_135).

.PARAMETER Data
  The game data of another edition, the name its SPIKE1_INIT* line takes (wwep).

.PARAMETER Machine
  The machine driver: spike1 (default), or spike1_lcd for a title with the CPU board's LCD (WWE LE).

.PARAMETER Out
  A file to write the lines to as well.

.EXAMPLE
  .\Make-Spike1RomDef.ps1 primus.zip -Set primus_104 -LongName "Primus (1.04.0)" -Year 2018
#>
param(
  [Parameter(Mandatory = $true, Position = 0)][string]$Path,
  [string]$Set,
  [string]$Folder,
  [string]$LongName,
  [string]$Title,
  [string]$Year,
  [string]$Parent,
  [string]$Data,
  [string]$Machine = 'spike1',
  [string]$Out
)

$ErrorActionPreference = 'Stop'

Add-Type -TypeDefinition @'
using System;
using System.IO;
using System.Security.Cryptography;

public static class Spike1RomHash
{
	static readonly uint[] Table = MakeTable();
	static uint[] MakeTable()
	{
		var t = new uint[256];
		for (uint n = 0; n < 256; n++) { uint c = n; for (int k = 0; k < 8; k++) c = (c & 1) != 0 ? 0xEDB88320u ^ (c >> 1) : c >> 1; t[n] = c; }
		return t;
	}

	// The stream's length, CRC32 and SHA1, in one pass
	public static object[] Hash(Stream s)
	{
		var buf = new byte[1 << 20];
		uint crc = 0xFFFFFFFFu;
		long length = 0;
		using (var sha = SHA1.Create())
		{
			int n;
			while ((n = s.Read(buf, 0, buf.Length)) > 0)
			{
				for (int i = 0; i < n; i++) crc = Table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
				sha.TransformBlock(buf, 0, n, null, 0);
				length += n;
			}
			sha.TransformFinalBlock(buf, 0, 0);
			return new object[] { length, ~crc, BitConverter.ToString(sha.Hash).Replace("-", "").ToLowerInvariant() };
		}
	}
}
'@

$item = Get-Item -LiteralPath $Path
$files = New-Object System.Collections.Generic.List[object]
function Add-Hashed([string]$name, [System.IO.Stream]$stream) {
  try {
    Write-Host ("hashing " + $name) -ForegroundColor DarkGray
    $h = [Spike1RomHash]::Hash($stream)
    $files.Add([pscustomobject]@{ Name = $name; Size = [long]$h[0]; Crc = [uint32]$h[1]; Sha1 = [string]$h[2] })
  }
  finally { $stream.Dispose() }
}

if ($item.PSIsContainer) {
  $stem = $item.Name
  foreach ($f in Get-ChildItem -LiteralPath $item.FullName -File) { Add-Hashed $f.Name ([System.IO.File]::OpenRead($f.FullName)) }
  $video = Join-Path $item.FullName 'video'
  if (Test-Path -LiteralPath $video -PathType Container) {
    foreach ($f in Get-ChildItem -LiteralPath $video -File) { Add-Hashed $f.Name ([System.IO.File]::OpenRead($f.FullName)) }
  }
}
else {
  $stem = [System.IO.Path]::GetFileNameWithoutExtension($item.Name)
  Add-Type -AssemblyName System.IO.Compression.FileSystem
  $zip = [System.IO.Compression.ZipFile]::OpenRead($item.FullName)
  try {
    foreach ($e in $zip.Entries) {
      if ($e.FullName.EndsWith('/')) { continue }
      Add-Hashed ([System.IO.Path]::GetFileName($e.FullName)) $e.Open()
    }
  }
  finally { $zip.Dispose() }
}
if ($files.Count -eq 0) { throw "No files in $Path" }

if (-not $Set) {
  if ($stem -notmatch '^[a-z0-9]+_[0-9a-z]+$') { throw "Give the set's name with -Set (<name>_<version>): '$stem' is not one." }
  $Set = $stem
}
if ($Set -notmatch '^([a-z0-9]+)_([0-9a-z]+)$') { throw "-Set must be <name>_<version>, as primus_104" }
$name = $Matches[1]; $ver = $Matches[2]
if (-not $Folder -and ($item.PSIsContainer -or $stem -ne $Set)) { $Folder = $stem }
if (-not $Folder) { $Folder = 'TODO_FOLDER' }
if (-not $LongName) { $LongName = 'TODO_LONGNAME' }
if (-not $Year) { $Year = 'TODO_YEAR' }

# The program, then image.bin, then the rest by name ignoring case
function Rank($f) { if ($f.Name -eq 'game' -or $f.Name -eq 'gamer') { 0 } elseif ($f.Name -eq 'image.bin') { 1 } else { 2 } }
$sorted = $files.ToArray()
$keys = [string[]]@($sorted | ForEach-Object { [string](Rank $_) + $_.Name.ToLowerInvariant() })
[Array]::Sort($keys, $sorted, [StringComparer]::Ordinal)  # by code point, as spike1.c's sets are: '.' before '_'
$programs = @($sorted | Where-Object { (Rank $_) -eq 0 })
if ($programs.Count -gt 1) { Write-Warning "Both 'game' and 'gamer' are in the set: the ELG titles' set holds the program the machine runs, alone." }

function Hex8([long]$v) { '0x{0:x8}' -f $v }
$lines = New-Object System.Collections.Generic.List[string]
$offset = [long]0
$loads = New-Object System.Collections.Generic.List[string]
foreach ($f in $sorted) {
  $loads.Add(('    ROM_LOAD("{0}", {1}, {2}, CRC({3:x8}) SHA1({4}))' -f $f.Name, (Hex8 $offset), (Hex8 $f.Size), $f.Crc, $f.Sha1))
  $offset = ($offset + $f.Size + 0xfff) -band (-bnot [long]0xfff)
}
if ($offset -gt [uint32]::MaxValue) { throw "The set spans more than 4 GB, more than a ROM region holds" }

$lines.Add('/*-- src/wpc/spike1.c --*/')
$lines.Add("ROM_START($Set)")
$lines.Add(('  ROM_REGION({0}, SPIKE1_REGION, ROMREGION_MAPPED)' -f (Hex8 $offset)))
$lines.AddRange($loads)
$lines.Add('ROM_END')
if ($Data) {
  if (-not $Parent) { throw "-Data needs -Parent, the edition this one sits beside" }
  $lines.Add(('SPIKE1_CLONEDEF({0}, {1}, {2}, {3}, "{4}", {5}, {6})' -f $name, $ver, $Parent, $Data, $LongName, $Year, $Machine))
}
elseif ($Parent) {
  if ($Parent -notmatch ('^' + [regex]::Escape($name) + '_([0-9a-z]+)$')) { throw "-Parent $Parent is another title's set: an edition with its own game data takes -Data" }
  $lines.Add(('CORE_CLONEDEF({0}, {1}, {2}, "{3}", {4}, "Stern", {5}, 0)' -f $name, $ver, $Matches[1], $LongName, $Year, $Machine))
}
else {
  $lines.Add(('CORE_GAMEDEF({0}, {1}, "{2}", {3}, "Stern", {4}, 0)' -f $name, $ver, $LongName, $Year, $Machine))
}
if (-not $Parent -or $Data) { $lines.Add("/* and its game data, SPIKE1_INIT*($(if ($Data) { $Data } else { $name }), `"$Folder`", ...): spike1boot --list-switches prints the game's tables */") }

# "Primus (1.04.0)" -> "Primus 1.04.0" for the listings
$title = if ($Title) { $Title } else { $LongName -replace '\s*\((.*)\)\s*$', ' $1' }
$plain = $LongName -replace '\s*\((.*)\)\s*$', ''
$version = if ($LongName -match '\(([^()]*)\)\s*$') { $Matches[1] } else { $ver }
$yy = if ($Year -match '^\d{4}$') { $Year.Substring(2) } else { '??' }
$lines.Add('')
$lines.Add('/*-- src/wpc/driver.c --*/')
$lines.Add(('{0,-24}//SPIKE 1: ??/{1} {2} - {3}' -f "DRIVER($name,$ver)", $yy, $plain, $version))

$lines.Add('')
$lines.Add('/*-- scripts/spike1/Build-Spike1RomSet.ps1, KnownSet.All --*/')
$lines.Add(("`t`t`tnew KnownSet {{ Set = `"{0}`", Folder = `"{1}`", Title = `"{2}`", Files = new KnownFile[] {{" -f $Set, $Folder, $title))
for ($i = 0; $i -lt $sorted.Count; $i++) {
  $f = $sorted[$i]
  $lines.Add(("`t`t`t`tnew KnownFile(`"{0}`", {1}, 0x{2:x8}){3}" -f $f.Name, (Hex8 $f.Size), $f.Crc, $(if ($i -eq $sorted.Count - 1) { ' } },' } else { ',' })))
}

$lines | Write-Output
if ($Out) { [System.IO.File]::WriteAllLines($Out, $lines) }
