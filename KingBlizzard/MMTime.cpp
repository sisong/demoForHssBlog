//MMTime.cpp

#include "MMTime.h"
#include "Windows.h"
#include "mmSystem.h"

unsigned long mmGetTime()
{
    return timeGetTime();
}


void mmSleep(unsigned long time_ms)
{
    Sleep(time_ms);
}
