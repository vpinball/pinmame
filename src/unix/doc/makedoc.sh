#! /bin/sh

if [ -z "$1" ] ; then
	echo makedoc
	echo 	-all   : make all   docus
	echo 	-txt   : make txt   docus
	echo 	-html  : make html  docus
	echo 	-man   : make groff docus
	echo 	-ps    : make ps    docus
	echo 	-clean : clean up all generated docus
	exit 0
fi

if [ "$1" = "-txt" -o "$1" = "-all" ] ; then
	echo sgml2txt -c latin xmame-doc.sgml
	sgml2txt -c latin xmame-doc.sgml
fi

if [ "$1" = "-html" -o "$1" = "-all" ] ; then
	rm -f *.html
	echo sgml2html -s 1 xmame-doc.sgml
	sgml2html -s 1 xmame-doc.sgml
fi

if [ "$1" = "-man" -o "$1" = "-all" ] ; then
	echo sgml2txt -m -f -c ascii xmame-doc.sgml
	sgml2txt -m -f -c ascii xmame-doc.sgml
	cat xmame-doc.man.skel > xmame.6
	cat xmame-doc.man >> xmame.6
	rm -f xmame-doc.man
fi

if [ "$1" = "-ps" ] ; then
	echo sgml2latex, dvips ...
	sgml2latex -o ps xmame-doc.sgml
fi

if [ "$1" = "-clean" ] ; then
	rm -vf $(find . -name xmame-doc\*.html)
	rm -vf xmame-doc.dvi
	rm -vf xmame-doc.man
	rm -vf xmame-doc.ps
	rm -vf xmame-doc.txt
	rm -vf xmame.6
fi
