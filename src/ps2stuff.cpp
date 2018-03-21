/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include "ps2s/ps2stuff.h"
#include "ps2s/gsmem.h"

void ps2sInit(void)
{
    GS::CMemArea::Init();
}

void ps2sFinish(void)
{
    GS::CMemArea::Finish();
}
