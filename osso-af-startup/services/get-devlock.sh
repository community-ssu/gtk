#!/bin/sh
dbus-send --system --type=method_call \
 --dest="com.nokia.mce" --print-reply \
 "/com/nokia/mce/request" \
 com.nokia.mce.request.get_devicelock_mode | \
 grep unlocked > /dev/null
if [ $? = 0 ]; then
  echo "unlocked"
else
  echo "locked"
fi
