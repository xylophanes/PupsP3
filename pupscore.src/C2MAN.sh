#!/bin/bash
#
#-------------------------------------------------------------------------------------
#  Generate stub man page documentation from a C or C++ source using c2man application
#  (C) M.A. O'Neill 2009-2023
#
#  $1 is source file to generate man page from.
#  $2 is name of man page file.
#-------------------------------------------------------------------------------------
#


#---------------------------
# Display usage information
#---------------------------

if [ $1 = "usage"]  || [ $1 = "help" ] ;  then
	echo
	echo "CM2MAN version 1.00"
	echo "(C) M.A. O'Neill, Tumbling Dice, 2009-2023"
	echo ""
	echo "Usage CM2MAN <source C file name> !<man file name>"
	echo ""

	exit 1


#---------------
# Display banner 
#---------------

else

	echo "C2MAN version 1.00"
	echo "(C) M.A. O'Neill, Tumbling Dice, 2009-2023"
	echo ""
fi


#-------------------------------------------
# Named output file specified for "man" page
#-------------------------------------------

if [ "$2" != "" ] ; then

	if [ -e $2 ] ;  then
		rm $2
	fi	

	c2man -DPSRP_AUTHENTICATE -DCKPT_SUPPORT -DSOCKET_SUPPORT -DPERSISTENT_HEAP_SUPPORT  -DPTHREAD_SUPPORT -DDLL_SUPPORT -I. -g -ln -n $1 -o- > $2

	if [ "$?" != 0 ] ; then
		echo ""
		echo "    ERROR: problem with c2man command
		echo ""

		exit 255
	fi


#-----------------------------------
# Default output file for "man" page
#-----------------------------------

else
	c2man -DPSRP_AUTHENTICATE -DCKPT_SUPPORT -DSOCKET_SUPPORT DPERSISTENT_HEAP_SUPPORT  -DPTHREAD_SUPPORT -DDLL_SUPPORT -I. -g -ln -n $1

	if [ "$?" != 0 ] ; then
		echo ""
		echo "    ERROR: problem with c2man command
		echo ""

		exit 255
	fi
fi

exit 0

