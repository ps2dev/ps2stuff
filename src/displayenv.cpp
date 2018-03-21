/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

/********************************************
 * includes
 */

#include "ps2s/displayenv.h"
#include "ps2s/utils.h"

namespace GS {

/********************************************
    * static stuff
    */

int CDisplayEnv::GS_fd = -1;

/********************************************
    * methods
    */

CDisplayEnv::CDisplayEnv(void)
{
    printf("this = %p, &gsrPMode = %p\n", this, &gsrPMode);
    *(tU64*)&gsrPMode = (tU64)0;
    puts("after");

    gsrPMode.CRTMD = 0;

    // some reasonable (?) defaults
    GsDisplayModeIs(DisplayModes::ntsc);
    SetUseReadCircuit1(false);
    SetUseReadCircuit2(true);
    BlendRC1WithRC2();
    // BlendUsingConstAlpha( 0 );
    BlendUsingRC1Alpha();
}

void CDisplayEnv::DontBlend(void)
{
    BlendRC1WithBG();
    BlendUsingConstAlpha(0xff);
}

void CDisplayEnv::SetFB(tGsrDispFB* dispFBReg,
    unsigned int wordAddr, unsigned int bufWidth,
    unsigned int xOffsetInBuf, unsigned int yOffsetInBuf,
    unsigned int psm)
{
    mErrorIf(wordAddr & 2047, "Frame buffers must be page aligned (8k)");

    dispFBReg->FBP = wordAddr / 2048; // page aligned
    dispFBReg->FBW = bufWidth / 64;   // width = num pixels / 64
    dispFBReg->PSM = psm;
    dispFBReg->DBX = xOffsetInBuf; // pixel offset
    dispFBReg->DBY = yOffsetInBuf; // pixel offset
}

void CDisplayEnv::SetDisplay(tGsrDisplay* displayReg,
    unsigned int width, unsigned int height,
    unsigned int screenX, unsigned int screenY,
    unsigned int magH, unsigned int magV)
{
    mErrorIf(magH == 0 || magV == 0, "magH and magV can't be zero.");

    //        displayReg->DX = 302;
    //        displayReg->DY = 24;
    //        displayReg->MAGH = 0;
    //        displayReg->MAGV = 0;
    //        displayReg->DW = 1279;
    //        displayReg->DH = 719;

    // printf("this = %p, displayReg = %p\n", this, displayReg );

    displayReg->DX   = HorizontalOverScan + screenX * magH;
    displayReg->DY   = VerticalOverScan + screenY * magV;
    displayReg->MAGH = magH - 1;
    displayReg->MAGV = magV - 1;
    displayReg->DW   = width * magH; // width = num pixels * clocks per pixel
    displayReg->DH   = (height - 1) * magV;
}

void CDisplayEnv::SendSettings(void)
{
    using namespace GS::ControlRegs;
    *(tU64*)pmode = *(tU64*)&gsrPMode;
    *(tU64*)dispfb1 = *(tU64*)&gsrDispFB1;
    *(tU64*)dispfb2 = *(tU64*)&gsrDispFB2;
    *(tU64*)display1 = *(tU64*)&gsrDisplay1;
    *(tU64*)display2 = *(tU64*)&gsrDisplay2;
    *(tU64*)bgcolor = *(tU64*)&gsrBGColor;
}

} // namespace GS
