#!/bin/sh

if [ $# -lt 3 ]; then
    echo "Usage: $0 {start|stop|restart} appname cmdline [params]"
    exit 0
fi

STOP=FALSE
START=FALSE
SBRSH=""

case "$1" in
start)  START=TRUE
        ;;
stop)   STOP=TRUE
        ;;
restart)
        STOP=TRUE
        START=TRUE
        ;;
*)      echo "Usage: $0 {start|stop|restart} appname cmdline [params]"
        exit 0
        ;;
esac

shift
SVC=$1
shift
CMD=$1
shift
PARAMS=$@

BASENAME=`basename $CMD`
TMPSVC=`echo $SVC | sed -e 's/ /_/g'`;
PIDFILE="$AF_PIDDIR/$BASENAME-$TMPSVC.pid"

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


if [ $STOP = TRUE ]; then
    if [ ! -e $PIDFILE ]; then
	echo "$SVC is not running"
    else
        echo "Stopping $SVC"
        PID=`cat $PIDFILE`
        grep "$BASENAME" /proc/$PID/cmdline >/dev/null 2>&1
        if [ $? -eq 0 ]; then       
            kill -9 $PID
        else
            echo "$SVC is not running"
        fi
        rm -f $PIDFILE
    fi
fi

if [ $START = TRUE ]; then
    DOIT=TRUE
    if [ -e $PIDFILE ]; then
	PID=`cat $PIDFILE`
	$SBRSH grep "$BASENAME" /proc/$PID/cmdline >/dev/null 2>&1
	if [ $? -eq 0 ]; then
	    echo "$SVC is already running, doing nothing"
	    DOIT=FALSE
	fi
    fi
    
    if [ $DOIT = TRUE ]; then
	echo "Starting $SVC"
	if [ -n $GDB ] && [ "$GDB" = "$BASENAME" ]; then
	    gdb --args $CMD $PARAMS
	else
	    if [ -n "$VALGRIND" ] && [ "$VALGRINDCMD" = "$BASENAME" ]; then
		$VALGRIND $CMD $PARAMS &
	    else
		if [ -z "$SBRSH" ]; then
		    # no sbrsh
		    $CMD $PARAMS &
		    echo $! > $PIDFILE  
		else
    		    # We need to run it with sbrsh show that we get the pid of
		    # the sbrsh and then on stop we can kill the sbrsh and 
		    # the actual process will die.
		    $SBRSH -d $PWD $CMD $PARAMS &
		    echo $! > $PIDFILE  
		fi
	    fi
	fi
    fi
fi
