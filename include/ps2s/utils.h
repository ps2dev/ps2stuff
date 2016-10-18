/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_utils_h
#define ps2s_utils_h

#include <stdio.h>
#include "ps2s/debug.h"
#include "ps2s/types.h"

/********************************************
	 * Utils
	 */

namespace Utils
{
	inline void MemCpy128( tU128* dest, const tU128* src, tU32 numQwords );

	inline void QwordHexDump( tU32* mem, tU32 numQwords );
	inline void QwordDecDump( tU32* mem, tU32 numQwords );
	inline void QwordFloatDump( float* mem, tU32 numQwords );
}


/********************************************
 * Utils inlines
 */

inline void
Utils::MemCpy128( tU128* dest, const tU128* src, tU32 numQwords ) {
	mAssert( ((tU32)dest & 0xf) == 0 && ((tU32)src & 0xf) == 0 );
   while( numQwords-- > 0 ) *(dest++) = *(src++);
}

inline void
Utils::QwordHexDump( tU32* mem, tU32 numQwords ) {
   tU32 i;
   for ( i = 0; i < numQwords; i++, mem += 4 )
      printf("%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n", (tU32)mem, mem[0], mem[1], mem[2], mem[3] );
}

inline void
Utils::QwordDecDump( tU32* mem, tU32 numQwords ) {
   tU32 i;
   for ( i = 0; i < numQwords; i++, mem += 4 )
      printf("%08x: %d %d %d %d\n", (tU32)mem, mem[0], mem[1], mem[2], mem[3] );
}

inline void
Utils::QwordFloatDump( float* mem, tU32 numQwords ) {
   tU32 i;
   for ( i = 0; i < numQwords; i++, mem += 4 )
      printf("%08x: %f %f %f %f\n", (tU32)mem, mem[0], mem[1], mem[2], mem[3] );
}

#endif // ps2s_utils_h
