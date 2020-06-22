#!/bin/bash
#
# This script curls a url with the zone and state parameters.
# To use, copy this script to /usr/local/bin/sprinklers_pi_zones and
# make sure it is executable with "chmod +x /usr/local/bin/sprinklers_pi_zones"

URL="http://example.com/updateZone"
ZONE_PARAM="zone"
STATE_PARAM="state"

if [ $# -eq 2 ] ; then
  ZONE=$1

  # map 1 to "on" and 0 (or anything else) to "off"
  if [ "$2" = "1" ] ; then
    STATE="on"
  else
    STATE="off"
  fi

  curl "${URL}?${ZONE_PARAM}=${ZONE}&${STATE_PARAM}=${STATE}"
fi