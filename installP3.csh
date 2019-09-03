#!/bin/tcsh 
#
#----------------------------------------------------------------
#  Master build script for P3
#  M.A. O'Neill, (C) Tumbling Dice, 2009-2018
#
#  $1 is the name of logfile /dev/tty or tty implies terminal
#     and /dev/null or null data sink
#
#  $2 is the type of P3 build (vanilla or cluster)
#  $3 is default for default security build paraemeters
#  $4 set to 'debug' to builds debuggable libraries
#-----------------------------------------------------------------
#

#--------------------------
# Parse command line itmes
#--------------------------

if($# == 0 || $1 == help || $1 == usage || $# == 0) then
	echo "" 
	echo "    PUPS/P3 builder (version 1.07)"
	echo "    (C) M.A. O'Neill, Tumbling Dice, 2018"
	echo ""
	echo "    Usage: buildP3.csh [help | usage] | [logfile] [build type] [force] [default] [debug]"
	echo ""

	exit 0
else if($1 == default || $1 == "" || $1 == "null" || $1 == /dev/null) then
	set logFile = /dev/null
else if($1 == "tty" || $1 == /dev/tty) then
	set logFile = /dev/tty
else
	set logFile = $1

        if(! $?cshlevel) then
                setenv cshlevel 1
		echo ""													>& $logFile
	endif
endif

if($2 == "") then
        set btype = cluster
else if($2 == "vanilla" || $2 == "cluster" || $2 == clean) then
        set btype = $2
else
        echo ""														>>& $logFile
        echo "    buildP3 ERROR: expecting buildtypes 'vanilla' or 'cluster' or 'clean' directive"			>>& $logFile
	echo "    Usage: buildP3 [logfile:/dev/null] [vanilla | cluster:cluster] [force | noforce] [default] [debug]"	>>& $logFile
        echo ""														>>& $logFile

        exit -1
endif

if($3 == "" || $3 == default) then
	set securitymode = default 
else if($3 != "" && $3 != default) then
	echo ""														>>& $logFile
	echo "    buildP3 ERROR: 'default' expected"									>>& $logFile
	echo "    Usage: buildP3 [logfile:/dev/null] [vanilla | cluster:cluster] [default] [debug]"			>>& $logFile
        echo ""														>>& $logFile

        exit -1
endif

if($4 == debug) then
	set debugmode = "debug"
else if($4 == "") then
	set debugmode = ""
else
	echo ""                                                                                         		>>& $logFile
        echo "    buildP3 ERROR: 'debug' expected"                                                      		>>& $logFile
        echo "    Usage: buildP3 [logfile:/dev/null] [vanilla | cluster:cluster] [default] [debug]"     		>>& $logFile
        echo ""                                                                                         		>>& $logFile

        exit -1
endif


#----------------------------
# Begin installation process
#----------------------------

pushd pupscore.src 													>& /dev/null

if($logFile == "tty" || $logFile == "/dev/tty") then

	if($btype == clean) then


                #-----------------------------------
                # Clean up the mess after the build
                #-----------------------------------

		./build_pupsP3.csh clean										>& /dev/tty 
		if($status != 0) then
        		echo ""                                                                               		>>& $logFile
        		echo "ERROR operation failed [build_pupsP3 clean]"                                    		>>& $logFile
        		echo ""                                                                                		>>& $logFile
		
			exit -1
		endif
	else

                #-----------------------------------------
		# Build PUPS/P3 environment in debug mode
		#-----------------------------------------

		./build_pupsP3.csh $btype /dev/tty $force $securitymode $debugmode			
		if($status != 0) then
        		echo ""                                                                                		>>& $logFile
        		echo "ERROR operation failed [build_pupsP3 $btype]"                                    		>>& $logFile
        		echo ""                                                                               		>>& $logFile
		
			exit -1
		endif


                #-----------------------------------
                # Clean up the mess after the build
                #-----------------------------------

		./build_pupsP3.csh clean										>& /dev/tty 
		if($status != 0) then
        		echo ""                                                                               		>>& $logFile
        		echo "ERROR operation failed [build_pupsP3 clean]"                                    		>>& $logFile
        		echo ""                                                                                		>>& $logFile
		
			exit -1
		endif
	endif

else if($logFile == "null" || $logFile == "/dev/null") then

	if($btype == clean) then
		./build_pupsP3.csh clean										>& /dev/null
		if($status != 0) then
        		echo ""                                                                                		>>& $logFile
        		echo "ERROR operation failed [build_pupsP3 clean]"                                     		>>& $logFile
        		echo ""                                                                                		>>& $logFile
		
			exit -1
		endif
	else

                #---------------------------
		# Build PUPS/P3 environment
		#---------------------------

		./build_pupsP3.csh $btype /dev/null $3 $securitymode $debugmode						>& /dev/null
		if($status != 0) then
        		echo ""                                                                                		>>& $logFile
        		echo "ERROR operation failed [build_pupsP3 $btype]"                                    		>>& $logFile
        		echo ""                                                                                		>>& $logFile
		
			exit -1
		endif

                #---------------------------------------
		# Clean up all the junk after the build
		#---------------------------------------

		./build_pupsP3.csh clean										>& /dev/null
		if($status != 0) then
        		echo ""                                                                                		>>& $logFile
        		echo "ERROR operation failed [build_pupsP3 clean]"                                     		>>& $logFile
        		echo ""                                                                                		>>& $logFile
		
			exit -1
		endif
	endif
else

	if($btype == clean) then
		./build_pupsP3.csh clean										>>& $logFile
		if($status != 0) then
        		echo ""                                                                               		>>& $logFile
        		echo "ERROR operation failed build_pupsP3 clean]"                                     		>>& $logFile
        		echo ""                                                                                		>>& $logFile
		
			exit -1
		endif
	else
		./build_pupsP3.csh $btype ../$logFile $3 $securitymode $debugmode					>& /dev/null
		if($status != 0) then
        		echo ""                                                                                		>>& $logFile
        		echo "ERROR operation failed build_pupsP3 $btype]"                                    		>>& $logFile
        		echo ""                                                                                		>>& $logFile
		
			exit -1
		endif
	endif
endif

popd 															>& /dev/null

echo ""															>>& $logFile
echo "    buildP3: finished"												>>& $logFile
echo ""															>>& $logFile

exit 0

