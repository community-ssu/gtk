#!/bin/sh
# Script for doing the Cleanup user data (CUD) operation.

DIR=/etc/osso-af-init

# sanity checks
if [ "x$MYDOCSDIR" = "x" ]; then
  echo "$0: Error, MYDOCSDIR is not defined"
  exit 1
fi
if [ "x$HOME" = "x" ]; then
  echo "$0: Error, HOME is not defined"
  exit 1
fi
if [ "x$USER" = "xroot" ]; then
  echo "$0: Error, I'm root"
  exit 1
fi

# shut down things
sudo /etc/init.d/af-base-apps stop
# define AF-wide environment
source $DIR/af-defines.sh
sudo $DIR/gconf-daemon.sh stop

if [ "x$OSSO_CUD_DOES_NOT_DESTROY" = "x" ]; then
  # Remove all user data
  sudo /usr/sbin/gconf-clean.sh 
  rm -rf $HOME/.osso/*
  OLDDIR=`pwd`
  cd $HOME/.osso-cud-scripts
  for f in `ls *.sh`; do
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
exit 0
