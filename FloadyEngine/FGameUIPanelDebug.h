#pragma once
#include "FGameUIPanel.h"

class FGameUIPanelDebug : public FGameUIPanel
{
public:
	FGameUIPanelDebug();
	void Update() override;
	~FGameUIPanelDebug();
};

