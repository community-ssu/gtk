#!/bin/sh
#export OSSO_RFS_DOES_NOT_DESTROY=1
if [ -x /usr/sbin/dsmetool ]; then
  exec /usr/sbin/dsmetool -U user -n -1 -o /usr/bin/app-killer
else
  exec /usr/bin/app-killer
fi
