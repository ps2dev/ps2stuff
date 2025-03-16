/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

/********************************************
 * includes
 */

#include <stdio.h>

#include "dma.h"

#include "ps2s/core.h"
#include "ps2s/drawenv.h"
#include "ps2s/utils.h"

namespace GS {

/********************************************
 * code
 */

CDrawEnv::CDrawEnv(GS::tContext context)
    : uiNumGSRegs(13)
    , GifPacket((uint128_t*)&SettingsDmaTag, uiNumGSRegs + 2, DMAC::Channels::gif, Packet::kDontXferTags, Core::MemMappings::Normal, Packet::kFull)
{
    InitCommon(context);
}

CDrawEnv::CDrawEnv(const CDrawEnv& rhs)
    : uiNumGSRegs(13)
    , GifPacket((uint128_t*)&SettingsDmaTag, uiNumGSRegs + 2, DMAC::Channels::gif, Packet::kDontXferTags, Core::MemMappings::Normal, Packet::kFull)
{
    *this = rhs;
}

CDrawEnv::CDrawEnv(GS::tContext context, uint32_t fbW, uint32_t fbH, uint32_t fbWordAddr, uint32_t zbufWordAddr)
    : uiNumGSRegs(13)
    , GifPacket((uint128_t*)&SettingsDmaTag, uiNumGSRegs + 2, DMAC::Channels::gif, Core::MemMappings::Normal, Packet::kFull)
{
    InitCommon(context);

    SetFrameBufferAddr(fbWordAddr);
    SetFrameBufferDim(fbW, fbH);

    if (zbufWordAddr)
        SetDepthBufferAddr(zbufWordAddr);
    else
        DisableDepthTest();
}

void CDrawEnv::SetContext(GS::tContext context)
{
    // now set addresses for context-dependent registers
    if (context == GS::kContext1) {
        FrameAddr    = GS::RegAddrs::frame_1;
        ZBufAddr     = GS::RegAddrs::zbuf_1;
        XYOffsetAddr = GS::RegAddrs::xyoffset_1;
        TestAddr     = GS::RegAddrs::test_1;
        AlphaAddr    = GS::RegAddrs::alpha_1;
        FBAAddr      = GS::RegAddrs::fba_1;
        ScissorAddr  = GS::RegAddrs::scissor_1;
    } else {
        FrameAddr    = GS::RegAddrs::frame_2;
        ZBufAddr     = GS::RegAddrs::zbuf_2;
        XYOffsetAddr = GS::RegAddrs::xyoffset_2;
        TestAddr     = GS::RegAddrs::test_2;
        AlphaAddr    = GS::RegAddrs::alpha_2;
        FBAAddr      = GS::RegAddrs::fba_2;
        ScissorAddr  = GS::RegAddrs::scissor_2;
    }
}

void CDrawEnv::InitCommon(GS::tContext context)
{
    // set up the dma end tag
    SettingsDmaTag.QWC  = uiNumGSRegs + 1; // num regs + 1 giftag
    SettingsDmaTag.PCE  = 0;
    SettingsDmaTag.ID   = DMAC::kEnd;
    SettingsDmaTag.IRQ  = 0; // no irq
    SettingsDmaTag.ADDR = 0; // no next tag
    SettingsDmaTag.SPR  = 0; // not from sp

    // set up the gif tag
    // zero it first

    SettingsGifTag.NLOOP = uiNumGSRegs;
    SettingsGifTag.EOP   = 1;
    SettingsGifTag.PRE   = 0;
    SettingsGifTag.FLG   = 0;
    SettingsGifTag.NREG  = 1;
    SettingsGifTag.REGS0 = 0xe;

    eZTestPassMode   = ZTest::kGEqual;
    uiFBWidth        = 0;
    uiFBHeight       = 0;
    InterlacedOffset = 0.5f;

    // first set addresses for the registers that are context-independent
    PrModeContAddr = GS::RegAddrs::prmodecont;
    ColClampAddr   = GS::RegAddrs::colclamp;
    PABEAddr       = GS::RegAddrs::pabe;
    DTHEAddr       = GS::RegAddrs::dthe;
    DIMXAddr       = GS::RegAddrs::dimx;
    FogColAddr     = GS::RegAddrs::fogcol;

    SetContext(context);

    gsrPrModeCont.control = 1; // use PRIM register
    EnableColorClamp();
    DisableSelectiveAlphaBlend();
    SetFrameBufAlphaORMask(0);
    DisableDither();
    using namespace GS::Dither;
    SetDitherMatrix(kMinusFour, kTwo, kMinusThree, kThree,
        kZero, kMinusTwo, kOne, kMinusOne,
        kMinusThree, kThree, kMinusFour, kTwo,
        kOne, kMinusOne, kZero, kMinusTwo);
    SetFogColor(0, 0, 0);

    // gsrFrame.FBP = 0x0; // just to have something...
    // gsrFrame.FBW = 0; // invalid
    SetFrameBufferPSM(GS::kPsm32);
    SetFrameBufferDrawMask(0); // enable write to frame buffer

    gsrZBuf.fb_addr     = 0; // after two 640x224 frame buffers
    gsrZBuf.psm         = 0; // KPSMZ32
    gsrZBuf.update_mask = 0; // enable write to z-buffer

    gsrXYOffset.offset_x = 0; // probably won't make a lot of sense later...
    gsrXYOffset.offset_y = 0;

    gsrScissor.clip_x0 = 0;
    gsrScissor.clip_y0 = 0;
    gsrScissor.clip_x1 = 0x7ff;
    gsrScissor.clip_y1 = 0x7ff;

    gsrTest.atest_enable      = 0;    // disable alpha test
    gsrTest.atest_method      = 0;    // never
    gsrTest.atest_reference   = 0x80; // identity
    gsrTest.atest_fail_method = 0;    // keep - ?
    gsrTest.datest_enable     = 0;    // disable destination alpha test
    gsrTest.datest_mode       = 0;    // 0 = pass
    gsrTest.ztest_enable      = 1;    // enable z-test
    gsrTest.ztest_method      = (uint64_t)eZTestPassMode;

    // make sure things are qword aligned (I don't trust the compiler...)
    mAssert(((uint32_t)&SettingsGifTag & 0xf) == 0);
}

void CDrawEnv::CalculateClippedFBXYOffsets(bool addHalfPixel)
{
    float gsCenterX, gsCenterY;
    gsCenterX = gsCenterY = 4096.0f / 2.0f;

    float gsOffsetX, gsOffsetY;
    gsOffsetX = gsCenterX - (float)(uiFBWidth / 2);
    gsOffsetY = gsCenterY - (float)(uiFBHeight / 2);

    if (addHalfPixel)
        gsOffsetY += InterlacedOffset;

    gsrXYOffset.offset_x = (uint16_t)Core::FToI4(gsOffsetX);
    gsrXYOffset.offset_y = (uint16_t)Core::FToI4(gsOffsetY);
}

void CDrawEnv::SendSettings(bool waitForEnd, bool flushCache)
{
    GifPacket.Send(waitForEnd, flushCache);
}

void CDrawEnv::SendSettings(CSCDmaPacket& packet)
{
    bool opened_tag;
    if ((opened_tag = !packet.HasOpenTag()))
        packet.Cnt();
    packet.Add(&SettingsGifTag, uiNumGSRegs + 1);
    if (opened_tag)
        packet.CloseTag();
}

void CDrawEnv::SendSettings(CVifSCDmaPacket& packet)
{
    // the data needs to be qword-aligned, so pad with appropriate # of vifnops to
    // put the direct vifcode at the end of a qword
    bool opened_tag = false;
    if (packet.HasOpenTag()) {
        // assume that the packet is aligned to 128 bits
        packet.Nop().Nop().Nop();
    } else {
        opened_tag = true;
        packet.Cnt();
        packet.Nop();
        if (!packet.GetTTE()) {
            packet.Nop().Nop();
        }
    }

    packet.OpenDirect();
    packet.Add(&SettingsGifTag, uiNumGSRegs + 1);
    packet.CloseDirect();

    if (opened_tag)
        packet.CloseTag();
}

void CDrawEnv::SetFrameBufferDim(uint32_t pixelW, uint32_t pixelH)
{
    // width must be a multiple of 64
    if (pixelW < 64)
        pixelW = 64;
    mAssert((pixelW & 63) == 0);

    gsrFrame.fb_width = pixelW / 64;
    uiFBWidth         = pixelW;
    uiFBHeight        = pixelH;

    gsrScissor.clip_x0 = 0;
    gsrScissor.clip_y0 = 0;
    gsrScissor.clip_x1 = pixelW - 1;
    gsrScissor.clip_y1 = pixelH - 1;
}

void CDrawEnv::operator=(const CDrawEnv& otherDE)
{
    Utils::MemCpy128((uint128_t*)&SettingsGifTag, (uint128_t*)&otherDE.SettingsGifTag, uiNumGSRegs + 2);
    eZTestPassMode = otherDE.eZTestPassMode;
    uiFBWidth      = otherDE.uiFBWidth;
    uiFBHeight     = otherDE.uiFBHeight;
}

} // namespace GS
