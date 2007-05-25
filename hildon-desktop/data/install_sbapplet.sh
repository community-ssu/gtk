#!/bin/sh -e
# moimart

if [ ! "$#" -eq "1" ];
then
  echo "1 args required"
  echo "desktopfile"
  exit 1;
fi

DESKTOP_FILE=$1

if [ "`uname -m`" == "armv6l" ];
then
  SB_HOME_DIR=/home/user
  USER=user
else
  SB_HOME_DIR=$HOME
  USER=`basename $HOME`
fi

SB_CONF=statusbar.conf
SB_USRDIRCONF=$SB_HOME_DIR/.osso/hildon-desktop/$SB_CONF
SB_SYSDIRCONF=/etc/hildon-desktop/$SB_CONF
SB_PATH=$SB_USRDIRCONF

if [ ! -r $SB_PATH ];
then
  cp $SB_SYSDIRCONF $SB_USRDIRCONF
  echo "cp $SB_SYSDIRCONF $SB_USRDIRCONF"
fi

if [ ! -r $SB_PATH ];
then
  echo "Can't access configuration"
  exit 0
fi

chown $USER $SB_USRDIRCONF && echo "Setting owner"

if [ "`cat $SB_PATH|grep \"\[$DESKTOP_FILE\]\"|wc -l`" -gt "0" ];
then
  echo "Entry already configured"
  echo "" >> $SB_PATH
  exit 0;
fi

echo "Creating entry..."

echo -e "[$DESKTOP_FILE]\n\
Mandatory=$MANDATORY\n" >> $SB_PATH
