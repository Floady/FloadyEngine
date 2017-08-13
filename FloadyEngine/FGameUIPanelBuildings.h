#pragma once
#include "FGameUIPanel.h"

class FGameEntity;
class FGameBuildingBlueprint;

class FGameUIPanelBuildings : public FGameUIPanel
{
public:
	virtual void Update() override;
	FGameUIPanelBuildings();
	~FGameUIPanelBuildings();
private:
	const FGameBuildingBlueprint* myCurrentlySelectedBuilding;
};

