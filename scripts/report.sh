#!/bin/bash

#Set to 1 to not email if no records found. Any other value will send email regardless.
SUPPRESS_EMAIL=1

#sprinklers pi log database location
DB=/usr/local/etc/sprinklers_pi/db.sql

#temporary file to write output to for sending email
TMPFILE=/tmp/spriklerreport.txt


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

calc () {
    bc <<< "$@"
}

DATE=`date +%s`

TIMEZONE=`date +%z`
TIMEZONE=`calc $TIMEZONE / 100`

TIMESPAN=`calc "($HOURS - $TIMEZONE) * 3600"`

TIME=`calc $DATE - $TIMESPAN`

echo "Sprinkler Report for $HOURS hours preceeding `date`" > $TMPFILE

#check if any zones ran
ROWS=`sqlite3 $DB "SELECT COUNT(*) FROM zonelog WHERE date > $TIME" | tr -d '\n'`
if [ $ROWS -gt 1 ] ; then
  #get average seasonal adjustment
  SEASON=`sqlite3 $DB "SELECT AVG(seasonal) FROM zonelog WHERE date > $TIME AND seasonal >= 0" | tr -d '\n'`
  if [ "x$SEASON" != "x" ] ; then
    echo "Seasonal Adjustment: $SEASON%" >> $TMPFILE
  fi
  
  #get average wunderground adjustment
  WUN=`sqlite3 $DB "SELECT AVG(wunderground) FROM zonelog WHERE date > $TIME AND wunderground >= 0" | tr -d '\n'`
  if [ "x$WUN" != "x" ] ; then
    echo "Wunderground Adjustment: $WUN%" >> $TMPFILE
  fi
  
  #print each zones runtime
  for i in {1..15} ; do
    LOG=`sqlite3 $DB "SELECT SUM(duration) FROM zonelog WHERE zone = $i AND date > $TIME" | tr -d '\n'`
    if [ "x$LOG" != "x" ] ; then
      echo "Zone $i: $LOG seconds." >> $TMPFILE
    fi
  done
else
  echo "No zones active for this time period." >> $TMPFILE
  if [ $SUPPRESS_EMAIL -eq 1 ] ; then
    EMAIL=ECHO
  fi
fi

#output to screen or email
if [ "$EMAIL" == "ECHO" ] ; then
  cat $TMPFILE
else
  cat $TMPFILE | mail -s "Sprinkler Report `date`" $EMAIL
fi

rm $TMPFILE
