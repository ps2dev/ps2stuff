/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include "ps2s/timer.h"

#include <stdio.h>

/********************************************
 * static member definitions
 */

const unsigned int CHTimer::InitialChildrenSize;
CHTimer* CHTimer::pTopTimer;
std::stack<CHTimer*, std::vector<CHTimer*> >* CHTimer::TimerStack;
CEETimer* CHTimer::pRealTimer;

void CHTimer::InitHierarchy(CEETimer* realTimer, CEETimer::tResolution resolution)
{
    // create a dummy timer to be the top of the hierarchy
    pTopTimer  = new CHTimer("topTimer", true);
    TimerStack = new std::stack<CHTimer*, std::vector<CHTimer*> >;
    TimerStack->push(pTopTimer);

    pRealTimer = realTimer;
    pRealTimer->SetResolution(resolution);
    pRealTimer->Start();
}

CHTimer::~CHTimer()
{
    for (unsigned int i = 0; i < pTopTimer->Children.size(); i++)
        delete pTopTimer->Children[i];
}

void CHTimer::DeleteHierarchy()
{
    delete TimerStack;
    delete pTopTimer;
}

void CHTimer::DisplayHierarchy(void)
{
    for (unsigned int i = 0; i < pTopTimer->Children.size(); i++)
        pTopTimer->Children[i]->Display(std::string());

    printf("\n");
}

void CHTimer::UpdateHierarchy(void)
{
    pTopTimer->Update();
    pRealTimer->Start();
}

void CHTimer::StartTimer(const char* name, bool enableDisplay)
{
    CHTimer* timer;
    // is this the first time we have been called under the current node?
    if ((timer = TimerStack->top()->GetChild(name)) == NULL)
        TimerStack->top()->AddChild(timer = new CHTimer(name, enableDisplay));

    // make this timer the current timer
    TimerStack->push(timer);
    timer->Start();
}

unsigned int
CHTimer::StopTimer(const char* name)
{
    unsigned int ticks = TimerStack->top()->Stop();
    TimerStack->pop();

    return ticks;
}

/********************************************
 * methods
 */

void CHTimer::Display(const std::string& path)
{
    printf("%s%.2f%% - %s\n",
        path.c_str(),
        100.0f * (float)uiElapsedTicks / (float)pRealTimer->GetTicksPerFrame(),
        &Name.c_str()[REAL_NAME_OFFSET]);

    std::string newPath = path + "\t";
    for (unsigned int i = 0; i < Children.size(); i++)
        Children[i]->Display(newPath);
}

void CHTimer::Update(void)
{
    uiElapsedTicks = 0;
    for (unsigned int i = 0; i < Children.size(); i++)
        Children[i]->Update();
}
