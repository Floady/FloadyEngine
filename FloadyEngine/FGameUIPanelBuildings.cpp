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
	const FGameEntity* selectedEntity = FGame::GetInstance()->GetSelectedEntity();
	const FGameBuilding* selectedBuilding = dynamic_cast<const FGameBuilding*>(selectedEntity);
	if (selectedBuilding && selectedBuilding != myCurrentlySelectedBuilding) // check for building
	{
		DestroyAllItems();
		myCurrentlySelectedBuilding = selectedBuilding;

		currentX = startX;
		width = 0.1f;
		int idx = 0;
		for (const FGameBuilding::MenuItem& item : selectedBuilding->GetMenuItems())
		{
			FGUIButton* button = new FGUIButton(FVector3(currentX, 0.85f, 0.0), FVector3(currentX + width, 0.95f, 0.0), "buttonBlanco.png", FDelegate2<void(int)>::from<FGameBuilding, &FGameBuilding::ExecuteMenuItem>(selectedBuilding), idx);
			button->SetDynamicText(item.myIcon.c_str());
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
