/*
 * $Id$
 *
 * Copyright (C) 2005, 2006 Nokia Corporation
 *
 * Authors: Michael Natterer <mitch@imendio.com>
 *	    Guillem Jover <guillem.jover@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
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

#include "config.h"

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
#ifdef HAVE_PRCTL_SET_NAME
#include <sys/prctl.h>
#endif
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>

#include "report.h"
#include "invokelib.h"
#include "comm_dbus.h"
#include "booster.h"

/* FIXME: Should go into '/var/run/'. */
#define LAUNCHER_PIDFILE "/tmp/"PROG_NAME".pid"

typedef int (*entry_t)(int, char **);

typedef struct
{
  int options;
  int argc;
  char **argv;
  char *filename;
  char *name;
  int prio;
  entry_t entry;
} prog_t;

typedef struct
{
  char *name;
  pid_t pid;
  int sock;
} child_t;

typedef struct
{
  int n;
  int used;
  child_t *list;
} kindergarten_t;

static char *pidfilename = LAUNCHER_PIDFILE;
static pid_t is_parent = 1;
static volatile bool sigchild_catched = false;
static volatile bool sighup_catched = false;
static bool send_app_died = false;

#ifdef DEBUG
extern char **environ;
#endif

static bool
rise_oom_defense(pid_t pid)
{
  pid_t defender_pid;
  int status;

  defender_pid = fork();
  if (defender_pid)
    waitpid(defender_pid, &status, 0);
  else
  {
    char pidstr[20];

    snprintf(pidstr, sizeof(pidstr), "%d", pid);
    execl(MAEMO_DEFENDER, MAEMO_DEFENDER, pidstr, NULL);
    _exit(1);
  }

  if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
    return true;
  else
  {
    error("rising the oom shield for pid=%d status=%d\n", pid, status);
    return false;
  }
}

static void
load_main(prog_t *prog)
{
  void *module;
  char *error_s;

  /* Load the launched application. */
  module = dlopen(prog->filename, RTLD_LAZY | RTLD_GLOBAL);
  if (!module)
    die(1, "loading invoked application: '%s'\n", dlerror());

  dlerror();
  prog->entry = (entry_t)dlsym(module, "main");
  error_s = dlerror();
  if (error_s != NULL)
    die(1, "loading symbol 'main': '%s'\n", error_s);
}

static void
launch_process(prog_t *prog)
{
  int cur_prio;

  load_main(prog);

  /* Possibly restore process priority. */
  errno = 0;
  cur_prio = getpriority(PRIO_PROCESS, 0);
  if (!errno && cur_prio < prog->prio)
    setpriority(PRIO_PROCESS, 0, prog->prio);

  /* Protect our special childs from the oom monster. */
  if (prog->prio < 0)
    rise_oom_defense(getpid());

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

  _exit(prog->entry(prog->argc, prog->argv));
}

#ifdef HAVE_PRCTL_SET_NAME
static void
set_process_name(const char *progname)
{
  prctl(PR_SET_NAME, basename(progname));
}
#else
static inline void
set_process_name(const char *progname)
{
}
#endif

static void
set_progname(char *progname, int argc, char **argv)
{
  int argvlen = 0;
  int i;

  for (i = 0; i < argc; i++)
    argvlen += strlen(argv[i]) + 1;

  memset(argv[0], 0, argvlen);
  strncpy(argv[0], progname, argvlen - 1);

  set_process_name(progname);

  setenv("_", progname, true);
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
invoked_get_magic(int fd, prog_t *prog)
{
  uint32_t msg;

  /* Receive the magic. */
  invoke_recv_msg(fd, &msg);
  if ((msg & INVOKER_MSG_MASK) == INVOKER_MSG_MAGIC)
  {
    if ((msg & INVOKER_MSG_MAGIC_VERSION_MASK) == INVOKER_MSG_MAGIC_VERSION)
      invoke_send_msg(fd, INVOKER_MSG_ACK);
    else
    {
      error("receiving bad magic version (%08x)\n", msg);
      return false;
    }
  }
  else
  {
    error("receiving bad magic (%08x)\n", msg);
    return false;
  }

  prog->options = msg & INVOKER_MSG_MAGIC_OPTION_MASK;

  return true;
}

static bool
invoked_get_name(int fd, prog_t *prog)
{
  uint32_t msg;

  /* Get the action. */
  invoke_recv_msg(fd, &msg);
  if (msg != INVOKER_MSG_NAME)
  {
    error("receiving invalid action (%08x)\n", msg);
    return false;
  }

  prog->name = invoke_recv_str(fd);
  if (!prog->name)
    return false;

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
invoked_get_prio(int fd, prog_t *prog)
{
  uint32_t msg;

  invoke_recv_msg(fd, &msg);
  prog->prio = msg;

  invoke_send_msg(fd, INVOKER_MSG_ACK);

  return true;
}

static bool
invoked_send_pid(int fd, pid_t pid)
{
  invoke_send_msg(fd, INVOKER_MSG_PID);
  invoke_send_msg(fd, pid);

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
    case INVOKER_MSG_PRIO:
      invoked_get_prio(fd, prog);
      break;
    case INVOKER_MSG_END:
      invoke_send_msg(fd, INVOKER_MSG_ACK);

      if (prog->options & INVOKER_MSG_MAGIC_OPTION_WAIT)
        invoked_send_pid(fd, getpid());

      return true;
    default:
      error("receiving invalid action (%08x)\n", msg);
      return false;
    }
  }
}

static bool
invoked_send_exit(int fd, int status)
{
  invoke_send_msg(fd, INVOKER_MSG_EXIT);
  invoke_send_msg(fd, status);

  return true;
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

static kindergarten_t *
alloc_childs(int n)
{
  kindergarten_t *childs;

  childs = malloc(sizeof(kindergarten_t));
  if (!childs)
  {
    error("allocating a kindergarten\n");
    return NULL;
  }

  childs->used = 0;
  childs->n = n;
  childs->list = calloc(n, sizeof(child_t));
  if (!childs->list)
  {
    error("allocating a child list\n");
    return NULL;
  }

  return childs;
}

static bool
grow_childs(kindergarten_t *childs)
{
  int halfsize = childs->n * sizeof(child_t);
  int size = halfsize * 2;
  void *p;

  p = realloc(childs->list, size);
  if (!p)
    return false;

  childs->list = p;
  memset(childs->list + childs->n, 0, halfsize);

  childs->n *= 2;

  return true;
}

static bool
release_childs(kindergarten_t *childs)
{
  int i;
  child_t *list = childs->list;

  for (i = 0; i < childs->used; i++)
  {
    close(list[i].sock);
    free(list[i].name);
  }

  childs->used = 0;
  childs->n = 0;
  free(childs->list);

  return true;
}

static int
get_child_slot_by_pid(kindergarten_t *childs, pid_t pid)
{
  child_t *list = childs->list;
  int i;

  for (i = 0; i < childs->n; i++)
    if (list[i].pid == pid)
      return i;

  return -1;
}

static bool
invoked_send_fake_exit(int sock)
{
  /* Send a fake exit code, so the invoker does not wait for us. */
  invoked_send_exit(sock, 0);
  close(sock);

  return true;
}

static bool
assign_child_slot(kindergarten_t *childs, child_t *child)
{
  int id;

  if (childs->used == childs->n && !grow_childs(childs))
  {
    error("cannot make a bigger kindergarten, not tracking child %d\n",
	  child->pid);
    invoked_send_fake_exit(child->sock);
    return false;
  }

  id = get_child_slot_by_pid(childs, 0);
  if (id < 0)
  {
    error("this cannot be happening! no free slots on the kindergarten,\n"
	  "not tracking child %d\n", child->pid);
    invoked_send_fake_exit(child->sock);
    return false;
  }

  childs->list[id].name = child->name;
  childs->list[id].sock = child->sock;
  childs->list[id].pid = child->pid;
  childs->used++;

  return true;
}

static bool
child_died_painfully(int status)
{
  if ((WIFEXITED(status) && WEXITSTATUS(status) != 0) ||
      (WIFSIGNALED(status) && (WTERMSIG(status) != SIGINT &&
			       WTERMSIG(status) != SIGTERM &&
			       WTERMSIG(status) != SIGKILL)))
    return true;
  else
    return false;
}

static bool
release_child_slot(kindergarten_t *childs, pid_t pid, int status)
{
  int id = get_child_slot_by_pid(childs, pid);

  if (id >= 0)
  {
    child_t *child = &childs->list[id];

    if (send_app_died && child_died_painfully(status))
      comm_send_app_died(child->name, pid, status);

    invoked_send_exit(child->sock, status);

    close(child->sock);
    free(child->name);

    child->name = NULL;
    child->sock = 0;
    child->pid = 0;

    childs->used--;
  }
  else
    info("no child %i found in the kindergarten.\n", pid);

  return true;
}

static void
clean_childs(kindergarten_t *childs)
{
  int status;
  char *cause;
  pid_t childpid;

  while ((childpid = waitpid(-1, &status, WNOHANG)) > 0)
  {
    release_child_slot(childs, childpid, status);

    if (WIFEXITED(status))
      asprintf(&cause, "exit()=%d", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
      asprintf(&cause, "signal=%d", WTERMSIG(status));

    info("child (pid=%d) exited due to %s\n", childpid, cause);

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
sigs_init(void)
{
  struct sigaction sig;

  memset(&sig, 0, sizeof(sig));
  sig.sa_flags = SA_RESTART;

  sig.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &sig, NULL);

  sig.sa_handler = clean_daemon;
  sigaction(SIGINT, &sig, NULL);
  sigaction(SIGTERM, &sig, NULL);

  sig.sa_handler = sigchild_handler;
  sigaction(SIGCHLD, &sig, NULL);

  sig.sa_handler = sighup_handler;
  sigaction(SIGHUP, &sig, NULL);
}

static void
sigs_restore(void)
{
  struct sigaction sig;

  memset(&sig, 0, sizeof(sig));
  sig.sa_handler = SIG_DFL;
  sig.sa_flags = SA_RESTART;

  sigaction(SIGPIPE, &sig, NULL);
  sigaction(SIGINT, &sig, NULL);
  sigaction(SIGTERM, &sig, NULL);
  sigaction(SIGCHLD, &sig, NULL);
  sigaction(SIGHUP, &sig, NULL);
}

static void
sigs_interrupt(int flag)
{
  siginterrupt(SIGCHLD, flag);
}

static void
env_init(void)
{
  unsetenv("LD_BIND_NOW");
}

static void
create_pidfile(void)
{
  FILE *pidfile = fopen(pidfilename, "w");

  if (pidfile)
  {
    pid_t pid = getpid();
    fprintf(pidfile, "%d\n", pid);
    fclose(pidfile);
  }
}

static void
daemonize(void)
{
  pid_t pid;

  if (fork())
    _exit(0);

  if (setsid() < 0)
    die(1, "setting session id");

  pid = fork();
  if (pid < 0)
    die(1, "forking while daemonizing");
  else if (pid)
    _exit(0);

  chdir("/");

  /* Protect us from the oom monster. */
  rise_oom_defense(getpid());
}

static void
console_quiet(void)
{
  report_set_output(report_syslog);

  close(0);
  close(1);
  close(2);

  if (open("/dev/null", O_RDONLY) < 0)
    die(1, "opening /dev/null readonly");
  if (dup(open("/dev/null", O_WRONLY)) < 0)
    die(1, "opening /dev/null writeonly");
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
	 "\n"
	 "Options:\n"
	 "  --daemon            Fork and go into the background.\n"
	 "  --pidfile FILE      Specify a different pid file (default %s).\n"
	 "  --booster MODULE    Specify a booster module to use (default '%s').\n"
	 "  --send-app-died     Send application died signal.\n"
	 "  --quiet             Do not print anything.\n"
	 "  --version           Print program version.\n"
	 "  --help              Print this help message.\n"
	 "\n"
	 "Use the invoker to start a <shared object> from the launcher.\n"
	 "Where <shared object> is a binary including a 'main' symbol.\n"
	 "Note that the binary needs to be linked with -shared or -pie.\n",
	 PROG_NAME, LAUNCHER_PIDFILE, MAEMO_BOOSTER_DEFAULT);

  exit(status);
}

int
main(int argc, char *argv[])
{
  booster_t booster = { .name = MAEMO_BOOSTER_DEFAULT };
  kindergarten_t *childs;
  const int initial_child_slots = 64;
  int i;
  int fd;
  bool daemon = false;
  bool quiet = false;

  /*
   * Parse arguments.
   */
  for (i = 1; i < argc; ++i)
  {
    if (strcmp(argv[i], "--daemon") == 0)
      daemon = true;
    else if (strcmp(argv[i], "--quiet") == 0)
      quiet = true;
    else if (strcmp(argv[i], "--pidfile") == 0)
    {
      if (argv[++i])
	pidfilename = argv[i];
    }
    else if (strcmp(argv[i], "--booster") == 0)
    {
      if (argv[++i])
	booster.name = argv[i];
    }
    else if (strcmp(argv[i], "--send-app-died") == 0)
      send_app_died = true;
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
  booster_module_load(&booster);

  booster.state = booster.api->booster_preinit(&argc, &argv);

  sigs_init();
  env_init();

  /* Setup child tracking. */
  childs = alloc_childs(initial_child_slots);

  /* Setup the conversation channel with the invoker. */
  fd = invoked_init();

  if (daemon)
    daemonize();

  if (quiet)
    console_quiet();

  create_pidfile();

  /*
   * Application invokation loop.
   */
  while (1)
  {
    prog_t prog;
    int sd;
    int errno_accept;

    /* Accept a new invokation. */
    sigs_interrupt(1);
    sd = accept(fd, NULL, NULL);
    errno_accept = errno;
    sigs_interrupt(0);

    /* Handle signals received. */
    if (sigchild_catched)
    {
      clean_childs(childs);
      sigchild_catched = false;
    }

    if (sighup_catched)
    {
      booster.api->booster_reload(booster.state);
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
    memset(&prog, 0, sizeof(prog));
    if (!invoked_get_magic(sd, &prog))
    {
      close(sd);
      continue;
    }
    if (!invoked_get_name(sd, &prog))
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
      if (setsid() < 0)
	error("setting session id\n");

      sigs_restore();

      invoked_get_actions(sd, &prog);

      close(sd);
      close(fd);
      free(prog.name);

      release_childs(childs);

      /* Invoke it. */
      if (prog.filename)
      {
        info("invoking '%s'\n", prog.filename);
        set_progname(prog.argv[0], argc, argv);

	booster.api->booster_init(prog.argv[0], booster.state);

	launch_process(&prog);
      }
      else
        error("nothing to invoke\n");

      _exit (0);
      break;

    default: /* Parent. */
      if (prog.options & INVOKER_MSG_MAGIC_OPTION_WAIT)
      {
	child_t child;

	child.pid = is_parent;
	child.sock = sd;
	child.name = prog.name;

	assign_child_slot(childs, &child);
      }
      else
	close(sd);
      break;
    }
  }

  return 0;
}

