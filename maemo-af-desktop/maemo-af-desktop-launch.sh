#!/bin/sh

source af.conf

if [ "$1" != "start" ] && [ "$1" != "stop" ] && [ "$1" != "restart" ]; then
  echo "Usage: $0 {start|stop|restart}"
  exit 0
fi

launch-wrapper.sh $1 $DISP af-desktop $PREFIX/bin/maemo_af_desktop
