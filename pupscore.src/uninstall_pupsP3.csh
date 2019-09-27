#!/bin/tcsh
#
#-----------------------------------------------------------------------------------
#  Uninstall PUPS/P3 and support components
#  (C) M.A. O'Neill. Tumbling Dice, 2009-2019
#-----------------------------------------------------------------------------------


#-----------------------------------------------------------------------------------
# Initialise logfile
#-----------------------------------------------------------------------------------

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

echo LOGFILE $logFile

echo ""																>>& $logFile		
echo "    P3-2019 (SUP3) uninstall script (C) M.A. O'Neill Tumbling Dice 2010-2019"						>>& $logFile
echo ""																>>& $logFile


#--------------------------------------------------------------------------------
# Locations of items (you may need to edit these for platforms other than Linux)
#--------------------------------------------------------------------------------

if(`whoami` == root) then
	set mandir  = /usr/share/man
	set bindir  = /usr/bin
	set sbindir = /usr/sbin
	set libdir  = /usr/lib
else
	set mandir  = ~/man
	set bindir  = ~/bin
	set sbindir = ~/sbin
	set libdir  = ~/lib
endif


#------------------------------
# Lists of items to be deleted
#------------------------------

set man1_remove_list =   "hdid.1 ecrypt.1 pupsconf.1 vstamp.1 vtagup.1 p2c.1 appgen.1 libgen.1 dllgen.1 hupter.1 nkill.1 htype.1 farm.1 ssave.1 restart.1 lol.1 gob.1 arse.1 mktty.1 mkfile.1 lyosome.1 psrptool.1 servertool.1 stripper.1 iscard.1 isint ishex.1 isfloat.1 tas.1 ask.1 suffix.1 prefix.1 gethostip.1 catfiles.1 new_session.1 upcase.1 downcase.1 striplc.1 error.1 psrp.1 embryo.1 pass.1 maggot.1 fsw.1 protect.1 xcat.1 xtee.1 tcell.1 mantohtml.1 manc.1 tall.1 pupsuname.1 configure.1 somake.1 WIPE.1 TMPWIPE.1 PSRPTOOL.1 P3L2MAN.1"

set man3_remove_list  =  "casino.3 dlllib.3  hashlib.3  mvmlib.3  nfolib.3 larraylib.3 pheap.3 utilib.3 fhtlib.3 netlib.3 psrplib.3 utilib.3.grp veclib.3" 

set man7_remove_list =   "pups.7"

set bin_remove_list  =	"hdid ecrypt pupsconf vstamp vtagup p2c appgen libgen dllgen hupter nkill htype farm ssave lol gob arse mktty mkfile lyosome psrptool servertool stripper iscard isint ishex isfloat tas ask suffix prefix gethostip catfiles new_session upcase downcase striplc error psrp embryo pass maggot fsw protect xcat xtee tcell mantohtml manc tall pupsuname configure somake WIPE TMPWIPE PSRPTOOL P3L2MAN"

set lib_remove_list =   "libpups.a libpups.so libbubble.a libbubble.so libphmalloc.a libphmalloc.so"


#---------------------
# Uninstall man pages
#---------------------

echo ""																>>& $logFile
echo "    ... removing 'man' pages (from '$mandir/man1')"									>>& $logFile
pushd $mandir/man1 														>& /dev/null

set eff_man1_remove_list = `sh -c "ls $man1_remove_list 2>/dev/null"`

if("$eff_man1_remove_list" != "") then
	foreach i (`ls $eff_man1_remove_list`)
		\rm -f $i 													>& /dev/null 

 		if($status != 0) then
 			echo "    uninstall WARNING: failed to remove '$i'"							>>& $logFile
 		endif
	end

	popd   															>& /dev/null
else
	echo "    ... nothing to do (no PUPS/P3 man pages found in '$mandir/man1')"						>>& $logFile
	echo ""
endif

echo "    ... removing 'man' pages (from '$mandir/man3')"									>>& $logFile
pushd $mandir/man3 >& /dev/null
set eff_man3_remove_list = `sh -c "ls $man3_remove_list 2>/dev/null"`

if("$eff_man3_remove_list" != "") then
	foreach i (`ls $eff_man3_remove_list`)
        	\rm -f $i 													>& /dev/null

         	if($status != 0) then
                 	echo "    uninstall WARNING: failed to remove '$i'"							>>& $logFile
         	endif
	end
else
	echo "    ... nothing to do (no PUPS/P3 man pages found in '$mandir/man3')"						>>& $logFile
	echo ""															>>& $logFile
endif

popd   																>& /dev/null

echo "    ... removing 'man' pages (from '$mandir/man7')"									>>& $logFile
pushd $mandir/man7 														>& /dev/null
set eff_man7_remove_list = `sh -c "ls $man7_remove_list 2>/dev/null"`

if("$eff_man7_remove_list" != "") then
	foreach i (`ls $eff_man7_remove_list`)
       		\rm -f $i													>& /dev/null

         	if($status != 0) then
                 	echo "    uninstall WARNING: failed to remove '$i' (not found)"						>>& $logFile
         	endif
	end
else
	echo "    ... nothing to do (no PUPS/P3 man pages found in '$mandir/man7')"						>>& $logFile
	echo ""															>>& $logFile
endif

popd   																>& /dev/null

\rm $$ >&/dev/null
popd   >& /dev/null


#--------------------------------
# Uninstall binaries and scripts 
#--------------------------------

echo "    ... removing binaries and scripts (from '$bindir')"									>>& $logFile
pushd $bindir 															>& /dev/null
set eff_bin_remove_list = `sh -c "ls $bin_remove_list 2>/dev/null"`

if("$eff_bin_remove_list" != "") then
	foreach i (`ls $eff_bin_remove_list`)
		\rm $i 														>& /dev/null 

 		if($status != 0) then
 			echo "    uninstall WARNING: failed to remove '$i' (not found)"						>>& $logFile
 		endif
	end
else
	echo "    ... nothing to do (no PUPS/P3 binaries found in '$bindir')"							>>& $logFile
	echo ""															>>& $logFile
endif

popd   																>& /dev/null

echo "    ... removing binaries and scripts (from '$sbindir')"									>>& $logFile
pushd $sbindir 															>& /dev/null
set eff_sbin_remove_list = `sh -c "ls $sbin_remove_list 2>/dev/null"`

if("$eff_sbin_remove_list" != "") then
	foreach i (`ls $eff_sbin_remove_list`)
        	\rm $i 														>& /dev/null

         	if($status != 0) then
                 	echo "    uninstall WARNING: failed to remove '$i' (not found)"						>>& $logFile
         	endif
	end
else
	echo "    ... nothing to do (no PUPS/P3 (system) binaries found in '$sbindir')"						>>& $logFile
	echo ""															>>& $logFile
endif

popd   																>& /dev/null


#---------------------
# Uninstall libraries 
#---------------------

echo "    ... removing libraries (from '$libdir')"										>>& $logFile
pushd $libdir 															>& /dev/null
set eff_lib_remove_list = `sh -c "ls $lib_remove_list 2>/dev/null"`

if("$eff_lib_remove_list" != "") then
	foreach i (`ls $eff_lib_remove_list`)
        	\rm $i 														>& /dev/null

        	if($status != 0) then
                	echo "    uninstall WARNING: failed to remove '$i' (not found)"						>>& $logFile
        	endif
	end
else
	echo "    ... nothing to do (no PUPS/P3 libraries found in '$libdir')"							>>& $logFile
        echo ""															>>& $logFile
endif

popd																>& /dev/null

#
#----------------------------------------------------------------------------------------------------
#   Unintstall PUPS/P3 specific directories etc
#----------------------------------------------------------------------------------------------------
#

if(`whoami` == root) then
	echo "    ... removing PUPS/P3 project directory ('/pups')"								>>& $logFile
	if(-e /pups) then
 		\rm -rf /pups													>& /dev/null
	else
		echo "    ... PUPS/P3 project directory ('/pups') not found"							>>& $logFile
                echo ""														>>& $logFile
        endif
else
	echo "    ... removing PUPS/P3 project directory ('~/pups')"								>>& $logFile
	if(-e ~/pups) then
		\rm -rf ~/pups													>& /dev/null
	else
		echo "    ... PUPS/P3 project directory ('~/pups') not found"							>>& $logFile
                echo ""														>>& $logFile
        endif

endif

#
#----------------------------------------------------------------------------------------------------
#  Uninstall header files ...
#----------------------------------------------------------------------------------------------------
#

if(`whoami` == root) then
        echo "    ... removing PUPS/P3 header file directory ('/usr/include/p3')"						>>& $logFile
	if(-e /usr/include/p3) then
        	\rm -rf /usr/include/p3												>& /dev/null
	else
		echo "    ... PUPS/P3 header file directory ('/usr/include/p3') not found"					>>& $logFile
		echo ""														>>& $logFile
	endif
else
        echo "    ... removing PUPS/P3 header files ('~/include/p3')"								>>& $logFile
	if(-e ~/include/p3) then
        	\rm -rf ~/include/p3												>& /dev/null
	else
                echo "    ... PUPS/P3 header file directory ('~/include/p3') not found"						>>& $logFile
                echo ""														>>& $logFile
        endif

endif


#------------------------------
# Remove soft dongle directory
#------------------------------

echo "    ... removing PUPS/P3 dongle directory ('`whoami`/.sdongles')"								>>& $logFile
if(-e `whoami`/.sdongles) then
	\rm -rf `whoami`/.sdongles												>& /dev/null 
else
	echo "    ... PUPS/P3 dongle directory ('`whoami`/.sdongles') not found"						>>& $logFile
	echo ""															>>& $logFile
endif

echo ""																>>& $logFile
echo "  ----------------------------------------------------------------------------------------------------------"		>>& $logFile
echo "  Warning: if ssh or checkpoint components were installed by build_pups_services, you may want to"			>>& $logFile
echo "  remove them as well."													>>& $logFile
echo "  ----------------------------------------------------------------------------------------------------------"		>>& $logFile
echo ""																>>& $logFile

echo ""																>>& $logFile
echo "    Finished"														>>& $logFile
echo ""																>>& $logFile

exit 0
