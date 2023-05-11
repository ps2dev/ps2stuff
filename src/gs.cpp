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
    tU64 texFlush;
    tU64 texFlushAddr;
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
    *GIF::Registers::fifo = *(tU128*)&gifTag;

    struct {
        tU64 data;
        tU64 addr;
    } texFlush;
    texFlush.addr = GS::RegAddrs::texflush;
    // PLIN
    *GIF::Registers::fifo = *(tU128*)&texFlush;
}

void Flush(CSCDmaPacket& packet)
{
    packet.Cnt();
    packet.Add((tU128*)&FlushPkt, 2);
    packet.CloseTag();
}

void ReorderClut(tU32* oldClut, tU32* newClut)
{
    // make sure both are qword aligned
    mAssert(((tU32)oldClut & 0xf) == 0 && ((tU32)newClut & 0xf) == 0);
    // oldClut and newClut can't be the same
    mAssert(oldClut != newClut);

    tU128* curNewEntry = (tU128*)newClut;
    tU128* oldEntry1   = (tU128*)oldClut;
    tU128* oldEntry2   = (tU128*)oldClut + 4;

    tU32 i;
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
