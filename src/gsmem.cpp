/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include <string>

#include "ps2s/math.h"
#include "ps2s/gsmem.h"
#include "ps2s/debug.h"

/********************************************
 * CMemSlot
 */

namespace GS {

CMemSlot::~CMemSlot()
{
   if ( BoundMemArea )
      BoundMemArea->Unbind();
}

   void
   CMemSlot::Bind( CMemArea &memArea, int curFrame )
   {
      if ( BoundMemArea )
	 BoundMemArea->Unbind();

      BoundMemArea = &memArea;
      memArea.Bind( *this );

      RecordAccess( curFrame );
   }

   void
   CMemSlot::Unbind()
   {
      BoundMemArea->Unbind();
      BoundMemArea = NULL;

      List->MakeSlotLRU(this);
   }

// for Print just below

#define mCase( _psm ) case _psm:						\
	return std::string(mExpandQuote(_psm));

   std::string
   GetPSMString( GS::tPSM psm )
   {
      using namespace GS;
      switch (psm) {
	 mCase( kPsm32 );
	 mCase( kPsm24 );
	 mCase( kPsm16 );
	 mCase( kPsm16s );
	 mCase( kPsm8 );
	 mCase( kPsm8h );
	 mCase( kPsm4 );
	 mCase( kPsm4hh );
	 mCase( kPsm4hl );
	 mCase( kPsmz32 );
	 mCase( kPsmz24 );
	 mCase( kPsmz16 );
	 mCase( kPsmz16s );
	 default:
	    mError( "Unknown PSM!!" );
	    return NULL;
      }
   }

#undef mCase


   void
   CMemSlot::Print()
   {
      printf("[%3d, %3d]\t PixFormat: %s\t LastFrameUsed: %d\t",
	     FirstPage, FirstPage + PageLength - 1,
	     GetPSMString(PixFormat).c_str(),
	     LastFrameUsed);
      if ( BoundMemArea ) {
	 printf("bound");
      }
      else printf("free ");

      printf("\n");
   }

/********************************************
 * CMemSlotList
 */

   CMemSlotList::~CMemSlotList()
   {
      RemoveAllSlots();
   }

   void
   CMemSlotList::PrintSlots()
   {
      int count;
      tSlotIter curSlot = Slots.begin();
      for ( count = 0; curSlot != Slots.end(); curSlot++, count++ )
	 (*curSlot)->Print();
      if ( count > 0 ) printf("\n");
   }

   void
   CMemSlotList::RemoveSlot( CMemSlot *slot )
   {
      bool found = false;
      tSlotIter curSlot = Slots.begin();
      for ( ; curSlot != Slots.end(); curSlot++ )
	 if ( *curSlot == slot ) { found = true; break; }

      mErrorIf( ! found, "This list does not contain the specified slot!" );

      Slots.erase( curSlot );
   }

   void
   CMemSlotList::MakeSlotLRU( CMemSlot *slot )
   {
      RemoveSlot( slot );
      Slots.push_back( slot );
   }

   void
   CMemSlotList::MakeSlotMRU( CMemSlot *slot )
   {
      RemoveSlot( slot );
      Slots.push_front( slot );
   }

   void
   CMemSlotList::RemoveAllSlots()
   {
      tSlotIter curSlot = Slots.begin();
      for ( ; curSlot != Slots.end(); curSlot++ )
	 delete *curSlot;
      Slots.clear();
   }

/********************************************
 * CMemManager
 */

   CMemManager::~CMemManager()
   {
      RemoveAllSlots();
   }

   void
   CMemManager::RemoveAllSlots()
   {
      tSlotListIter curList = SlotLists.begin();
      for ( ; curList != SlotLists.end(); curList++ )
	 delete *curList;
      SlotLists.clear();

      LockedSlots.RemoveAllSlots();
   }

   CMemSlot*
   CMemManager::AddSlot( int firstPage, int pageLength, GS::tPSM pixFormat )
   {
      // is there already a list of slots of this type?
      CMemSlot *newSlot = new CMemSlot( firstPage, pageLength, pixFormat );
      mErrorIf ( newSlot == NULL, "Failed to create slot" );
      CMemSlotList *slotList = FindSlotListOfType( *newSlot );

      // did we find an existing list?
      if ( slotList == NULL ) {
	 // need to create a new list to hold this type of memory slot
	 slotList = new CMemSlotList( pageLength, pixFormat );
	 AddSlotList( slotList );
      }

      // add slot to the list
      slotList->AddSlot( newSlot );

      return newSlot;
   }

   void
   CMemManager::Alloc( CMemArea &memArea )
   {
      using namespace GS;
      switch( memArea.GetPixFormat() ) {
	 case kPsm4: Alloc4( memArea ); break;
	 case kPsm8h:
	 case kPsm8: Alloc8( memArea ); break;
	 case kPsm16: Alloc16( memArea ); break;
	 case kPsm24: Alloc24( memArea ); break;
	 case kPsm32: Alloc32( memArea ); break;
	 default:
	    mError( "Can't allocate MemAreas of pixel format %d", memArea.GetPixFormat() );
      }
   }

   int
   CMemManager::GetFreePriority( CMemSlot &slot, int areaPageLength )
   {
      int age = CurFrame - slot.GetLastFrameUsed();
      int size = 10 - (slot.GetPageLength() - areaPageLength);
      size = Math::Clamp( size, 0, 10 );

      return age + size + 10 * (! slot.IsBound());
   }

   void
   CMemManager::Alloc4( CMemArea &memArea )
   {
      CMemSlot *slot4 = FindLRUSlot( GS::kPsm4, memArea.GetPageLength() );
      CMemSlot *slot4hh = FindLRUSlot( GS::kPsm4hh, memArea.GetPageLength() * 4 );
      CMemSlot *slot4hl = FindLRUSlot( GS::kPsm4hl, memArea.GetPageLength() * 4 );
      CMemSlot *slot32 = FindLRUSlot( GS::kPsm32, memArea.GetPageLength() );

      int priority4 = -1, priority4hh = -1, priority4hl = -1, priority32 = -1;
      if ( slot4 )
	 priority4 = GetFreePriority( *slot4, memArea.GetPageLength() );
      if ( slot4hh )
	 priority4hh = GetFreePriority( *slot4hh, memArea.GetPageLength() * 4 );
      if ( slot4hl )
	 priority4hl = GetFreePriority( *slot4hl, memArea.GetPageLength() * 4 );
      if ( slot32 )
	 priority32 = GetFreePriority( *slot32, memArea.GetPageLength() );

      // stack the deck
      if ( slot4hh ) priority4hh += 2;
      if ( slot4hl ) priority4hl += 2;
      if ( slot4 ) priority4 += 1;

      int maxPriority = Math::Max( priority4, Math::Max(priority4hh,
							Math::Max(priority4hl,
								  priority32)) );
      mErrorIf( maxPriority == -1, "Failed to allocate a GS mem slot." );

      CMemSlot *slot = NULL;
      if ( maxPriority == priority4 ) slot = slot4;
      else if ( maxPriority == priority4hh ) slot = slot4hh;
      else if ( maxPriority == priority4hl ) slot = slot4hl;
      else if ( maxPriority == priority32 ) slot = slot32;

      slot->Bind( memArea, CurFrame );
   }

   void
   CMemManager::Alloc8( CMemArea &memArea )
   {
      CMemSlot *slot8 = FindLRUSlot( GS::kPsm8, memArea.GetPageLength() );
      CMemSlot *slot8h = FindLRUSlot( GS::kPsm8h, memArea.GetPageLength() * 4 );
      CMemSlot *slot32 = FindLRUSlot( GS::kPsm32, memArea.GetPageLength() );

      int priority8 = -1, priority8h = -1, priority32 = -1;
      if ( slot8 )
	 priority8 = GetFreePriority( *slot8, memArea.GetPageLength() );
      if ( slot8h )
	 priority8h = GetFreePriority( *slot8h, memArea.GetPageLength() * 4 );
      if ( slot32 )
	 priority32 = GetFreePriority( *slot32, memArea.GetPageLength() );

      // stack the deck
      if ( slot8h ) priority8h += 2;
      if ( slot8 ) priority8 += 1;

      int maxPriority = Math::Max( priority8, Math::Max(priority8h, priority32) );
      mErrorIf( maxPriority == -1,
		"Failed to allocate a %d page GS mem slot.", memArea.GetPageLength() );

      CMemSlot *slot = NULL;
      if ( maxPriority == priority8 ) slot = slot8;
      else if ( maxPriority == priority8h ) slot = slot8h;
      else if ( maxPriority == priority32 ) slot = slot32;

      slot->Bind( memArea, CurFrame );
   }

   void
   CMemManager::Alloc16( CMemArea &memArea )
   {
      CMemSlot *slot16 = FindLRUSlot( GS::kPsm16, memArea.GetPageLength() );
      CMemSlot *slot32 = FindLRUSlot( GS::kPsm32, memArea.GetPageLength() );

      int priority16 = -1, priority32 = -1;
      if ( slot16 )
	 priority16 = GetFreePriority( *slot16, memArea.GetPageLength() );
      if ( slot32 )
	 priority32 = GetFreePriority( *slot32, memArea.GetPageLength() );

      // stack the deck
      if ( slot16 ) priority16 += 1;

      int maxPriority = Math::Max( priority16, priority32 );
      mErrorIf( maxPriority == -1, "Failed to allocate a GS mem slot." );

      CMemSlot *slot = NULL;
      if ( maxPriority == priority16 ) slot = slot16;
      else if ( maxPriority == priority32 ) slot = slot32;

      slot->Bind( memArea, CurFrame );
   }

   void
   CMemManager::Alloc24( CMemArea &memArea )
   {
      CMemSlot *slot24 = FindLRUSlot( GS::kPsm24, memArea.GetPageLength() );
      CMemSlot *slot32 = FindLRUSlot( GS::kPsm32, memArea.GetPageLength() );

      int priority24 = -1, priority32 = -1;
      if ( slot24 )
	 priority24 = GetFreePriority( *slot24, memArea.GetPageLength() );
      if ( slot32 )
	 priority32 = GetFreePriority( *slot32, memArea.GetPageLength() );

      // stack the deck
      if ( slot24 ) priority24 += 1;

      int maxPriority = Math::Max( priority24, priority32 );
      mErrorIf( maxPriority == -1, "Failed to allocate a GS mem slot." );

      CMemSlot *slot = NULL;
      if ( maxPriority == priority24 ) slot = slot24;
      else if ( maxPriority == priority32 ) slot = slot32;

      slot->Bind( memArea, CurFrame );
   }

   void
   CMemManager::Alloc32( CMemArea &memArea )
   {
      CMemSlot *slot32 = FindLRUSlot( GS::kPsm32, memArea.GetPageLength() );

      mErrorIf( slot32 == NULL, "Failed to allocate a GS mem slot" );

      slot32->Bind( memArea, CurFrame );
   }

   void
   CMemManager::Free( CMemArea &memArea )
   {

   }

   CMemSlot*
   CMemManager::FindLRUSlot( GS::tPSM pixFormat, int pageLength )
   {
      CMemSlot *slot = NULL;
      CMemSlotList *slotList = NULL;
      tSlotListIter curList = SlotLists.begin();
      for ( ; curList != SlotLists.end(); curList++ ) {
	 if ( (*curList)->GetPixFormat() == pixFormat
	      && (*curList)->GetPageLength() >= pageLength ) {
	    slotList = *curList;
	    if ( slotList &&
		 (slot = slotList->GetLRUSlot()) )
	       break;
	 }
      }

      return slot;
   }

   void
   CMemManager::AddSlotList( CMemSlotList *newList )
   {
      // we want the lists in increasing page size order
      tSlotListIter curList = SlotLists.begin();
      CMemSlotList *prevList = NULL;
      for ( ; curList != SlotLists.end(); curList++ ) {
	 if ( ( (*curList)->GetPixFormat() == newList->GetPixFormat()
		&& (*curList)->GetPageLength() >= newList->GetPageLength() )
	      || ( (*curList)->GetPixFormat() != newList->GetPixFormat()
		   && prevList && prevList->GetPixFormat() == newList->GetPixFormat() ) ) {
	    break;
	 }
	 prevList = *curList;
      }
      SlotLists.insert( curList, newList );
   }

   CMemSlotList*
   CMemManager::FindSlotListOfType( const CMemSlot &slot )
   {
      CMemSlotList *slotList = NULL;
      tSlotListIter curList = SlotLists.begin();
      for ( ; curList != SlotLists.end(); curList++ ) {
	 if ( (*curList)->HasSameTypeAs(slot) )
	    slotList = *curList;
      }
      return slotList;
   }

   void
   CMemManager::PrintAllocation()
   {
      printf("\n\nGS Memory Allocation:\n\n");

      printf("Locked slots:\n\n");
      LockedSlots.PrintSlots();

      printf("\nUnlocked slots:\n\n");
      tSlotListIter curList = SlotLists.begin();
      for ( ; curList != SlotLists.end(); curList++ )
	 (*curList)->PrintSlots();
   }

/********************************************
 * CMemArea
 */

   CMemManager *CMemArea::MemManager;

   CMemArea::CMemArea( int width, int height,
		       GS::tPSM pixFormat,
		       tMemAlignment alignment = kAlignPage )
      : Width(width), Height(height),
	Slot(NULL), GSWordAddr(0),
	PixFormat(pixFormat), Alignment(alignment)
   {
      int width32 = width, height32 = height;
      XformDimensions( &width32, &height32, pixFormat );

      // figure out how much mem this area needs
      using Math::DivUp;
      int wPages = DivUp( width32, 64 );
      int hPages = DivUp( height32, 32 );
      PageLength = wPages * hPages;
   }

   CMemArea::~CMemArea()
   {
       // Ensure there are no outstanding references

       Free();
   }

   void
   CMemArea::Alloc()
   {
      MemManager->Alloc(*this);
   }

   void
   CMemArea::Free()
   {
      if (Slot)
         Slot->Unbind();
   }

   void
   CMemArea::Bind( CMemSlot &slot )
   {
      Slot = &slot;
      GSWordAddr = slot.GetFirstPage() * 2048;
      PixFormat = slot.GetPixFormat();
   }

   void
   CMemArea::Unbind()
   {
      Slot = NULL;
   }

   void
   CMemArea::XformDimensions( int* width, int* height, GS::tPSM pixFormat )
   {
      using Math::DivUp;

      // convert the dimensions into 32-bit buffer dimensions
      using namespace GS;
      switch ( pixFormat ) {
	 case kPsm4:
	    *width = DivUp( *width, 2 ); *height = DivUp( *height, 4 );
	    break;
	 case kPsm4hl:
	 case kPsm4hh:
	    break;
	 case kPsm8:
	    *width = DivUp( *width, 2 ); *height = DivUp( *height, 2 );
	    break;
	 case kPsm8h:
	    break;
	 case kPsm16:
	 case kPsm16s:
	 case kPsmz16:
	 case kPsmz16s:
	    *height = DivUp( *height, 2 );
	    break;
	 case kPsm24:
	 case kPsmz24:
	    break;
	 case kPsm32:
	 case kPsmz32:
	    break;
	 case kInvalidPsm:
	    mError("shouldn't get here");
	    break;
      }
   }

} // namespace GS
