/*
 * This file is part of osso-application-installer
 *
 * Parts of this file are derived from apt.  Apt is copyright 1997,
 * 1998, 1999 Jason Gunthorpe and others.
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

#include <apt-pkg/init.h>
#include <apt-pkg/error.h>
#include <apt-pkg/tagfile.h>
#include <apt-pkg/pkgcache.h>
#include <apt-pkg/pkgcachegen.h>
#include <apt-pkg/sourcelist.h>
#include <apt-pkg/acquire.h>
#include <apt-pkg/acquire-item.h>
#include <apt-pkg/cachefile.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/sptr.h>
#include <apt-pkg/packagemanager.h>

#include <glib/gstring.h>
#include <glib/gstrfuncs.h>
#include <glib/gmem.h>

#include "apt-worker-proto.h"

using namespace std;

#define DEBUG

/* This is the process that runs as root and does all the work.

   It is started from a separate program (as opposed to being forked
   directly from the GUI process) since that allows us to use sudo for
   starting it.

   This process communicates with the GUI process via some pipes that
   are inherited from that process.  You can't really use it from the
   command line.

   It will output stuff to stdin and stderr, which the GUI process is
   supposed to catch and put into its log.

   Invocation: apt-worker input-fifo output-fifo status-fifo cancel-fifo
 */

int input_fd, output_fd, status_fd, cancel_fd;

void
log_stderr (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  fprintf (stderr, "apt-worker: ");
  vfprintf (stderr, fmt, args);
  va_end (args);
  fprintf (stderr, "\n");
}

#ifdef DEBUG
#define DBG log_stderr
#else
#define DBG(...)
#endif

void
must_read (void *buf, size_t n)
{
  int r;

  /* XXX - maybe use streams here for buffering.
   */
  while (n > 0)
    {
      r = read (input_fd, buf, n);
      if (r < 0)
	{
	  perror ("read");
	  exit (1);
	}
      else if (r == 0)
	{
	  DBG ("exiting");
	  exit (0);
	}
      n -= r;
      buf = ((char *)buf) + r;
    }
}

static void
must_write (void *buf, size_t n)
{
  if (n > 0 && write (output_fd, buf, n) != n)
    {
      perror ("write");
      exit (1);
    }
}

static int
maybe_read_byte (int fd)
{
  fd_set set;
  FD_ZERO (&set);
  FD_SET (fd, &set);
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;

  if (select (fd+1, &set, NULL, NULL, &timeout) > 0)
    {
      if (FD_ISSET (fd, &set))
	{
	  unsigned char byte;
	  if (read (fd, &byte, 1) == 1)
	    return byte;
	}
    }
  return -1;
}

static void
drain_fd (int fd)
{
  while (maybe_read_byte (fd) >= 0)
    ;
}

char *
alloc_buf (int len, char *stack_buf, int stack_buf_len)
{
  if (len <= stack_buf_len)
    return stack_buf;
  else
    return new char[len];
}

void
free_buf (char *buf, char *stack_buf)
{
  if (buf != stack_buf)
    delete[] buf;
}

static pkgCacheFile *package_cache = NULL;

void
send_response_raw (int cmd, int seq, void *response, size_t len)
{
  apt_response_header res = { cmd, seq, len };
  must_write (&res, sizeof (res));
  must_write (response, len);
}

apt_proto_decoder request;
apt_proto_encoder response;

void
send_response (apt_request_header *req)
{
  send_response_raw (req->cmd, req->seq,
		     response.get_buf (), response.get_len ());
}

void
send_status (const char *label, int percent)
{

  static apt_proto_encoder status_response;
  static char last_label[80];
  static int last_percent;

  percent -= percent % 10;

  if (percent != last_percent
      || strncmp (label, last_label, 80))
    {
      last_percent = percent;
      strncpy (last_label, label, 80);
      
      status_response.reset ();
      status_response.encode_string (label);
      status_response.encode_int (percent);
      send_response_raw (APTCMD_STATUS, -1, 
			 status_response.get_buf (),
			 status_response.get_len ());
    }
}

void cache_init ();
void make_package_list_response (bool only_maemo,
				 bool only_installed,
				 bool only_available,
				 const char *pattern);
void make_package_info_response (const char *package);
void make_package_details_response (const char *package,
				    const char *version,
				    int summary_kind);

bool update_package_cache ();
void make_get_sources_list_response ();
bool set_sources_list ();

void install_check (const char *package);
void install_package (const char *package);
bool remove_package (const char *package);
bool clean ();

bool install_file (const char *filename);
void make_file_details_response (const char *filename);

void
handle_request ()
{
  apt_request_header req;
  char stack_reqbuf[4096];
  char *reqbuf;

  must_read (&req, sizeof (req));
  DBG ("got req %d/%d/%d", req.cmd, req.seq, req.len);

  reqbuf = alloc_buf (req.len, stack_reqbuf, 4096);
  must_read (reqbuf, req.len);

  drain_fd (cancel_fd);

  switch (req.cmd)
    {
    case APTCMD_NOOP:
      {
	send_response_raw (req.cmd, req.seq, NULL, 0);
	break;
      }
    case APTCMD_GET_PACKAGE_LIST:
      {
	request.reset (reqbuf, req.len);
	bool only_maemo = request.decode_int ();
	bool only_installed = request.decode_int ();
	bool only_available = request.decode_int ();
	const char *pattern = request.decode_string_in_place ();
	make_package_list_response (only_maemo,
				    only_installed,
				    only_available,
				    pattern);
	_error->DumpErrors ();
	send_response (&req);
	break;
      }
    case APTCMD_GET_PACKAGE_INFO:
      {
	request.reset (reqbuf, req.len);
	const char *package = request.decode_string_in_place ();
	make_package_info_response (package);
	_error->DumpErrors ();
	send_response (&req);
	break;
      }
    case APTCMD_GET_PACKAGE_DETAILS:
      {
	request.reset (reqbuf, req.len);
	const char *package = request.decode_string_in_place ();
	const char *version = request.decode_string_in_place ();
	int summary_kind = request.decode_int ();
	make_package_details_response (package, version, summary_kind);
	_error->DumpErrors ();
	send_response (&req);
	break;
      }
    case APTCMD_UPDATE_PACKAGE_CACHE:
      {
	response.reset ();
	bool success = update_package_cache ();
	_error->DumpErrors ();
	response.encode_int (success? 1 : 0);
	send_response (&req);
	break;
      }
    case APTCMD_GET_SOURCES_LIST:
      {
	make_get_sources_list_response ();
	_error->DumpErrors ();
	send_response (&req);
	break;
      }
    case APTCMD_SET_SOURCES_LIST:
      {
	response.reset ();
	request.reset (reqbuf, req.len);
	bool success = set_sources_list ();
	_error->DumpErrors ();
	response.encode_int (success? 1 : 0);
	send_response (&req);
	break;
      }
    case APTCMD_INSTALL_CHECK:
      {
	request.reset (reqbuf, req.len);
	const char *package = request.decode_string_in_place ();
	install_check (package);
	send_response (&req);
	break;
      }
    case APTCMD_INSTALL_PACKAGE:
      {
	request.reset (reqbuf, req.len);
	const char *package = request.decode_string_in_place ();
	install_package (package);
	send_response (&req);
	break;
      }
    case APTCMD_REMOVE_PACKAGE:
      {
	response.reset ();
	request.reset (reqbuf, req.len);
	const char *package = request.decode_string_in_place ();
	bool success = remove_package (package);
	_error->DumpErrors ();
	response.encode_int (success? 1 : 0);
	send_response (&req);
	break;
      }
    case APTCMD_CLEAN:
      {
	response.reset ();
	bool success = clean ();
	_error->DumpErrors ();
	response.encode_int (success? 1 : 0);
	send_response (&req);
	break;
      }
    case APTCMD_GET_FILE_DETAILS:
      {
	request.reset (reqbuf, req.len);
	const char *filename = request.decode_string_in_place ();
	make_file_details_response (filename);
	send_response (&req);
	break;
      }
    case APTCMD_INSTALL_FILE:
      {
	response.reset ();
	request.reset (reqbuf, req.len);
	const char *filename = request.decode_string_in_place ();
	bool success = install_file (filename);
	response.encode_int (success? 1 : 0);
	send_response (&req);
	break;
      }
    default:
      {
	log_stderr ("unrecognized request: %d", req.cmd);
	send_response_raw (req.cmd, req.seq, NULL, 0);
	break;
      }
    }

  free_buf (reqbuf, stack_reqbuf);
}

static int
must_open (char *filename, int flags)
{
  int fd = open (filename, flags);
  if (fd < 0)
    {
      perror (filename);
      exit (1);
    }
}

int
main (int argc, char **argv)
{
  if (argc != 5)
    {
      log_stderr ("wrong invocation");
      exit (1);
    }

  DBG ("starting up");

  input_fd = must_open (argv[1], O_RDONLY);
  output_fd = must_open (argv[2], O_WRONLY);
  status_fd = must_open (argv[3], O_WRONLY);
  cancel_fd = must_open (argv[4], O_RDONLY);

  DBG ("starting with pid %d, in %d, out %d, stat %d, cancel %d",
       getpid (), input_fd, output_fd, status_fd, cancel_fd);

  cache_init ();

  while (true)
    handle_request ();
}


class myProgress : public OpProgress
{
  virtual void
  Update ()
  {
    send_status ("updating", (int)Percent);
  }
};

class myAcquireStatus : public pkgAcquireStatus
{
  virtual bool
  MediaChange (string Media, string Drive)
  {
    return false;
  }

  virtual bool
  Pulse (pkgAcquire *Owner)
  {
    pkgAcquireStatus::Pulse (Owner);

    float fraction = (double(CurrentBytes + CurrentItems) / 
		      double(TotalBytes + TotalItems));
    send_status ("downloading", (int)(fraction*100));

    if (maybe_read_byte (cancel_fd) >= 0)
      return false;

    return true;
  }
};

// Specific command handlers

void
cache_init ()
{
  myProgress progress;

  if (package_cache)
    {
      DBG ("closing");
      package_cache->Close ();
      delete package_cache;
      DBG ("done");
    }

  package_cache = new pkgCacheFile;

  DBG ("init.");
  if (pkgInitConfig(*_config) == false ||
      pkgInitSystem(*_config,_system) == false ||
      package_cache->Open (progress) == false)
   {
     // XXX - launch into a deaf, blind and dumb mode.
     DBG ("failed.");
     _error->DumpErrors();
     exit (1);
   }
}

bool
is_maemo_package (pkgCache::VerIterator &ver)
{
  const char *section = ver.Section ();
 
  return section && !strncmp (section, "maemo/", 6);
}

bool
name_matches_pattern (pkgCache::PkgIterator &pkg,
		      const char *pattern)
{
  return strcasestr (pkg.Name(), pattern);
}

bool
description_matches_pattern (pkgCache::VerIterator &ver,
			     const char *pattern)
{
  pkgRecords Recs (*package_cache);
  pkgRecords::Parser &P = Recs.Lookup (ver.FileList ());
  const char *desc = P.LongDesc().c_str();

  return strcasestr (desc, pattern);
}

void
make_package_list_response (bool only_maemo,
			    bool only_installed,
			    bool only_available,
			    const char *pattern)
{
  pkgCache &cache = *package_cache;

  DBG ("pattern %s", pattern);

  response.reset ();
  for (pkgCache::PkgIterator pkg = cache.PkgBegin();
       pkg != cache.PkgEnd();
       pkg++)
    {
      pkgCache::VerIterator installed = pkg.CurrentVer ();

      // skip non maemo packages if requested
      //
      if (only_maemo
	  && !installed.end ()
	  && !is_maemo_package (installed))
	continue;

      // skip not-installed packaged if requested
      //
      if (only_installed
	  && installed.end ())
	continue;

      bool have_latest = false;
      pkgCache::VerIterator latest;

      for (pkgCache::VerIterator ver = pkg.VersionList();
	   ver.end() != true; ver++)
	{
	  if (!have_latest || latest.CompareVer (ver) < 0)
	    {
	      latest = ver;
	      have_latest = true;
	    }
	}

      // skip non maemo packages if requested
      //
      if (only_maemo
	  && have_latest
	  && !is_maemo_package (latest))
	continue;

      // skip non-available packages if requested
      //
      if (only_available
	  && !have_latest)
	continue;

      // skip packages that are not installed and not available
      //
      if (installed.end () && !have_latest)
	continue;

      // skip packages that don't match the pattern if requested
      //
      if (pattern
	  && !(name_matches_pattern (pkg, pattern)
	       || (!installed.end ()
		   && description_matches_pattern (installed, pattern))
	       || (have_latest
		   && description_matches_pattern (latest, pattern))))
	continue;
      
      // Name
      response.encode_string (pkg.Name ());

      // Installed version and installed size
      if (!installed.end())
	{
	  response.encode_string (installed.VerStr ());
	  response.encode_int (installed->InstalledSize);
	}
      else
	{
	  response.encode_string (NULL);
	  response.encode_int (0);
	}

      // Available version and section
      if (have_latest && installed.CompareVer (latest) < 0)
	{
	  response.encode_string (latest.VerStr ());
	  response.encode_string (latest.Section ());
	}
      else
	{
	  response.encode_string (NULL);
	  response.encode_string (NULL);
	}
    }
}

char *
get_short_description (pkgCache::VerIterator &ver)
{
  pkgRecords Recs (*package_cache);
  pkgRecords::Parser &P = Recs.Lookup (ver.FileList ());
  return g_strdup (P.ShortDesc().c_str());
}

int
all_white_space (const char *text)
{
  while (*text)
    if (!isspace (*text++))
      return 0;
  return 1;
}

char *
get_icon (pkgCache::VerIterator &ver)
{
  // XXX - merge with get_short_description to only setup the parser
  //       once.

  pkgRecords Recs (*package_cache);
  pkgRecords::Parser &P = Recs.Lookup (ver.FileList ());

  const char *start, *stop;
  P.GetRec (start, stop);

  /* NOTE: pkTagSection::Scan only succeeds when the records in two
           newlines, but pkgRecords::Parser::GetRec does not include the
           second newline in its returned region.  However, that second
           newline is always there, so we just pass one more character to
           Scan.
  */

  pkgTagSection section;
  if (!section.Scan (start, stop-start+1))
    return NULL;

  char *icon = g_strdup (section.FindS ("X-Maemo-Icon-26").c_str());
  if (all_white_space (icon))
    {
      g_free (icon);
      return NULL;
    }
  else
    return icon;
}

void
make_package_info_response (const char *package)
{
  int old_broken_count;
  apt_proto_package_info info;
  char *installed_description;
  char *available_description;
  char *installed_icon;
  char *available_icon;

  pkgDepCache &cache = *package_cache;
  pkgCache::PkgIterator pkg = cache.FindPkg (package);

  if (!pkg.end ())
    {
      pkgCache::VerIterator inst = pkg.CurrentVer ();
      if (!inst.end ())
	{
	  installed_description = get_short_description (inst);
	  installed_icon = get_icon (inst);
	}
      else
	{
	  installed_description = NULL;
	  installed_icon = NULL;
	}

      // simulate install

      cache.Init(NULL); // XXX - this is very slow

      old_broken_count = cache.BrokenCount();
      (*package_cache)->MarkInstall (pkg);
      if (cache.BrokenCount() > old_broken_count)
	{
	  info.installable = 0;
	  info.download_size = 0;
	  info.install_user_size_delta = 0;
	}
      else
	{
	  info.installable = 1;
	  info.download_size = (int) cache.DebSize ();
	  info.install_user_size_delta = (int) cache.UsrSize ();
	}

      pkgCache::VerIterator avail (cache, cache[pkg].CandidateVer);
      if (!avail.end ())
	{
	  available_description = get_short_description (avail);
	  available_icon = get_icon (avail);
	}
      else
	{
	  available_description = NULL;
	  available_icon = NULL;
	}
      
      // simulate remove

      cache.Init(NULL); // XXX - this is very slow
      
      old_broken_count = cache.BrokenCount();
      (*package_cache)->MarkDelete (pkg);
      if (cache.BrokenCount() > old_broken_count)
	{
	  info.removable = 0;
	  info.remove_user_size_delta = 0;
	}
      else
	{
	  info.removable = 1;
	  info.remove_user_size_delta = (int) cache.UsrSize ();
	}

      // We might sleep here to simulate the slowness of this
      // operation on the device.
      //
      //sleep (2);
    }
  else
    {
      info.installable = 0;
      info.download_size = 0;
      info.install_user_size_delta = 0;
      info.removable = 0;
      info.remove_user_size_delta = 0;
      installed_description = NULL;
      available_description = NULL;
      installed_icon = NULL;
      available_icon = NULL;
    }

  response.reset ();
  response.encode_mem (&info, sizeof (apt_proto_package_info));
  response.encode_string (installed_description);
  response.encode_string (installed_icon);
  if (available_description && installed_description
      && !strcmp (available_description, installed_description))
    response.encode_string (NULL);
  else
    response.encode_string (available_description);
  if (available_icon && installed_icon
      && !strcmp (available_icon, installed_icon))
    response.encode_string (NULL);
  else
    response.encode_string (available_icon);
  g_free (installed_description);
  g_free (installed_icon);
  g_free (available_description);
  g_free (available_icon);
}

void
encode_dependencies (pkgCache::VerIterator &ver)
{
  for (pkgCache::DepIterator dep = ver.DependsList(); !dep.end(); )
    {
      GString *str;
      apt_proto_deptype type;
      pkgCache::DepIterator start;
      pkgCache::DepIterator end;
      dep.GlobOr(start, end);

      if (start->Type == pkgCache::Dep::PreDepends ||
	  start->Type == pkgCache::Dep::Depends)
	type = deptype_depends;
      else if (start->Type == pkgCache::Dep::Conflicts)
	type = deptype_conflicts;
      else 
	continue;

      str = g_string_new ("");
      while (1)
	{
	  g_string_append_printf (str, " %s", start.TargetPkg().Name());
	  if (start.TargetVer() != 0)
	    g_string_append_printf (str, " (%s %s)",
				    start.CompType(), start.TargetVer());
	  
	  if (start == end)
	    break;
	  g_string_append_printf (str, " |");
	  start++;
	}

      response.encode_int (type);
      response.encode_string (str->str);
      g_string_free (str, 1);
    }

  response.encode_int (deptype_end);
}

void
encode_broken (pkgCache::PkgIterator &pkg, pkgCache::PkgIterator &want)
{
  pkgDepCache &Cache = *package_cache;
  pkgCache::VerIterator Ver = Cache[pkg].InstVerIter(Cache);
      
  if (Ver.end() == true)
    return;
      
  for (pkgCache::DepIterator D = Ver.DependsList(); D.end() == false;)
    {
      GString *str;
      apt_proto_sumtype type;

      // Compute a single dependency element (glob or)
      pkgCache::DepIterator Start;
      pkgCache::DepIterator End;
      D.GlobOr(Start,End); // advances D

      if ((Cache[End] & pkgDepCache::DepGInstall)
	  == pkgDepCache::DepGInstall)
	continue;

      if (Start->Type == pkgCache::Dep::PreDepends ||
	  Start->Type == pkgCache::Dep::Depends)
	type = sumtype_missing;
      else if (Start->Type == pkgCache::Dep::Conflicts)
	type = sumtype_conflicting;
      else
	continue;

      str = g_string_new ("");
      while (1)
	{
	  /* Show a summary of the target package if possible. In the case
	     of virtual packages we show nothing 
	  */
	  pkgCache::PkgIterator target = Start.TargetPkg ();

	  /* Never blame conflicts on the package that we want to
	     install.
	  */
	  if (target == want && Start->Type == pkgCache::Dep::Conflicts)
	    g_string_append_printf (str, "%s", pkg.Name());
	  else
	    {
	      g_string_append_printf (str, "%s", Start.TargetPkg().Name());
	      if (Start.TargetVer() != 0)
		g_string_append_printf (str, " %s %s",
					Start.CompType(), Start.TargetVer());
	    }

	  if (Start != End)
	    g_string_append_printf (str, " | ");
	  
	  if (Start == End)
	    break;
	  Start++;
	}

      response.encode_int (type);
      response.encode_string (str->str);
      g_string_free (str, 1);
    }
}

void
encode_package_and_version (const char *package, const char *version)
{
  GString *str = g_string_new ("");
  g_string_printf (str, "%s %s", package, version);
  response.encode_string (str->str);
  g_string_free (str, 1);
}

void
encode_install_summary (pkgCache::PkgIterator &want)
{
  pkgDepCache &cache = *package_cache;

  cache.Init(NULL);

  if (cache.BrokenCount() > 0)
    fprintf (stderr, "[ Some installed packages are broken! ]\n");

  (*package_cache)->MarkInstall (want);

  for (pkgCache::PkgIterator pkg = cache.PkgBegin();
       pkg.end() != true;
       pkg++)
    {
      apt_proto_sumtype type;

      if (cache[pkg].NewInstall())
	type = sumtype_installing;
      else if (cache[pkg].Upgrade())
	type = sumtype_upgrading;
      else if (cache[pkg].Delete())
	type = sumtype_removing;
      else 
	type = sumtype_end;

      if (type != sumtype_end)
	{
	  response.encode_int (type);
	  encode_package_and_version (pkg.Name(), cache[pkg].CandVersion);
	}

      if (cache[pkg].InstBroken())
	encode_broken (pkg, want);
    }

  response.encode_int (sumtype_end);
}

void
encode_remove_summary (pkgCache::PkgIterator &want)
{
  pkgDepCache &cache = *package_cache;

  cache.Init(NULL);

  if (cache.BrokenCount() > 0)
    log_stderr ("[ Some installed packages are broken! ]\n");

  (*package_cache)->MarkDelete (want);

  for (pkgCache::PkgIterator pkg = cache.PkgBegin();
       pkg.end() != true;
       pkg++)
    {
      apt_proto_sumtype type;

      if (cache[pkg].Delete())
	{
	  response.encode_int (sumtype_removing);
	  encode_package_and_version (pkg.Name(), cache[pkg].CandVersion);
	}

      if (cache[pkg].InstBroken())
	{
	  response.encode_int (sumtype_needed_by);
	  encode_package_and_version (pkg.Name(), cache[pkg].CurVersion);
	}
    }

  response.encode_int (sumtype_end);
}

bool
find_package_version (pkgDepCache &cache,
		      pkgCache::PkgIterator &pkg,
		      pkgCache::VerIterator &ver,
		      const char *package, const char *version)
{
  if (package == NULL || version == NULL)
    return false;

  pkg = cache.FindPkg (package);
  if (!pkg.end ())
    {
      for (ver = pkg.VersionList(); ver.end() != true; ver++)
	if (!strcmp (ver.VerStr (), version))
	  return true;
    }
  return false;
}

void
make_package_details_response (const char *package, const char *version,
			       int summary_kind)
{
  response.reset ();

  pkgDepCache &cache = *package_cache;
  pkgCache::PkgIterator pkg;
  pkgCache::VerIterator ver;

  if (find_package_version (cache, pkg, ver, package, version))
    {
      pkgRecords Recs (cache);
      pkgRecords::Parser &P = Recs.Lookup (ver.FileList ());

      response.encode_string (P.Maintainer().c_str());
      response.encode_string (P.LongDesc().c_str());
      encode_dependencies (ver);
      if (summary_kind == 1)
	encode_install_summary (pkg);
      else if (summary_kind == 2)
	encode_remove_summary (pkg);
      else
	response.encode_int (sumtype_end);
    }
  else
    {
      // not found
      response.encode_string (NULL);      // maintainer
      response.encode_string (NULL);      // description
      response.encode_int (deptype_end);  // dependencies
      response.encode_int (sumtype_end);  // summary
    }
}

bool
update_package_cache ()
{
  // Get the source list
  pkgSourceList List;
  if (List.ReadMainList () == false)
    return false;

  // Lock the list directory
  FileFd Lock;
  if (_config->FindB("Debug::NoLocking",false) == false)
    {
      Lock.Fd (GetLock (_config->FindDir("Dir::State::Lists") + "lock"));
      if (_error->PendingError () == true)
	return _error->Error ("Unable to lock the list directory");
    }
   
  // Create the download object
  myAcquireStatus Stat;
  pkgAcquire Fetcher(&Stat);

  // Populate it with the source selection
  if (List.GetIndexes(&Fetcher) == false)
    return false;
   
  // Run it
  if (Fetcher.Run() == pkgAcquire::Failed)
    return false;

  bool Failed = false;
  for (pkgAcquire::ItemIterator I = Fetcher.ItemsBegin();
       I != Fetcher.ItemsEnd(); I++)
    {
      if ((*I)->Status == pkgAcquire::Item::StatDone)
	continue;
      
      (*I)->Finished();
      
      _error->Error ("Failed to fetch %s  %s", (*I)->DescURI().c_str(),
		     (*I)->ErrorText.c_str());
      Failed = true;
    }
  
  // Clean out any old list files
  if (_config->FindB("APT::Get::List-Cleanup",true) == true)
    {
      if (Fetcher.Clean(_config->FindDir("Dir::State::lists")) == false ||
	  Fetcher.Clean(_config->FindDir("Dir::State::lists") + "partial/") == false)
	return false;
    }
   
  // Prepare the cache.   
  {
    myProgress Prog;
    if (!package_cache->BuildCaches (Prog))
      return false;
    cache_init ();
  }

  return !Failed;
}

bool
encode_file_response (const char *name)
{
  int fd = open (name, O_RDONLY);
  if (fd < 0)
    {
    fail:
      perror (name);
      response.encode_string ("");
      return false;
    }

  struct stat buf;
  if (fstat (fd, &buf) < 0)
    goto fail;
  
  DBG ("size %d", buf.st_size);

  // XXX
  char *mem = new char[buf.st_size+1];
  int n = read (fd, mem, buf.st_size);
  mem[buf.st_size-1] = '\0';
  close (fd);

  DBG ("read %d", n);

  response.encode_string (mem);
  delete[] mem;

  return true;
}
  
void
make_get_sources_list_response ()
{
  response.reset ();

  string name = _config->FindFile("Dir::Etc::sourcelist");
  FILE *f = fopen (name.c_str(), "r");

  if (f)
    {
      char *line = NULL;
      size_t len = 0;
      ssize_t n;

      while ((n = getline (&line, &len, f)) != -1)
	{
	  if (n > 0 && line[n-1] == '\n')
	    line[n-1] = '\0';

	  response.encode_string (line);
	}
      response.encode_string (NULL);
      response.encode_int (1);

      free (line);
      fclose (f);
    }
  else
    {
      perror (name.c_str());
      response.encode_string (NULL);
      response.encode_int (0);
    }
}

bool
set_sources_list ()
{
  string name = _config->FindFile("Dir::Etc::sourcelist");
  FILE *f = fopen (name.c_str(), "w");
  
  if (f)
    {
      while (true)
	{
	  const char *str = request.decode_string_in_place ();
	  if (str == NULL)
	    break;
	  fprintf (f, "%s\n", str);
	}
      fclose (f);
      return true;
    }
  else
    {
      perror (name.c_str ());
      return false;
    }
}

bool operation (bool only_check);

void
install_check (const char *package)
{
  pkgDepCache &cache = *package_cache;
  pkgCache::PkgIterator pkg = cache.FindPkg (package);
  bool success;

  response.reset ();
  if (!pkg.end ())
    {
      cache.Init (NULL);
      cache.MarkInstall (pkg);
      success = operation (true);
    }
  else
    success = false;

  response.encode_int (success);
}

void
install_package (const char *package)
{
  pkgDepCache &cache = *package_cache;
  pkgCache::PkgIterator pkg = cache.FindPkg (package);
  bool success;

  response.reset ();
  if (!pkg.end ())
    {
      cache.Init (NULL);
      cache.MarkInstall (pkg);
      success = operation (false);
    }
  else
    success = false;

  response.encode_int (success);
}

bool
remove_package (const char *package)
{
  pkgDepCache &cache = *package_cache;
  pkgCache::PkgIterator pkg = cache.FindPkg (package);

  if (!pkg.end ())
    {
      cache.Init (NULL);
      cache.MarkDelete (pkg);
      return operation (false);
    }
  else
    return false;
}

static void
encode_prep_summary (pkgAcquire& Fetcher)
{
  for (pkgAcquire::ItemIterator I = Fetcher.ItemsBegin();
       I < Fetcher.ItemsEnd(); ++I)
    {
      if (!(*I)->IsTrusted())
	{
	  response.encode_int (preptype_notauth);
	  response.encode_string ((*I)->ShortDesc().c_str());
	}
    }
  response.encode_int (preptype_end);
}

bool operation_1 (bool only_check);

bool
operation (bool only_check)
{
  bool res = operation_1 (only_check);
  _error->DumpErrors ();
  cache_init ();
  return res;
}

bool
operation_1 (bool check_only)
{
   pkgCacheFile &Cache = *package_cache;
   SPtr<pkgPackageManager> operation_pm;

   if (_config->FindB("APT::Get::Purge",false) == true)
   {
      pkgCache::PkgIterator I = Cache->PkgBegin();
      for (; I.end() == false; I++)
      {
	 if (I.Purge() == false && Cache[I].Mode == pkgDepCache::ModeDelete)
	    Cache->MarkDelete(I,true);
      }
   }
   
   bool Fail = false;
   
   // Sanity check
   if (Cache->BrokenCount() != 0)
   {
     return _error->Error("Internal error, install_packages was called with broken packages!");
   }

   if (Cache->DelCount() == 0 && Cache->InstCount() == 0 &&
       Cache->BadCount() == 0)
      return true;
   
   // Create the text record parser
   pkgRecords Recs (Cache);
   if (_error->PendingError() == true)
      return false;
   
   // Lock the archive directory
   FileFd Lock;
   if (_config->FindB("Debug::NoLocking",false) == false)
   {
      Lock.Fd(GetLock(_config->FindDir("Dir::Cache::Archives") + "lock"));
      if (_error->PendingError() == true)
	 return _error->Error("Unable to lock the download directory");
   }
   
   // Create the download object
   myAcquireStatus Stat;
   pkgAcquire Fetcher (&Stat);

   // Read the source list
   pkgSourceList List;
   if (List.ReadMainList() == false)
      return _error->Error("The list of sources could not be read.");
   
   // Create the package manager and prepare to download
   SPtr<pkgPackageManager> Pm = _system->CreatePM(Cache);
   if (Pm->GetArchives(&Fetcher,&List,&Recs) == false || 
       _error->PendingError() == true)
      return false;

   // Display statistics
   double FetchBytes = Fetcher.FetchNeeded();
   double FetchPBytes = Fetcher.PartialPresent();
   double DebBytes = Fetcher.TotalNeeded();
   if (DebBytes != Cache->DebSize())
     {
       fprintf (stderr, "%f != %f\n", DebBytes, Cache->DebSize());
       fprintf (stderr, "How odd.. The sizes didn't match, "
		"email apt@packages.debian.org\n");
     }
   
   // Number of bytes
   if (DebBytes != FetchBytes)
     fprintf (stderr, "Need to get %sB/%sB of archives.\n",
	      SizeToStr(FetchBytes).c_str(),SizeToStr(DebBytes).c_str());
   else
     fprintf (stderr, "Need to get %sB of archives.\n",
	      SizeToStr(DebBytes).c_str());

   // Size delta
   if (Cache->UsrSize() >= 0)
     fprintf (stderr, 
	      "After unpacking %sB of additional disk space will be used.\n",
	      SizeToStr(Cache->UsrSize()).c_str());
   else
     fprintf (stderr, 
	      "After unpacking %sB disk space will be freed.\n",
	      SizeToStr(-1*Cache->UsrSize()).c_str());

   if (_error->PendingError() == true)
      return false;

   /* Check for enough free space. */
   {
     struct statvfs Buf;
     string OutputDir = _config->FindDir("Dir::Cache::Archives");
     if (statvfs(OutputDir.c_str(),&Buf) != 0)
       return _error->Errno("statvfs","Couldn't determine free space in %s",
			    OutputDir.c_str());
     if (unsigned(Buf.f_bfree) < (FetchBytes - FetchPBytes)/Buf.f_bsize)
       return _error->Error("You don't have enough free space in %s.",
			    OutputDir.c_str());
   }
   
   if (check_only)
     {
       encode_prep_summary (Fetcher);
       return true;
     }

   // Run it
   while (1)
   {
      bool Transient = false;
      if (Fetcher.Run() == pkgAcquire::Failed)
	return false;
      
      // Print out errors
      bool Failed = false;
      for (pkgAcquire::ItemIterator I = Fetcher.ItemsBegin();
	   I != Fetcher.ItemsEnd(); I++)
      {
	 if ((*I)->Status == pkgAcquire::Item::StatDone &&
	     (*I)->Complete == true)
	    continue;
	 
	 if ((*I)->Status == pkgAcquire::Item::StatIdle)
	 {
	    Transient = true;
	    // Failed = true;
	    continue;
	 }

	 fprintf (stderr, 
		  "Failed to fetch %s: %s\n",
		  (*I)->DescURI().c_str(),
		  (*I)->ErrorText.c_str());
	 Failed = true;
      }

      if (Failed == true)
	{
	  return _error->Error("Unable to fetch some archives.");
	}
      
      _system->UnLock();

      pkgPackageManager::OrderResult Res = Pm->DoInstall (status_fd);
      if (Res == pkgPackageManager::Failed || _error->PendingError() == true)
	 return false;
      if (Res == pkgPackageManager::Completed)
	 return true;
      
      // Reload the fetcher object and loop again for media swapping
      Fetcher.Shutdown();
      if (Pm->GetArchives(&Fetcher,&List,&Recs) == false)
	 return false;
      
      _system->Lock();
   }
}

bool
clean ()
{
  // Lock the archive directory
  FileFd Lock;
  if (_config->FindB("Debug::NoLocking",false) == false)
    {
      Lock.Fd(GetLock(_config->FindDir("Dir::Cache::Archives") + "lock"));
      if (_error->PendingError() == true)
	return _error->Error("Unable to lock the download directory");
    }
   
  pkgAcquire Fetcher;
  Fetcher.Clean(_config->FindDir("Dir::Cache::archives"));
  Fetcher.Clean(_config->FindDir("Dir::Cache::archives") + "partial/");
  return true;
}

// XXX - interpret status codes, call dpkg-deb only once, maybe, or
//       use the libapt record parser or whatever.

static char *
get_deb_field (const char *filename, const char *field,
	       char *buf, size_t max)
{
  char *cmd =
    g_strdup_printf ("/usr/bin/dpkg-deb -f '%s' %s", filename, field);
  fprintf (stderr, "%s\n", cmd);
  FILE *f = popen (cmd, "r");
  g_free (cmd);
  if (f)
    {
      size_t n = fread (buf, 1, max-1, f);
      if (n > 0 && buf[n-1] == '\n')
	n--;
      if (n >= 0)
	buf[n] = '\0';
      
      int status = pclose (f);
      if (status != 0)
	return NULL;
      return buf;
    }
  return NULL;
}

void
make_file_details_response (const char *filename)
{
  const size_t max = 2048;
  char buf[max];
  
  response.reset ();
  response.encode_string (get_deb_field (filename, "Package", buf, max));
  response.encode_string (get_deb_field (filename, "Version", buf, max));
  response.encode_string (get_deb_field (filename, "Maintainer", buf, max));
  response.encode_string (get_deb_field (filename, "Section", buf, max));
  response.encode_string (get_deb_field (filename, "Installed-Size", buf,
					 max));
  response.encode_string (get_deb_field (filename, "Description", buf, max));
}

bool
install_file (const char *filename)
{
  _system->UnLock();

  char *cmd = g_strdup_printf ("/usr/bin/dpkg --install '%s'", filename);
  fprintf (stderr, "%s\n", cmd);
  int res = system (cmd);
  g_free (cmd);

  if (res)
    {
      char *cmd =
	g_strdup_printf ("/usr/bin/dpkg --purge "
			 "`/usr/bin/dpkg-deb -f '%s' Package`",
			 filename);
      fprintf (stderr, "%s\n", cmd);
      system (cmd);
      g_free (cmd);
    }

  _system->Lock();

  return res == 0;
}
