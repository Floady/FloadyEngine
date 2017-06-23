#pragma once
#include <vector>
#include <string>
#include "FGameEntity.h"


class FGameBuilding : public FGameEntity
{
public:
	struct MenuItem
	{
		float myBuildTime;
		std::string myClassName;
		std::string myIcon;
		const FJsonObject* mySetup;
		// add callbac kto execute this item, make it a global UI thing FGUIMenuItem
	};

public:
	FGameBuilding(const char* aSetup);
	~FGameBuilding();
	const std::vector<MenuItem>& GetMenuItems() const { return myMenuItems; }
	void ExecuteMenuItem(int aIdx) const;
	FGameEntity* GetEntity() const { return myEntity; }

	FRenderableObject* GetRenderableObject() override { return myEntity->GetRenderableObject(); }
protected:
	FGameEntity* myEntity; // representation of building
	std::vector<MenuItem> myMenuItems;
	FVector3 myRallyPointPos;
};

