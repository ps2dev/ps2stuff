/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include <stdlib.h>

#include "ps2s/eetimer.h"
#include "ps2s/debug.h"

#ifdef PS2_LINUX
#  include <unistd.h>
#  include <sys/mman.h>
// this will have been defined by glut or the application when opening the
// ps2stuff device
extern int Ps2stuffDeviceFd;
#endif

const unsigned int CEETimer::TicksPerFrame[4];

CEETimer::CEETimer( tTimer whichTimer )
{
   using namespace Timers;

#ifndef PS2_LINUX
   switch (whichTimer) {
      case Timer0:
	 pMode = (volatile unsigned int*)Timer0::mode;
	 pCount = (volatile unsigned int*)Timer0::count;
	 pComp = (volatile unsigned int*)Timer0::comp;
	 pHold = (volatile unsigned int*)Timer0::hold;
	 break;
      case Timer1:
	 pMode = (volatile unsigned int*)Timer1::mode;
	 pCount = (volatile unsigned int*)Timer1::count;
	 pComp = (volatile unsigned int*)Timer1::comp;
	 pHold = (volatile unsigned int*)Timer1::hold;
	 break;
      case Timer2:
	 pMode = (volatile unsigned int*)Timer2::mode;
	 pCount = (volatile unsigned int*)Timer2::count;
	 pComp = (volatile unsigned int*)Timer2::comp;
	 pHold = NULL;
	 break;
      case Timer3:
	 pMode = (volatile unsigned int*)Timer3::mode;
	 pCount = (volatile unsigned int*)Timer3::count;
	 pComp = (volatile unsigned int*)Timer3::comp;
	 pHold = NULL;
	 break;
   }
#else
   static bool initted = false;

   static unsigned int timer_base = 0;

   if ( ! initted ) {
     initted = true;
     mErrorIf( Ps2stuffDeviceFd < 0,
	       "The timer classes cannot be instantiated before the ps2stuff device is opened" );
     timer_base = (unsigned int)mmap(0, 4096 * 2,
				     PROT_READ | PROT_WRITE,
				     MAP_SHARED, Ps2stuffDeviceFd,
				     2 );
     mErrorIf( timer_base == 0,
	       "Failed to map timers!" );
   }

   unsigned int new_base = timer_base;
   switch (whichTimer) {
      case Timer0:
	new_base += (unsigned int)Timer0::mode & 0xff00;
	 break;
      case Timer1:
	new_base += (unsigned int)Timer1::mode & 0xff00;
	 break;
      case Timer2:
	new_base += (unsigned int)Timer2::mode & 0xff00;
	 break;
      case Timer3:
	new_base += (unsigned int)Timer3::mode & 0xff00;
	 break;
      default:
	 mError("Not supported.");
	 break;
   }

   pMode = (volatile unsigned int*)(new_base + 0x10);
   pCount = (volatile unsigned int*)(new_base + 0x00);
   pComp = (volatile unsigned int*)(new_base + 0x20);
   pHold = NULL;
#endif

   SetResolution( BusClock_256th );
   tMode.GATE = 0; // no gate
   tMode.CUE = 0; // don't count yet
   tMode.CMPE = 0; // no interrupt
   tMode.OVFE = 0; // no interrupt

   // clear interrupt flags
   tMode.EQUF = 1;
   tMode.OVFF = 1;
   *pMode = *((unsigned int*)&tMode);
   tMode.EQUF = 0;
   tMode.OVFF = 0;
}
