#!/usr/bin/perl -w
# makecd_mame2.pl
#
# This is a little script to burms roms mame. On each cd, you will have
# the roms, the cabinet, and the flyer. If a game needs another rom like neogeo game,
# the script will put this rom to on the cd, so you lost some space ~300Mo for
# complete mame romset 0.62.
# If you have an 650MB cdr (or better a dvd), you need to change the value
# 700*1024*1024 in the script.
#
# You need to have a roms dir like this:
#   /mnt/bigdur/jeux/arcade>ll
#   total 340
#   drwxr-xr-x   10 luc      luc          4096 2002-11-24 01:20 .
#   drwxr-xr-x    8 luc      luc          4096 2002-11-20 12:03 ..
#   drwxr-xr-x    2 luc      luc         20480 2002-05-03 10:59 cabinets
#   drwxr-xr-x    2 luc      luc         36864 2002-05-03 09:56 flyers
#   drwxr-xr-x    2 luc      luc         73728 2002-11-22 09:21 roms
#   drwxr-xr-x    2 luc      luc          4096 2002-11-24 01:53 samples
#
#  The file gamelist is the output of the last xmame release (xmame -li). If
#  you don't want to put additional rom to play a game, please replace the file
#  by /dev/null
# 
#  Created: Thu, 28 Nov 2002 00:21:29 +0100
#  Last modified: Thu, 28 Nov 2002 00:21:29 +0100
#  Changelog:
#    v0.1: first version
#    v0.1a: oops i forgot to put samples on the cd
#    v0.1b: put rom required on the cd.
#
#  Copyright (C) 2002 Luc Saillard <luc at saillard.org>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

use strict;
use POSIX qw(getcwd);

die "Usage: makecd_mame.pl gamedir burndir gameslist\n" if (@ARGV!=3);

my $gamedir=$ARGV[0];
my $burndir=$ARGV[1];
my $romsinfo=$ARGV[2];

my %roms=();
my %cabinets=();
my %flyers=();
my %samples=();
my %romsinfo=();

#
## Read roms size from $gamedir
##
my $oldcwd=getcwd();
chdir ("$gamedir/roms") || die "Can't change directory $gamedir/roms: $!\n";
opendir(DIR, ".") || die "Can't opendir $gamedir/roms: $!";
foreach (readdir(DIR))
{
  next if (/^\./);
  next unless (/\.zip$/);
  $roms{$_}=(stat($_))[7];
}
closedir DIR;
chdir ("$gamedir/samples") || die "Can't change directory $gamedir/samples $!\n";
opendir(DIR, ".") || die "Can't opendir $gamedir/samples $!";
foreach (readdir(DIR))
{
  next if (/^\./);
  next unless (/\.zip$/);
  $samples{$_}=(stat($_))[7];
}
closedir DIR;
chdir ("$gamedir/cabinets") || die "Can't change directory $gamedir/cabinets: $!\n";
opendir(DIR, ".") || die "Can't opendir $gamedir/cabinets: $!";
foreach (readdir(DIR))
{
  next if (/^\./);
  $cabinets{$_}=(stat($_))[7];
}
closedir DIR;
chdir ("$gamedir/flyers") || die "Can't change directory $gamedir/flyers $!\n";
opendir(DIR, ".") || die "Can't opendir $gamedir/flyers: $!";
foreach (readdir(DIR))
{
  next if (/^\./);
  $flyers{$_}=(stat($_))[7];
}
closedir DIR;

chdir($oldcwd);


#
# Read roms infos (xmame -li ouputs)
#
my $currentrom_name="";
my $currentrom_need;
my $total_games=0;
my $line=0;
open(F,$romsinfo) || die "Can't open file $romsinfo: $!\n";
while(<F>)
{
  chomp;
  # This is very simply parser
  if (/^game \($/) {
    $total_games++;
    undef($currentrom_name);
    undef($currentrom_need);
  } elsif (/^\tromof (\w+)$/) {
    $currentrom_need=$1;
  } elsif (/^\tname (\w+)$/) {
    $currentrom_name=$1;
  } elsif (/^\)$/) {
    print "In the previous block at line $line we haven't found a valid rom name\n" unless (defined($currentrom_name));
    $romsinfo{"$currentrom_name.zip"}="$currentrom_need.zip" if (defined($currentrom_need));
  }
  $line++;
}
close(F);
print "Total games: $total_games\n";


#
# Calculate size on each cd, and create links
#
my $currentcd_size=0;
my $currentcd_idx=1;
my $currentcd_nam="$burndir/Mame-cd 1";
mkdir ($burndir) unless (-d $burndir);
mkdir("$currentcd_nam") unless (-d "$currentcd_nam");
mkdir("$currentcd_nam/roms") unless (-d "$currentcd_nam/roms");
mkdir("$currentcd_nam/cabinets") unless (-d "$currentcd_nam/cabinets");
mkdir("$currentcd_nam/flyers") unless (-d "$currentcd_nam/flyers");
mkdir("$currentcd_nam/samples") unless (-d "$currentcd_nam/samples");

foreach (sort keys %roms)
{
  my $imagename = $_;
  $imagename =~ s/\.zip$/.png/;

  # Try to put the next rom of the cd
  my $size=0;
  $size+=$roms{$_} unless (-l "$currentcd_nam/roms/$_"); 
  $size+=$roms{$romsinfo{$_}} if (defined($romsinfo{$_}) && (! -l "$currentcd_nam/roms/$romsinfo{$_}"));
  $size+=$samples{$_} if (defined($samples{$_}) && (! -l "$currentcd_nam/samples/$_"));
  $size+=$cabinets{$imagename} if (defined($cabinets{$imagename}) && (! -l "$currentcd_nam/cabinets/$imagename"));
  $size+=$flyers{$imagename} if (defined($flyers{$imagename}) && (! -l "$currentcd_nam/flyers/$imagename"));

  # Check if the file can be put on the cd
  if ($currentcd_size+$size>680*1024*1024) {
    $currentcd_size=0;
    $currentcd_idx++;
    $currentcd_nam="$burndir/Mame-cd $currentcd_idx";
    mkdir("$currentcd_nam") unless (-d "$currentcd_nam");
    mkdir("$currentcd_nam/roms") unless (-d "$currentcd_nam/roms");
    mkdir("$currentcd_nam/cabinets") unless (-d "$currentcd_nam/cabinets");
    mkdir("$currentcd_nam/flyers") unless (-d "$currentcd_nam/flyers");
    mkdir("$currentcd_nam/samples") unless (-d "$currentcd_nam/samples");

  }
  $currentcd_size+=$size;

  symlink("$gamedir/roms/$_","$currentcd_nam/roms/$_") unless (-l "$currentcd_nam/roms/$_");
  symlink("$gamedir/roms/$romsinfo{$_}","$currentcd_nam/roms/$romsinfo{$_}") if (defined($romsinfo{$_}) && (! -l "$currentcd_nam/roms/$romsinfo{$_}"));
  symlink("$gamedir/samples/$_","$currentcd_nam/samples/$_") if (defined($samples{$_}) && (! -l "$currentcd_nam/samples/$_"));
  symlink("$gamedir/cabinets/$imagename","$currentcd_nam/cabinets/$imagename") if (defined($cabinets{$imagename}) && (! -l "$currentcd_nam/cabinets/$imagename"));
  symlink("$gamedir/flyers/$imagename","$currentcd_nam/flyers/$imagename") if (defined($flyers{$imagename}) && (! -l "$currentcd_nam/flyers/$imagename"));
}


print "Generate iso with:\n";
print "cd $burndir ; for i in " . join(" ",1 ... $currentcd_idx) . "; do \n";
print "mkisofs -J -r -f -v -V \"Mame cd \$i\" -o mamecd\$i.iso \"Mame-cd \$i\" \n";
print "done\n";




