/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America
       	  
       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2stuff_kmodule_h
#define ps2stuff_kmodule_h

#include <linux/ioctl.h>

#define PS2STUFF_MAGIC		'g'

#define PS2STUFF_IOCTV1DMAK	_IO(PS2STUFF_MAGIC, 1)
#define PS2STUFF_IOCTV1DMAW	_IO(PS2STUFF_MAGIC, 2)

#define PS2STUFF_IOCQPHYSADDR	_IOR(PS2STUFF_MAGIC, 3, unsigned int)

#define PS2STUFF_IOCTMEMRESET	_IO(PS2STUFF_MAGIC, 4)

#endif // ps2stuff_kmodule_h
