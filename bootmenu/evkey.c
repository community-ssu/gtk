/*
    Quick hack to get key codes and key state from kernel input subsystem

    Copyright (c) 2006 Frantisek Dufka <dufkaf@seznam.cz>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/poll.h>
#include <linux/bitops.h>
#include <linux/input.h>

static inline int
test_bit (int nr, const volatile unsigned long *p)
{
  return (p[nr >> 5] >> (nr & 31)) & 1UL;
}

static int fd;
int command;
int timeout; //miliseconds

void keywait(int down){
 struct pollfd inpollfd;
 struct input_event ev;
 int err,rb;
 int done=0;
 memset(&inpollfd,0,sizeof(inpollfd));
 
 inpollfd.fd=fd;
 inpollfd.events=POLLIN;
 while (!done){
    err=poll(&inpollfd,1,timeout);
    if (err<1) {done=1;continue;}
    rb=read(fd,&ev,sizeof(struct input_event));
    if (rb<sizeof(struct input_event))
	done=1;
    else {
	if (ev.type == EV_KEY && ev.value == down){
    	    printf("%d\n",ev.code);
	    done=1;
	}
    }
 }
}

void keydownwait(){
keywait(1);
}
void keyupwait(){
keywait(0);
}

void
keystate()
{
  unsigned char key_b[KEY_MAX / 8 + 1];
  int yalv;
  if (!fd)
    return;
  memset (key_b, 0, sizeof (key_b));
  ioctl (fd, EVIOCGKEY (sizeof (key_b)), key_b);

  for (yalv = 0; yalv < KEY_MAX; yalv++)
    {
      if (test_bit (yalv, key_b))
	{
	  /* the bit is set in the key state */
	  printf ("%d\n", yalv);
	}
    }
}

void help(){
    printf("usage: evkey -s|-u|-d -t timeout eventdevice\n");
}

int
main (int argc, char *argv[])
{

  int c;
  int morethanone=0;

  while (1)
    {
      int this_option_optind = optind ? optind : 1;
      c = getopt (argc, argv, "sdut:h");
      if (c == -1)
	break;

      switch (c)
	{
	case 's':
	case 'd':
	case 'u':
	  if (!command) command=c;
	  else
	    morethanone=c;
	  break;

	case 't':
	  timeout=atoi(optarg);
	  break;

	case '?':
	  break;

	case 'h':
	  help();
	  break;

	default:
	  printf ("?? getopt returned character code 0%o ??\n", c);
	}
    }

  if (!timeout) timeout=100;

  if (!command){
    fprintf(stderr,"one of s,u,d options is needed\n");
    exit(0);
  }
  if (morethanone){
    fprintf(stderr,"only one of s,u,d options is allowed\n");
    exit(0);
  }

  if (optind >= argc){
    fprintf(stderr,"missing input device name\n");
    exit(0);
  }
  if ((fd = open (argv[optind], O_RDONLY)) < 0)
    {
      perror ("evdev open");
    }
    switch (command){
    case 'd':
	keydownwait();
	break;
    case 'u':
	keyupwait();
	break;
    case 's':
	keystate();
	break;
    }
}
