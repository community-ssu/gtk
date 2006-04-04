#! /bin/sh
#
# $Id$
#

PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
NAME=maemo-launcher
DESC="Maemo Launcher"
DAEMON=/usr/bin/$NAME
DAEMON_BASE_OPTS="--daemon"
PIDFILE=/tmp/$NAME.pid

# OSSO AF Init definitions
DEFSDIR=/etc/osso-af-init/

# When inside scratchbox we are not really root nor do we have 'user' user
if [ -f /targets/links/scratchbox.config ]; then
  DAEMON_OPTS="$DAEMON_BASE_OPTS"
else
  USER=user
  HOME=/home/$USER
  if [ `id -u` = 0 ]; then
    CHUID="--chuid $USER"
    # Make sure the launcher is not an OOM candidate
    NICE="--nicelevel -1"
  fi
  DAEMON_OPTS="$DAEMON_BASE_OPTS --quiet"
fi

test -x $DAEMON || exit 0

set -e

# Those files set needed environment variables for the Maemo applications
if [ -e $DEFSDIR/af-defines.sh ]
then
  . $DEFSDIR/af-defines.sh
  for f in $DEFSDIR/*.defs; do
    . $f
  done
else
  echo "$DEFSDIR/af-defines.sh not found!"
  exit 1
fi

case "$1" in
  start)
    echo -n "Starting $DESC: $NAME"
    start-stop-daemon --start --quiet --pidfile $PIDFILE $CHUID $NICE \
      --exec $DAEMON -- $DAEMON_OPTS || echo -n " start failed"
    echo "."
    ;;
  stop)
    echo -n "Stopping $DESC: $NAME"
    start-stop-daemon --stop --quiet --pidfile $PIDFILE --exec $DAEMON \
      || echo -n " not running"
    echo "."
    ;;
  restart|force-reload)
    $0 stop
    sleep 1
    $0 start
    ;;
  *)
    N=/etc/init.d/$NAME
    echo "Usage: $N {start|stop|restart|force-reload}" >&2
    exit 1
    ;;
esac

exit 0

# vim: syn=sh

