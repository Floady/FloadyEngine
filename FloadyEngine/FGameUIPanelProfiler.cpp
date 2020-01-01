#include "FGameUIPanelProfiler.h"
#include "GUI\FGUILabel.h"
#include "FLightManager.h"
#include "FNavMeshManagerRecast.h"
#include "FGame.h"
#include "FD3d12Renderer.h"
#include "FProfiler.h"
#include "FSceneGraph.h"
#include "FPhysicsWorld.h"
#include "FCamera.h"
#include "FProfiler.h"

static const char* ourLabelLight = "labelLight.png";
static const char* ourLabelDark = "labelDark.png";

FGameUIPanelProfiler::FGameUIPanelProfiler()
{
	FVector3 pos(0,0,0);
	FVector3 size(0.4f, 0.035f, 0);
	FVector3 offset(0, 0.035f, 0);
	FVector3 timerOffset(0.4f, 0.0f, 0);

	if (FProfiler* profiler = FProfiler::GetInstanceNoCreate())
	{
		const std::map<const char*, FProfiler::TimerInfo>& timings = profiler->GetTimings();
		bool useLightTex = true;
		for (const auto& timing : timings)
		{
			{
				FGUILabel* label = new FGUILabel(pos, pos + size, useLightTex ? ourLabelLight : ourLabelDark);
				char buff[128];
				sprintf_s(buff, "%s", timing.first);
				label->SetDynamicText(buff);
				AddObject(label);
				myTitleLabels.push_back(label);
			}

			{
				FGUILabel* label = new FGUILabel(pos + timerOffset, pos + timerOffset + size, useLightTex ? ourLabelLight : ourLabelDark);
				AddObject(label);
				myTimesLabels.push_back(label);
			}

			pos += offset;
			useLightTex = !useLightTex;
		}
	}
}

void FGameUIPanelProfiler::Update()
{
	if (FProfiler* profiler = FProfiler::GetInstanceNoCreate())
	{
		int labelIdx = 0;
		const std::map<const char*, FProfiler::TimerInfo>& timings = profiler->GetTimings();
		for (const auto& timing : timings)
		{
			char buff[128];
			sprintf_s(buff, "%05.2f ms", timing.second.myFrameTimings[profiler->GetCurrentHistoryFrameIndex()].myTime);
			myTimesLabels[labelIdx]->SetDynamicText(buff);
			labelIdx++;
		}
	}
}

FGameUIPanelProfiler::~FGameUIPanelProfiler()
{
}
