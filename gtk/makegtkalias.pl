#!/usr/bin/perl -w

my $do_def = 0;

if (($#ARGV >= 0) && ($ARGV[0] eq "-def")) {
    shift;
    $do_def = 1;
}

print <<EOF;
/* Generated by makegtkalias.pl */

#ifndef DISABLE_VISIBILITY

#include <glib.h>

#ifdef G_HAVE_GNUC_VISIBILITY

EOF

if ($do_def) {
    print <<EOF
#undef IN_FILE
#define IN_FILE defined

#undef IN_HEADER
#define IN_HEADER(x) 1

EOF
} 
else { 
    print <<EOF
#define IN_FILE(x) 1
#define IN_HEADER defined

EOF
}

my $in_comment = 0;
my $in_skipped_section = 0;

while (<>) {

  # ignore empty lines
  next if /^\s*$/;

  # skip comments
  if ($_ =~ /^\s*\/\*/)
  {
      $in_comment = 1;
  }
  
  if ($in_comment)
  {
      if ($_ =~  /\*\/\s$/)
      {
	  $in_comment = 0;
      }
      
      next;
  }

  # handle ifdefs
  if ($_ =~ /^\#endif/)
  {
      if (!$in_skipped_section)
      {
	  print $_;
      }

      $in_skipped_section = 0;

      next;
  }

  if ($_ =~ /^\#ifdef\s+(INCLUDE_VARIABLES|ALL_FILES)/)
  {
      $in_skipped_section = 1;
  }

  if ($in_skipped_section)
  {
      next;
  }

  if ($_ =~ /^\#ifn?def\s+[GM]/)
  {
      print $_;
      
      next;
  }
 
  if ($_ =~ /^\#if.*(IN_FILE|IN_HEADER)/)
  {
      print $_;
      
      next;
  }

  chop;
  my $str = $_;
  my @words;
  my $attributes = "";

  @words = split(/ /, $str);
  $str = shift(@words);
  chomp($str);
  my $alias = "IA__".$str;
  
  # Drop any Win32 specific .def file syntax,  but keep attributes
  foreach $word (@words) {
      $attributes = "$attributes $word" unless $word eq "PRIVATE";
  }
      
  if (!$do_def) {
    print <<EOF
extern __typeof ($str) $alias __attribute((visibility("hidden")))$attributes;
\#define $str $alias

EOF
  }
  else {
    print <<EOF
\#undef $str 
extern __typeof ($str) $str __attribute((alias("$alias"), visibility("default")));

EOF
  }
}

print <<EOF;
#endif /* G_HAVE_GNUC_VISIBILITY */
#endif /* DISABLE_VISIBILITY */
EOF


