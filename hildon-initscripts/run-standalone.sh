#!/bin/sh

if [ -e /etc/osso-af-init/af-defines.sh ]
then
  source /etc/osso-af-init/af-defines.sh
else
  echo "/etc/osso-af-init/af-defines.sh not found!"
  exit 1
fi

for i in /etc/osso-af-init/*.defs
do
  source $i
done

$*
