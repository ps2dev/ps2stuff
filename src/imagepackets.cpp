/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include "ps2s/imagepackets.h"

#include "ps2s/gs.h"
#include "ps2s/math.h"

/********************************************
 * CImageUploadPkt methods
 */

#  define kPacketLength 21

CImageUploadPkt::CImageUploadPkt( void )
   : CVifSCDmaPacket( &FirstDmaTag, kPacketLength, DMAC::Channels::gif, Packet::kDontXferTags )
{
   InitCommon();
}

CImageUploadPkt::CImageUploadPkt( tU128* imagePtr, tU32 w, tU32 h, GS::tPSM psm, tU32 gsBufWidth, tU32 gsWordAddress )
   : CVifSCDmaPacket( &FirstDmaTag, kPacketLength, DMAC::Channels::gif, Packet::kDontXferTags )
{
   InitCommon();

   SetImage( imagePtr, w, h, psm );
   SetGsBufferWidth( gsBufWidth );
   SetGsAddr( gsWordAddress );
}

#undef kPacketLength

void
CImageUploadPkt::InitCommon( void )
{
   uiNumXferGSRegs = 4;

   // register addresses
   BitBltBufAddr = GS::RegAddrs::bitbltbuf;
   TrxRegAddr = GS::RegAddrs::trxreg;
   TrxPosAddr = GS::RegAddrs::trxpos;
   TrxDirAddr = GS::RegAddrs::trxdir;

   // source info
   // gsrBitBltBuf.SBP = 0x3fff; // invalid?
   // gsrBitBltBuf.SBW = 0x3f; // invalid
   // gsrBitBltBuf.SPSM = SCE_GS_PSMCT32;

   // dest info
   gsrBitBltBuf.DBP = 0x3fff; // invalid?
   gsrBitBltBuf.DBW = 0x3f; // invalid
   gsrBitBltBuf.DPSM = GS::kPsm32;

   gsrTrxDir.XDR = 0; // host -> local (main mem -> gs)

   // offsets within the source/dest buffers (in pixels)
   gsrTrxPos.SSAX = 0;  gsrTrxPos.SSAY = 0;
   gsrTrxPos.DSAX = 0;  gsrTrxPos.DSAY = 0;
   gsrTrxPos.DIR = 0; // upper-left -> lower-right (only for xfers within gs mem)

   // dimensions of xfer area
   gsrTrxReg.RRW = 0;
   gsrTrxReg.RRH = 0;

   // setup the giftag for sending image transfer settings to the gs
   ImageXferSettingsGifTag.NLOOP = uiNumXferGSRegs;
   ImageXferSettingsGifTag.EOP = 0;
   ImageXferSettingsGifTag.PRE = 0;
   ImageXferSettingsGifTag.FLG = 0; // packed
   ImageXferSettingsGifTag.NREG = 1;
   ImageXferSettingsGifTag.REGS0 = 0xe;
}

// the packet we build will be suitable for transferring on path1 (through vif1) when tte == 1,
// or path3 (gif) when tte == 0.

void
CImageUploadPkt::BuildXferTags( void )
{
   // see comment above this function
   SetTTE( true );

	// transfer the registers
   this->Cnt();
   this->Nop().OpenDirect();
   pNext += 5 * 16;
   this->CloseDirect().CloseTag();

   // some giftag defaults
// PLIN
//     sceGifTag imageDataGifTag;
//     SCE_GIF_CLEAR_TAG( &imageDataGifTag );
   tGifTag imageDataGifTag = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
   imageDataGifTag.FLG = 2; // image mode
   imageDataGifTag.EOP = 0;
   imageDataGifTag.NLOOP = 0;

   mAssert( pImage != NULL );
   tU128 *image = pImage;

   bool imageOnSP = false;

#ifndef PS2_LINUX
   // is the image on the scratchpad?
   imageOnSP = ((tU32)pImage & 0x70000000);
   if ( imageOnSP ) {
      image = (tU128*)((tU32)image & 0x3ff0);
   }
#endif

   tU32 width = gsrTrxReg.RRW, height = gsrTrxReg.RRH;
   tU32 bytesInImage = width * height * GS::GetBitsPerPixel((GS::tPSM)gsrBitBltBuf.DPSM) / 8;

   tU32 numQuadsInImage = ( (bytesInImage & 0xf) == 0 ) ? bytesInImage / 16 : bytesInImage / 16 + 1;
   tU32 numQuadsLeft = numQuadsInImage;
   const tU32 maxQuadsPerGT = (1 << 15) - 1; // limited by the NLOOP field

   while ( numQuadsLeft > 0 ) {
      // set up giftag
      tU32 numQuadsThisGT = Math::Min( numQuadsLeft, maxQuadsPerGT );
      imageDataGifTag.NLOOP = numQuadsThisGT;
      imageDataGifTag.EOP = (numQuadsThisGT == numQuadsLeft) ? 1 : 0;

      // xfer giftag
      this->Cnt();
      this->Nop().OpenDirect();
      *this += imageDataGifTag;
      this->CloseDirect();
      this->CloseTag();

      if ( numQuadsThisGT == numQuadsLeft )
	 this->Refe( &image[numQuadsInImage - numQuadsLeft], numQuadsThisGT, Packet::kNoIrq, imageOnSP );
      else
	 this->Ref( &image[numQuadsInImage - numQuadsLeft], numQuadsThisGT, Packet::kNoIrq, imageOnSP );

      // these will fit in the upper 64 bits after the dma tag (ref or refe)
      this->Nop().OpenDirect().CloseDirect( numQuadsThisGT );

      numQuadsLeft -= numQuadsThisGT;
   }

   // see comment above this function
   SetTTE( false );
}
//  #ifndef PS2_LINUX
//  #else // PS2_LINUX
#if 0
void
CImageUploadPkt::BuildXferTags( void )
{
   // see comment above this function
   SetTTE( false );

   // transfer the registers
   this->Cnt();
   this->Nop().Nop().Nop().OpenDirect();
   pNext += 5 * 16;
   this->CloseDirect().CloseTag();

   this->SetTTE( true );

   // some giftag defaults
   tGifTag imageDataGifTag;
   imageDataGifTag.PRE = 0;
   imageDataGifTag.NREG = 0;
   imageDataGifTag.FLG = 2; // image mode
   imageDataGifTag.EOP = 0;
   imageDataGifTag.NLOOP = 0;

   mAssert( pImage != NULL );
   tU128 *image = pImage;
   // is the image on the scratchpad?
   // on linux, no.
//     if ( (tU32)image & 0x70000000 ) {
//        image = (tU128*)((tU32)image & 0x3ff0);
//     }

   tU32 width = gsrTrxReg.RRW, height = gsrTrxReg.RRH;
   tU32 bytesInImage = width * height * GS::GetBitsPerPixel((GS::tPSM)gsrBitBltBuf.DPSM) / 8;

   tU32 numQuadsInImage = ( (bytesInImage & 0xf) == 0 ) ? bytesInImage / 16 : bytesInImage / 16 + 1;
   tU32 numQuadsLeft = numQuadsInImage;
   const tU32 maxQuadsPerGT = (1 << 15) - 1; // limited by the NLOOP field

   while ( numQuadsLeft > 0 ) {
      // set up giftag
      tU32 numQuadsThisGT = Math::Min( numQuadsLeft, maxQuadsPerGT );
      imageDataGifTag.NLOOP = numQuadsThisGT;
      imageDataGifTag.EOP = (numQuadsThisGT == numQuadsLeft) ? 1 : 0;

      // xfer giftag
      this->Cnt();
      this->Nop().OpenDirect();
      *this += imageDataGifTag;
      this->CloseDirect();
      this->CloseTag();

      bool imageOnSP = false; // ((tU32)pImage & 0x70000000);
      if ( numQuadsThisGT == numQuadsLeft )
	 this->Refe( &image[numQuadsInImage - numQuadsLeft], numQuadsThisGT, Packet::kNoIrq, imageOnSP );
      else
	 this->Ref( &image[numQuadsInImage - numQuadsLeft], numQuadsThisGT, Packet::kNoIrq, imageOnSP );

      // these will fit in the upper 64 bits after the dma tag (ref or refe)
      this->Nop().OpenDirect().CloseDirect( numQuadsThisGT );

      numQuadsLeft -= numQuadsThisGT;
   }

   // see comment above this function
   SetTTE( false );
}
#endif

void
CImageUploadPkt::Send( CSCDmaPacket& packet )
{
   mErrorIf( packet.GetTTE(), "Only vif source chain packets can use this class to xfer images with tte on." );

   tU128* thisPktCopy = packet.Add( (CDmaPacket&)*this );

   // change the last refe in the copy to a ref
   tSourceChainTag* lastRefe = reinterpret_cast<tSourceChainTag*>( thisPktCopy + this->GetByteLength()/16 - 1 );
   mErrorIf( lastRefe->ID != DMAC::kRefe, "Something ain't right.." );
   lastRefe->ID = DMAC::kRef;
}

void
CImageUploadPkt::Send( CVifSCDmaPacket& packet )
{
   mErrorIf( ! packet.GetTTE(), "Vif source chains need to turn tte on to xfer images with this class." );

   tU128* thisPktCopy = packet.Add( (CDmaPacket&)*this );

   // change the last refe in the copy to a ref
   tSourceChainTag* lastRefe = reinterpret_cast<tSourceChainTag*>( thisPktCopy + this->GetByteLength()/16 - 1 );
   mErrorIf( lastRefe->ID != DMAC::kRefe, "Something ain't right.." );
   lastRefe->ID = DMAC::kRef;
}

