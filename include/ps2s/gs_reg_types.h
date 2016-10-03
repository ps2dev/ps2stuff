/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America
       	  
       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef pgl_gs_reg_types_h
#define pgl_gs_reg_types_h

#include "ps2s/types.h"

namespace GS {

typedef struct {
	tU64 DIMX00:  3 __attribute__((packed));
	tU64 pad03: 1 __attribute__((packed));
	tU64 DIMX01:  3 __attribute__((packed));
	tU64 pad07: 1 __attribute__((packed));
	tU64 DIMX02:  3 __attribute__((packed));
	tU64 pad11: 1 __attribute__((packed));
	tU64 DIMX03:  3 __attribute__((packed));
	tU64 pad15: 1 __attribute__((packed));
	tU64 DIMX10:  3 __attribute__((packed));
	tU64 pad19: 1 __attribute__((packed));
	tU64 DIMX11:  3 __attribute__((packed));
	tU64 pad23: 1 __attribute__((packed));
	tU64 DIMX12:  3 __attribute__((packed));
	tU64 pad27: 1 __attribute__((packed));
	tU64 DIMX13:  3 __attribute__((packed));
	tU64 pad31: 1 __attribute__((packed));
	tU64 DIMX20:  3 __attribute__((packed));
	tU64 pad35: 1 __attribute__((packed));
	tU64 DIMX21:  3 __attribute__((packed));
	tU64 pad39: 1 __attribute__((packed));
	tU64 DIMX22:  3 __attribute__((packed));
	tU64 pad43: 1 __attribute__((packed));
	tU64 DIMX23:  3 __attribute__((packed));
	tU64 pad47: 1 __attribute__((packed));
	tU64 DIMX30:  3 __attribute__((packed));
	tU64 pad51: 1 __attribute__((packed));
	tU64 DIMX31:  3 __attribute__((packed));
	tU64 pad55: 1 __attribute__((packed));
	tU64 DIMX32:  3 __attribute__((packed));
	tU64 pad59: 1 __attribute__((packed));
	tU64 DIMX33:  3 __attribute__((packed));
	tU64 pad63: 1 __attribute__((packed));
} tDimx __attribute__((packed));

#ifndef PS2_LINUX
#  include "eetypes.h"
#  include "eestruct.h"

   typedef sceGsAlpha		tAlpha;
   typedef sceGsBitbltbuf	tBitbltbuf;
   typedef sceGsClamp		tClamp;
   typedef sceGsColclamp	tColclamp;
   typedef sceGsDthe		tDthe;
   typedef sceGsFba		tFba;
   typedef sceGsFinish		tFinish;
   typedef sceGsFog		tFog;
   typedef sceGsFogcol		tFogcol;
   typedef sceGsFrame		tFrame;
   typedef sceGsHwreg		tHwreg;
   typedef sceGsLabel		tLabel;
   typedef sceGsMiptbp1		tMiptbp1;
   typedef sceGsMiptbp2		tMiptbp2;
   typedef sceGsPabe		tPabe;
   typedef sceGsPrim		tPrim;
   typedef sceGsPrmode		tPrmode;
   typedef sceGsPrmodecont	tPrmodecont;
   typedef sceGsRgbaq		tRgbaq;
   typedef sceGsScanmsk		tScanmsk;
   typedef sceGsScissor		tScissor;
   typedef sceGsSignal		tSignal;
   typedef sceGsSt		tSt;
   typedef sceGsTest		tTest;
   typedef sceGsTex0		tTex0;
   typedef sceGsTex1		tTex1;
   typedef sceGsTex2		tTex2;
   typedef sceGsTexa		tTexa;
   typedef sceGsTexclut		tTexclut;
   typedef sceGsTexflush	tTexflush;
   typedef sceGsTrxdir		tTrxdir;
   typedef sceGsTrxpos		tTrxpos;
   typedef sceGsTrxreg		tTrxreg;
   typedef sceGsUv		tUv;
   typedef sceGsXyoffset	tXyoffset;
   typedef sceGsXyz		tXyz;
   typedef sceGsXyzf		tXyzf;
   typedef sceGsZbuf		tZbuf;
#else
#  include <linux/ps2/gs.h>

   typedef ps2_gsreg_alpha	tAlpha;
   typedef ps2_gsreg_bitbltbuf	tBitbltbuf;
   typedef ps2_gsreg_clamp	tClamp;
   typedef ps2_gsreg_colclamp	tColclamp;
   typedef ps2_gsreg_dthe	tDthe;
   typedef ps2_gsreg_fba	tFba;
   typedef ps2_gsreg_finish	tFinish;
   typedef ps2_gsreg_fog	tFog;
   typedef ps2_gsreg_fogcol	tFogcol;
   typedef ps2_gsreg_frame	tFrame;
   typedef ps2_gsreg_hwreg	tHwreg;
   typedef ps2_gsreg_label	tLabel;
   typedef ps2_gsreg_miptbp1	tMiptbp1;
   typedef ps2_gsreg_miptbp2	tMiptbp2;
   typedef ps2_gsreg_pabe	tPabe;
   typedef ps2_gsreg_prim	tPrim;
   typedef ps2_gsreg_prmode	tPrmode;
   typedef ps2_gsreg_prmodecont	tPrmodecont;
   typedef ps2_gsreg_rgbaq	tRgbaq;
   typedef ps2_gsreg_scanmsk	tScanmsk;
   typedef ps2_gsreg_scissor	tScissor;
   typedef ps2_gsreg_signal	tSignal;
   typedef ps2_gsreg_st		tSt;
   typedef ps2_gsreg_test	tTest;
   typedef ps2_gsreg_tex0	tTex0;
   typedef ps2_gsreg_tex1	tTex1;
   typedef ps2_gsreg_tex2	tTex2;
   typedef ps2_gsreg_texa	tTexa;
   typedef ps2_gsreg_texclut	tTexclut;
   typedef ps2_gsreg_texflush	tTexflush;
   typedef ps2_gsreg_trxdir	tTrxdir;
   typedef ps2_gsreg_trxpos	tTrxpos;
   typedef ps2_gsreg_trxreg	tTrxreg;
   typedef ps2_gsreg_uv		tUv;
   typedef ps2_gsreg_xyoffset	tXyoffset;
   typedef ps2_gsreg_xyz	tXyz;
   typedef ps2_gsreg_xyzf	tXyzf;
   typedef ps2_gsreg_zbuf	tZbuf;
#endif
}

#endif // pgl_gs_reg_types_h
