#!/bin/sh
# Sapwood server startup/shutdown script

if [ "x$AF_PIDDIR" = "x" ]; then
  echo "$0: Error, AF_PIDDIR is not defined"
  exit 2
fi
if [ ! -w $AF_PIDDIR ]; then
  echo "$0: Error, directory $AF_PIDDIR is not writable"
  exit 2
fi

PROG=/usr/lib/sapwood/sapwood-server
SVC="Sapwood image server"

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
  # check that required environment is defined
  if [ "x$DISPLAY" = "x" ]; then
    echo "$0: Error, DISPLAY is not defined"
    exit 2
  fi

  # NB#22173 memory saving hack, depends on modified gtk+
  GDK_DISABLE_XSHM=1; export GDK_DISABLE_XSHM
  $LAUNCHWRAPPER_NICE start "$SVC" $PROG
  unset GDK_DISABLE_XSHM
else
  $LAUNCHWRAPPER_NICE stop "$SVC" $PROG
fi
