#!/bin/bash

if ! hash bc 2>/dev/null ; then
  echo bc is required. Try "sudo apt-get install bc".
  exit 2
fi

if [ $# -lt 1 -o $# -gt 2 ] ; then
  echo "Usage: $0 [hours] [email]"
  echo "hours: (required) number of hours in the past to report for."
  echo "email: (optional) email address to send report to. If no email then prints to screen."
  exit 1
fi

HOURS=$1

if [ $# -eq 2 ] ; then
  if ! hash mail 2>/dev/null ; then
    echo mail program is required to send email
    exit 2
  fi
  EMAIL=$2
else
  EMAIL=ECHO
fi

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

if [ "$EMAIL" == "ECHO" ] ; then
  cat $TMPFILE
else
  cat $TMPFILE | mail -s "Sprinkler Report `date`" $EMAIL
fi

rm $TMPFILE
