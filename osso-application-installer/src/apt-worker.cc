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

#include <apt-pkg/init.h>
#include <apt-pkg/error.h>
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

   Invocation: apt-worker input-fd output-fd
 */

int input_fd, output_fd;

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
void make_package_list_response ();
void make_package_info_response (const char *package);
void make_package_details_response (const char *package,
				    const char *version,
				    int summary_kind);
bool update_package_cache ();

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

  switch (req.cmd)
    {
    case APTCMD_NOOP:
      {
	send_response_raw (req.cmd, req.seq, NULL, 0);
	break;
      }
    case APTCMD_GET_PACKAGE_LIST:
      {
	make_package_list_response ();
	send_response (&req);
	break;
      }
    case APTCMD_GET_PACKAGE_INFO:
      {
	request.reset (reqbuf, req.len);
	const char *package = request.decode_string_in_place ();
	make_package_info_response (package);
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
    default:
      {
	log_stderr ("unrecognized request: %d", req.cmd);
	send_response_raw (req.cmd, req.seq, NULL, 0);
	break;
      }
    }

  free_buf (reqbuf, stack_reqbuf);
}

int
main (int argc, char **argv)
{
  if (argc != 3)
    {
      log_stderr ("wrong invocation");
      exit (1);
    }

  input_fd = atoi (argv[1]);
  output_fd = atoi (argv[2]);

  DBG ("starting with pid %d, in %d, out %d",
       getpid (), input_fd, output_fd);

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
      package_cache->Close ();
      delete package_cache;
    }

  package_cache = new pkgCacheFile;

  if (pkgInitConfig(*_config) == false ||
      pkgInitSystem(*_config,_system) == false ||
      package_cache->Open (progress) == false)
   {
     // XXX - launch into a deaf, blind and dumb mode.
     _error->DumpErrors();
     exit (1);
   }
}

void
make_package_list_response ()
{
  pkgCache &cache = *package_cache;

  response.reset ();
  for (pkgCache::PkgIterator pkg = cache.PkgBegin();
       pkg != cache.PkgEnd();
       pkg++)
    {
      pkgCache::VerIterator installed = pkg.CurrentVer ();

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

      // skip packages that are not installed and not available
      //
      if (installed.end () && !have_latest)
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

void
make_package_info_response (const char *package)
{
  int old_broken_count;
  apt_proto_package_info info;
  char *installed_description;
  char *available_description;

  pkgDepCache &cache = *package_cache;
  pkgCache::PkgIterator pkg = cache.FindPkg (package);

  if (!pkg.end ())
    {
      pkgCache::VerIterator inst = pkg.CurrentVer ();
      if (!inst.end ())
	installed_description = get_short_description (inst);
      else
	installed_description = NULL;

      // simulate install

      cache.Init(NULL);
      
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
	available_description = get_short_description (avail);
      else
	available_description = NULL;
      
      // simulate remove

      cache.Init(NULL);
      
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
    }

  response.reset ();
  response.encode_mem (&info, sizeof (apt_proto_package_info));
  response.encode_string (installed_description);
  if (available_description && installed_description
      && !strcmp (available_description, installed_description))
    response.encode_string (NULL);
  else
    response.encode_string (available_description);
  if (installed_description)
    g_free (installed_description);
  if (available_description)
    g_free (available_description);
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

      printf ("encoding %d %s\n", type, str->str);

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
    log_stderr ("[ Some installed packages are broken! ]\n");

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
