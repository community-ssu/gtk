/*
 * $Id$
 *
 * Copyright (C) 2005 Nokia
 *
 * Authors: Michael Natterer <mitch@imendio.com>
 *	    Guillem Jover <guillem.jover@nokia.com>
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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>

#include "config.h"
#include "ui.h"
#include "report.h"
#include "invokelib.h"

/* FIXME: Should go into '/var/run/'. */
#define LAUNCHER_PIDFILE "/tmp/"PROG_NAME".pid"

typedef struct
{
  int argc;
  char **argv;
  char *filename;
} prog_t;

static char *pidfilename = LAUNCHER_PIDFILE;
static pid_t is_parent = 1;
static volatile bool sigchild_catched = false;
static volatile bool sighup_catched = false;

#ifdef DEBUG
extern char **environ;
#endif

typedef int (*entry_t)(int, char **);

static void
launch_process(prog_t *prog, ui_state state)
{
  void *module;
  entry_t entry;
  char *error_s;
  int prio;

  /* Load the launched application. */
  module = dlopen(prog->filename, RTLD_LAZY);
  if (!module)
  {
    error("loading invoked application: '%s'\n", dlerror());
    return;
  }

  dlerror();
  entry = (entry_t)dlsym(module, "main");
  error_s = dlerror();
  if (error_s != NULL)
  {
    error("loading symbol 'main': '%s'\n", error_s);
    return;
  }

  /* Initialize the launched application. */
  setenv("_", prog->argv[0], true);

  ui_client_init(prog->argv[0], state);

  /* Possibly restore process priority. */
  errno = 0;
  prio = getpriority(PRIO_PROCESS, 0);
  if (!errno && prio < 0)
    setpriority(PRIO_PROCESS, 0, 0);

#ifdef DEBUG
  {
    int i;

    for (i = 0; environ[i] != NULL; i++)
      debug("env[%i]='%s'\n", i, environ[i]);

    for (i = 0; i < prog->argc; i++)
      debug("argv[%i]='%s'\n", i, prog->argv[i]);
  }

  debug("launching process: '%s'\n", prog->filename);

  report_set_output(report_console);
#else
  report_set_output(report_none);
#endif

  _exit(entry(prog->argc, prog->argv));
}

static void
set_progname(char *progname, int argc, char **argv)
{
  int argvlen = 0;
  int i;

  for (i = 0; i < argc; i++)
    argvlen += strlen(argv[i]) + 1;

  memset(argv[0], 0, argvlen);
  strncpy(argv[0], progname, argvlen - 1);
}

static int
invoked_init(void)
{
  int fd;
  struct sockaddr_un sun;

  fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (fd < 0)
    die(1, "opening invoker socket\n");

  unlink(INVOKER_SOCK);
  umask(022);

  sun.sun_family = AF_UNIX;
  strcpy(sun.sun_path, INVOKER_SOCK);

  if (bind(fd, &sun, sizeof(sun)) < 0)
    die(1, "binding to invoker socket\n");

  if (listen(fd, 10) < 0)
    die(1, "listening to invoker socket\n");

  return fd;
}

static bool
invoked_get_magic(int fd)
{
  uint32_t msg;

  /* Receive the magic. */
  invoke_recv_msg(fd, &msg);
  if (msg != INVOKER_MSG_MAGIC)
  {
    error("receiving bad magic (%08x)\n", msg);
    return false;
  }
  else
    invoke_send_msg(fd, INVOKER_MSG_ACK);

  return true;
}

static bool
invoked_get_exec(int fd, prog_t *prog)
{
  prog->filename = invoke_recv_str(fd);
  if (!prog->filename)
    return false;

  invoke_send_msg(fd, INVOKER_MSG_ACK);

  return true;
}

static bool
invoked_get_args(int fd, prog_t *prog)
{
  int i;
  uint32_t msg;
  size_t size;

  /* Get argc. */
  invoke_recv_msg(fd, &msg);
  prog->argc = msg;
  size = msg * sizeof(char *);
  if (size < msg)
  {
    error("on buggy or malicious invoker code, heap overflow avoided\n");
    return false;
  }
  prog->argv = malloc(size);
  if (!prog->argv)
  {
    error("mallocing\n");
    return false;
  }

  /* Get argv. */
  for (i = 0; i < prog->argc; i++)
  {
    prog->argv[i] = invoke_recv_str(fd);
    if (!prog->argv[i])
    {
      error("receiving argv[%i]\n", i);
      return false;
    }
  }

  invoke_send_msg(fd, INVOKER_MSG_ACK);

  return true;
}

static bool
invoked_get_actions(int fd, prog_t *prog)
{
  while (1)
  {
    uint32_t msg;

    /* Get the action. */
    invoke_recv_msg(fd, &msg);

    switch (msg)
    {
    case INVOKER_MSG_EXEC:
      invoked_get_exec(fd, prog);
      break;
    case INVOKER_MSG_ARGS:
      invoked_get_args(fd, prog);
      break;
    case INVOKER_MSG_END:
      invoke_send_msg(fd, INVOKER_MSG_ACK);
      return true;
    default:
      error("receiving invalid action (%08x)\n", msg);
      return false;
    }
  }
}

static void
clean_daemon(int signal)
{
  if (is_parent)
  {
    unlink(pidfilename);
    unlink(INVOKER_SOCK);
    killpg(0, signal);
  }

  exit(0);
}

static void
clean_childs(void)
{
  int status;
  char *cause;
  pid_t childpid;

  while ((childpid = waitpid(-1, &status, WNOHANG)) > 0)
  {
    if (WIFEXITED(status))
      asprintf(&cause, "exit()=%d", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
      asprintf(&cause, "signal=%d", WTERMSIG(status));
    else if (WCOREDUMP(status))
      cause = strdup("coredump");

    info("child (pid=%d) exited due to %s", childpid, cause);

    free(cause);
  }
}

static void
sigchild_handler(int sig)
{
  sigchild_catched = sig;
}

static void
sighup_handler(int sig)
{
  sighup_catched = true;
}

static void
version(void)
{
  printf("%s (%s) %s\n", PROG_NAME, PACKAGE, VERSION);

  exit(0);
}

static void
usage(int status)
{
  printf("Usage: %s [options]\n"
	 "  --daemon            Fork and go into the background.\n"
	 "  --pidfile FILE      Specify a different pid file (default %s).\n"
	 "  --quiet             Do not print anything.\n"
	 "  --version           Print program version.\n"
	 "  --help              Print this help message.\n"
	 "\n"
	 "Use the invoker to start a <shared object> from the launcher.\n"
	 "Where <shared object> is a binary including a 'main' symbol.\n"
	 "Note that the binary needs to be linked with -shared or -pie.\n",
	 PROG_NAME, LAUNCHER_PIDFILE);

  exit(status);
}

int
main(int argc, char *argv[])
{
  ui_state state;
  int i;
  int fd;
  FILE *pidfile;
  bool daemonize = false;
  bool quiet = false;
  struct sigaction sig;

  /*
   * Parse arguments.
   */
  for (i = 1; i < argc; ++i)
  {
    if (strcmp(argv[i], "--daemon") == 0)
      daemonize = true;
    else if (strcmp(argv[i], "--quiet") == 0)
      quiet = true;
    else if (strcmp(argv[i], "--pidfile") == 0)
    {
      if (argv[++i])
	pidfilename = argv[i];
    }
    else if (strcmp(argv[i], "--version") == 0)
      version();
    else if (strcmp(argv[i], "--help") == 0)
      usage(0);
    else
      usage(-1);
  }

  /*
   * Daemon initialization.
   */
  report_set_output(report_syslog);

  state = ui_daemon_init(&argc, &argv);

  /* Setup signal handlers. */
  memset(&sig, 0, sizeof(sig));
  sig.sa_flags = SA_RESTART;

  sig.sa_handler = clean_daemon;
  sigaction(SIGINT, &sig, NULL);
  sigaction(SIGTERM, &sig, NULL);

  sig.sa_handler = sigchild_handler;
  sigaction(SIGCHLD, &sig, NULL);

  sig.sa_handler = sighup_handler;
  sigaction(SIGHUP, &sig, NULL);

  /* Setup the conversation channel with the invoker. */
  fd = invoked_init();

  /* Daemonize. */
  if (daemonize && fork())
      _exit(0);

  if (quiet)
  {
    close(0);
    close(1);
    close(2);

    if (open("/dev/null", O_RDONLY) < 0)
      die(1, "opening /dev/null readonly");
    if (dup(open("/dev/null", O_WRONLY)) < 0)
      die(1, "opening /dev/null writeonly");
  }

  if (daemonize)
  {
    pid_t pid;

    if (setsid() < 0)
      die(1, "setting session id");

    pid = fork();
    if (pid < 0)
      die(1, "forking while daemonizing");
    else if (pid)
      _exit(0);
  }

  /* Generate a pid file. */
  pidfile = fopen(pidfilename, "w");
  if (pidfile)
  {
    pid_t pid = getpid();
    fprintf(pidfile, "%d\n", pid);
    fclose(pidfile);
  }

  /*
   * Application invokation loop.
   */
  while (1)
  {
    prog_t prog;
    int sd;
    int errno_accept;

    /* Accept a new invokation. */
    siginterrupt(SIGCHLD, 1);
    sd = accept(fd, NULL, NULL);
    errno_accept = errno;
    siginterrupt(SIGCHLD, 0);

    /* Handle signals received. */
    if (sigchild_catched)
    {
      clean_childs();
      sigchild_catched = false;
    }

    if (sighup_catched)
    {
      ui_state_reload(state);
      sighup_catched = false;
    }

    /* Minimal error handling. */
    if (sd < 0)
    {
      if (errno_accept != EINTR)
	error("accepting connections (%s)\n", strerror(errno_accept));

      continue;
    }

    /* Start conversation with the invoker. */
    if (!invoked_get_magic(sd))
    {
      close(sd);
      continue;
    }

    is_parent = fork();
    switch (is_parent)
    {
    case -1: /* Error. */
      error("forking while invoking\n");
      break;

    case 0: /* Child. */
      /* Restore default signals. */
      memset(&sig, 0, sizeof(sig));
      sig.sa_handler = SIG_DFL;
      sig.sa_flags = SA_RESTART;

      sigaction(SIGINT, &sig, NULL);
      sigaction(SIGTERM, &sig, NULL);
      sigaction(SIGCHLD, &sig, NULL);
      sigaction(SIGHUP, &sig, NULL);

      invoked_get_actions(sd, &prog);

      close(sd);
      close(fd);

      /* Invoke it. */
      if (prog.filename)
      {
        info("invoking '%s'\n", prog.filename);
        set_progname(prog.argv[0], argc, argv);
        launch_process(&prog, state);
      }
      else
        error("nothing to invoke\n");

      _exit (0);
      break;

    default: /* Parent. */
      close(sd);
      break;
    }
  }

  return 0;
}

