#pragma once
#include <vector>

class FGameUIPanelProfilerIMGUI
{
public:
	FGameUIPanelProfilerIMGUI();
	void Update();

private:
	bool myPauseProfiler;
	float myBeginTime;
	float myEndTime;
};

