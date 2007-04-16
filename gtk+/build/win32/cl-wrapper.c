/* cl-wrapper -- gcc-lookalike wrapper for the Microsoft C compiler
 * Copyright (C) 2001--2004 Tor Lillqvist
 *
 * This program accepts Unix-style C compiler command line arguments,
 * and runs the Microsoft C compiler (cl) with corresponding arguments.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <io.h>

static char *cmdline;
static const char *cmdline_compiler;
static const char *cmdline_args;
static const char **libraries;
static const char **libdirs;
static const char **objects;
static const char *output_executable = NULL;
static char *output_object = NULL;
static const char *executable_type = NULL;
static const char *source = NULL;
static const char *def_file = NULL;
static int lib_ix = 0;
static int libdir_ix = 0;
static int object_ix = 0;

static int compileonly = 0, debug = 0, output = 0, verbose = 0, version = 0;
static int nsources = 0;

static float cl_version;

static FILE *force_header = NULL;

#define DUMMY_C_FILE "__dummy__.c"
#define INDIRECT_CMDLINE_FILE "__cl_wrapper.at"
#define FORCE_HEADER "__force_header.h"

#if 0
static int MD_flag = 0;
static int MP_flag = 0;
static const char *MT_file = NULL;
static const char *MF_file = NULL;
#endif

/* cl warnings that should be errors */
static const int error_warnings[] = {
  4002,			        /* too many actual parameters for macro */
  4003,				/* not enough actual parameters for macro */
  4020,				/* too many actual parameters */
  4021,				/* too few actual parameters */
  4045,				/* array bounds overflow */
  0
};

/* cl warnings that should be disabled */
static const int disable_warnings[] = {
  4057,				/* indirection to slightly different base
				   types */
  4132,			        /* const object should be initialized */
  4761,				/* integral size mismatch in argument; conversion supplied */
  4996,
  0
};

/* cl warnings that should be ignored even if the gcc -Wall option is
 * used, to make cl more gcc like
 */
static const int ignore_warnings_with_Wall[] = {
  4018,				/* signed/unisgned mismatch */
  4100,				/* unreferenced formal parameter */
  4127,				/* conditional expression is constant */
  4152,				/* nonstandard extension, function/data
				   pointer conversion in expression */
  4505,				/* unreferenced local function */
  4514,				/* unreferenced inline function */
  4996,
  0
};

/* gcc links with these by default */
static const char *default_libs[] = {
  "gdi32",
  "comdlg32",
  "user32",
  "advapi32",
  "shell32",
  "oldnames",
  /* mingw has dirent functions in its mingwex library, silly,
   * but bite the bucket and link with the dirent library
   */
  "dirent",
  NULL
};

static const struct
{
  const char *gcc;
  const char *cl;
} gcc_warnings[] = {
  { "traditional", "4001" },
  { "unused-variable", "4101" },
  { "unused-parameter", "4100" },
  { "unused-label", "4102" },
  { "unused-function", "4505" },
  { "unused", "4101,4100,4102,4505" },
  { NULL, NULL }
};

static char *
backslashify (const char *string)
{
  char *result = malloc (strlen (string) + 1);
  const char *p = string;
  char *q = result;

  while (*p)
    {
      if (*p == '/')
	*q = '\\';
      else
	*q = *p;
      p++;
      q++;
    }
  *q = '\0';

  return result;
}

static char *
slashify (const char *string)
{
  char *result = malloc (strlen (string) + 1);
  const char *p = string;
  char *q = result;

  while (*p)
    {
      if (*p == '\\')
	*q = '/';
      else
	*q = *p;
      p++;
      q++;
    }
  *q = '\0';

  return result;
}

static const char *
quote (const char *string)
{
  char *result = malloc (strlen (string) * 2 + 1);
  const char *p = string;
  char *q = result;

  while (*p)
    {
      if (*p == '"')
	*q++ = '\\';
      *q++ = *p;
      p++;
    }
  *q = '\0';
  return result;
}

static void
open_force_header (void)
{
  if (force_header != NULL)
    return;

  force_header = fopen (FORCE_HEADER, "wt");

  if (force_header == NULL)
    {
      fprintf (stderr, "Could not open temporary file " FORCE_HEADER " for writing: %s\n", strerror (errno));
      exit (1);
    }
}

static void
process_argv (int argc,
	      const char **argv)
{
  int i, k;
  const char *lastdot;

  for (i = 1; i < argc; i++)
    if (strcmp (argv[i], "-c") == 0)
      {
	compileonly++;
	strcat (cmdline, " -c");
      }
    else if (strncmp (argv[i], "-D", 2) == 0)
      {
	strcat (cmdline, " ");
	strcat (cmdline, quote (argv[i]));
	if (strlen (argv[i]) == 2)
	  {
	    i++;
	    strcat (cmdline, quote (argv[i]));
	  }
      }
    else if (strncmp (argv[i], "-U", 2) == 0)
      {
	strcat (cmdline, " ");
	strcat (cmdline, argv[i]);
      }
    else if (strcmp (argv[i], "-E") == 0)
      strcat (cmdline, " -E");
    else if (strcmp (argv[i], "-g") == 0)
      debug++;
    else if (strncmp (argv[i], "-I", 2) == 0)
      {
	strcat (cmdline, " -I ");
	if (strlen (argv[i]) > 2)
	  {
	    strcat (cmdline, quote (backslashify (argv[i] + 2)));
	  }
	else
	  {
	    i++;
	    strcat (cmdline, quote (backslashify (argv[i])));
	  }
      }
    else if (strcmp (argv[i], "-idirafter") == 0)
      {
	const char *include = getenv ("INCLUDE");
	const char *afterdir = backslashify (argv[i+1]);
	char *newinclude;

	if (!include)
	  include = "";
	
	newinclude = malloc (strlen (include) + strlen (afterdir) + 20);

	strcat (newinclude, "INCLUDE=");
	if (*include)
	  {
	    strcat (newinclude, include);
	    strcat (newinclude, ";");
	  }
	strcat (newinclude, afterdir);

	_putenv (newinclude);
	i++;
      }
#if 0
    else if (strcmp (argv[i], "-MT") == 0)
      {
	i++;
	MT_file = argv[i];
      }
    else if (strcmp (argv[i], "-MD") == 0)
      MD_flag++;
    else if (strcmp (argv[i], "-MP") == 0)
      MP_flag++;
    else if (strcmp (argv[i], "-MF") == 0)
      {
	i++;
	MF_file = argv[i];
      }
#endif
    else if (strncmp (argv[i], "-l", 2) == 0)
      {
	/* Ignore -lm */
	if (strcmp (argv[i], "-lm") != 0)
	  libraries[lib_ix++] = argv[i] + 2;
      }
    else if (strncmp (argv[i], "-L", 2) == 0)
      {
	if (strlen (argv[i]) > 2)
	  {
	    libdirs[libdir_ix++] = backslashify (argv[i] + 2);
	  }
	else
	  {
	    i++;
	    libdirs[libdir_ix++] = backslashify (argv[i]);
	  }
      }
    else if (strcmp (argv[i], "-shared") == 0)
      ;				/* See -LD below */
    else if (strcmp (argv[i], "-o") == 0)
      {
	if (output)
	  {
	    fprintf (stderr, "Multiple -o options\n");
	    exit (1);
	  }

	output = 1;
	i++;
	lastdot = strrchr (argv[i], '.');
	if (lastdot != NULL && (stricmp (lastdot, ".exe") == 0
				|| stricmp (lastdot, ".dll") == 0))
	  {
	    strcat (cmdline, " -Fe");
	    strcat (cmdline, backslashify (argv[i]));
	    output_executable = argv[i];
	    executable_type = strrchr (output_executable, '.');
	  }
	else if (lastdot != NULL && (strcmp (lastdot, ".obj") == 0 ||
				     strcmp (lastdot, ".o") == 0 ))
	  {
	    strcat (cmdline, " -Fo");
	    output_object = backslashify (argv[i]);
	    strcat (cmdline, output_object);
	  }
	else
	  {
	    strcat (cmdline, " -Fe");
	    strcat (cmdline, argv[i]);
	    if (lastdot == NULL || strcmp (lastdot, ".exe") != 0)
	      strcat (cmdline, ".exe");
	  }
      }
    else if (strncmp (argv[i], "-O", 2) == 0)
      strcat (cmdline, " -O2");
    else if (strncmp (argv[i], "-mtune=", 7) == 0)
      {
	if (cl_version <= 13)
	  {
	    const char *cpu = argv[i]+7;

	    if (strcmp (cpu, "i386") == 0)
	      strcat (cmdline, " -G3");
	    else if (strcmp (cpu, "i486") == 0)
	      strcat (cmdline, " -G4");
	    else if (strcmp (cpu, "i586") == 0 ||
		     strcmp (cpu, "pentium") == 0)
	      strcat (cmdline, " -G5");
	    else if (strcmp (cpu, "i686") == 0 ||
		     strcmp (cpu, "pentiumpro") == 0 ||
		     strcmp (cpu, "pentium2") == 0 ||
		     strcmp (cpu, "pentium3") == 0 ||
		     strcmp (cpu, "pentium3") == 0 ||
		     strcmp (cpu, "pentium4") == 0)
	      strcat (cmdline, " -G6");
	    else
	      fprintf (stderr, "Ignored CPU flag %s\n", argv[i]);
	  }
      }
    else if (strcmp (argv[i], "-mno-sse3") == 0 ||
	     strcmp (argv[i], "-mno-sse") == 0)
      {
      }
    else if (strcmp (argv[i], "-mno-sse2") == 0 ||
	     strcmp (argv[i], "-msse") == 0)
      {
	if (cl_version >= 13)
	  strcat (cmdline, " -arch:SSE");
      }
    else if (strcmp (argv[i], "-msse3") == 0 ||
	     strcmp (argv[i], "-msse2") == 0)
      {
	if (cl_version >= 13)
	  strcat (cmdline, " -arch:SSE2");
      }
    else if (strcmp (argv[i], "-mms-bitfields") == 0)
      ;				/* Obviously the default... */
    else if (strncmp (argv[i], "-W", 2) == 0)
      {
	if (strlen (argv[i]) > 3 && argv[i][3] == ',')
	  fprintf (stderr, "Ignored subprocess option %s\n", argv[i]);
	else if (strcmp (argv[i], "-Wall") == 0)
	  {
	    if (cl_version >= 13)
	      strcat (cmdline, " -Wall");
	    else
	      strcat (cmdline, " -W4");
	    for (k = 0; ignore_warnings_with_Wall[k] != 0; k++)
	      fprintf (force_header,
		       "#pragma warning(disable:%d)\n",
		       ignore_warnings_with_Wall[k]);
	  }
	else if (strcmp (argv[i], "-Werror") == 0)
	  strcat (cmdline, " -WX");
	else
	  {
	    for (k = 0;
		 (gcc_warnings[k].gcc != NULL &&
		  strcmp (argv[i]+2, gcc_warnings[k].gcc) != 0);
		 k++)
	      ;
	    if (gcc_warnings[k].gcc == NULL)
	      fprintf (stderr, "Ignored warning option %s\n", argv[i]);
	    else
	      {
		const char *p = gcc_warnings[k].cl;

		while (p)
		  {
		    const char *q = strchr (p, ',');
		    if (q == NULL)
		      q = p + strlen (p);
		    if (cl_version >= 13)
		      sprintf (cmdline + strlen (cmdline),
			       " -w2%.*s", q - p, p);
		    else
		      fprintf (force_header, "#pragma warning(2:%.*s)\n",
			       q - p, p);
		    if (*q)
		      p = q+1;
		    else
		      p = NULL;
		  }
	      }
	  }
      }
    else if (strcmp (argv[i], "-w") == 0)
      strcat (cmdline, " -w");
    else if (strcmp (argv[i], "-v") == 0)
      verbose++;
    else if (strcmp (argv[i], "--version") == 0)
      version = 1;
    else if (strcmp (argv[i], "-print-prog-name=ld") == 0 ||
	     strcmp (argv[i], "--print-prog-name=ld") == 0)
      {
	printf ("link.exe\n");
	exit (0);
      }
    else if (strcmp (argv[i], "-print-search-dirs") == 0 ||
	     strcmp (argv[i], "--print-search-dirs") == 0)
      {
	printf ("libraries: %s\n", slashify (getenv ("LIB")));
	exit (0);
      }
    else if (argv[i][0] == '-')
      fprintf (stderr, "Ignored flag %s\n", argv[i]);
    else
      {
	lastdot = strrchr (argv[i], '.');
	if (lastdot != NULL && (stricmp (lastdot, ".c") == 0
				|| stricmp (lastdot, ".cpp") == 0
				|| stricmp (lastdot, ".cc") == 0))
	  {
	    nsources++;
	    strcat (cmdline, " ");
	    if (stricmp (lastdot, ".cc") == 0)
	      strcat (cmdline, "-Tp");
	    source = backslashify (argv[i]);
	    strcat (cmdline, backslashify (argv[i]));
	  }
	else if (lastdot != NULL && (stricmp (lastdot, ".obj") == 0 ||
				     stricmp (lastdot, ".o") == 0))
	  objects[object_ix++] = argv[i];
	else if (lastdot != NULL && stricmp (lastdot, ".a") == 0)
	  {
	    /* Copy .a file to .lib. Or what? Rename temporarily?
	     * Nah, too risky. A symlink would of course be ideal, but
	     * this is Windows.
	     */
	    char *libfilename = malloc (strlen (argv[i]) + 20);
	    char *copy_command = malloc (strlen (argv[i]) * 2 + 20);

	    sprintf (libfilename, "%.*s",
		     lastdot - argv[i], backslashify (argv[i]));
	    strcat (libfilename, ".temp.lib");

	    sprintf (copy_command, "copy %s %s",
		     backslashify (argv[i]), libfilename);
	    fprintf (stderr, "%s\n", copy_command);
	    if (system (copy_command) != 0)
	      {
		fprintf (stderr, "Failed\n");
		exit (1);
	      }
	    strcat (cmdline, " ");
	    strcat (cmdline, libfilename);
	  }
	else if (lastdot != NULL && stricmp (lastdot, ".def") == 0)
	  def_file = argv[i];
	else
	  fprintf (stderr, "Ignored argument: %s\n", argv[i]);
      }
}

int
main (int argc,
      char **argv)
{
  FILE *pipe;
  char buf[100];

  int retval;

  int i, j, k;
  char *p;

  libraries = malloc (argc * sizeof (char *));
  libdirs = malloc ((argc+10) * sizeof (char *));
  objects = malloc (argc * sizeof (char *));

  for (k = 0, i = 1; i < argc; i++)
    k += strlen (argv[i]);

  k += 1000 + argc;
  cmdline = malloc (k);

  pipe = _popen ("cl 2>&1", "r");
  if (pipe == NULL)
    {
      fprintf (stderr, "Could not open pipe from cl, is Microsoft Visual C installed?\n");
      exit (-1);
    }

  fgets (buf, sizeof (buf), pipe);
  p = strstr (buf, "Version ");

  if (p == NULL ||
      sscanf (p + strlen ("Version "), "%f", &cl_version) != 1)
    {
      fprintf (stderr,
	       "Could not deduce version of Microsoft compiler, "
	       "assuming 12\n");
      cl_version = 12;
    }

  /* -MD: Use msvcrt.dll runtime */
  strcpy (cmdline, "cl");

  cmdline_compiler = strdup (cmdline);
  cmdline_args = cmdline + strlen (cmdline);
  strcat (cmdline, " -MD -Zm500");

  /* Make cl more like gcc. */
  open_force_header ();
  for (k = 0; error_warnings[k] != 0; k++)
    fprintf (force_header,
	     "#pragma warning(error:%d)\n", error_warnings[k]);
  for (k = 0; disable_warnings[k] != 0; k++)
    fprintf (force_header,
	     "#pragma warning(disable:%d)\n", disable_warnings[k]);

  process_argv (argc, (const char **) argv);

  fclose (force_header);

  if (version)
    strcat (cmdline, " -c nul.c");
  else
    {
      if (nsources == 1 && output_object == NULL)
	{
	  const char *base = strrchr (source, '\\');
	  char *dot;

	  if (base == NULL)
	    base = source;

	  output_object = malloc (strlen (base) + 1);
	  strcpy (output_object, base);
	  dot = strrchr (output_object, '.');
	  strcpy (dot, ".o");

	  strcat (cmdline, " -Fo");
	  strcat (cmdline, output_object);
	}

      if (!verbose)
	strcat (cmdline, " -nologo");

      if (output_executable != NULL)
	{
	  if (stricmp (executable_type, ".dll") == 0)
	    strcat (cmdline, " -LD");
	}

      if (debug)
	strcat (cmdline, " -Zi");

      if (nsources == 0)
	{
	  FILE *dummy = fopen (DUMMY_C_FILE, "w");
	  fprintf (dummy, "static int foobar = 42;\n");
	  fclose (dummy);

	  strcat (cmdline, " " DUMMY_C_FILE);
	}

      strcat (cmdline, " -FI" FORCE_HEADER);

      if (!output && !compileonly)
	strcat (cmdline, " -Fea.exe");

      if (!compileonly)
	{
	  strcat (cmdline, " -link");

	  for (i = 0; i < object_ix; i++)
	    {
	      strcat (cmdline, " ");
	      strcat (cmdline, backslashify (objects[i]));
	    }

	  for (i = 0; i < lib_ix; i++)
	    {
	      for (j = 0; j < libdir_ix; j++)
		{
		  char b[1000];

		  sprintf (b, "%s\\%s.lib", libdirs[j], libraries[i]);
		  if (access (b, 4) == 0)
		    {
		      strcat (cmdline, " ");
		      strcat (cmdline, b);
		      break;
		    }
		  sprintf (b, "%s\\lib%s.lib", libdirs[j], libraries[i]);
		  if (access (b, 4) == 0)
		    {
		      strcat (cmdline, " ");
		      strcat (cmdline, b);
		      break;
		    }
		}

	      if (j == libdir_ix)
		{
		  strcat (cmdline, " ");
		  strcat (cmdline, libraries[i]);
		  strcat (cmdline, ".lib");
		}
	    }

	  for (k = 0; default_libs[k]; k++)
	    {
	      strcat (cmdline, " ");
	      strcat (cmdline, default_libs[k]);
	      strcat (cmdline, ".lib");
	    }
	}
    }

  fprintf (stderr, "%s\n", cmdline);

  if (strlen (cmdline) >= 200)	/* Real limit unknown */
    {
      FILE *atfile = fopen (INDIRECT_CMDLINE_FILE, "wt");
      char *indirect_cmdline = malloc (strlen (cmdline_compiler) +
				       strlen (INDIRECT_CMDLINE_FILE) + 10);

      if (atfile == NULL)
	{
	  fprintf (stderr, "Could not open %s for writing: %s\n",
		   INDIRECT_CMDLINE_FILE, strerror (errno));
	  exit (-1);
	}

      fprintf (atfile, "%s\n", cmdline_args);
      fclose (atfile);

      sprintf (indirect_cmdline, "%s @" INDIRECT_CMDLINE_FILE,
	       cmdline_compiler);
      dup2 (2, 1);
      retval = system (indirect_cmdline);
#if 0
      remove (INDIRECT_CMDLINE_FILE);
#endif
    }
  else
    {
      dup2 (2, 1);
      retval = system (cmdline);
    }

  if (nsources == 0)
    remove (DUMMY_C_FILE);

#if 0
  remove (FORCE_HEADER);
#endif

#if 0
  /* Produce a dummy make dependency file if asked to... Perhaps it
   * would be feasible to look for this info from the object file, to
   * produce a real dependency file?
   */
  if (retval == 0 && MD_flag && MF_file != NULL)
    {
      FILE *MF = fopen (MF_file, "wt");

      if (MF == NULL)
	{
	  fprintf (stderr, "Could not open %s for writing: %s\n",
		   MF_file, strerror (errno));
	  exit (-1);
	}

      if (MT_file != NULL)
	fprintf (MF, "%s ", MT_file);

      fprintf (MF, "%s: %s\n", output_object, source);

      fclose (MF);
    }
#endif

  exit (retval);
}
