#!/bin/sh

set -x

IMAGE_LOADER=/usr/bin/home-image-loader

OSSO_CONF_DIR="${HOME}/.osso"
HOME_CONF_DIR="${OSSO_CONF_DIR}/hildon-home"
BACKGROUND_CACHE="hildon_home_bg_user.png"


if [ ! $1 ]
then
  exit 0
fi

if [ ! -e $1 ]
then
  exit 0
fi

if cat $1 |grep "^${HOME_CONF_DIR}/.\+$"
then
# We always remove the background cache so it gets regenerated on
# next boot
  rm -f "${HOME_CONF_DIR}/${BACKGROUND_CACHE}"
fi
