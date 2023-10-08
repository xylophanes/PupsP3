#!/bin/tcsh
#
######################################
# Unexport P3 signal mappings for csh 
# (C) M.A O'Neill, Tumbling Dice 2023
######=###############################

#-----------------------
# Unexport signal mappings
#-----------------------

unsetenv PUPSP3_SIGEVENT
unsetenv PUPSP3_SIGPSRP
unsetenv PUPSP3_SIGCHAN
unsetenv PUPSP3_SIGINIT
unsetenv PUPSP3_SIGCLIENT
unsetenv PUPSP3_SIGTHREAD
unsetenv PUPSP3_SIGSLAVE
unsetenv PUPSP3_SIGCHECK
unsetenv PUPSP3_SIGRESTART
unsetenv PUPSP3_SIGALIVE 
unsetenv PUPSP3_SIGTHREADSTOP
unsetenv PUPSP3_SIGTHREADRESTART
unsetenv PUPSP3_SIGCRITICAL
