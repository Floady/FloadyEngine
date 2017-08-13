#include "FGameBuilding.h"
#include "..\FJson\FJson.h"
#include "FGameEntityFactory.h"
#include "FGameEntity.h"
#include "FGameAgent.h"
#include "FGame.h"
#include "FPlacingManager.h"
#include "FUtilities.h"
#include "FGameLevel.h"

REGISTER_GAMEENTITY2(FGameBuilding);

FGameBuildingBlueprint::FGameBuildingBlueprint(const char* aSetup)
{
	myEntity = nullptr;

	myBuilding = FJson::Parse(aSetup);
	
	// get building representation
	const FJsonObject* child = myBuilding->GetFirstChild();

	// menu items
	child = myBuilding->GetNextChild();
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
	FUtilities::FLog("Menu item: %i \n", aIdx);
	myMenuItems[aIdx]->Execute();
}

FGameBuilding * FGameBuildingBlueprint::CreateBuilding()
{
	return new FGameBuilding(this);
}

const FJsonObject * FGameBuildingBlueprint::GetBuildingRepresentation() const
{
	return myBuilding->GetFirstChild();
}

FGameBuilding::FGameBuilding(FGameBuildingBlueprint* aBluePrint)
	: FGameEntity()
	, myBluePrint(aBluePrint)
{
	myRallyPointPos = aBluePrint->GetRallyPointPos();

	myRepresentation = FGameEntityFactory::GetInstance()->Create(aBluePrint->GetBuildingRepresentation()->GetName());
	myRepresentation->Init(*aBluePrint->GetBuildingRepresentation());
	myRepresentation->SetOwnerEntity(this);
}

FGameBuilding::~FGameBuilding()
{
	delete myRepresentation;
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
	FGame::GetInstance()->GetLevel()->AddEntity(newAgent);
}

void FGameBuildingBlueprint::SetRallyPointMenuItem::Execute()
{
	FGameBuilding* selectedEntity = dynamic_cast<FGameBuilding*>(FGame::GetInstance()->GetSelectedEntity());
	if (!selectedEntity)
		return;

	FGame::GetInstance()->GetPlacingManager()->ClearPlacable();
	FGame::GetInstance()->GetPlacingManager()->SetPlacable(true, FVector3(0.1, 0.1, 0.1), FDelegate2<void(FVector3)>::from<FGameBuilding, &FGameBuilding::SetRallyPointPosWorld>(selectedEntity));
}
