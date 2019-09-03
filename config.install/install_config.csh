#!/bin/tcsh
#
#----------------------------------------------------------------------------------
#  Install configuration directory in p3 directory hierarchy
#  M.A. O'Neill, Tumbling Dice, 2009-2017
#
#  $1 is the OS architecture (e.g Linux) 
#  $2 is the P3 build type (vanilla or cluster)
#----------------------------------------------------------------------------------
#

echo ""
echo "    PUPS/P3 Configuration installer (version 1.06)"
echo "    M.A. O'Neill, Tumbling Dice, 2017"
echo ""

if($1 == "") then
	set arch = linux
else if($1 == usage || $1 == help) then
	echo ""
	echo "    Usage: configure_config.csh [arch:linux] [btype: cluster]"
	echo ""

	exit 1
else
	set arch = $1
endif 

if($2 == "") then
	set btype = cluster
else if($2 != vanilla && $2 != cluster) then
	echo ""
	echo "    config_install ERROR: expecting 'vanilla' or 'cluster'"
	echo ""

	exit -1
else
	set btype = $2
endif


#---------------------------
# Set appropriate base path
#---------------------------

if(`whoami` == root) then
	set basepath = "/usr"
else
	set basepath = ~
endif

if(`whoami` == root) then
	set libPath = "/usr/lib" 
	set incPath = "/usr/include/p3"
else
	set libPath = "$basepath/lib"
	set incPath = "$basepath/include/p3"
endif

set mach = `upcase $MACHTYPE`
sed "s|BASEPATH|$basepath|g" <$arch.$btype | sed "s|INCPATH|$incPath|g" | sed "s|DARCH|D$mach|g" | sed "s|LIBPATH|$libPath|g" >$arch.$btype.$$


#-------------------------
# Copy configuration file
#-------------------------

if(`whoami` == root) then
	\mv $arch.$btype.$$ /p3/$arch.$btype
else
	\mv $arch.$btype.$$ ~/p3/$arch.$btype
endif

echo ""
echo "    Finished"
echo ""

exit 0

