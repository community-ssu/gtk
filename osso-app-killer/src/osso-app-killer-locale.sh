#!/bin/sh

# give time to TN for waiting and sending the TERM signal
sleep 4

sudo /etc/init.d/af-base-apps stop
if [ -x /etc/init.d/maemo-launcher ]; then
  /etc/init.d/maemo-launcher restart
fi
DIR=/etc/osso-af-init
source $DIR/af-defines.sh
if [ -x $DIR/osso-media-server.sh ]; then
  # media-server cannot live without D-BUS session bus
  $DIR/osso-media-server.sh stop
fi
$DIR/dbus-sessionbus.sh stop
# wait for D-BUS to die
TMP=`ps x | grep -- --session | grep -v "grep -- --session" | wc -l | tr -d ' \t'`
while [ $TMP = 1 ]; do
  sleep 1
  TMP=`ps x | grep -- --session | grep -v "grep -- --session" | wc -l | tr -d ' \t'`
done
sleep 2
$DIR/dbus-sessionbus.sh start
/usr/sbin/waitdbus session
sudo /etc/init.d/af-base-apps start
sudo /etc/init.d/osso-systemui restart
