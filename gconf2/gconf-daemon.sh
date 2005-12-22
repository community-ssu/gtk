#!/bin/sh
# GConf daemon startup/shutdown script

PROG=/usr/lib/gconf2/gconfd-2
SVC="GConf daemon"

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
  echo "Not starting $SVC, D-BUS starts it"
else
  PID=`pidof $PROG`
  if [ "x$PID" != "x" ]; then
    echo "Stopping $SVC"
    kill $PID
  fi
fi
