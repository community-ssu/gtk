#!/bin/sh
cd /etc/osso-af-init/gconf-dir
if [ $? = 0 ]; then
  for d in `ls`; do
    if [ "x$d" = "xschemas" ]; then
      continue
    fi
    if [ "x$CUD" != "x" ]; then
      if [ "x$d" = "xsystem" ]; then
        for f in `find system -name *.xml`; do
          echo "$f" | grep -e '\(connectivity\)\|\(bluetooth\)' > /dev/null
          if [ $? = 1 ]; then
            echo "$0: removing $f"
            rm -f $f
          fi
        done
        continue
      fi
    fi
    echo "$0: removing GConf subdirectory $d"
    rm -rf $d
  done
else
  echo "$0: 'cd' command failed, doing nothing"
fi
