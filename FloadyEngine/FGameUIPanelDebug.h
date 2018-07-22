#pragma once
#include "FGameUIPanel.h"

class FGUIButton;
class FGameUIPanelDebug : public FGameUIPanel
{
public:
	FGameUIPanelDebug();
	void Update() override;
	~FGameUIPanelDebug();

	FGUIButton* myCameraPositionLabel;
};

