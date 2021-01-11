/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef pgl_gs_reg_types_h
#define pgl_gs_reg_types_h

#include "libgs.h"
#include "ps2s/types.h"

namespace GS {

typedef struct {
    tU64 DIMX00 : 3 __attribute__((packed));
    tU64 pad03 : 1 __attribute__((packed));
    tU64 DIMX01 : 3 __attribute__((packed));
    tU64 pad07 : 1 __attribute__((packed));
    tU64 DIMX02 : 3 __attribute__((packed));
    tU64 pad11 : 1 __attribute__((packed));
    tU64 DIMX03 : 3 __attribute__((packed));
    tU64 pad15 : 1 __attribute__((packed));
    tU64 DIMX10 : 3 __attribute__((packed));
    tU64 pad19 : 1 __attribute__((packed));
    tU64 DIMX11 : 3 __attribute__((packed));
    tU64 pad23 : 1 __attribute__((packed));
    tU64 DIMX12 : 3 __attribute__((packed));
    tU64 pad27 : 1 __attribute__((packed));
    tU64 DIMX13 : 3 __attribute__((packed));
    tU64 pad31 : 1 __attribute__((packed));
    tU64 DIMX20 : 3 __attribute__((packed));
    tU64 pad35 : 1 __attribute__((packed));
    tU64 DIMX21 : 3 __attribute__((packed));
    tU64 pad39 : 1 __attribute__((packed));
    tU64 DIMX22 : 3 __attribute__((packed));
    tU64 pad43 : 1 __attribute__((packed));
    tU64 DIMX23 : 3 __attribute__((packed));
    tU64 pad47 : 1 __attribute__((packed));
    tU64 DIMX30 : 3 __attribute__((packed));
    tU64 pad51 : 1 __attribute__((packed));
    tU64 DIMX31 : 3 __attribute__((packed));
    tU64 pad55 : 1 __attribute__((packed));
    tU64 DIMX32 : 3 __attribute__((packed));
    tU64 pad59 : 1 __attribute__((packed));
    tU64 DIMX33 : 3 __attribute__((packed));
    tU64 pad63 : 1 __attribute__((packed));
} __attribute__((packed)) tDimx ;

typedef GS_ALPHA tAlpha;
typedef GS_BITBLTBUF tBitbltbuf;
typedef GS_CLAMP tClamp;
typedef GS_COLCLAMP tColclamp;
typedef GS_DTHE tDthe;
typedef GS_FBA tFba;
typedef GS_FINISH tFinish;
typedef GS_FOG tFog;
typedef GS_FOGCOLOR tFogcol;
typedef GS_FRAME tFrame;
typedef GS_HWREG tHwreg;
typedef GS_LABEL tLabel;
typedef GS_MIPTBP1 tMiptbp1;
typedef GS_MIPTBP2 tMiptbp2;
typedef GS_PABE tPabe;
typedef GS_PRIM tPrim;
typedef GS_PRMODE tPrmode;
typedef GS_PRMODECONT tPrmodecont;
typedef GS_RGBAQ tRgbaq;
typedef GS_SCANMSK tScanmsk;
typedef GS_SCISSOR tScissor;
typedef GS_SIGNAL tSignal;
typedef GS_ST tSt;
typedef GS_TEST tTest;
typedef GS_TEX0 tTex0;
typedef GS_TEX1 tTex1;
typedef GS_TEX2 tTex2;
typedef GS_TEXA tTexa;
typedef GS_TEXCLUT tTexclut;
typedef GS_TEXFLUSH tTexflush;
typedef GS_TRXDIR tTrxdir;
typedef GS_TRXPOS tTrxpos;
typedef GS_TRXREG tTrxreg;
typedef GS_UV tUv;
typedef GS_XYOFFSET tXyoffset;
typedef GS_XYZ tXyz;
typedef GS_XYZF tXyzf;
typedef GS_ZBUF tZbuf;
}

#endif // pgl_gs_reg_types_h
