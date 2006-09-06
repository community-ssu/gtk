#!/usr/bin/perl --
#
# dselect - Debian package maintenance user interface
# mkcurkeys.pl - generate strings mapping key names to ncurses numbers
#
# Copyright (C) 1995 Ian Jackson <ian@chiark.greenend.org.uk>
#
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2,
# or (at your option) any later version.
#
# This is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public
# License along with this; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

$#ARGV == 1 || die ("usage: mkcurkeys.pl <filename> <curses.h>");

open(OV,"<$ARGV[0]") || die $!;
while (<OV>) {
    chomp;
    /^#/ && next;		# skip comments
    /\S/ || next;		# ignore blank lines
    /^(\w+)\s+(\S.*\S)\s*$/ || die ("cannot parse line:\n$_\n");
    $over{$1}= $2;
    $base{$1}= '';
}
close(OV);

for ($i=1, $let='A'; $i<=26; $i++, $let++) {
    $name{$i}= "^$let";
    $base{$i}= '';
}

open(NCH,"<$ARGV[1]") || die $!;
while (<NCH>) {
    s/\s+$//;
    m/#define KEY_(\w+)\s+\d+\s+/ || next;
    $rhs= $';
    $k= "KEY_$1";
    $_= $1;
    &capit;
    $base{$k}= $_;
    $_= $rhs;
    s/(\w)[\(\)]/$1/g;
    s/\w+ \((\w+)\)/$1/;
    next unless m|^/\* (\w[\w ]+\w) \*/$|;
    $_= $1;
    s/ key$//;
    next if s/^shifted /shift / ? m/ .* .* / : m/ .* /;
    &capit;
    $name{$k}= $_;
}
close(NCH);

printf(<<'END') || die $!;
/*
 * WARNING - THIS FILE IS GENERATED AUTOMATICALLY - DO NOT EDIT
 * It is generated by mkcurkeys.pl from <curses.h>
 * and keyoverride.  If you want to override things try adding
 * them to keyoverride.
 */

END

for ($i=33; $i<=126; $i++) {
    $k= $i;
    $v= pack("C",$i);
    if ($v eq ',') { $comma=$k; next; }
    &p;
}

for $k (sort {
    $a+0 eq $a ?
        $b+0 eq $b ? $a <=> $b : -1
            : $b+0 eq $b ? 1 :
                $a cmp $b
                } keys %base) {
    $v= $base{$k};
    $v= $name{$k} if defined($name{$k});
    $v= $over{$k} if defined($over{$k});
    next if $v eq '[elide]';
    &p;
}

for ($i=1; $i<64; $i++) {
    $k= "KEY_F($i)"; $v= "F$i";
    &p;
}

$k=$comma; $v=','; &p;

print(<<'END') || die $!;
  { -1,              0                    }
END

close(STDOUT) || die $!;
exit(0);

sub capit {
    $o= ''; y/A-Z/a-z/; $_= " $_";
    while (m/ (\w)/) {
        $o .= $`.' ';
        $_ = $1;
        y/a-z/A-Z/;
        $o .= $_;
        $_ = $';
    }
    $_= $o.$_; s/^ //;
}

sub p {
    $v =~ s/["\\]/\\$&/g;
    printf("  { %-15s \"%-20s },\n",
           $k.',',
           $v.'"') || die $!;
}
