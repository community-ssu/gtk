#!/bin/sh
cd /etc/osso-af-init/gconf-dir
if [ $? = 0 ]; then
  for d in `ls`; do
    if [ "x$d" = "xschemas" ]; then
      continue
    fi
    echo "$0: removing GConf subdirectory $d"
    rm -rf $d
  done
else
  echo "$0: 'cd' command failed, doing nothing"
fi
