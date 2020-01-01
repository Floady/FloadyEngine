#pragma once

class FGameUIPanelBuildings;
class FGameUIPanelDebug;
class FGameUIPanelProfiler;
class FGameUIPanelProfilerIMGUI;

class FGameUIManager
{
public:
	enum GuiState
	{
		MainScreen = 0,
		InGame,
		Debug,
		Profiler,
		No_UI
	};

	FGameUIManager();
	void Update();
	void SetState(GuiState aState);
	
	~FGameUIManager();
private:
	GuiState myState;
	FGameUIPanelBuildings* myBuildingPanel;
	FGameUIPanelDebug* myDebugPanel;
	FGameUIPanelProfiler* myProfilerPanel;
	FGameUIPanelProfilerIMGUI* myProfilerPanelIMGUI;
};

