/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_drawenv_h
#define ps2s_drawenv_h

/********************************************
 * includes
 */

// PLIN
//  #include <libgraph.h>
//  #include <eestruct.h>

#include "ps2s/core.h"
#include "ps2s/dmac.h"
#include "ps2s/gs.h"
#include "ps2s/packet.h"
#include "ps2s/types.h"

namespace GS {

/********************************************
    * constants
    */

static const bool kDontAddHalfPixel = false;
static const bool kAddHalfPixel     = true;

/********************************************
    * typedefs
    */

// the "using"s after each namespace { ... } are to make the type
// available from the GS namespace (ie, so you can say "GS::tAlphaBlendVal")

namespace ABlend {
    typedef enum {
        kSourceRGB   = 0,
        kDestRGB     = 1,
        kSourceAlpha = 0,
        kDestAlpha   = 1,
        kZero        = 2,
        kFix         = 2
    } tAlphaBlendVal;
}
using ABlend::tAlphaBlendVal;

namespace ATest {
    typedef enum {
        kNever,
        kAlways,
        kLess,
        kLEqual,
        kEqual,
        kGEqual,
        kGreater,
        kNotEqual
    } tAlphaTestPassMode;

    typedef enum {
        kKeep,
        kFrameBuffOnly,
        kZBuffOnly,
        kRGBOnly
    } tAlphaTestFailAction;
}
using ATest::tAlphaTestPassMode;
using ATest::tAlphaTestFailAction;

namespace ZTest {
    typedef enum {
        kNever,
        kAlways,
        kGEqual,
        kGreater
    } tZTestPassMode;
}
using ZTest::tZTestPassMode;

namespace Dither {
    typedef enum {
        kMinusFour  = 0x4,
        kMinusThree = 0x5,
        kMinusTwo   = 0x6,
        kMinusOne   = 0x7,
        kZero       = 0,
        kOne        = 1,
        kTwo        = 2,
        kThree      = 3
    } tDitherVal;
}
using Dither::tDitherVal;

/********************************************
    * class definition
    */

class CDrawEnv {
public:
    CDrawEnv(GS::tContext context);
    CDrawEnv(const CDrawEnv& rhs);
    CDrawEnv(GS::tContext context, tU32 fbW, tU32 fbH, tU32 fbWordAddr, tU32 zbufWordAddr = NULL);

    void operator=(const CDrawEnv& otherDE);

    void SetContext(GS::tContext context);

    inline void EnableAlphaTest(void) { gsrTest.atest_enable = 1; }
    inline void DisableAlphaTest(void) { gsrTest.atest_enable = 0; }
    inline void SetAlphaTestPassMode(tAlphaTestPassMode mode) { gsrTest.atest_method = (tU64)mode; }
    inline void SetAlphaTestFailAction(tAlphaTestFailAction action) { gsrTest.atest_fail_method = (tU64)action; }
    inline void SetAlphaRefVal(tU8 refVal) { gsrTest.atest_reference = refVal; }

    inline void EnableSelectiveAlphaBlend(void) { gsrPABE.enable = 1; }
    inline void DisableSelectiveAlphaBlend(void) { gsrPABE.enable = 0; }

    inline void SetAlphaBlendFunc(tAlphaBlendVal a, tAlphaBlendVal b, tAlphaBlendVal c, tAlphaBlendVal d, tU32 fix);

    inline void EnableColorClamp(void) { gsrColClamp.clamp = 1; }
    inline void DisableColorClamp(void) { gsrColClamp.clamp = 0; }

    inline void EnableDestAlphaTest(void) { gsrTest.datest_enable = 1; }
    inline void DisableDestAlphaTest(void) { gsrTest.datest_enable = 0; }
    inline void SetDestAlpha0Pass(void) { gsrTest.datest_mode = 0; }
    inline void SetDestAlpha1Pass(void) { gsrTest.datest_mode = 1; }

    inline void SetDepthTestPassMode(tZTestPassMode passMode)
    {
        gsrTest.ztest_enable = (tU64)passMode;
        eZTestPassMode       = passMode;
    }
    inline void SetDepthBufferAddr(tU32 wordAddress) { gsrZBuf.fb_addr = wordAddress / 2048; }
    inline void SetDepthBufferPSM(tU32 psm) { gsrZBuf.psm = (tU64)psm; }
    inline void EnableDepthTest(void)
    {
        gsrZBuf.update_mask = 0;
        SetDepthTestPassMode(eZTestPassMode);
    }
    inline void DisableDepthTest(void)
    {
        gsrZBuf.update_mask  = 1;
        gsrTest.ztest_enable = (tU64)ZTest::kAlways;
    }
    inline void SetDepthWriteEnabled(bool write) { gsrZBuf.update_mask = !write; }

    inline void SetFrameBufferAddr(tU32 wordAddress)
    {
        mAssert((wordAddress & 2047) == 0);
        gsrFrame.fb_addr = wordAddress / 2048;
    }
    inline void SetFrameBufferDrawMask(tU32 drawMask) { gsrFrame.draw_mask = drawMask; }
    void SetFrameBufferDim(tU32 pixelW, tU32 pixelH);
    inline void SetFrameBufferPSM(tU32 psm) { gsrFrame.psm = psm; }
    float GetInterlacedPixelOffset() const { return InterlacedOffset; }
    void SetInterlacedPixelOffset(float offset) { InterlacedOffset = offset; }
    void CalculateClippedFBXYOffsets(bool addHalfPixel);
    inline void SetXYOffsets(tU16 offsetX, tU16 offsetY)
    {
        gsrXYOffset.offset_x = offsetX << 4;
        gsrXYOffset.offset_y = offsetY << 4;
    }
    inline void SetXYOffsetsFix4(tU16 offsetX, tU16 offsetY)
    {
        gsrXYOffset.offset_x = offsetX;
        gsrXYOffset.offset_y = offsetY;
    }

    inline void SetFrameBufAlphaORMask(tU32 mask) { gsrFBA.alpha = mask; }
    inline void SetFogColor(tU32 r, tU32 g, tU32 b)
    {
        gsrFogCol.r = r;
        gsrFogCol.g = g;
        gsrFogCol.b = b;
    }

    inline void EnableDither(void) { gsrDTHE.enable = 1; }
    inline void DisableDither(void) { gsrDTHE.enable = 0; }
    inline void SetDitherMatrix(tDitherVal dm00, tDitherVal dm01, tDitherVal dm02, tDitherVal dm03,
        tDitherVal dm10, tDitherVal dm11, tDitherVal dm12, tDitherVal dm13,
        tDitherVal dm20, tDitherVal dm21, tDitherVal dm22, tDitherVal dm23,
        tDitherVal dm30, tDitherVal dm31, tDitherVal dm32, tDitherVal dm33);

    inline void SetScissorArea(tU32 scX, tU32 scY, tU32 scWidth, tU32 scHeight); // must be after SetFrameBufferDim() to have effect

    void SendSettings(bool waitForEnd = false, bool flushCache = true);
    void SendSettings(CSCDmaPacket& packet);
    void SendSettings(CVifSCDmaPacket& packet);

    // accessors
    tU32 GetFrameBufferAddr(void) { return gsrFrame.fb_addr * 2048; }

    inline void* operator new(size_t size)
    {
        return Core::New16(size);
    }

protected:
    tSourceChainTag SettingsDmaTag;
    tGifTag SettingsGifTag;
    GS::tFrame gsrFrame;
    tU64 FrameAddr;
    GS::tZbuf gsrZBuf;
    tU64 ZBufAddr;
    GS::tXyoffset gsrXYOffset;
    tU64 XYOffsetAddr;
    GS::tPrmodecont gsrPrModeCont;
    tU64 PrModeContAddr;
    GS::tColclamp gsrColClamp;
    tU64 ColClampAddr;
    GS::tTest gsrTest;
    tU64 TestAddr;
    GS::tAlpha gsrAlpha;
    tU64 AlphaAddr;
    GS::tPabe gsrPABE;
    tU64 PABEAddr;
    GS::tFba gsrFBA;
    tU64 FBAAddr;
    GS::tDthe gsrDTHE;
    tU64 DTHEAddr;
    GS::tDimx gsrDIMX;
    tU64 DIMXAddr;
    GS::tScissor gsrScissor;
    tU64 ScissorAddr;
    GS::tFogcol gsrFogCol;
    tU64 FogColAddr;
    tU32 uiNumGSRegs;

    CSCDmaPacket GifPacket;

    tZTestPassMode eZTestPassMode;
    tU32 uiFBWidth, uiFBHeight;
    float InterlacedOffset;

private:
    void InitCommon(GS::tContext context);

} __attribute__((aligned(16)));

/********************************************
    * inline methods
    */

inline void
CDrawEnv::SetAlphaBlendFunc(tAlphaBlendVal a, tAlphaBlendVal b, tAlphaBlendVal c, tAlphaBlendVal d, tU32 fix)
{
    gsrAlpha.a     = a;
    gsrAlpha.b     = b;
    gsrAlpha.c     = c;
    gsrAlpha.d     = d;
    gsrAlpha.alpha = fix;
}

inline void
CDrawEnv::SetDitherMatrix(tDitherVal dm00, tDitherVal dm01, tDitherVal dm02, tDitherVal dm03,
    tDitherVal dm10, tDitherVal dm11, tDitherVal dm12, tDitherVal dm13,
    tDitherVal dm20, tDitherVal dm21, tDitherVal dm22, tDitherVal dm23,
    tDitherVal dm30, tDitherVal dm31, tDitherVal dm32, tDitherVal dm33)
{
    gsrDIMX.DIMX00 = (tU64)dm00;
    gsrDIMX.DIMX01 = (tU64)dm01;
    gsrDIMX.DIMX02 = (tU64)dm02;
    gsrDIMX.DIMX03 = (tU64)dm03;
    gsrDIMX.DIMX10 = (tU64)dm10;
    gsrDIMX.DIMX11 = (tU64)dm11;
    gsrDIMX.DIMX12 = (tU64)dm12;
    gsrDIMX.DIMX13 = (tU64)dm13;
    gsrDIMX.DIMX20 = (tU64)dm20;
    gsrDIMX.DIMX21 = (tU64)dm21;
    gsrDIMX.DIMX22 = (tU64)dm22;
    gsrDIMX.DIMX23 = (tU64)dm23;
    gsrDIMX.DIMX30 = (tU64)dm30;
    gsrDIMX.DIMX31 = (tU64)dm31;
    gsrDIMX.DIMX32 = (tU64)dm32;
    gsrDIMX.DIMX33 = (tU64)dm33;
}

inline void
CDrawEnv::SetScissorArea(tU32 scX, tU32 scY, tU32 scWidth, tU32 scHeight)
{
    gsrScissor.clip_x0 = scX;
    gsrScissor.clip_y0 = scY;
    gsrScissor.clip_x1 = scX + scWidth - 1;
    gsrScissor.clip_y1 = scY + scHeight - 1;
}

} // namespace GS

#endif // ps2s_drawenv_h
