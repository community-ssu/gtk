#ifndef HILDON_USE_TIMESTAMPING

#define TIMER_START( filename )
#define TIMER_STOP()
#define TIMER_STOP_AND_EXIT()
#define TIMESTAMP( message )

#else

#ifndef TIMER_H
#define TIMER_H

#define TIMER_START( filename ) timer_start( filename );
#define TIMER_STOP() timer_stop()
#define TIMER_STOP_AND_EXIT() timer_stop(); return 0;
#define TIMESTAMP( message ) print_timestamp( message );

void timer_start( char * );
void timer_stop( void );
void print_timestamp( char * );

#endif /* TIMER_H */

#endif /* HILDON_USE_TIMESTAMPING */
