/*
 * $Id$
 *
 * Copyright (C) 2005, 2006 Nokia
 *
 * Author: Guillem Jover <guillem.jover@nokia.com>
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
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "config.h"
#include "report.h"
#include "invokelib.h"

#define DEFAULT_DELAY 0

static bool
invoke_recv_ack(int fd)
{
  uint32_t msg;

  /* Revceive ACK. */
  invoke_recv_msg(fd, &msg);

  if (msg != INVOKER_MSG_ACK)
    die(1, "receiving wrong ack (%08x)\n", msg);
  else
    return true;
}

static int
invoker_init(unsigned int delay)
{
  int fd;
  int options = 0;
  struct sockaddr_un sun;

  fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (fd < 0)
    die(1, "opening invoker socket\n");

  sun.sun_family = AF_UNIX;
  strcpy(sun.sun_path, INVOKER_SOCK);

  if (connect(fd, (struct sockaddr *)&sun, sizeof(sun)) < 0)
    die(1, "connecting to the launcher\n");

  if (!delay)
    options |= INVOKER_MSG_MAGIC_OPTION_WAIT;

  /* Send magic. */
  invoke_send_msg(fd, INVOKER_MSG_MAGIC | INVOKER_MSG_MAGIC_VERSION | options);
  invoke_recv_ack(fd);

  return fd;
}

static bool
invoker_send_exec(int fd, char *exec)
{
  /* Send action. */
  invoke_send_msg(fd, INVOKER_MSG_EXEC);
  invoke_send_str(fd, exec);

  invoke_recv_ack(fd);

  return true;
}

static bool
invoker_send_args(int fd, int argc, char **argv)
{
  int i;

  /* Send action. */
  invoke_send_msg(fd, INVOKER_MSG_ARGS);
  invoke_send_msg(fd, argc);
  for (i = 0; i < argc; i++)
    invoke_send_str(fd, argv[i]);

  invoke_recv_ack(fd);

  return true;
}

static bool
invoker_send_end(int fd)
{
  /* Send action. */
  invoke_send_msg(fd, INVOKER_MSG_END);

  invoke_recv_ack(fd);

  return true;
}

static int
invoker_recv_exit(int fd)
{
  uint32_t msg;

  /* Receive action. */
  invoke_recv_msg(fd, &msg);

  if (msg != INVOKER_MSG_EXIT)
    die(1, "receiving bad exit status (%08x)\n", msg);

  /* Receive status. */
  invoke_recv_msg(fd, &msg);

  return msg;
}

static uint32_t
get_file_int(const char *filename)
{
  FILE *f;
  uint32_t u;

  f = fopen(filename, "r");
  if (f == NULL)
    return 0;

  if (fscanf(f, "%u", &u) < 1)
    u = 0;

  fclose(f);

  return u;
}

/*
 * This function is Linux specific and depends on the lowmem LSM kernel module,
 * but should behave as a nop on systems not supporting it.
 */
static unsigned int
get_linux_lowmem_modifier(void)
{
  uint32_t mem_allowed;
  uint32_t mem_used;
  const unsigned int lowmem_threshold = 80; /* Memory use percentage. */
  int modifier;

  mem_allowed = get_file_int("/proc/sys/vm/lowmem_allowed_pages");
  mem_used = get_file_int("/proc/sys/vm/lowmem_used_pages");

  if (mem_used && mem_allowed)
  {
    modifier = ((mem_used * 100) / mem_allowed) - lowmem_threshold;
    if (modifier < 0)
      modifier = 0;
  }
  else
    modifier = 0;

  return modifier + 1;
}

static unsigned int
get_delay(char *delay_arg)
{
  char *delay_str;
  unsigned int delay;

  if (delay_arg)
    delay_str = delay_arg;
  else
  {
    delay_str = getenv("MAEMO_INVOKER_DELAY");
    /* XXX: Backward compatibility. */
    if (!delay_str)
      delay_str = getenv("MAEMO_INVOKER_TIMEOUT");
  }

  if (delay_str)
    delay = strtoul(delay_str, NULL, 10);
  else
    delay = DEFAULT_DELAY;

  return delay;
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
  printf("Usage: %s [options] file [file-options]\n"
	 "  --delay SECS        After invoking sleep for SECS seconds (default %d).\n"
	 "  --version           Print program version.\n"
	 "  --help              Print this help message.\n",
	 PROG_NAME, DEFAULT_DELAY);

  exit(status);
}

int
main(int argc, char *argv[])
{
  int i;
  int fd;
  int prog_argc = 0;
  char **prog_argv = NULL;
  char *prog_name = NULL;
  int prog_ret = 0;
  char *launch = NULL;
  char *delay_str = NULL;
  unsigned int delay;

  if (strstr(argv[0], PROG_NAME))
  {
    /* Called with our default name. Parse arguments. */
    for (i = 1; i < argc; ++i)
    {
      if (strcmp(argv[i], "--delay") == 0)
      {
	if (argv[++i])
	  delay_str = argv[i];
      }
      else if (strcmp(argv[i], "--version") == 0)
	version();
      else if (strcmp(argv[i], "--help") == 0)
	usage(0);
      else if (strncmp(argv[i], "--", 2) == 0)
	usage(1);
      else
      {
	prog_name = argv[i];
	prog_argc = argc - i;
	prog_argv = &argv[i];
	break;
      }
    }
  }
  else
  {
    /* Called with a different name. Add the proper extension and launch it.
     * Do not try to parse any arguments. */
    asprintf(&launch, "%s.launch", argv[0]);
    prog_name = launch;
    prog_argc = argc;
    prog_argv = argv;

    report_set_output(report_syslog);
  }

  if (!prog_name)
  {
    error("nothing to invoke\n");
    usage(0);
  }

  debug("invoking execution: '%s'\n", prog_name);

  delay = get_delay(delay_str);

  fd = invoker_init(delay);
  invoker_send_exec(fd, prog_name);
  invoker_send_args(fd, prog_argc, prog_argv);
  invoker_send_end(fd);

  if (launch)
    free(launch);

  if (delay)
  {
    /* DBUS cannot cope some times if the invoker exits too early. */
    delay *= get_linux_lowmem_modifier();
    debug("delaying exit for %d seconds\n", delay);
    sleep(delay);
  }
  else
  {
    debug("waiting for invoked program to exit\n");
    prog_ret = invoker_recv_exit(fd);
  }

  close(fd);

  return prog_ret;
}

