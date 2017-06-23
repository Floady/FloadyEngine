#include "FGameUIManager.h"
#include "FGame.h"
#include "GUI\FGUIButton.h"
#include "GUI\FGUIManager.h"
#include "FDelegate.h"
#include "FGameBuildingManager.h"
#include "FGameBuilding.h"
#include "FGameEntity.h"
#include "FGameUIPanelBuildings.h"

FGameUIManager::FGameUIManager()
{
	myBuildingPanel = new FGameUIPanelBuildings();
}

void FGameUIManager::Update()
{
	switch(myState)
	{
	case MainScreen:
		break;
	case InGame:
	{
		myBuildingPanel->Update();
		break;
	}
	}
}

static std::vector<FGUIButton*> ourBuildings;

void FGameUIManager::SetState(GuiState aState)
{
	if (aState == myState)
		return;

	//@todo: make ui scenes and use as state machine, state is responsible for tracking ui items and destroy (not a clearall)
	// other systems could have things in the global ui manager
	// scenes can be panels with relative coords (setScene(buildingScene, FVector3(0,0.5,0))
	myState = aState;

	//FGUIManager::GetInstance()->ClearAll();

	switch (myState)
	{
		case MainScreen:
		{
			myBuildingPanel->Hide();
			break;
		}
		case InGame:
		{
			// show buildings bottom screen
			if(ourBuildings.size() == 0)
			{
				const auto& buildingTemplates = FGame::GetInstance()->GetBuildingManager()->GetBuildingTemplates();
				float startX = 0.3f;
				float width = 0.2f;
				float currentX = startX;
				for (const auto& buildingTemplate : buildingTemplates)
				{
					FGUIButton* button = new FGUIButton(FVector3(currentX, 0.95f, 0.0), FVector3(currentX+width, 1.0f, 0.0), "buttonBlanco.png", FDelegate2<void(const char*)>::from<FGame, &FGame::ConstructBuilding>(FGame::GetInstance()), buildingTemplate.first.c_str());
					button->SetDynamicText(buildingTemplate.first.c_str());
					FGUIManager::GetInstance()->AddObject(button);
					currentX += width;
					ourBuildings.push_back(button);
				}
			}

			myBuildingPanel->Show();
			
			break;
		}
	}
}


FGameUIManager::~FGameUIManager()
{
}
