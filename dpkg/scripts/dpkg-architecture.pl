#! /usr/bin/perl
#
# dpkg-architecture
#
# Copyright � 2004-2005 Scott James Remnant <scott@netsplit.com>,
# Copyright � 1999 Marcus Brinkmann <brinkmd@debian.org>.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

$version="1.0.0"; # This line modified by Makefile
$0 = `basename $0`; chomp $0;

$dpkglibdir="/usr/lib/dpkg";
push(@INC,$dpkglibdir);
require 'controllib.pl';

sub usageversion {
    print STDERR
"Debian $0 $version.
Copyright (C) 2004-2005 Scott James Remnant <scott\@netsplit.com>,
Copyright (C) 1999-2001 Marcus Brinkmann <brinkmd\@debian.org>.
This is free software; see the GNU General Public Licence
version 2 or later for copying conditions.  There is NO warranty.

Usage:
  $0 [<option> ...] [<action>]
Options:
       -a<debian-arch>    set current Debian architecture
       -t<gnu-system>     set current GNU system type
       -L                 list valid architectures
       -f                 force flag (override variables set in environment)
Actions:
       -l                 list variables (default)
       -e<debian-arch>    compare with current Debian architecture
       -i<arch-alias>     check if current Debian architecture is <arch-alias>
       -q<variable>       prints only the value of <variable>.
       -s                 print command to set environment variables
       -u                 print command to unset environment variables
       -c <command>       set environment and run the command in it.
";
}

sub split_debian {
    local ($_) = @_;
    
    if (/^([^-]*)-([^-]*)-(.*)/) {
	return ($1, $2, $3);
    } elsif (/^([^-]*)-(.*)/) {
	return ("gnu", $1, $2);
    } elsif (/^armel/) {
	return ("gnueabi", "linux", "arm");
    } else {
	return ("gnu", "linux", $_);
    }
}

sub debian_to_gnu {
    local ($arch) = @_;
    local ($abi, $os, $cpu) = &split_debian($arch);

    return undef unless exists($cputable{$cpu}) && exists($ostable{"$abi-$os"});
    return join("-", $cputable{$cpu}, $ostable{"$abi-$os"});
}

sub split_gnu {
    local ($_) = @_;

    /^([^-]*)-(.*)/;
    return ($1, $2);
}

sub gnu_to_debian {
    local ($gnu) = @_;
    local ($cpu, $os);
    local ($a);

    local ($gnu_cpu, $gnu_os) = &split_gnu($gnu);
    foreach $_cpu (@cpu) {
	if ($gnu_cpu =~ /^$cputable_re{$_cpu}$/) {
	    $cpu = $_cpu;
	    last;
	}
    }

    foreach $_os (@os) {
	if ($gnu_os =~ /^(.*-)?$ostable_re{$_os}$/) {
	    $os = $_os;
	    last;
	}
    }

    return undef if !defined($cpu) || !defined($os);
    return debian_arch_fix($os, $cpu);
}

# Check for -L
if (grep { m/^-L$/ } @ARGV) {
    foreach $os (@os) {
	foreach $cpu (@cpu) {
	    print debian_arch_fix($os, $cpu)."\n";
	}
    }
    exit unless $#ARGV;
}

# Set default values:

chomp ($deb_build_arch = `dpkg --print-architecture`);
&syserr("dpkg --print-architecture failed") if $?>>8;
$deb_build_gnu_type = &debian_to_gnu($deb_build_arch);

# Default host: Current gcc.
$gcc = `\${CC:-gcc} -dumpmachine`;
if ($?>>8) {
    &warn("Couldn't determine gcc system type, falling back to default (native compilation)");
    $gcc = '';
} else {
    chomp $gcc;
}

if ($gcc ne '') {
    $deb_host_arch = &gnu_to_debian($gcc);
    unless (defined $deb_host_arch) {
	&warn ("Unknown gcc system type $gcc, falling back to default (native compilation)");
	$gcc = '';
    } else {
	$gcc = $deb_host_gnu_type = &debian_to_gnu($deb_host_arch);
    }
}
if (!defined($deb_host_arch)) {
    # Default host: Native compilation.
    $deb_host_arch = $deb_build_arch;
    $deb_host_gnu_type = $deb_build_gnu_type;
}


$req_host_arch = '';
$req_host_gnu_type = '';
$req_build_gnu_type = '';
$req_eq_arch = '';
$req_is_arch = '';
$action='l';
$force=0;

while (@ARGV) {
    $_=shift(@ARGV);
    if (m/^-a/) {
	$req_host_arch = "$'";
    } elsif (m/^-t/) {
	$req_host_gnu_type = "$'";
    } elsif (m/^-e/) {
	$req_eq_arch = "$'";
	$action = 'e';
    } elsif (m/^-i/) {
	$req_is_arch = "$'";
	$action = 'i';
    } elsif (m/^-[lsu]$/) {
	$action = $_;
	$action =~ s/^-//;
    } elsif (m/^-f$/) {
        $force=1;
    } elsif (m/^-q/) {
        $req_variable_to_print = "$'";
        $action = 'q';
    } elsif (m/^-c$/) {
       $action = 'c';
       last;
    } elsif (m/^-L$/) {
       # Handled already
    } else {
	usageerr("unknown option \`$_'");
    }
}

if ($req_host_arch ne '' && $req_host_gnu_type eq '') {
    $req_host_gnu_type = &debian_to_gnu ($req_host_arch);
    die ("unknown Debian architecture $req_host_arch, you must specify \GNU system type, too") unless defined $req_host_gnu_type;
}

if ($req_host_gnu_type ne '' && $req_host_arch eq '') {
    $req_host_arch = &gnu_to_debian ($req_host_gnu_type);
    die ("unknown GNU system type $req_host_gnu_type, you must specify Debian architecture, too") unless defined $req_host_arch;
}

if ($req_host_gnu_type ne '' && $req_host_arch ne '') {
    $dfl_host_gnu_type = &debian_to_gnu ($req_host_arch);
    &warn("Default GNU system type $dfl_host_gnu_type for Debian arch $req_host_arch does not match specified GNU system type $req_host_gnu_type") if $dfl_host_gnu_type ne $req_host_gnu_type;
}

$deb_host_arch = $req_host_arch if $req_host_arch ne '';
$deb_host_gnu_type = $req_host_gnu_type if $req_host_gnu_type ne '';

#$gcc = `\${CC:-gcc} --print-libgcc-file-name`;
#$gcc =~ s!^.*gcc-lib/(.*)/\d+(?:.\d+)*/libgcc.*$!$1!s;
&warn("Specified GNU system type $deb_host_gnu_type does not match gcc system type $gcc.") if !($req_is_arch or $req_eq_arch) && ($gcc ne '') && ($gcc ne $deb_host_gnu_type);

# Split the Debian and GNU names
($deb_host_arch_abi, $deb_host_arch_os, $deb_host_arch_cpu) = &split_debian($deb_host_arch);
($deb_build_arch_abi, $deb_build_arch_os, $deb_build_arch_cpu) = &split_debian($deb_build_arch);
($deb_host_gnu_cpu, $deb_host_gnu_system) = &split_gnu($deb_host_gnu_type);
($deb_build_gnu_cpu, $deb_build_gnu_system) = &split_gnu($deb_build_gnu_type);

%env = ();
if (!$force) {
    $deb_build_arch = $ENV{DEB_BUILD_ARCH} if (exists $ENV{DEB_BUILD_ARCH});
    $deb_build_arch_abi = $ENV{DEB_BUILD_ARCH_ABI} if (exists $ENV{DEB_BUILD_ARCH_ABI});
    $deb_build_arch_os = $ENV{DEB_BUILD_ARCH_OS} if (exists $ENV{DEB_BUILD_ARCH_OS});
    $deb_build_arch_cpu = $ENV{DEB_BUILD_ARCH_CPU} if (exists $ENV{DEB_BUILD_ARCH_CPU});
    $deb_build_gnu_cpu = $ENV{DEB_BUILD_GNU_CPU} if (exists $ENV{DEB_BUILD_GNU_CPU});
    $deb_build_gnu_system = $ENV{DEB_BUILD_GNU_SYSTEM} if (exists $ENV{DEB_BUILD_GNU_SYSTEM});
    $deb_build_gnu_type = $ENV{DEB_BUILD_GNU_TYPE} if (exists $ENV{DEB_BUILD_GNU_TYPE});
    $deb_host_arch = $ENV{DEB_HOST_ARCH} if (exists $ENV{DEB_HOST_ARCH});
    $deb_host_arch_abi = $ENV{DEB_HOST_ARCH_ABI} if (exists $ENV{DEB_HOST_ARCH_ABI});
    $deb_host_arch_os = $ENV{DEB_HOST_ARCH_OS} if (exists $ENV{DEB_HOST_ARCH_OS});
    $deb_host_arch_cpu = $ENV{DEB_HOST_ARCH_CPU} if (exists $ENV{DEB_HOST_ARCH_CPU});
    $deb_host_gnu_cpu = $ENV{DEB_HOST_GNU_CPU} if (exists $ENV{DEB_HOST_GNU_CPU});
    $deb_host_gnu_system = $ENV{DEB_HOST_GNU_SYSTEM} if (exists $ENV{DEB_HOST_GNU_SYSTEM});
    $deb_host_gnu_type = $ENV{DEB_HOST_GNU_TYPE} if (exists $ENV{DEB_HOST_GNU_TYPE});
}

@ordered = qw(DEB_BUILD_ARCH DEB_BUILD_ARCH_ABI DEB_BUILD_ARCH_OS DEB_BUILD_ARCH_CPU
	      DEB_BUILD_GNU_CPU DEB_BUILD_GNU_SYSTEM DEB_BUILD_GNU_TYPE
	      DEB_HOST_ARCH DEB_HOST_ARCH_ABI DEB_HOST_ARCH_OS DEB_HOST_ARCH_CPU
	      DEB_HOST_GNU_CPU DEB_HOST_GNU_SYSTEM DEB_HOST_GNU_TYPE);

$env{'DEB_BUILD_ARCH'}=$deb_build_arch;
$env{'DEB_BUILD_ARCH_ABI'}=$deb_build_arch_abi;
$env{'DEB_BUILD_ARCH_OS'}=$deb_build_arch_os;
$env{'DEB_BUILD_ARCH_CPU'}=$deb_build_arch_cpu;
$env{'DEB_BUILD_GNU_CPU'}=$deb_build_gnu_cpu;
$env{'DEB_BUILD_GNU_SYSTEM'}=$deb_build_gnu_system;
$env{'DEB_BUILD_GNU_TYPE'}=$deb_build_gnu_type;
$env{'DEB_HOST_ARCH'}=$deb_host_arch;
$env{'DEB_HOST_ARCH_ABI'}=$deb_host_arch_abi;
$env{'DEB_HOST_ARCH_OS'}=$deb_host_arch_os;
$env{'DEB_HOST_ARCH_CPU'}=$deb_host_arch_cpu;
$env{'DEB_HOST_GNU_CPU'}=$deb_host_gnu_cpu;
$env{'DEB_HOST_GNU_SYSTEM'}=$deb_host_gnu_system;
$env{'DEB_HOST_GNU_TYPE'}=$deb_host_gnu_type;

if ($action eq 'l') {
    foreach $k (@ordered) {
	print "$k=$env{$k}\n";
    }
} elsif ($action eq 's') {
    foreach $k (@ordered) {
	print "$k=$env{$k}; ";
    }
    print "export ".join(" ",@ordered)."\n";
} elsif ($action eq 'u') {
    print "unset ".join(" ",@ordered)."\n";
} elsif ($action eq 'e') {
    exit !debian_arch_eq($deb_host_arch, $req_eq_arch);
} elsif ($action eq 'i') {
    exit !debian_arch_is($deb_host_arch, $req_is_arch);
} elsif ($action eq 'c') {
    @ENV{keys %env} = values %env;
    exec @ARGV;
} elsif ($action eq 'q') {
    if (exists $env{$req_variable_to_print}) {
        print "$env{$req_variable_to_print}\n";
    } else {
        die "$req_variable_to_print is not a supported variable name";
    }
}