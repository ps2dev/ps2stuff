/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_packet_h
#define ps2s_packet_h

/********************************************
 * includes
 */

#include <stdlib.h>

#ifndef PS2_LINUX
#  include "dma.h"
#else
// ps2stuff, the kernel module
#  include "ps2stuff_kmodule.h"
#endif

#include "ps2s/types.h"
#include "ps2s/dmac.h"
#include "ps2s/vif.h"

#include "ps2s/utils.h"
#include "ps2s/core.h"

/********************************************
 * constants
 */

namespace Packet {

   // constructors' isFull = ?
   static const bool kFull = true;
   static const bool kNotFull = false;
   static const bool kWait = true;
   static const bool kDontWait = false;
   static const bool kFlushCache = true;
   static const bool kDontFlushCache = false;

   // CSCDmaPacket
   static const bool kXferTags = true;
   static const bool kDontXferTags = false;
   static const bool kIrq = true;
   static const bool kNoIrq = false;
   static const bool kFromSp = true;
   static const bool kFromMem = false;

   // CVifSCDmaPacket
   static const bool kDoubleBuff = true;
   static const bool kSingleBuff = false;
   static const bool kMasked = true;
   static const bool kNotMasked = false;

} // namespace Packet

/********************************************
 * class DmaPacket
 */

class CDmaPacket
{
   public:
      // need to add support for different memory mappings
      CDmaPacket( tU32 bufferQWSize, tDmaChannelId channel, tU32 memMapping = Core::MemMappings::Normal );
      CDmaPacket( tU128* buffer, tU32 bufferQWSize, tDmaChannelId channel, tU32 memMapping = Core::MemMappings::Normal, bool isFull = false );

      virtual ~CDmaPacket( void );

#ifdef PS2_LINUX
      /// dma packets need access to the gs, vu0 and vu1 devices
      inline static void InitFileDescriptors( int gs_fd,
					      int vu0_fd, int vu1_fd,
					      int pgl_fd ) {
	 GS_fd = gs_fd;
	 VU0_fd = vu0_fd;
	 VU1_fd = vu1_fd;
	 PGL_fd = pgl_fd;
      }
#endif

      // mutators

      template< class dataType > inline void operator += ( const dataType data );
      template< class dataType > inline dataType* Add( const dataType data );
      template< class dataType > inline dataType* Add( const dataType* data, tU32 num );

      // to use these you MUST cast to CDmaPacket&!! (otherwise you'll get a compiler error..  I
      // could make inlines for all of CDmaPacket's descendants, but I will certainly forget to
      // update them at some point in the future, when I may not be lucky enough to have
      // the compiler catch it...)
      void operator += ( const CDmaPacket& otherPkt );
      inline tU128* Add( const CDmaPacket& otherPkt );

      inline void Reset( void ) { pNext = pBase; }
      inline void SetDmaChannel( tDmaChannelId channel );
      virtual void Send( bool waitForEnd = false, bool flushCache = true );

      // accessors

      tU128* GetBase( void ) const { return (tU128*)pBase; }
      tU8* GetNextPtr( void ) const { return pNext; }
      tU32 GetByteLength( void ) const { return (tU32)pNext - (tU32)pBase; }

      static void* AllocBuffer( int numQwords, unsigned int memMapping );
      // be VERY careful using this.. it swaps its internal dma buffer with the new,
      // returning the old..  be aware of where memory is being deallocated..
      void* SwapOutBuffer( void *newBuffer );

      void HexDump( tU32 numQwords = 0 );
      virtual void Print( void );

   protected:
      tU8 		*pBase, *pNext;
      tDmaChannelId	dmaChannelId;
#ifdef PS2_LINUX
      int*		ChannelFd;
#endif
      tU32 		uiBufferQwordSize;

#ifdef PS2_LINUX
      static int	VU0_fd, VU1_fd, GS_fd, PGL_fd;

      int* GetChannelFd( tDmaChannelId channel ) {
 	 if (channel == DMAC::Channels::vif0)
 	    return &VU0_fd;
 	 else if (channel == DMAC::Channels::gif)
 	    return &GS_fd;
 	 else if (channel == DMAC::Channels::vif1)
 	    return &VU1_fd;
 	 else {
 	    mError("This dma channel is not supported in linux yet..");
	    return NULL; // calm down, gcc
 	 }
      }
#endif

   private:
      // And on the third day it was proclaimed: "Thou shalt not copy dma packets!"
      // For those who lack faith, the reason is that although a copy constructor
      // might occasionally be useful, making it illegal will more often catch
      // unexpected temporaries.
      CDmaPacket( const CDmaPacket& pktToCopy );
      CDmaPacket& operator=( const CDmaPacket& pktToCopy );

      bool 	bDeallocateBuffer;
};

/********************************************
 * class SCDmaPacket (Source Chain dma packet)
 */

class CSCDmaPacket : public CDmaPacket
{
   public:
      CSCDmaPacket( tU32 bufferQWSize, tDmaChannelId channel, bool tte, tU32 memMapping = Core::MemMappings::Normal );
      CSCDmaPacket( tU128* buffer, tU32 bufferQWSize, tDmaChannelId channel, bool tte, tU32 memMapping = Core::MemMappings::Normal, bool isFull = false );
      virtual ~CSCDmaPacket( void ) {}

      template< class dataType > inline void operator += ( const dataType data );
      template< class dataType > inline dataType* Add( const dataType data );
      template< class dataType > inline dataType* Add( const dataType* data, tU32 num );

      inline void operator += ( const CDmaPacket& otherPkt );
      inline tU128* Add( const CDmaPacket& otherPkt );

      inline CSCDmaPacket& Cnt( bool irq = false, tU32 pce = 0, bool sp = false );
      inline CSCDmaPacket& Next( const tDmaTag* nextTag, bool irq = false, bool sp = false, tU32 pce = 0 );
      inline CSCDmaPacket& Ref( const void *refData, tU32 dataQWLen, bool irq = false, bool sp = false, tU32 pce = 0 );
      inline CSCDmaPacket& Refs( const void *refData, tU32 dataQWLen, bool irq = false, bool sp = false, tU32 pce = 0 );
      inline CSCDmaPacket& Refe( const void *refData, tU32 dataQWLen, bool irq = false, bool sp = false, tU32 pce = 0 );
      inline CSCDmaPacket& Call( const void* nextTag, bool irq = false, bool sp = false, tU32 pce = 0 );
      inline CSCDmaPacket& Call( const CSCDmaPacket& pkt, bool irq = false, bool sp = false, tU32 pce = 0 );
      inline CSCDmaPacket& Ret( bool irq = false, tU32 pce = 0 );
      inline CSCDmaPacket& End( bool irq = false, tU32 pce = 0 );
      inline CSCDmaPacket& CloseTag( void );

      CSCDmaPacket& Pad96( tU32 padData );
      CSCDmaPacket& Pad128( tU32 padData );

      virtual void Send( bool waitForEnd = false, bool flushCache = true );

      bool GetTTE( void ) const { return bTTE; }
      void SetTTE( bool onOff ) { bTTE = onOff; }

      bool HasOpenTag( ) const { return pOpenTag != NULL; }

   protected:
      void SetDmaTag( tDmaTag *tag, tU32 QWC, tU32 PCE, tU32 ID, tU32 IRQ, const tU128 *ADDR, tU32 SPR );

#ifdef PS2_LINUX
      void* GetPhysAddr( const void *va );
#endif

      bool		bTTE;
      tDmaTag		*pOpenTag;
      tU32		uiTTEBytesLeft;

   private:

      // these will be passed as u32's as the QWC field, which is only 16 bits wide
      // so it should be ok to use the upper half-word
      static const tU32 countQWC = 1 << 16;
      static const tU32 dontCountQWC = 1 << 17;

      inline void AddDmaTag( tU32 QWC, tU32 PCE, tU32 ID, tU32 IRQ, const tU128 *ADDR, tU32 SPR );

      // see the note in CDmaPacket
      CSCDmaPacket( const CSCDmaPacket& pktToCopy );
      CSCDmaPacket& operator=( const CSCDmaPacket& pktToCopy );
};

/********************************************
 * class VifSCDmaPacket
 */

class CVifSCDmaPacket : public CSCDmaPacket
{
   public:
      CVifSCDmaPacket( tU32 bufferQWSize, tDmaChannelId channel, bool tte, tU32 memMapping = Core::MemMappings::Normal );
      CVifSCDmaPacket( tU128* buffer, tU32 bufferQWSize, tDmaChannelId channel, bool tte, tU32 memMapping = Core::MemMappings::Normal, bool isFull = false );

      virtual ~CVifSCDmaPacket( void ) {}

      inline CVifSCDmaPacket& Nop( bool irq = false );
      inline CVifSCDmaPacket& Stcycl( tU32 wl, tU32 cl, bool irq = false );
      inline CVifSCDmaPacket& Offset( tU32 offset, bool irq = false );
      inline CVifSCDmaPacket& Base( tU32 base, bool irq = false );
      inline CVifSCDmaPacket& Itop( tU32 itops, bool irq = false );
      inline CVifSCDmaPacket& Stmod( tU32 mode, bool irq = false );
      inline CVifSCDmaPacket& Mskpath3( tU32 mask, bool irq = false );
      inline CVifSCDmaPacket& Mark( tU32 value, bool irq = false );
      inline CVifSCDmaPacket& Flushe( bool irq = false );
      inline CVifSCDmaPacket& Flush( bool irq = false );
      inline CVifSCDmaPacket& Flusha( bool irq = false );
      inline CVifSCDmaPacket& Mscal( tU32 addr, bool irq = false );
      inline CVifSCDmaPacket& Mscnt( bool irq = false );
      inline CVifSCDmaPacket& Mscalf( tU32 addr, bool irq = false );
      inline CVifSCDmaPacket& Stmask( Vifs::tMask mask, bool irq = false );
      inline CVifSCDmaPacket& Strow( const void* rowArray, bool irq = false );
      inline CVifSCDmaPacket& Stcol( const void* colArray, bool irq = false );

      inline CVifSCDmaPacket& OpenUnpack( tU32 mode, tU32 vuAddr, bool dblBuffered,
					  bool masked = false, bool usigned = true, bool irq = false );
      CVifSCDmaPacket& CloseUnpack( void );
      inline CVifSCDmaPacket& CloseUnpack( tU32 wl, tU32 cl );
      inline CVifSCDmaPacket& CloseUnpack( tU32 unpackNUM );

      inline CVifSCDmaPacket& OpenDirect( bool irq = false );
      inline CVifSCDmaPacket& CloseDirect( void );
      inline CVifSCDmaPacket& CloseDirect( tU32 numQuads );

      inline CVifSCDmaPacket& Pad96( void );
      inline CVifSCDmaPacket& Pad128( void );

   private:
      static const tU32 Unused = 0;
      Vifs::tVifCode *pOpenVifCode;
      tU32 uiWL, uiCL;
};


/********************************************
 * CDmaPacket inlines
 */


#define mCheckFreeSpace( __type ) \
   mErrorIf( pNext+sizeof(__type) > (pBase + uiBufferQwordSize*16), "Not enough space in packet!" )
#define mCheckFreeSpaceN( __type, __num )																							\
   mErrorIf( pNext+(__num)*sizeof(__type) > (pBase + uiBufferQwordSize*16), "Not enough space in packet!" )

#define mCheckPktAlignment( __type )												\
   mWarnIf( sizeof(__type) == 16												\
	     && (tU32)pNext & (16 - 1),												\
	     "You're trying to add 16-byte data to this packet on a non-16-byte boundary.. Are you sure this is right?" )

#define mAddData( __type, __data ) *reinterpret_cast<__type*>(pNext) = __data; pNext += sizeof(__type);

template< class dataType > inline void
CDmaPacket::operator += ( const dataType data ) {
   mCheckFreeSpace( dataType );
   mCheckPktAlignment( dataType );

   mAddData( dataType, data );
}

template< class dataType > inline dataType*
CDmaPacket::Add( const dataType data ) {
   mCheckFreeSpace( dataType );
   mCheckPktAlignment( dataType );

   dataType *dataStart = reinterpret_cast<dataType*>(pNext);
   mAddData( dataType, data );
   return dataStart;
}

template< class dataType > inline dataType*
CDmaPacket::Add( const dataType* data, tU32 num ) {
   mCheckFreeSpaceN( dataType, num );
   mCheckPktAlignment( dataType );

   dataType *dataStart = reinterpret_cast<dataType*>(pNext);
   dataType *nextStore = dataStart;
   for ( tU32 i = 0; i < num; i++ )
      *nextStore++ = *data++;
   pNext = reinterpret_cast<tU8*>(nextStore);

   return dataStart;
}

inline void
CDmaPacket::operator += ( const CDmaPacket& otherPkt )
{
   tU32 numBytes = otherPkt.GetByteLength();
   mErrorIf( numBytes & (16-1), "Can only add packets that are an even # of quads." );

   tU32 numQuads = numBytes / 16;
   Add( otherPkt.GetBase(), numQuads );
}

inline tU128*
CDmaPacket::Add( const CDmaPacket& otherPkt )
{
   tU128 *dataStart = reinterpret_cast<tU128*>(pNext);
   operator+=( otherPkt );
   return dataStart;
}

inline void
CDmaPacket::SetDmaChannel( tDmaChannelId channel )
{
#ifndef PS2_LINUX
   dmaChannelId = channel;
#else
   ChannelFd = GetChannelFd(channel);
#endif
}

/*

// I'll leave this here as an example of how I could trick gcc
// into partially specializing the Add templates based on data size...

template< class dataType > inline void
CDmaPacket::operator += ( const dataType data ) {
	Helper<dataType, sizeof(dataType)>::AddSize ( *this, data );
}

template< class dataType, int byteSize >
class CDmaPacket::Helper {
	public:
		static inline void AddSize( CDmaPacket& packet, const dataType data ) {
			mErrorIf( packet.pNext+byteSize > (packet.pBase + packet.uiBufferQwordSize*16), "Not enough space in packet!" );
			mErrorIf( (tU32)packet.pNext & (byteSize - 1), "Free space in packet not properly aligned!" );

			*(dataType*)packet.pNext = data; packet.pNext += byteSize;
		}
};

template< class dataType >
class CDmaPacket::Helper < dataType, 16 > {
	public:
		static inline void AddSize( CDmaPacket& packet, const dataType data ) {
			mErrorIf( packet.pNext+sizeof(data) > (packet.pBase + packet.uiBufferQwordSize*16), "Not enough space in packet!" );
			mErrorIf( (tU32)packet.pNext & (sizeof(data) - 1), "Free space in packet not properly aligned!" );

			union { dataType realData; tU128 qword; } uData = { data };
			*(tU128*)packet.pNext = uData.qword; packet.pNext += 16;
			// mAdd128( uData.qword );
		}
};
*/

#undef mCheckFreeSpace
#undef mCheckPktAlignment
#undef mAddData

/********************************************
 * CSCDmaPacket inlines
 */


// adds

#  define mTemporaryTTEHandler( __dataSize )

#define mCheckTTESpaceN( __dataType, __num )					\
   mErrorIf( (! pOpenTag) && uiTTEBytesLeft < ((__num) * sizeof(__dataType)),	\
             "Don't you think you should open a dma tag before adding data?" )

#define mCheckXferAddrAlign( _addr )								\
   mErrorIf( (tU32)(_addr) & (16-1), "I suggest you only point to qword-aligned memory.." )

template< class dataType > inline dataType*
CSCDmaPacket::Add( const dataType data ) {
   mCheckTTESpaceN(dataType, 1);
   dataType* retValue = CDmaPacket::Add<dataType>( data );
   mTemporaryTTEHandler( sizeof(dataType) );
   return retValue;
}

template< class dataType > inline void
CSCDmaPacket::operator += ( const dataType data ) {
   CSCDmaPacket::Add( data );
}

template< class dataType > inline dataType*
CSCDmaPacket::Add( const dataType* data, tU32 num ) {
   mCheckTTESpaceN(dataType, num);
   dataType *retValue = CDmaPacket::Add<dataType>( data, num );
   mTemporaryTTEHandler( sizeof(dataType) * num );
   return retValue;
}

#undef mTemporaryTTEHandler
#undef mCheckTTESpaceN

inline void
CSCDmaPacket::operator += ( const CDmaPacket& otherPkt ) {
   CDmaPacket::operator+=(otherPkt);
}
inline tU128*
CSCDmaPacket::Add( const CDmaPacket& otherPkt ) {
   return CDmaPacket::Add( otherPkt );
}

// dma tags

inline void
CSCDmaPacket::SetDmaTag( tDmaTag *tag, tU32 QWC, tU32 PCE, tU32 ID, tU32 IRQ, const tU128 *ADDR, tU32 SPR )
{
   tag->QWC = QWC;
   tag->PCE = PCE;
   tag->ID = ID;
   tag->IRQ = IRQ;
   tag->ADDR = (tU64)((tU32)ADDR);
   tag->SPR = SPR;
}

inline CSCDmaPacket&
CSCDmaPacket::CloseTag( void )
{
   mErrorIf( ! pOpenTag, "You called CloseTag(), but no dma tags are open!" );
   mErrorIf( ((tU32)pNext & (16-1)) != 0, "Packet is not qword aligned" );
   // set the qwc field of any open tags.. (- 1 is so that we don't count the qword
   // containing the *pOpenTag)
   if ( pOpenTag ) pOpenTag->QWC = ( ((tU32)pNext - (tU32)pOpenTag) / 16 - 1 );

   pOpenTag = NULL;
   return *this;
}

inline void
CSCDmaPacket::AddDmaTag( tU32 QWC, tU32 PCE, tU32 ID, tU32 IRQ, const tU128 *ADDR, tU32 SPR )
{
   mErrorIf( ((tU32)pNext & 0xf) != 0, "Free space in packet is not aligned properly." );
   mErrorIf( pOpenTag, "You need to close any open dma tags before opening another!" );

   if ( QWC == countQWC ) {
      SetDmaTag( (tDmaTag*)pNext, 0, PCE, ID, IRQ, ADDR, SPR );
      pOpenTag = (tDmaTag*)pNext;
   }
   else {
      SetDmaTag( (tDmaTag*)pNext, QWC, PCE, ID, IRQ, ADDR, SPR );
      pOpenTag = NULL;
   }

   if ( bTTE ) {
      pNext += 8;
      uiTTEBytesLeft = 8;
   }
   else
      pNext += 16;
}

#undef mCheckFreeSpaceN

#ifdef PS2_LINUX
#  define xlateAddr( __va )			\
   (__va) = (__typeof__(__va))GetPhysAddr(__va)
#else
#  define xlateAddr( __va )
#endif

inline CSCDmaPacket&
CSCDmaPacket::Cnt( bool irq, tU32 pce, bool sp ) {
   AddDmaTag( countQWC, pce, DMAC::kCnt, irq, 0, sp );
   return *this;
}

inline CSCDmaPacket&
CSCDmaPacket::Next( const tDmaTag* nextTag, bool irq, bool sp, tU32 pce ) {
   mCheckXferAddrAlign( nextTag );
   xlateAddr(nextTag);
   AddDmaTag( countQWC, pce, DMAC::kNext, irq, (tU128*)nextTag, sp );
   return *this;
}

inline CSCDmaPacket&
CSCDmaPacket::Ref( const void *refData, tU32 dataQWLen, bool irq, bool sp, tU32 pce ) {
   mCheckXferAddrAlign( refData );
   xlateAddr(refData);
   AddDmaTag( dataQWLen, pce, DMAC::kRef, irq, (const tU128*)refData, sp );
   return *this;
}

inline CSCDmaPacket&
CSCDmaPacket::Refs( const void *refData, tU32 dataQWLen, bool irq, bool sp, tU32 pce ) {
   mCheckXferAddrAlign( refData );
   xlateAddr(refData);
   AddDmaTag( dataQWLen, pce, DMAC::kRefs, irq, (const tU128*)refData, sp );
   return *this;
}

inline CSCDmaPacket&
CSCDmaPacket::Refe( const void *refData, tU32 dataQWLen, bool irq, bool sp, tU32 pce ) {
   mCheckXferAddrAlign( refData );
   xlateAddr(refData);
   AddDmaTag( dataQWLen, pce, DMAC::kRefe, irq, (const tU128*)refData, sp );
   return *this;
}

inline CSCDmaPacket&
CSCDmaPacket::Call( const void* nextTag, bool irq, bool sp, tU32 pce ) {
   mCheckXferAddrAlign( nextTag );
   xlateAddr(nextTag);
   AddDmaTag( countQWC, pce, DMAC::kCall, irq, (tU128*)nextTag, sp );
   return *this;
}

inline CSCDmaPacket&
CSCDmaPacket::Call( const CSCDmaPacket& pkt, bool irq, bool sp, tU32 pce ) {
   mCheckXferAddrAlign( pkt.pBase );
#ifdef PS2_LINUX
   tU128 *base = (tU128*)pkt.pBase;
   xlateAddr(base);
   AddDmaTag( countQWC, pce, DMAC::kCall, irq,
	      base, sp );
#else
   AddDmaTag( countQWC, pce, DMAC::kCall, irq,
	      Core::MakePtrNormal((tU128*)pkt.pBase), sp );
#endif

   return *this;
}

inline CSCDmaPacket&
CSCDmaPacket::Ret( bool irq, tU32 pce ) {
   AddDmaTag( countQWC, pce, DMAC::kRet, irq, 0, false );
   return *this;
}

inline CSCDmaPacket&
CSCDmaPacket::End( bool irq, tU32 pce ) {
   AddDmaTag( countQWC, pce, DMAC::kEnd, irq, 0, false );
   return *this;
}

inline CSCDmaPacket&
CSCDmaPacket::Pad96( tU32 padData ) {
   while( (((tU32)pNext + 4) & 0xf) != 0 )
	*this += padData;
   return *this;
}

inline CSCDmaPacket&
CSCDmaPacket::Pad128( tU32 padData ) {
   while( ((tU32)pNext & 0xf) != 0 )
	*this += padData;
   return *this;
}

#undef mCheckFreeSpace
#undef mCheckFreeSpaceN
#undef mCheckPktAlignment
#undef mAddData

#undef mCheckXferAddrAlign

/********************************************
 * CVifSCDmaPacket inlines
 */


// VifCodes

#define mMakeVifCode( _immediate, _num, _cmd, _irq ) ( (tU32)(_immediate) | ((tU32)(_num) << 16) | ((tU32)(_cmd) << 24) | ((tU32)(_irq) << 31) )

inline CVifSCDmaPacket&
CVifSCDmaPacket::Nop( bool irq ) {
   *this += mMakeVifCode(Unused, Unused, Vifs::Opcodes::nop, irq);
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Stcycl( tU32 wl, tU32 cl, bool irq ) {
   uiWL = wl;
   uiCL = cl;
   *this += mMakeVifCode(cl | (wl << 8), Unused, Vifs::Opcodes::stcycl, irq);
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Offset( tU32 offset, bool irq ) {
   *this += mMakeVifCode(offset, Unused, Vifs::Opcodes::offset, irq);
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Base( tU32 base, bool irq ) {
   *this += mMakeVifCode( base, Unused, Vifs::Opcodes::base, irq);
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Itop( tU32 itops, bool irq ) {
   *this += mMakeVifCode( itops, Unused, Vifs::Opcodes::itop, irq);
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Stmod( tU32 mode, bool irq ) {
   *this += mMakeVifCode( mode, Unused, Vifs::Opcodes::stmod, irq);
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Mskpath3( tU32 mask, bool irq ) {
   *this += mMakeVifCode( mask, Unused, Vifs::Opcodes::mskpath3, irq);
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Mark( tU32 value, bool irq ) {
   *this += mMakeVifCode( value, Unused, Vifs::Opcodes::mark, irq);
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Flushe( bool irq ) {
   *this += mMakeVifCode( Unused, Unused, Vifs::Opcodes::flushe, irq);
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Flush( bool irq ) {
   *this += mMakeVifCode( Unused, Unused, Vifs::Opcodes::flush, irq);
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Flusha( bool irq ) {
   *this += mMakeVifCode( Unused, Unused, Vifs::Opcodes::flusha, irq);
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Mscal( tU32 addr, bool irq ) {
   *this += mMakeVifCode( addr, Unused, Vifs::Opcodes::mscal, irq);
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Mscnt( bool irq ) {
   *this += mMakeVifCode( Unused, Unused, Vifs::Opcodes::mscnt, irq);
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Mscalf( tU32 addr, bool irq ) {
   *this += mMakeVifCode( addr, Unused, Vifs::Opcodes::mscalf, irq);
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Stmask( Vifs::tMask mask, bool irq ) {
   *this += mMakeVifCode( Unused, Unused, Vifs::Opcodes::stmask, irq);
   *this += mask;
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Strow( const void* rowArray, bool irq ) {
   const tU32 *wordArray = (const tU32*)rowArray;
   *this += mMakeVifCode( Unused, Unused, Vifs::Opcodes::strow, irq);
   *this += wordArray[0];
   *this += wordArray[1];
   *this += wordArray[2];
   *this += wordArray[3];
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Stcol( const void* colArray, bool irq ) {
   const tU32 *wordArray = (const tU32*)colArray;
   *this += mMakeVifCode( Unused, Unused, Vifs::Opcodes::stcol, irq);
   *this += wordArray[0];
   *this += wordArray[1];
   *this += wordArray[2];
   *this += wordArray[3];
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::OpenUnpack( tU32 mode, tU32 vuAddr, bool dblBuffered, bool masked, bool usigned, bool irq )
{
   mErrorIf( pOpenVifCode != NULL, "There is still another vifcode open." );
   pOpenVifCode = (Vifs::tVifCode*)pNext;
   *this += mMakeVifCode( vuAddr | ((tU32)usigned << 14) | ((tU32)dblBuffered << 15),
			  0,
			  mode | ((tU32)masked << 4) | 0x60, irq );
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::CloseUnpack( tU32 unpackNUM )
{
   // make sure we're u32 aligned and a vifcode is open and it's an unpack
   mAssert( ((tU32)pNext & 0x3) == 0 && pOpenVifCode && ((pOpenVifCode->cmd & 0x60) == 0x60) );
   mAssert( unpackNUM <= 256 );
   pOpenVifCode->num = ( unpackNUM == 256 ) ? 0 : unpackNUM;
   pOpenVifCode = NULL;
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::CloseUnpack( tU32 wl, tU32 cl ) {
   uiWL = wl; uiCL = cl;
   return CloseUnpack();
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::OpenDirect( bool irq ) {
   mAssert( pOpenVifCode == NULL );
   pOpenVifCode = (Vifs::tVifCode*)pNext;
   *this += mMakeVifCode( 0, Unused, Vifs::Opcodes::direct, irq );
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::CloseDirect( tU32 numQuads ) {
   mAssert( pOpenVifCode != NULL && (((tU32)pNext - ((tU32)pOpenVifCode + 4)) & 0xf) == 0 );
   pOpenVifCode->immediate = numQuads;
   pOpenVifCode = NULL;
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::CloseDirect( void ) {
   return CloseDirect( ((tU32)pNext - ((tU32)pOpenVifCode + 4)) / 16 );
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Pad96( void ) {
   CSCDmaPacket::Pad96( mMakeVifCode(Unused, Unused, Vifs::Opcodes::nop, false) );
   return *this;
}

inline CVifSCDmaPacket&
CVifSCDmaPacket::Pad128( void ) {
   CSCDmaPacket::Pad128( mMakeVifCode(Unused, Unused, Vifs::Opcodes::nop, false) );
   return *this;
}

#undef mMakeVifCode

#endif // ps2s_packet_h
