open(READ,'src/rules.mak');
$cpulist = "";
$soundlist = "";
while (<READ>)
{
	if (/CPU=\$\(strip \$\(findstring ([^@]*)@,\$\(CPUS\)\)\)/)
	{
		$cpulist .= "#if (HAS_$1)\n\tCPU_$1,\n#endif\n";
	}
	if (/SOUND=\$\(strip \$\(findstring ([^@]*)@,\$\(SOUNDS\)\)\)/)
	{
		$soundlist .= "#if (HAS_$1)\n\tSOUND_$1,\n#endif\n";
	}
}
close(READ);

undef $/;
$file = 'src/cpuintrf.h';
open(READ,$file);
$src = <READ>;
close(READ);
if ($src =~ s/CPU_DUMMY.*CPU_COUNT/CPU_DUMMY,\n$cpulist\tCPU_COUNT/sg)
{
	print "Regenerating src/cpuintrf.h\n";
	open(WRITE,">$file");
	print WRITE $src;
	close(WRITE);
}
else
{
	print "makelist.pl: can't find CPU list in src/cpuintrf.h\n";
}

$file = 'src/sndintrf.h';
open(READ,$file);
$src = <READ>;
close(READ);
if ($src =~ s/SOUND_DUMMY.*SOUND_COUNT/SOUND_DUMMY,\n$soundlist\tSOUND_COUNT/sg)
{
	print "Regenerating src/sndintrf.h\n";
	open(WRITE,">$file");
	print WRITE $src;
	close(WRITE);
}
else
{
	print "makelist.pl: can't find SOUND list in src/sndintrf.h\n";
}
