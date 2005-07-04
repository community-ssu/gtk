#ifdef HILDON_USE_TIMESTAMPING

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include "timer.h"


static struct timeval _timer_start;
FILE *timer_logfile;

void timer_start( char *filename )
{
	timer_logfile = fopen( filename, "w" );

	gettimeofday( &_timer_start, NULL );

	return;
}

void timer_stop(void)
{
	fclose( timer_logfile );

	return;
}

void print_timestamp( char *info ) {

	struct timeval _timer_end;
	double t1, t2, t3;

	gettimeofday( &_timer_end, NULL );

	t1 =  (double)_timer_start.tv_sec + (double)_timer_start.tv_usec/(1000*1000);
	t2 =  (double)_timer_end.tv_sec + (double)_timer_end.tv_usec/(1000*1000);
	
	t3 = t2 - t1;
	
	fprintf( timer_logfile, "%4.8f %s\n", t3, info );

	return;
}

#endif /* HILDON_USE_TIMESTAMPING */
