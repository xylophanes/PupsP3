#!/bin/tcsh
#
#####################################
# Export P3 signal mappings for csh 
# (C) M.A O'Neill, Tumbling Dice 2025
#####################################

#-------------------------------------
# This might be appliocation dependent
#-------------------------------------

set SIGRTMIN = 34


#-------------------------
# Generate signal mappings
#-------------------------

@ PUPSP3_SIGEVENT          = ( $SIGRTMIN + 4  )
@ PUPSP3_SIGPSRP           = ( $SIGRTMIN + 5  )
@ PUPSP3_SIGCHAN           = ( $SIGRTMIN + 6  )
@ PUPSP3_SIGINIT           = ( $SIGRTMIN + 7  )
@ PUPSP3_SIGCLIENT         = ( $SIGRTMIN + 8  )
@ PUPSP3_SIGTHREAD         = ( $SIGRTMIN + 9  )
@ PUPSP3_SIGSLAVE          = ( $SIGRTMIN + 10 )
@ PUPSP3_SIGCHECK          = ( $SIGRTMIN + 11 )
@ PUPSP3_SIGRESTART        = ( $SIGRTMIN + 12 )
@ PUPSP3_SIGALIVE          = ( $SIGRTMIN + 13 )
@ PUPSP3_SIGTHREADSTOP     = ( $SIGRTMIN + 14 )
@ PUPSP3_SIGTHREADRESTART  = ( $SIGRTMIN + 15 )
@ PUPSP3_SIGCRITICAL       = ( $SIGRTMIN + 16 )


#-----------------------
# Export signal mappings
#-----------------------

setenv PUPSP3_SIGEVENT          $PUPSP3_SIGEVENT
setenv PUPSP3_SIGPSRP           $PUPSP3_SIGPSRP
setenv PUPSP3_SIGCHAN           $PUPSP3_SIGCHAN
setenv PUPSP3_SIGINIT           $PUPSP3_SIGINIT
setenv PUPSP3_SIGCLIENT         $PUPSP3_SIGCLIENT
setenv PUPSP3_SIGTHREAD         $PUPSP3_SIGTHREAD
setenv PUPSP3_SIGSLAVE          $PUPSP3_SIGSLAVE
setenv PUPSP3_SIGCHECK          $PUPSP3_SIGCHECK
setenv PUPSP3_SIGRESTART        $PUPSP3_SIGRESTART
setenv PUPSP3_SIGALIVE          $PUPSP3_SIGALIVE
setenv PUPSP3_SIGTHREADSTOP     $PUPSP3_SIGTHREADSTOP
setenv PUPSP3_SIGTHREADRESTART  $PUPSP3_SIGTHREADRESTART
setenv PUPSP3_SIGCRITICAL       $PUPSP3_SIGCRITICAL
