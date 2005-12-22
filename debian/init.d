#! /bin/sh
# 
# kdbusd	Kernel UEvents to D-BUS system daemon proxy
#
#		Written by Timo Teräs based on skeleton code
#		written by Miquel van Smoorenburg <miquels@cistron.nl>.
#		Modified for Debian 
#		by Ian Murdock <imurdock@gnu.ai.mit.edu>.
#

PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/sbin/kdbusd
NAME=kdbusd
DESC="kdbusd"
USER=root
DTOOL=/usr/sbin/dsmetool

test -x $DAEMON || exit 0

set -e

case "$1" in
  start)
	# Start daemons
	echo -n "Starting $DESC: "
	if [ -x $DTOOL ]; then
        	$DTOOL -U $USER -n -1 -t $DAEMON
	else
		start-stop-daemon -b --start --quiet --user $USER \
			--exec $DAEMON
	fi
	echo "$NAME."
	;;
  stop)
	echo -n "Stopping $DESC: "
	if [ -x $DTOOL ]; then
		$DTOOL -k $DAEMON
	else
		start-stop-daemon --stop --quiet --oknodo --user $USER \
			--exec $DAEMON
	fi
	echo "$NAME."
	;;
  reload|restart|force-reload)
	#
	#	If the "reload" option is implemented, move the "force-reload"
	#	option to the "reload" entry above. If not, "force-reload" is
	#	just the same as "restart".
	#
	"$0" stop
	"$0" start
	;;
  *)
	N=/etc/init.d/$NAME
	# echo "Usage: $N {start|stop|restart|reload|force-reload}" >&2
	echo "Usage: $N {start|stop|restart|force-reload}" >&2
	exit 1
	;;
esac

exit 0
