#pragma once
#include <string>
#include <map>

class FGameBuildingBlueprint;

class FGameBuildingManager
{
public:
	FGameBuildingManager();
	~FGameBuildingManager();
	const std::map<std::string, FGameBuildingBlueprint*>& GetBuildingTemplates() { return myBuildingTemplates; }
private:

	std::map<std::string, FGameBuildingBlueprint*> myBuildingTemplates;
};

