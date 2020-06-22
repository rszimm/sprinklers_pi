#!/bin/bash
#
# This script simply logs the state changes to /tmp/zone.log and
# then passes through to the command line version of wiringpi.
# To use, copy this script to /usr/local/bin/sprinklers_pi_zones and
# make sure it is executable with "chmod +x /usr/local/bin/sprinklers_pi_zones"

echo "PIN: $1 OUTPUT: $2" >> /tmp/zone.log
gpio write $1 $2