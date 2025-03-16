/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

/********************************************
 * includes
 */

#include "ps2s/sprite.h"
#include "ps2s/packet.h"
#include "ps2s/utils.h"

/********************************************
 * methods
 */

CSprite::CSprite(GS::tContext context, uint32_t minX, uint32_t minY, uint32_t width, uint32_t height)
    : GifPacket((uint128_t*)&DrawGifTag, 6, DMAC::Channels::gif, Core::MemMappings::Normal, Packet::kFull)
{
    GS::tPrim prim;

    DrawGifTag.NLOOP = 1;
    DrawGifTag.EOP   = 1;
    DrawGifTag.PRE   = 1; // enable prim
    DrawGifTag.FLG   = 0; // packed mode
    DrawGifTag.NREG  = 5;
    DrawGifTag.REGS0 = 0x1; // rgbaq
    DrawGifTag.REGS1 = 0xf; // nop
    DrawGifTag.REGS2 = 0x4; // xyzf2
    DrawGifTag.REGS3 = 0xf; // nop
    DrawGifTag.REGS4 = 0x4; // xyzf2
    prim.prim_type   = 6;   // sprite
    prim.iip         = 1;   // flat shading
    prim.tme         = 0;   // no texture mapping
    prim.fge         = 0;   // no fog
    prim.abe         = 0;   // no alpha
    prim.aa1         = 0;   // no aa
    prim.fst         = 0;   // stq coords
    prim.ctxt        = (uint64_t)context;
    prim.fix         = 0; // what the hell does this do?
    DrawGifTag.PRIM  = *(uint64_t*)&prim;

    SetVertices(minX, minY, width, height);

    mAssert(((uint32_t)&DrawGifTag & 0xf) == 0);
}

void CSprite::SetSTQs(float minS, float minT, float width, float height)
{
    GS::tPrim prim;
    *(uint64_t*)&prim    = DrawGifTag.PRIM;
    prim.fst         = 0; // stq
    DrawGifTag.PRIM  = *(uint64_t*)&prim;
    DrawGifTag.REGS1 = 0x2;
    DrawGifTag.REGS3 = 0x2;

    TexCoords1.stq[0] = minS;
    TexCoords1.stq[1] = minT;
    TexCoords2.stq[0] = minS + width;
    TexCoords2.stq[1] = minT + height;

    SetUseTexture(true);
}

void CSprite::SetUVs(float minU, float minV, float width, float height)
{
    GS::tPrim prim;
    *(uint64_t*)&prim    = DrawGifTag.PRIM;
    prim.fst         = 1; // uv
    DrawGifTag.PRIM  = *(uint64_t*)&prim;
    DrawGifTag.REGS1 = 0x3;
    DrawGifTag.REGS3 = 0x3;

    TexCoords1.uv[0] = Core::FToI4((float)minU);
    TexCoords1.uv[1] = Core::FToI4((float)minV);
    TexCoords2.uv[0] = Core::FToI4(minU + width);
    TexCoords2.uv[1] = Core::FToI4(minV + height);

    SetUseTexture(true);
}

void CSprite::SetUVs(uint16_t minU, uint16_t minV, uint16_t width, uint16_t height)
{
    GS::tPrim prim;
    *(uint64_t*)&prim    = DrawGifTag.PRIM;
    prim.fst         = 1; // uv
    DrawGifTag.PRIM  = *(uint64_t*)&prim;
    DrawGifTag.REGS1 = 0x3;
    DrawGifTag.REGS3 = 0x3;

    TexCoords1.uv[0] = (minU << 4) + 8;
    TexCoords1.uv[1] = (minV << 4) + 8;
    TexCoords2.uv[0] = ((minU + width) << 4) + 8;
    TexCoords2.uv[1] = ((minV + height) << 4) + 8;

    SetUseTexture(true);
}

void CSprite::SetUVsFix4(uint16_t minU, uint16_t minV, uint16_t maxU, uint16_t maxV)
{
    GS::tPrim prim;
    *(uint64_t*)&prim    = DrawGifTag.PRIM;
    prim.fst         = 1; // uv
    DrawGifTag.PRIM  = *(uint64_t*)&prim;
    DrawGifTag.REGS1 = 0x3;
    DrawGifTag.REGS3 = 0x3;

    TexCoords1.uv[0] = minU;
    TexCoords1.uv[1] = minV;
    TexCoords2.uv[0] = maxU;
    TexCoords2.uv[1] = maxV;

    SetUseTexture(true);
}

void CSprite::Draw(CSCDmaPacket& packet)
{
    packet.Cnt();
    packet.Add((uint128_t*)&DrawGifTag, 6);
    packet.CloseTag();
}

void CSprite::Draw(CVifSCDmaPacket& packet)
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
        packet.Add((uint128_t*)&DrawGifTag, 6);
        packet.CloseDirect();
    }
    packet.CloseTag();
}
