/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

#   include <unistd.h>
#   include <sys/stat.h>
#   include <fcntl.h>

#include "ps2stuff_kmodule.h"

int
main( int argc, char **argv )
{
   int fd = -1;
   unsigned int phys_addr = 0;
  void *mapped_addr;

  fd = open( "/dev/ps2stuff", O_RDWR );

  if ( fd < 0 ) {
    fprintf( stderr, "Couldn't open ps2stuff: %s\n", strerror(errno) );
    exit(-1);
  }
  printf("successfully opened /dev/ps2stuff\n");

  printf("trying to get some dma memory...");
  mapped_addr = mmap(0, 16 * 1024 - 1,
		     PROT_READ | PROT_WRITE,
		     MAP_SHARED, fd,
		     0 );
  if ( (int)mapped_addr < 0 ) {
    printf("failed\n");
    fprintf( stderr, "mmap error: %s\n", strerror(errno) );
  }
  else {
    printf("got it!\n");
  }

  printf("trying to translate virtual address to physical...");
  phys_addr = ioctl( fd, PS2STUFF_IOCQPHYSADDR, mapped_addr );
  printf("0x%08x -> 0x%08x\n", mapped_addr, phys_addr );

  close(fd);
  return 0;
}
