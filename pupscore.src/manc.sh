#!/bin/bash
#
#-------------------------------------------------------------------------------------
#  Generate stub man page documentation from a C or C++ source using c2man application
#  (C) M.A. O'Neill 2009-2021
#
#  $1 is source file to generate man page from.
#  $2 is name of man page file.
#-------------------------------------------------------------------------------------
#

c2man -DPSRP_AUTHENTICATE -DCKPT_SUPPORT -DSOCKET_SUPPORT -DSUPPORT_SHARED_HEAPS -DDLL_SUPPORT -g -I. -I../include.libs <$1 >$2
exit 0
