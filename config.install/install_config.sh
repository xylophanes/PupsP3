#!/bin/bash

#-----------------------------------------------------------
#  Install configuration directory in p3 directory hierarchy
#  M.A. O'Neill, Tumbling Dice, 2009-2024
#
#  $1 is the OS architecture (e.g Linux) 
#-----------------------------------------------------------

echo ""
echo "    PUPS/P3 Configuration installer (version 3.00)"
echo "    M.A. O'Neill, Tumbling Dice 2000-2024"
echo ""

if [ "$1" = "" ] ; then
	arch=linux
elif [ "$1" = usage ]  || [ "$1" = help ] ; then
	echo ""
	echo "    Usage: configure_config.csh [arch:linux] [btype: cluster]"
	echo ""

	exit 1 
else
	arch=$1
fi 

btype=cluster


#---------------------------
# Set appropriate base path
#---------------------------
#------
# Root
#------

if [ $(whoami) = root ] ; then
	basepath="/usr"


#------
# User
#------

else
	basepath=$HOME
fi


#------
# Root
#------

if [ $(whoami) = root ] ; then


	#------------------
	# 64/32 bit system
	#------------------

	if [ "$(pupsuname | grep x86_64)" != "" ] || [ "$(pupsuname | grep aarch64)" != "" ]; then
		libPath="/usr/lib64" 


	#---------------------
	# 64 or 32 bit system
	#---------------------

	else
		libPath="/usr/lib" 
	fi

	incPath="/usr/include/p3"


#------
# User
#------

else

	#-------------------------
	# Local PUPS/P3 libraries
	#-------------------------

	if [ -e $basepath/p3 ] ; then
		libPath="$basepath/lib"
		incPath="$basepath/include/p3"


	#-------------------------------
	# System wide PUPS/P3 libraries
	#-------------------------------

	else

		#-------
		# 64 bit
		#-------

		if [ "$(pupsuname | grep x86_64)" != "" ] ; then
			libPath="/usr/lib64" 


		#-------
		# 32 bit
		#-------

		else
			libPath="/usr/lib" 
		fi

		incPath="/usr/include/p3"
	fi
fi

mach=$(upcase $MACHTYPE)
sed "s|BASEPATH|$basepath|g" <$arch.$btype | sed "s|INCPATH|$incPath|g" | sed "s|DARCH|D$mach|g" | sed "s|LIBPATH|$libPath|g" >$arch.$btype.$$


#------------------------
# Copy configuration file
#------------------------

if [ $(whoami) = root ] ; then
	\cp $arch.$btype.$$ /p3/$mach.$arch.$btype	2> /dev/null
else
	\cp $arch.$btype.$$ $HOME/p3/$mach.$arch.$btype 2> /dev/null
fi
\rm $arch.$btype.$$ 2> /dev/null

echo ""
echo "    Finished"
echo ""

exit 0

