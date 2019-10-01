#!/bin/bash
#
#----------------------------------------------------------------------------------
#  Install configuration directory in p3 directory hierarchy
#  M.A. O'Neill, Tumbling Dice, 2009-2019
#
#  $1 is the OS architecture (e.g Linux) 
#  $2 is the P3 build type (vanilla or cluster)
#----------------------------------------------------------------------------------
#

echo ""
echo "    PUPS/P3 Configuration installer (version 2.00)"
echo "    M.A. O'Neill, Tumbling Dice, 2019"
echo ""

if [ "$1" = "" ] ; then
	arch=linux
elif [ "$1" = usage]  || [ "$1" = help ] ; then
	echo ""
	echo "    Usage: configure_config.csh [arch:linux] [btype: cluster]"
	echo ""

	exit 1 
else
	arch=$1
fi 

if [ "$2" = "" ] ; then
	btype=cluster
elif [ "$2" != vanilla ]  &&  [ "$2" != cluster ] ; then
	echo ""
	echo "    config_install ERROR: expecting 'vanilla' or 'cluster'"
	echo ""

	exit 255
else
	btype=$2
fi


#---------------------------
# Set appropriate base path
#---------------------------

if [ $(whoami) = root ] ; then
	basepath="/usr"
else
	basepath=$HOME
fi

if [ $(whoami) = root ] ; then
	libPath="/usr/lib" 
	incPath="/usr/include/p3"
else
	libPath="$basepath/lib"
	incPath="$basepath/include/p3"
fi

mach=$(upcase $MACHTYPE)
sed "s|BASEPATH|$basepath|g" <$arch.$btype | sed "s|INCPATH|$incPath|g" | sed "s|DARCH|D$mach|g" | sed "s|LIBPATH|$libPath|g" >$arch.$btype.$$


#-------------------------
# Copy configuration file
#-------------------------

if [ $(whoami) = root ] ; then
	\mv $arch.$btype.$$ /p3/$mach.$arch.$btype 2> /dev/null
else
	\mv $arch.$btype.$$ $HOME/p3/$mach.$arch.$btype 2> /dev/null
fi

echo ""
echo "    Finished"
echo ""

exit 0

