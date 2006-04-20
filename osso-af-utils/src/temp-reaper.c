/*
 * Daemon to clean up orphaned temporary files.
 *
 * Copyright (C) 2005-2006 Nokia Corporation
 *
 * Guillem Jover <guillem.jover@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <ftw.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/wait.h>

/* The tempdirs we will cleanup. */
#define TEMPDIRS "/var/tmp", "/var/log"

/* Percentage above which the root partition is considered "almost full". */
#define ALMOSTFULL 90

/* Time after last modification (seconds). */
#define TIMEOUT (60 * 15)

/* Time to sleep before each iteration of the loop (seconds) */
#define INTERVAL (60 * 30)

/* Max number of allowed open file descriptors in ftw(). */
#define MAX_FTW_FDS 100

/* Use a hardcoded path to speedup execl.  */
#define PATH_LSOF "/usr/bin/lsof"

/* Disable output on normal builds to avoid slow downs. */
#ifdef DEBUG
#define debug(msg, ...) fprintf(stderr, msg, __VA_ARGS__);
#define VERBOSE 1
#else
#define debug(...)
#define VERBOSE 0
#endif

time_t curt;

static bool
is_open(const char *file)
{
  int status;
  pid_t pid;

  pid = fork();
  switch (pid)
  {
  case -1:
    return false;
  case 0:
    execl(PATH_LSOF, "lsof", file, NULL);
    _exit(1);
  default:
    waitpid(pid, &status, 0);

    /* Is this file not open at the moment? */
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
      return true;
    else
      return false;
  }
}

static int
reaper(const char *file, const struct stat *sb, int flag)
{
  /* All regular files which were not changed the last TIMEOUT seconds. */
  if (flag == FTW_F) {
    time_t filet = sb->st_mtime;
    time_t age = curt - filet;

    if (age > TIMEOUT) {
      if (is_open(file)) {
	debug("file '%s' %lus old but still open, not deleting\n", file, age);
      } else {
	debug("file '%s' %lus old, deleting\n", file, age);
	unlink(file);
      }
    }
  }

  return 0;
}

int
main()
{
  if (!VERBOSE) {
    close(STDIN_FILENO);
    close(STDERR_FILENO);
    close(STDOUT_FILENO);
  }

  while (1) {
    struct statvfs fs;
    int used;

    sleep(INTERVAL);

    time(&curt);

    /*
     * Get the usage percentage for the root partition
     * if it's not almost full, don't bother to delete anything
     */
    statvfs("/", &fs);

    used = 100 - (fs.f_bfree * 100 / fs.f_blocks);

    if (used > ALMOSTFULL) {
      const char *tempdirs[] = {TEMPDIRS, NULL};
      int i;

      /* Cleanup tempdirs */
      for (i = 0; tempdirs[i]; i++)
	ftw(tempdirs[i], reaper, MAX_FTW_FDS);
    }
  }

  return 0;
}

