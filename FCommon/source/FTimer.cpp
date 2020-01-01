#include "FTimer.h"
#include <windows.h>
#include "FUtilities.h"

FTimer::FTimer()
{
	Restart();
	myPausedTime = 0.0;
	myLastUnpauseTimeStamp = 0.0;
	myIsPaused = false;
}

void FTimer::Pause()
{
	if (myIsPaused)
		return;

	myPausedTime = GetTimeMS();
	myIsPaused = true;
	Restart();
	FLOG("Paused");
}

void FTimer::Unpause()
{
	if (!myIsPaused)
		return;

//	myPausedTime += GetTimeMS();
	myLastUnpauseTimeStamp = myPausedTime + GetTimeMS();
	myIsPaused = false;
	Restart();
	FLOG("Unpaused");
}

void FTimer::Restart()
{
	QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
	QueryPerformanceCounter((LARGE_INTEGER *)&ctr2);
	QueryPerformanceCounter((LARGE_INTEGER *)&ctr1);
}

double FTimer::GetTimeMS()
{
	if (myIsPaused)
		return myLastUnpauseTimeStamp + myPausedTime;

	QueryPerformanceCounter((LARGE_INTEGER *)&ctr2);
	return myPausedTime + (((ctr2 - ctr1) * 1.0 / freq) * 1000);
}

double FTimer::GetTimeUS()
{
	if (myIsPaused)
		return (myLastUnpauseTimeStamp + myPausedTime) * 1000.0;

	QueryPerformanceCounter((LARGE_INTEGER *)&ctr2);
	return (myPausedTime * 1000.0) + ((ctr2 - ctr1) * 1.0 / freq);
}

FTimer::~FTimer()
{
}
