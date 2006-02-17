#!/bin/sh
# Script for doing the Cleanup user data (CUD) operation.

DIR=/etc/osso-af-init

if [ "x$USER" = "xroot" ]; then
  SUDO=''
  echo "$0: Warning, I'm root"
else
  if [ "x$USER" = "x" ]; then
    SUDO=''
    USER=root
    echo "$0: Warning, USER is not defined, assuming '$USER'"
  else
    SUDO='sudo'
  fi
fi

# sanity checks
if [ "x$MYDOCSDIR" = "x" ]; then
  MYDOCSDIR=/home/user/MyDocs
  echo "$0: Warning, MYDOCSDIR is not defined, assuming '$MYDOCSDIR'"
fi
if [ "x$HOME" = "x" ]; then
  HOME=/home/user
  echo "$0: Warning, HOME is not defined, assuming '$HOME'"
fi

# shut down things
$SUDO /etc/init.d/af-base-apps stop
# define AF-wide environment
source $DIR/af-defines.sh
$SUDO $DIR/gconf-daemon.sh stop

if [ "x$OSSO_CUD_DOES_NOT_DESTROY" = "x" ]; then
  # Remove all user data
  $SUDO /usr/sbin/gconf-clean.sh 
  rm -rf $HOME/.osso/*
  OLDDIR=`pwd`
  cd $HOME/.osso-cud-scripts
  for f in `ls *.sh`; do
    # if we are root, this is run as root (but no can do because
    # user 'user' might not exist)
    ./$f
    RC=$?
    if [ $RC != 0 ]; then
      echo "$0: Warning, '$f' returned non-zero return code $RC"
    fi
  done
  rm -rf $MYDOCSDIR/*
  rm -rf $MYDOCSDIR/.documents/*
  rm -rf $MYDOCSDIR/.documents/.games/*
  rm -rf $MYDOCSDIR/.images/*
  rm -rf $MYDOCSDIR/.sounds/*
  rm -rf $MYDOCSDIR/.videos/*
  cd $OLDDIR
else
  echo "$0: OSSO_CUD_DOES_NOT_DESTROY defined, no data deleted"
fi
# ask MCE to reboot the system
dbus-send --system --type=method_call \
  --dest="com.nokia.mce" --print-reply \
  "/com/nokia/mce/request" \
  com.nokia.mce.request.req_reboot
exit 0
