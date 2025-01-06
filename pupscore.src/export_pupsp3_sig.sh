#!/bin/bash
#
#####################################
# Export P3 signal mappings for bash 
# (C) M.A O'Neill, Tumbling Dice 2025
#####################################

#------------------------------------
# This might be application dependent
#------------------------------------

SIGRTMIN=34

export PUPSP3_SIGEVENT=$(($SIGRTMIN + 4))
export PUPSP3_SIGPSRP=$(($SIGRTMIN + 5))
export PUPSP3_SIGCHAN=$(($SIGRTMIN + 6))
export PUPSP3_SIGINIT=$(($SIGRTMIN + 7))
export PUPSP3_SIGCLIENT=$(($SIGRTMIN + 8))
export PUPSP3_SIGTHREAD=$(($SIGRTMIN + 9))
export PUPSP3_SIGSLAVE=$(($SIGRTMIN + 10))
export PUPSP3_SIGCHECK=$(($SIGRTMIN + 11))
export PUPSP3_SIGRESTART=$(($SIGRTMIN + 12))
export PUPSP3_SIGALIVE=$(($SIGRTMIN + 13))
export PUPSP3_SIGTHREADSTOP=$(($SIGRTMIN + 14))
export PUPSP3_SIGTHREADRESTART=$(($SIGRTMIN + 15))
export PUPSP3_SIGCRITICAL=$(($SIGRTMIN + 16))

