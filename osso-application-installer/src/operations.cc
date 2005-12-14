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

#include "operations.h"
#include "util.h"

#include <apt-pkg/init.h>
#include <apt-pkg/error.h>
#include <apt-pkg/pkgcache.h>
#include <apt-pkg/pkgcachegen.h>
#include <apt-pkg/sourcelist.h>
#include <apt-pkg/acquire.h>
#include <apt-pkg/acquire-item.h>
#include <apt-pkg/cachefile.h>

#define _(x) x

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
    pkgCacheFile Cache;
    OpGuiProgress Prog (progress);
    if (!Cache.BuildCaches (Prog, true))
      return false;
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

bool
update_package_cache ()
{
  // XXX - only ask after the configured time-out

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
