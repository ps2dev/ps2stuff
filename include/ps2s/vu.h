/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2s_vu_h
#define ps2s_vu_h

#include "ps2s/debug.h"
#include "ps2s/types.h"
#include "ps2s/core.h"

/********************************************
	 * common
	 */

namespace VUs {
}

/********************************************
	 * VU0 specific
	 */

namespace VU0 {
inline void CopyQwordsToVU0(uint32_t vu0QwordOffset, uint128_t* mainMemSrc, uint32_t numQwords);
inline void CopyEvenQwordsToVU0(uint32_t vu0QwordOffset, uint128_t* mainMemSrc, uint32_t numQwords);
inline void CopyQwordsFromVU0(uint128_t* mainMemDest, uint32_t vu0QwordOffset, uint32_t numQwords);
inline void CopyEvenQwordsFromVU0(uint128_t* mainMemDest, uint32_t vu0QwordOffset, uint32_t numQwords);
}

/********************************************
	 * VU1 specific
	 */

namespace VU1 {
}

/********************************************
 * VU0 inlines
 */

void VU0::CopyQwordsToVU0(uint32_t vu0QwordOffset, uint128_t* mainMemSrc, uint32_t numQwords)
{
    mAssert(((uint32_t)mainMemSrc & 0xf) == 0);

    // VU0 memory is mapped starting at VU0Code
    // Each qword is 16 bytes, so address = VU0Code + (vu0QwordOffset * 16)
    uint32_t vu0_addr = Core::MemMappings::VU0Code + (vu0QwordOffset << 4);

    asm __volatile__(
        "addu      $8, %0, $zero   \n"
        "sll       $9, %1, 4       \n"
        "addu      $10, $8, $9     \n"
        "addu      $11, %2, $zero  \n"
        "0:                        \n"
        "lqc2      $vf2, 0($8)     \n"
        "addiu     $8, $8, 16      \n"
        "sqc2      $vf2, 0($11)    \n"
        "addiu     $11, $11, 16    \n"
        "bne       $8, $10, 0b     \n"
        "nop                       \n"
        :
        : "r"(mainMemSrc), "r"(numQwords), "r"(vu0_addr)
        : "memory", "$8", "$9", "$10", "$11");
}

void VU0::CopyEvenQwordsToVU0(uint32_t vu0QwordOffset, uint128_t* mainMemSrc, uint32_t numQwords)
{
    mAssert(((uint32_t)mainMemSrc & 0xf) == 0);
    mErrorIf(numQwords & 1, "numQwords must be EVEN!");

    // VU0 memory is mapped starting at VU0Code
    // Each qword is 16 bytes, so address = VU0Code + (vu0QwordOffset * 16)
    uint32_t vu0_addr = Core::MemMappings::VU0Code + (vu0QwordOffset << 4);

    asm __volatile__(
        "addu      $8, %0, $zero   \n"
        "sll       $9, %1, 4       \n"
        "addu      $10, $8, $9     \n"
        "addu      $11, %2, $zero  \n"
        "0:                        \n"
        "lqc2      $vf2, 0($8)     \n"
        "addiu     $8, $8, 16      \n"
        "lqc2      $vf3, 0($8)     \n"
        "addiu     $8, $8, 16      \n"
        "sqc2      $vf2, 0($11)    \n"
        "addiu     $11, $11, 16    \n"
        "sqc2      $vf3, 0($11)    \n"
        "addiu     $11, $11, 16    \n"
        "bne       $8, $10, 0b     \n"
        "nop                       \n"
        :
        : "r"(mainMemSrc), "r"(numQwords), "r"(vu0_addr)
        : "memory", "$8", "$9", "$10", "$11");
}

void VU0::CopyQwordsFromVU0(uint128_t* mainMemDest, uint32_t vu0QwordOffset, uint32_t numQwords)
{
    mAssert(((uint32_t)mainMemDest & 0xf) == 0);

    // VU0 memory is mapped starting at VU0Code
    // Each qword is 16 bytes, so address = VU0Code + (vu0QwordOffset * 16)
    uint32_t vu0_addr = Core::MemMappings::VU0Code + (vu0QwordOffset << 4);

    asm __volatile__(
        "addu      $8, %0, $zero   \n"
        "sll       $9, %1, 4       \n"
        "addu      $10, $8, $9     \n"
        "addu      $11, %2, $zero  \n"
        "0:                        \n"
        "lqc2      $vf1, 0($11)    \n"
        "addiu     $11, $11, 16    \n"
        "sqc2      $vf1, 0($8)     \n"
        "addiu     $8, $8, 16      \n"
        "bne       $8, $10, 0b     \n"
        "nop                       \n"
        :
        : "r"(mainMemDest), "r"(numQwords), "r"(vu0_addr)
        : "memory", "$8", "$9", "$10", "$11");
}

void VU0::CopyEvenQwordsFromVU0(uint128_t* mainMemDest, uint32_t vu0QwordOffset, uint32_t numQwords)
{
    mAssert(((uint32_t)mainMemDest & 0xf) == 0);
    mErrorIf(numQwords & 1, "numQwords must be EVEN!");

    // VU0 memory is mapped starting at VU0Code
    // Each qword is 16 bytes, so address = VU0Code + (vu0QwordOffset * 16)
    uint32_t vu0_addr = Core::MemMappings::VU0Code + (vu0QwordOffset << 4);

    asm __volatile__(
        "addu      $8, %0, $zero   \n"
        "sll       $9, %1, 4       \n"
        "addu      $10, $8, $9     \n"
        "addu      $11, %2, $zero  \n"
        "0:                        \n"
        "lqc2      $vf1, 0($11)    \n"
        "addiu     $11, $11, 16    \n"
        "sqc2      $vf1, 0($8)     \n"
        "addiu     $8, $8, 16      \n"
        "lqc2      $vf2, 0($11)    \n"
        "addiu     $11, $11, 16    \n"
        "sqc2      $vf2, 0($8)     \n"
        "addiu     $8, $8, 16      \n"
        "bne       $8, $10, 0b     \n"
        "nop                       \n"
        :
        : "r"(mainMemDest), "r"(numQwords), "r"(vu0_addr)
        : "memory", "$8", "$9", "$10", "$11");
}

#endif // ps2s_vu_h
