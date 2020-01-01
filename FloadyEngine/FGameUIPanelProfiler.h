#pragma once
#include "FGameUIPanel.h"
#include <vector>

class FGUILabel;
class FGameUIPanelProfiler : public FGameUIPanel
{
public:
	FGameUIPanelProfiler();
	void Update() override;
	~FGameUIPanelProfiler();

	std::vector<FGUILabel*> myTitleLabels;
	std::vector<FGUILabel*> myTimesLabels;
};

