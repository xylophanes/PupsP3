#!/bin/csh

#-------------------------------------------------------------------------------
# Generate HTML versions of PUPS man pages ...
#-------------------------------------------------------------------------------
#

if($# != 2 || $1 == "--usage") then
	echo ""
	echo "    PUPS-2000 (SUPUPS) manpage to HTML convertor (c) M.A. O'Neill, Tumbling Dice 2002"
	echo "    Checkpointing library is derived from the Tennessee Checkpointing Library (C) 1995 Gerry Kingsley"
	echo "    Process migration via Hebrew University of Jurusalem MOSIX LINUX-Kernel extensions"
	echo ""

	echo "Usage: mantohtml <man page root directory> <HTML directory>"
	echo ""
	exit 0
endif

if(! -e $1) then
	echo "mantohtml: cannot find man page root directory [$1]"
	exit -1
endif

if(! -e $2) then
	echo "... Creating $2"
	mkdir $2 >& /dev/null
else
	\rm $2/* >&/dev/null
endif

set pwd = `pwd`

cd $1
foreach i (`ls -d *`)
	echo ""
	echo "... HTMLising man pages in $i"
	echo ""
	pushd $i >& /dev/null
	foreach j (`ls *`)
		echo "... processing $j"
		man2html $j >$pwd/$2/$j.html
	end
	popd >& /dev/null
end

echo ""
echo "done"
echo ""
exit 0
