#include "FProfiler.h"
#include "FGame.h"
#include "FDynamicText.h"
#include "windows.h"

FProfiler* FProfiler::ourInstance = nullptr;
unsigned int FProfiler::ourHistoryBufferCount = 120;
static const float textHeight = 0.05f;
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
			myLabels.push_back(new FDynamicText(FGame::GetInstance()->GetRenderer(), FVector3(-1.0f, -1.0f, 0.0), "FPS Counter", 0.5f, textHeight, true, true));
			FGame::GetInstance()->GetRenderer()->GetSceneGraph().AddObject(myLabels[myLabels.size() - 1], true); // transparant == nondeferred for now..
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

		pos.y -= textHeight * 2;

		if (pos.y < -1)
		{
			pos.y = 1;
			pos.x = 0;
		}

		labelIdx++;
	}
}
