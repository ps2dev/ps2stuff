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

CDmaPacket::CDmaPacket(uint128_t* buffer, uint32_t bufferQWSize, tDmaChannelId channel, uint32_t memMapping, bool isFull)
    : pBase((uint8_t*)buffer)
    , pNext((uint8_t*)((isFull) ? buffer + bufferQWSize : buffer))
    , dmaChannelId(channel)
    , uiBufferQwordSize(bufferQWSize)
    , bDeallocateBuffer(false)
{
    // PLIN
    mErrorIf(memMapping == Core::MemMappings::Uncached || ((memMapping == Core::MemMappings::UncachedAccl) && ((uint32_t)buffer & (64 - 1))),
        "Dma buffer should be aligned on a cache line (64-byte boundary) when using the uncached mem mappings!");
    mErrorIf((memMapping == Core::MemMappings::Uncached || memMapping == Core::MemMappings::UncachedAccl) && bufferQWSize & (4 - 1),
        "Dma buffer size should be a whole number of cache lines (64 bytes = 4 quads) when using the uncached mem mappings!");
}

CDmaPacket::CDmaPacket(uint32_t bufferQWSize, tDmaChannelId channel, uint32_t memMapping)
    : dmaChannelId(channel)
    , uiBufferQwordSize(bufferQWSize)
    , bDeallocateBuffer(true)
{
    // PLIN
    mErrorIf((memMapping == Core::MemMappings::Uncached || memMapping == Core::MemMappings::UncachedAccl) && bufferQWSize & (4 - 1),
        "Dma buffer size should be a whole number of cache lines (64 bytes = 4 quads) when using the uncached mem mappings!");

    pBase = pNext = (uint8_t*)AllocBuffer(bufferQWSize, memMapping);
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
    uint32_t alignment = 64;
    void* mem      = (void*)((uint32_t)memalign(alignment, numQwords * 16) | memMapping);
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
    pBase           = (uint8_t*)newBuffer;
    return oldBuffer;
}

#define mCheckPktLength() mErrorIf((uint32_t)pNext & 0xf, "You don't really want to send a packet that isn't an even number of quads, do you?")

void CDmaPacket::Send(bool waitForEnd, bool flushCache)
{
    mCheckPktLength();

    uint32_t pktQWLength = ((uint32_t)pNext - (uint32_t)pBase) / 16;
    mAssert(pktQWLength != 0);

    // dma_channel_send_normal always flushes the data cache
    //if (flushCache)
    //    FlushCache(0);

    // clear any memory mappings (this won't work for sp)
    dma_channel_wait(dmaChannelId, 1000000);
    dma_channel_send_normal(dmaChannelId, (void*)((uint32_t)pBase & 0x0fffffff), pktQWLength, 0, 0);

    if (waitForEnd)
        dma_channel_wait(dmaChannelId, 1000000);
}

void CDmaPacket::HexDump(uint32_t numQwords)
{
    if (numQwords == 0)
        numQwords = ((uint32_t)pNext - (uint32_t)pBase) / 16;

    printf("dumping %ld words (%ld qwords)\n", ((uint32_t)pNext - (uint32_t)pBase) / 4, numQwords);

    uint32_t i = 0;
    for (uint32_t *nextWord = (uint32_t*)pBase; nextWord != (uint32_t*)pNext; nextWord++, i++) {
        if ((i % 4) == 0)
            printf("\n0x%08lx:  ", (uint32_t)nextWord);
        printf("0x%08lx ", *nextWord);
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

CSCDmaPacket::CSCDmaPacket(uint32_t bufferQWSize, tDmaChannelId channel, bool tte, uint32_t memMapping)
    : CDmaPacket(bufferQWSize, channel, memMapping)
    , bTTE(tte)
    , pOpenTag(NULL)
    , uiTTEBytesLeft(0)
{
}

CSCDmaPacket::CSCDmaPacket(uint128_t* buffer, uint32_t bufferQWSize, tDmaChannelId channel, bool tte, uint32_t memMapping, bool isFull)
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
    dma_channel_wait(dmaChannelId, 1000000);
    dma_channel_send_chain(dmaChannelId, (void*)((uint32_t)pBase & 0x0fffffff), 0, bTTE ? DMA_FLAG_TRANSFERTAG : 0, 0);

    if (waitForEnd)
        dma_channel_wait(dmaChannelId, 1000000);
}

/********************************************
 * Vif Source Chain DmaPacket
 */

CVifSCDmaPacket::CVifSCDmaPacket(uint32_t bufferQWSize, tDmaChannelId channel, bool tte, uint32_t memMapping)
    : CSCDmaPacket(bufferQWSize, channel, tte, memMapping)
    , pOpenVifCode(NULL)
    , uiWL(1)
    , uiCL(1)
{
}

CVifSCDmaPacket::CVifSCDmaPacket(uint128_t* buffer, uint32_t bufferQWSize, tDmaChannelId channel, bool tte,
    uint32_t memMapping, bool isFull)
    : CSCDmaPacket(buffer, bufferQWSize, channel, tte, memMapping, isFull)
    , pOpenVifCode(NULL)
{
}

CVifSCDmaPacket&
CVifSCDmaPacket::CloseUnpack(void)
{
    mAssert(pOpenVifCode);
    uint32_t vn = (pOpenVifCode->cmd & 0xc) >> 2;
    uint32_t vl = pOpenVifCode->cmd & 0x3;

    // the goal here is to find the num field of the open unpack, which is the number of
    // qwords ACTUALLY WRITTEN to vu memory (it does not count quads that are skipped in
    // "skipping write" mode.)  But first forget about skipping/filling writes and compute the number
    // of qwords that the data in this packet will expand to.
    uint32_t numBytes         = (uint32_t)pNext - (uint32_t)pOpenVifCode - 4;
    uint32_t numBytesPerBlock = 4 >> vl;
    uint32_t numBlocksPerQuad = vn + 1;
    // make sure that the data length is a multiple of 8, 16, or 32 bits, whichever is appropriate
    mAssert((numBytes & (numBytesPerBlock - 1)) == 0);
    uint32_t numQuads = (numBytes / numBytesPerBlock) / numBlocksPerQuad;

    // We have the number of quads our data will directly expand to, so now we need to account for
    // skipping/filling write modes.

    // skipping write is easy -- we are already done

    // filling 	write: now we get ambiguous -- what to do when numQuads == CL in the last
    // block?  I will say that in this case the vif should still do the full wl length block, filling
    // with internal registers.  If you want different behavior call CloseUnpack with a num field you've
    // computed yourself
    if (uiCL < uiWL) {
        uint32_t numWLBlocks    = (numQuads / uiCL);
        uint32_t lastBlockQuads = numQuads - numWLBlocks * uiCL;
        if (lastBlockQuads == uiCL)
            lastBlockQuads = uiWL;
        numQuads           = numWLBlocks * uiWL + lastBlockQuads;
    }

    return CloseUnpack(numQuads);
}

#undef mCheckPktLength
