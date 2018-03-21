/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_sprite_h
#define ps2s_sprite_h

/********************************************
 * includes
 */

#include "ps2s/core.h"
#include "ps2s/dmac.h"
#include "ps2s/gs.h"
#include "ps2s/packet.h"

/********************************************
 * typedefs
 */

typedef union {
    tU32 uv[4];
    float stq[4];
} tTexCoords;

/********************************************
 * class def
 */

class CSprite {
public:
    CSprite(GS::tContext context, tU32 minX, tU32 minY, tU32 width, tU32 height);

    void Draw(bool waitForEnd = false, bool flushCache = true)
    {
        GifPacket.Send(waitForEnd, flushCache);
    }
    void Draw(CSCDmaPacket& packet);
    void Draw(CVifSCDmaPacket& packet);

    void SetColor(tU32 r, tU32 g, tU32 b, tU32 a, tU8 fog = 255)
    {
        Color[0]   = r;
        Color[1]   = g;
        Color[2]   = b;
        Color[3]   = a;
        Vertex2[3] = (tU32)fog << 4;
    }
    void SetColor(tU32 rgba)
    {
        Color[0] = 0xff & rgba;
        Color[1] = (0xff00 & rgba) >> 8;
        Color[2] = (0xff0000 & rgba) >> 16;
        Color[3] = (0xff000000 & rgba) >> 24;
    }

    void SetDepth(tU32 depth) { Vertex1[2] = Vertex2[2] = depth; }

    void SetSTQs(float minS, float minT, float width, float height);

    void SetUVs(float minU, float minV, float width, float height);
    void SetUVs(tU16 minU, tU16 minV, tU16 width, tU16 height);
    void SetUVs(t32 minU, t32 minV, t32 width, t32 height)
    {
        SetUVs((tU16)minU, (tU16)minV, (tU16)width, (tU16)height);
    }
    void SetUVsFix4(tU16 minU, tU16 minV, tU16 maxU, tU16 maxV);

    void SetVertices(tU32 minX, tU32 minY, tU32 width, tU32 height)
    {
        Vertex1[0] = minX << 4;
        Vertex1[1] = minY << 4;
        Vertex2[0] = (minX + width) << 4;
        Vertex2[1] = (minY + height) << 4;
    }

    inline CSprite& SetUseAlphaBlend(bool useAlpha);
    inline CSprite& SetUseTexture(bool useTex);
    inline CSprite& SetUseFog(bool useFog);

    CDmaPacket& GetPacket(void) { return GifPacket; }

    inline void* operator new(size_t size)
    {
        return Core::New16(size);
    }

protected:
private:
    tGifTag DrawGifTag;
    tU32 Color[4];
    tTexCoords TexCoords1;
    tU32 Vertex1[4];
    tTexCoords TexCoords2;
    tU32 Vertex2[4];

    CDmaPacket GifPacket;

} __attribute__((aligned(16)));

/********************************************
 * inline methods
 */

inline CSprite&
CSprite::SetUseAlphaBlend(bool useAlpha)
{
    if (useAlpha)
        DrawGifTag.PRIM |= (tU64)1 << 6; // abe
    else
        DrawGifTag.PRIM &= ~((tU64)1 << 6);

    return *this;
}

inline CSprite&
CSprite::SetUseTexture(bool useTex)
{
    if (useTex)
        DrawGifTag.PRIM |= (tU64)1 << 4; // tme
    else
        DrawGifTag.PRIM &= ~((tU64)1 << 4);

    return *this;
}

inline CSprite&
CSprite::SetUseFog(bool useFog)
{
    if (useFog)
        DrawGifTag.PRIM |= (tU64)1 << 5; // fge
    else
        DrawGifTag.PRIM &= ~((tU64)1 << 5);

    return *this;
}

#endif // ps2s_sprite_h
