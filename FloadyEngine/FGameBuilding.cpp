#include "FGameBuilding.h"
#include "..\FJson\FJson.h"
#include "FGameEntityFactory.h"
#include "FGameEntity.h"
#include "FGameAgent.h"
#include "FGame.h"
#include "FPlacingManager.h"

REGISTER_GAMEENTITY2(FGameBuilding);

FGameBuildingBlueprint::FGameBuildingBlueprint(const char* aSetup) : FGameEntity()
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
		const FJsonObject* agentNode = menuItemObj->GetChildByName("agent");
		CreateAgentMenuItem* menuItem = new CreateAgentMenuItem(agentNode->GetName(), menuItemObj->GetItem("icon").GetAs<string>());
		menuItem->myBuildTime = menuItemObj->GetItem("buildTime").GetAs<double>();
		menuItem->mySetup = agentNode->GetFirstChild();
		myMenuItems.push_back(menuItem);

		menuItemObj = child->GetNextChild();
	}

	myRallyPointPos = FVector3(5.0, 0, 0);

	SetRallyPointMenuItem* item = new SetRallyPointMenuItem("RallyPos");
	myMenuItems.push_back(item);
}


FGameBuildingBlueprint::~FGameBuildingBlueprint()
{
}

void FGameBuildingBlueprint::ExecuteMenuItem(int aIdx) const
{
	char buff[100];
	sprintf(buff, "Menu item: %i \n", aIdx);
	OutputDebugStringA(buff);
	myMenuItems[aIdx]->Execute();
}

FGameEntity * FGameBuildingBlueprint::CreateBuilding()
{
	return new FGameBuilding(this);
}

FGameBuilding::FGameBuilding(FGameBuildingBlueprint* aBluePrint)
	: FGameEntity()
	, myBluePrint(aBluePrint)
{
	FGameEntity::Init(aBluePrint->GetPos(), FVector3(1, 1, 1), FGameEntity::ModelType::Box);
	myRallyPointPos = aBluePrint->GetRallyPointPos();
}

FGameBuilding::~FGameBuilding()
{
}

void FGameBuilding::Init(const FJsonObject & anObj)
{
	FGameEntity::Init(anObj);
}

void FGameBuilding::Update(double aDeltaTime)
{
	FGameEntity::Update(aDeltaTime);
}

void FGameBuildingBlueprint::CreateAgentMenuItem::Execute()
{
	const FGameBuilding* selectedEntity = dynamic_cast<const FGameBuilding*>(FGame::GetInstance()->GetSelectedEntity());
	if (!selectedEntity)
		return;

	FGameAgent* newAgent = new FGameAgent();
	newAgent->Init(*mySetup);
	newAgent->SetPos(selectedEntity->GetPos() + selectedEntity->GetRallyPointPos());
	FGame::GetInstance()->AddEntity(newAgent);
}

void FGameBuildingBlueprint::SetRallyPointMenuItem::Execute()
{
	FGameBuilding* selectedEntity = dynamic_cast<FGameBuilding*>(FGame::GetInstance()->GetSelectedEntity());
	if (!selectedEntity)
		return;

	FGame::GetInstance()->GetPlacingManager()->ClearPlacable();
	FGame::GetInstance()->GetPlacingManager()->SetPlacable(true, FVector3(1, 1, 1), FDelegate2<void(FVector3)>::from<FGameBuilding, &FGameBuilding::SetRallyPointPosWorld>(selectedEntity));
}
