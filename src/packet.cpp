/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

/********************************************
 * includes
 */

#include <malloc.h>
#include <stdio.h>

#include "kernel.h"

#include "ps2s/gs.h"
#include "ps2s/math.h"
#include "ps2s/packet.h"

/********************************************
 * DmaPacket
 */

CDmaPacket::CDmaPacket(tU128* buffer, tU32 bufferQWSize, tDmaChannelId channel, tU32 memMapping, bool isFull)
    : pBase((tU8*)buffer)
    , pNext((tU8*)((isFull) ? buffer + bufferQWSize : buffer))
    , dmaChannelId(channel)
    , uiBufferQwordSize(bufferQWSize)
    , bDeallocateBuffer(false)
{
    // PLIN
    mErrorIf(memMapping == Core::MemMappings::Uncached || memMapping == Core::MemMappings::UncachedAccl && (tU32)buffer & (64 - 1),
        "Dma buffer should be aligned on a cache line (64-byte boundary) when using the uncached mem mappings!");
    mErrorIf((memMapping == Core::MemMappings::Uncached || memMapping == Core::MemMappings::UncachedAccl) && bufferQWSize & (4 - 1),
        "Dma buffer size should be a whole number of cache lines (64 bytes = 4 quads) when using the uncached mem mappings!");
}

CDmaPacket::CDmaPacket(tU32 bufferQWSize, tDmaChannelId channel, tU32 memMapping)
    : dmaChannelId(channel)
    , uiBufferQwordSize(bufferQWSize)
    , bDeallocateBuffer(true)
{
    // PLIN
    mErrorIf((memMapping == Core::MemMappings::Uncached || memMapping == Core::MemMappings::UncachedAccl) && bufferQWSize & (4 - 1),
        "Dma buffer size should be a whole number of cache lines (64 bytes = 4 quads) when using the uncached mem mappings!");

    pBase = pNext = (tU8*)AllocBuffer(bufferQWSize, memMapping);
    mAssert(pBase != NULL);
}

CDmaPacket::~CDmaPacket()
{
    if (bDeallocateBuffer)
        free(Core::MakePtrNormal(pBase));
}

void* CDmaPacket::AllocBuffer(int numQwords, unsigned int memMapping)
{
    // an alignment of 64 bytes is strictly only necessary for uncached or uncached accl mem mappings, but
    // it ain't a bad idea in general..
    tU32 alignment = 64;
    void* mem      = (void*)((tU32)memalign(alignment, numQwords * 16) | memMapping);
    // I hate to do this, but I've wasted FAR too much time hunting down cache incoherency
    if (memMapping == Core::MemMappings::Uncached || memMapping == Core::MemMappings::UncachedAccl) {
        // PLIN
        FlushCache(0);
    }
    return mem;
}

void* CDmaPacket::SwapOutBuffer(void* newBuffer)
{
    void* oldBuffer = (void*)pBase;
    pBase           = (tU8*)newBuffer;
    return oldBuffer;
}

#define mCheckPktLength() mErrorIf((tU32)pNext & 0xf, "You don't really want to send a packet that isn't an even number of quads, do you?")

void CDmaPacket::Send(bool waitForEnd, bool flushCache)
{
    mCheckPktLength();

    tU32 pktQWLength = ((tU32)pNext - (tU32)pBase) / 16;
    mAssert(pktQWLength != 0);

    // dma_channel_send_normal always flushes the data cache
    //if (flushCache)
    //    FlushCache(0);

    // clear any memory mappings (this won't work for sp)
    dma_channel_fast_waits(dmaChannelId);
    dma_channel_send_normal(dmaChannelId, (void*)((tU32)pBase & 0x0fffffff), pktQWLength, 0, 0);

    if (waitForEnd)
        dma_channel_fast_waits(dmaChannelId);
}

void CDmaPacket::HexDump(tU32 numQwords)
{
    if (numQwords == 0)
        numQwords = ((tU32)pNext - (tU32)pBase) / 16;

    printf("dumping %d words\n", ((tU32)pNext - (tU32)pBase) / 4);

    tU32 i = 0;
    for (tU32 *nextWord = (tU32*)pBase; nextWord != (tU32*)pNext; nextWord++, i++) {
        if ((i % 4) == 0)
            printf("\n0x%08x:  ", (tU32)nextWord);
        printf("0x%08x ", *nextWord);
        if (i / 4 == numQwords)
            break;
    }

    printf("\n\n");
}

void CDmaPacket::Print(void)
{
    HexDump();
}

/********************************************
 * Source Chain DmaPacket
 */

CSCDmaPacket::CSCDmaPacket(tU32 bufferQWSize, tDmaChannelId channel, bool tte, tU32 memMapping)
    : CDmaPacket(bufferQWSize, channel, memMapping)
    , bTTE(tte)
    , pOpenTag(NULL)
    , uiTTEBytesLeft(0)
{
}

CSCDmaPacket::CSCDmaPacket(tU128* buffer, tU32 bufferQWSize, tDmaChannelId channel, bool tte, tU32 memMapping, bool isFull)
    : CDmaPacket(buffer, bufferQWSize, channel, memMapping, isFull)
    , bTTE(tte)
    , pOpenTag(NULL)
    , uiTTEBytesLeft(0)
{
}

void CSCDmaPacket::Send(bool waitForEnd, bool flushCache)
{
    mCheckPktLength();

    // make sure we haven't forgotten to close the last dma tag
    mAssert(pOpenTag == NULL);

    // dma_channel_send_chain does NOT flush all data that is "source chained"
    if (flushCache)
        FlushCache(0);

    // clear any memory mappings (this won't work for sp)
    dma_channel_fast_waits(dmaChannelId);
    dma_channel_send_chain(dmaChannelId, (void*)((tU32)pBase & 0x0fffffff), 0, bTTE ? DMA_FLAG_TRANSFERTAG : 0, 0);

    if (waitForEnd)
        dma_channel_fast_waits(dmaChannelId);
}

/********************************************
 * Vif Source Chain DmaPacket
 */

CVifSCDmaPacket::CVifSCDmaPacket(tU32 bufferQWSize, tDmaChannelId channel, bool tte, tU32 memMapping)
    : CSCDmaPacket(bufferQWSize, channel, tte, memMapping)
    , pOpenVifCode(NULL)
    , uiWL(1)
    , uiCL(1)
{
}

CVifSCDmaPacket::CVifSCDmaPacket(tU128* buffer, tU32 bufferQWSize, tDmaChannelId channel, bool tte,
    tU32 memMapping, bool isFull)
    : CSCDmaPacket(buffer, bufferQWSize, channel, tte, memMapping, isFull)
    , pOpenVifCode(NULL)
{
}

CVifSCDmaPacket&
CVifSCDmaPacket::CloseUnpack(void)
{
    mAssert(pOpenVifCode);
    tU32 vn = (pOpenVifCode->cmd & 0xc) >> 2;
    tU32 vl = pOpenVifCode->cmd & 0x3;

    // the goal here is to find the num field of the open unpack, which is the number of
    // qwords ACTUALLY WRITTEN to vu memory (it does not count quads that are skipped in
    // "skipping write" mode.)  But first forget about skipping/filling writes and compute the number
    // of qwords that the data in this packet will expand to.
    tU32 numBytes         = (tU32)pNext - (tU32)pOpenVifCode - 4;
    tU32 numBytesPerBlock = 4 >> vl;
    tU32 numBlocksPerQuad = vn + 1;
    // make sure that the data length is a multiple of 8, 16, or 32 bits, whichever is appropriate
    mAssert((numBytes & (numBytesPerBlock - 1)) == 0);
    tU32 numQuads = (numBytes / numBytesPerBlock) / numBlocksPerQuad;

    // We have the number of quads our data will directly expand to, so now we need to account for
    // skipping/filling write modes.

    // skipping write is easy -- we are already done

    // filling 	write: now we get ambiguous -- what to do when numQuads == CL in the last
    // block?  I will say that in this case the vif should still do the full wl length block, filling
    // with internal registers.  If you want different behavior call CloseUnpack with a num field you've
    // computed yourself
    if (uiCL < uiWL) {
        tU32 numWLBlocks    = (numQuads / uiCL);
        tU32 lastBlockQuads = numQuads - numWLBlocks * uiCL;
        if (lastBlockQuads == uiCL)
            lastBlockQuads = uiWL;
        numQuads           = numWLBlocks * uiWL + lastBlockQuads;
    }

    return CloseUnpack(numQuads);
}

#undef mCheckPktLength
