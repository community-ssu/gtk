#!/bin/sh

IMAGE_LOADER=/usr/bin/home-image-loader

OSSO_CONF_DIR="${HOME}/.osso"
NAVIGATOR_CONF_DIR="${OSSO_CONF_DIR}/hildon-navigator"
NAVIGATOR_CONF_FILE="${NAVIGATOR_CONF_DIR}/plugins.conf"



if [ ! $1 ]
then
  exit 0
fi

if [ ! -e $1 ]
then
  exit 0
fi


# Remove the TN plugin configuration
if cat $1 | grep ${NAVIGATOR_CONF_FILE}
then
  rm -f ${NAVIGATOR_CONF_FILE}
fi

# Remove possible watch files left over by crashed TN plugin
for i in `cat $1 |grep "^${OSSO_CONF_DIR}/navigator.\+watch$"`
do
  rm -f $i
done
