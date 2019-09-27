#!/bin/tcsh

#---------------------------------------------------------------------
# Edit echo to return machine architecture:
#      e.g. echo ix86.linux    - linux system. Intel 80x86 Proc.
#           echo sparc.linux   - linux system. SPARC Proc.
#           echo sparc.sunos   - SunOS system. SPARC Proc
#           echo sparc.solaris - Solaris system. SPARC proc.
#           echo axp.osf1      - Digital UNIX system. AXP Proc.
#
#---------------------------------------------------------------------
#

if($# > 1 || $1 == "usage" || $1 == "help" || $1 == "-usage" || $1 == "--usage") then
	echo ""
	echo "    PUPS-2000 (SUPUPS) system architecture identifier script (c) M.A. O'Neill, Tumbling Dice 2002-2014"
	echo ""

        echo "Usage: pupsuname [upcase]"
        echo ""
        exit 0
endif

set BTYPE = unknown 
if(-e ~/p3) then
	if(`ls ~/p3 | grep cluster` != "") then
		set BTYPE = cluster
	else if(`ls ~/p3 | grep vanilla` != "") then
                set BTYPE = vanilla
	else
		set BTYPE = unknown
	endif
endif

echo $MACHTYPE.$OSTYPE.$BTYPE 
exit 0
