#!/bin/bash

P=`pwd`

echo $P
cd /usr/bin
ln -sf $P/test_client.py test_client

cd /usr/share/dbus-1/services
ln -sf $P/test_client.service com.nokia.test_client.service

cd /usr/share/applications/hildon/
ln -sf $P/test_client.desktop test_client.desktop

af-sb-init.sh restart
