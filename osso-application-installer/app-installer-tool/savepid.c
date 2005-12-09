#include <unistd.h>
#include <stdio.h>

int
main (int argc, char **argv)
{
  if (argc > 2)
    {
      FILE *f = fopen (argv[1], "w");
      if (f)
	{
	  fprintf (f, "%d\n", getpid ());
	  fclose (f);

	  execvp (argv[2], argv+2);
	  perror ("execvp");
	}
      else
	perror (argv[1]);
    }

  return 1;
}
