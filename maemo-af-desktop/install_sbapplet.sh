#!/bin/sh -e
# moimart

if [ ! "$#" -eq "4" ];
then
  echo "4 args required"
  echo "desktopfile library category mandatory"
  exit 1;
fi

DESKTOP_FILE=$1
LIBRARY=$2
CATEGORY=$3

case $4 in
  [0-1])
    MANDATORY=$4
    ;;
  [tT]rue)
    MANDATORY=true
    ;;
  [fF]alse)
    MANDATORY=false
   ;;
  *)
    MANDATORY=false
    ;;
esac

SB_CONF=plugins.conf
SB_USRDIRCONF=/home/user/.osso/hildon-status-bar/$SB_CONF
SB_SYSDIRCONF=/etc/hildon-status-bar/$SB_CONF
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

if [ "`cat $SB_PATH|grep \"\[$DESKTOP_FILE\]\"|wc -l`" -gt "0" ];
then
  echo "Entry already configured"
  echo "" >> $SB_PATH
  exit 0;
fi

echo "Creating entry..."

POSITION=`cat $SB_PATH|grep Position|wc -l`

echo -e "[$DESKTOP_FILE]\n\
Library=$LIBRARY\n\
Desktop-file=$DESKTOP_FILE\n\
Category=$CATEGORY\n\
Position=$POSITION\n\
Mandatory=$MANDATORY\n" >> $SB_PATH
