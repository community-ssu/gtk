#!/bin/sh

version='1.0.11'; # This line modified by Makefile
admindir="/var/lib/dpkg"; # This line modified by Makefile
dpkglibdir="/usr/lib/dpkg"; # This line modified by Makefile

testmode=0
dorename=0
pattern='.*'
verbose=1
mode='';
divs=$admindir/diversions


die(){
   echo "$@"
   exit 1;
}

quit() { 
    echo "dpkg-divert: $@" 
    exit 2; 
}

enoent=`$dpkglibdir/enoent` || die "Cannot get ENOENT value from $dpkglibdir/enoent: $!"
ENOENT(){
   $enoent
}

showversion(){
   echo -e "Debian dpkg-divert $version.\n" || die "failed to write version: $!"
}

usage(){
   showversion
   cat <<EOF
Copyright (C) 1995 Ian Jackson.
Copyright (C) 2000,2001 Wichert Akkerman.
Copyright (C) 2006 Yauheni Kaliuta

This is free software; see the GNU General Public Licence version 2 or later
for copying conditions. There is NO warranty.

Usage:

 dpkg-divert [options] [--add] <file>               - add a diversion
 dpkg-divert [options] --remove <file>              - remove the diversion
 dpkg-divert [options] --list [<glob-pattern>]      - show file diversions
 dpkg-divert [options] --truename <file>            - return the diverted file

Options:
   --package <package>        name of the package whose copy of <file>
                              will not be diverted.
   --local                    all packages' versions are diverted.
   --divert <divert-to>       the name used by other packages' versions.
   --rename                   actually move the file aside (or back).
   --quiet                    quiet operation, minimal output
   --test                     don't do anything, just demonstrate
   --help                     print this help screen and exit
   --version                  output version and exit
   --admindir <directory>     set the directory with the diversions file

When adding, default is --local and --divert <original>.distrib.
When removing, --package or --local and --divert must match if specified.
Package preinst/postrm scripts should always specify --package and --divert.
EOF
}

badusage() {
   echo -e "dpkg-divert: $@\n\n"
   echo "You need --help"
   exit 2
}


checkmanymodes() {
   [ x"$mode" = x ] && return
   badusage "two modes specified: $1 and --$mode"
}



checkrename() {
   [ x$dorename = x0 ] && return
   rsrc=$1
   rdest=$2
   set -e
   touch $rsrc.dpkg-devert.tmp
   touch $rdest.dpkg-devert.tmp
   rm $rsrc.dpkg-devert.tmp $rdest.dpkg-devert.tmp
   set +e
}

save() {
    divs=$1
    set -e
    mv $divs $divs-old
    mv $divs-new $divs
    set +e
}

dorename() {
   [ x$dorename = x0 -o x$testmode != x0 ] && return
   mv $1 $2
}

truename() {
    file=$1
    handler='BEGIN {n = 0} 
                 n == 0 && $1 == name { file = $1; n = 3 ; next }
                 n == 0  { file=$1; n = 1; next; }
                 n == 1  { n =2 ; next }
                 n == 2  { n = 0; }
                 n == 3  { print file, $1; n =4; }
                 n == 4  { exit } '
    awk -v name=$file "$handler" < $divs
}

while [ "$1" != "" ]; do
   case $1
       in
       --help)
           usage
           exit 0;;
       --version)
           showversion
           exit 0;;
       --test)
           testmode=1
           shift;;
       --rename)
           dorename=1
           shift;;
       --quiet)
           verbose=0
           shift;;
       --local)
           package=':'
           shift;;
       --add|--remove|--truename)
#           [ x"${2%%--*}" = x -a x"$1" ] && badusage "$1 needs a argument"
           checkmanymodes $1
           mode="${1##--}"
	   shift
#           file="$2"
#           [ x"$file" != x`echo $file | tr -d '\n'` ] && badusage "file may not contain newlines"
#           shift 2
	   ;;
       --list)
           checkmanymodes $1
           mode="list"
           if [ x"${2%%--*}" != x ]; then
               pattern="$2"
               shift
           fi
           shift;;
       --divert)
           [ x"${2%%--*}" = x ] && badusage "$1 needs a argument"
           divertto="$2"
           [ x"$divertto" != x`echo $divertto | tr -d '\n'` ] && badusage "divert-to may not contain newlines"
           shift 2;;
       --package)
           [ x"${2%%--*}" = x ] && badusage "$1 needs a argument"
           package="$2"
           [ x"$package" != x`echo $package | tr -d '\n'` ] && badusage "package may not contain newlines"
           shift 2;;
       --admindir)
           [ x"${2%%--*}" = x ] && badusage "$1 needs a argument"
           admindir="$2"
           shift 2;;
       --*)
           echo "Unknown option $1"
           exit;;
       *)
           [ x"$file" = x ] || badusage "too many arguments"
           file="$1"
	   [ x"$file" != x`echo $file | tr -d '\n'` ] && badusage "file may not contain newlines"
           shift;;
   esac
done


[ x$mode = x ] && mode='add'

parser='BEGIN {n = 0}; 
                       n == 0 { file=$1;  n = 1; next; } ;
                       n == 1 { alts[file] = $1; n = 2; next; };
                       n == 2 { packages[file] = $1; n = 0; }'

case $mode in
   add)
       echo $file | grep -q '^/' || badusage "filename $file is not absolute"
       [ -d "$file" ]  && badusage "Cannot divert directories"
       [ x"$divertto" = x ] && divertto="$file.distrib"
       echo $divertto | grep -q '^/' || badusage "filename $divertto is not absolute"
       [ x"$package" = x ] && package=':'

       handler='END { 
             for (file in alts) {
                      if( file == dfile || alts[file] == dfile ||
                          file == divertto || alts[file] == divertto) {
                        if( file == dfile && alts[file] == divertto && package == packages[file]) 
                             exit 101;
                        exit 102;
                      }
             }
             file = dfile; alts[file] = divertto; packages[file] = package;
             for (file in alts) { print file; print alts[file]; print packages[file]; }
        } '
       awk -v dfile=$file -v divertto=$divertto -v package=$package  \
           "$parser $handler"  < $divs > $divs-new
       ret=$?
       case $ret in
	   101) 
	       echo "Leaving diversion unchanged"
	       exit 0;;
	   102) 
	       quit "Diversion clashes with other" ;;
	   0)
	       ;;
	   *) 
	       quit "Unknown awk retcode $ret";;
       esac
       checkrename $file $divertto
       save $divs
       dorename $file $divertto
       exit 0
       ;;
   remove)
	handler='END { 
             for (file in alts) {
                      if(file != dfile)
                           continue;
                      if ((divertto != "") && (alts[file] != divertto))
                           exit 101;
                      if ((package != "") && (packages[file] != package))
                           exit 102;
                      delete alts[file]; 
                      delete packages[file]; 
                      for (file in alts) { print file; print alts[file]; print packages[file]; }
                      exit 0;
             }
             exit 103;
        } '
       awk -v dfile=$file -v divertto="$divertto" -v package="$package"  \
           "$parser $handler"  < $divs > $divs-new
       ret=$?
       case $ret in
	   101) 
	       quit "Mismatch on divert-to";;
	   102) 
	       quit "Mismatch on package" ;;
	   103)
	       quit "Unknown diversion";;
	   0)
	       ;;
	   *) 
	       quit "Unknown awk retcode $ret";;
       esac
#       dorename=1
       divertto=`truename $file | cut -d" " -f 2`
       checkrename $divertto $file
       save $divs
       dorename  $divertto $file
       exit 0
       
       ;;
    list)
	handler='BEGIN {n = 0; toprint=""};
                       (n == 0) && ($1 ~ reg) { toprint = $1 } 
                       n == 0 { file=$1; n = 1; next; } ;
                       (n == 1) && ($1 ~ reg) { toprint = file };
                       n == 1 { alts = $1; n = 2; next; };
                       (n == 2) && ($1 ~ reg) { toprint = file };
                       (n == 2) && (toprint != "") { print toprint; print alts; print $1 };
                       n == 2 {  n = 0; toprint = ""}'

	awk -v reg=$pattern "$handler"   < $divs
       ;;
    truename)
	truename $file
	;;
   *)
       ;;
esac
