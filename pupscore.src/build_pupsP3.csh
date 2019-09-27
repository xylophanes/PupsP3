#!/bin/tcsh 
#
#------------------------------------------------------------------------------
#  Build PUPSP3 libraries and service functions
#  (C) M.A. O'Neill, Tumbling Dice Ltd, 2013-201a9
#
#  $1 is build type
#  $2 is logfile
#  $3 is set to default (if security mode enable and we want default settings)
#  $4 is set to debug (to build debuggable system e.g -g)
#-------------------------------------------------------------------------------
 
#------------------------------------------------------
#  Build in checkpoint support
#------------------------------------------------------

set build_ckpt      = FALSE 


#---------------------------------------------------
#  This can be set to float or double and controls
#  floating point precision in P3 libraries
#---------------------------------------------------

set ftype           = FLOAT

#-----------------------------------------------
#  Set MACHTYPE if it isn't defined (e.g. on
#  Raspberry-Pi running Raspbian)
#-----------------------------------------------

set usable_machtype = `echo $MACHTYPE | grep unknown`
if($usable_machtype != "") then
	set MACHTYPE = `uname -a | awk '{print $12}'`
endif

#-------------------------------------------------------
# Correct "namecreep" in 32 bit Intel/AMD architectures
#-------------------------------------------------------

if($MACHTYPE == i686) then
	set MACHTYPE = i386
endif

#-----------------------------------------------
# Make sure we use the right configuration file
#-----------------------------------------------

rm -f ../config/$OSTYPE.cluster                                                                 				>& /dev/null
rm -f ../config/linux.cluster													>& /dev/null
rm -f ../config.install/$OSTYPE.cluster                                                         				>& /dev/null
rm -f ../config.install/linux.cluster												>& /dev/null
rm -f ../config.debug/$OSTYPE.cluster                                                           				>& /dev/null
rm -f ../config.debug/linux.cluster                                                             				>& /dev/null

onintr abort_build


#-----------------------------
# Stop complaints from loader
#-----------------------------

setenv LD_LIBRARY_PATH


#-----------------------------------------------------------------------------------
# Initialise logfile
#-----------------------------------------------------------------------------------

if($# == 0 || $1 == help || $1 == usage || $# == 0) then
	echo ""
	echo "    P3-2019 (SUP3) build script (C) M.A. O'Neill, Tumbling Dice Ltd 2019"
	echo ""
        echo "    Usage: build_pupsP3 [usage] | [clean] | [vanilla | cluster] [tty | logfile]"
        echo "                        [default | nodefault] [debug]"

	echo ""

	exit 0
else if ($1 == clean) then

	echo ""
	echo "    ... cleaning build tree"
	echo ""	

	\rm -f  *.a														>& /dev/null
	\rm -f  *.o														>& /dev/null
	\rm -f  *.so														>& /dev/null
	\rm -rf ../lib*														>& /dev/null
	\rm -rf ../bin*														>& /dev/null
	\rm -f ../config/*linux.cluster												>& /dev/null
	\rm -f ../config.debug/*linux.cluster											>& /dev/null
	\rm -f ../config.install/*linux.cluster											>& /dev/null
	\rm -f BUILDLOCK													>& /dev/null 
	\rm -f Makefile														>& /dev/null

	exit 0
else if($2 == default || $2 == "" || $2 == null || $2 == /dev/null) then
        set logFile = /dev/null
else if($2 == "tty" || $2 == /dev/tty) then
        set logFile = /dev/tty
else
        set logFile = $2


        if(! $?cshlevel) then
                setenv cshlevel 1
                echo ""														>& $logFile
        endif
endif


#-----------------------------------------------------------
# Make sure we pick up the correct configuration directory
#-----------------------------------------------------------

if($4 == debug) then
	set debug = TRUE
else if($4 != "") then
	echo ""															>>& $logFile
	echo "    ERROR: 'debug' expected"											>>& $logFile
	echo ""

	exit -1
else
	set debug = FALSE
endif

if($debug == FALSE) then
        setenv P3_CONFDIR   ../config                                                           				>& /dev/null
        setenv PUPS_CONFDIR ../config                                                           				>& /dev/null

        set confdir = ../config
else
        setenv P3_CONFDIR   ../config.debug                                                     				>& /dev/null
        setenv PUPS_CONFDIR ../config.debug                                                     				>& /dev/null

        set confdir = ../config.debug
endif


#----------------------------------------------------------------------
# Set up configuration links - we also set the default floating point
# precision here
#----------------------------------------------------------------------

if($build_ckpt == TRUE) then


	#-------------------------------
	# Build with checkpoint support
	#-------------------------------

	pushd ../config														>& /dev/null
	sed s/FTYPE/$ftype/g <$OSTYPE.cluster.ckpt >linux.cluster
	popd															>& /dev/null

	if($debug == FALSE) then
        	pushd ../config.install                                                         				>& /dev/null
		sed s/FTYPE/$ftype/g <$OSTYPE.cluster.ckpt >linux.cluster
        	popd                                                                            				>& /dev/null
	else
                pushd ../config.debug                                                           				>& /dev/null
                sed s/FTYPE/$ftype/g <$OSTYPE.cluster.ckpt >linux.cluster
                popd                                                                            				>& /dev/null
        endif
else

	#----------------------------------
	# Build without checkpoint support
	#----------------------------------

	pushd ../config														>& /dev/null
	sed s/FTYPE/$ftype/g <$OSTYPE.cluster.nockpt >linux.cluster
	popd															>& /dev/null

	if($debug == FALSE) then
        	pushd ../config.install                                                         				>& /dev/null

		sed s/FTYPE/$ftype/g <$OSTYPE.cluster.nockpt >linux.cluster
        	popd                                                                            				>& /dev/null
	else
        	pushd ../config.debug                                                           				>& /dev/null
		sed s/FTYPE/$ftype/g <$OSTYPE.cluster.nockpt >linux.cluster
        	popd                                                                            				>& /dev/null
	endif
endif


#-------------------------------
# Get number of processor cores
#-------------------------------

set nCores = `lscpu | grep "CPU(s):" | head -1 | awk '{print $2}'`


echo ""																>>& $logFile
echo "    P3-2019 (SUP3) build script (C) M.A. O'Neill, Tumbling Dice Ltd 2019"							>>& $logFile

if($debug == TRUE) then
	echo "    Configuring P3-2019 for "$MACHTYPE.$OSTYPE "host (debug mode)"						>>& $logFile
	echo "    $nCores processor cores found"										>>& $logFile
else
	echo "    Configuring P3-2019 for "$MACHTYPE.$OSTYPE "host"								>>& $logFile
	echo "    $nCores processor cores found"										>>& $logFile
endif

if($ftype == FLOAT) then
	echo "    Floating point precision for PUPS/P3 is 'float' (4 bytes)"							>>& $logFile
else
	echo "    Floating point precision for PUPS/P3 is 'double' (8 bytes)"							>>& $logFile
endif

echo ""																>>& $logFile

if($build_ckpt == TRUE) then
        echo "    Checkpointing support enabled"                                                				>>& $logFile
endif



#----------------------------------------------------------------------------------
#  Set security mode
#----------------------------------------------------------------------------------

if($3 == default) then
	set securitymode = default
else if($3 != "" && $3 != default && $3 != nodefault) then
	echo "    ERROR build_pupsP3: expecting 'default'"									>>& $logFile
        echo "    Usage: build_pupsP3 [usage] | [clean] | [[vanilla | cluster] [tty | logfile]"					>>& $logFile
        echo "                        [default | nodefault] [debug]"								>>& $logFile

	exit -1
else 
	set securitymode = ""
endif


#---------------------------------------
# Distribution installation directories
#---------------------------------------

if(`whoami` == "root") then
	# Root is special
        set PROJECTDIR    = /p3
	set BINDIR	  = /usr/bin


	#-----------------------------------------
	# Test for x86_64 architecture and adjust
	# LIBDIR if found
	#-----------------------------------------

	if(`uname -a | awk '{print $12}'` == x86_64) then
		set LIBDIR  = /usr/lib64
	else
		set LIBDIR  = /usr/lib
	endif

        set INCDIR        = /usr/include
	set HDRDIR	  = /usr/include/p3
	set MANDIR	  = /usr/share/man
	set projectPath   = \"\/p3\" 
else
	set PROJECTDIR    = ~/p3
	set BINDIR        = ~/bin
	set LIBDIR        = ~/lib
        set INCDIR        = ~/include
	set HDRDIR	  = ~/include/p3
	set MANDIR        = ~/man
	set projectPath   = \"~/p3\"
endif


#---------------------------------------------------
# We need to make any directories which we may need
#---------------------------------------------------

if(! -e $PROJECTDIR) then
        echo ""															>>& $logFile
        echo "    ... Creating '$PROJECTDIR' (PUPS/P3 project templates directory)"						>>& $logFile
        echo ""															>>& $logFile

        mkdir $PROJECTDIR													>& /dev/null
	endif
endif

if(! -e $BINDIR) then
	echo ""															>>& $logFile
	echo "    ... Creating '$BINDIR' (PUPS/P3 binaries directory)"								>>& $logFile
	echo ""															>>& $logFile

	mkdir $BINDIR														>& /dev/null
	endif
endif

if(! -e $LIBDIR) then
        echo ""															>>& $logFile
        echo "    ... Creating '$LIBDIR' (PUPS/P3 library directory)"								>>& $logFile
        echo ""															>>& $logFile

        mkdir $LIBDIR														>& /dev/null
	endif
endif

if(! -e $HDRDIR) then
        echo ""															>>& $logFile
        echo "    ... Creating '$HDRDIR' (PUPS/P3 header directory)"								>>& $logFile
        echo ""															>>& $logFile

        mkdir $INCDIR														>& /dev/null
       	mkdir $HDRDIR														>& /dev/null
	endif
endif

if(! -e $MANDIR) then
        echo ""															>>& $logFile
        echo "    ... Creating '$MANDIR' (PUPS/P3 manual directory)"								>>& $logFile
        echo ""															>>& $logFile

        mkdir $MANDIR														>& /dev/null
	mkdir $MANDIR/man1													>& /dev/null
	mkdir $MANDIR/man3													>& /dev/null
	mkdir $MANDIR/man7													>& /dev/null
endif


#---------------------------------------------------------------
# If we haven't got .cshrc and/or .tcshrc for this user install
# them now
#---------------------------------------------------------------

if(! -e ~/.cshrc) then
	echo ""															>>& $logFile
	echo "Setting up P3M environment (for csh)"										>>& $logFile
	echo ""															>>& $logFile

	\cp p3m_cshrc ~/.cshrc													>& /dev/null
endif

if(! -e ~/.tcshrc) then
	echo ""															>>& $logFile
	echo "Setting up P3M environment (for tcsh)"										>>& $logFile
	echo ""															>>& $logFile

	\cp p3m_tcshrc ~/.tcshrc												>& /dev/null
endif

gcc -w -Wfatal-errors -I../include.libs -O3 -o iscard iscard.c									>>& $logFile
if($status != 0) then
	echo ""															>>& $logFile
	echo "ERROR operation failed [gcc iscard]"										>>& $logFile
	echo ""															>>& $logFile

	exit -1
endif

gcc -w -Wfatal-errors -I../include.libs -O3 -o ishex ishex.c  									>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "ERROR operation failed [gcc ishex]"                                                      				>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit -1
endif

gcc -w -Wfatal-errors -I../include.libs -O3 -o isint isint.c  									>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "ERROR operation failed [gcc isint]"                                                      				>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit -1
endif

gcc -I../include.libs -DFLOAT -w -Wfatal-errors -O3 -o isfloat isfloat.c  							>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "ERROR operation failed [gcc isfloat]"                                                    				>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit -1
endif

gcc -w -DBSD_FUNCTION_SUPPORT -Wfatal-errors -O3 -o tas tas.c 									>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "ERROR operation failed gcc tas]"                                                        				>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit -1
endif

gcc -w -Wfatal-errors -I../include.libs -DBSD_FUNCTION_SUPPORT -DHAVE_READLINE -O3 -o ask ask.c -lreadline -ltermcap		>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "ERROR operation failed [gcc ask]"                                                        				>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit -1
endif

gcc -w -Wfatal-errors -O3 -o add add.c												>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "ERROR operation failed [gcc add]"                                                        				>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit -1
endif

gcc -I../include.libs -DFLOAT -w -Wfatal-errors -O3 -o fadd fadd.c                              				>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "ERROR operation failed [gcc fadd]"                                                       				>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit -1
endif

gcc -w -Wfatal-errors -O3 -o sub sub.c                                                          				>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "ERROR operation failed [gcc sub]"                                                       				>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit -1
endif

gcc -I../include.libs -DFLOAT -w -Wfatal-errors -O3 -o fsub fsub.c                              				>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "ERROR operation failed gcc fsub]"                                                        				>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit -1
endif

gcc -w -Wfatal-errors -O3 -o mul mul.c                                                          				>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "ERROR operation failed [gcc mul]"                                                       				>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit -1
endif

gcc -I../include.libs -DFLOAT -w -Wfatal-errors -O3 -o fmul fmul.c                              				>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "ERROR operation failed [gcc fmul]"                                                       				>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit -1
endif

gcc -w -Wfatal-errors -O3 -o div div.c                                                          				>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "ERROR operation failed [gcc div]"                                                        				>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit -1
endif

gcc -I../include.libs -DFLOAT -w -Wfatal-errors -O3 -o fdiv fdiv.c                              				>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "ERROR operation failed [gcc fdiv]"                                                       				>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit -1
endif

gcc -I../include.libs -DFLOAT -w -Wfatal-errors -O3 -o fint fint.c -lm                          				>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "ERROR operation failed [gcc fint]"                                                       				>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit -1
endif

gcc -I../include.libs -DFLOAT -w -Wfatal-errors -O3 -o intf intf.c                              				>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "ERROR operation failed [gcc intf]"                                                       				>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit -1
endif

strip tas iscard ishex isint isfloat ask add sub mul div fadd fsub fmul fdiv							>& /dev/null
\mv tas iscard ishex isint isfloat ask add sub mul div fadd fsub fmul fdiv fint intf $BINDIR					>& /dev/null
if($status != 0) then
	echo ""                                                                                		 			>>& $logFile
	echo "ERROR operation failed [install PUPS/P3 build tools"								>>& $logFile
	echo ""															>>& $logFile
        exit -1
endif

source ~/.cshrc															>& /dev/null
pushd ..															>& /dev/null
set path=($path . .. `pwd`)
popd																>& /dev/null

if(-e BUILDLOCK) then

ask_again:

	set ret = `ask "    buildlock exists  -- delete?"`									>& /dev/tty
	if($ret == "y" || $ret == "yes") then
		\rmdir BUILDLOCK												>& /dev/null
	else if($ret == "q" || $ret == "quit" || $ret == "bye") then
		\rmdir BUILDLOCK        											>& /dev/null
		exit -1
	endif

	if($ret != "yes" && $ret != "y") then
		goto ask_again
	endif
endif

tas BUILDLOCK															>& /dev/null

echo ""																>>& $logFile
echo "    ... Building compilation tools (upcase, downcase, prefix, suffix, striplc, error)"					>>& $logFile
echo ""																>>& $logFile

gcc -w -Wfatal-errors -I../include.libs  -O3 -o suffix  suffix.c     								>>& $logFile
if($status != 0) then
        echo ""                                              		                                                   	>>& $logFile
        echo "ERROR operation failed [gcc suffix]"                                                              		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit -1
endif

gcc -w -Wfatal-errors -I../include.libs -DBSD_FUNCTION_SUPPORT -O3 -o prefix  prefix.c     					>>& $logFile
if($status != 0) then
        exit -1
endif

gcc -w -Wfatal-errors -O3 -o striplc striplc.c    										>>& $logFile
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "ERROR operation failed [gcc stripl]"                                                              		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit -1
endif

gcc -w -Wfatal-errors -O3 -o downcase downcase.c  										>>& $logFile
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "ERROR operation failed [gcc downcase]"                                                            		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit -1
endif

gcc -w -Wfatal-errors -O3 -o upcase upcase.c      										>>& $logFile
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "ERROR operation failed [gcc upcase]"                                                              		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit -1
endif

gcc -w -Wfatal-errors -O3 -o error error.c      										>>& $logFile
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "ERROR operation failed [gccc errpr]"                                                              		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit -1
endif


strip suffix prefix striplc downcase upcase error										>& /dev/null
\mv suffix prefix striplc downcase upcase error $BINDIR 									>& /dev/null
if($status != 0) then
	echo ""                                                                                		 			>>& $logFile
	echo "ERROR operation failed [install PUPS/P3 trivial support tools"							>>& $logFile
	echo ""															>>& $logFile
        exit -1
endif

rehash

set rawtarget = $MACHTYPE.$OSTYPE 
set target    = $OSTYPE 
set utarget   = `upcase $target`
set target    = `downcase $target`
set arch      = $MACHTYPE 
set arch      = `upcase $arch`

echo ""																>>& $logFile
echo "    Configuring P3-2019 for "$rawtarget "host"										>>& $logFile
echo ""																>>& $logFile

if($# == 0) then
	set buildtype = cluster

	echo ""															>>& $logFile	
	echo "    ... Build type is 'cluster' (default)"									>>& $logFile
	echo ""															>>& $logFile
else if($1 == "usage" || $1 == "--usage") then
	echo "    ERROR build_pupsP3: expecting 'default'"i									>>& $logFile
        echo "    Usage: build_pupsP3 [usage] | [clean] | [[vanilla | cluster] [tty | logfile] [default]"			>>& $logFile
	echo "                        [default | nodefault] [debug]"								>>& $logFile
        echo ""															>>& $logFile

        exit 0 
else if ($1 != "vanilla" && $1 != "cluster") then
	echo "    ERROR build_pupsP3: expecting 'default'"									>>& $logFile
        echo "    Usage: build_pupsP3 [usage] | [clean] | [[vanilla | cluster]  [tty | logfile] [default]"			>>& $logFile
	echo "                        [default | nodefault] [debug]"								>>& $logFile
        echo ""

	\rmdir BUILDLOCK        												>& /dev/null
        exit -1 
else
	set buildtype = $1

	echo ""															>>& $logFile
	echo "    ... Build type is '$1'"											>>& $logFile
	echo ""															>>& $logFile
endif

echo ""
if(! -e pupsuname) then
        echo "    ERROR build_pupsP3: cannot determine P3 architecture (no [pupsuname] file present)"				>>& $logFile

	\rmdir BUILDLOCK													>& /dev/null
        exit -1
endif

if(! -e ../include.libs) then
	echo "    ERROR build_pupsP3: header file directory [../include.libs] missing"						>>& $logFile

	\rmdir BUILDLOCK													>& /dev/null
	exit -1
endif

if(! -e $confdir) then
	echo "    ERROR build_pupsP3: configuration file directory [$confdir] missing"						>>& $logFile

	\rmdir BUILDLOCK													>& /dev/null
	exit -1
endif

if($target == osf1 || $target == darwin) then
   set crypt = ""
else
   set crypt = "-lcrypt"
endif

if($target == darwin) then
	set nis    = ""
	set AUTHEN = ""
else if($target == osf1) then
	set nis    = ""
        set AUTHEN = ""
else
	set nis    = "-lnss_nis"
	set AUTHEN = "-DNIS_SUPPORT -DSHADOW_SUPPORT"

	echo ""															>>& $logFile
	echo "    Authentication method support ($AUTHEN)"									>>& $logFile
	echo ""															>>& $logFile
endif

echo ""																>>& $logFile
echo "    ... Making P3-2019 binary directories for libraries and service functions"						>>& $logFile
echo ""																>>& $logFile

set pctarget = $target
set mode     = INVALID

if($buildtype == vanilla) then
   echo "    ... Building basic P3/PSRP for standalone host"									>>& $logFile
   echo ""															>>& $logFile
   set dummy_shgmalloc = need_dummy_shgmalloc
endif

if($buildtype == cluster) then
   echo "    Dynamic link library shared memory, checkpointing  and network support enabled"					>>& $logFile
   echo "    ... Building dynamic P3/PSRP for network clustered MIMD environment"						>>& $logFile
   echo ""															>>& $logFile
   set pctarget = $target.cluster
   set dummy_shgmalloc = none
endif

pushd ..															>& /dev/null

\rm -rf bin lib															>& /dev/null
if(! -e lib.$arch.$pctarget) then
	mkdir lib.$arch.$pctarget 												>& /dev/null
endif
ln -s lib.$arch.$pctarget lib 													>& /dev/null

if(! -e bin.$arch.$pctarget) then
	mkdir bin.$arch.$pctarget												>& /dev/null
endif
ln -s bin.$arch.$pctarget bin													>& /dev/null

if($buildtype != "") then
   ls -s btype $buildtype													>& /dev/null
endif

if(-e lib.$arch.$pctarget && `ls -l lib.$arch.$pctarget | grep "total 0"` == "") then
	\rm -f lib.$arch.$pctarget/*												>& /dev/null
endif

if(-e bin.$arch.$pctarget && `ls -l bin.$arch.$pctarget | grep "total 0"` == "") then
	\rm -f bin.$arch.$pctarget/*												>& /dev/null
endif

popd																>& /dev/null


#-------------------------------------
# Are we in secure installation mode?
#-------------------------------------

if(`grep "\#-DSECURE" <$confdir/$target.$buildtype` != "") then
        set install_mode = insecure
else
        set install_mode = secure
endif

set method = default
if($install_mode == secure) then

	echo ""															>>& $logFile	
        echo "    -----------------------------------------------------------------------------------------"			>>& $logFile
	echo "    You have specified secure binaries in your configuration file -- so you will need to"				>>& $logFile
	echo "    specify a licencing method (sdongle or dserial) under $OSTYPE."						>>& $logFile
        echo "    -----------------------------------------------------------------------------------------"			>>& $logFile
	echo ""															>>& $logFile

	if($securitymode == default) then
		set method = sdongle
	else

ask_method_again:
        	set method = `ask "    Licensing method (sdongle or dserial) [default: sdongle]"`

		if($method == "bye" || $method == "quit" || $method == "q") then
			\rmdir BUILDLOCK											>& /dev/null
       	         	exit -1
		endif

		if($method != "" && $method != "sdongle" && $method != "dserial") then
			echo ""													>>& $logFile
			echo "    '$method' is not a valid licensing method ('sdongle' or 'dserial' expected)"			>>& $logFile
			echo ""													>>& $logFile

			goto ask_method_again
		endif
	endif

        if($method == dserial) then
                echo ""														>>& $logFile
                echo "    Licence method is disk serial"									>>& $logFile
		echo ""														>>& $logFile
		echo "    ... Building hard disk device serial number extraction tool"						>>& $logFile
		echo ""														>>& $logFile
		gcc -w -DBSD_FUNCTION_SUPPORT -Wfatal-errors -DLINUX -I../include.libs -O3 -o hdid hdid.c			>>& $logFile
		if($status != 0) then
        	   echo ""                                                                                     			>>& $logFile
        	   echo "ERROR operation failed [gcc hdid"                                                      		>>& $logFile
        	   echo ""                                                                                      		>>& $logFile

        	   exit -1
		endif
		\mv hdid $BINDIR                										>& /dev/null

               	set default_hd_device = `df | grep boot | awk '{print $1}' | striplc | sed s/"\/dev\/"//g`
               	set hd_device = `ask "hd_device [default  $default_hd_device]"`
               	if($hd_device == "q" || $hd_device == "quit" || $hd_device == "bye") then
			\rmdir BUILDLOCK	>& /dev/null
	         	exit -1
 	        else if($hd_device == "" || $hd_device == "default") then
              		set hd_device = $default_hd_device
                       	echo "    Hard disk device is" $hd_device "(default)"							>>& $logFile
               	endif

               	set seed = `ask "    encryption seed [default 9999]"`
               	if($seed == "q" || $seed == "quit" || $seed == "bye") then
			\rmdir BUILDLOCK        >& /dev/null
		  	exit -1
               	else if($seed == "" || $seed == "default") then
			set seed = 9999
                       	echo "Encryption seed is" $seed "(default)"								>>& $logFile
               	endif

		set dserial = `ask "    disk serial [default use build disk serial]"`
                if("$dserial" == "q" || "$dserial" == "quit" || "$dserial" == "bye") then
			\rmdir BUILDLOCK	>& /dev/null
                        exit -1
		endif

		if("$dserial" != "" || "$dserial" == "default") then
			setenv USE_DISK_SERIAL "$dserial"
		else
			echo ""													>>& $logFile
                        echo "    -----------------------------------------------------------------------------------------"	>>& $logFile
                	echo "    You must change permissions of installation disk device (a+r)"				>>& $logFile
                        echo "    -----------------------------------------------------------------------------------------"	>>& $logFile
			echo ""													>>& $logFile

get_passwd:
                	if(`whoami` != root) then
                        	echo "    (Root) password: "
                        	echo ""

                        	if(`su -c "chmod a+r /dev/$hd_device" |& awk '{print $2}'` == incorrect) then
					echo ""
                                	echo "    not authenticated"
					echo ""

					goto get_passwd
                	        endif
                	else
                        	echo "    authenticated"									
                	endif
		endif

                sed s/WHICH_DEVICE/$hd_device/g <../include.libs/securicor.h  | sed s/SVAL/$seed/g >../include.libs/sed_securicor.h
        else if($method == "" || $method == sdongle || $method == default) then
                echo ""														>>& $logFile
                echo "    Licence method is soft dongle"									>>& $logFile

                set dongle_dir = ~`whoami`/.sdongles
		if(! -e $dongle_dir) then
			mkdir $dongle_dir											>& /dev/null
		endif
		
                echo ""														>>& $logFile
                echo "    ... Building security tools (sdongle)"								>>& $logFile
                echo ""														>>& $logFile

                gcc -w -Wfatal-errors -I../include.libs -O3 -o sdongle sdongle.c  						>>& $logFile
		if($status != 0) then
		        echo ""                                                                                                 >>& $logFile
        		echo "ERROR operation failed [gcc sdongle]"                                                             >>& $logFile
        		echo ""                                                                                                 >>& $logFile

        		exit -1
		endif
                \mv sdongle $dongle_dir/sdongle 										>& /dev/null
		if($status != 0) then
		        echo ""                                                                                                 >>& $logFile
        		echo "ERROR operation failed [install sdongle]"                                                         >>& $logFile
        		echo ""                                                                                                 >>& $logFile

		        exit -1
		endif

		set is_key = NO
		if($securitymode == default) then
			set dongle_file_name = pups.dongle
		else
                	set dongle_file_name = `ask "    dongle file name (default pups.dongle)"`
                	if($dongle_file_name == "q" || $dongle_file_name == "quit" || $dongle_file_name == "bye") then
				\rmdir BUILDLOCK	>& /dev/null
                        	exit -1
                	else if($dongle_file_name == "") then
				set dongle_file_name = pups.dongle
			else if(`ishex $dongle_file_name | grep TRUE` != "") then
				set is_key = YES
                	endif
		endif

		if($securitymode == default) then
			set seed = 9999
		else 	
                	set seed = `ask "    encryption seed [default 9999]"`
                	if($seed == "q" || $seed == "quit" || $seed == "bye") then
				\rmdir BUILDLOCK	>& /dev/null
                        	exit -1
			endif

	                if($seed == "") then
				set seed = 9999
                        	echo "    Encryption seed is" $seed "(default)"							>>& $logFile
			endif
                endif

		pushd $dongle_dir												>& /dev/null
		echo ""														>>& $logFile
		echo "    Creating dongle file in  `pwd`"									>>& $logFile
		echo ""														>>& $logFile
                \rm pups.dongle		>& /dev/null

		if($is_key == "NO") then
                	sdongle  pups.dongle	>& /dev/null
                	setenv USE_SOFT_DONGLE $dongle_dir/pups.dongle 
			echo ""													>>& $logFile
                        echo "    -----------------------------------------------------------------------------------------"	>>& $logFile
                	echo "    You need to setenv USE_SOFT_DONGLE $dongle_dir/pups.dongle" 					>>& $logFile
                        echo "    -----------------------------------------------------------------------------------------"	>>& $logFile
			echo ""
		else
			echo ""													>>& $logFile
                        echo "    -----------------------------------------------------------------------------------------"	>>& $logFile
                	echo "    You need to setenv USE_SOFT_DONGLE appopriately (on host holding dongle)" 			>>& $logFile
                        echo "    -----------------------------------------------------------------------------------------"	>>& $logFile
			echo ""													>>& $logFile
		endif

		chmod a+r *dongle*												>& /dev/null
		popd 														>& /dev/null

                sleep 1
                sed s/WHICH_DEVICE/dummy/g <../include.libs/securicor.h | sed s/SVAL/$seed/g >../include.libs/sed_securicor.h
        endif
endif

echo ""																>>& $logFile
echo "    ... Building (enigma) encrypting tool (ecrypt)"									>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -I../include.libs -O3 -o ecrypt ecrypt.c									>>& $logFile
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "ERROR operation failed [gcc ecrypt]"                                                                    		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit -1
endif

strip ecrypt
\mv ecrypt $BINDIR 														>& /dev/null
if($status != 0) then
        echo ""    		                                                                               			>>& $logFile
        echo "ERROR operation failed [install ecrypt]"                                                                 		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit -1
endif


echo ""																>>& $logFile
echo "    ... Compiling P3-2019 configure and binary time stamping utilities"							>>& $logFile
echo ""																>>& $logFile

set target = `upcase $target`
if($install_mode == secure) then
	gcc -w -Wfatal-errors -DBSD_FUNCTION_SUPPORT -DDEFAULT_CONFIGDIR=$projectPath -DSECURE -I../include.libs -D$target \
												  -O3 -o pupsconf pupsconf.c	>>& $logFile
else
	gcc -w -Wfatal-errors -DBSD_FUNCTION_SUPPORT -DDEFAULT_CONFIGDIR=$projectPath -I../include.libs -D$target          \
												  -O3 -o pupsconf pupsconf.c  	>>& $logFile
endif
if($status != 0) then
        echo ""                         		                                                                        >>& $logFile
        echo "ERROR operation failed [gcc pupsconf]"       		                                                        >>& $logFile
        echo ""                                                        			                                        >>& $logFile

        exit -1
endif

\rm $BINDIR/configure														>& /dev/null
sed "s|CONFIG_PATH|$PROJECTDIR|g; s|INC_PATH|$INCDIR|g" <configure.in >$$
\mv $$ configure														>& /dev/null

strip pupsconf															>& /dev/null
\mv pupsconf $BINDIR 														>& /dev/null
if($status != 0) then
        echo ""                         		                                                                        >>& $logFile
        echo "ERROR operation failed [install pupsconf]"       		                                                        >>& $logFile
        echo ""                                                        			                                        >>& $logFile

        exit -1
endif

gcc -w -Wfatal-errors -D$target -I../include.libs -O3 -o vstamp vstamp.c							>>& $logFile
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "ERROR operation failed [gcc vstamp]"                                                                     		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit -1
endif

strip vstamp															>& /dev/null
\mv vstamp $BINDIR 														>& /dev/null
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "ERROR operation failed [install vstamp]"                                                                     		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit -1
endif

gcc -w -Wfatal-errors -D$target -I../include.libs -O3 -o vtagup vtagup.c  							>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [gcc vtagup]"                                                                                   >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

strip vtagup															>& /dev/null
\mv vtagup $BINDIR 														>& /dev/null
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "ERROR operation failed [install vtagup]"                                                                		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit -1
endif

if($build_ckpt == TRUE) then
	echo ""															>>& $logFile
	echo "    ... Set version number for state save (Criu) checkpoints"                                                     >>& $logFile
	echo ""															>>& $logFile
	vtagup SSAVETAG utilib.c                                                                                                >& /dev/null
endif

echo ""																>>& $logFile
echo "    ... Building P3 Dynamic C [D] to ANSI C translator (language level support for dynamic functions and threads)"	>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -DBSD_FUNCTION_SUPPORT -I. -I../include.libs -D$target -D$arch  -O3 -o pc2c pc2c.c			>>& $logFile
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "ERROR operation failed [gcc pc2c"                                                                        		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit -1
endif

strip pc2c 
\mv pc2c $BINDIR 
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "ERROR operation failed [install pc2c]"                                                                   		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit -1
endif


echo ""																>>& $logFile
echo "    ... Building P3 skeleton application generator (generates P3 application source frameworks)"				>>& $logFile
echo ""																>>& $logFile

gcc  -w -Wfatal-errors -DBSD_FUNCTION_SUPPORT -DDEFAULT_CONFIGDIR=$projectPath -I. -I../include.libs -D$target -D$arch \
												  -O3 -o appgen appgen.c	>>& $logFile
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "ERROR operation failed [gcc appgen]"                                                                    		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit -1
endif

strip appgen 															>& /dev/null
\mv appgen $BINDIR 														>& /dev/null
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "ERROR operation failed [install appgeb]"                                                                 		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit -1
endif


echo ""																>>& $logFile
echo "    ... Building P3 skeleton library generator (generates P3 library source frameworks)"					>>& $logFile
echo ""																>>& $logFile

gcc  -w -Wfatal-errors -DBSD_FUNCTION_SUPPORT -DDEFAULT_CONFIGDIR=$projectPath -I. -I../include.libs -D$target \
										  -O3 -D$arch -o libgen libgen.c		>>& $logFile
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "ERROR operation failed [gcc libgen]"                                                                    		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit -1
endif

strip libgen															>& /dev/null
\mv libgen $BINDIR 														>& /dev/null
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "ERROR operation failed [install libgen]"                                                                 		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit -1
endif


echo ""
echo "    ... Building P3 skeleton application DLL generator (generates P3 DLL source frameworks)"				>>& $logFile
echo ""

gcc  -w -Wfatal-errors -DBSD_FUNCTION_SUPPORT -DDEFAULT_CONFIGDIR=$projectPath -I. -I../include.libs -D$target -D$arch \
												  -O3 -o dllgen dllgen.c	>>& $logFile
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "ERROR operation failed [gcc dllgen]"                                                                     		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit -1
endif

strip dllgen 															>& /dev/null
\mv dllgen $BINDIR 														>& /dev/null
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "ERROR operation failed [install dllgen]"                                                                 		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit -1
endif


sed s/\$ARCH/$arch/g <$confdir/$pctarget >$confdir/$arch.$pctarget

if($buildtype == cluster) then
    echo ""															>>& $logFile
    echo "    ... Building P3 dynamic bubble memory allocation library"								>>& $logFile
    echo ""															>>& $logFile

echo ARCH $arch.$pctarget

    $BINDIR/pupsconf $arch.$pctarget Make_bubble.in Makefile									#>&  /dev/null 
    if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "ERROR operation failed [generate bubble Makefile]"                                                     		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

    	exit -1
    endif

    make -j$nCores 														>>& $logFile
    if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "ERROR operation failed [make bubble memory allocator]"                                                  		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit -1
    endif

    make clean 															>>& $logFile
    echo ""															>>& $logFile
    echo ""															>>& $logFile
endif

if($buildtype == cluster) then

    echo "    Network support enabled (Building P3/PSRP for hetrogenous network MIMD environment)"				>>& $logFile
    echo ""															>>& $logFile
    echo "" 															>>& $logFile

    echo ""															>>& $logFile
    echo "    ... Building P3 HUP to TERM signal relay (relays HUP as TERM to payload process)"					>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o hupter hupter.c						>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc hupter]"                                                                              >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip hupter														>& /dev/null

    echo ""
    echo "    ... Building P3 network signal distributor [nkill] (sends signals to PID or process name on [remote] host)"	>>& $logFile
    echo ""

    gcc -w -Wfatal-errors -DBSD_FUNCTION_SUPPORT -D$utarget -D$arch $AUTHEN -DSSH_SUPPORT -DHAVE_PROC_FS -O3 -I.        \
									   -I../include.libs -o nkill nkill.c $crypt $nis  	>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc nkill]"                                                                               >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip nkill															>& /dev/null

    set target = `downcase $target`

    echo ""															>>& $logFile
    echo "    ... Building P3 text file stripper [stripper] (strips comments from files in VDM's)"				>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o stripper stripper.c 					>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc stripper]"                                                                            >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip stripper														>& /dev/null

    echo ""															>>& $logFile
    echo "    ... Building p3f (checks if process is P3 aware)"									>>& $logFile
    echo ""															>>& $logFile

    gcc -w -Wfatal-errors -DBSD_FUNCTION_SUPPORT -D$utarget -D$arch -DSSH_SUPPORT -DHAVE_PROC_FS -O3 -I.              \
									     -I../include.libs -o p3f p3f.c $crypt $nis  	>>& $logFile
    if($status != 0) then
	echo ""                                                                                                                 >>& $logFile
	echo "ERROR operation failed [gcc p3f]"                                                                                 >>& $logFile
	echo ""                                                                                                                 >>& $logFile

	exit -1
    endif

    strip p3f					   	 									>& /dev/null

    set target = `downcase $target`

    echo ""															>>& $logFile
    echo "    ... Building P3 lightweight file homeostat [lyosome] (protects a file for specified time period)"			>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -DBSD_FUNCTION_SUPPORT -D$utarget -D$arch -O3 -I../include.libs -o lyosome lyosome.c			>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc lyosome]"                                                                             >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip lyosome														>& /dev/null

    echo ""															>>& $logFile
    echo "    ... Building P3 lightweight garbage collector [kepher] (removes stale temporary files)"				>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -DBSD_FUNCTION_SUPPORT -D$utarget -D$arch -O3 -I../include.libs -o kepher kepher.c			>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc kepher]"                                                                              >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip kepher                      												>& /dev/null

    echo ""															>>& $logFile
    echo "    ... Building P3 sub-path extractor [leaf] (extracts twigs from pathname branches)"				>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -DBSD_FUNCTION_SUPPORT -D$utarget -D$arch -O3 -I../include.libs -o leaf leaf.c     			>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc leaf]"                                                                                >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip leaf                        												>& /dev/null

    echo ""															>>& $logFile
    echo "    ... Building psrptool (starts psrp client process in new [vte] window)"						>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -DBSD_FUNCTION_SUPPORT -D$utarget -D$arch -O3 -I../include.libs -o psrptool psrptool.c 		>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed gcc psrptool]"                                                                             >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip psrptool														>& /dev/null

    echo ""															>>& $logFile
    echo "    ... Building servertool (starts psrp server process in new [xterm] window)"					>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -DBSD_FUNCTION_SUPPORT -D$utarget -D$arch -O3 -I../include.libs -o servertool servertool.c		>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc servertool]"                                                                          >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip servertool														>& /dev/null

    echo ""															>>& $logFile
    echo "    ... Building P3 farm (permits definition of network-wide [homogenous] process farms)"			        >>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -DBSD_FUNCTION_SUPPORT -D$utarget -D$arch -DSSH_SUPPORT -O3 -I../include.libs -o farm farm.c $crypt	>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc farm]"                                                                                >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip farm	   													        >& /dev/null

    echo ""                                                                                                                     >>& $logFile
    echo "    ... Building htype (tests if Linux instance is running in container or on regular host)"                          >>& $logFile
    echo ""                                                                                                                     >>& $logFile
    gcc -w -Wfatal-errors -O3 -o htype htype.c                                                                                  >>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc htype]"                                                                               >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip htype 														>& /dev/null

    if($build_ckpt == TRUE) then
       echo ""                                                                                                                  >>& $logFile
       echo "    ... Building ssave (saves process state via Criu)"						                >>& $logFile
       echo ""                                                                                                                  >>& $logFile
       vtagup SSAVETAG ssave.c													>& /dev/null 
       gcc -w -DBSD_FUNCTION_SUPPORT -Wfatal-errors -O3 -o ssave ssave.c                                                        >>& $logFile
       if($status != 0) then
          echo ""                                                                                                               >>& $logFile
          echo "ERROR operation failed [gcc ssave]"                                                                             >>& $logFile
          echo ""                                                                                                               >>& $logFile

          exit -1
       endif
       strip ssave 

       echo ""                                                                                                                  >>& $logFile
       echo "    ... Building restart (restarted Criu checkpoints)"						                >>& $logFile
       echo ""                                                                                                                  >>& $logFile
       vtagup SSAVETAG restart.c												>& /dev/null
       gcc -w -DBSD_FUNCTION_SUPPORT -Wfatal-errors -O3 -o restart restart.c                                                    >>& $logFile
       if($status != 0) then
          echo ""                                                                                                               >>& $logFile
          echo "ERROR operation failed [gcc restart]"                                                                           >>& $logFile
          echo ""                                                                                                               >>& $logFile

          exit -1
       endif
       strip restart 
    endif

    echo ""															>>& $logFile
    echo "    ... Building lol (detects whether owner of a lid associated with a simple link lock is alive)"			>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -DBSD_FUNCTION_SUPPORT -D$utarget -D$arch -DSSH_SUPPORT -O3 -I../include.libs -o lol lol.c 		>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc lol]"                                                                                 >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip lol															>& /dev/null

    echo ""															>>& $logFile
    echo "    ... Building gob (provides named gobhole FIFO allowing asynchronous data transfer to pipeline)"			>>& $logFile
 
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -DBSD_FUNCTION_SUPPORT -D$utarget -D$arch -DSSH_SUPPORT -O3 -I../include.libs -o gob gob.c 		>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc gob]"                                                                                 >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip gob															>& /dev/null

    echo ""															>>& $logFile
    echo "    ... Building arse (provides named arse hole FIFO allowing asynchronous data transfer from pipeline)"		>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -DBSD_FUNCTION_SUPPORT -D$utarget -D$arch -DSSH_SUPPORT -O3 -I../include.libs -o arse arse.c		>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc arse]"                                                                                >>& $logFile
        echo ""                                                                                                                 >>& $logFile

    	exit -1
    endif
    strip arse															>& /dev/null

    echo ""															>>& $logFile
    echo "    ... Building P3 tty device homeostat [mktty] (restores /dev/tty if it is deleted)"				>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -D$utarget -D$arch -DSSH_SUPPORT -O3 -I../include.libs -o mktty mktty.c  				>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed gcc mktty]"                                                                                >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip mktty															>& /dev/null

    echo ""															>>& $logFile
    echo "    ... Building P3 file creator [mkfile] (creates arbitrary files)"							>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o mkfile mkfile.c						>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [mklfile]"                                                                                 >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip mkfile														>& /dev/null

    if($dummy_shgmalloc == need_dummy_shgmalloc) then
       echo ""															>>& $logFile
       echo "    ... Building dummy persistent heap library (needed to keep make happy)"					>>& $logFile
       echo ""															>>& $logFile
       gcc -w -Wfatal-errors -c dummy_phgmalloc.c										>>& $logFile
       if($status != 0) then
          echo ""                                                                                                               >>& $logFile
          echo "ERROR operation failed [gcc dummy_phgmalloc]"                                                                   >>& $logFile
          echo ""                                                                                                               >>& $logFile

          exit -1
       endif

       \mv dummy_shgmalloc.o $LIBDIR/shgmalloc.o										>& /dev/null
       if($status != 0) then
          echo ""                                                                                                               >>& $logFile
          echo "ERROR operation failed [install dummy_shgmalloc]"                                                               >>& $logFile
          echo ""                                                                                                               >>& $logFile

          exit -1
       endif
    endif

    echo ""															>>& $logFile
    echo "    ... Building gethostip (gets I.P. address information for DHCP clients)"						>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -DBSD_FUNCTION_SUPPORT -I. -I../include.libs -D$target -O3 -o gethostip gethostip.c  			>>& $logFile
    if($status != 0) then
	echo ""															>>& $logFile
	echo "ERROR operation failed [gcc gethostip]"										>>& $logFile
	echo ""															>>& $logFile

	exit -1
    endif
    strip gethostip														>& /dev/null

    \mv gethostip hupter nkill htype farm lol gob arse mktty mkfile lyosome kepher leaf psrptool servertool p3f stripper      \
															$BINDIR	>& /dev/null
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [install PUPS/P3 support tools]"                                                           >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif

    if($build_ckpt == TRUE) then
        \mv ssave restart $BINDIR												>& /dev/null

        if($status != 0) then
           echo ""                                                                                                              >>& $logFile
           echo "ERROR operation failed [install PUPS/P3 checkpoint support tools]"                                             >>& $logFile
           echo ""                                                                                                              >>& $logFile

           exit -1
        endif
    endif
else

    echo ""															>>& $logFile
    echo "    ... Building P3 HUP to TERM signal relay (relays HUP as TERM to payload process)"					>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o hupter hupter.c						>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc hupter]"                                                                              >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip hupter														>& /dev/null

    echo "    ... Building P3 network signal distributor [nkill] (sends signals to PID or process name on [remote] host)"	>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o nkill nkill.c $crypt  					>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc nkill]"                                                                               >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip nkill															>& /dev/null

    echo ""															>>& $logFile
    echo "    ... Building P3 text file stripper [nkill] (strips comments from files in VDM's)"					>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o stripper stripper.c  					>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc stripper]"                                                                            >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip stripper														>& /dev/null

    echo ""															>>& $logFile
    echo "    ... Building P3 tty device homeostat [mktty] (restores /dev/tty if it is deleted)"				>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o mktty mktty.c  >>& $logFile				>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc mktty]"                                                                               >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip mktty															>& /dev/null

    echo ""															>>& $logFile
    echo "    ... Building P3 file creator [mkfile] (creates arbitrary files)"							>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o mkfile mkfile.c  >>& $logFile				>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc mkfile]"                                                                              >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip mkfile														>& /dev/null

    echo ""															>>& $logFile
    echo "    ... Building P3 lightweight file homeostat [lyosome] (protects a file for specified time period)"			>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -DBSD_FUNCTION_SUPPORT -D$utarget -D$arch -O3 -I../include.libs -o lyosome lyosome.c			>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc lyosome]"                                                                             >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif

    echo ""															>>& $logFile
    echo "    ... Building P3 lightweight garbage collector [kepher] (removes stale temporary files)"				>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o kepher kepher.c						>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc kepher]"                                                                              >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip kepher                      												>& /dev/null

    echo ""															>>& $logFile
    echo "    ... Building P3 sub-path extractor [leaf] (extracts twigs from pathname branches)"				>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o leaf leaf.c   						>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc leaf]"                                                                                >>& $logFile
        echo ""                                                                                                                 >>& $logFile
        exit -1
    endif
    strip leaf                        												>& /dev/null

    echo ""															>>& $logFile
    echo "    ... Building psrptool (starts psrp client process in new [xterm] window)"						>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o psrptool psrptool.c  					>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc psrptool]"                                                                            >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip psrptool														>& /dev/null

    echo ""															>>& $logFile
    echo "    ... Building P3 farm (permits definition of network-wide [homogenous] process farms)"			        >>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -DBSD_FUNCTION_SUPPORT -D$utarget -D$arch -O3 -I../include.libs -o farm farm.c $crypt  		>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc farm]"                                                                                >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif

    echo ""                                                                                                                     >>& $logFile
    echo "    ... Building htype (tests if Linux instance is running in container or on regular host)"                          >>& $logFile
    echo ""                                                                                                                     >>& $logFile
    gcc -w -Wfatal-errors -O3 -o htype htype.c                                                                                  >>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc htype]"                                                                               >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip htype															>& /dev/null	

    if($build_ckpt == TRUE) then
       echo ""                                                                                                                  >>& $logFile
       echo "    ... Building ssave (saves process state via Criu)"                                                             >>& $logFile
       echo ""                                                                                                                  >>& $logFile
       vtagup SSAVETAG ssave.c													>& /dev/null 
       gcc -w -Wfatal-errors -o ssave ssave.c                                                                                   >>& $logFile
       if($status != 0) then
          echo ""                                                                                                               >>& $logFile
          echo "ERROR operation failed [gcc ssave]"                                                                             >>& $logFile
          echo ""                                                                                                               >>& $logFile

          exit -1
       endif
       strip ssave

       echo ""                                                                                                                  >>& $logFile
       echo "    ... Building restart (restarted Criu checkpoints)"                                                             >>& $logFile
       echo ""                                                                                                                  >>& $logFile
       vtagup SSAVETAG restart.c												>& /dev/null 
       gcc -w -Wfatal-errors -O3 -o restart restart.c                                                                           >>& $logFile
       if($status != 0) then                                                                                                    
          echo ""                                                                                                               >>& $logFile
          echo "ERROR operation failed [gcc restart]"                                                                           >>& $logFile
          echo ""                                                                                                               >>& $logFile
        
          exit -1                                                                                                               
       endif
       strip restart
    endif

    echo ""															>>& $logFile
    echo "    ... Building lol (detects whether owner of a lid associated with a simple link lock is alive)"			>>& $logFile
    echo ""															>>& $logFile
    gcc -w -Wfatal-errors -D$utarget -D$arch -DSSH_SUPPORT -O3 -I../include.libs -o lol lol.c  					>>& $logFile
    if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "ERROR operation failed [gcc lol]"                                                                                 >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit -1
    endif
    strip lol															>& /dev/null

    if($dummy_phgmalloc == need_dummy_phgmalloc) then
       echo ""															>>& $logFile
       echo "... Building dummy persistent heap library (needed to keep make happy)"						>>& $logFile
       echo ""															>>& $logFile
       gcc -w -Wfatal-errors -c dummy_phgmalloc.c										>>& $logFile
       if($status != 0) then
          echo ""                                                                                                               >>& $logFile
          echo "ERROR operation failed [gcc dummy_phgmalloc]"                                                                   >>& $logFile
          echo ""                                                                                                               >>& $logFile

          exit -1
       endif

       \mv dummy_shgmalloc.o $LIBDIR/shgmalloc.o										>& /dev/null
       if($status != 0) then
          echo ""                                                                                                               >>& $logFile
          echo "ERROR operation failed [install dummy_phgmalloc]"                                                               >>& $logFile
          echo ""                                                                                                               >>& $logFile

          exit -1
       endif
    endif

    \mv hupter nkill mktty mkfile htype farm lol lyosome kepher leaf psrptool servertool stripper $BINDIR 			>& /dev/null
    if($status != 0) then
       echo ""                                                                                                                  >>& $logFile
       echo "ERROR operation failed [install PUPS/P3 support tools]"                                                            >>& $logFile
       echo ""                                                                                                                  >>& $logFile

       exit -1
    endif

    if($build_ckpt == TRUE) then
    	\mv ssave restart $BINDIR												>& /dev/null

    	if($status != 0) then
	   echo ""                                                                                                              >>& $logFile
           echo "ERROR operation failed [install PUPS/P3 checkpoint support tools]"                                             >>& $logFile
           echo ""                                                                                                              >>& $logFile

           exit -1
        endif
    endif
endif

echo ""																>>& $logFile
echo "    ... Building catfiles (appends two files preserving inode of first)"							>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -I. -I../include.libs -D$utarget -D$arch -O3 -o catfiles catfiles.c  					>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [gcc catfiles]"                                                                                 >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

strip catfiles															>& /dev/null
\mv catfiles $BINDIR 														>& /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [install catfiles]"                                                                             >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif


echo ""																>>& $logFile
echo "    ... Building new_session (creates new session)"									>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -D$utarget -D$arch -I. -O3 -o new_session new_session.c							>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed gcc new_session]"                                                                               >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

strip new_session 														>& /dev/null
\mv new_session $BINDIR 													>& /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [install new_session]"                                                                          >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

if($buildtype == cluster) then
   echo ""															>>& $logFile
   echo "    ... Making persistent heap library"										>>& $logFile
   echo ""															>>& $logFile
   $BINDIR/pupsconf $arch.$pctarget Make_phmalloc.in Makefile									>&  /dev/null 
   if($status != 0) then
      echo ""                                                                                                                   >>& $logFile
      echo "ERROR operation failed [configure phmalloc]"                                                                        >>& $logFile
      echo ""                                                                                                                   >>& $logFile

      exit -1
   endif

   make -j$nCores 														>>& $logFile
   if($status != 0) then
      echo ""                                                                                                                   >>& $logFile
      echo "ERROR operation failed make phmalloc]"                                                                              >>& $logFile
      echo ""                                                                                                                   >>& $logFile

      exit -1
   endif

   make clean															>>& $logFile
   echo ""															>>& $logFile
endif

echo ""                                                                                                                         >>& $logFile
echo "    ... Making nobubble compatability library"                                                                            >>& $logFile
echo ""                                                                                                                         >>& $logFile
$BINDIR/pupsconf $arch.$pctarget Make_nobubble.in Makefile                                                                      >&  /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [configure nobubble]"                                                                           >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

make -j$nCores                                                                                                                  >>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [make nobubble]"                                                                                >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

echo ""																>>& $logFile
echo "    ... Making P3-2019 libraries"												>>& $logFile
echo ""																>>& $logFile
$BINDIR/pupsconf $arch.$pctarget Make_libs.in Makefile										>&  /dev/null 
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [configure PUpS/P3 libraries]"                                                                  >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

make -j$nCores															>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [make PUPS/P3 libaries]"                                                                        >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif


#------------------------------------
# Build PUPS/P3 service applications
#------------------------------------

echo ""																>>& $logFile
echo "    ... Making P3-2019 service applications"										>>& $logFile
echo ""																>>& $logFile


#-------------------------
# Build psrp client shell 
#-------------------------

echo "    ... Building P3-2019 interaction client (allows user to manipulate arbitrary processes)"				>>& $logFile
echo ""

$BINDIR/pupsconf $arch.$pctarget Make_psrp.in Makefile										>& /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [configure psrp client]"                                                                        >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

make -j$nCores   														>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [make psrp client]"                                                                             >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

if($debug == TRUE) then
	make unstripped  													>>& $logFile
else
	make install														>>& $logFile
endif
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [install psrp client]"                                                                          >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

make clean    															>>& $logFile


#-----------------------------------------
# Build maggot (stale resource recylcler) 
#-----------------------------------------

echo ""																>>& $logFile
echo "    ... Building P3-2019 maggot (removes stale PSRP resources from system)"						>>& $logFile
echo ""																>>& $logFile
$BINDIR/pupsconf $arch.$pctarget Make_maggot.in Makefile									>& /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [configure maggot]"                                                                             >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

make -j$nCores     														>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [make maggot]"                                                                                  >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

if($debug == TRUE) then
	make unstripped  													>>& $logFile
else
	make install														>>& $logFile
endif
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [install maggot]"                                                                                                >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

make clean    															>>& $logFile


#-----------------------------------------------
# Build embryo (uncommitted dynamic application 
#-----------------------------------------------

echo ""																>>& $logFile
echo "    ... Building embryo application (an unassigned dynamically programmable server)"					>>& $logFile
echo ""																>>& $logFile

$BINDIR/pupsconf $arch.$pctarget Make_embryo.in Makefile									>& /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [configure embryo]"                                                                             >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

make -j$nCores    														>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [make embryp]"                                                                                  >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

if($debug == TRUE) then
	make unstripped  													>>& $logFile
else
	make install														>>& $logFile
endif
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [install embryo]"                                                                               >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

make clean    															>>& $logFile


#---------------------------------
# Build pass (pipeline) processor 
#---------------------------------

echo ""																>>& $logFile
echo "    ... Building pass application (executes arbitrary command pipeline connected to P3/PSRP server)"			>>& $logFile
echo ""
$BINDIR/pupsconf $arch.$pctarget Make_pass.in Makefile										>& /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [configure pass]"                                                                               >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

make -j$nCores          													>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [make pass]"                                                                                    >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

if($debug == TRUE) then
	make unstripped  													>>& $logFile
else
	make install														>>& $logFile
endif
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [install pass]"                                                                                 >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

make clean    															>>& $logFile


#--------------------------------------
# Build fsw (smart filesystem watcher) 
#--------------------------------------

echo ""																>>& $logFile
echo "    ... Building P3-2019 file system watcher (intelligently deals with full filesystems)"					>>& $logFile
echo ""																>>& $logFile
$BINDIR/pupsconf $arch.$pctarget Make_fsw.in Makefile										>& /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [configure fsw]"                                                                                >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

make -j$nCores      														>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed make fsw]"                                                                                      >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

if($debug == TRUE) then
	make unstripped  													>>& $logFile
else
	make install														>>& $logFile
endif
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [install fsw]"                                                                                  >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

make clean    															>>& $logFile


#------------------------------------------
# Build protect (arbitrary file protector) 
#------------------------------------------

echo ""																>>& $logFile
echo "    ... Building P3-2019 file/directory protector (homeostatic protection for files and directories)"			>>& $logFile
echo ""																>>& $logFile
$BINDIR/pupsconf $arch.$pctarget Make_protect.in Makefile									>& /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [configure protect]"                                                                            >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

make -j$nCores    														>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [make protect]"                                                                                 >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

if($debug == TRUE) then
	make unstripped  													>>& $logFile
else
	make install														>>& $logFile
endif
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [install protect]"                                                                              >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

make clean    															>>& $logFile


#-----------------------------------
# Build xcat (extended cat command) 
#-----------------------------------

echo ""																>>& $logFile
echo "    ... Building P3-2019 extended file catenator (replacement for cat command))"						>>& $logFile
echo ""																>>& $logFile
$BINDIR/pupsconf $arch.$pctarget Make_xcat.in Makefile										>& /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [configure xcat]"                                                                               >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

make -j$nCores          													>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [make xcat]"                                                                                    >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

if($debug == TRUE) then
	make unstripped  													>>& $logFile
else
	make install														>>& $logFile
endif
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [install xcat]"                                                                                 >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

make clean    															>>& $logFile


#-----------------------------------
# Build xtee (extended tee command) 
#-----------------------------------

echo ""																>>& $logFile
echo "    ... Building P3-2019 extended pipeline tee (replacement for tee command))"						>>& $logFile
echo ""																>>& $logFile

$BINDIR/pupsconf $arch.$pctarget Make_xtee.in Makefile										>& /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [configure xtee]"                                                                               >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

make -j$nCores          													>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [make xtee]"                                                                                    >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

if($debug == TRUE) then
	make unstripped  													>>& $logFile
else
	make install														>>& $logFile
endif
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [install xtee]"                                                                                 >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

make clean    															>>& $logFile


#------------------------------------------
# Build tcell (license violation detector) 
#------------------------------------------

echo ""																>>& $logFile
echo "    ... Building P3-2019 licence violation detector (terminates arbitrary process violating license)"			>>& $logFile
echo ""																>>& $logFile

$BINDIR/pupsconf $arch.$pctarget Make_tcell.in Makefile										>& /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [configure tcell]"                                                                              >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

make -j$nCores  														>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [make tcell]"                                                                                   >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

if($debug == TRUE) then
	make unstripped  													>>& $logFile
else
	make install														>>& $logFile
endif
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "ERROR operation failed [install tcell]"                                                                                >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit -1
endif

make clean    															>>& $logFile
echo ""																>>& $logFile


if(debug == TRUE) then
	echo ""
	echo "    ----------------------------------------------------"
	echo "    Installing debuggable PUPS/P3 libraries and tools"
	echo "    ----------------------------------------------------"
	echo ""
endif


#------------------------------------------------
# Copy PSRP bash profile to local home directory
#------------------------------------------------

echo ""																>>& $logFile
echo "    ... installing P3 bash profile in '$HOME'"										>>& $logFile
\cp psrp_bash_profile $HOME/.psrp_bash_profile											>& /dev/null
if($status < 0) then
    echo ""                                                                                                             	>>& $logFile
    echo "ERROR operation failed [install PUPS/P3 bash profile]"                                                              	>>& $logFile
    echo ""                                                                                                             	>>& $logFile

    exit -1
endif


#-----------------------------------------
# Copy tool script to local bin directory
#-----------------------------------------

echo ""																>>& $logFile
echo "    ... installing P3 scripts in '$BINDIR'"										>>& $logFile
chmod +x uninstall_pups mantohtml manc tall pupsuname configure somake WIPE TMPWIPE PSRPTOOL P3L2MAN				>& /dev/null
\cp  uninstall_pupsP3.csh mantohtml manc tall pupsuname configure somake WIPE TMPWIPE PSRPTOOL P3L2MAN $BINDIR			>& /dev/null
if($status < 0) then
    echo ""                                                                                                             	>>& $logFile
    echo "ERROR operation failed [install PUPS/P3 tool scripts]"                                                              	>>& $logFile
    echo ""                                                                                                             	>>& $logFile

    exit -1
endif


#------------------------------------------------------------
# Copy project templates to local project template directory 
#------------------------------------------------------------

echo "    ... installing P3 project templates in '$PROJECTDIR'"									>>& $logFile
\cp dll_skel.c dll_skelfunc.c lib_skel.c lib_skelfunc.c  Make_skelpapp.in  skelpapp.c $PROJECTDIR				>& /dev/null
if($status != 0) then
    echo ""                                                                                                             	>>& $logFile
    echo "ERROR operation failed [install project templates]"                                                                  	>>& $logFile
    echo ""                                                                                                             	>>& $logFile

    exit -1
endif


#----------------------------------------------
# Copy distribution to local binary directory
#----------------------------------------------

echo "    ... installing P3 binaries in '$BINDIR'"										>>& $logFile	
pushd ../bin															>& /dev/null 
\mv * $BINDIR 															>& /dev/null
if($status != 0) then
    echo ""                                                                                                             	>>& $logFile
    echo "ERROR operation failed [install PUPS/P3 distribution]"                                                              	>>& $logFile
    echo ""                                                                                                             	>>& $logFile

    exit -1
endif

popd																>& /dev/null


#--------------------------------------------
# Copy libraries to local library directory
#--------------------------------------------

echo "    ... installing P3 libaries in '$LIBDIR'"										>>& $logFile
pushd ../lib															>& /dev/null
\cp * $LIBDIR 															>& /dev/null
if($status != 0) then
    echo ""                                                                                                             	>>& $logFile
    echo "ERROR operation failed [install PUPS/P3 libraries]"                                                                  	>>& $logFile
    echo ""                                                                                                             	>>& $logFile

    exit -1
endif

popd																>& /dev/null


#---------------------------------------
# Copy header files to local directory 
#---------------------------------------

echo "    ... installing P3 headers in '$HDRDIR'"										>>& $logFile
echo ""																>>& $logFile
pushd ../include.libs														>& /dev/null
\cp * $HDRDIR 															>& /dev/null
if($status != 0) then
    echo ""                                                                                                             	>>& $logFile
    echo "ERROR operation failed [install PUPS/P3 headers]"                                                                   	>>& $logFile
    echo ""                                                                                                             	>>& $logFile

    exit -1
endif

popd																>& /dev/null


#-----------------------------------
# Copy man pages to local directory
#-----------------------------------

echo "    ... installing P3 manual pages in '$MANDIR'"										>>& $logFile
echo ""																>>& $logFile
\cp ../man/man1/* $MANDIR/man1													>& /dev/null
if($status != 0) then
    echo ""                                                                                                             	>>& $logFile
    echo "ERROR operation failed [install PUPS/P3 man1 pages]"                                                                	>>& $logFile
    echo ""                                                                                                             	>>& $logFile

    exit -1
endif

\cp ../man/man3/* $MANDIR/man3													>& /dev/null
if($status != 0) then
    echo ""                                                                                                             	>>& $logFile
    echo "ERROR operation failed [install PUPS/P3 man3 pages]"                                                                	>>& $logFile
    echo ""                                                                                                             	>>& $logFile

    exit -1
endif

\cp ../man/man7/* $MANDIR/man7													>& /dev/null
if($status != 0) then
    echo ""                                                                                                             	>>& $logFile
    echo "ERROR operation failed [install PUPS/P3 man7 pages]"                                                                 	>>& $logFile
    echo ""                                                                                                             	>>& $logFile

    exit -1
endif


#------------------------------------------
# Debuggable libraries cannot be installed 
#------------------------------------------

if($debug == FALSE) then


	#--------------------------------
	# Install configuation directory
	#--------------------------------

	echo ""															>>& $logFile
	echo "    ... installing configuration directory (in '$PROJECTDIR')"							>>& $logFile
	echo ""															>>& $logFile
	pushd ../config.install													>& /dev/null

	./install_config.csh $target $buildtype											>>& $logFile
	if($status != 0) then
		echo ""                                                                                                        	>>& $logFile
   		echo "ERROR operation failed [install configuration directory]"                                              	>>& $logFile
   		echo ""                                                                                                        	>>& $logFile

       		exit -1
	endif

	popd                    												>& /dev/null
endif


echo ""																>>& $logFile
echo "    ------------------------------------------------------------------------------------------"				>>& $logFile
echo "    Make sure your ~/.tcshrc script has the following lines in it:"							>>& $logFile
echo ""																>>& $logFile

if(`whoami` == root) then
        echo "    setenv P3_CONFDIR /usr/include/p3"										>>& $logFile
        echo "    setenv LD_LIBRARY_PATH /usr/lib (should be set by default)"							>>& $logFile
else
        echo "    setenv P3_CONFDIR ~/p3"											>>& $logFile
        echo "    setenv LD_LIBRARY_PATH ~/lib"											>>& $logFile
	endif
endif

echo "    ------------------------------------------------------------------------------------------"				>>& $logFile
echo ""																>>& $logFile


if($debug == TRUE) then
	echo ""															>>& $logFile
	echo "    ... Marking P3-2019 libraries as debuggable"									>>& $logFile
	echo ""
	mkfile ../lib.$arch.linux.cluster/debug											>& /dev/null
else
	\rm ../lib.$arch.linux.cluster/debug											>& /dev/null
endif

echo ""																>>& $logFile
echo "    ... build of P3-2019 libraries and services complete"									>>& $logFile
echo ""																>>& $logFile

\rmdir BUILDLOCK														>&/dev/null 
exit 0


#--------------
# Abort build
#--------------

abort_build:

echo ""
echo "    *** interrupted [PUPSP3]"												>>& $logFile	
echo ""																>>& $logFile

\rmdir BUILDLOCK														>& /dev/null 
exit -1

