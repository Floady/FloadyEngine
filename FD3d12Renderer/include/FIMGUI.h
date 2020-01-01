#pragma once
#include "FDelegate.h"
#include <vector>

//extern "C" void ImGui_ImplDX12_NewFrame();

class FIMGUI
{
public:
	FIMGUI();
	void Update();
	~FIMGUI();

	static std::vector<FDelegate2<void(void)>> myRenderCallbacks;
};

