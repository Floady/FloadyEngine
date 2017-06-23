#pragma once
#include "FGameUIPanel.h"

class FGameEntity;

class FGameUIPanelBuildings : public FGameUIPanel
{
public:
	virtual void Update() override;
	FGameUIPanelBuildings();
	~FGameUIPanelBuildings();
private:
	const FGameEntity* myCurrentlySelectedBuilding;
};

