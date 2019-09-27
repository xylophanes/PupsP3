#!/bin/tcsh
#
#-----------------------------------------------------------------------------
#  Master uninstall script for P3
#  M.A. O'Neill, (C) Tumbling Dice, 2009-2018
#
#  $1 is the name of logfile /dev/tty implies terminal
#     and /dev/null data sink
#----------------------------------------------------------------------------
#

#--------------------------
# Parse command line items
#--------------------------

if($1 == "null") then
	set logFile = /dev/null
else if($1 == "tty") then
	set logFile = tty
else
	set logFile = $1

        if(! $?cshlevel) then
                setenv cshlevel 1
                echo "" >& $logFile
        endif
endif


#-----------------
# Begin uninstall
#-----------------

pushd pupscore.src 											>& /dev/null
if($logFile == "tty" || $logFile == "/dev/tty") then
	./uninstall_pupsP3.csh  /dev/tty
else if($logFile == "null" || $logFile == "/dev/null") then
	./uninstall_pupsP3.csh /dev/null
else
	./uninstall_pupsP3.csh $logFile
endif
if($status != 0) then
        echo ""                                                                                         >>& $logFile
        echo "ERROR operation failed"                                                                   >>& $logFile
        echo ""                                                                                         >>& $logFile

	exit -1
endif

popd 													>& /dev/null
exit 0

