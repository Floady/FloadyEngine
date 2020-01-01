#include "FGameUIPanelProfilerIMGUI.h"
#include "FGUIButtonToggle.h"
#include "FLightManager.h"
#include "FNavMeshManagerRecast.h"
#include "FGame.h"
#include "FD3d12Renderer.h"
#include "FProfiler.h"
#include "FSceneGraph.h"
#include "FPhysicsWorld.h"
#include "FCamera.h"
#include "imgui.h"
#include "FDelegate.h"
#include "FIMGUI.h"

FGameUIPanelProfilerIMGUI::FGameUIPanelProfilerIMGUI()
 : myPauseProfiler(false)
	, myBeginTime(0.0f)
	, myEndTime(1000.0f)
{
	FIMGUI::myRenderCallbacks.push_back(FDelegate2<void(void)>::from<FGameUIPanelProfilerIMGUI, &FGameUIPanelProfilerIMGUI::Update>(this));
}

void FGameUIPanelProfilerIMGUI::Update()
{
	ImGui::Begin("Profiler!"); 
	ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	
	ImGui::Checkbox("Pause", &myPauseProfiler);
	ImGui::SameLine();
	ImGui::DragFloatRange2("range", &myBeginTime, &myEndTime, 1.25f, 0.0f, 10000.0f, "Min: %.1f ms", "Max: %.1f ms");

	ImGui::Columns(3, "mycolumns3", false);  // 3-ways, no border
	ImGui::Separator();

	int labelIdx = 0;
	if (FProfiler* profiler = FProfiler::GetInstanceNoCreate())
	{
		profiler->SetPause(myPauseProfiler);

		const std::map<const char*, FProfiler::TimerInfo>& timings = profiler->GetTimings();
		for (const auto& timing : timings)
		{
			char buff[128];
			char buff2[128];
			sprintf_s(buff, "%05.2f ms", timing.second.myFrameTimings[profiler->GetCurrentHistoryFrameIndex()].myTime);
			sprintf_s(buff2, "%d", timing.second.myFrameTimings[profiler->GetCurrentHistoryFrameIndex()].myOccurences);

			bool isSelected = false;
			if (ImGui::Selectable(timing.first, &isSelected, ImGuiSelectableFlags_SpanAllColumns)) {}
			ImGui::NextColumn();
			if (ImGui::Selectable(buff)) {}
			ImGui::NextColumn();
			if (ImGui::Selectable(buff2)) {}
			ImGui::NextColumn();

			labelIdx++;
		}

		ImGui::Columns(1);
		ImGui::Separator();

		for (const auto& timing : timings)
		{
			char buff[128];
			float startTime = timing.second.myFrameTimings[profiler->GetCurrentHistoryFrameIndex()].myStartTime;
			sprintf_s(buff, "%s %f", timing.first, startTime);
			
			float f = static_cast<float>(timing.second.myFrameTimings[profiler->GetCurrentHistoryFrameIndex()].myTime);
			//ImGui::Text("PushItemWidth(GetWindowWidth() * 0.5f)");
			
			float ratio2 = ImGui::GetWindowWidth() / (myEndTime - myBeginTime);
		//	ImGui::SameLine(); 
		//	ImGui::PushItemWidth(ratio);
			//ImGui::DragFloat(buff, &f);
			ImGui::SameLine(0, ratio2 * (startTime - myBeginTime));
			ImGui::Selectable(buff, true, 0, ImVec2(ratio2 * f, 20));
			ImGui::NewLine();
			//ImGui::Text(timing.first);
			//ImGui::Text("::");
		//	ImGui::PopItemWidth();
			labelIdx++;
		}

	}


	// draw label width - use for drawing timeline
	/*ImGui::Text("PushItemWidth(GetWindowWidth() * 0.5f)");
	ImGui::SameLine(); ShowHelpMarker("Half of window width.");
	ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);
	ImGui::DragFloat("float##2", &f);
	ImGui::PopItemWidth();
*/
	ImGui::End();
}
