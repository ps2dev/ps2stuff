/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

/********************************************
 * includes
 */

#include "ps2s/utils.h"
#include "ps2s/packet.h"
#include "ps2s/sprite.h"

/********************************************
 * methods
 */

CSprite::CSprite( GS::tContext context, tU32 minX, tU32 minY, tU32 width, tU32 height )
	: GifPacket( (tU128*)&DrawGifTag, 6, DMAC::Channels::gif, Core::MemMappings::Normal, Packet::kFull )
{
   GS::tPrim prim;

   DrawGifTag.NLOOP = 1;
   DrawGifTag.EOP = 1;
   DrawGifTag.PRE = 1; // enable prim
   DrawGifTag.FLG = 0; // packed mode
   DrawGifTag.NREG = 5;
   DrawGifTag.REGS0 = 0x1; // rgbaq
   DrawGifTag.REGS1 = 0xf; // nop
   DrawGifTag.REGS2 = 0x4; // xyzf2
   DrawGifTag.REGS3 = 0xf; // nop
   DrawGifTag.REGS4 = 0x4; // xyzf2
   prim.PRIM = 6; // sprite
   prim.IIP = 1; // flat shading
   prim.TME = 0; // no texture mapping
   prim.FGE = 0; // no fog
   prim.ABE = 0; // no alpha
   prim.AA1 = 0; // no aa
   prim.FST = 0; // stq coords
   prim.CTXT = (tU64)context;
   prim.FIX = 0; // what the hell does this do?
   DrawGifTag.PRIM = *(tU64*)&prim;

   SetVertices( minX, minY, width, height );

   mAssert( ((tU32)&DrawGifTag & 0xf) == 0 );
}

void
CSprite::SetSTQs( float minS, float minT, float width, float height )
{
	GS::tPrim prim;
	*(tU64*)&prim = DrawGifTag.PRIM;
	prim.FST = 0; // stq
	DrawGifTag.PRIM = *(tU64*)&prim;
	DrawGifTag.REGS1 = 0x2;
	DrawGifTag.REGS3 = 0x2;

	TexCoords1.stq[0] = minS; TexCoords1.stq[1] = minT;
	TexCoords2.stq[0] = minS + width; TexCoords2.stq[1] = minT + height;

	SetUseTexture( true );
}

void
CSprite::SetUVs( float minU, float minV, float width, float height )
{
	GS::tPrim prim;
	*(tU64*)&prim = DrawGifTag.PRIM;
	prim.FST = 1; // uv
	DrawGifTag.PRIM = *(tU64*)&prim;
	DrawGifTag.REGS1 = 0x3;
	DrawGifTag.REGS3 = 0x3;

	TexCoords1.uv[0] = Core::FToI4((float)minU); TexCoords1.uv[1] = Core::FToI4((float)minV);
	TexCoords2.uv[0] = Core::FToI4(minU + width); TexCoords2.uv[1] = Core::FToI4(minV + height);

	SetUseTexture( true );
}

void
CSprite::SetUVs( tU16 minU, tU16 minV, tU16 width, tU16 height )
{
	GS::tPrim prim;
	*(tU64*)&prim = DrawGifTag.PRIM;
	prim.FST = 1; // uv
	DrawGifTag.PRIM = *(tU64*)&prim;
	DrawGifTag.REGS1 = 0x3;
	DrawGifTag.REGS3 = 0x3;

	TexCoords1.uv[0] = (minU << 4) + 8; TexCoords1.uv[1] = (minV << 4) + 8;
	TexCoords2.uv[0] = ((minU + width) << 4) + 8; TexCoords2.uv[1] = ((minV + height) << 4) + 8;

	SetUseTexture( true );
}

void
CSprite::SetUVsFix4( tU16 minU, tU16 minV, tU16 maxU, tU16 maxV )
{
	GS::tPrim prim;
	*(tU64*)&prim = DrawGifTag.PRIM;
	prim.FST = 1; // uv
	DrawGifTag.PRIM = *(tU64*)&prim;
	DrawGifTag.REGS1 = 0x3;
	DrawGifTag.REGS3 = 0x3;

	TexCoords1.uv[0] = minU; TexCoords1.uv[1] = minV;
	TexCoords2.uv[0] = maxU; TexCoords2.uv[1] = maxV;

	SetUseTexture( true );
}

void
CSprite::Draw( CSCDmaPacket& packet )
{
	packet.Cnt();
	packet.Add( (tU128*)&DrawGifTag, 6 );
	packet.CloseTag();
}

void
CSprite::Draw( CVifSCDmaPacket& packet )
{
	packet.Cnt();
	{
		// the data needs to be qword-aligned, so pad with appropriate # of vifnops to
		// put the direct vifcode at the end of a qword
		packet.Nop();
		if ( ! packet.GetTTE() ) {
			packet.Nop().Nop();
		}

		packet.OpenDirect();
		packet.Add( (tU128*)&DrawGifTag, 6 );
		packet.CloseDirect();
	}
	packet.CloseTag();
}
