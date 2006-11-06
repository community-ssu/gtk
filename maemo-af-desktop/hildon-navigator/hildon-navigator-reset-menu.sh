#!/bin/sh

set -x

OSSO_CONF_DIR="${HOME}/.osso"
MENU_CONF_DIR="${OSSO_CONF_DIR}/menus"

if [ ! $1 ]
then
  exit 0
fi

if [ ! -e $1 ]
then
  exit 0
fi

if cat $1 |grep "^${MENU_CONF_DIR}/.\+$"
then
  rm -f "${MENU_CONF_DIR}/applications.menu"
fi
