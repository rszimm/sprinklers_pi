#!/bin/bash
### BEGIN INIT INFO
# Provides:          sprinklers_pi
# Required-Start:    $local_fs $network $time $syslog
# Required-Stop:     $local_fs $network $time $syslog
# Should-Start:      ntp
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start sprinklers_pi daemon at boot time
# Description:       Start sprinkler_pi daemon at boot time.
### END INIT INFO
# pidfiles:          $prefix/var/sprinklers_pi.pid
################################################################################


# add to the shared library search path
#export LD_LIBRARY_PATH=/lib:/usr/local/lib:/usr/lib
NAME=sprinklers_pi
RUN_USER=root
RUN_DIRECTORY=/usr/local/etc/$NAME
INSTALL_DIR=/usr/local/sbin
DAEMON_PID=/var/run/$NAME.pid
DAEMON=$INSTALL_DIR/$NAME
LOGDIR=/var/log
DEFAULTS=/etc/default/sprinklers_pi

if [ -f $DEFAULTS ] ; then
    . $DEFAULTS
fi

. /lib/lsb/init-functions

do_start()
{
        # Return
        #   0 if daemon has been started
        #   1 if daemon was already running
        #   2 if daemon could not be started
        start-stop-daemon --start --quiet --background --make-pidfile --pidfile $DAEMON_PID --exec $DAEMON --test > /dev/null \
                || return 1
		if [ "$1" -eq 1 ] ; then
			start-stop-daemon --start --quiet --no-close --background --make-pidfile --pidfile $DAEMON_PID --exec $DAEMON -c $RUN_USER --chdir $RUN_DIRECTORY \
                $DAEMON_ARGS \
                || return 2
		else
			start-stop-daemon --start --quiet --background --make-pidfile --pidfile $DAEMON_PID --exec $DAEMON -c $RUN_USER --chdir $RUN_DIRECTORY -- \
                $DAEMON_ARGS -L$LOGDIR/$NAME\
                || return 2
		fi
}


case "$1" in
	start)
		log_daemon_msg "Starting $DESC" "$NAME"
		do_start 0
        case "$?" in
                0|1) [ "$VERBOSE" != no ] && log_end_msg 0 ;;
                2) [ "$VERBOSE" != no ] && log_end_msg 1 ;;
        esac
		;;
	start-trace)
		log_daemon_msg "Starting $DESC" "$NAME"
		do_start 1
        case "$?" in
                0|1) [ "$VERBOSE" != no ] && log_end_msg 0 ;;
                2) [ "$VERBOSE" != no ] && log_end_msg 1 ;;
        esac
		;;
	stop)
		log_daemon_msg "Stopping $DESC" "$NAME"
		start-stop-daemon --stop --oknodo --pidfile $DAEMON_PID --name $NAME\
			--exec $DAEMON --retry 5
		RETVAL="$?"
		[ "$RETVAL" = 2 ] && return 2
		rm -f $DAEMON_PID
		exit "$RETVAL"
		;;
	restart)
		$0 stop  && sleep 2
		$0 start
		;;
	status)
		start-stop-daemon --status --pidfile $DAEMON_PID
		RUNNING=$?
		if [ $RUNNING -eq 0 ]; then
			echo -n "Running ($RUNNING) PID:"
			cat $DAEMON_PID
		else
			echo "NOT Running ($RUNNING)"
		fi
		status_of_proc "$DAEMON" "$NAME" && exit 0 || exit $?
		
		;;
	*)
		echo "Usage: $0 {start|stop|restart|status}"
		exit 1
esac

exit 0
 
