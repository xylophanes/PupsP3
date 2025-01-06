#!/bin/tcsh 
#
#-------------------------------------------------------------------------------------
#  Install PUPSP3 libraries and service functions
#  (C) M.A. O'Neill, Tumbling Dice Ltd, 2000-2025
#
#  $1 is logfile
#  $2 is set to debug (to build debuggable system e.g -g)
#  $2 is set to sanitize (to build bounds checking debuggable system (e.g. -fsanitize)
#-------------------------------------------------------------------------------------

set do_abort = FALSE


#----------------------------
# Check we have gcc installed
#----------------------------

set no_gcc = `/usr/bin/which gcc`
if("$no_gcc" == "") then
	echo ""
	echo "    ERROR install_pupsp3: cannot find gcc compiler"
	echo ""

	set do_abort = TRUE
endif


#-------------------------------
# Check we have patch installed
#-------------------------------

set no_patch = `/usr/bin/which patch`
if("$no_patch" == "") then
	echo ""
	echo "    ERROR install_pupsp3: cannot find patch"
	echo ""

	set do_abort = TRUE
endif


#-----------------------------
# Check we have sudo installed
#-----------------------------

set no_sudo = `/usr/bin/which sudo`
if("$no_sudo" == "") then
	echo ""
	echo "    ERROR install_pupsp3: cannot find sudo"
	echo ""

	set do_abort = TRUE
endif


#------------------------------------------
# Check we have the libraries which we need
#------------------------------------------
#---------------------
# BSD function support
#---------------------

set no_bsd = `gcc -lbsd |& grep cannot`
if("$no_bsd" != "") then
	echo ""
	echo "    ERROR install_pupsp3: cannot find required library libbsd"
	echo ""

	set do_abort = TRUE 
endif


#-------------
# GNU readline
#-------------

set no_readline = `gcc -lreadline |& grep cannot`
if("$no_readline" != "") then
	echo ""
	echo "    ERROR install_pupsp3: cannot find required library libreadline"
	echo ""

	set do_abort = TRUE 
endif


#-----
# TIFF
#-----

set no_tiff = `gcc -ltiff |& grep cannot`
if("$no_tiff" != "") then
	echo ""
	echo "    ERROR install_pupsp3: cannot find required library libtiff"
	echo ""

	set do_abort = TRUE 
endif


#--------------------------------------------------
# Abort if build environment has missing components
#--------------------------------------------------

if($do_abort == TRUE) then
	exit 255
endif


#-----------------------------
#  Build in checkpoint support
#-----------------------------

set build_ckpt      = FALSE 


#--------------------------------------------------
#  This can be set to float or double and controls
#  floating point precision in P3 libraries
#--------------------------------------------------

set ftype           = FLOAT


#-------------------------------------------
#  Set MACHTYPE if it isn't defined (e.g. on
#  Raspberry-Pi running Raspbian)
#-------------------------------------------

#set usable_machtype = `echo $MACHTYPE | grep unknown`
#if($usable_machtype != "") then
	set MACHTYPE = `uname -m`
#endif


#------------------------------------------------------
# Correct "namecreep" in 32 bit Intel/AMD architectures
#------------------------------------------------------

if($MACHTYPE == i686) then
	set MACHTYPE = i386
endif


#----------------------------------------------
# Make sure we use the right configuration file
#----------------------------------------------

rm -f ../config/$OSTYPE.cluster                                                                 				>&  /dev/null
rm -f ../config/linux.cluster													>&  /dev/null
rm -f ../config.install/$OSTYPE.cluster                                                         				>&  /dev/null
rm -f ../config.install/linux.cluster												>&  /dev/null
rm -f ../config.debug/$OSTYPE.cluster                                                           				>&  /dev/null
rm -f ../config.debug/linux.cluster                                                             				>&  /dev/null
rm -f ../config.sanitize/$OSTYPE.cluster                                                           				>&  /dev/null
rm -f ../config.sanitize/linux.cluster                                                             				>&  /dev/null
rm -f ../config.nobubble/$OSTYPE.cluster                                                           				>&  /dev/null
rm -f ../config.nobubble/linux.cluster                                                             				>&  /dev/null

onintr abort_build


#----------------------------
# Stop complaints from loader
#----------------------------

setenv LD_LIBRARY_PATH


#-------------------
# Initialise logfile
#-------------------

if($# == 0 && $# != 2 &&  $# != 3 || $1 == help || $1 == usage) then
	echo ""
	echo "    P3-2025 (SUP3) build script (C) M.A. O'Neill, Tumbling Dice Ltd 2025"
        echo "    Usage: install_pupsP3 [usage] | [clean] | \!tty | logfile\! [nobubble] | [debug] | [sanitize]"
	echo ""

	exit 0
else if ($1 == clean) then

	echo ""
	echo "    ... cleaning build tree"
	echo ""	

	\rm -f  *.a														>&  /dev/null
	\rm -f  *.o														>&  /dev/null
	\rm -f  *.so														>&  /dev/null
	\rm -rf ../lib*														>&  /dev/null
	\rm -rf ../bin*														>&  /dev/null
	\rm -f ../config/*linux.cluster												>&  /dev/null
	\rm -f ../config.debug/*linux.cluster											>&  /dev/null
	\rm -f ../config.sanitize/*linux.cluster										>&  /dev/null
	\rm -f ../config.nobubble/*linux.cluster										>&  /dev/null
	\rm -f ../config.install/*linux.cluster											>&  /dev/null
	\rm -f BUILDLOCK													>&  /dev/null 
	\rm -f Makefile														>&  /dev/null

	exit 0

else if($1 == default || $1 == "" || $1 == null || $1 == /dev/null) then
        set logFile = /dev/null

else if($1 == "tty" || $1 == /dev/tty) then
        set logFile = /dev/tty

else
        set logFile = $2


        if(! $?cshlevel) then
                setenv cshlevel 1
                echo ""														>&  $logFile
        endif
endif


#---------------------------------------------------------
# Make sure we pick up the correct configuration directory
#---------------------------------------------------------
#-----------
# Debug mode
#-----------

if($2 == debug) then
	set debug    = TRUE
	set sanitize = FALSE
	set nobubble = FALSE


#--------------
# Sanitize mode
#--------------

else if($2 == sanitize) then
	set debug    = FALSE 
	set sanitize = TRUE
	set nobubble = FALSE


#--------------
# Nobubble mode
#--------------

else if($2 == sanitize) then
	set debug    = FALSE 
	set sanitize = FALSE 
	set nobubble = TRUE

#------
# Error
#------

else if($2 != "") then
	echo ""															>>& $logFile
	echo "    ERROR install_pupsp3: 'debug' or 'sanitize' expected"								>>& $logFile
	echo ""                                                                                                                 >>& $logFile

	exit 255 
else
	set debug    = FALSE
	set sanitize = FALSE
	set nobubble = FALSE
endif


#------------
# Debug build
#------------

if($debug == TRUE) then
        setenv P3_CONFDIR   ../config.debug                                                     				>&  /dev/null
        setenv PUPS_CONFDIR ../config.debug                                                     				>&  /dev/null

        set confdir = ../config.debug


#---------------
# Sanitize build
#---------------

else if($sanitize == TRUE) then
        setenv P3_CONFDIR   ../config.sanitize                                                   				>&  /dev/null
        setenv PUPS_CONFDIR ../config.sanitize                                                     				>&  /dev/null

        set confdir = ../config.sanitize


#---------------
# Nobubble build
#---------------

else if($nobubble == TRUE) then
        setenv P3_CONFDIR   ../config.nobubble                                                   				>&  /dev/null
        setenv PUPS_CONFDIR ../config.nobubble                                                     				>&  /dev/null

        set confdir = ../config.nobubble


#--------------
# Default build
#--------------

else
        setenv P3_CONFDIR   ../config                                                           				>&  /dev/null
        setenv PUPS_CONFDIR ../config                                                           				>&  /dev/null

        set confdir = ../config
endif




#############################################################################
#### Set up configuration links - we also set the default floating point ####
#### precision here                                                      ####
#############################################################################
#------------------------------
# Build with checkpoint support
#------------------------------

if($build_ckpt == TRUE) then


	#--------------
	# Default build
	#--------------

	pushd ../config														>&  /dev/null
	sed s/FTYPE/$ftype/g <$OSTYPE.cluster.ckpt >linux.cluster
	popd															>&  /dev/null


	#------------
	# Debug build
	#------------

	if($debug == TRUE) then
                pushd ../config.debug                                                           				>&  /dev/null
                sed s/FTYPE/$ftype/g <$OSTYPE.cluster.ckpt >linux.cluster
                popd                                                                            				>&  /dev/null


	#---------------
	# Sanitize build
	#---------------

	else if($sanitize == TRUE) then
                pushd ../config.sanitize                                                           				>&  /dev/null
                sed s/FTYPE/$ftype/g <$OSTYPE.cluster.ckpt >linux.cluster
                popd                                                                            				>&  /dev/null


	#---------------
	# Nobubble build
	#---------------

	else if($nobubble == TRUE) then
                pushd ../config.nobubble                                                           				>&  /dev/null
                sed s/FTYPE/$ftype/g <$OSTYPE.cluster.ckpt >linux.cluster
                popd                                                                            				>&  /dev/null


	#--------------
	# Install build
	#---------------

	else
        	pushd ../config.install                                                         				>&  /dev/null
		sed s/FTYPE/$ftype/g <$OSTYPE.cluster.ckpt >linux.cluster
        	popd                                                                            				>&  /dev/null
        endif


#---------------------------------
# Build without checkpoint support
#---------------------------------

else

	#---------------------------------
	# Build without checkpoint support
	#---------------------------------

	pushd ../config														>&  /dev/null
	sed s/FTYPE/$ftype/g <$OSTYPE.cluster.nockpt >linux.cluster
	popd															>&  /dev/null


	#------------
	# Debug build
	#------------

	if($debug == TRUE) then
        	pushd ../config.debug                                                           				>&  /dev/null
		sed s/FTYPE/$ftype/g <$OSTYPE.cluster.nockpt >linux.cluster
        	popd                                                                            				>&  /dev/null


	#---------------
	# Sanitize build
	#----------------

	else if($sanitize == TRUE) then
        	pushd ../config.sanitize                                                           				>&  /dev/null
		sed s/FTYPE/$ftype/g <$OSTYPE.cluster.nockpt >linux.cluster
        	popd   	                                                                        				>&  /dev/null


	#---------------
	# Nobubble build
	#---------------

	else if($nobubble == TRUE) then
        	pushd ../config.nobubble                                                           				>&  /dev/null
		sed s/FTYPE/$ftype/g <$OSTYPE.cluster.nockpt >linux.cluster
        	popd   	                                                                        				>&  /dev/null


	#--------------
	# Install build
	#--------------

	else
        	pushd ../config.install                                                         				>&  /dev/null
		sed s/FTYPE/$ftype/g <$OSTYPE.cluster.nockpt >linux.cluster
        	popd                                                                            				>&  /dev/null
	endif
endif


#------------------------------
# Get number of processor cores
#------------------------------

set nCores = `lscpu | grep "CPU(s):" | head -1 | awk '{print $2}'`


echo ""																>>& $logFile
echo "    P3-2025 (SUP3) build script (C) M.A. O'Neill, Tumbling Dice Ltd 2025"							>>& $logFile


#------
# Debug
#------

if($debug == TRUE) then
	echo "    Configuring P3-2025 for "$MACHTYPE-$OSTYPE-gnu "host (debug mode)"						>>& $logFile
	echo "    $nCores processor cores found"										>>& $logFile


#---------
# Sanitize
#---------

else if($sanitize == TRUE) then
	echo "    Configuring P3-2025 for "$MACHTYPE-$OSTYPE-gnu "host (sanitize mode)"						>>& $logFile
	echo "    $nCores processor cores found"										>>& $logFile


#---------
# Nobubble
#---------

else if($nobubble == TRUE) then
	echo "    Configuring P3-2025 for "$MACHTYPE-$OSTYPE-gnu "host (nobubble mode)"						>>& $logFile
	echo "    $nCores processor cores found"										>>& $logFile


#-----------
# Production
#-----------

else
	echo "    Configuring P3-2025 for "$MACHTYPE-$OSTYPE-gnu "host"								>>& $logFile
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



#--------------------------------------
# Distribution installation directories
#--------------------------------------
#-----
# Root
#-----

if(`whoami` == "root") then

	#----------------
	# Root is special
	#----------------

        set PROJECTDIR    = /p3
	set BINDIR	  = /usr/bin


	#----------------------------------------
	# Test for 64 bit architecture and adjust
	# LIBDIR if found
	#----------------------------------------
	#-------
	# 64 bit
	#------- 
	if(`uname -m` == x86_64 || `uname -m` == aarch64) then
		set LIBDIR  = /usr/lib64


	#-------
	# 32 bit
	#-------

	else
		set LIBDIR  = /usr/lib
	endif

        set INCDIR        = /usr/include
	set HDRDIR	  = /usr/include/p3
	set MANDIR	  = /usr/share/man
	set projectPath   = \"\/p3\" 


	#------------------------------------------
	# Install virt-what patch and sudoers file 
	#------------------------------------------

	patch -p1 /usr/sbin/virt-what < virt-what.mao.patch      								>& /dev/null

	\mv /etc/sudoers /etc/sudoers                   									>& /dev/null
	\cp sudoers     /etc/sudoers                   										>& /dev/null

	echo ""															>>& $logFile
	echo "    ... Installed PUP/P3 specific sudoers file and patched virt-what"						>>& $logFile
	echo ""															>>& $logFile


#------------
# Normal user
#------------

else
	set PROJECTDIR    = $HOME/p3
	set BINDIR        = $HOME/bin


	#----------------------------------------
	# Test for 64 bit architecture and adjust
	# LIBDIR if found
	#----------------------------------------
	#-------
	# 64 bit
	#-------
	if(`uname -m` == x86_64 || `uname -m` == aarch64) then
		set LIBDIR = $HOME/lib64


	#-------
	# 32 bit
	#-------

	else
		set LIBDIR = $HOME/lib
	endif

        set INCDIR        = $HOME/include
	set HDRDIR	  = $HOME/include/p3
	set MANDIR        = $HOME/man
	set projectPath   = \"$HOME/p3\"
endif


#--------------------------------------------------
# We need to make any directories which we may need
#--------------------------------------------------

if(! -e $PROJECTDIR) then
        echo ""															>>& $logFile
        echo "    ... Creating '$PROJECTDIR' (PUPS/P3 project templates directory)"						>>& $logFile
        echo ""															>>& $logFile

        mkdir $PROJECTDIR													>&  /dev/null


	#-----------------------------
	# Allow normal users to create
	# default configurations
	#-----------------------------

	chmod 0777 $PROJECTDIR													>&  /dev/null
	endif
endif


#--------------
# Bin directory
#--------------

if(! -e $BINDIR) then
	echo ""															>>& $logFile
	echo "    ... Creating '$BINDIR' (PUPS/P3 binaries directory)"								>>& $logFile
	echo ""															>>& $logFile

	mkdir $BINDIR														>&  /dev/null
	endif
endif


#------------------
# Library directory
#------------------

if(! -e $LIBDIR) then
        echo ""															>>& $logFile
        echo "    ... Creating '$LIBDIR' (PUPS/P3 library directory)"								>>& $logFile
        echo ""															>>& $logFile

        mkdir $LIBDIR														>&  /dev/null
	endif
endif


#-----------------
# Header directory
#-----------------

if(! -e $HDRDIR) then
        echo ""															>>& $logFile
        echo "    ... Creating '$HDRDIR' (PUPS/P3 header directory)"								>>& $logFile
        echo ""															>>& $logFile

        mkdir $INCDIR														>&  /dev/null
       	mkdir $HDRDIR														>&  /dev/null
endif


#------------------------
# Top level man directory
#------------------------

if(! -e $MANDIR) then
        echo ""															>>& $logFile
        echo "    ... Creating '$MANDIR' (PUPS/P3 manual directory)"								>>& $logFile
        echo ""															>>& $logFile

        mkdir $MANDIR														>&  /dev/null
endif


#-----
# Man1
#-----

if(! -e $MANDIR/man1) then
	mkdir $MANDIR/man1													>&  /dev/null
endif


#-----
# Man3
#-----

if(! -e $MANDIR/man3) then
	mkdir $MANDIR/man3													>&  /dev/null
endif


#-----
# Man7
#-----

if(! -e $MANDIR/man7) then
	mkdir $MANDIR/man7													>&  /dev/null
endif


#--------------------------------------------------------------
# If we haven't got .cshrc and/or .tcshrc for this user install
# them now
#--------------------------------------------------------------

if(! -e ~/.cshrc) then
	echo ""															>>& $logFile
	echo "Setting up P3M environment (for csh)"										>>& $logFile
	echo ""															>>& $logFile

	\cp p3m_cshrc ~/.cshrc													>&  /dev/null
endif

if(! -e ~/.tcshrc) then
	echo ""															>>& $logFile
	echo "Setting up P3M environment (for tcsh)"										>>& $logFile
	echo ""															>>& $logFile

	\cp p3m_tcshrc ~/.tcshrc												>&  /dev/null
endif

gcc -w -Wfatal-errors -I../include.libs -O3 -o isatty isatty.c									>>& $logFile
if($status != 0) then
	echo ""															>>& $logFile
	echo "    ERROR install_pupsp3:  operation failed [gcc isatty]"								>>& $logFile
	echo ""															>>& $logFile

	exit 255 
endif

gcc -w -Wfatal-errors -I../include.libs -O3 -o iscard iscard.c									>>& $logFile
if($status != 0) then
	echo ""															>>& $logFile
	echo "    ERROR install_pupsp3:  operation failed [gcc iscard]"								>>& $logFile
	echo ""															>>& $logFile

	exit 255 
endif

gcc -w -Wfatal-errors -I../include.libs -O3 -o ishex ishex.c  									>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "    ERROR install_pupsp3: operation failed [gcc ishex]"                                 				>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit 255 
endif

gcc -w -Wfatal-errors -I../include.libs -O3 -o isint isint.c  									>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "    ERROR  install_pupsp3: operation failed [gcc isint]"                                              		>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit 255
endif

gcc -I../include.libs -D$ftype -w -Wfatal-errors -O3 -o isfloat isfloat.c  							>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "    ERROR  install_pupsp3: operation failed [gcc isfloat]"                                             		>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit 255 
endif

gcc -w -Wfatal-errors -O3 -o tas tas.c -lbsd											>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "    ERROR  install_pupsp3: operation failed gcc tas]"                                                     	>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit 255
endif

gcc -w -Wfatal-errors -I../include.libs -DHAVE_READLINE -O3 -o ask ask.c -lbsd -lreadline -ltermcap				>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "    ERROR  install_pupsp3: operation failed [gcc ask]"                                                  		>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit 255
endif

gcc -w -Wfatal-errors -O3 -o add add.c												>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "    ERROR  install_pupsp3: operation failed [gcc add]"                                  				>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit 255
endif

gcc -I../include.libs -D$ftype -w -Wfatal-errors -O3 -o fadd fadd.c                              				>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "    ERROR  install_pupsp3:  operation failed [gcc fadd]"                                 				>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit 255 
endif

gcc -w -Wfatal-errors -O3 -o sub sub.c                                                          				>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "    ERROR operation failed [gcc sub]"                                                    				>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit 255
endif

gcc -I../include.libs -D$ftype -w -Wfatal-errors -O3 -o fsub fsub.c                              				>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "    ERROR install_pupsp3: operation failed gcc fsub]"                                                   		>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit 255
endif

gcc -w -Wfatal-errors -O3 -o mul mul.c                                                          				>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "    ERROR install_pupsp3: operation failed [gcc mul]"                                                   		>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit 255 
endif

gcc -I../include.libs -D$ftype -w -Wfatal-errors -O3 -o fmul fmul.c                              				>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "    ERROR install_pupsp3: operation failed [gcc fmul]"                                                   		>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit 255
endif

gcc -w -Wfatal-errors -O3 -o div div.c                                                          				>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "    ERROR install_pupsp3: operation failed [gcc div]"                                                   		>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit 255
endif

gcc -I../include.libs -D$ftype -w -Wfatal-errors -O3 -o fdiv fdiv.c                              				>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "    ERROR install_pupsp3: operation failed [gcc fdiv]"                                                  		>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit 255
endif

gcc -I../include.libs -D$ftype -w -Wfatal-errors -O3 -o fint fint.c -lm                          				>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "    ERROR install_pupsp3: operation failed [gcc fint]"                                                   		>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit 255
endif

gcc -I../include.libs -D$ftype -w -Wfatal-errors -O3 -o intf intf.c                              				>>& $logFile
if($status != 0) then
        echo ""                                                                                 				>>& $logFile
        echo "    ERROR install_pupsp3: operation failed [gcc intf]"                                                  		>>& $logFile
        echo ""                                                                                 				>>& $logFile

        exit 255
endif

strip tas isatty iscard ishex isint isfloat ask add sub mul div fadd fsub fmul fdiv						>&  /dev/null
\mv tas isatty iscard ishex isint isfloat ask add sub mul div fadd fsub fmul fdiv fint intf $BINDIR				>&  /dev/null
if($status != 0) then
	echo ""                                                                                		 			>>& $logFile
	echo "    ERROR install_pupsp3: operation failed [install PUPS/P3 build tools"						>>& $logFile
	echo ""															>>& $logFile
        exit 255
endif

source ~/.cshrc															>&  /dev/null
pushd ..															>&  /dev/null
set path=($path . .. `pwd`)
popd																>&  /dev/null


#----------------
# Check buildlock
#----------------

if(-e BUILDLOCK) then


	#---------------------------
	# Check for interactive mode
	#---------------------------

	isatty stdin														>& /dev/null


	#--------------------------------------------------------
	# Error - cannot clear build lock in non-interactive mode
	#--------------------------------------------------------

	if ($? != 0) then
		echo ""														>&  $logFile
		echo "    ERROR cannot clear build lock in non-interactive mode"						>&  $logFile
		echo ""														>&  $logFile

		exit 255


	#---------------------------------------------
	# Interactive mode - build lock can be cleared
	#---------------------------------------------

	else

ask_again:

		set ret = `ask "    buildlock exists  -- delete?"`								>&  /dev/tty
		if($ret == "y" || $ret == "yes") then
			\rmdir BUILDLOCK											>&  /dev/null
		else if($ret == "q" || $ret == "quit" || $ret == "bye") then
			\rmdir BUILDLOCK        										>&  /dev/null
			exit 255 
		endif

		if($ret != "yes" && $ret != "y") then
			goto ask_again
		endif
	endif
endif


#---------------------------------------
# Apply build lock (atomic test and set)
#---------------------------------------

tas BUILDLOCK															>&  /dev/null

echo ""																>>& $logFile
echo "    ... Building compilation tools (upcase, downcase, prefix, suffix, striplc, error)"					>>& $logFile
echo ""																>>& $logFile

gcc -w -Wfatal-errors -I../include.libs  -O3 -o suffix  suffix.c     								>>& $logFile
if($status != 0) then
        echo ""                                              		                                                   	>>& $logFile
        echo "    ERROR operation failed [gcc suffix]"                                                              		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit 255
endif

gcc -w -Wfatal-errors -I../include.libs -O3 -o prefix  prefix.c -lbsd			    					>>& $logFile
if($status != 0) then
        exit 255
endif

gcc -w -Wfatal-errors -O3 -o striplc striplc.c    										>>& $logFile
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "    ERROR install_pupsp3: operation failed [gcc stripl]"                                                        	>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit 255
endif

gcc -w -Wfatal-errors -O3 -o downcase downcase.c  										>>& $logFile
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "    ERROR install_pupsp3: operation failed [gcc downcase]"                                                       	>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit 255
endif

gcc -w -Wfatal-errors -O3 -o upcase upcase.c      										>>& $logFile
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "    ERROR install_pupsp3: operation failed [gcc upcase]"                                                        	>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit 255
endif

gcc -w -Wfatal-errors -O3 -o error error.c      										>>& $logFile
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "    ERROR install_pupsp3: operation failed [gccc errpr]"                                                         	>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit 255
endif


strip suffix prefix striplc downcase upcase error										>&  /dev/null
\mv suffix prefix striplc downcase upcase error $BINDIR 									>&  /dev/null
if($status != 0) then
	echo ""                                                                                		 			>>& $logFile
	echo "    ERROR install_pupsp3: operation failed [install PUPS/P3 trivial support tools]"				>>& $logFile
	echo ""															>>& $logFile
        exit 255
endif

rehash

set rawtarget = $MACHTYPE.$OSTYPE 
set target    = $OSTYPE 
set utarget   = `upcase $target`
set target    = `downcase $target`
set arch      = $MACHTYPE 
set arch      = `upcase $arch`

echo ""																>>& $logFile
echo "    Configuring P3-20 for "$rawtarget "host"										>>& $logFile
echo ""																>>& $logFile

set buildtype = $1
if(! -e pupsuname) then
        echo "    ERROR install_pupsP3: cannot determine P3 architecture (no [pupsuname] file present)"				>>& $logFile

	\rmdir BUILDLOCK													>&  /dev/null
        exit 255
endif

if(! -e ../include.libs) then
	echo "    ERROR install_pupsP3: header file directory [../include.libs] missing"					>>& $logFile

	\rmdir BUILDLOCK													>&  /dev/null
	exit 255
endif

if(! -e $confdir) then
	echo "    ERROR install_pupsP3: configuration file directory [$confdir] missing"					>>& $logFile

	\rmdir BUILDLOCK													>&  /dev/null
	exit 255
endif

set crypt = ""
set crypt = "-lcrypt"


#--------------------------------
# Set to -lss_nis for NIS support
#--------------------------------

set nis    = ""
set AUTHEN = "-DSHADOW_SUPPORT"

echo ""																>>& $logFile
echo "    Authentication method support ($AUTHEN)"										>>& $logFile
echo ""																>>& $logFile

echo ""																>>& $logFile
echo "    ... Making P3-2025 binary directories for libraries and service functions"						>>& $logFile
echo ""																>>& $logFile

set pctarget = $target
set mode     = INVALID

echo "    Dynamic link library shared memory, checkpointing  and network support enabled"					>>& $logFile
echo ""                                                                                                                         >>& $logFile
echo "    ... Building dynamic P3/PSRP for network clustered MIMD environment"							>>& $logFile
echo ""																>>& $logFile
set pctarget = $target.cluster

pushd ..															>&  /dev/null

\rm -rf bin lib															>&  /dev/null
if(! -e lib.$arch.$pctarget) then
	mkdir lib.$arch.$pctarget 												>&  /dev/null
endif
ln -s lib.$arch.$pctarget lib 													>&  /dev/null

if(! -e bin.$arch.$pctarget) then
	mkdir bin.$arch.$pctarget												>&  /dev/null
endif
ln -s bin.$arch.$pctarget bin													>&  /dev/null

if($buildtype != "") then
   ls -s btype $buildtype													>&  /dev/null
endif

if(-e lib.$arch.$pctarget && `ls -l lib.$arch.$pctarget | grep "total 0"` == "") then
	\rm -f lib.$arch.$pctarget/*												>&  /dev/null
endif

if(-e bin.$arch.$pctarget && `ls -l bin.$arch.$pctarget | grep "total 0"` == "") then
	\rm -f bin.$arch.$pctarget/*												>&  /dev/null
endif

popd																>&  /dev/null




#################################
#### Configuration utilities ####
#################################

echo ""																>>& $logFile
echo "    ... Compiling P3-2025 configure and binary time stamping utilities"							>>& $logFile
echo ""																>>& $logFile


#---------
# pupsconf
#---------

set target = `upcase $target`
gcc -w -Wfatal-errors -DDEFAULT_CONFIGDIR=$projectPath -I../include.libs -D$target -O3 -o pupsconf pupsconf.c -lbsd	  	>>& $logFile
endif
if($status != 0) then
        echo ""                         		                                                                        >>& $logFile
        echo "    ERROR install_pupsP3: operation failed [gcc pupsconf]"       		                                        >>& $logFile
        echo ""                                                        			                                        >>& $logFile

        exit 255
endif


#----------
# configure
#----------

\rm $BINDIR/configure														>&  /dev/null
sed "s|CONFIG_PATH|$PROJECTDIR|g; s|INC_PATH|$INCDIR|g" <configure.in >$$
\mv $$ configure														>&  /dev/null

strip pupsconf															>&  /dev/null
\mv pupsconf $BINDIR 														>&  /dev/null
if($status != 0) then
        echo ""                         		                                                                        >>& $logFile
        echo "    ERROR install_pupsP3: operation failed [install pupsconf]"                                                    >>& $logFile
        echo ""                                                        			                                        >>& $logFile

        exit 255
endif


#-------
# vstamp
#-------

gcc -w -Wfatal-errors -D$target -I../include.libs -O3 -o vstamp vstamp.c							>>& $logFile
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "    ERROR  install_pupsP3: operation failed [gcc vstamp]"                                                		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit 255
endif

strip vstamp															>&  /dev/null
\mv vstamp $BINDIR 														>&  /dev/null
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "    ERROR  install_pupsP3: operation failed [install vstamp]"                                                     >>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit 255
endif


#-------
# vtagup
#-------

gcc -w -Wfatal-errors -D$target -I../include.libs -O3 -o vtagup vtagup.c  							>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR  install_pupsP3: operation failed [gcc vtagup]"                                                              >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

strip vtagup															>&  /dev/null
\mv vtagup $BINDIR 														>&  /dev/null
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "    ERROR  install_pupsP3: operation failed [install vtagup]"                                               	>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit 255
endif

if($build_ckpt == TRUE) then
	echo ""															>>& $logFile
	echo "    ... Set version number for state save (Criu) checkpoints"                                                     >>& $logFile
	echo ""															>>& $logFile
	vtagup SSAVETAG utilib.c                                                                                                >&  /dev/null
endif


#-----
# pc2c
#-----

echo ""																>>& $logFile
echo "    ... Building P3 Dynamic C [D] to ANSI C translator (language level support for dynamic functions and threads)"	>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -I. -I../include.libs -D$target -D$arch  -O3 -o pc2c pc2c.c -lbsd						>>& $logFile
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "    ERROR  install_pupsP3: operation failed [gcc pc2c]"                                                  		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit 255
endif

strip pc2c 
\mv pc2c $BINDIR 
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "    ERROR install_pupsP3: operation failed [install pc2c]"                                                        >>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit 255
endif




##########################################
#### Application generator frameworks ####
##########################################

echo ""																>>& $logFile
echo "    ... Building P3 skeleton application generator (generates P3 application source frameworks)"				>>& $logFile
echo ""																>>& $logFile


#-------
# appgen
#-------

gcc  -w -Wfatal-errors -DDEFAULT_CONFIGDIR=$projectPath -I. -I../include.libs -D$target -D$arch \
								     -O3 -o appgen appgen.c -lbsd				>>& $logFile
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "    ERROR install_pupsP3: operation failed [gcc appgen]"                                                          >>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit 255
endif

strip appgen 															>&  /dev/null
\mv appgen $BINDIR 														>&  /dev/null
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "    ERROR install_pupsP3: operation failed [install appgen]"                                                      >>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit 255
endif


#-------
# libgen
#-------

echo ""																>>& $logFile
echo "    ... Building P3 skeleton library generator (generates P3 library source frameworks)"					>>& $logFile
echo ""																>>& $logFile

gcc  -w -Wfatal-errors -DDEFAULT_CONFIGDIR=$projectPath -I. -I../include.libs -D$target \
						     -O3 -D$arch -o libgen libgen.c -lbsd					>>& $logFile
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "    ERROR install_pupsP3: operation failed [gcc libgen]"                                                      	>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit 255
endif

strip libgen															>&  /dev/null
\mv libgen $BINDIR 														>&  /dev/null
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "    ERROR install_pupsP3: operation failed [install libgen]"                                                     	>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit 255
endif


#-------
# dllgen
#-------

echo ""                                                                                                                         >>& $logFile
echo "    ... Building P3 skeleton application DLL generator (generates P3 DLL source frameworks)"				>>& $logFile
echo ""                                                                                                                         >>& $logFile

gcc  -w -Wfatal-errors -DDEFAULT_CONFIGDIR=$projectPath -I. -I../include.libs -D$target -D$arch \
								     -O3 -o dllgen dllgen.c -lbsd				>>& $logFile
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "    ERROR install_pupsP3: operation failed [gcc dllgen]"                                                		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit 255
endif

strip dllgen 															>&  /dev/null
\mv dllgen $BINDIR 														>&  /dev/null
if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "    ERROR install_pupsP3: operation failed [install dllgen]"                                            		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit 255
endif


sed s/\$ARCH/$arch/g <$confdir/$pctarget >$confdir/$arch.$pctarget




###############################
#### Bubble memory support ####
###############################
#----------------------------
# Build bubble memory library 
#----------------------------

if($sanitize == FALSE) then
    echo ""															>>& $logFile
    echo "    ... Building P3 dynamic bubble memory allocation library"								>>& $logFile
    echo ""															>>& $logFile

    $BINDIR/pupsconf $arch.$pctarget Make_bubble.in Makefile									>&  /dev/null 
    if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "    ERROR install_pupsP3: operation failed [generate bubble Makefile]"                                  		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

    	exit 255
    endif

    make -j$nCores 														>>& $logFile
    if($status != 0) then
        echo ""                                                                                                 		>>& $logFile
        echo "    ERROR install_pupsP3: operation failed [make bubble memory allocator]"                               		>>& $logFile
        echo ""                                                                                                 		>>& $logFile

        exit 255
    endif

    make cleanall 														>>& $logFile
    echo ""															>>& $logFile
    echo ""															>>& $logFile
endif




##############################################
#### Service functions (not psrp enabled) ####
##############################################

#-------
# hupter
#-------

echo "    Network support enabled (Building P3/PSRP for hetrogenous network MIMD environment)"					>>& $logFile
echo ""																>>& $logFile
echo "" 															>>& $logFile

echo ""																>>& $logFile
echo "    ... Building P3 HUP to TERM signal relay (relays HUP as TERM to payload process)"					>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o hupter hupter.c						>>& $logFile
if($status != 0) then
	echo ""                                                                                                                 >>& $logFile
	echo "    ERROR install_pupsP3: operation failed [gcc hupter]"                                                          >>& $logFile
	echo ""                                                                                                                 >>& $logFile

	exit 255
endif
strip hupter															>&  /dev/null


#------
# nkill
#------

echo ""																>>& $logFile
echo "    ... Building P3 network signal distributor [nkill] (sends signals to PID or process name on [remote] host)"		>>& $logFile
echo ""                                                                                                                     	>>& $logFile

gcc -w -Wfatal-errors -D$utarget -D$arch $AUTHEN -DSSH_SUPPORT -DHAVE_PROC_FS -O3 -I.        \
				          -I../include.libs -o nkill nkill.c $crypt $nis -lbsd  				>>& $logFile
if($status != 0) then
	echo ""                                                                                                                 >>& $logFile
	echo "    ERROR install_pupsP3: operation failed [gcc nkill]"                                                           >>& $logFile
	echo ""                                                                                                                 >>& $logFile

	exit 255
endif
strip nkill															>&  /dev/null

set target = `downcase $target`


#---------
# stripper
#---------

echo ""																>>& $logFile
echo "    ... Building P3 text file stripper [stripper] (strips comments from files in VDM's)"					>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o stripper stripper.c 						>>& $logFile
if($status != 0) then
	echo ""                                                                                                                 >>& $logFile
	echo "    ERROR install_pupsP3: operation failed [gcc stripper]"                                                        >>& $logFile
	echo ""                                                                                                                 >>& $logFile

	exit 255
endif
strip stripper															>&  /dev/null


#----
# p3f
#----

echo ""																>>& $logFile
echo "    ... Building p3f (checks if process is P3 aware)"									>>& $logFile
echo ""																>>& $logFile

gcc -w -Wfatal-errors -D$utarget -D$arch -DSSH_SUPPORT -DHAVE_PROC_FS -O3 -I.    \
			         -I../include.libs -o p3f p3f.c $crypt $nis  -lbsd 						>>& $logFile
if($status != 0) then
	echo ""                                                                                                                 >>& $logFile
	echo "    ERROR install_pupsP3: operation failed [gcc p3f]"                                                             >>& $logFile
	echo ""                                                                                                                 >>& $logFile

	exit 255
endif
strip p3f					   	 									>&  /dev/null

set target = `downcase $target`


#--------
# lyosome
#--------
 
echo ""																>>& $logFile
echo "    ... Building P3 lightweight file homeostat [lyosome] (protects a file for specified time period)"			>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o lyosome lyosome.c	-lbsd					>>& $logFile
if($status != 0) then
	echo ""                                                                                                                 >>& $logFile
	echo "    ERROR install_pupsP3: operation failed [gcc lyosome]"                                                         >>& $logFile
	echo ""                                                                                                                 >>& $logFile

exit 255
endif
strip lyosome															>&  /dev/null


#----------
# phagocyte
#----------

echo ""																>>& $logFile
echo "    ... Building P3 lightweight process homeostat [phagocyte] (removes damaged processes)"				>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -D$utarget -D$arch -D$ftype -O3 -I../include.libs -o phagocyte phagocyte.c -lbsd				>>& $logFile
if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "    ERROR install_pupsP3: operation failed [gcc phagocyte]"                                                       >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit 255
endif
strip phagocyte															>&  /dev/null


#--------
# softdog
#--------

echo ""																>>& $logFile
echo "    ... Building P3 software watchdog (removes damaged processes)"							>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o softdog softdog.c -lbsd					>>& $logFile
if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "    ERROR install_pupsP3: operation failed [gcc phagocyte]"                                                       >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit 255
endif
strip softdog															>&  /dev/null


#-------
# kepher
#-------

echo ""																>>& $logFile
echo "    ... Building P3 lightweight garbage collector [kepher] (removes stale temporary files)"				>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o kepher kepher.c -lbsd						>>& $logFile
if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "    ERROR install_pupsP3: operation failed [gcc kepher]"                                                          >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit 255
endif
strip kepher                      												>&  /dev/null


#-----
# leaf
#-----

echo ""																>>& $logFile
echo "    ... Building P3 sub-pathname extractor [leaf] (extracts apical twigs from pathname branches)"				>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o leaf leaf.c -lbsd			     			>>& $logFile
if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "    ERROR install_pupsP3: operation failed [gcc leaf]"                                                            >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit 255
endif
strip leaf                        												>&  /dev/null

echo ""																>>& $logFile
echo "    ... Building P3 basal-path extractor [branch] (extracts basal path from pathname branches)"				>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o branch branch.c -lbsd		   				>>& $logFile
if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "    ERROR install_pupsP3: operation failed [gcc branch]"                                                          >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit 255
endif
strip branch                       												>&  /dev/null


#---------
# psrptool
#---------
 
echo ""																>>& $logFile
echo "    ... Building psrptool (starts psrp client process in new [vte] window)"						>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o psrptool psrptool.c -lbsd			 		>>& $logFile
if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "    ERROR install_pupsP3: operation failed gcc psrptool]"                                                         >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit 255
endif
strip psrptool															>&  /dev/null


#-----------
# servertool
#-----------

echo ""																>>& $logFile
echo "    ... Building servertool (starts psrp server process in new [xterm] window)"						>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o servertool servertool.c -lbsd					>>& $logFile
if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "    ERROR install_pupsP3: operation failed [gcc servertool]"                                                      >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit 255
endif
strip servertool														>&  /dev/null


#-----
# farm
#-----

echo ""																>>& $logFile
echo "    ... Building P3 farm (permits definition of network-wide [homogenous] process farms)"			        	>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -D$utarget -D$arch -DSSH_SUPPORT -O3 -I../include.libs -o farm farm.c $crypt -lbsd			>>& $logFile
if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "    ERROR install_pupsP3: operation failed [gcc farm]"                                                            >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit 255
endif
strip farm	   													        >&  /dev/null


#------
# htype
#------

echo ""                                                                                                                     	>>& $logFile
echo "    ... Building htype (tests if Linux instance is running in container or on regular host)"                          	>>& $logFile
echo ""                                                                                                                     	>>& $logFile
gcc -w -Wfatal-errors -O3 -o htype htype.c                                                                                  	>>& $logFile
if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "    ERROR install_pupsP3: operation failed [gcc htype]"                                                           >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit 255
endif
strip htype 															>&  /dev/null


#------
# ssave
#------

if($build_ckpt == TRUE) then
       echo ""                                                                                                                  >>& $logFile
       echo "    ... Building ssave (saves process state via Criu)"						                >>& $logFile
       echo ""                                                                                                                  >>& $logFile
       vtagup SSAVETAG ssave.c													>&  /dev/null 
       gcc -w -Wfatal-errors -O3 -o ssave ssave.c -lbsd										>>& $logFile
       if($status != 0) then
		echo ""                                                                                                         >>& $logFile
          	echo "    ERROR install_pupsP3: operation failed [gcc ssave]"                                                   >>& $logFile
          	echo ""                                                                                                         >>& $logFile

          	exit 255
       endif
       strip ssave 

       echo ""                                                                                                                  >>& $logFile
       echo "    ... Building restart (restarted Criu checkpoints)"						                >>& $logFile
       echo ""                                                                                                                  >>& $logFile
       vtagup SSAVETAG restart.c												>&  /dev/null
       gcc -w -Wfatal-errors -O3 -o restart restart.c -lbsd									>>& $logFile
       if($status != 0) then
		echo ""                                                                                                         >>& $logFile
		echo "    ERROR install_pupsP3: operation failed [gcc restart]"                                                 >>& $logFile
		echo ""                                                                                                         >>& $logFile

		exit 255
       endif
       strip restart 
 endif


#--------
# cpuload
#--------

echo ""																>>& $logFile
echo "    ... Building cpuload (monitors server cpu loading)"									>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -D$utarget -D$arch -DSSH_SUPPORT -O3 -I../include.libs -o cpuload cpuload.c -lbsd		 		>>& $logFile
if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "    ERROR install_pupsP3: operation failed [gcc cpuload]"                                                         >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit 255
endif
strip cpuload															>&  /dev/null


#----
# lol
#----

echo ""																>>& $logFile
echo "    ... Building lol (detects whether owner of a lid associated with a simple link lock is alive)"			>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -D$utarget -D$arch -DSSH_SUPPORT -O3 -I../include.libs -o lol lol.c -lbsd		 			>>& $logFile
if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "    ERROR install_pupsP3: operation failed [gcc lol]"                                                             >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit 255
endif
strip lol															>&  /dev/null


#----
# gob
#----

echo ""																>>& $logFile
echo "    ... Building gob (provides named gobhole FIFO allowing asynchronous data transfer to pipeline)"			>>& $logFile
 
echo ""																>>& $logFile
gcc -w -Wfatal-errors -D$utarget -D$arch -DSSH_SUPPORT -O3 -I../include.libs -o gob gob.c -lbsd		 			>>& $logFile
if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "    ERROR install_pupsP3: operation failed [gcc gob]"                                                             >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit 255
endif
strip gob															>&  /dev/null


#-----
# arse
#-----

echo ""																>>& $logFile
echo "    ... Building arse (provides named arse hole FIFO allowing asynchronous data transfer from pipeline)"			>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -D$utarget -D$arch -DSSH_SUPPORT -O3 -I../include.libs -o arse arse.c -lbsd				>>& $logFile
if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "    ERROR install_pupsP3: operation failed [gcc arse]"                                                            >>& $logFile
        echo ""                                                                                                                 >>& $logFile

    	exit 255
endif
strip arse															>&  /dev/null


#------
# mktty
 #------

echo ""																>>& $logFile
echo "    ... Building P3 tty device homeostat [mktty] (restores /dev/tty if it is deleted)"					>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -D$utarget -D$arch -DSSH_SUPPORT -O3 -I../include.libs -o mktty mktty.c  					>>& $logFile
if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "    ERROR install_pupsP3: operation failed gcc mktty]"                                                            >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit 255
endif
strip mktty															>&  /dev/null


#-------
# mkfile
#-------

echo ""																>>& $logFile
echo "    ... Building P3 file creator [mkfile] (creates arbitrary files)"							>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -D$utarget -D$arch -O3 -I../include.libs -o mkfile mkfile.c						>>& $logFile
if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "    ERROR install_pupsP3: operation failed [mklfile]"                                                             >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit 255
endif
strip mkfile															>&  /dev/null


#----------
# gethostip
#----------

echo ""																>>& $logFile
echo "    ... Building gethostip (gets I.P. address information for DHCP clients)"						>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -I. -I../include.libs -D$target -O3 -o gethostip gethostip.c -lbsd					>>& $logFile
if($status != 0) then
	echo ""															>>& $logFile
	echo "    ERROR install_pupsP3: operation failed [gcc gethostip]"							>>& $logFile
	echo ""															>>& $logFile

	exit 255
endif
strip gethostip															>&  /dev/null


#---------
# catfiles
#---------

echo ""																>>& $logFile
echo "    ... Building catfiles (appends two files preserving inode of first)"							>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -I. -I../include.libs -D$utarget -D$arch -O3 -o catfiles catfiles.c  					>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [gcc catfiles]"                                                             >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif
strip catfiles															>&  /dev/null


#------------
# new_session
#------------

echo ""																>>& $logFile
echo "    ... Building new_session (creates new session)"									>>& $logFile
echo ""																>>& $logFile
gcc -w -Wfatal-errors -D$utarget -D$arch -I. -O3 -o new_session new_session.c							>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed gcc new_session]"                                                           >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif
strip new_session 														>&  /dev/null


#----------------------
# Install service tools
#----------------------

\mv gethostip hupter nkill htype farm cpuload lol gob arse mktty mkfile lyosome phagocyte softdog kepher leaf branch psrptool    \
   									 servertool p3f stripper catfiles new_session $BINDIR	>&  /dev/null

if($status != 0) then
        echo ""                                                                                                                 >>& $logFile
        echo "    ERROR install_pupsP3: operation failed [install PUPS/P3 support tools]"                                       >>& $logFile
        echo ""                                                                                                                 >>& $logFile

        exit 255
endif

if($build_ckpt == TRUE) then
        \mv ssave restart $BINDIR												>&  /dev/null

        if($status != 0) then
		echo ""                                                                                                         >>& $logFile
		echo "    ERROR install_pupsP3: operation failed [install PUPS/P3 checkpoint support tools]"                    >>& $logFile
		echo ""                                                                                                         >>& $logFile

		exit 255
	endif
endif




#############################################
#### Bubble memory compatibility support ####
#############################################
#---------------------------------
# Dummy bubble memory library stub
#---------------------------------

if($sanitize == TRUE || $nobubble == TRUE) then
echo ""                                                                                                                         >>& $logFile
echo "    ... Making nobubble compatability library"                                                                            >>& $logFile
echo ""                                                                                                                         >>& $logFile
$BINDIR/pupsconf $arch.$pctarget Make_nobubble.in Makefile                                                                      >&  /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [configure nobubble]"                                                       >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif
make -j$nCores                                                                                                                  >>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [make nobubble]"                                                            >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif
endif




#################################
#### Persistent heap support ####
#################################
#------------------------
# Persistent heap library
#------------------------

echo ""																>>& $logFile
echo "    ... Making persistent heap library"											>>& $logFile
echo ""																>>& $logFile
$BINDIR/pupsconf $arch.$pctarget Make_phmalloc.in Makefile									>&  /dev/null 
if($status != 0) then
      echo ""                                                                                                           	>>& $logFile
      echo "    ERROR install_pupsP3: operation failed [configure phmalloc]"                                            	>>& $logFile
      echo ""                                                                                                           	>>& $logFile

      exit 255
endif

make -j$nCores 															>>& $logFile

if($status != 0) then
      echo ""                                                                                                           	>>& $logFile
      echo "    ERROR install_pupsP3: operation failed make phmalloc]"                                                  	>>& $logFile
      echo ""                                                                                                           	>>& $logFile

      exit 255
endif

make clean															>>& $logFile
echo ""																>>& $logFile




#################################
#### Build PUPS/P3 libraries ####
#################################

echo ""																>>& $logFile
echo "    ... Making P3-2025 libraries"												>>& $logFile
echo ""																>>& $logFile


#-----------
# cmain stub
#-----------
#------
# Debug
#------
if ($debug == TRUE) then
	echo "    ... compiling cmain [debug format, bubble memory support]"							>>& $logFile
	gcc -DPTHREAD_SUPPORT -DBUBBLE_MEMORY_SUPPORT -g -I../include.libs -c cmain.c						>>& $logFile


#--------
# Santize
#--------

else if ($sanitize == TRUE) then
	echo "    ... compiling cmain [sanitize format]"									>>& $logFile
	gcc -DPTHREAD_SUPPORT -g -I../include.libs -c cmain.c									>>& $logFile


#-----------
# Production
#-----------

else
	echo "    ... compiling cmain"												>>& $logFile
	gcc -DPTHREAD_SUPPORT -DBUBBLE_MEMORY_SUPPORT -O3 -I../include.libs -c cmain.c						>>& $logFile
endif
mv cmain.o ../lib


#----------------------------------
# Generate libraries from templates
#----------------------------------

echo "    ... generating libraries from templates"										>>& $logFile
sed s/VDIM/3/g < veclib.c.in                 > vec3lib.c
sed s/VDIM/3/g < ../include.libs/vector.h.in > ../include.libs/vector3.h


#--------------------------
# Compile PUPS/P3 libraries
#--------------------------

if ($debug == TRUE) then
	echo "    ... compiling PUPS/P3 libraries [debug format]"								>>& $logFile
else
	echo "    ... compiling PUPS/P3 libraries"										>>& $logFile
endif

$BINDIR/pupsconf $arch.$pctarget Make_libs.in Makefile										>&  /dev/null 
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [configure PUpS/P3 libraries]"                                              >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

make -j$nCores															>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [make PUPS/P3 libaries]"                                                    >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif




############################################
#### Build PUPS/P3 service applications ####
############################################

echo ""																>>& $logFile
echo "    ... Making P3-2025 service applications"										>>& $logFile
echo ""																>>& $logFile


#------------------------
# Build psrp client shell 
#------------------------

echo "    ... Building P3-2025 interaction client (allows user to manipulate arbitrary processes)"				>>& $logFile
echo ""

$BINDIR/pupsconf $arch.$pctarget Make_psrp.in Makefile										>&  /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [configure psrp client]"                                                    >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

make -j$nCores   														>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [make psrp client]"                                                         >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

if($debug == TRUE || $sanitize == TRUE) then
	make unstripped  													>>& $logFile
else
	make install														>>& $logFile
endif
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [install psrp client]"                                                      >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

make cleanall   														>>& $logFile


#---------------------------------------
# Build maggot (stale resource recylcer) 
#---------------------------------------

echo ""																>>& $logFile
echo "    ... Building P3-2025 maggot (removes stale PSRP resources from system)"						>>& $logFile
echo ""																>>& $logFile
$BINDIR/pupsconf $arch.$pctarget Make_maggot.in Makefile									>&  /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [configure maggot]"                                                         >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

make -j$nCores     														>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [make maggot]"                                                              >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

if($debug == TRUE || sanitize == TRUE) then
	make unstripped  													>>& $logFile
else
	make install														>>& $logFile
endif
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [install maggot]"                                                           >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

make cleanall  															>>& $logFile


#----------------------------------------------
# Build embryo (uncommitted dynamic application 
#----------------------------------------------

echo ""																>>& $logFile
echo "    ... Building embryo application (an unassigned dynamically programmable server)"					>>& $logFile
echo ""																>>& $logFile

$BINDIR/pupsconf $arch.$pctarget Make_embryo.in Makefile									>&  /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [configure embryo]"                                                         >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

make -j$nCores    														>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [make embryo]"                                                              >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

if($debug == TRUE || $sanitize == TRUE) then
	make unstripped  													>>& $logFile
else
	make install														>>& $logFile
endif
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [install embryo]"                                                           >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

make cleanall  															>>& $logFile


#--------------------------------
# Build pass (pipeline) processor 
#--------------------------------

echo ""																>>& $logFile
echo "    ... Building pass application (executes arbitrary command pipeline connected to P3/PSRP server)"			>>& $logFile
echo ""
$BINDIR/pupsconf $arch.$pctarget Make_pass.in Makefile										>&  /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [configure pass]"                                                           >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

make -j$nCores          													>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [make pass]"                                                                >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

if($debug == TRUE || $sanitize == TRUE) then
	make unstripped  													>>& $logFile
else
	make install														>>& $logFile
endif
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [install pass]"                                                             >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

make cleanall   														>>& $logFile


#-------------------------------------
# Build fsw (smart filesystem watcher) 
#-------------------------------------

echo ""																>>& $logFile
echo "    ... Building P3-2025 file system watcher (intelligently deals with full filesystems)"					>>& $logFile
echo ""																>>& $logFile
$BINDIR/pupsconf $arch.$pctarget Make_fsw.in Makefile										>&  /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [configure fsw]"                                                            >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

make -j$nCores      														>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed make fsw]"                                                                  >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

if($debug == TRUE || $sanitize == TRUE) then
	make unstripped  													>>& $logFile
else
	make install														>>& $logFile
endif
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [install fsw]"                                                              >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

make cleanall  															>>& $logFile


#-----------------------------------------
# Build protect (arbitrary file protector) 
#-----------------------------------------

echo ""																>>& $logFile
echo "    ... Building P3-2025 file/directory protector (homeostatic protection for files and directories)"			>>& $logFile
echo ""																>>& $logFile
$BINDIR/pupsconf $arch.$pctarget Make_protect.in Makefile									>&  /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR  install_pupsP3: operation failed [configure protect]"                                                       >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

make -j$nCores    														>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [make protect]"                                                             >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

if($debug == TRUE || $sanitize == TRUE) then
	make unstripped  													>>& $logFile
else
	make install														>>& $logFile
endif
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [install protect]"                                                          >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

make cleanall   														>>& $logFile


#----------------------------------
# Build xcat (extended cat command) 
#----------------------------------

echo ""																>>& $logFile
echo "    ... Building P3-2025 extended file catenator (replacement for cat command))"						>>& $logFile
echo ""																>>& $logFile
$BINDIR/pupsconf $arch.$pctarget Make_xcat.in Makefile										>&  /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [configure xcat]"                                                           >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

make -j$nCores          													>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [make xcat]"                                                                >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

if($debug == TRUE || $sanitize == TRUE) then
	make unstripped  													>>& $logFile
else
	make install														>>& $logFile
endif
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [install xcat]"                                                             >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

make cleanall    														>>& $logFile


#----------------------------------
# Build xtee (extended tee command) 
#----------------------------------

echo ""																>>& $logFile
echo "    ... Building P3-2025 extended pipeline tee (replacement for tee command))"						>>& $logFile
echo ""																>>& $logFile

$BINDIR/pupsconf $arch.$pctarget Make_xtee.in Makefile										>&  /dev/null
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [configure xtee]"                                                           >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

make -j$nCores          													>>& $logFile
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [make xtee]"                                                                >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

if($debug == TRUE || $sanitize == TRUE) then
	make unstripped  													>>& $logFile
else
	make install														>>& $logFile
endif
if($status != 0) then
   echo ""                                                                                                                      >>& $logFile
   echo "    ERROR install_pupsP3: operation failed [install xtee]"                                                             >>& $logFile
   echo ""                                                                                                                      >>& $logFile

   exit 255
endif

make cleanall   														>>& $logFile




############################
#### Final installation ####
############################
#-----------------------------------------------
# Copy PSRP bash profile to local home directory
#-----------------------------------------------

echo ""																>>& $logFile
echo "    ... installing P3 bash profile in '$HOME'"										>>& $logFile
\cp psrp_bash_profile $HOME/.psrp_bash_profile											>&  /dev/null
if($status < 0) then
    echo ""                                                                                                             	>>& $logFile
    echo "    ERROR install_pupsP3: operation failed [install PUPS/P3 bash profile]"                                          	>>& $logFile
    echo ""                                                                                                             	>>& $logFile

    exit 255
endif


#---------------------------
# Build PUPS/P3 version tool
#---------------------------

echo "    ... building P3 version tool"												>>& $logFile
./pupsp3_setversion.sh


#----------------------------------------
# Copy tool script to local bin directory
#----------------------------------------

echo "    ... installing P3 scripts in '$BINDIR'"										>>& $logFile
chmod +x uninstall_pups mantohtml manc.sh C2MAN.sh  tall pupsuname configure somake dllbuild pupsp3-version			>&  /dev/null
\cp  uninstall_pupsP3.csh mantohtml manc.sh C2MAN.sh  tall pupsuname configure somake pupsp3-version $BINDIR			>&  /dev/null
if($status < 0) then
    echo ""                                                                                                             	>>& $logFile
    echo "    ERROR install_pupsP3: operation failed [install PUPS/P3 tool scripts]"                                           	>>& $logFile
    echo ""                                                                                                             	>>& $logFile

    exit 255
endif


#-----------------------------------------------------------
# Copy project templates to local project template directory 
#-----------------------------------------------------------

echo "    ... installing P3 project templates in '$PROJECTDIR'"									>>& $logFile
\cp dll_skel.c dll_skelfunc.c lib_skel.c lib_skelfunc.c  Make_skelpapp.in  skelpapp.c $PROJECTDIR				>&  /dev/null
if($status != 0) then
    echo ""                                                                                                             	>>& $logFile
    echo "    ERROR install_pupsP3: operation failed [install project templates]"                                             	>>& $logFile
    echo ""                                                                                                             	>>& $logFile

    exit 255
endif


#--------------------------------------
# Copy distribution to binary directory
#--------------------------------------

echo "    ... installing P3 binaries in '$BINDIR'"										>>& $logFile	
pushd ../bin															>&  /dev/null 
\mv * $BINDIR 															>&  /dev/null
if($status != 0) then
    echo ""                                                                                                             	>>& $logFile
    echo "    ERROR install_pupsP3: operation failed [install PUPS/P3 distribution]"                                           	>>& $logFile
    echo ""                                                                                                             	>>& $logFile

    exit 255
endif

popd																>&  /dev/null


#------------------------------------
# Copy libraries to library directory
#------------------------------------

echo "    ... installing P3 libaries in '$LIBDIR'"										>>& $logFile
pushd ../lib															>&  /dev/null
\cp * $LIBDIR 															>&  /dev/null
if($status != 0) then
    echo ""                                                                                                             	>>& $logFile
    echo "    ERROR install_pupsP3: operation failed [install PUPS/P3 libraries]"                                              	>>& $logFile
    echo ""                                                                                                             	>>& $logFile

    exit 255
endif

popd																>&  /dev/null


#---------------------------------------
# Copy header files to include directory 
#---------------------------------------

echo "    ... installing P3 headers in '$HDRDIR'"										>>& $logFile
pushd ../include.libs														>&  /dev/null
\cp * $HDRDIR 															#>&  /dev/null
if($status != 0) then
    echo ""                                                                                                             	>>& $logFile
    echo "    ERROR install_pupsP3: operation failed [install PUPS/P3 headers]"                                                	>>& $logFile
    echo ""                                                                                                             	>>& $logFile

    exit 255
endif

popd																>&  /dev/null


#-------------------------------------------------
# Copy (bash) signal mappings to project directory
#-------------------------------------------------


echo "    ... installing P3 signal mappings in '$PROJECTDIR'"									>>& $logFile
\cp export_pupsp3_sig.sh unset_pupsp3_sig.sh export_pupsp3_sig.csh unexport_pupsp3_sig.csh  $PROJECTDIR				>& /dev/null
if($status != 0) then
    echo ""                                                                                                             	>>& $logFile
    echo "    ERROR install_pupsP3: operation failed [install PUPS/P3 signal mappings]"                                       	>>& $logFile
    echo ""                                                                                                             	>>& $logFile

    exit 255
endif


#--------------------------------
# Copy man pages to man directory
#--------------------------------

echo "    ... installing P3 manual pages in '$MANDIR'"										>>& $logFile
\cp ../man/man1/* $MANDIR/man1													>&  /dev/null
if($status != 0) then
    echo ""                                                                                                             	>>& $logFile
    echo "    ERROR install_pupsP3: operation failed [install PUPS/P3 man1 pages]"                                            	>>& $logFile
    echo ""                                                                                                             	>>& $logFile

    exit 255
endif

\cp ../man/man3/* $MANDIR/man3													>&  /dev/null
if($status != 0) then
    echo ""                                                                                                             	>>& $logFile
    echo "    ERROR install_pupsP3: operation failed [install PUPS/P3 man3 pages]"                                            	>>& $logFile
    echo ""                                                                                                             	>>& $logFile

    exit 255
endif

\cp ../man/man7/* $MANDIR/man7													>&  /dev/null
if($status != 0) then
    echo ""                                                                                                             	>>& $logFile
    echo "    ERROR install_pupsP3: operation failed [install PUPS/P3 man7 pages]"                                             	>>& $logFile
    echo ""                                                                                                             	>>& $logFile

    exit 255
endif


#---------------------------------------------------
# Debuggable/sanitized libraries cannot be installed 
#---------------------------------------------------

if($debug == FALSE && $sanitize == FALSE) then


	#-------------------------------
	# Install configuation directory
	#-------------------------------

	echo "    ... installing configuration directory (in '$PROJECTDIR')"							>>& $logFile
	pushd ../config.install													>&  /dev/null

	./install_config.sh $target $buildtype											>>& $logFile
	if($status != 0) then
		echo ""                                                                                                        	>>& $logFile
   		echo "    ERROR install_pupsP3: operation failed [install configuration directory]"                            	>>& $logFile
   		echo ""                                                                                                        	>>& $logFile

       		exit 255
	endif

	popd                    												>&  /dev/null
endif


#-------------------------------------
# Additions to .tchrc (for tcsh users)
#-------------------------------------

echo ""																>>& $logFile
echo "    ---------------------------------------------------------------------------------"					>>& $logFile
echo "    If you use tcsh Make sure your ~/.tcshrc script has the following lines in it:"					>>& $logFile
echo ""																>>& $logFile

if(`whoami` == root) then
        echo "    setenv P3_CONFDIR /usr/include/p3"										>>& $logFile

	if($MACHTYPE == x86_64 || $MACHTYPE == aarch64) then
        	echo "    setenv LD_LIBRARY_PATH /usr/lib64 (should be set by default)"						>>& $logFile
	else
        	echo "    setenv LD_LIBRARY_PATH /usr/lib (should be set by default)"						>>& $logFile
	endif

else
        echo "    setenv P3_CONFDIR ~/p3"											>>& $logFile
       	echo "    setenv LD_LIBRARY_PATH ~/lib"											>>& $logFile
endif

echo "    ---------------------------------------------------------------------------------"					>>& $logFile
echo ""																>>& $logFile


#------------------------------------------
# Additions to .profile (for bash/sh users)
#------------------------------------------

echo ""																>>& $logFile
echo "    ----------------------------------------------------------------------------------"					>>& $logFile
echo "    If you use bash/sh Make sure your ~/.profile script has the following lines in it:"					>>& $logFile
echo ""																>>& $logFile

if(`whoami` == root) then
        echo "    export P3_CONFDIR=/usr/include/p3"										>>& $logFile

	if($MACHTYPE == x86_64 || $MACHTYPE == aarch64) then
        	echo "    export LD_LIBRARY_PATH=/usr/lib64 (should be set by default)"						>>& $logFile
	else
        	echo "    export LD_LIBRARY_PATH=/usr/lib (should be set by default)"						>>& $logFile
	endif

else
        echo "    export P3_CONFDIR=$HOME/p3"											>>& $logFile
        echo "    export LD_LIBRARY_PATH=$HOME/lib"										>>& $logFile
	endif
endif

echo "    ----------------------------------------------------------------------------------"					>>& $logFile
echo ""																>>& $logFile


#---------------------
# Debuggable libraries
#---------------------

if($debug == TRUE) then
	echo ""															>>& $logFile
	echo "    ... Making P3-2025 libraries debuggable"									>>& $logFile
	echo ""                                                                                                                 >>& $logFile
	\rm ../lib.$arch.linux.cluster/sanitize											>&  /dev/null
	mkfile ../lib.$arch.linux.cluster/debug											>&  /dev/null


#--------------------
# Sanitized libraries
#--------------------

else if($sanitize == TRUE) then
	echo ""															>>& $logFile
	echo "    ... Making P3-2025 libraries debuggable and santized"								>>& $logFile
	echo ""                                                                                                                 >>& $logFile
	\rm ../lib.$arch.linux.cluster/debug											>&  /dev/null
	mkfile ../lib.$arch.linux.cluster/sanitize										>&  /dev/null


#-------------------
# Nobubble libraries
#-------------------

else if($nobubble == TRUE) then
	echo ""															>>& $logFile
	echo "    ... Making  P3-2025 production libraries without bubble memory support"					>>& $logFile
	echo ""                                                                                                                 >>& $logFile
	\rm ../lib.$arch.linux.cluster/debug											>&  /dev/null
	mkfile ../lib.$arch.linux.cluster/nobubble										>&  /dev/null



#---------------------
# Production libraries
#---------------------

else
	echo ""															>>& $logFile
	echo "    ... Making  P3-2025 production libraries"									>>& $logFile
	echo ""                                                                                                                 >>& $logFile
	\rm ../lib.$arch.linux.cluster/debug											>&  /dev/null
	\rm ../lib.$arch.linux.cluster/sanitize											>&  /dev/null
	\rm ../lib.$arch.linux.cluster/nobubble											>&  /dev/null
endif

if(`whoami` == root) then
	if($MACHTYPE == x86_64 || $MACHTYPE == aarch64) then

		echo "    ----------------------------------------------------------------------------------"			>>& $logFile
		echo "    make sure that /usr/lib64 is added to /etc/ld.so.conf"						>>& $logFile
		echo "    ----------------------------------------------------------------------------------"			>>& $logFile
		echo ""														>>& $logFile

		sleep 2


		#----------------------------------
		# Check install script is connected
		# to a terminal
		#----------------------------------

		isatty stdin													>& /dev/null
		if($? == 0)  then
			vi /etc/ld.so.conf
			ldconfig												>& /dev/null
		endif
	endif
endif

echo ""																>>& $logFile
echo "    ... build of P3-2025 libraries and services complete"									>>& $logFile
echo ""																>>& $logFile

\rmdir BUILDLOCK														>&/dev/null 
exit 0


#------------
# Abort build
#------------

abort_build:

echo ""                                                                                                                         >>& $logFile
echo "    *** interrupted [PUPSP3]"												>>& $logFile	
echo ""																>>& $logFile

\rmdir BUILDLOCK														>&  /dev/null 
exit 255 

