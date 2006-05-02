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

/* This is the process that runs as root and does all the work.

   It is started from a separate program (as opposed to being forked
   directly from the GUI process) since that allows us to use sudo for
   starting it.

   This process communicates with the GUI process via some named pipes
   that are created by that process.  You can't really use it from the
   command line.

   It will output stuff to stdin and stderr, which the GUI process is
   supposed to catch and put into its log.

   The program tries hard not to exit prematurely.  Once the
   connection between the GUI process and this process has been
   established, the apt-worker is supposed to stick around until that
   connection is broken, even if it has to fail every request send to
   it.  This allows the user to try and fix the system after something
   went wrong, although the options are limited, of course.  The best
   example is a corrupted /etc/apt/sources.list: even tho you can't do
   anything related to packages, you still need the apt-worker to
   correct /etc/apt/sources.list itself in the UI.
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
#include <errno.h>

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
#include <apt-pkg/deblistparser.h>
#include <apt-pkg/debversion.h>
#include <apt-pkg/dpkgpm.h>

#include <glib/glist.h>
#include <glib/gstring.h>
#include <glib/gstrfuncs.h>
#include <glib/gmem.h>

#include "apt-worker-proto.h"

using namespace std;

/* Table of contents.
 
   COMPILE-TIME CONFIGURATION
   
   GENERAL UTILITIES

   COMMUNICATING WITH THE FRONTEND

   COMMAND DISPATCHER

*/


/** COMPILE-TIME CONFIGURATION
 */

/* Defining this to non-zero will also recognize packages in the
   "maemo" section as user packages.  There are still packages
   floating around that follow this old rule.
*/
#define ENABLE_OLD_MAEMO_SECTION_TEST 1

/* Requests up to this size are put into a stack allocated buffer.
 */
#define FIXED_REQUEST_BUF_SIZE 4096

/* You know what this means.
 */
//#define DEBUG


/** GENERAL UTILITIES
 */

/* ALLOC_BUF and FREE_BUF can be used to manage a temporary buffer of
   arbitrary size without having to allocate memory from the heap when
   the buffer is small.

   The way to use them is to allocate a buffer of 'normal' but fixed
   size statically or on the stack and the use ALLOC_BUF when the
   actual size of the needed buffer is known.  If the actual size is
   small enough, ALLOC_BUF will use the fixed size buffer, otherwise
   it will allocate a new one.  FREE_BUF will free that buffer.
*/

/* Return a pointer to LEN bytes of free storage.  When LEN is less
   than or equal to FIXED_BUF_LEN return FIXED_BUF, otherwise a newly
   allocated block of memory is returned.  ALLOC_BUF never return
   NULL.
*/
char *
alloc_buf (int len, char *fixed_buf, int fixed_buf_len)
{
  if (len <= fixed_buf_len)
    return fixed_buf;
  else
    return new char[len];
}

/* Free the block of memory pointed to by BUF if it is different from
   FIXED_BUF.
*/
void
free_buf (char *buf, char *fixed_buf)
{
  if (buf != fixed_buf)
    delete[] buf;
}

/* Open FILENAME with FLAGS, or die.
 */
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

/* MAYBE_READ_BYTE reads a byte from FD if one is available without
   blocking.  If there is one, it is returned (as an unsigned number).
   Otherwise, -1 is returned.
*/
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

/* DRAIN_FD reads all bytes from FD that are available without
   blocking.
*/
static void
drain_fd (int fd)
{
  while (maybe_read_byte (fd) >= 0)
    ;
}

/** COMMUNICATING WITH THE FRONTEND.
 
   The communication with the frontend happens over four
   unidirectional fifos: requests are read from INPUT_FD and
   responses are sent back via OUTPUT_FD.  No new requests is read
   until the response to the current one has been completely sent.

   The data read from INPUT_FD must follow the request format
   specified in <apt-worker-proto.h>.  The data written to OUTPUT_FD
   follows the response format specified there.

   The CANCEL_FD is polled periodically and when something is
   available to be read, the current operation is aborted.  There is
   currently no meaning defined for the actual bytes that are sent,
   the mere arrival of a byte triggers the abort.

   When using the apt-pkg PackageManager, it is configured in such a
   way that it sents it "pmstatus:" message lines to STATUS_FD.
   Other asynchronous status reports are sent as spontaneous
   APTCMD_STATUS responses via OUTPUT_FD.  'Spontaneous' should mean
   that no request is required to receive APTCMD_STATUS responses.
   In fact, APTCMD_STATUS requests are treated as an error by the
   apt-worker.

   Logging and debug output, and output from dpkg and the maintainer
   scripts appears normally on stdout and stderr of the apt-worker
   process.
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

/* MUST_READ and MUST_WRITE read and write blocks of raw bytes from
   INPUT_FD and to OUTPUT_FD.  If they return, they have succeeded and
   read or written the whole block.
*/

void
must_read (void *buf, size_t n)
{
  int r;

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

/* This function sends a response on OUTPUT_FD with the given CMD and
   SEQ.  It either succeeds or does not return.
*/
void
send_response_raw (int cmd, int seq, void *response, size_t len)
{
  apt_response_header res = { cmd, seq, len };
  must_write (&res, sizeof (res));
  must_write (response, len);
}

/* Fabricate and send a APTCMD_STATUS response.  Parameters OP,
   ALREADY, and TOTAL are as specified in apt-worker-proto.h.

   A status response is only sent when there is enough change since
   the last time.  The following counts as 'enough': ALREADY has
   decreased, it has increased by more than MIN_CHANGE, it is equal to
   -1, LAST_TOTAL has changed, or OP has changed.
*/
void
send_status (int op, int already, int total, int min_change)
{

  static apt_proto_encoder status_response;
  static int last_op;
  static int last_already;
  static int last_total;

  if (already == -1 
      || already < last_already 
      || already >= last_already + min_change
      || total != last_total
      || op != last_op)
    {
      last_already = already;
      last_total = total;
      last_op = op;
      
      status_response.reset ();
      status_response.encode_int (op);
      status_response.encode_int (already);
      status_response.encode_int (total);
      send_response_raw (APTCMD_STATUS, -1, 
			 status_response.get_buf (),
			 status_response.get_len ());
    }
}


/** STARTUP AND COMMAND DISPATCHER.
 */

/* Since the apt-worker only works on a single command at a time, we
   use two global encoder and decoder engines that manage the
   parameters of the request and the result values of the response.

   Handlers of specific commands will read the parameters from REQUEST
   and put the results into RESPONSE.  The command dispatcher will
   prepare REQUEST before calling the command handler and ship out
   RESPONSE after it returned.
*/
apt_proto_decoder request;
apt_proto_encoder response;

void cmd_get_package_list ();
void cmd_get_package_info ();
void cmd_get_package_details ();
void cmd_update_package_cache ();
void cmd_get_sources_list ();
void cmd_set_sources_list ();
void cmd_install_check ();
void cmd_install_package ();
void cmd_get_packages_to_remove ();
void cmd_remove_package ();
void cmd_clean ();
void cmd_get_file_details ();
void cmd_install_file ();

static bool init_cache_after_request;

void cache_init ();

void
need_cache_init ()
{
  init_cache_after_request = true;
}

void
handle_request ()
{
  apt_request_header req;
  char stack_reqbuf[FIXED_REQUEST_BUF_SIZE];
  char *reqbuf;

  must_read (&req, sizeof (req));
  DBG ("got req %d/%d/%d", req.cmd, req.seq, req.len);

  reqbuf = alloc_buf (req.len, stack_reqbuf, FIXED_REQUEST_BUF_SIZE);
  must_read (reqbuf, req.len);

  drain_fd (cancel_fd);

  request.reset (reqbuf, req.len);
  response.reset ();

  init_cache_after_request = false;

  switch (req.cmd)
    {

    case APTCMD_NOOP:
      // Nothing to do.
      break;

    case APTCMD_GET_PACKAGE_LIST:
      cmd_get_package_list ();
      break;

    case APTCMD_GET_PACKAGE_INFO:
      cmd_get_package_info ();
      break;

    case APTCMD_GET_PACKAGE_DETAILS:
      cmd_get_package_details ();
      break;

    case APTCMD_UPDATE_PACKAGE_CACHE:
      cmd_update_package_cache ();
      break;

    case APTCMD_GET_SOURCES_LIST:
      cmd_get_sources_list ();
      break;

    case APTCMD_SET_SOURCES_LIST:
      cmd_set_sources_list ();
      break;

    case APTCMD_INSTALL_CHECK:
      cmd_install_check ();
      break;

    case APTCMD_INSTALL_PACKAGE:
      cmd_install_package ();
      break;

    case APTCMD_GET_PACKAGES_TO_REMOVE:
      cmd_get_packages_to_remove ();
      break;

    case APTCMD_REMOVE_PACKAGE:
      cmd_remove_package ();
      break;

    case APTCMD_CLEAN:
      cmd_clean ();
      break;

    case APTCMD_GET_FILE_DETAILS:
      cmd_get_file_details ();
      break;

    case APTCMD_INSTALL_FILE:
      cmd_install_file ();
      break;

    default:
      log_stderr ("unrecognized request: %d", req.cmd);
      break;
    }

  _error->DumpErrors ();

  send_response_raw (req.cmd, req.seq,
		     response.get_buf (), response.get_len ());

  free_buf (reqbuf, stack_reqbuf);

  if (init_cache_after_request)
    cache_init ();
}

void read_certified_conf ();

int
main (int argc, char **argv)
{
  if (argc != 5)
    {
      log_stderr ("wrong invocation");
      exit (1);
    }

  DBG ("starting up");

  /* The order here is important to avoid deadlocks with the frontend.
   */
  input_fd = must_open (argv[1], O_RDONLY);
  output_fd = must_open (argv[2], O_WRONLY);
  status_fd = must_open (argv[3], O_WRONLY);
  cancel_fd = must_open (argv[4], O_RDONLY);

  DBG ("starting with pid %d, in %d, out %d, stat %d, cancel %d",
       getpid (), input_fd, output_fd, status_fd, cancel_fd);

  /* Don't let our heavy lifting starve the UI.
   */
  errno = 0;
  if (nice (20) == -1 && errno != 0)
    log_stderr ("nice: %m");

  cache_init ();
  read_certified_conf ();

  while (true)
    handle_request ();
}

/** COMMAND HANDLERS
 */

/* We only report real progress information when reconstructing the
   cache and during downloads.  Only downloads can be cancelled.

   The following two classes allow us to hook into libapt-pkgs
   progress reporting mechanism.  Instances of UPDATE_PROGESS are used
   for cache related activities, and instances of DOWNLOAD_STATUS are
   used when 'acquiring' things.
*/

class UpdateProgress : public OpProgress
{
  virtual void
  Update ()
  {
    send_status (op_updating_cache, (int)Percent, 100, 5);
  }
};

class DownloadStatus : public pkgAcquireStatus
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

    send_status (op_downloading, (int)CurrentBytes, (int)TotalBytes, 1000);

    if (maybe_read_byte (cancel_fd) >= 0)
      return false;

    return true;
  }
};

bool
is_user_section (const char *section, const char *end)
{
  if (section == NULL)
    return false;

#if ENABLE_OLD_MAEMO_SECTION_TEST
  if (end-section > 6 && !strncmp (section, "maemo/", 6))
    return true;
#endif
  
  return end-section > 6 && !strncmp (section, "user/", 5);
}

bool
is_user_package (const pkgCache::VerIterator &ver)
{
  const char *section = ver.Section ();

  if (section == NULL)
    return false;

  return is_user_section (section, section + strlen (section));
}

/* We keep a global pointer to a pkgCacheFile instance that is used by
   most of the command handlers.

   PACKAGE_CACHE might be NULL when CACHE_INIT failed to create it for
   some reason.  Every command handler must deal with this.

   XXX - there is some voodoo coding here since I don't yet have the
         full overview about the differences between a pkgCacheFile, a
         pkgCache, a pkgDebCache, etc.
*/
pkgCacheFile *package_cache = NULL;
char *auto_flags = NULL;
unsigned int package_count;

/* Save and restore the 'Auto' flags of the cache.  Libapt-pkg does
   not seem to save it so we do it.
*/

void
save_auto_flags ()
{
  // XXX - log errors.

  if (package_cache == NULL)
    return;

  if (mkdir ("/var/lib/osso-application-installer", 0777) < 0 && errno != EEXIST)
    {
      log_stderr ("/var/lib/osso-application-installer: %m");
      return;
    }

  memset (auto_flags, 0, package_count);

  FILE *f = fopen ("/var/lib/osso-application-installer/autoinst", "w");
  if (f)
    {
      pkgDepCache &cache = *package_cache;

      for (pkgCache::PkgIterator pkg = cache.PkgBegin(); !pkg.end (); pkg++)
	if (cache[pkg].Flags & pkgCache::Flag::Auto)
	  {
	    auto_flags[pkg->ID] = true;
	    fprintf (f, "%s\n", pkg.Name ());
	  }
      fclose (f);
    }
}

void
restore_auto_flags ()
{
  if (package_cache == NULL)
    return;

  pkgDepCache &cache = *package_cache;
  
  for (pkgCache::PkgIterator pkg = cache.PkgBegin(); !pkg.end (); pkg++)
    {
      if (auto_flags[pkg->ID])
	cache[pkg].Flags |= pkgCache::Flag::Auto;
      else
	cache[pkg].Flags &= ~pkgCache::Flag::Auto;
    }
}

void
load_auto_flags ()
{
  // XXX - log errors.

  if (package_cache == NULL)
    return;

  memset (auto_flags, 0, package_count);

  FILE *f = fopen ("/var/lib/osso-application-installer/autoinst", "r");
  if (f)
    {
      pkgDepCache &cache = *package_cache;

      char *line = NULL;
      size_t len = 0;
      ssize_t n;

      while ((n = getline (&line, &len, f)) != -1)
	{
	  if (n > 0 && line[n-1] == '\n')
	    line[n-1] = '\0';

	  pkgCache::PkgIterator pkg = cache.FindPkg (line);
	  if (!pkg.end ())
	    {
	      DBG ("auto: %s", pkg.Name ());
	      auto_flags[pkg->ID] = true;
	    }
	}

      free (line);
      fclose (f);

      restore_auto_flags ();
    }
}

/* Initialize libapt-pkg if this has not been already and (re-)create
   PACKAGE_CACHE.  If the cache can not be created, PACKAGE_CACHE is
   set to NULL and an appropriate message is output.
*/
void
cache_init ()
{
  static bool global_initialized = false;

  if (package_cache)
    {
      DBG ("closing");
      package_cache->Close ();
      delete package_cache;
      package_cache = NULL;
      DBG ("done");
    }

  if (!global_initialized)
    {
      if (pkgInitConfig(*_config) == false ||
	  pkgInitSystem(*_config,_system) == false)
	{
	  _error->DumpErrors ();
	  return;
	}
      global_initialized = true;
    }

  /* We need to dump the errors here since any pending errors will
     cause the following operations to fail.
  */
  _error->DumpErrors ();
	  
  UpdateProgress progress;
  package_cache = new pkgCacheFile;

  DBG ("init.");
  if (!package_cache->Open (progress))
    {
      DBG ("failed.");
      _error->DumpErrors();
      delete package_cache;
      package_cache = NULL;
    }

  if (auto_flags)
    {
      delete[] auto_flags;
      auto_flags = NULL;
    }

  if (package_cache)
    {
      pkgDepCache &cache = *package_cache;
      package_count = cache.Head().PackageCount;
      auto_flags = new char[package_count];
      memset (auto_flags, 0, package_count);
    }

  load_auto_flags ();
}

/* Determine whether a package was installed automatically to satisfy
   a dependency.
*/
bool
is_auto_package (pkgCache::PkgIterator &pkg)
{
  pkgDepCache &cache = *package_cache;
  
  return (cache[pkg].Flags & pkgCache::Flag::Auto) != 0;
}

/* Revert the cache to its initial state.  More concretely, all
   packages are marked as 'keep'.

   The effect should be the same as calling pkgDebCache::Init, but
   much faster.
*/
void
cache_reset_package (pkgCache::PkgIterator &pkg)
{
  pkgDepCache &cache = *package_cache;

  cache.MarkKeep (pkg);
  if (auto_flags[pkg->ID])
    cache[pkg].Flags |= pkgCache::Flag::Auto;
  else
    cache[pkg].Flags &= ~pkgCache::Flag::Auto;
}

void
cache_reset ()
{
  if (package_cache == NULL)
    return;

  pkgDepCache &cache = *package_cache;

  for (pkgCache::PkgIterator pkg = cache.PkgBegin(); !pkg.end (); pkg++)
    cache_reset_package (pkg);
}

/* Mark a package for installation, using a 'no-surprises' approach
   suitable for the Application installer.

   Concretely, installing a package will never automatically remove
   other packages.  Thus, we undo the removals scheduled by
   MarkInstall.  Doing this will break the original package, but that
   is what we want.
*/
static void
mark_for_install (pkgCache::PkgIterator &pkg)
{
  pkgDepCache &cache = *package_cache;

  cache.MarkInstall (pkg);
  for (pkgCache::PkgIterator p = cache.PkgBegin (); !p.end (); p++)
    {
      if (cache[p].Delete ())
	cache_reset_package (p);
    }
}


/* Mark a package for removal and also remove as many of the packages
   that it depends on as possible.
*/
static void
mark_for_remove (pkgCache::PkgIterator &pkg, bool only_maybe = false)
{
  pkgDepCache &cache = *package_cache;

  int n_broken = cache.BrokenCount ();

  cache.MarkDelete (pkg);
  cache[pkg].Flags &= ~pkgCache::Flag::Auto;

  if (!cache[pkg].Delete () || cache.BrokenCount () > n_broken)
    {
      if (only_maybe)
	cache_reset_package (pkg);
      return;
    }

  // Now try to remove all non-user, auto-installed dependencies of
  // this package.

  pkgCache::VerIterator cur = pkg.CurrentVer ();
  if (cur.end ())
    return;

  for (pkgCache::DepIterator dep = cur.DependsList(); dep.end() == false;
       dep++)
    {
      if (dep->Type == pkgCache::Dep::PreDepends ||
	  dep->Type == pkgCache::Dep::Depends)
	{
	  pkgCache::PkgIterator p = dep.TargetPkg ();
	  if (!p.end ()
	      && is_auto_package (p)
	      && !p.CurrentVer().end()
	      && !is_user_package (p.CurrentVer()))
	    mark_for_remove (p, true);
	}
    }
}

/* APTCMD_GET_PACKAGE_LIST 

   The get_package_list command can do some filtering and we have a
   few utility functions for implementing the necessary checks.  The
   check generally take cache iterators to identify a package or a
   version.
 */

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

  // XXX - UTF8?
  return strcasestr (desc, pattern);
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
  pkgRecords Recs (*package_cache);
  pkgRecords::Parser &P = Recs.Lookup (ver.FileList ());

  const char *start, *stop;
  P.GetRec (start, stop);

  /* NOTE: pkTagSection::Scan only succeeds when the record ends in
           two newlines, but pkgRecords::Parser::GetRec does not
           include the second newline in its returned region.
           However, that second newline is always there, so we just
           pass one more character to Scan.
  */

  pkgTagSection section;
  if (!section.Scan (start, stop-start+1))
    return NULL;

  char *icon = g_strdup (section.FindS ("Maemo-Icon-26").c_str());
  if (all_white_space (icon))
    {
      g_free (icon);
      return NULL;
    }
  else
    return icon;
}

static void
encode_version_info (pkgCache::VerIterator &ver, bool include_size)
{
  char *desc, *icon;

  response.encode_string (ver.VerStr ());
  if (include_size)
    response.encode_int (ver->InstalledSize);
  response.encode_string (ver.Section ());
  desc = get_short_description (ver);
  response.encode_string (desc);
  g_free (desc);
  icon = get_icon (ver);
  response.encode_string (icon);
  g_free (icon);
}

static void
encode_empty_version_info (bool include_size)
{
  response.encode_string (NULL);
  if (include_size)
    response.encode_int (0);
  response.encode_string (NULL);
  response.encode_string (NULL);
  response.encode_string (NULL);
}

void
cmd_get_package_list ()
{
  bool only_user = request.decode_int ();
  bool only_installed = request.decode_int ();
  bool only_available = request.decode_int ();
  const char *pattern = request.decode_string_in_place ();

  if (package_cache == NULL)
    {
      response.encode_int (0);
      return;
    }

  response.encode_int (1);
  pkgDepCache &cache = *package_cache;

  for (pkgCache::PkgIterator pkg = cache.PkgBegin(); !pkg.end (); pkg++)
    {
      pkgCache::VerIterator installed = pkg.CurrentVer ();

      // skip non user packages if requested
      //
      if (only_user
	  && !installed.end ()
	  && !is_user_package (installed))
	continue;

      // skip not-installed packages if requested
      //
      if (only_installed
	  && installed.end ())
	continue;

      pkgCache::VerIterator candidate = cache.GetCandidateVer (pkg);

      // skip non user packages if requested
      //
      if (only_user
	  && !candidate.end ()
	  && !is_user_package (candidate))
	continue;

      // skip non-available packages if requested
      //
      if (only_available
	  && candidate.end ())
	continue;

      // skip packages that are not installed and not available
      //
      if (installed.end () && candidate.end ())
	continue;

      // skip packages that don't match the pattern if requested
      //
      if (pattern
	  && !(name_matches_pattern (pkg, pattern)
	       || (!installed.end ()
		   && description_matches_pattern (installed, pattern))
	       || (!candidate.end ()
		   && description_matches_pattern (candidate, pattern))))
	continue;

      // Name
      response.encode_string (pkg.Name ());

      // Broken.
      //
      // This check doesn't catch all kinds of broken packages, only
      // those that failed to unpack or configure, but not, for
      // example, those that have been forcfully installed despite
      // missing dependencies.  This is probably OK for now since only
      // the former kinds of brokenness should be producable with the
      // Application installer anyway.
      //
      response.encode_int (pkg.State ()
			   != pkgCache::PkgIterator::NeedsNothing);

      // Installed version
      if (!installed.end())
	encode_version_info (installed, true);
      else
	encode_empty_version_info (true);

      // Available version and section
      // XXX - avoid duplicating information
      if (!candidate.end () && installed.CompareVer (candidate) < 0)
	encode_version_info (candidate, false);
      else
	encode_empty_version_info (false);
    }
}

/* APTCMD_GET_PACKAGE_INFO

   This command performs a simulated install and removal of the
   specified package to gather the requested information.
 */

static int
installable_status_1 (pkgCache::PkgIterator &pkg,
		      pkgCache::PkgIterator &want)
{
  pkgDepCache &Cache = *package_cache;
  pkgCache::VerIterator Ver = Cache[pkg].InstVerIter(Cache);

  bool some_missing = false, some_conflicting = false;

  if (Ver.end() == true)
    return status_unable;
      
  for (pkgCache::DepIterator D = Ver.DependsList(); D.end() == false;)
    {
      // Compute a single dependency element (glob or)
      pkgCache::DepIterator Start;
      pkgCache::DepIterator End;
      D.GlobOr(Start,End); // advances D

      if ((Cache[End] & pkgDepCache::DepGInstall)
	  == pkgDepCache::DepGInstall)
	continue;

      if (Start->Type == pkgCache::Dep::PreDepends ||
	  Start->Type == pkgCache::Dep::Depends)
	some_missing = true;
      else if (Start->Type == pkgCache::Dep::Conflicts)
	some_conflicting = true;
    }

  if (some_missing)
    return status_missing;
  if (some_conflicting)
    return status_conflicting;

  return status_unable;
}

static int
combine_status (int s1, int s2)
{
  return max (s1, s2);
}

static int
installable_status (pkgCache::PkgIterator &want)
{
  pkgDepCache &cache = *package_cache;
  int installable_status = status_unable;
  
  for (pkgCache::PkgIterator pkg = cache.PkgBegin();
       pkg.end() != true;
       pkg++)
    {
      if (cache[pkg].InstBroken())
	installable_status =
	  combine_status (installable_status_1 (pkg, want), 
			  installable_status);
    }

  return installable_status;
}

static int
removable_status (pkgCache::PkgIterator &want)
{
  pkgDepCache &cache = *package_cache;
  for (pkgCache::PkgIterator pkg = cache.PkgBegin();
       pkg.end() != true;
       pkg++)
    {
      if (cache[pkg].InstBroken())
	return status_needed;
    }

  return status_unable;
}

void
cmd_get_package_info ()
{
  const char *package = request.decode_string_in_place ();
  bool only_installable_info = request.decode_int ();

  apt_proto_package_info info;

  info.installable_status = status_unknown;
  info.download_size = 0;
  info.install_user_size_delta = 0;
  info.removable_status = status_unknown;
  info.remove_user_size_delta = 0;

  if (package_cache)
    {
      pkgDepCache &cache = *package_cache;
      pkgCache::PkgIterator pkg = cache.FindPkg (package);

      if (!pkg.end ())
	{
	  pkgCache::VerIterator inst = pkg.CurrentVer ();
	  int old_broken_count;

	  // simulate install
	  
	  old_broken_count = cache.BrokenCount();
	  mark_for_install (pkg);
	  if (cache.BrokenCount() > old_broken_count)
	    info.installable_status = installable_status (pkg);
	  else
	    info.installable_status = status_able;
	  info.download_size = (int) cache.DebSize ();
	  info.install_user_size_delta = (int) cache.UsrSize ();
	  cache_reset ();

	  if (!only_installable_info)
	    {
	      // simulate remove
	  
	      old_broken_count = cache.BrokenCount();
	      mark_for_remove (pkg);
	      if (cache.BrokenCount() > old_broken_count)
		info.removable_status = removable_status (pkg);
	      else
		info.removable_status = status_able;
	      info.remove_user_size_delta = (int) cache.UsrSize ();
	      cache_reset ();
	    }
	}
    }

  response.encode_mem (&info, sizeof (apt_proto_package_info));
}

/* APTCMD_GET_PACKAGE_DETAILS

   Like APTCMD_GET_PACKAGE_INFO, this command performs a simulated
   install or removal (as requested), but it gathers a lot more
   information about the package and what is happening.
*/

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
	      g_string_append_printf (str, "%s", target.Name());
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

  // XXX - the summary is not really correct when there are broken
  //       packages in the device.  The problems of those packages
  //       might be included in the report.

  if (cache.BrokenCount() > 0)
    fprintf (stderr, "[ Some installed packages are broken! ]\n");

  mark_for_install (want);

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

  cache_reset ();
}

void
encode_remove_summary (pkgCache::PkgIterator &want)
{
  pkgDepCache &cache = *package_cache;

  if (cache.BrokenCount() > 0)
    log_stderr ("[ Some installed packages are broken! ]\n");

  mark_for_remove (want);

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

  cache_reset ();
}

bool
find_package_version (pkgCacheFile *cache_file,
		      pkgCache::PkgIterator &pkg,
		      pkgCache::VerIterator &ver,
		      const char *package, const char *version)
{
  if (cache_file == NULL || package == NULL || version == NULL)
    return false;

  pkgDepCache &cache = *cache_file;
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
cmd_get_package_details ()
{
  const char *package = request.decode_string_in_place ();
  const char *version = request.decode_string_in_place ();
  int summary_kind = request.decode_int ();

  pkgCache::PkgIterator pkg;
  pkgCache::VerIterator ver;

  if (find_package_version (package_cache, pkg, ver, package, version))
    {
      pkgDepCache &cache = *package_cache;
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

/* APTCMD_UPDATE_PACKAGE_CACHE

   This is copied straight from "apt-get update".
*/

int
update_package_cache ()
{
  // Get the source list
  pkgSourceList List;
  if (List.ReadMainList () == false)
    return rescode_failure;

  // Lock the list directory
  FileFd Lock;
  if (_config->FindB("Debug::NoLocking",false) == false)
    {
      Lock.Fd (GetLock (_config->FindDir("Dir::State::Lists") + "lock"));
      if (_error->PendingError () == true)
	{
	  _error->Error ("Unable to lock the list directory");
	  return rescode_failure;
	}
    }
   
  // Create the download object
  DownloadStatus Stat;
  pkgAcquire Fetcher(&Stat);

  // Populate it with the source selection
  if (List.GetIndexes(&Fetcher) == false)
    return rescode_failure;
   
  // Run it
  if (Fetcher.Run() == pkgAcquire::Failed)
    return rescode_failure;

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
      Fetcher.Clean (_config->FindDir("Dir::State::lists"));
      Fetcher.Clean (_config->FindDir("Dir::State::lists") + "partial/");
    }
  
  // Prepare the cache.   
  cache_init ();

  return Failed? rescode_failure : rescode_success;
}

void
cmd_update_package_cache ()
{
  const char *http_proxy = request.decode_string_in_place ();

  if (http_proxy)
    {
      setenv ("http_proxy", http_proxy, 1);
      DBG ("http_proxy: %s", http_proxy);
    }
  
  int result_code = update_package_cache ();

  response.encode_int (result_code);
}

/* APTCMD_GET_SOURCES_LIST
*/

void
cmd_get_sources_list ()
{
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

void
cmd_set_sources_list ()
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
      response.encode_int (1);
    }
  else
    {
      perror (name.c_str ());
      response.encode_int (0);
    }
}

int operation (bool only_check);

void
cmd_install_check ()
{
  const char *package = request.decode_string_in_place ();
  int result_code = rescode_failure;

  if (package_cache)
    {
      pkgDepCache &cache = *package_cache;
      pkgCache::PkgIterator pkg = cache.FindPkg (package);

      if (!pkg.end ())
	{
	  mark_for_install (pkg);
	  result_code = operation (true);
	  cache_reset ();
	}
    }

  response.encode_int (result_code == rescode_success);
}

void
cmd_install_package ()
{
  const char *package = request.decode_string_in_place ();
  const char *http_proxy = request.decode_string_in_place ();
  int result_code = rescode_failure;

  if (http_proxy)
    {
      setenv ("http_proxy", http_proxy, 1);
      DBG ("http_proxy: %s", http_proxy);
    }

  if (package_cache)
    {
      pkgDepCache &cache = *package_cache;
      pkgCache::PkgIterator pkg = cache.FindPkg (package);

      if (!pkg.end ())
	{
	  mark_for_install (pkg);
	  result_code = operation (false);
	}
    }

  need_cache_init ();
  response.encode_int (result_code);
}

void
cmd_get_packages_to_remove ()
{
  const char *package = request.decode_string_in_place ();

  if (package_cache)
    {
      pkgDepCache &cache = *package_cache;
      pkgCache::PkgIterator pkg = cache.FindPkg (package);

      if (!pkg.end ())
	{
	  mark_for_remove (pkg);
	  
	  for (pkgCache::PkgIterator pkg = cache.PkgBegin();
	       pkg.end() != true;
	       pkg++)
	    {
	      if (cache[pkg].Delete())
		response.encode_string (pkg.Name());
	    }

	  cache_reset ();
	}
    }

  response.encode_string (NULL);
}

void
cmd_remove_package ()
{
  const char *package = request.decode_string_in_place ();
  int result_code = rescode_failure;

  if (package_cache)
    {
      pkgDepCache &cache = *package_cache;
      pkgCache::PkgIterator pkg = cache.FindPkg (package);

      if (!pkg.end ())
	{
	  mark_for_remove (pkg);
	  result_code = operation (false);
	}
    }

  need_cache_init ();
  response.encode_int (result_code == rescode_success);
}

static GList *certified_uri_prefixes = NULL;

void
read_certified_conf ()
{
  const char *name = "/etc/osso-application-installer/certified.list";

  FILE *f = fopen (name, "r");

  if (f)
    {
      char *line = NULL;
      size_t len = 0;
      ssize_t n;

      while ((n = getline (&line, &len, f)) != -1)
	{
	  if (n > 0 && line[n-1] == '\n')
	    line[n-1] = '\0';

	  char *hash = strchr (line, '#');
	  if (hash)
	    *hash = '\0';

	  char *saveptr;
	  char *type = strtok_r (line, " \t", &saveptr);
	  if (type)
	    {
	      if (!strcmp (type, "uri-prefix"))
		{
		  char *prefix = strtok_r (NULL, " \t", &saveptr);
		  DBG ("certified: %s", prefix);
		  certified_uri_prefixes =
		    g_list_append (certified_uri_prefixes,
				   g_strdup (prefix));
		}
	      else
		fprintf (stderr, "unsupported type in certified.list: %s\n",
			 type);
	    }
	}

      free (line);
      fclose (f);
    }
  else if (errno != ENOENT)
    perror (name);
}

static bool
is_certified_source (string uri)
{
  for (GList *p = certified_uri_prefixes; p; p = p->next)
    {
      if (g_str_has_prefix (uri.c_str(), (char *)p->data))
	return true;
    }
  return false;
}

static void
encode_prep_summary (pkgAcquire& Fetcher)
{
  for (pkgAcquire::ItemIterator I = Fetcher.ItemsBegin();
       I < Fetcher.ItemsEnd(); ++I)
    {
      if (!is_certified_source ((*I)->DescURI ()))
	{
	  cerr << "notcert: " << (*I)->DescURI () << "\n";
	  response.encode_int (preptype_notcert);
	  response.encode_string ((*I)->ShortDesc().c_str());
	}
      
      if (!(*I)->IsTrusted())
	{
	  cerr << "notauth: " << (*I)->DescURI () << "\n";
	  response.encode_int (preptype_notauth);
	  response.encode_string ((*I)->ShortDesc().c_str());
	}
    }
  response.encode_int (preptype_end);
}

static void
encode_upgrades ()
{
  if (package_cache)
    {
      pkgDepCache &cache = *package_cache;

      for (pkgCache::PkgIterator pkg = cache.PkgBegin();
	   pkg.end() != true;
	   pkg++)
	{
	  if (cache[pkg].Upgrade() && !cache[pkg].NewInstall())
	    {
	      response.encode_string (pkg.Name());
	      response.encode_string (cache[pkg].CandVersion);
	    }
	}
    }

  response.encode_string (NULL);
}

/* We modify the pkgDPkgPM package manager slightly so that it never
   attempts to configure packages that are not otherwise operated on
   during a package management run.  We do this to so that we only get
   failure reports for the actual packages being worked on, and not
   for ones that failed to configure in an earlier run.

   XXX - there might be a way to get the same effect by cleverly
         manipulating the pkgDepCache, but I would have to look
         harder...
*/

class myDPkgPM : public pkgDPkgPM
{
  virtual bool Configure(PkgIterator Pkg);
  
public:

  myDPkgPM(pkgDepCache *Cache);
};

bool
myDPkgPM::Configure (PkgIterator Pkg)
{
  for (int i = 0; i < List.size (); i++)
    if (List[i].Pkg == Pkg)
      return pkgDPkgPM::Configure (Pkg);
  log_stderr ("not configuring unrelated package %s", Pkg.Name ());
  return true;
}

myDPkgPM::myDPkgPM (pkgDepCache *Cache)
  : pkgDPkgPM (Cache)
{
}

int
operation (bool check_only)
{
   pkgCacheFile &Cache = *package_cache;

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
   
   if (Cache->DelCount() == 0 && Cache->InstCount() == 0 &&
       Cache->BadCount() == 0)
      return rescode_success;
   
   // Create the text record parser
   pkgRecords Recs (Cache);
   if (_error->PendingError() == true)
      return rescode_failure;
   
   // Lock the archive directory
   FileFd Lock;
   if (_config->FindB("Debug::NoLocking",false) == false)
   {
      Lock.Fd(GetLock(_config->FindDir("Dir::Cache::Archives") + "lock"));
      if (_error->PendingError() == true)
	{
	  _error->Error("Unable to lock the download directory");
	  return rescode_failure;
	}
   }
   
   // Create the download object
   DownloadStatus Stat;
   pkgAcquire Fetcher (&Stat);

   // Read the source list
   pkgSourceList List;
   if (List.ReadMainList() == false)
     {
       _error->Error("The list of sources could not be read.");
       return rescode_failure;
     }
   
   // Create the package manager and prepare to download
   SPtr<pkgPackageManager> Pm = new myDPkgPM (Cache);
   if (Pm->GetArchives(&Fetcher,&List,&Recs) == false || 
       _error->PendingError() == true)
     return rescode_failure;

   double FetchBytes = Fetcher.FetchNeeded();
   double FetchPBytes = Fetcher.PartialPresent();
   double DebBytes = Fetcher.TotalNeeded();

   if (_error->PendingError() == true)
      return rescode_failure;

   if (check_only)
     {
       encode_prep_summary (Fetcher);
       encode_upgrades ();
       return rescode_success;
     }

   /* Check for enough free space. */
   {
     struct statvfs Buf;
     string OutputDir = _config->FindDir("Dir::Cache::Archives");
     if (statvfs(OutputDir.c_str(),&Buf) != 0)
       {
	 _error->Errno("statvfs","Couldn't determine free space in %s",
		       OutputDir.c_str());
	 return rescode_failure;
       }

     if (unsigned(Buf.f_bfree) < (FetchBytes - FetchPBytes)/Buf.f_bsize)
       {
	 _error->Error("You don't have enough free space in %s.",
		       OutputDir.c_str());
	 return rescode_out_of_space;
       }
   }
   
   if (Fetcher.Run() == pkgAcquire::Failed)
     return rescode_failure;
      
   // Print out errors
   bool Failed = false, not_found = true;
   for (pkgAcquire::ItemIterator I = Fetcher.ItemsBegin();
	I != Fetcher.ItemsEnd(); I++)
     {
       if ((*I)->Status == pkgAcquire::Item::StatDone &&
	   (*I)->Complete == true)
	 continue;
       
       if ((*I)->Status == pkgAcquire::Item::StatIdle)
	 continue;

       fprintf (stderr, 
		"Failed to fetch %s: %s\n",
		(*I)->DescURI().c_str(),
		(*I)->ErrorText.c_str());

       /* The http method puts the result code of the request at the
	  start of the error text.  We rely on this to detect the
	  "Package not found" error.
       */

       int result = atoi ((*I)->ErrorText.c_str());

       /* We report a "Package not found" error when all downloads
	  failed with result code 404.
       */
       if (result != 404)
	 not_found = false;

       Failed = true;
     }

   if (Failed == true)
     {
       _error->Error("Unable to fetch some archives.");
       return not_found? rescode_packages_not_found : rescode_download_failed;
     }
      
   send_status (op_general, -1, 0, 0);
      
   _system->UnLock();
   pkgPackageManager::OrderResult Res = Pm->DoInstall (status_fd);
   _system->Lock();

   save_auto_flags ();

   if (Res == pkgPackageManager::Failed || _error->PendingError() == true)
     return rescode_failure;

   if (Res == pkgPackageManager::Completed)
     return rescode_success;
     
   return rescode_failure;
}

void
cmd_clean ()
{
  bool success = true;

  // Lock the archive directory
  FileFd Lock;
  if (_config->FindB("Debug::NoLocking",false) == false)
    {
      Lock.Fd(GetLock(_config->FindDir("Dir::Cache::Archives") + "lock"));
      if (_error->PendingError() == true)
	success = _error->Error("Unable to lock the download directory");
    }
   
  if (success)
    {
      pkgAcquire Fetcher;
      Fetcher.Clean(_config->FindDir("Dir::Cache::archives"));
      Fetcher.Clean(_config->FindDir("Dir::Cache::archives") + "partial/");
    }

  response.encode_int (success);
}

// XXX - interpret status codes

static char *
get_deb_record (const char *filename)
{
  char *cmd =
    g_strdup_printf ("/usr/bin/dpkg-deb -f '%s'", filename);
  fprintf (stderr, "%s\n", cmd);
  FILE *f = popen (cmd, "r");
  g_free (cmd);
  if (f)
    {
      const size_t incr = 2000;
      char *record = NULL;
      size_t size = 0;

      do
	{
	  // increase buffer and try to fill it, leaving room for the
	  // trailing newlines and nul.
	  // XXX - do it properly.
	  
	  char *new_record = new char[size+incr+3];
	  if (record)
	    {
	      memcpy (new_record, record, size);
	      delete record;
	    }
	  record = new_record;
	  
	  size += fread (record+size, 1, incr, f);
	}
      while (!feof (f));

      int status = pclose (f);
      if (status != 0)
	{
	  delete (record);
	  return NULL;
	}

      record[size] = '\n';
      record[size+1] = '\n';
      record[size+2] = '\0';
      return record;
    }
  return NULL;
}

static bool
check_dependency (string &package, string &version, unsigned int op)
{
  if (package_cache == NULL)
    return false;

  pkgDepCache &cache = (*package_cache);
  pkgCache::PkgIterator pkg;
  pkgCache::VerIterator installed;

  pkg = cache.FindPkg (package);
  if (pkg.end ())
    return false;

  installed = pkg.CurrentVer ();
  if (installed.end ())
    {
      // might be a virtual package, check the provides list.

      pkgCache::PrvIterator P = pkg.ProvidesList();

      for (; P.end() != true; P++)
	{
	  // Check if the provides is a hit
	  if (P.OwnerPkg().CurrentVer() != P.OwnerVer())
	    continue;

	  // Compare the versions.
	  if (debVS.CheckDep (P.ProvideVersion(), op, version.c_str ()))
	    return true;
	}

      return false;
    }
  else
    return debVS.CheckDep (installed.VerStr (), op, version.c_str ());
}

static void
add_dep_string (string &str,
		string &package, string &version, unsigned int op)
{
  str += package;
  if (op != pkgCache::Dep::NoOp)
    {
      str += " (";
      str += pkgCache::CompType (op);
      str += " ";
      str += version;
      str += ")";
    }
}

static int
check_and_encode_missing_dependencies (const char *deps, const char *end,
				       bool only_check)
{
  const char *ptr;
  string package, version;
  unsigned int op;
  bool dep_ok = true;

  ptr = deps;
  while (true)
    {
      // check one 'or group'

      bool group_ok = false;
      string group_string = "";

      while (true)
	{
	  ptr = debListParser::ParseDepends (ptr, end,
					     package, version, op,
					     false);
	  if (ptr == NULL)
	    {
	      cerr << "Error parsing depends list\n";
	      return false;
	    }

	  if (only_check && package == "maemo")
	    return status_incompatible_current;

	  add_dep_string (group_string, package, version, op);

	  if (!group_ok)
	    group_ok = check_dependency (package, version,
					 op & ~pkgCache::Dep::Or);

	  if ((op & pkgCache::Dep::Or) == 0)
	    break;

	  group_string += " | ";
	}

      if (!group_ok)
	{
	  if (only_check)
	    cerr << "FAILED: " << group_string << "\n";
	  else
	    {
	      response.encode_int (sumtype_missing);
	      response.encode_string (group_string.c_str ());
	    }
	  dep_ok = false;
	}

      if (ptr == end)
	break;
    }

  return dep_ok? status_able : status_missing;
}

static bool
get_field (pkgTagSection *section, const char *field,
	   const char *&start, const char *&end)
{
  if (section->Find (field, start, end))
    {
      // fprintf (stderr, "%s = %.*s\n", field, end-start, start);
      return true;
    }
  else
    {
      // fprintf (stderr, "%s = <undefined>\n", field);
      return false;
    }
}

static int
get_field_int (pkgTagSection *section, const char *field, int def)
{
  const char *start, *end;
  if (get_field (section, field, start, end))
    return atoi (start);
  else
    return def;
}

static void
encode_field (pkgTagSection *section, const char *field)
{
  const char *start, *end;
  if (get_field (section, field, start, end))
    response.encode_stringn (start, end-start);
  else
    response.encode_string (NULL);
}

static bool
substreq (const char *start, const char *end, const char *str)
{
  return end-start == strlen (str) && !strncmp (start, str, end-start);
}

static int
check_installable (pkgTagSection &section, bool only_user)
{
  int installable_status = status_able;
  const char *start, *end;

  if (!get_field (&section, "Architecture", start, end)
      || !(substreq (start, end, DEB_HOST_ARCH)
	   || substreq (start, end, "all")))
    installable_status = status_incompatible;
    
  if (only_user
      && get_field (&section, "Section", start, end)
      && !is_user_section (start, end))
    {
      /* Put more information for developers into the log.  They will
	 likely be confused by the "incompatible" error message when
	 testing a package that has not been properly 'sectionized'.
       */
      fprintf (stderr,
	       "Package must have \"Section: user/FOO\" "
	       "to be considered compatible.\n");
      installable_status = status_incompatible;
    }

  if (get_field (&section, "Pre-Depends", start, end))
    installable_status =
      combine_status (check_and_encode_missing_dependencies (start, end, true),
		      installable_status);

  if (get_field (&section, "Depends", start, end))
    installable_status = 
      combine_status (check_and_encode_missing_dependencies (start, end, true),
		      installable_status);

  return installable_status;
}

static void
encode_missing_dependencies (pkgTagSection &section)
{
  const char *start, *end;

  if (get_field (&section, "Pre-Depends", start, end))
    check_and_encode_missing_dependencies (start, end, false);

  if (get_field (&section, "Depends", start, end))
    check_and_encode_missing_dependencies (start, end, false);
}

void
cmd_get_file_details ()
{
  bool only_user = request.decode_int ();
  const char *filename = request.decode_string_in_place ();

  char *record = get_deb_record (filename);
  pkgTagSection section;
  if (record == NULL || !section.Scan (record, strlen (record)))
    {
      response.encode_string (basename (filename));
      response.encode_string ("");        // installed_version
      response.encode_int (0);            // installed_size
      response.encode_string ("");        // version
      response.encode_string ("");        // maintainer
      response.encode_string ("");        // section
      response.encode_int (status_corrupted);
      response.encode_int (0);            // installed size
      response.encode_string ("");        // description
      response.encode_string (NULL);      // icon
      response.encode_int (sumtype_end);
      return;
    }

  int installable_status = check_installable (section, only_user);

  const char *installed_version = NULL;
  int installed_size = 0;

  if (package_cache)
    {
      pkgDepCache &cache = *package_cache;
      pkgCache::PkgIterator pkg = cache.FindPkg (section.FindS ("Package"));
      if (!pkg.end ())
	{
	  pkgCache::VerIterator cur = pkg.CurrentVer ();
	  if (!cur.end ())
	    {
	      installed_version = cur.VerStr ();
	      installed_size = cur->InstalledSize;
	    }
	}
    }

  encode_field (&section, "Package");
  response.encode_string (installed_version);
  response.encode_int (installed_size);
  encode_field (&section, "Version");
  encode_field (&section, "Maintainer");
  encode_field (&section, "Section");
  response.encode_int (installable_status);
  response.encode_int (1024 * get_field_int (&section, "Installed-Size", 0)
		       - installed_size);
  encode_field (&section, "Description");
  encode_field (&section, "Maemo-Icon-26");

  if (installable_status != status_able)
    encode_missing_dependencies (section);
  response.encode_int (sumtype_end);

  delete[] record;
}

void
cmd_install_file ()
{
  const char *filename = request.decode_string_in_place ();

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

  need_cache_init ();
  response.encode_int (res == 0);
}
