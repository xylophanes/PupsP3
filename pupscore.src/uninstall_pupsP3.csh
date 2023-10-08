#!/bin/tcsh
#
#-------------------------------------------
#  Uninstall PUPS/P3 and support components
#  $1 logfile
#
#  (C) M.A. O'Neill. Tumbling Dice 2000-2022
#-------------------------------------------
#--------------------
# Target architecture
#--------------------
set arch = `uname -a | awk '{print $12}'`


#-------------------
# Initialise logfile
#--------------------

if($1 == "" || $1 == null || $1 == /dev/null) then
	set logFi1e = /dev/null
else if($1 == "tty" || $1 == /dev/tty) then
	set logFile = /dev/tty
else
	set logFile = $1

        if(! $?cshlevel) then
                setenv cshlevel 1
		echo "" >& $logFile
	endif
endif


#-------
# Banner
#-------

echo ""																>>& $logFile		
echo "    P3-2022 (SUP3) uninstall script (C) M.A. O'Neill Tumbling Dice 2000-2022"						>>& $logFile
echo ""																>>& $logFile


#-----------------------------
# Locations of installed items
#-----------------------------
#-----
# Root
#-----
if(`whoami` == root) then
	set mandir  = /usr/share/man
	set bindir  = /usr/bin
	set sbindir = /usr/sbin

	if($arch == x86_64 || $arch == aarch64) then
		set libdir  = /usr/lib64
	else
		set libdir  = /usr/lib
	endif


#------------
# Normal user
#------------

else
	set mandir  = $HOME/man
	set bindir  = $HOME/bin
	set sbindir = $HOME/sbin

	if($arch == x86_64 || $arch == aarch64) then
		set libdir  = $HOME/lib64
	else
		set libdir  = $HOME/lib
	endif
endif


#-----------------------------
# Lists of items to be deleted
#-----------------------------
#-----
# man1
#-----
set man1_remove_list =   "hdid.1 ecrypt.1 pupsconf.1 vstamp.1 vtagup.1 p2c.1 appgen.1 libgen.1 dllgen.1                \
                          hupter.1 nkill.1 htype.1 farm.1 ssave.1 restart.1 cpuload.1 lol.1 gob.1 arse.1 mktty.1       \
                          mkfile.1 lyosome.1 psrptool.1 servertool.1 stripper.1 iscard.1 isint ishex.1                 \
                          isfloat.1 tas.1 ask.1 suffix.1 prefix.1 gethostip.1 catfiles.1 new_session.1                 \
                          leaf.1 branch.1 upcase.1 downcase.1 striplc.1 error.1 psrp.1 embryo.1 pass.1 maggot.1 fsw.1  \
                          protect.1 xcat.1 xtee.1 tcell.1 mantohtml.1 manc.1 tall.1 pupsuname.1 configure.1            \
                          somake.1"

#-----
# man3
#-----

set man3_remove_list  =  "casino.3 dlllib.3  hashlib.3  mvmlib.3  nfolib.3 larraylib.3 pheap.3 utilib.3                \
                          fhtlib.3 netlib.3 psrplib.3 utilib.3.grp veclib.3" 


#-----
# man7
#-----

set man7_remove_list =   "pups.7"


#---------------
# Binaries (bin)
#---------------

set bin_remove_list  =	"hdid ecrypt pupsconf vstamp vtagup p2c appgen libgen dllgen hupter nkill htype               \
                         farm ssave cpuload lol gob arse mktty mkfile lyosome psrptool servertool stripper iscard     \
                         isint ishex isfloat tas ask suffix prefix gethostip catfiles new_session upcase              \
                         leaf, branch downcase striplc error psrp embryo pass maggot fsw protect xcat xtee tcell      \
                         mantohtml manc tall pupsuname configure somake"


#----------------
# Binaries (sbin)
#----------------

set sbin_remove_list  = ""


#----------
# Libraries
#----------

set lib_remove_list =   "libpups.a libpups.so libbubble.a libbubble.so libphmalloc.a libphmalloc.so"


#--------------------
# Uninstall man pages
#--------------------
#-----
# man1
#-----

echo ""																>>& $logFile
echo "    ... removing 'man' pages (from '$mandir/man1')"									>>& $logFile
pushd $mandir/man1 														>&  /dev/null

foreach i ( $man1_remove_list )
	\rm -f $i 														>&  /dev/null 
end
popd   																>&  /dev/null


#-----
# man3
#-----

echo "    ... removing 'man' pages (from '$mandir/man3')"									>>& $logFile
pushd $mandir/man3 														>&  /dev/null

foreach i ( $man3_remove_list )
       	\rm -f $i 														>&  /dev/null
end
popd   																>&  /dev/null


#-----
# man7
#-----

echo "    ... removing 'man' pages (from '$mandir/man7')"									>>& $logFile
pushd $mandir/man7 														>&  /dev/null

foreach i ( $man7_remove_list )
	\rm -f $i														>&  /dev/null
end
popd 																>&  /dev/null


#-------------------------------
# Uninstall binaries and scripts 
#-------------------------------
#----
# bin
#----
echo "    ... removing binaries and scripts (from '$bindir')"									>>& $logFile
pushd $bindir 															>&  /dev/null
foreach i ( $bin_remove_list )
	\rm $i 															>&  /dev/null 
end
popd   																>&  /dev/null


#-----
# sbin
#-----

echo "    ... removing binaries and scripts (from '$sbindir')"									>>& $logFile
pushd $sbindir 															>&  /dev/null

foreach i ( $sbin_remove_list )
       	\rm $i 															>&  /dev/null
end
popd   																>&  /dev/null


#---------------------
# Uninstall libraries 
#---------------------

echo "    ... removing libraries (from '$libdir')"										>>& $logFile
pushd $libdir 															>&  /dev/null

foreach i ( $lib_remove_list )
       	\rm $i 															>&  /dev/null
end
popd																>&  /dev/null


#-----------------------------------------
#  Unintstall PUPS/P3 specific directories 
#-----------------------------------------

if(`whoami` == root) then
	echo "    ... removing PUPS/P3 project directory ('/p3')"								>>& $logFile
	\rm -rf /p3														>&  /dev/null
else
	echo "    ... removing PUPS/P3 project directory ('$HOME/p3')"								>>& $logFile
	\rm -rf $HOME/p3													>&  /dev/null
endif


#------------------------
#  Uninstall header files 
#------------------------

if(`whoami` == root) then
        echo "    ... removing PUPS/P3 header file directory ('/usr/include/p3')"						>>& $logFile
       	\rm -rf /usr/include/p3													>& /dev/null
else
        echo "    ... removing PUPS/P3 header files ('~/include/p3')"								>>& $logFile
       	\rm -rf $HOME/include/p3												>& /dev/null

endif


#------------------------------
# Remove soft dongle directory
#------------------------------

echo "    ... removing PUPS/P3 dongle directory ('`whoami`/.sdongles')"								>>& $logFile
if(-e `whoami`/.sdongles) then
	\rm -rf `whoami`/.sdongles												>& /dev/null 
endif

echo ""																>>& $logFile
echo "  ----------------------------------------------------------------------------------------------------------"		>>& $logFile
echo "  WARNING: if ssh or checkpoint components were installed by build_pups_services, you may want to"			>>& $logFile
echo "  remove them as well"													>>& $logFile
echo "  ----------------------------------------------------------------------------------------------------------"		>>& $logFile
echo ""																>>& $logFile

echo ""																>>& $logFile
echo "    Finished"														>>& $logFile
echo ""																>>& $logFile

exit 0
