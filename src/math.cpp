/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include "ps2s/math.h"

namespace Math {

// This is the rand from K&R.  rand() is supposed to be portable, but..
unsigned long int next = 1;
int Rand(void)
{
    next = next * 1103515245 + 12345;
    return (unsigned int)(next / 65536) % 32768;
}

void SRand(unsigned int seed)
{
    next = (unsigned long int)seed;
}

} // namespace Math
