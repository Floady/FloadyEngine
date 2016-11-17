#include "FTimer.h"
#include <windows.h>


FTimer::FTimer()
{
	Restart();
}

void FTimer::Restart()
{
	QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
	QueryPerformanceCounter((LARGE_INTEGER *)&ctr1);
}

double FTimer::GetTimeMS()
{
	QueryPerformanceCounter((LARGE_INTEGER *)&ctr2);
	return ((ctr2 - ctr1) * 1.0 / freq);
}

FTimer::~FTimer()
{
}
