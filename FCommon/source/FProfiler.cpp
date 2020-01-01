#include "FProfiler.h"
#include "windows.h"
#include "FJobSystem.h"
#include "FUtilities.h"


FProfiler* FProfiler::ourInstance = nullptr;
unsigned int FProfiler::ourHistoryBufferCount = 120;
static const float textHeight = 0.03f;
static const float textWidth = 0.6f;

FTimer scopedMarker::myTimer;

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

	scopedMarker::myTimer.Restart();
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

void FProfiler::ProcessMarker(const scopedMarker& aMarker)
{
	if (myIsPaused)
		return;
	myIsPaused = true;
	if (myTimings.find(aMarker.myName) == myTimings.end())
	{
		DWORD dwWaitResult = WaitForSingleObject(
			ghMutex,    // handle to mutex
			INFINITE);  // no time-out interval

		if (myTimings.find(aMarker.myName) == myTimings.end())
		{
			myTimings[aMarker.myName].myFrameTimings.resize(ourHistoryBufferCount);
		}

		ReleaseMutex(ghMutex);
	}

	myTimings[aMarker.myName].myFrameTimings[myCurrentFrame % ourHistoryBufferCount].myTime += (aMarker.myEndTime - aMarker.myStartTime);
	myTimings[aMarker.myName].myFrameTimings[myCurrentFrame % ourHistoryBufferCount].myStartTime = aMarker.myStartTime;
	myTimings[aMarker.myName].myFrameTimings[myCurrentFrame % ourHistoryBufferCount].myEndTime = aMarker.myEndTime;
	myTimings[aMarker.myName].myFrameTimings[myCurrentFrame % ourHistoryBufferCount].myOccurences++;
	myTimings[aMarker.myName].myTotalTime.myTime += (aMarker.myEndTime - aMarker.myStartTime);
	myTimings[aMarker.myName].myTotalTime.myOccurences++;
	myIsPaused = false;
}

void FProfiler::AddTimedMarker(const char* aMarkerName)
{
	MarkerInfo m;
	m.myName = aMarkerName;
	m.myStartTime = scopedMarker::myTimer.GetTimeMS();
	myMarkers.push_back(m);
}
void FProfiler::SetPause(bool aPaused)
{
	myIsPaused = aPaused;
	if (myIsPaused)
		scopedMarker::myTimer.Pause();
	else
		scopedMarker::myTimer.Unpause();
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

void FProfiler::RenderIMGUI()
{

}

void scopedMarker::Start()
{
	myStartTime = myTimer.GetTimeMS();
}

scopedMarker::~scopedMarker()
{
	if (!FProfiler::GetInstanceNoCreate())
		return;

	myEndTime = myTimer.GetTimeMS();
	FProfiler::GetInstance()->ProcessMarker(*this);
}
