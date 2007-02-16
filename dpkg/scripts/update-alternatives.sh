#!/bin/sh

version='0.0.1'; # This line modified by Makefile
admindir="/var/lib/dpkg"; # This line modified by Makefile

testmode=0
verbose=1
mode='';
alts=$admindir/alternatives
altsd="/etc/alternatives"
genname=""
symlink=""
altern=""
priority=""
	

die(){
   echo "$@"
   exit 1;
}

quit() { 
    echo "update-alternatives: $@" 
    exit 2; 
}
showversion(){
   echo -e "Debian another update-alternatives $version.\n" || die "failed to write version: $!"
}

usage(){
   showversion
   cat <<EOF
Copyright (C) 2006 Yauheni Kaliuta

This is free software; see the GNU General Public Licence version 2
for copying conditions. There is NO warranty.

Usage:

 update-alternatives [options] --install genname symlink altern priority [--slave genname symlink altern]
 update-alternatives [options] --remove name path

Options:
   --verbose 
   --quiet 
   --test    
        not emplemented
  --version
  --altdir directory
  --admindir directory
EOF
}

badusage() {
   echo -e "update-alternatives: $*\n\n"
   echo "You need --help"
   exit 2
}


checkmanymodes() {
   [ x"$mode" = x ] && return
   badusage "two modes specified: $1 and --$mode"
}


install() {
    ln -sf "$altsd/$2" "$1"
    ln -sf "$3" "$altsd/$2"
}

slave() {
    install "$@"
}

remove() {
    return 0
    rm -f "$altsd/$1"
}

while [ "$1" != "" ]; do
   case $1
       in
       -h|--help)
           usage
           exit 0;;
       -V|--version)
           showversion
           exit 0;;
       --test)
	   testmode=1
           shift;;
       -v|--verbose)
           verbose=1
           shift;;
       --quiet)
           verbose=0
           shift;;
       --install)
           checkmanymodes "$1"
	   mode="${1##--}"
	   genname="$2"
	   symlink="$3"
	   altern="$4"
	   priority="$5"
	   [ x$genname = x -o x$symlink = x -o x$altern = x -o x$priority = x ] && badusage "wrong install arguments"
	   install "$genname" "$symlink" "$altern" "$priority"
	   shift 5;;
       --slave)
	   [ x$mode = xinstall ] || badusage "$1 used only with install"
	   slave "$2" "$3" "$4"
	   shift 4;;
       --remove)
	   checkmanymodes "$1"
	   mode="${1##--}"
	   symlink="$2"
	   altern="$3"
	   shift 3;;
       --remove-all)
	   checkmanymodes "$1"
	   mode=removeall
	   symlink="$2"
	   shift 2;;
       --all|--auto|--display|--list|--config)
           checkmanymodes $1
	   echo "$1 unemplemented"
	   exit 0;;
       --*)
           echo "Unknown option $1"
           exit;;
       *)
           [ x"$file" = x ] || badusage "too many arguments"
           file="$1"
           shift;;
   esac
done

[ x$mode = x ] && badusage "no mode specifies"

case "$mode" in
    remove|removeall)
	remove "$symlink" "$altern"
	;;
    *)
	;;
esac

