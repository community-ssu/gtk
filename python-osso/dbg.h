#include <stdlib.h>
#include <execinfo.h>
#include <stdio.h>

void print_trace (void);

/* Obtain a backtrace and print it to stdout. */
void
print_trace (void)
{
  void *array[100];
  size_t size;
  char **strings;
  size_t i;

  size = backtrace (array, 100);
  strings = backtrace_symbols (array, size);

  fprintf (stderr, "XXX Obtained %zd stack frames.\n", size);

  for (i = 0; i < size; i++)
     printf ("XXX    %s\n", strings[i]);

  free (strings);
}
