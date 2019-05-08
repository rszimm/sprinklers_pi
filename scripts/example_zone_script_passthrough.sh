#!/bin/bash
#
# This script simply logs the state changes to /tmp/zone.log and
# then passes through to the command line version of wiringpi.

echo "PIN: $1 OUTPUT: $2" >> /tmp/zone.log
gpio write $1 $2