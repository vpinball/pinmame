#!/usr/bin/perl
#
# romulan.pl - Nathan Hand (nathanh@manu.com.au)
#
# MAME rom analyser. Needs a merged set in /usr/local/share/xmame/roms
# and the xmame + unzip binaries in your PATH. Romulan will detect &
# report problems that commonly occur with merged sets, eg.
#
#   - missing sets
#   - missing roms from a set
#   - extra files in a set
#   - roms with the wrong crc
#   - roms with the wrong name
#   - roms in the wrong set
#
# Revision History
#   13 May 2000 - nathanh - initial release, detects bad crcs
#   19 Apr 2001 - nathanh - detects misnamed roms
#    9 Mar 2002 - nathanh - detects misplaced roms
#   10 Mar 2002 - nathanh - added command line options
#
# ****
# This script is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or any later version.
#
# This script is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
# ****
# 

require 5.000;
use Data::Dumper;
use Getopt::Long;

# config options, nothing fancy

$zipdir = "/usr/local/share/xmame/roms";
$unzip_binary = "unzip";
$xmame_binary = "xmame";
$merged = 1;
$strict = 0;
$help = 0;

# read user options

$result = GetOptions('merged!' => \$merged, 'strict!' => \$strict, 'help' => \$help);

if (!$result || $help) {
    print stderr "usage: romulan.pl [options]\n\n";
    print stderr "\t--merged     merged sets [default]\n";
    print stderr "\t--nomerged   unmerged sets\n";
    print stderr "\t--strict     strict merged sets\n";
    print stderr "\t--nostrict   non-strict merged sets [default]\n";
    print stderr "\t--help       this help screen\n\n";
    print stderr "Merged means roms in a master set are not duplicated in\n";
    print stderr "the clone set. For example, roms in pacman would not be\n";
    print stderr "duplicated in pacmanbl.\n\n";
    print stderr "Strict merged means that clones with common masters are\n";
    print stderr "further consolidated: roms common to two or more clones\n";
    print stderr "are moved from the clones to the master.\n\n";
    print stderr "This script doesn't do anything itself. It just reports\n";
    print stderr "on what needs to be done.\n";
    exit(1);
}

if (!$merged && $strict) {
    print stderr "error: you can't have strict sets that aren't merged!\n";
    print stderr "(hint: try 'romulan.pl --merged --strict' instead)\n";
    exit(1);
}

# useful things

sub array_subtract {
    my($ref1, $ref2, %mark) = @_;
    grep $mark{$_}++, @{$ref2};
    return grep !$mark{$_}, @{$ref1};
}

sub array_intersect {
    my($ref1, $ref2, %mark) = @_;
    grep $mark{$_}++, @{$ref2};
    return grep $mark{$_}, @{$ref1};
}

# examine files, extract fileinfo

opendir(DIR, $zipdir) || die "cannont opendir $zipdir: $!";
@files = grep { -f "$zipdir/$_" } readdir(DIR);
closedir(DIR);

foreach $file (@files) {
    foreach $line (`$unzip_binary -v -qq $zipdir/$file`) {
        $line =~ tr/A-Z/a-z/;
        @line = split /\s+/, $line;
        ($dum, $len, $met, $siz, $rat, $dat, $tim, $crc, $nam) = @line;

        $file =~ tr/A-Z/a-z/;
        $file =~ s/\.zip$//;
        $info = "name $nam size $len crc $crc";

        push @{$zips{$file}}, $info;
    }
}

# examine listinfo, extract listinfo

foreach $line (`$xmame_binary -listinfo 2>/dev/null`) {
    chop $line;

    $name = $1             if ($line =~ /^\s+name\s(.*)$/);
    $description{$name} = $1       if ($line =~ /^\s+description\s(.*)$/);
    $year{$name} = $1          if ($line =~ /^\s+year\s(.*)$/);
    $manufacturer{$name} = $1      if ($line =~ /^\s+manufacturer\s(.*)$/);
    $history{$name} = $1       if ($line =~ /^\s+history\s(.*)$/);
    $video{$name} = $1         if ($line =~ /^\s+video\s(.*)$/);
    $sound{$name} = $1         if ($line =~ /^\s+sound\s(.*)$/);
    $input{$name} = $1         if ($line =~ /^\s+input\s(.*)$/);
    $driver{$name} = $1        if ($line =~ /^\s+driver\s(.*)$/);
    $master{$name} = $1        if ($line =~ /^\s+cloneof\s(.*)$/);
    $master{$name} = $1        if ($line =~ /^\s+romof\s(.*)$/);

    if ($line =~ /^\s+rom\s\(\s(.*)\s\)$/) {
        %info = split /\s+/, $1;
        push @{$roms{$name}}, "name $info{name} size $info{size} crc $info{crc}";
    }

    if ($line =~ /^\s+chip\s\(\s(.*)\s\)$/) {
        push @{$chips{$name}}, $1;
    }

    if ($line =~ /^\s+dipswitch\s\(\s(.*)\s\)$/) {
        push @{$dipswitches{$name}}, $1;
    }

    if ($line eq "game (" || $line eq "resource (") {
        undef $rom;
    }
}

# strip master roms from clone sets

if ($merged) {
    foreach $clone (keys %master) {
        @{$roms{$clone}} = &array_subtract($roms{$clone}, $roms{$master{$clone}});
    }
}

# move shared roms from clones to master
#
#   step 1 - find cousins of clone, build list of common roms
#   step 2 - strip common roms from cousins and clone
#   step 3 - add common roms to master

if ($strict) {
    foreach $clone (keys %master) {
        local(@common_roms);

        foreach $cousin (keys %description) {
            next if $clone eq $cousin;
            next if $master{$clone} ne $master{$cousin};
            push @common_roms, &array_intersect($roms{$cousin}, $roms{$clone});
        }

        foreach $cousin (keys %description) {
            next if $master{$clone} ne $master{$cousin};
            @{$roms{$cousin}} = &array_subtract($roms{$cousin}, \@common_roms);
        }

        local(%mark);
        grep $mark{$_}++, @common_roms;
        push @{$roms{$master{$clone}}}, keys %mark;
    }
}

# find sets without a corresponding zip, and vice versa

@zipkeys = keys %zips;
@romkeys = keys %roms;

foreach $zip (&array_subtract(\@romkeys, \@zipkeys)) {
    print "file for $zip is missing!\n";
}

foreach $zip (&array_subtract(\@zipkeys, \@romkeys)) {
    print "what is file $zip for?\n";
}

# find roms that are missing, extra, misnamed or wrongcrc

foreach $set (keys %description) {
    @{$missing_roms{$set}} = &array_subtract($roms{$set}, $zips{$set});
    @{$extra_roms{$set}} = &array_subtract($zips{$set}, $roms{$set});

    @{$misnamed_roms{$set}} = grep {
        %info = split /\s+/, $rom=$_;
        push @{$notmissing_roms{$set}}, grep {
            ${$misnamed_be{$set}}{$rom} = $1 if /name (.*) size $info{size} crc $info{crc}/;
        } @{$missing_roms{$set}};
        ${$misnamed_be{$set}}{$rom};
    } @{$extra_roms{$set}};

    @{$wrongcrc_roms{$set}} = grep {
        %info = split /\s+/, $rom=$_;
        push @{$notmissing_roms{$set}}, grep {
            ${$wrongcrc_be{$set}}{$rom} = $1 if /name $info{name} size $info{size} crc (.*)/;
        } @{$missing_roms{$set}};
        ${$wrongcrc_be{$set}}{$rom};
    } @{$extra_roms{$set}};

    foreach $rom (@{$extra_roms{$set}}) { $extra_roms_all{$rom} = $set; }
    foreach $rom (@{$missing_roms{$set}}) { $missing_roms_all{$rom} = $set; }
}

# find roms that are in wrong sets

foreach $set (keys %description) {
    foreach $rom (@{$extra_roms{$set}}) {
        if ($otherset = $missing_roms_all{$rom}) {
            push @{$otherset_roms{$otherset}}, $rom;
            ${$otherset_be{$otherset}}{$rom} = $set;
        }
    }

    foreach $rom (@{$missing_roms{$set}}) {
        if ($otherset = $extra_roms_all{$rom}) {
            push @{$wrongset_roms{$otherset}}, $rom;
            ${$wrongset_be{$otherset}}{$rom} = $set;
        }
    }
}

# consolidate the discoveries

foreach $set (keys %description) {
    @{$extra_roms{$set}} = &array_subtract($extra_roms{$set}, $misnamed_roms{$set});
    @{$extra_roms{$set}} = &array_subtract($extra_roms{$set}, $wrongcrc_roms{$set});
    @{$extra_roms{$set}} = &array_subtract($extra_roms{$set}, $wrongset_roms{$set});
    @{$missing_roms{$set}} = &array_subtract($missing_roms{$set}, $otherset_roms{$set});
    @{$missing_roms{$set}} = &array_subtract($missing_roms{$set}, $notmissing_roms{$set});
}

# report the discoveries

foreach $set (keys %description) {
    print "examining $set... ";
    print "broken!\n" if @{$missing_roms{$set}} || @{$extra_roms{$set}} || @{$misnamed_roms{$set}} || @{$wrongcrc_roms{$set}} || @{$wrongset_roms{$set}} || @{$otherset_roms{$set}};
    print "perfect!\n" unless @{$missing_roms{$set}} || @{$extra_roms{$set}} || @{$misnamed_roms{$set}} || @{$wrongcrc_roms{$set}} || @{$wrongset_roms{$set}} || @{$otherset_roms{$set}};

    foreach $rom (@{$missing_roms{$set}})    { print "\tmissing:   $rom\n"; }
    foreach $rom (@{$extra_roms{$set}})      { print "\textra:     $rom\n"; }
    foreach $rom (@{$otherset_roms{$set}})   { print "\tmissing:   $rom (found in other set ${$otherset_be{$set}}{$rom})\n"; }
    foreach $rom (@{$wrongset_roms{$set}})   { print "\textra:     $rom (should be in set ${$wrongset_be{$set}}{$rom})\n"; }
    foreach $rom (@{$misnamed_roms{$set}})   { print "\tmisnamed:  $rom should be ${$misnamed_be{$set}}{$rom}\n"; }
    foreach $rom (@{$wrongcrc_roms{$set}})   {
        if (${$wrongcrc_be{$set}}{$rom} eq "00000000") {
            print "\twrongcrc:  $rom BEST DUMP KNOWN\n";
        } else {
            print "\twrongcrc:  $rom should be ${$wrongcrc_be{$set}}{$rom}\n"
        }
    }
}
