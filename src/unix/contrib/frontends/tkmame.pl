#!/usr/bin/perl -w 

# set the path to your ROMS below
my ($MAMEDIR) = "/usr/local/share/xmame/roms";

# set location/name of xmamerc here (typically ~/xmame/) or (/usr/local/share/xmame/)
my ($XMAMERC_LOC) = "/usr/local/share/xmame/xmamerc";

# set name of MAME executable/bin
my ($MAMEBIN) = "/usr/local/bin/xmame";


my ($ROM,$ROMLIST,$col,$row,$num,$widget,$mw,$table,@ROMLIST,$SWITCH);
my ($conf,@mameconf,@mame_settings,$name,$line,$change,$newline,@mamechange,%settings,$setting);

use Tk;
use English;
use Tk::Table;
use strict;
use Tk::BrowseEntry;

$mw = new MainWindow;
$mw->Label(-text=>'MAME GAMES')->pack;


# set default game options
$SWITCH = "-cheat";


#Note: if you have directories in your $MAMEDIR dir they will be on your menu. 
#you might want to add more greps if you have other non-ROM stuff in there
chdir($MAMEDIR);
opendir(MAME,$MAMEDIR);
@ROMLIST = sort(grep( /.zip$/ , (grep(-f,readdir(MAME)))));
closedir(MAME);

chomp (@ROMLIST);
my ($mycol) = $#ROMLIST/7;
$mycol =  13 unless ($mycol < 13);
$table =$mw->Table(-rows=>$mycol,-columns=>7,-scrollbars=>'e',)->pack(-expand=>1,-fill=>'x');
# the main loop
$col=0;
$row=0;
$num=0;
foreach $ROM (@ROMLIST) {
my ($ROM) = $ROM;
$ROM =~ s/\.zip//ig;
if ($col > 6) {
$col=0;
$row++;
}
$widget = $table->Button(-text=>$ROM,-command=>sub{play($ROM)});
$num++;

$table->put($row,$col,$widget);
$col++;
}

$mw->Button(-text=>'Quit',-command=>['destroy',$mw])->pack(-side=>'bottom',-expand=>1,-fill=>'x');
$mw->Button(-text=>'Configure xmamerc',-command=>sub{&config_mame})->pack(-side=>'bottom',-side=>'right');
  my  $switch = $mw->BrowseEntry(-label=>"Command line options:",-variable=>\$SWITCH);
    $switch->pack;

MainLoop;

sub play {
my ($GAME) = @_;
system("$MAMEBIN $GAME $SWITCH");
}
sub set_switch {
    ($SWITCH) = @_;
}




sub config_mame {
undef(@mameconf);
undef(@mame_settings);
%settings = ();
open(MAMECONF,"+<".$XMAMERC_LOC) || die $!;
@mameconf= <MAMECONF>;
close(MAMECONF);
foreach $name (@mameconf) {
if ($name !~ /^#/ && $name =~ /\S+/) {
chomp $name;
    push(@mame_settings,$name);
}}

$conf = $mw->Toplevel();
foreach $setting (@mame_settings) {
$setting =~ s/\s+/ /g;
my ($name,$value) = split(/ /,$setting);
$settings{"$name"} = $value;

$conf->BrowseEntry(-label=>$name,-variable=>\$settings{"$name"})->pack(-side=>'top');
}

$conf->Button(-text=>"Save Settings to xmamerc",-command=>sub{&write_rc})->pack(-side=>"bottom",-side=>'left',-expand=>1,-fill=>'x');
$conf->Button(-text=>"Cancel",-command=>['destroy',$conf])->pack(-side=>'bottom',-side=>'right',-expand=>1,-fill=>'x');
}

sub write_rc {
undef(@mameconf);
open(MAMECONF,"+<".$XMAMERC_LOC) || die $!;
@mameconf = <MAMECONF>;
close(MAMECONF);

foreach $line (@mameconf) {
          if ($line !~ /^#/) {
	       foreach $change (keys %settings) {
		   if ($line =~ /$change/g) {
			   $line = "$change"."     "."$settings{($change)}"."\n";
			   push(@mamechange,$line);
		       } 
	       }
	  } else {     
	      $line = $line . "\n" unless ($line =~ /\n$/);
			    push(@mamechange,$line); 
			}

	}       
		open(MAMEWRITE,"+<".$XMAMERC_LOC) || die $!;
		foreach $newline (@mamechange) {
						print MAMEWRITE $newline;
						}
		close(MAMEWRITE);
		print "Changes saved to ".$XMAMERC_LOC;
my $msg = $conf->Toplevel;
	 
$msg->Label(-text=>"Changes saved to $XMAMERC_LOC")->pack(-side=>'top');
$msg->Button(-text=>"Dismiss",-command=>['destroy',$conf])->pack(-side=>'bottom');
}







