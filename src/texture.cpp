/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

/********************************************
 * includes
 */

#include <stdlib.h>

#ifndef PS2_LINUX
#include "dma.h"
#include "graph.h"
#endif

#include "ps2s/core.h"
#include "ps2s/gs.h"
#include "ps2s/imagepackets.h"
#include "ps2s/math.h"
#include "ps2s/texture.h"
#include "ps2s/utils.h"

namespace GS {

/********************************************
 * CTexEnv methods
 */

CTexEnv::CTexEnv(GS::tContext context, tU32 width, tU32 height, GS::tPSM psm)
    : uiNumSettingsGSRegs(5)
    , SettingsPacket((tU128*)&SettingsDmaTag, uiNumSettingsGSRegs + 2,
          DMAC::Channels::gif, Packet::kDontXferTags,
          Core::MemMappings::Normal, Packet::kFull)
{
    InitCommon(context);

    SetDimensions(width, height);
    SetPSM(psm);
}

CTexEnv::CTexEnv(GS::tContext context)
    : uiNumSettingsGSRegs(5)
    ,
    // gee, it's too bad c++ doesn't have a way of chaining constructors....
    SettingsPacket((tU128*)&SettingsDmaTag, uiNumSettingsGSRegs + 2,
        DMAC::Channels::gif, Packet::kDontXferTags,
        Core::MemMappings::Normal, Packet::kFull)
{
    InitCommon(context);
}

CTexEnv::CTexEnv(const CTexEnv& rhs)
    : uiNumSettingsGSRegs(5)
    ,
    // gee, it's too bad c++ doesn't have a way of chaining constructors....
    SettingsPacket((tU128*)&SettingsDmaTag, uiNumSettingsGSRegs + 2,
        DMAC::Channels::gif, Packet::kDontXferTags,
        Core::MemMappings::Normal, Packet::kFull)
{
    *this = rhs;
}

void CTexEnv::InitCommon(GS::tContext context)
{
    mAssert(context == GS::kContext1 || context == GS::kContext2);

    uiTexPixelWidth = uiTexPixelHeight = 0;

    // set up the settings xfer dma tag
    SettingsDmaTag.QWC  = uiNumSettingsGSRegs + 1; // num regs + 1 giftag
    SettingsDmaTag.PCE  = 0;
    SettingsDmaTag.ID   = DMAC::kEnd;
    SettingsDmaTag.IRQ  = 0; // no irq
    SettingsDmaTag.ADDR = 0; // no next tag
    SettingsDmaTag.SPR  = 0; // not from sp

    // setup the texturing settings gif tag
    SettingsGifTag.NLOOP = uiNumSettingsGSRegs;
    SettingsGifTag.EOP   = 1;
    SettingsGifTag.PRE   = 0;
    SettingsGifTag.FLG   = 0; // packed
    SettingsGifTag.NREG  = 1;
    SettingsGifTag.REGS0 = 0xe;

    // register addresses

    // first the addresses for the registers that are context-independent
    // flush by default
    TexflushAddr = GS::RegAddrs::texflush;
    TexAAddr     = GS::RegAddrs::texa;

    SetContext(context); // do the context-dependent stuff

    // gs register defaults

    gsrTexA.alpha_0      = 0x80; // identity
    gsrTexA.alpha_1      = 0x80; // identity
    gsrTexA.alpha_method = 0;

    // don't use SetWrapMode() here!
    // make this default to clamp because I want to issue an error when
    // SetRegion is called if the mode is repeat and the width or height is not
    // of the form (2^x -1).  If I start this off as repeat and the user wants to
    // use clamp mode then they will have to call SetWrapMode before SetRegion, which
    // isn't good.
    gsrClamp.wrap_mode_s = TexWrapMode::kClamp;
    gsrClamp.wrap_mode_t = TexWrapMode::kClamp;

    // gsrTex0.tb_addr = 0;
    // gsrTex0.tb_width = 0x3f; // invalid setting
    gsrTex0.psm = GS::kPsm32;
    // gsrTex0.tex_width = 0xf; // invalid setting
    // gsrTex0.tex_height = 0xf; // invalid setting
    gsrTex0.tex_cc      = 1; // use texture alpha
    gsrTex0.tex_funtion = TexMode::kModulate;
    //	gsrTex0.cb_addr = 0x3fff;
    //	gsrTex0.clut_pixmode = SCE_GS_PSMCT32;
    gsrTex0.clut_smode    = 0; // csm1 storage mode (don't reference texclut)
    gsrTex0.clut_offset   = 0; // talk about a weird addressing mode...  I'm not even going to mess with this now...
    gsrTex0.clut_loadmode = 4; // load clut to buffer if cbp0 != cbp

    gsrTex1.lcm = 1; // fixed LOD
    gsrTex1.mxl = 0; // max mip map level
    SetMagMode(MagMode::kLinear);
    gsrTex1.mmin = 1; // linear
    gsrTex1.mtba = 0; // use miptbp1/2
    gsrTex1.l    = 0;
    gsrTex1.k    = 0;

    // make sure things are qword aligned (I don't trust the compiler...)
    mAssert(((tU32)&SettingsGifTag & 0xf) == 0);
}

CTexEnv&
CTexEnv::operator=(const CTexEnv& rhs)
{
    Utils::MemCpy128(reinterpret_cast<tU128*>(&SettingsGifTag), reinterpret_cast<const tU128*>(&rhs.SettingsGifTag), uiNumSettingsGSRegs + 1);
    uiTexPixelHeight = rhs.uiTexPixelHeight;
    uiTexPixelWidth  = rhs.uiTexPixelWidth;
    return *this;
}

void CTexEnv::SetImageGsAddr(tU32 gsMemWordAddress)
{
    gsrTex0.tb_addr = gsMemWordAddress / 64;
}

void CTexEnv::SetClutGsAddr(tU32 gsMemWordAddress)
{
    gsrTex0.cb_addr = gsMemWordAddress / 64;
}

void CTexEnv::SetContext(GS::tContext context)
{
    // set the context-dependent registers
    if (context == GS::kContext1) {
        ClampAddr = GS::RegAddrs::clamp_1;
        Tex0Addr  = GS::RegAddrs::tex0_1;
        Tex1Addr  = GS::RegAddrs::tex1_1;
    } else {
        ClampAddr = GS::RegAddrs::clamp_2;
        Tex0Addr  = GS::RegAddrs::tex0_2;
        Tex1Addr  = GS::RegAddrs::tex1_2;
    }
}

void CTexEnv::SendSettings(bool waitForEnd, bool flushCache)
{
    SettingsPacket.Send(waitForEnd, flushCache);
}

void CTexEnv::SendSettings(CSCDmaPacket& packet)
{
    packet.Cnt();
    packet.Add((tU128*)&SettingsGifTag, uiNumSettingsGSRegs + 1);
    packet.CloseTag();
}

void CTexEnv::SendSettings(CVifSCDmaPacket& packet)
{
    packet.Cnt();
    {
        // the data needs to be qword-aligned, so pad with appropriate # of vifnops to
        // put the direct vifcode at the end of a qword
        packet.Nop();
        if (!packet.GetTTE()) {
            packet.Nop().Nop();
        }

        packet.OpenDirect();
        packet.Add((tU128*)&SettingsGifTag, uiNumSettingsGSRegs + 1);
        packet.CloseDirect();
    }
    packet.CloseTag();
}

void CTexEnv::SetDimensions(tU32 w, tU32 h)
{
    uiTexPixelWidth  = w;
    uiTexPixelHeight = h;

    // get the log base 2 of the dimensions
    tU32 logW = Math::Log2(w);
    tU32 logH = Math::Log2(h);

    // if the texture dimensions are not powers of two, they need to be rounded up
    // to the next power of two
    if (((tU32)1 << logW) != w)
        logW++;
    if (((tU32)1 << logH) != h)
        logH++;

    gsrTex0.tex_width  = logW;
    gsrTex0.tex_height = logH;

    // the width of the texture buffer should be a multiple of the page size for
    // perspective tex mapping (but what about plain old (u,v)?? -- FIXME)

    int page_width   = 64;
    unsigned int bpp = GS::GetBitsPerPixel((enum GS::tPSM)gsrTex0.psm);
    if (bpp == 8 || bpp == 4)
        page_width = 128;

    gsrTex0.tb_width = ((w + page_width - 1) / page_width) * page_width / 64;

    // if we're using bi- or tri-linear filtering the tex needs to be at least 8x8
    mAssert((gsrTex1.mmag == MagMode::kNearest) || (logW >= 3 && logH >= 3));
    mAssert((gsrTex1.mmin == MinMode::kNearest) || (gsrTex1.mmin == MinMode::kNearestMipmapNearest) || (logW >= 3 && logH >= 3));
}

// Caution about regions:  Note that in 3.4.5 "Texture Wrap Modes" REPEAT, CLAMP, and REGION_CLAMP, are
// all pretty reasonable, but REGION_REPEAT uses the bitwise operations & and | instead of the % and + operations
// that you would expect.  RTM.

void CTexEnv::SetRegion(tU32 originU, tU32 originV, tU32 w, tU32 h)
{
    tTexWrapMode sMode, tMode;
    sMode = (gsrClamp.wrap_mode_s & 2) ? (tTexWrapMode)(3 - gsrClamp.wrap_mode_s) : (tTexWrapMode)gsrClamp.wrap_mode_s;
    tMode = (gsrClamp.wrap_mode_t & 2) ? (tTexWrapMode)(3 - gsrClamp.wrap_mode_t) : (tTexWrapMode)gsrClamp.wrap_mode_t;

    // the meaning of MIN/MAX in gsrClamp differs in clamp and repeat modes

    using namespace TexWrapMode;
    if (sMode == kClamp) {
        gsrClamp.min_clamp_u = originU;
        gsrClamp.max_clamp_u = originU + w - 1;
    } else {
        gsrClamp.min_clamp_u = w - 1;
        gsrClamp.max_clamp_u = originU;
    }

    if (tMode == kClamp) {
        gsrClamp.min_clamp_v = originV;
        gsrClamp.max_clamp_v = originV + h - 1;
    } else {
        gsrClamp.min_clamp_v = h - 1;
        gsrClamp.max_clamp_v = originV;
    }

    gsrClamp.wrap_mode_s = 3 - (tU32)sMode;
    gsrClamp.wrap_mode_t = 3 - (tU32)tMode;

    // see note above this method
    mAssert((sMode == kClamp) || (((gsrClamp.max_clamp_u & gsrClamp.min_clamp_u) == 0) && Math::IsPow2(gsrClamp.min_clamp_u + 1)));
    mAssert((tMode == kClamp) || (((gsrClamp.max_clamp_v & gsrClamp.min_clamp_v) == 0) && Math::IsPow2(gsrClamp.min_clamp_v + 1)));
}

void CTexEnv::SetPSM(GS::tPSM newPSM)
{
    gsrTex0.psm = newPSM;

    switch (newPSM) {
    case kPsm8:
    case kPsm8h:
        gsrTex0.clut_pixmode = kPsm32;
        break;
    case kPsm4:
    case kPsm4hl:
    case kPsm4hh:
        gsrTex0.clut_pixmode = kPsm16;
        break;
    default:
        break;
    }
}

void CTexEnv::SetWrapModeS(tTexWrapMode sMode)
{
    using namespace TexWrapMode;
    // the meaning of MIN/MAX differs between clamp and wrap modes,
    // so if we're using a region and
    // the old mode is not the new mode we need to convert the region data
    tTexWrapMode oldMode = (gsrClamp.wrap_mode_s & 2) ? (tTexWrapMode)(3 - gsrClamp.wrap_mode_s) : (tTexWrapMode)sMode;
    if (oldMode != sMode) {
        if (oldMode == kRepeat) {
            tU32 temp            = gsrClamp.min_clamp_u;
            gsrClamp.min_clamp_u = gsrClamp.max_clamp_u;
            gsrClamp.max_clamp_u += temp;
        } else {
            tU32 temp            = gsrClamp.min_clamp_u;
            gsrClamp.min_clamp_u = gsrClamp.max_clamp_u - gsrClamp.min_clamp_u;
            gsrClamp.max_clamp_u = temp;
        }
    }
    // are we using a region?
    if (gsrClamp.wrap_mode_s & 2)
        sMode = (tTexWrapMode)(3 - sMode);

    gsrClamp.wrap_mode_s = sMode;

    // see note above SetRegion()
    mAssert((gsrClamp.wrap_mode_s <= 2) || (((gsrClamp.max_clamp_u & gsrClamp.min_clamp_u) == 0) && Math::IsPow2(gsrClamp.min_clamp_u + 1)));
}

void CTexEnv::SetWrapModeT(tTexWrapMode tMode)
{
    using namespace TexWrapMode;
    // see note in SetWrapModeU
    tTexWrapMode oldMode = (gsrClamp.wrap_mode_t & 2) ? (tTexWrapMode)(3 - gsrClamp.wrap_mode_t) : (tTexWrapMode)tMode;
    if (oldMode != tMode) {
        if (oldMode == kRepeat) {
            tU32 temp            = gsrClamp.min_clamp_v;
            gsrClamp.min_clamp_v = gsrClamp.max_clamp_v;
            gsrClamp.max_clamp_v += temp;
        } else {
            tU32 temp            = gsrClamp.min_clamp_v;
            gsrClamp.min_clamp_v = gsrClamp.max_clamp_v - gsrClamp.min_clamp_v;
            gsrClamp.max_clamp_v = temp;
        }
    }
    // are we using a region?
    if (gsrClamp.wrap_mode_t & 2)
        tMode = (tTexWrapMode)(3 - tMode);

    gsrClamp.wrap_mode_t = tMode;

    // see note above SetRegion()
    mAssert((gsrClamp.wrap_mode_t <= 2) || (((gsrClamp.max_clamp_v & gsrClamp.min_clamp_v) == 0) && Math::IsPow2(gsrClamp.min_clamp_v + 1)));
}

/********************************************
    * CTexture methods
    */

CTexture::CTexture(GS::tContext context)
    : CTexEnv(context)
    , pImageUploadPkt(NULL)
    , pClutUploadPkt(NULL)
{
    InitCommon(context);
}

CTexture::~CTexture(void)
{
    if (bFreeMemOnExit) {
        mAssert(pImage != NULL);
        free(pImage);
    }
    if (pImageUploadPkt)
        delete pImageUploadPkt;
    if (pClutUploadPkt)
        delete pClutUploadPkt;
}

void CTexture::InitCommon(GS::tContext context)
{
    pImageUploadPkt = new CImageUploadPkt;
    pImage          = NULL;
    bFreeMemOnExit  = false;
}

void CTexture::SetImageGsAddr(tU32 gsMemWordAddress)
{
    CTexEnv::SetImageGsAddr(gsMemWordAddress);
    // gsrBitBltBuf.DBP = gsMemWordAddress/64;
    pImageUploadPkt->SetGsAddr(gsMemWordAddress);
}

void CTexture::SetClutGsAddr(tU32 gsMemWordAddress)
{
    CTexEnv::SetClutGsAddr(gsMemWordAddress);
    pClutUploadPkt->SetGsAddr(gsMemWordAddress);
}

tU128*
CTexture::AllocMem(tU32 w, tU32 h, GS::tPSM psm)
{
    mAssert(pImage == NULL);

    tU128* image = (tU128*)malloc(h * w * GS::GetBitsPerPixel(psm) / 8);
    mAssert(image != NULL);
    bFreeMemOnExit = true;

    return image;
}

void CTexture::SetImage(tU128* imagePtr, tU32 w, tU32 h, GS::tPSM psm, tU32* clutPtr)
{
    // make sure the image is qword aligned
    mAssert(((tU32)imagePtr & 0xf) == 0);

    pImage = imagePtr;

    CTexEnv::SetPSM(psm);

    // dimensions
    CTexEnv::SetDimensions(w, h);

    // I think this isn't necessary anymore...
    /*
      // width of the area in gs mem (64 pixel units) to use
      tU32 gsBufWidth = ( (w % 64) != 0 ) ? w/64 + 1 : w/64;
      gsBufWidth = Math::Max( ((tU32)1 << gsrTex0.tex_width) / 64, gsBufWidth );
      gsrTex0.tb_width = gsBufWidth;
      */

    unsigned int gsBufWidth = gsrTex0.tb_width * 64;
    pImageUploadPkt->SetGsBufferWidth(gsBufWidth);
    // width and height of the region to xfer
    pImageUploadPkt->SetImage(imagePtr, w, h, psm);

    // clut
    pClut = (tU128*)clutPtr;
    if (clutPtr != NULL) {
        mAssert(pClutUploadPkt == NULL);
        pClutUploadPkt = new CClutUploadPkt;
        pClutUploadPkt->SetClut(clutPtr);
    }
}

void CTexture::Reset()
{
    if (bFreeMemOnExit)
        free(pImage);
    bFreeMemOnExit = false;
    pImage = pClut = NULL;
    if (pImageUploadPkt)
        pImageUploadPkt->Reset();
    if (pClutUploadPkt)
        pClutUploadPkt->Reset();
}

void CTexture::SendImage(bool waitForEnd, bool flushCache)
{
    mAssert(pImage != NULL);
    pImageUploadPkt->Send(waitForEnd, flushCache);
}

void CTexture::SendImage(CSCDmaPacket& packet)
{
    pImageUploadPkt->Send(packet);
}

void CTexture::SendImage(CVifSCDmaPacket& packet)
{
    pImageUploadPkt->Send(packet);
}

void CTexture::SendClut(bool waitForEnd, bool flushCache)
{
    mAssert(pClut != NULL);

    pClutUploadPkt->Send(waitForEnd, flushCache);
}

void CTexture::SendClut(CSCDmaPacket& packet)
{
    pClutUploadPkt->Send(packet);
}

void CTexture::SendClut(CVifSCDmaPacket& packet)
{
    pClutUploadPkt->Send(packet);
}

/********************************************
    * CClut methods
    */

CClut::CClut(const void* table, int numEntries)
    : GsAddr(0)
{
    UploadPkt = new CClutUploadPkt;
    UploadPkt->SetClut((tU32*)table);
}

/********************************************
    * CCheckTex methods
    */

CCheckTex::CCheckTex(GS::tContext context, tU32 width, tU32 height, tU32 xCellSize, tU32 yCellSize, tU32 color1, tU32 color2)
    : CTexture(context)
{
    tU128* image = AllocMem(width, height, GS::kPsm32);
    SetImage(image, width, height, GS::kPsm32);

    MakeCheckerboard(xCellSize, yCellSize, color1, color2);
}

void CCheckTex::MakeCheckerboard(tU32 xCellSize, tU32 yCellSize, tU32 color1, tU32 color2)
{
    tU32* curPixel = (tU32*)pImage;
    tU32 row, col;
    for (row = 0; row < uiTexPixelHeight; row++) {
        for (col = 0; col < uiTexPixelWidth; col++)
            *curPixel++ = (Math::IsEven(row / yCellSize) ^ Math::IsEven(col / xCellSize)) ? color1 : color2;
    }
}

} // namespace GS
