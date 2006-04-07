#!/bin/sh
# Task Navigator startup/shutdown script

export NAVIGATOR_DO_BGKILL=yes
export NAVIGATOR_LOWMEM_DIM=no
export NAVIGATOR_LOWMEM_NOTIFY_ENTER=yes
export NAVIGATOR_LOWMEM_NOTIFY_LEAVE=no
export NAVIGATOR_LOWMEM_PAVLOV_DIALOG=no
export NAVIGATOR_LOWMEM_LAUNCH_BANNER_TIMEOUT=20
export NAVIGATOR_LOWMEM_LAUNCH_THRESHOLD_DISTANCE=2500
export NAVIGATOR_LOWMEM_TIMEOUT_MULTIPLIER=2


# set theme for maemo_af_desktop  
PREFIX=/usr
CURRENT_THEME_FILE=$HOME/.osso/current-gtk-theme
CURRENT_MAEMO_THEME_FILE=$HOME/.osso/current-gtk-theme.maemo_af_desktop
CURRENT_THEME=$(cat $CURRENT_THEME_FILE | sed -e "s/include\ \"\/usr\/share\/themes\///" -e 's/\/gtk\-2\.0\/gtkrc\"//' | tr -d " ")

export GTK2_RC_FILES=$HOME/.osso/current-gtk-theme:$HOME/.osso/current-gtk-theme.maemo_af_desktop

if [ ! -f $CURRENT_MAEMO_THEME_FILE ]; then
	  echo "include \"$PREFIX/share/themes/$CURRENT_THEME/gtk-2.0/gtkrc.maemo_af_desktop\"" \  > $CURRENT_MAEMO_THEME_FILE
fi 



if [ "x$AF_PIDDIR" = "x" ]; then
  echo "$0: Error, AF_PIDDIR is not defined"
  exit 2
fi
if [ ! -w $AF_PIDDIR ]; then
  echo "$0: Error, directory $AF_PIDDIR is not writable"
  exit 2
fi
PROG=/usr/bin/maemo_af_desktop
SVC="MAEMO AF Desktop"

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

  if [ "x$HOME" = "x" ]; then
    echo "$0: Error, HOME is not defined"
    exit 2
  fi

  $LAUNCHWRAPPER_NICE start "$SVC" $PROG
else
  $LAUNCHWRAPPER_NICE stop "$SVC" $PROG
fi
