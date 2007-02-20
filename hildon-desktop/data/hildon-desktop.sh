#!/bin/sh


PREFIX=/usr
CURRENT_THEME_FILE=$HOME/.osso/current-gtk-theme
CURRENT_MAEMO_THEME_FILE=$HOME/.osso/current-gtk-theme.maemo_af_desktop
CURRENT_THEME=$(cat $CURRENT_THEME_FILE | sed -e "s/include\ \"\/usr\/share\/themes\///" -e 's/\/gtk\-2\.0\/gtkrc\"//' | tr -d " ")

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

PROG=/usr/bin/hildon-desktop
SVC="Hildon Desktop"

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

  $LAUNCHWRAPPER_NICE_TRYRESTART start "$SVC" $PROG
else
  $LAUNCHWRAPPER_NICE_TRYRESTART stop "$SVC" $PROG
fi
