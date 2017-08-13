#include "FProfiler.h"
#include "FGame.h"
#include "FDynamicText.h"
#include "windows.h"
#define USE_PIX
#include <pix3.h>

FProfiler* FProfiler::ourInstance = nullptr;
unsigned int FProfiler::ourHistoryBufferCount = 120;
static const float textHeight = 0.04f;
static const float textWidth = 0.5f;
FProfiler * FProfiler::GetInstance()
{
	if (!ourInstance)
		ourInstance = new FProfiler();

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

	if (myTimings.find(aName) == myTimings.end())
	{
		DWORD dwWaitResult = WaitForSingleObject(
			ghMutex,    // handle to mutex
			INFINITE);  // no time-out interval
		
		if (myTimings.find(aName) == myTimings.end())
		{
			myTimings[aName].resize(ourHistoryBufferCount);

			// add a label for it
			myLabels.push_back(new FDynamicText(FGame::GetInstance()->GetRenderer(), FVector3(-1.0f, -1.0f, 0.0), "FPS Counter", textWidth, textHeight, true, true));
		}
		ReleaseMutex(ghMutex);
	}

	myTimings[aName][myCurrentFrame % ourHistoryBufferCount].myTime += aTime;
	myTimings[aName][myCurrentFrame % ourHistoryBufferCount].myOccurences++;
}

void FProfiler::StartFrame()
{
	if (myIsPaused)
		return;

	myCurrentFrame++;

	for (auto& item : myTimings)
	{
		item.second[myCurrentFrame % ourHistoryBufferCount].Reset();
	}
}

void FProfiler::Render()
{
	if (!myIsVisible)
		return;

	int labelIdx = 0;

	char buff[128];

	FVector3 pos = FVector3(-1, 0.8, 0);
	for (auto item : myTimings)
	{
		if (labelIdx > myLabels.size())
			break;

		sprintf(buff, "%s : %f   %u ", item.first, item.second[myCurrentFrame % ourHistoryBufferCount].myTime, item.second[myCurrentFrame % ourHistoryBufferCount].myOccurences);
		myLabels[labelIdx]->SetText(buff);
		myLabels[labelIdx]->SetPos(pos);

		pos.y -= textHeight;
		pos.y -= 0.01; // custom spacing

		if (pos.y < -1)
		{
			pos.y = 1;
			pos.x = 0;
		}

		labelIdx++;
	}
}

void FProfiler::SetVisible(bool aVisible)
{
	myIsVisible = aVisible;

	for (FDynamicText* label : myLabels)
	{
		if(myIsVisible)
			FGame::GetInstance()->GetRenderer()->GetSceneGraph().AddObject(label, true);
		else
			FGame::GetInstance()->GetRenderer()->GetSceneGraph().RemoveObject(label);
	}
}

void scopedMarker::Start()
{
	PIXBeginEvent(FD3d12Renderer::GetInstance()->GetCommandQueue(), 0, myName);
	myTimer.Restart();
}

scopedMarker::~scopedMarker()
{
	double time = myTimer.GetTimeMS();
	FProfiler::GetInstance()->AddTiming(myName, time);
	PIXEndEvent(FD3d12Renderer::GetInstance()->GetCommandQueue());
}
