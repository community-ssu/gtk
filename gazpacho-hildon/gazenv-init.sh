#!/bin/sh

STOP=FALSE
START=FALSE
DIR=/etc/osso-af-init
GDB=

if [ $# = 0 ]; then
  echo "Usage: $0 {start|stop|restart} [--valgrind cmd] [--gdb appname]"
  exit 0
fi

case "$1" in
start)  START="TRUE"
        ;;
stop)   STOP="TRUE"
        ;;
restart|force-reload)
        STOP="TRUE"
        START="TRUE"
        ;;
*)      echo "Usage: $0 {start|stop|restart} [--valgrind cmd] [--gdb cmd]"
        exit 0
        ;;
esac
shift

while [ $# != 0 ]; do
  if [ "$1" = "--valgrind" ]; then
    shift
    export VALGRINDCMD=$1
    export VALGRIND="valgrind --leak-check=yes --trace-children=yes --num-callers=100 --logfile=/tmp/valgrind-$VALGRINDCMD.log"
  fi
  if [ "$1" = "--gdb" ]; then
    shift
    export GDB=$1
  fi
  shift
done

AF_DEFS=$DIR/af-defines.sh

if [ ! -r $AF_DEFS ]; then
  echo "$0: Error, $AF_DEFS does not exist or is not readable"
  exit 2
fi

source $AF_DEFS

# let's use /var/run as pid dir and make sure it is not a link
export AF_PIDDIR=/var/run
if [ -L $AF_PIDDIR ]; then
  rm $AF_PIDDIR
fi

if [ ! -d $AF_PIDDIR ]; then
  mkdir $AF_PIDDIR
fi

# dbus want's this, so let's make sure it is found.
mkdir -p /var/run/dbus

# overriding some variables
export HOME="/home/$USER"
export MYDOCSDIR=$HOME/MyDocs

source $DIR/osso-gtk.defs

# Let's use SB launcher wrapper
export LAUNCHWRAPPER=/usr/bin/scratchbox-launcher.sh

# We have only one wrapper in SB
export LAUNCHWRAPPER_NICE=$LAUNCHWRAPPER
export LAUNCHWRAPPER_NICE_TRYRESTART=$LAUNCHWRAPPER

# Check our environment
if [ -e /targets/links/scratchbox.config ]; then
  source /targets/links/scratchbox.config
  if echo $SBOX_CPUTRANSPARENCY_METHOD | grep "sbrsh$" >/dev/null; then
    if [ -z "$SBOX_TARGET_NAME" ]; then
      echo "$0: SBOX_CPUTRANSPARENCY_METHOD defined but SBOX_TARGET_NAME not!"
      exit 1
    fi

    # we are using Scratchbox and sbrsh
    SBRSH="sbrsh $SBOX_TARGET_NAME"
  fi
fi

# we need to save these, because the source commands below messes them up
TMPSTART=$START
TMPSTOP=$STOP
if [ "$STOP" = "TRUE" ]; then

  for svc in \
      keyboard \
      sapwood-server \
      dbus-sessionbus \
      dbus-systembus;
  do
    if [ -e $DIR/$svc.defs ]; then
      source $DIR/$svc.defs
    fi
    if [ -e $DIR/$svc.sh ]; then
      source $DIR/$svc.sh stop
    fi
  done

  # stop maemo-launcher
  if [ -x /etc/init.d/maemo-launcher ]; then
    /etc/init.d/maemo-launcher stop
  fi
  # remove dbus own dbus pid file
  rm -rf /var/run/dbus/pid

  maemo-gtk-im-switch osso-input-method
fi

START=$TMPSTART
STOP=$TMPSTOP
if [ "$START" = "TRUE" ]; then

  # if we are restarting, give some time for the programs to shutdown
  if [ "$STOP" = "TRUE" ]; then
    # if we are not using sbrsh sleep just a seconds, otherwise a bit longer
    if [ -z "$SBRSH" ]; then
      sleep 1
    else
      sleep 5
    fi
  fi

  if [ "x$MYDOCSDIR" = "x" ]; then
      echo "$0: Error, MYDOCSDIR is not defined"
      exit 2
  fi

  if [ "x$DISPLAY" = "x" ]; then
      echo "$0: Error, DISPLAY is not defined"
      echo "DISPLAY must be host:display when using sbrsh in Scratchbox and "
      echo "X forwarding will be used. Set display to :0 (the display number "
      echo "of your Desktop X) to launch Xephyr."
      exit 1
  fi

  host=`echo $DISPLAY | cut -f 1 -d ':'`
  if [ -z "$host" ]; then
  	echo "Note: For remote X connections DISPLAY should contain hostname!"
  fi

  # Create some files to play with
  if [ ! -d $MYDOCSDIR ]; then
    # normally osso-af-startup creates these...
    mkdir -p $MYDOCSDIR/.documents/.games $MYDOCSDIR/.videos
    mkdir -p $MYDOCSDIR/.images $MYDOCSDIR/.sounds
  fi
  if [ -e $MYDOCSDIR/foo.txt ]; then
    echo "Sample files present."
  else
    touch $MYDOCSDIR/foo.txt
    touch $MYDOCSDIR/.videos/video1.mpg $MYDOCSDIR/.videos/video2.avi
    touch $MYDOCSDIR/.sounds/clip1.wav $MYDOCSDIR/.sounds/clip2.mp3
    touch $MYDOCSDIR/.documents/sheet1.xls $MYDOCSDIR/.documents/document.doc
    touch $MYDOCSDIR/.images/image1.png $MYDOCSDIR/.images/image2.jpg
  fi

  # remove dbus own dbus pid file
  rm -rf /var/run/dbus/pid
  
  # start dbus system and session busses
  for svc in \
      dbus-systembus \
      dbus-sessionbus;
  do
    if [ -e $DIR/$svc.defs ]; then
      source $DIR/$svc.defs
    fi
    if [ -e $DIR/$svc.sh ]; then
      source $DIR/$svc.sh start
    fi
  done

  # resource AF_DEFS to read the address of the dbus session address
  source $AF_DEFS

  # start maemo-launcher
  if [ -x /etc/init.d/maemo-launcher ]; then
    /etc/init.d/maemo-launcher start
  fi

  # start other things
  for svc in \
      sapwood-server \
      keyboard; 
  do
    if [ -e $DIR/$svc.defs ]; then
      source $DIR/$svc.defs
    fi
    if [ -e $DIR/$svc.sh ]; then
      source $DIR/$svc.sh start
    fi
  done

  maemo-gtk-im-switch xim
fi