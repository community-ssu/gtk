#!/bin/sh

if [ $# -lt 3 ]; then
    echo "Usage: $0 {start|stop|restart} appname cmdline [params]"
    exit 0
fi

STOP=FALSE
START=FALSE
CPU_TRANSPARENCY_METHOD=""

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
    CPU_TRANSPARENCY_METHOD="sbrsh $SBOX_TARGET_NAME"
  fi
  if echo $SBOX_CPUTRANSPARENCY_METHOD | grep "qemu-arm$" >/dev/null; then
    # we are using qemu-arm
    CPU_TRANSPARENCY_METHOD="qemu-arm"
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
	$CPU_TRANSPARENCY_METHOD grep "$BASENAME" /proc/$PID/cmdline >/dev/null 2>&1
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
		if [ -z "$CPU_TRANSPARENCY_METHOD" ]; then
		    # no sbrsh
		    $CMD $PARAMS &
		    echo $! > $PIDFILE  
		else
	            if [ x"$CPU_TRANSPARENCY_METHOD" == x"sbrsh $SBOX_TARGET_NAME" ]; then
    		       # We need to run it with sbrsh show that we get the pid of
		       # the sbrsh and then on stop we can kill the sbrsh and 
		       # the actual process will die.
		       $CPU_TRANSPARENCY_METHOD -d $PWD $CMD $PARAMS &
		       echo $! > $PIDFILE
		    fi
		    if [ x"$CPU_TRANSPARENCY_METHOD" == x"qemu-arm" ]; then
		       $CPU_TRANSPARENCY_METHOD $CMD $PARAMS &
		       echo $! > $PIDFILE
		    else
		       echo "Unkown CPU transparency method: $CPU_TRANSPARENCY_METHOD"
		       exit 1
		    fi
		fi
	    fi
	fi
    fi
fi
