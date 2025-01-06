#!/bin/bash
#
#--------------------------------------------------------------------
# Build version command using verion information in distribution path
# (C) M.A. O'Neill, Tumbling Dice 2025
#--------------------------------------------------------------------
#---------------------
# Get version and date
#---------------------

version=$(pwd | sed s/^.*pupsp3-/" "/g | sed s/"\.s"/" "/g | awk '{print $1}')
vdate=$(date +"%Y")


#--------------------------
# Build version tool script
# and make it excutable
#--------------------------

sed s/VERSION/$version/g <pupsp3-version.in  | sed s/DATE/$vdate/g 1>pupsp3-version
chmod +x pupsp3-version

exit  0
