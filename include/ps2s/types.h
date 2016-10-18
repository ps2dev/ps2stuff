/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_types_h
#define ps2s_types_h

#  ifdef __cplusplus

typedef unsigned char		tU8;
typedef unsigned short int	tU16;
typedef unsigned int 		tU32;
#ifndef PS2_LINUX
typedef unsigned long		tU64;
#else
typedef unsigned long long	tU64;
#endif
// it doesn't really make sense to have a non-16-byte-aligned qword type...
typedef unsigned int		tU128 __attribute__ (( mode(TI), aligned(16) ));

typedef char 			t8;
typedef short int 		t16;
typedef int 			t32;
#ifndef PS2_LINUX
typedef long 			t64;
#else
typedef long long		t64;
#endif
typedef int 			t128 __attribute__ (( mode(TI), aligned(16) ));

typedef struct { tU16 whole : 12; tU16 frac : 4; } tFix12_4;

#  endif // __cplusplus

// this needs to stay outside of the namespace for C routines..
typedef int			tBool;

#endif // ps2s_types_h
