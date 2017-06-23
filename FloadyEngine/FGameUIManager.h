#pragma once

class FGameUIPanelBuildings;

class FGameUIManager
{
public:
	enum GuiState
	{
		MainScreen = 0,
		InGame
	};

	FGameUIManager();
	void Update();
	void SetState(GuiState aState);
	
	~FGameUIManager();
private:
	GuiState myState;
	FGameUIPanelBuildings* myBuildingPanel;
};

