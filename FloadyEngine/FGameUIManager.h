#pragma once

class FGameUIPanelBuildings;
class FGameUIPanelDebug;

class FGameUIManager
{
public:
	enum GuiState
	{
		MainScreen = 0,
		InGame,
		Debug,
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
};

