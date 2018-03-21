/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include <stdlib.h>

#include "ps2s/debug.h"
#include "ps2s/eetimer.h"

const unsigned int CEETimer::TicksPerFrame[4] = { 2500000, 156250, 9760, 262 };

CEETimer::CEETimer(tTimer whichTimer)
{
    using namespace Timers;

    switch (whichTimer) {
    case Timer0:
        pMode  = (volatile unsigned int*)Timer0::mode;
        pCount = (volatile unsigned int*)Timer0::count;
        pComp  = (volatile unsigned int*)Timer0::comp;
        pHold  = (volatile unsigned int*)Timer0::hold;
        break;
    case Timer1:
        pMode  = (volatile unsigned int*)Timer1::mode;
        pCount = (volatile unsigned int*)Timer1::count;
        pComp  = (volatile unsigned int*)Timer1::comp;
        pHold  = (volatile unsigned int*)Timer1::hold;
        break;
    case Timer2:
        pMode  = (volatile unsigned int*)Timer2::mode;
        pCount = (volatile unsigned int*)Timer2::count;
        pComp  = (volatile unsigned int*)Timer2::comp;
        pHold  = NULL;
        break;
    case Timer3:
        pMode  = (volatile unsigned int*)Timer3::mode;
        pCount = (volatile unsigned int*)Timer3::count;
        pComp  = (volatile unsigned int*)Timer3::comp;
        pHold  = NULL;
        break;
    }

    SetResolution(BusClock_256th);
    tMode.GATE = 0; // no gate
    tMode.CUE  = 0; // don't count yet
    tMode.CMPE = 0; // no interrupt
    tMode.OVFE = 0; // no interrupt

    // clear interrupt flags
    tMode.EQUF = 1;
    tMode.OVFF = 1;
    *pMode     = *((unsigned int*)&tMode);
    tMode.EQUF = 0;
    tMode.OVFF = 0;
}
