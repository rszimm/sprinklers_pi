#!/bin/bash

HOURS=$1
DB=/usr/local/etc/sprinklers_pi/db.sql
TMPFILE=/tmp/spriklerreport.txt

calc () {
    bc <<< "$@"
}

DATE=`date +%s`

TIMEZONE=`date +%z`
TIMEZONE=`calc $TIMEZONE / 100`

TIMESPAN=`calc "($HOURS - $TIMEZONE) * 3600"`

TIME=`calc $DATE - $TIMESPAN`

echo "Sprinkler Report for $HOURS hours preceeding `date`" > $TMPFILE

SEASON=`sqlite3 $DB "SELECT AVG(seasonal) FROM zonelog WHERE date > $TIME AND seasonal >= 0"`
echo "Seasonal Adjustment: $SEASON" >> $TMPFILE

WUN=`sqlite3 $DB "SELECT AVG(wunderground) FROM zonelog WHERE date > $TIME AND wunderground >= 0"`
echo "Wunderground Adjustment: $WUN" >> $TMPFILE

for i in {1..15} ; do
  LOG=`sqlite3 $DB "SELECT SUM(duration) FROM zonelog WHERE zone = $i AND date > $TIME" | tr -d '\n'`
  if [ "x$LOG" != "x" ] ; then
    echo "Zone $i: $LOG seconds." >> $TMPFILE
  fi
done
