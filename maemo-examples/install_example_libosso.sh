#!/bin/sh
set -x
cp -f example_libosso example_message /usr/bin/
cp -f example_libosso.desktop /usr/share/applications/hildon/0010_example_libosso.desktop
cp -f example_libosso.service /usr/lib/dbus-1.0/services/
( cd /etc/others-menu/ ; ln -fs /usr/share/applications/hildon/0010_example_libosso.desktop /etc/others-menu/0010_example_libosso.desktop )

