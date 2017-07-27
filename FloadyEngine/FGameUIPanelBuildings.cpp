#include "FGameUIPanelBuildings.h"
#include "FGameBuildingManager.h"
#include "FGameBuilding.h"
#include "FGameEntity.h"
#include "FGame.h"
#include "GUI\FGUIButton.h"

void FGameUIPanelBuildings::Update()
{
	float startX = 0.3f;
	float width = 0.2f;
	float currentX = startX;

	// show building actions on right side
	const FGameBuilding* selectedEntity = dynamic_cast<const FGameBuilding*>(FGame::GetInstance()->GetSelectedEntity());
	if (!selectedEntity)
		return;

	const FGameBuildingBlueprint* selectedBuilding = selectedEntity->GetBluePrint();
	if (selectedBuilding && selectedBuilding != myCurrentlySelectedBuilding) // check for building
	{
		DestroyAllItems();
		myCurrentlySelectedBuilding = selectedBuilding;

		currentX = startX;
		width = 0.1f;
		int idx = 0;
		for (FMenuItemBase* item : selectedBuilding->GetMenuItems())
		{
			FGUIButton* button = new FGUIButton(FVector3(currentX, 0.85f, 0.0), FVector3(currentX + width, 0.95f, 0.0), "buttonBlanco.png", FDelegate2<void(int)>::from<FGameBuildingBlueprint, &FGameBuildingBlueprint::ExecuteMenuItem>(selectedBuilding), idx);
			button->SetDynamicText(item->GetIcon().c_str());
			AddObject(button);
			currentX += width;
			idx++;
		}
	}

}

FGameUIPanelBuildings::FGameUIPanelBuildings()
{
	myCurrentlySelectedBuilding = nullptr;
}


FGameUIPanelBuildings::~FGameUIPanelBuildings()
{
}
