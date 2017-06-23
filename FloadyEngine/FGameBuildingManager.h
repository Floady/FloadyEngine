#pragma once
#include <vector>
#include <tchar.h>
#include <stdio.h>
#include "FGameBuilding.h""
#include <map>

class FGameBuildingManager
{
public:
	FGameBuildingManager();
	~FGameBuildingManager();
	const std::map<std::string, FGameBuilding*>& GetBuildingTemplates() { return myBuildingTemplates; }
private:

	std::map<std::string, FGameBuilding*> myBuildingTemplates;
};

