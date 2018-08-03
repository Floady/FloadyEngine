#include "FProfiler.h"
#include "FGame.h"
#include "FDynamicText.h"
#include "windows.h"
#include "FJobSystem.h"
#include "FUtilities.h"

#if GRAPHICS_DEBUGGING
#include <pix.h>
#else
#define USE_PIX
#include <pix3.h>
#endif

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

			// add a label for it
			//myLabels.push_back(new FDynamicText(FGame::GetInstance()->GetRenderer(), FVector3(-1.0f, -1.0f, 0.0), "FPS Counter", textWidth, textHeight, true, true));
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

void FProfiler::Render()
{
	if (!myIsVisible)
		return;

	int labelIdx = 0;

	char buff[128];

	FVector3 pos = FVector3(-1, 0.8f, 0);
	for (const auto& item : myTimings)
	{
		if (labelIdx >= myLabels.size())
		{
			myLabels.push_back(new FDynamicText(FGame::GetInstance()->GetRenderer(), FVector3(-1.0f, -1.0f, 0.0), "FPS Counter", textWidth, textHeight, true, true));
		}

		sprintf(buff, "%s : %4.2f   %u   total: %4.2f I %u", item.first,
			item.second.myFrameTimings[myCurrentFrame % ourHistoryBufferCount].myTime,
			item.second.myFrameTimings[myCurrentFrame % ourHistoryBufferCount].myOccurences,
			item.second.myTotalTime.myTime,
			item.second.myTotalTime.myOccurences
		);
		myLabels[labelIdx]->SetText(buff);
		myLabels[labelIdx]->SetPos(pos);

		pos.y -= textHeight;
		pos.y -= 0.01f; // custom spacing

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
	if (!FD3d12Renderer::GetInstanceNoCreate())
		return;

	// CPU markers for Pix, when we arent doing graphics debugging (then we get double markers)
#if !GRAPHICS_DEBUGGING
	PIXBeginEvent(myColor, FUtilities::ConvertFromUtf8ToUtf16(myName).c_str());
#endif

	if(myIsGPU)
	{
		if(ID3D12GraphicsCommandList* cmdList = FD3d12Renderer::GetInstance()->GetCommandListForWorkerThread(FJobSystem::ourThreadIdx))
			PIXBeginEvent(cmdList, 0, myName);
	}

	myTimer.Restart();
}

scopedMarker::~scopedMarker()
{
	if (!FD3d12Renderer::GetInstanceNoCreate())
		return;

	// CPU markers for Pix, when we arent doing graphics debugging (then we get double markers)
#if !GRAPHICS_DEBUGGING
	PIXEndEvent();
#endif

	if (myIsGPU)
	{
		if (ID3D12GraphicsCommandList* cmdList = FD3d12Renderer::GetInstance()->GetCommandListForWorkerThread(FJobSystem::ourThreadIdx))
			PIXEndEvent(cmdList);
	}

	if (!FProfiler::GetInstanceNoCreate())
		return;

	double time = myTimer.GetTimeMS();
	FProfiler::GetInstance()->AddTiming(myName, time);
}
