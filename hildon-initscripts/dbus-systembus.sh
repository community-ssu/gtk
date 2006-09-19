#!/bin/sh
# Statusbar startup/shutdown script

if [ "x$AF_PIDDIR" = "x" ]; then
  echo "$0: Error, AF_PIDDIR is not defined"
  exit 2
fi
if [ ! -w $AF_PIDDIR ]; then
  echo "$0: Error, directory $AF_PIDDIR is not writable"
  exit 2
fi

PROG=/usr/bin/dbus-daemon
SVC="DBUS system bus"

case "$1" in
start)  START=TRUE
        ;;
stop)   START=FALSE
        ;;
*)      echo "Usage: $0 {start|stop}"
        exit 1
        ;;
esac

if [ $START = TRUE ]; then

    if [ -f /targets/links/scratchbox.config -a \
	-w /etc/dbus-1/system.conf ]; then
        # we are inside scratchbox 
	# and we need to hackout "<user></user>" directive
	grep -v -e "<user>" -e "<fork/>" /etc/dbus-1/system.conf > \
	    /etc/dbus-1/system.conf.new && \
	    mv /etc/dbus-1/system.conf.new /etc/dbus-1/system.conf
    fi

  $LAUNCHWRAPPER start "$SVC" $PROG --system
else
  $LAUNCHWRAPPER stop "$SVC" $PROG
fi
