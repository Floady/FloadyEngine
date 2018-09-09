#include "FProfiler.h"
#include "windows.h"
#include "FJobSystem.h"
#include "FUtilities.h"

FProfiler* FProfiler::ourInstance = nullptr;
unsigned int FProfiler::ourHistoryBufferCount = 120;
static const float textHeight = 0.03f;
static const float textWidth = 0.6f;
FProfiler * FProfiler::GetInstance()
{
	if (!ourInstance)
		ourInstance = new FProfiler();

	return ourInstance;
}

FProfiler * FProfiler::GetInstanceNoCreate()
{
	return ourInstance;
}

static HANDLE ghMutex;
FProfiler::FProfiler()
{
	myCurrentFrame = 0;
	myIsPaused = 0;
	myIsVisible = 0;

	ghMutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL);             // unnamed mutex

}

FProfiler::~FProfiler()
{
}

static LONG myCanAdd = 0;
void FProfiler::AddTiming(const char * aName, double aTime)
{
	if (myIsPaused)
		return;
	myIsPaused = true;
	if (myTimings.find(aName) == myTimings.end())
	{
		DWORD dwWaitResult = WaitForSingleObject(
			ghMutex,    // handle to mutex
			INFINITE);  // no time-out interval
		
		if (myTimings.find(aName) == myTimings.end())
		{
			myTimings[aName].myFrameTimings.resize(ourHistoryBufferCount);
		}

		ReleaseMutex(ghMutex);
	}

	myTimings[aName].myFrameTimings[myCurrentFrame % ourHistoryBufferCount].myTime += aTime;
	myTimings[aName].myFrameTimings[myCurrentFrame % ourHistoryBufferCount].myOccurences++;
	myTimings[aName].myTotalTime.myTime += aTime;
	myTimings[aName].myTotalTime.myOccurences++;
	myIsPaused = false;
}

void FProfiler::StartFrame()
{
	if (myIsPaused)
		return;

	myCurrentFrame++;

	for (auto& item : myTimings)
	{
		item.second.myFrameTimings[myCurrentFrame % ourHistoryBufferCount].Reset();
	}
}

void scopedMarker::Start()
{
	myTimer.Restart();
}

scopedMarker::~scopedMarker()
{
	if (!FProfiler::GetInstanceNoCreate())
		return;

	double time = myTimer.GetTimeMS();
	FProfiler::GetInstance()->AddTiming(myName, time);
}
