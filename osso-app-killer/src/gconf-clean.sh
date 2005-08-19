#!/bin/sh
for d in `ls /etc/osso-af-init/gconf-dir/`; do
  if [ "x$d" = "xschemas" ]; then
    continue
  fi
  echo "$0: removing GConf subdirectory $d"
  rm -rf $d
done
