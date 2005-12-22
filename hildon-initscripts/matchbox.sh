#!/bin/sh
# Matchbox window manager startup/shutdown script

if [ "x$AF_PIDDIR" = "x" ]; then
  echo "$0: Error, AF_PIDDIR is not defined"
  exit 2
fi
if [ ! -w $AF_PIDDIR ]; then
  echo "$0: Error, directory $AF_PIDDIR is not writable"
  exit 2
fi

PROG=/usr/bin/matchbox-window-manager
SVC="Matchbox window manager"

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
  if [ "x$MBTHEME" = "x" ]; then
    echo "$0: Error, MBTHEME is not defined"
    exit 2
  fi
  if [ "x$TITLEBAR" = "x" ]; then
    echo "$0: Error, TITLEBAR is not defined"
    exit 2
  fi
  if [ "x$DESKTOPMODE" = "x" ]; then
    echo "$0: Error, DESKTOPMODE is not defined"
    exit 2
  fi
  if [ "x$LOWLIGHT" = "x" ]; then
    echo "$0: Error, LOWLIGHT is not defined"
    exit 2
  fi
  if [ "x$SHOWCURSOR" = "x" ]; then
    echo "$0: Error, SHOWCURSOR is not defined"
    exit 2
  fi
  if [ "x$DIALOGMODE" = "x" ]; then
    echo "$0: Error, DIALOGMODE is not defined"
    exit 2
  fi
  if [ "x$SUPERMODAL" = "x" ]; then
    echo "$0: Error, SUPERMODAL is not defined"
    exit 2
  fi
		  
  $LAUNCHWRAPPER_NICE start "$SVC" $PROG \
      -theme $MBTHEME \
      -use_titlebar $TITLEBAR \
      -use_desktop_mode $DESKTOPMODE \
      -use_lowlight $LOWLIGHT \
      -use_cursor $SHOWCURSOR \
      -use_dialog_mode $DIALOGMODE \
      -use_super_modal $SUPERMODAL
else
  # dsmetool wants identical arguments for stopping
  $LAUNCHWRAPPER_NICE stop "$SVC" $PROG \
      -theme $MBTHEME \
      -use_titlebar $TITLEBAR \
      -use_desktop_mode $DESKTOPMODE \
      -use_lowlight $LOWLIGHT \
      -use_cursor $SHOWCURSOR \
      -use_dialog_mode $DIALOGMODE \
      -use_super_modal $SUPERMODAL
fi
