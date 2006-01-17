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

#include <iostream>
#include <sys/statvfs.h>
#include <pthread.h>

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

#include "operations.h"
#include "util.h"
#include "log.h"

#define _(x) x

using namespace std;

static pkgCacheFile *package_cache = NULL;

void
operations_init ()
{
  OpProgress progress;

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
     // XXX - maybe launch into a deaf, blind and dumb mode that can
     //       only present the log.
     _error->DumpErrors();
     exit (1);
   }
}

class OpGuiProgress : public OpProgress
{
  virtual void
  Update ()
  {
    if (progress)
      progress->set ("Updating", Percent/100.0);
  }

  progress_with_cancel *progress;

public:
  OpGuiProgress (progress_with_cancel *pc)
  {
    progress = pc;
  }
};

class GuiAcquireStatus : public pkgAcquireStatus
{
  progress_with_cancel *progress;

  virtual bool
  MediaChange (string Media, string Drive)
  {
    return false;
  }

  virtual bool
  Pulse (pkgAcquire *Owner)
  {
    pkgAcquireStatus::Pulse (Owner);

    if (progress)
      {
	float fraction = (double(CurrentBytes + CurrentItems) / 
			  double(TotalBytes + TotalItems));
	progress->set ("Downloading", fraction);
	return !progress->is_cancelled ();
      }

    return true;
  }

 public:
  GuiAcquireStatus (progress_with_cancel *pc)
  {
    progress = pc;
  }
};

static bool
update_package_cache_no_note (progress_with_cancel *progress)
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
  GuiAcquireStatus Stat (progress);
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
    OpGuiProgress Prog (progress);
    if (!package_cache->BuildCaches (Prog))
      return false;
    operations_init ();
  }

  return !Failed;
}

bool
update_package_cache_no_confirm ()
{
  bool success;

  progress_with_cancel progress;
  success = update_package_cache_no_note (&progress);
  progress.close ();

  if (!success)
    {
      _error->DumpErrors ();
      annoy_user ("The list of packages could not be updated.");
      return false;
    }
  return true;
}

static bool
is_package_cache_out_of_date ()
{
  // XXX - ask after the configured time-out
  return false;
}

bool
update_package_cache ()
{
  if (is_package_cache_out_of_date ())
    {
      if (ask_yes_no ("The list of packages might be out-of-date.  "
		      "Do you want to update the list now?"))
	return update_package_cache_no_confirm ();
      else
	{
	  annoy_user ("You can update the list of packages at any "
		      "time from the menu.");
	  return false;
	}
    }
  else
    return true;
}

static version_info *
make_version_info (pkgCache::VerIterator &ver)
{
  if (ver.end())
    {
      return NULL;
    }
  else
    {
      version_info *vi = new version_info;
      pkgRecords Recs (*package_cache);
      pkgRecords::Parser &P = Recs.Lookup (ver.FileList ());

      vi->version = ver.VerStr();
      vi->section = ver.Section();
      vi->maintainer = g_strdup (P.Maintainer().c_str());
      vi->short_description = g_strdup (P.ShortDesc().c_str());
      vi->cache = ver;
      return vi;
    }
}

gchar *
get_long_description (version_info *v)
{
  pkgRecords Recs (*package_cache);
  pkgRecords::Parser &P = Recs.Lookup (v->cache.FileList ());
  return g_strdup (P.LongDesc().c_str());
}

static package_info *
make_package_info (pkgCache::PkgIterator &pkg,
		   pkgCache::VerIterator &installed,
		   pkgCache::VerIterator &available)
{
  package_info *pi = new package_info;
  pi->name = pkg.Name();
  pi->installed = make_version_info (installed);
  if (pi->installed)
    pi->installed_size = installed->InstalledSize;
  else
    pi->installed_size = 0;
  if (installed == available)
    pi->available = NULL;
  else
    pi->available = make_version_info (available);
  pi->cache = pkg;

  pi->install_simulated = false;
  pi->uninstall_simulated = false;

  return pi;
}

static gint
compare_package_info (gconstpointer a_data, gconstpointer b_data)
{
  package_info *a = (package_info *)a_data;
  package_info *b = (package_info *)b_data;
  return strcmp (a->name, b->name);
}

static section_info *
find_section_info (GList **list_ptr, const gchar *name)
{
  if (name == NULL)
    name = "none";

  for (GList *ptr = *list_ptr; ptr; ptr = ptr->next)
    if (!strcmp (((section_info *)ptr->data)->name, name))
      return (section_info *)ptr->data;

  section_info *si = new section_info;
  si->name = name;
  si->packages = NULL;
  *list_ptr = g_list_prepend (*list_ptr, si);
  return si;
}

static void
sort_section_packages (gpointer data, gpointer unused)
{
  section_info *si = (section_info *)data;
  si->packages = g_list_sort (si->packages, compare_package_info);
}

static gint
compare_section_info (gconstpointer a_data, gconstpointer b_data)
{
  section_info *a = (section_info *)a_data;
  section_info *b = (section_info *)b_data;

  return strcmp (a->name, b->name);
}
 
GList *
get_package_list (bool installed_flag,
		  bool skip_non_available)
{
  pkgCache &cache = *package_cache;
  GList *result = NULL;

  for (pkgCache::PkgIterator pkg = cache.PkgBegin();
       pkg != cache.PkgEnd();
       pkg++)
    {
      pkgCache::VerIterator installed = pkg.CurrentVer ();
      bool is_installed = !installed.end();
      if (is_installed == installed_flag)
	{
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

	  if (have_latest)
	    {
	      if (!skip_non_available || installed.CompareVer (latest) < 0)
		{
		  package_info *pi = make_package_info (pkg,
							installed, 
							latest);
		  result = g_list_prepend (result, pi);
		}
	    }
	}
    }

  printf ("got %d packages\n", g_list_length (result));
  result = g_list_sort (result, compare_package_info);
  return result;
}

static void
free_package_info (gpointer data, gpointer user_data)
{
  package_info *pi = (package_info *)data;
  delete pi;
}

void
free_package_list (GList *pl)
{
  g_list_foreach (pl, free_package_info, NULL);
  g_list_free (pl);
}

GList *
sectionize_packages (GList *packages, bool installed)
{
  GList *result = NULL;
  while (packages)
    {
      GList *next = packages->next;
      package_info *pi = (package_info *)packages->data;
      const gchar *section = (installed
			      ? pi->installed->section
			      : pi->available->section);
      section_info *si = find_section_info (&result, section);
      g_list_remove_link (packages, packages);
      si->packages = g_list_concat (si->packages, packages);
      packages = next;
    }
  return g_list_sort (result, compare_section_info);
}

static void
free_section_info (gpointer data, gpointer user_data)
{
  section_info *si = (section_info *)data;
  free_package_list (si->packages);
  delete si;
}

void
free_section_list (GList *sl)
{
  g_list_foreach (sl, free_section_info, NULL);
  g_list_free (sl);
}

void
simulate_install (package_info *pi)
{
  if (pi->install_simulated)
    return;

  pi->install_simulated = true;
  pi->download_size = 0;
  pi->install_user_size_delta = 0;

  if (pi->available == NULL)
    pi->install_possible = false;
  else
    {
      pkgDepCache &cache = *package_cache;
      pkgCache::PkgIterator pkg = pi->cache;
      int old_broken_count;

      cache.Init(NULL);
      
      old_broken_count = cache.BrokenCount();
      (*package_cache)->MarkInstall (pkg);
      if (cache.BrokenCount() > old_broken_count)
	pi->install_possible = false;
      else
	{
	  pi->install_possible = true;
	  pi->download_size = cache.DebSize ();
	  pi->install_user_size_delta = cache.UsrSize ();
	}
    }
}

void
simulate_uninstall (package_info *pi)
{
  if (pi->uninstall_simulated)
    return;

  pi->uninstall_simulated = true;
  pi->uninstall_user_size_delta = 0;

  pkgDepCache &cache = *package_cache;
  pkgCache::PkgIterator pkg = pi->cache;
  int old_broken_count;

  cache.Init(NULL);

  old_broken_count = cache.BrokenCount();
  (*package_cache)->MarkDelete (pkg);
  if (cache.BrokenCount() > old_broken_count)
    pi->uninstall_possible = false;
  else
    {
      pi->uninstall_possible = true;
      pi->uninstall_user_size_delta = cache.UsrSize ();
    }
}

void show_broken (pkgCache::PkgIterator &pkg, pkgCache::PkgIterator &want,
		  GString *str);

/* Dependency details are one of:

   When the install/upgrade/uninstall can succeed:

   <pkg> <ver> is needed as well and will be installed/upgraded.
   <pkg> <ver> is no longer needed and will be removed.

   When it is known to fail:

   <pkg> <ver> is needed but is not available.
   <pkg> <ver> and <pkg> <ver> can not be installed at the same time.
   <pkg> <ver> is needed by <pkg> <ver>.
*/

void
show_install_summary (package_info *p, GString *str)
{
  pkgDepCache &cache = *package_cache;
  pkgCache::PkgIterator want = p->cache;

  cache.Init(NULL);

  if (cache.BrokenCount() > 0)
    printf ("[ Some installed packages are broken! ]\n");

  (*package_cache)->MarkInstall (want);

  for (pkgCache::PkgIterator pkg = cache.PkgBegin();
       pkg.end() != true;
       pkg++)
    {
      if (cache[pkg].NewInstall())
	g_string_append_printf (str, " Installing: %s %s\n",
				pkg.Name(), cache[pkg].CandVersion);
       
      if (cache[pkg].Upgrade() && !cache[pkg].NewInstall())
	g_string_append_printf (str, " Upgrading: %s %s\n",
				pkg.Name(), cache[pkg].CandVersion);

      if (cache[pkg].Delete())
	g_string_append_printf (str, " Removing: %s %s\n",
				pkg.Name(), cache[pkg].CandVersion);

      if (cache[pkg].InstBroken())
	show_broken (pkg, want, str);
    }
}

void
show_uninstall_summary (package_info *p, GString *str)
{
  pkgDepCache &cache = *package_cache;
  pkgCache::PkgIterator want = p->cache;

  cache.Init(NULL);

  if (cache.BrokenCount() > 0)
    printf ("[ Some installed packages are broken! ]\n");

  (*package_cache)->MarkDelete (want);

  for (pkgCache::PkgIterator pkg = cache.PkgBegin();
       pkg.end() != true;
       pkg++)
    {
      if (cache[pkg].Delete())
	g_string_append_printf (str, " Removing: %s %s\n",
				pkg.Name(), cache[pkg].CandVersion);

      if (cache[pkg].InstBroken())
	g_string_append_printf (str, " Needed by: %s %s\n",
				pkg.Name(), cache[pkg].CurVersion);
    }
}

void
show_broken (pkgCache::PkgIterator &pkg, pkgCache::PkgIterator &want,
	     GString *str)
{
  const char *tag;
  pkgDepCache &Cache = *package_cache;
  pkgCache::VerIterator Ver = Cache[pkg].InstVerIter(Cache);
      
  if (Ver.end() == true)
    return;
      
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
	tag = "Missing";
      else if (Start->Type == pkgCache::Dep::Conflicts)
	tag = "Conflicting";
      else
	continue;

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
	    g_string_append_printf (str, " %s: %s", tag, pkg.Name());
	  else
	    {
	      g_string_append_printf (str, " %s: %s",
				      tag, Start.TargetPkg().Name());
	      if (Start.TargetVer() != 0)
		g_string_append_printf (str, " %s %s",
					Start.CompType(), Start.TargetVer());
	    }

	  if (Start != End)
	    g_string_append_printf (str, " or");
	  g_string_append_printf (str, "\n");
	  
	  if (Start == End)
	    break;
	  Start++;
	}
    }   
}

void
show_dependencies (version_info *vi, GString *str)
{
  pkgCache::VerIterator ver = vi->cache;

  for (pkgCache::DepIterator dep = ver.DependsList(); !dep.end(); )
    {
      pkgCache::DepIterator start;
      pkgCache::DepIterator end;
      dep.GlobOr(start, end);

      g_string_append_printf (str, " %s", start.DepType ());
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
      g_string_append_printf (str, "\n");
    }
}

static bool
CheckAuth(pkgAcquire& Fetcher)
{
   string UntrustedList;
   for (pkgAcquire::ItemIterator I = Fetcher.ItemsBegin(); I < Fetcher.ItemsEnd(); ++I)
   {
      if (!(*I)->IsTrusted())
      {
         UntrustedList += string((*I)->ShortDesc()) + " ";
      }
   }

   if (UntrustedList == "")
   {
      return true;
   }
        
   cout << "WARNING: The following packages cannot be authenticated: "
	<< UntrustedList << "\n";

   return true;
}

void
setup_install (package_info *pi)
{
  pkgDepCache &cache = *package_cache;
  pkgCache::PkgIterator pkg = pi->cache;

  cache.Init(NULL);
  (*package_cache)->MarkInstall (pkg);
}

void
setup_uninstall (package_info *pi)
{
  pkgDepCache &cache = *package_cache;
  pkgCache::PkgIterator pkg = pi->cache;

  cache.Init(NULL);
  (*package_cache)->MarkDelete (pkg);
}

struct package_manager_thread_data {
  int status_fd;
  pkgPackageManager *PM;
  pkgPackageManager::OrderResult res;
};

static void *
package_manager_thread (void *raw_data)
{
  package_manager_thread_data *data = (package_manager_thread_data *)raw_data;
  printf ("thread started\n");
  data->res = data->PM->DoInstall (data->status_fd);
  printf ("thread done, closing\n");
  close (data->status_fd);
  return NULL;
}

struct status_reader_data {
  GString *string;
  GMainLoop *loop;
  progress_with_cancel *progress;
};

static gboolean
read_status (GIOChannel *channel, GIOCondition cond, gpointer raw_data)
{
  status_reader_data *data = (status_reader_data *)raw_data;
  gchar buf[256];
  gsize count;
  GIOStatus status;
  int id;
  
#if 0
  /* XXX - this blocks sometime.  Maybe setting the encoding to NULL
           will work, but for now we just do it the old school way...
  */
  status = g_io_channel_read_chars (channel, buf, 256, &count, NULL);
#else
  {
    int fd = g_io_channel_unix_get_fd (channel);
    printf ("reading\n");
    int n = read (fd, buf, 256);
    printf ("read %d\n", n);

    if (n > 0)
      {
	status = G_IO_STATUS_NORMAL;
	count = n;
      }
    else
      {
	status = G_IO_STATUS_EOF;
	count = 0;
      }
  }
#endif

  if (status == G_IO_STATUS_NORMAL)
    {
      GString *string = data->string;
      g_string_append_len (data->string, buf, count);
      gchar *line_end;

      while ((line_end = strchr (string->str, '\n')))
	{
	  float percentage;
	  gchar *ptr, *title;

	  *line_end = '\0';
	  ptr = string->str;
	  if (strncmp (ptr, "pmstatus:", 9) != 0)
	    goto fail;
	  ptr += 9;
	  ptr = strchr (ptr, ':');
	  if (ptr == NULL)
	    goto fail;
	  ptr += 1;
	  percentage = atof (ptr);
	  ptr = strchr (ptr, ':');
	  if (ptr == NULL)
	    goto fail;
	  ptr += 1;
	  title = ptr;
	     
	  data->progress->set (title, percentage/100.0);
	  goto out;
	  
	fail:
	  printf ("unrecognized status: %s\n", string->str);
	out:
	  g_string_erase (string, 0, line_end - string->str + 1);
	}
      return TRUE;
    }
  else
    {
      g_io_channel_shutdown (channel, 0, NULL);
      g_main_loop_quit (data->loop);
      return FALSE;
    }
}

static pkgPackageManager::OrderResult
run_package_manager_in_thread (pkgPackageManager *PM,
			       progress_with_cancel *progress)
{
#if 0
  /* The thread sometimes blocks unmotivatedly, but only in Scratchbox.
   */
  pthread_t thread;
  package_manager_thread_data data;
  int fds[2];

  if (pipe (fds) < 0)
    {
      perror ("pipe");
      return pkgPackageManager::Failed;
    }
      
  data.PM = PM;
  data.status_fd = fds[1];
  if (pthread_create (&thread, NULL, package_manager_thread, &data))
    {
      perror ("pthread_create");
      return pkgPackageManager::Failed;
    }

  {
    GIOChannel *channel = g_io_channel_unix_new (fds[0]);
    status_reader_data data;

    data.string = g_string_new ("");
    data.loop = g_main_loop_new (NULL, 0);
    data.progress = progress;
    g_io_add_watch (channel, GIOCondition (G_IO_IN | G_IO_HUP | G_IO_ERR),
		    read_status, &data);
    g_io_channel_unref (channel);

    g_main_loop_run (data.loop);
    g_main_loop_unref (data.loop);
  }

  pthread_join (thread, NULL);

  return data.res;
#else
  return PM->DoInstall (-1);
#endif
}

bool
do_operations (progress_with_cancel *pc)
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
   
   // Sanity check
   if (Cache->BrokenCount() != 0)
   {
     return _error->Error(_("Internal error, install_packages was called with broken packages!"));
   }

   if (Cache->DelCount() == 0 && Cache->InstCount() == 0 &&
       Cache->BadCount() == 0)
      return true;
   
   // Create the text record parser
   pkgRecords Recs(Cache);
   if (_error->PendingError() == true)
      return false;
   
   // Lock the archive directory
   FileFd Lock;
   if (_config->FindB("Debug::NoLocking",false) == false)
   {
      Lock.Fd(GetLock(_config->FindDir("Dir::Cache::Archives") + "lock"));
      if (_error->PendingError() == true)
	 return _error->Error(_("Unable to lock the download directory"));
   }
   
   // Create the download object
   GuiAcquireStatus Stat(pc);   
   pkgAcquire Fetcher(&Stat);

   // Read the source list
   pkgSourceList List;
   if (List.ReadMainList() == false)
      return _error->Error(_("The list of sources could not be read."));
   
   // Create the package manager and prepare to download
   SPtr<pkgPackageManager> PM= _system->CreatePM(Cache);
   if (PM->GetArchives(&Fetcher,&List,&Recs) == false || 
       _error->PendingError() == true)
      return false;

   // Display statistics
   double FetchBytes = Fetcher.FetchNeeded();
   double FetchPBytes = Fetcher.PartialPresent();
   double DebBytes = Fetcher.TotalNeeded();
   if (DebBytes != Cache->DebSize())
     {
       add_log ("%f != %f\n", DebBytes, Cache->DebSize());
       add_log ("How odd.. The sizes didn't match, "
		"email apt@packages.debian.org\n");
     }
   
   // Number of bytes
   if (DebBytes != FetchBytes)
     add_log ("Need to get %sB/%sB of archives.\n",
	      SizeToStr(FetchBytes).c_str(),SizeToStr(DebBytes).c_str());
   else
     add_log ("Need to get %sB of archives.\n",
	      SizeToStr(DebBytes).c_str());

   // Size delta
   if (Cache->UsrSize() >= 0)
     add_log ("After unpacking %sB of additional disk space will be used.\n",
	      SizeToStr(Cache->UsrSize()).c_str());
   else
     add_log ("After unpacking %sB disk space will be freed.\n",
	      SizeToStr(-1*Cache->UsrSize()).c_str());

   if (_error->PendingError() == true)
      return false;

   /* Check for enough free space. */
   {
     struct statvfs Buf;
     string OutputDir = _config->FindDir("Dir::Cache::Archives");
     if (statvfs(OutputDir.c_str(),&Buf) != 0)
       return _error->Errno("statvfs",_("Couldn't determine free space in %s"),
			    OutputDir.c_str());
     if (unsigned(Buf.f_bfree) < (FetchBytes - FetchPBytes)/Buf.f_bsize)
       return _error->Error(_("You don't have enough free space in %s."),
			    OutputDir.c_str());
   }
   
   if (!CheckAuth(Fetcher))
      return false;

   // Run it
   while (1)
   {
      bool Transient = false;
      if (Fetcher.Run() == pkgAcquire::Failed)
	 return false;
      
      // Print out errors
      bool Failed = false;
      for (pkgAcquire::ItemIterator I = Fetcher.ItemsBegin(); I != Fetcher.ItemsEnd(); I++)
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

	 add_log ("Failed to fetch %s: %s\n",
		  (*I)->DescURI().c_str(),
		  (*I)->ErrorText.c_str());
	 Failed = true;
      }

      if (Failed == true)
      {
	return _error->Error(_("Unable to fetch some archives."));
      }
      
      _system->UnLock();
      pkgPackageManager::OrderResult Res =
	run_package_manager_in_thread (PM, pc);
      if (Res == pkgPackageManager::Failed || _error->PendingError() == true)
	 return false;
      if (Res == pkgPackageManager::Completed)
	 return true;
      
      // Reload the fetcher object and loop again for media swapping
      Fetcher.Shutdown();
      if (PM->GetArchives(&Fetcher,&List,&Recs) == false)
	 return false;
      
      _system->Lock();
   }
}
