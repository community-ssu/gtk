#!/bin/sh

if [ -r /etc/osso-af-init/af-defines.sh ]; then
  unset AF_DEFINES_SOURCED
  source /etc/osso-af-init/af-defines.sh
else
  echo "$0: /etc/osso-af-init/af-defines.sh is not readable!"
  exit 1
fi

$*
