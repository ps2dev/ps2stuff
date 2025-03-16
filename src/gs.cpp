/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include "ps2s/gs.h"
#include "ps2s/packet.h"

namespace GS {

/********************************************
 * static (private) data
 */

typedef struct {
    tGifTag gt;
    uint64_t texFlush;
    uint64_t texFlushAddr;
} __attribute__((packed,aligned(16))) tFlushPkt;
static tFlushPkt FlushPkt;

/********************************************
 * functions
 */

void Init(void)
{
    FlushPkt.gt.NLOOP = 1;
    FlushPkt.gt.EOP   = 1;
    FlushPkt.gt.PRE   = 0;
    FlushPkt.gt.FLG   = 0; // packed
    FlushPkt.gt.NREG  = 1;
    FlushPkt.gt.REGS0 = 0xe; // a+d

    FlushPkt.texFlushAddr = GS::RegAddrs::texflush;
}

void Flush(void)
{
    tGifTag gifTag;
    gifTag.NLOOP = 1;
    gifTag.EOP   = 1;
    gifTag.PRE   = 0;
    gifTag.FLG   = 0; // packed
    gifTag.NREG  = 1;
    gifTag.REGS0 = 0xe;
    // PLIN
    *GIF::Registers::fifo = *(uint128_t*)&gifTag;

    struct {
        uint64_t data;
        uint64_t addr;
    } texFlush;
    texFlush.addr = GS::RegAddrs::texflush;
    // PLIN
    *GIF::Registers::fifo = *(uint128_t*)&texFlush;
}

void Flush(CSCDmaPacket& packet)
{
    packet.Cnt();
    packet.Add((uint128_t*)&FlushPkt, 2);
    packet.CloseTag();
}

void ReorderClut(uint32_t* oldClut, uint32_t* newClut)
{
    // make sure both are qword aligned
    mAssert(((uint32_t)oldClut & 0xf) == 0 && ((uint32_t)newClut & 0xf) == 0);
    // oldClut and newClut can't be the same
    mAssert(oldClut != newClut);

    uint128_t* curNewEntry = (uint128_t*)newClut;
    uint128_t* oldEntry1   = (uint128_t*)oldClut;
    uint128_t* oldEntry2   = (uint128_t*)oldClut + 4;

    uint32_t i;
    for (i = 0; i < 8; i++) {
        *curNewEntry++ = *oldEntry1++;
        *curNewEntry++ = *oldEntry1++;

        *curNewEntry++ = *oldEntry2++;
        *curNewEntry++ = *oldEntry2++;

        *curNewEntry++ = *oldEntry1++;
        *curNewEntry++ = *oldEntry1++;

        *curNewEntry++ = *oldEntry2++;
        *curNewEntry++ = *oldEntry2++;

        oldEntry1 += 4;
        oldEntry2 += 4;
    }
}

} // namespace GS
