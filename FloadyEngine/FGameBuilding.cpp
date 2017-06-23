#include "FGameBuilding.h"
#include "..\FJson\FJson.h"
#include "FGameEntityFactory.h"
#include "FGameEntity.h"
#include "FGameAgent.h"
#include "FGame.h"


FGameBuilding::FGameBuilding(const char* aSetup) : FGameEntity()
{
	myEntity = nullptr;

	FJsonObject* building = FJson::Parse(aSetup);
	
	// get building representation
	const FJsonObject* child = building->GetFirstChild();
	myEntity = FGameEntityFactory::GetInstance()->Create(child->GetName());
	myEntity->Init(*child);
	myEntity->SetPhysicsActive(true);
	myEntity->SetOwnerEntity(this);

	// menu items
	child = building->GetNextChild();
	const FJsonObject* menuItemObj = nullptr;
	if (child)
		menuItemObj = child->GetFirstChild();

	while (menuItemObj)
	{
		MenuItem menuItem;
		menuItem.myBuildTime = menuItemObj->GetItem("buildTime").GetAs<double>();
		const FJsonObject* agentNode = menuItemObj->GetChildByName("agent");
		menuItem.mySetup = agentNode->GetFirstChild();
		menuItem.myClassName = agentNode->GetName();
		menuItem.myIcon = menuItemObj->GetItem("icon").GetAs<string>();
		myMenuItems.push_back(menuItem);

		menuItemObj = child->GetNextChild();
	}

	myRallyPointPos = FVector3(5.0, 0, 0);
}


FGameBuilding::~FGameBuilding()
{
}

void FGameBuilding::ExecuteMenuItem(int aIdx) const
{
	char buff[100];
	sprintf(buff, "Menu item: %i \n", aIdx);
	OutputDebugStringA(buff);
	
	FGameAgent* newAgent = new FGameAgent();
	newAgent->Init(*myMenuItems[aIdx].mySetup);
	newAgent->SetPos(myEntity->GetPos() + myRallyPointPos);
	FGame::GetInstance()->AddEntity(newAgent);
}
