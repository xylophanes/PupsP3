#!/bin/bash
#
#-----------------------------------------------------
# Restart process which has been checkpointed via Criu
# M.A. O'Neill, Tumbling Dice, 2021-2025 
#-----------------------------------------------------


echo ""
echo "    Restart version 2,00"
echo "    (C) Tumbling Dice 2016-2025"
echo ""

#-------------------------
# Parse command tail items
#-------------------------

if [ "$1" = usage ] ||  [ "$1" = help ] ;  then
	echo ""
	echo "    Usage: restart.sh !<checkpoint directory name>!"
	echo ""

	exit 1


#------------------------
# Process restart request
#------------------------

else


	#-------------
	# Sanity check
	#-------------
	
	if [ "$1" == "" ] ; then
		echo "restart.sh ERROR: checkpoint directory not specified"
		echo ""

		exit 255
	fi

	#---------------------------
	# Does checkpoint file exit?
	#---------------------------

	if [ ! -d $1 ] ; then


		#---------------------------------------
		# See if checkpoint is in /tmp directory
		#---------------------------------------

		absolute_path=$(echo $1 | head -c 1)
		if [ "$absolute_path" != "/" ] && [ ! -d /tmp/$1 ] ; then
			echo ""
			echo "restart.sh ERROR: cannot find checkpoint directory: $1"
			echo ""

			exit 255
		else
			checkpoint=/tmp/$1
		fi	


	#---------------------------------------
	# Checkpoint is in current directory
	# or at absolute location in filesystem
	#---------------------------------------

	else

		checkpoint=$1

		echo "    Restarting checkpoint (from directory: $checkpoint)"
		echo ""
	fi	
fi


#-----------------------------
# Restart checkpointed process
#-----------------------------

if [ "$2" == detach ] then
	exec criu --log-file /dev/null --shell-job restore -d -D $checkpoint
else
	exec criu --log-file /dev/null --shell-job restore -D $checkpoint 
fi

exit 0
