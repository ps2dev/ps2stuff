/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_drawenv_h
#define ps2s_drawenv_h

/********************************************
 * includes
 */

// PLIN
//  #include <libgraph.h>
//  #include <eestruct.h>

#include "ps2s/core.h"
#include "ps2s/types.h"
#include "ps2s/dmac.h"
#include "ps2s/gs.h"
#include "ps2s/packet.h"

namespace GS {

   /********************************************
    * constants
    */

   static const bool	kDontAddHalfPixel = false;
   static const bool	kAddHalfPixel = true;

   /********************************************
    * typedefs
    */

   // the "using"s after each namespace { ... } are to make the type
   // available from the GS namespace (ie, so you can say "GS::tAlphaBlendVal")

   namespace ABlend {
      typedef enum {
	 kSourceRGB = 0,
	 kDestRGB = 1,
	 kSourceAlpha = 0,
	 kDestAlpha = 1,
	 kZero = 2,
	 kFix = 2
      } tAlphaBlendVal;
   }
   using ABlend::tAlphaBlendVal;

   namespace ATest {
      typedef enum {
	 kNever,
	 kAlways,
	 kLess,
	 kLEqual,
	 kEqual,
	 kGEqual,
	 kGreater,
	 kNotEqual
      } tAlphaTestPassMode;

      typedef enum {
	 kKeep,
	 kFrameBuffOnly,
	 kZBuffOnly,
	 kRGBOnly
      } tAlphaTestFailAction;
   }
   using ATest::tAlphaTestPassMode;
   using ATest::tAlphaTestFailAction;

   namespace ZTest {
      typedef enum {
	 kNever,
	 kAlways,
	 kGEqual,
	 kGreater
      } tZTestPassMode;
   }
   using ZTest::tZTestPassMode;

   namespace Dither {
     typedef enum {
       kMinusFour = 0x4,
       kMinusThree = 0x5,
       kMinusTwo = 0x6,
       kMinusOne = 0x7,
       kZero = 0,
       kOne = 1,
       kTwo = 2,
       kThree = 3
     } tDitherVal;
   }
   using Dither::tDitherVal;

   /********************************************
    * class definition
    */

   class CDrawEnv {
      public:
	 CDrawEnv( GS::tContext context );
	 CDrawEnv( const CDrawEnv& rhs );
	 CDrawEnv( GS::tContext context, tU32 fbW, tU32 fbH, tU32 fbWordAddr, tU32 zbufWordAddr = NULL );

	 void operator=( const CDrawEnv& otherDE );

	 void SetContext( GS::tContext context );

	 inline void EnableAlphaTest( void ) { gsrTest.ATE = 1; }
	 inline void DisableAlphaTest( void ) { gsrTest.ATE = 0; }
	 inline void SetAlphaTestPassMode( tAlphaTestPassMode mode ) { gsrTest.ATST = (tU64)mode; }
	 inline void SetAlphaTestFailAction( tAlphaTestFailAction action ) { gsrTest.AFAIL = (tU64)action; }
	 inline void SetAlphaRefVal( tU8 refVal ) { gsrTest.AREF = refVal; }

	 inline void EnableSelectiveAlphaBlend( void ) { gsrPABE.PABE = 1; }
	 inline void DisableSelectiveAlphaBlend( void ) { gsrPABE.PABE = 0; }

	 inline void SetAlphaBlendFunc( tAlphaBlendVal a, tAlphaBlendVal b, tAlphaBlendVal c, tAlphaBlendVal d, tU32 fix );

	 inline void EnableColorClamp( void ) { gsrColClamp.CLAMP = 1; }
	 inline void DisableColorClamp( void ) { gsrColClamp.CLAMP = 0; }

	 inline void EnableDestAlphaTest( void ) { gsrTest.DATE = 1; }
	 inline void DisableDestAlphaTest( void ) { gsrTest.DATE = 0; }
	 inline void SetDestAlpha0Pass( void ) { gsrTest.DATM = 0; }
	 inline void SetDestAlpha1Pass( void ) { gsrTest.DATM = 1; }

	 inline void SetDepthTestPassMode( tZTestPassMode passMode ) { gsrTest.ZTST = (tU64)passMode; eZTestPassMode = passMode; }
	 inline void SetDepthBufferAddr( tU32 wordAddress ) { gsrZBuf.ZBP = wordAddress/2048; }
	 inline void SetDepthBufferPSM( tU32 psm ) { gsrZBuf.PSM = (tU64)psm; }
	 inline void EnableDepthTest( void ) { gsrZBuf.ZMSK = 0; SetDepthTestPassMode(eZTestPassMode); }
	 inline void DisableDepthTest( void ) { gsrZBuf.ZMSK = 1; gsrTest.ZTST = (tU64)ZTest::kAlways; }
	 inline void SetDepthWriteEnabled( bool write ) { gsrZBuf.ZMSK = ! write; }

	 inline void SetFrameBufferAddr( tU32 wordAddress ) { mAssert( (wordAddress & 2047) == 0 ); gsrFrame.FBP = wordAddress/2048; }
	 inline void SetFrameBufferDrawMask( tU32 drawMask ) { gsrFrame.FBMSK = drawMask; }
	 void SetFrameBufferDim( tU32 pixelW, tU32 pixelH );
	 inline void SetFrameBufferPSM( tU32 psm ) { gsrFrame.PSM = psm; }
	 float GetInterlacedPixelOffset() const { return InterlacedOffset; }
	 void SetInterlacedPixelOffset( float offset ) { InterlacedOffset = offset; }
	 void CalculateClippedFBXYOffsets( bool addHalfPixel );
	 inline void SetXYOffsets( tU16 offsetX, tU16 offsetY ) { gsrXYOffset.OFX = offsetX << 4; gsrXYOffset.OFY = offsetY << 4; }
	 inline void SetXYOffsetsFix4( tU16 offsetX, tU16 offsetY ) { gsrXYOffset.OFX = offsetX; gsrXYOffset.OFY = offsetY; }

	 inline void SetFrameBufAlphaORMask( tU32 mask ) { gsrFBA.FBA = mask; }
	 inline void SetFogColor( tU32 r, tU32 g, tU32 b ) { gsrFogCol.FCR = r; gsrFogCol.FCG = g; gsrFogCol.FCB = b; }

	 inline void EnableDither( void ) {  gsrDTHE.DTHE = 1; }
	 inline void DisableDither( void ) { gsrDTHE.DTHE = 0; }
	 inline void SetDitherMatrix( tDitherVal dm00, tDitherVal dm01, tDitherVal dm02, tDitherVal dm03,
				      tDitherVal dm10, tDitherVal dm11, tDitherVal dm12, tDitherVal dm13,
				      tDitherVal dm20, tDitherVal dm21, tDitherVal dm22, tDitherVal dm23,
				      tDitherVal dm30, tDitherVal dm31, tDitherVal dm32, tDitherVal dm33 );

	 inline void SetScissorArea( tU32 scX, tU32 scY, tU32 scWidth, tU32 scHeight ); // must be after SetFrameBufferDim() to have effect

	 void SendSettings( bool waitForEnd = false, bool flushCache = true );
	 void SendSettings( CSCDmaPacket& packet );
	 void SendSettings( CVifSCDmaPacket& packet );

	 // accessors
	 tU32 GetFrameBufferAddr( void ) { return gsrFrame.FBP * 2048; }

	 inline void* operator new( size_t size ) {
	    return Core::New16(size);
	 }

      protected:

	 tSourceChainTag SettingsDmaTag;
	 tGifTag	SettingsGifTag;
	 GS::tFrame	gsrFrame;	tU64 FrameAddr;
	 GS::tZbuf	gsrZBuf;	tU64 ZBufAddr;
	 GS::tXyoffset	gsrXYOffset;	tU64 XYOffsetAddr;
	 GS::tPrmodecont gsrPrModeCont;	tU64 PrModeContAddr;
	 GS::tColclamp	gsrColClamp;	tU64 ColClampAddr;
	 GS::tTest	gsrTest;	tU64 TestAddr;
	 GS::tAlpha	gsrAlpha;	tU64 AlphaAddr;
	 GS::tPabe	gsrPABE;	tU64 PABEAddr;
	 GS::tFba	gsrFBA;		tU64 FBAAddr;
	 GS::tDthe	gsrDTHE;	tU64 DTHEAddr;
	 GS::tDimx	gsrDIMX;	tU64 DIMXAddr;
	 GS::tScissor	gsrScissor;	tU64 ScissorAddr;
	 GS::tFogcol	gsrFogCol;	tU64 FogColAddr;
	 tU32		uiNumGSRegs;

	 CSCDmaPacket	GifPacket;

	 tZTestPassMode eZTestPassMode;
	 tU32		uiFBWidth, uiFBHeight;
	 float		InterlacedOffset;

      private:
	 void InitCommon( GS::tContext context );

   } __attribute__ ((aligned(16)));

   /********************************************
    * inline methods
    */

   inline void
   CDrawEnv::SetAlphaBlendFunc( tAlphaBlendVal a, tAlphaBlendVal b, tAlphaBlendVal c, tAlphaBlendVal d, tU32 fix )
   {
      gsrAlpha.A = a;
      gsrAlpha.B = b;
      gsrAlpha.C = c;
      gsrAlpha.D = d;
      gsrAlpha.FIX = fix;
   }

   inline void
   CDrawEnv::SetDitherMatrix( tDitherVal dm00, tDitherVal dm01, tDitherVal dm02, tDitherVal dm03,
			      tDitherVal dm10, tDitherVal dm11, tDitherVal dm12, tDitherVal dm13,
			      tDitherVal dm20, tDitherVal dm21, tDitherVal dm22, tDitherVal dm23,
			      tDitherVal dm30, tDitherVal dm31, tDitherVal dm32, tDitherVal dm33 )
   {
      gsrDIMX.DIMX00 = (tU64)dm00; gsrDIMX.DIMX01 = (tU64)dm01; gsrDIMX.DIMX02 = (tU64)dm02; gsrDIMX.DIMX03 = (tU64)dm03;
      gsrDIMX.DIMX10 = (tU64)dm10; gsrDIMX.DIMX11 = (tU64)dm11; gsrDIMX.DIMX12 = (tU64)dm12; gsrDIMX.DIMX13 = (tU64)dm13;
      gsrDIMX.DIMX20 = (tU64)dm20; gsrDIMX.DIMX21 = (tU64)dm21; gsrDIMX.DIMX22 = (tU64)dm22; gsrDIMX.DIMX23 = (tU64)dm23;
      gsrDIMX.DIMX30 = (tU64)dm30; gsrDIMX.DIMX31 = (tU64)dm31; gsrDIMX.DIMX32 = (tU64)dm32; gsrDIMX.DIMX33 = (tU64)dm33;
   }

   inline void
   CDrawEnv::SetScissorArea( tU32 scX, tU32 scY, tU32 scWidth, tU32 scHeight )
   {
      gsrScissor.SCAX0 = scX; gsrScissor.SCAY0 = scY;
      gsrScissor.SCAX1 = scX + scWidth - 1; gsrScissor.SCAY1 = scY + scHeight - 1;
   }

} // namespace GS

#endif // ps2s_drawenv_h

