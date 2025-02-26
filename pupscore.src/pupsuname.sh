#!/bin/bash
#
#---------------------------------------------------
# Get PUPS/P3 system architecture
# (C) M.A. O'Neill, Tumbling Dice, 2nd February 2025 
#--------------------------------------------------- 
#--------------------
# Parse command line
#--------------------

do_upcase=FALSE
if [ "$#" -gt 0 ] ; then


	#--------
	# Upcase
	#--------

	if [ "$1" = upcase ] ; then
		do_upcase=TRUE


	#--------------------
	# Error - show usage
	#--------------------

	else
		echo ""
		echo "    PUPSP3 (SUPUPS) system architecture identifier script (c) M.A. O'Neill, Tumbling Dice 2002-2025"
		echo ""

        	echo "Usage: pupsuname [upcase]"
        	echo ""

        	exit 255
	fi
fi


#--------------------
# PUPS/P3 build type
#--------------------

BTYPE=unknown 
if [ -e ~/p3 ] ; then

	if [  "$(ls ~/p3 | grep cluster)" != ""  ] ; then
		BTYPE=cluster
	elif [ "$(ls ~/p3 | grep vanilla)" != "" ] ;  then
                BTYPE=vanilla
	else
		BTYPE=unknown
	fi
fi


#--------------------------
# Upcase if asked to do so
#--------------------------

if [ "$do_upcase" = TRUE ] ; then

	MACHTYPE=$(upcase $MACHTYPE)
	OSTYPE=$(upcase $OSTYPE)
	BTYPE=$(upcase $BTYPE)

fi


#--------------------------------------
# PUPS/P3 archecture identifier string
#--------------------------------------

echo $MACHTYPE.$OSTYPE.$BTYPE 


#------
# Done
#------

exit 0
