#include "FGame.h"
#include "FRenderWindow.h"
#include "FD3d12Input.h"
#include "FCamera.h"
#include "FTimer.h"
#include "FD3d12Renderer.h"
#include "FRenderWindow.h"
#include "FGameCamera.h"
#include "FPrimitiveBox.h"
#include "FDynamicText.h"
#include "FBulletPhysics.h"
#include "FGameEntity.h"
#include "BulletDynamics\Dynamics\btRigidBody.h"
#include "..\FJson\FJson.h"
#include "FScreenQuad.h"
#include "GUI\FGUIButton.h"
#include "GUI\FGUIManager.h"
#include "FDebugDrawer.h"
#include "FNavMeshManager.h"
#include "F3DPicker.h"
#include "FGameAgent.h"
#include "FGameEntityFactory.h"
#include "FPostProcessEffect.h"
#include "FGameHighlightManager.h"
#include "FPlacingManager.h"
#include "FGameBuilding.h"
#include "FGameBuildingManager.h"
#include "FGameUIManager.h"
#include "FLightManager.h"
#include "FProfiler.h"

FGame* FGame::ourInstance = nullptr;

void * operator new(std::size_t n) throw(std::bad_alloc)
{
	//...
	void* p = malloc(n);
	return p;
}
void operator delete(void * p) throw()
{
	//...
	free(p);
}

void FGame::ConstructBuilding(FVector3 aPos)
{
	if (!myBuildingName)
		return;

	myPlacingManager->ClearPlacable();

	OutputDebugString(L"ConstructBuilding:  ");
	OutputDebugStringA(myBuildingName);	
	OutputDebugString(L"\n");

	FGameEntity* entity = (myBuildingManager->GetBuildingTemplates().find(myBuildingName))->second->GetEntity();
	entity->SetPos(aPos);
	myEntityContainer.push_back(entity);

	myBuildingName = nullptr;
}

void FGame::ConstructBuilding(const char * aBuildingName)
{
	OutputDebugString(L"ConstructBuilding:  ");
	OutputDebugStringA(aBuildingName);
	OutputDebugString(L"\n");
	
	myBuildingName = aBuildingName;
	myPlacingManager->SetPlacable(true, FVector3(2, 1, 1), FDelegate2<void(FVector3)>::from<FGame, &FGame::ConstructBuilding>(this));
}

void FGame::Test()
{
	OutputDebugString(L"Test\n");

	FGameAgent* newAgent = new FGameAgent();
	myEntityContainer.push_back(newAgent);
}

void FGame::Test(FVector3 aPos)
{
	OutputDebugString(L"Test aPos\n");

	FGameAgent* newAgent = new FGameAgent(aPos);
	
	myEntityContainer.push_back(newAgent);

	FGameBuilding* someBuilding = new FGameBuilding("Configs//buildings//firstbuilding.json");
}

void FGame::AddEntity(FGameEntity * anEntity)
{
	myEntityContainer.push_back(anEntity);
}

FGame::FGame()
{
	// create the basics (renderer, input, camera) - and make the window message handler talk to our input system
	myInput = new FD3d12Input();
	auto someDelegate = FDelegate2<void(UINT, WPARAM, LPARAM)>::from<FD3d12Input, &FD3d12Input::MessageHandler>(myInput);
	myRenderWindow = new FRenderWindow(someDelegate);
	myRenderer = new FD3d12Renderer();
	myCamera = new FGameCamera(myInput, 800.0f, 600.0f);
	myPhysics = new FBulletPhysics();
	myPicker = new F3DPicker(myCamera, myRenderWindow);
	ourInstance = this;
	myIsMouseCaptured = false;
	myPickedEntity = nullptr;
	myHighlightManager = nullptr;
	myPlacingManager = nullptr;
	myBuildingManager = nullptr;
	myGameUIManager = nullptr;
	myPickedAgent = nullptr;
}

FGame::~FGame()
{
	myRenderWindow->Shutdown();
	delete myRenderWindow;
	myRenderWindow = nullptr;

	myRenderer->Shutdown();
	delete myRenderer;
	myRenderer = nullptr;

	delete myInput;
	myInput = nullptr;

	delete myCamera;
	myCamera = nullptr;
}

void FGame::Init()
{
	bool result = myRenderWindow->Initialize();

	if (!result)
	{
		OutputDebugStringA("Failed to init Render window\n");
		return;
	}

	// Init engine basics
	if (!myRenderer->Initialize(myRenderWindow->GetWindowHeight(), myRenderWindow->GetWindowWidth(), myRenderWindow->GetHWND(), false, false))// vsync + fullscreen here
	{
		OutputDebugStringA("Failed to init Renderer\n");
		return;
	}

	myRenderer->SetCamera(myCamera);
	myPhysics->Init(myRenderer);
	myHighlightManager = new FGameHighlightManager();
	myBuildingManager = new FGameBuildingManager();
	myGameUIManager = new FGameUIManager();

	FJson jsonObject;

	// Add some objects to the scene
	myFpsCounter = new FDynamicText(myRenderer, FVector3(-1.0f, -1.0f, 0.0),"FPS Counter", 0.3f, 0.05f , true, true);
	//FGUIButton* button = new FGUIButton(FVector3(0.3f, 0.95f, 0.0), FVector3(0.5f, 1.0f, 0.0), "button.png", FDelegate2<void()>::from<FGame, &FGame::Test>(this));
	//FGUIManager::GetInstance()->AddObject(button);
	myRenderer->GetSceneGraph().AddObject(myFpsCounter, true); // transparant == nondeferred for now..

	FJsonObject* level = FJson::Parse("Configs//level.txt");
	const FJsonObject* child = level->GetFirstChild();
	/*while (child)
	{
		FGameEntity* newEntity = FGameEntityFactory::GetInstance()->Create(child->GetName());
		newEntity->Init(*child);
		myEntityContainer.push_back(newEntity);
		child = level->GetNextChild();
	}*/

	FLightManager::GetInstance()->AddDirectionalLight(FVector3(0, 0, -50), FVector3(0, -1, 1), FVector3(0.2, 0.2, 0.2));

	// test lights
	//int stepX = 15;
	//int stepY = 15;
	//int gridSize = 3;
	//for (int i = 0; i < gridSize; i++)
	//{
	//	for (int j = 0; j < gridSize; j++)
	//	{
	//		if (i == 0 && j == 0)
	//			continue;

	//		FLightManager::GetInstance()->AddLight(FVector3(i * stepX, 2.0f, j * stepY), 10.0f);
	//	}
	//}

	FLightManager::GetInstance()->AddLight(FVector3(30, 15, 10), 10.0f);
	FLightManager::GetInstance()->AddSpotlight(FVector3(0, 10, 10), FVector3(0, -1, -1.05), 30.0f, FVector3(1, 0, 0), 25.0f);
	FLightManager::GetInstance()->AddSpotlight(FVector3(30, 15, -20), FVector3(0, -1, 0.5), 10.0f, FVector3(0, 1, 0), 25.0f);
	FLightManager::GetInstance()->AddLight(FVector3(-20, 15, 10), 10.0f);
	FLightManager::GetInstance()->AddLight(FVector3(300, 15, 100), 10.0f);
	// init navmesh
	
	FNavMeshManager::GetInstance()->AddBlockingAABB(FVector3(5, 0, 5), FVector3(8, 0, 8));
	FNavMeshManager::GetInstance()->GenerateMesh(FVector3(0, 0, 0), FVector3(20, 0, 20));

	std::vector<FPostProcessEffect::BindInfo> resources;
	resources.push_back(FPostProcessEffect::BindInfo(myRenderer->GetGBufferTarget(0), myRenderer->gbufferFormat[0]));
	resources.push_back(FPostProcessEffect::BindInfo(myRenderer->GetGBufferTarget(1), myRenderer->gbufferFormat[1]));
	resources.push_back(FPostProcessEffect::BindInfo(myRenderer->GetGBufferTarget(2), myRenderer->gbufferFormat[2]));
	resources.push_back(FPostProcessEffect::BindInfo(myRenderer->GetGBufferTarget(3), myRenderer->gbufferFormat[3]));
	resources.push_back(FPostProcessEffect::BindInfo(myRenderer->GetGBufferTarget(4), myRenderer->gbufferFormat[4]));
	myRenderer->RegisterPostEffect(new FPostProcessEffect(resources, "lightshader2.hlsl", "PostProcess1"));

	myPlacingManager = new FPlacingManager();
	myGameUIManager->SetState(FGameUIManager::GuiState::InGame);
}

bool FGame::Update(double aDeltaTime)
{
	FProfiler::GetInstance()->StartFrame();

	FPROFILE_FUNCTION("FGame Update");

	// quit on escape
	if (myInput->IsKeyDown(VK_ESCAPE))
		return false;
	if (myInput->IsKeyDown(VK_F2))
		myRenderer->GetShaderManager().ReloadShaders();

	// update input if we have focus
	if (myRenderWindow->IsFocussed())
		myInput->Update();

	if (myInput->IsKeyDown(VK_F3))
	{
		myGameUIManager->SetState(FGameUIManager::GuiState::MainScreen);
		// re-init navmesh
		std::vector<FBulletPhysics::AABB> aabbs = myPhysics->GetAABBs();
		FNavMeshManager::GetInstance()->RemoveAllBlockingAABB();
		for (FBulletPhysics::AABB& aabb : aabbs)
		{
			FNavMeshManager::GetInstance()->AddBlockingAABB(aabb.myMin, aabb.myMax);
		}

		FNavMeshManager::GetInstance()->GenerateMesh(FVector3(-20, 0, -20), FVector3(200, 0, 200));
	}

	myGameUIManager->Update();

	if (myInput->IsKeyDown(VK_F4))
		myGameUIManager->SetState(FGameUIManager::GuiState::InGame);

	if (myInput->IsKeyDown(VK_F5))
		myPlacingManager->ClearPlacable();

	if (myInput->IsKeyDown(VK_F6))
		myPlacingManager->SetPlacable(true, FVector3(2,1,1), FDelegate2<void(FVector3)>::from<FGame, &FGame::Test>(this));

	static FVector3 vNavEnd;
	static FVector3 vNavStart;
	// Control captures mouse and moves camera, otherwise update UI
	if (myRenderWindow->IsFocussed() && myInput->IsKeyDown(VK_CONTROL))
	{
		if (myIsMouseCaptured) // skip first update
		{
			myRenderWindow->SetCursorVisible(false);
			myCamera->Update(aDeltaTime); 
		}

		int x = myRenderWindow->GetScreenWidth() / 2;
		int y = myRenderWindow->GetScreenHeight() / 2;
		myInput->SetMousePos(x, y);
		
		myIsMouseCaptured = true;
	}
	else
	{
		myIsMouseCaptured = false;

		myRenderWindow->SetCursorVisible(true);

		float windowMouseX = (myInput->GetMouseX() - myRenderWindow->GetPosX()) / static_cast<float>(myRenderWindow->GetWindowWidth());
		float windowMouseY = (myInput->GetMouseY() - myRenderWindow->GetPosY()) / static_cast<float>(myRenderWindow->GetWindowHeight());
		windowMouseX = windowMouseX > 1.0f ? 1.0f : windowMouseX;
		windowMouseX = windowMouseX < 0.0f ? 0.0f : windowMouseX;
		windowMouseY = windowMouseY > 1.0f ? 1.0f : windowMouseY;
		windowMouseY = windowMouseY < 0.0f ? 0.0f : windowMouseY;

		// update UI
		bool isonUI = FGUIManager::GetInstance()->Update(windowMouseX, windowMouseY, myInput->IsMouseButtonDown(true), myInput->IsMouseButtonDown(false));

		// example entity picking
		if (myInput->IsMouseButtonDown(true))
		{
			FVector3 navPos = myPicker->PickNavMeshPos(FVector3(windowMouseX, windowMouseY, 0.0f));
			
			if(!isonUI)
			{
				myPlacingManager->MouseDown(navPos);

				FVector3 pickPosNear = myPicker->UnProject(FVector3(windowMouseX, windowMouseY, 0.0f));
				FVector3 pickPosFar = myPicker->UnProject(FVector3(windowMouseX, windowMouseY, 100.0f));
				FGameEntity* entity = myPhysics->GetFirstEntityHit(pickPosNear, pickPosNear + (pickPosFar - pickPosNear).Normalized() * 100.0f);
				if (entity != myPickedEntity)
				{
					myHighlightManager->RemoveSelectableObject(myPickedEntity);
					myHighlightManager->AddSelectableObject(entity);
					myPickedEntity = entity;

					if (entity && dynamic_cast<FGameAgent*>(entity))
					{
						vNavStart = entity->GetPos();
						vNavEnd = vNavStart; // reset, or make sure no pathfinding is done

						myPickedAgent = dynamic_cast<FGameAgent*>(entity);
					}
				}
			}
		}

		if (myPickedAgent && myInput->IsMouseButtonDown(false)) 			// if something is picked and there is no other entity under mouse, find navpos
		{
			FVector3 line = myPicker->PickNavMeshPos(FVector3(windowMouseX, windowMouseY, 0.0f));
			vNavEnd = line;
			if (FPathfindComponent* pathFindComponent = myPickedAgent->GetPathFindingComponent())
			{
				vNavStart = myPickedAgent->GetPos();
				pathFindComponent->FindPath(vNavStart, vNavEnd);
			}
		}

		FVector3 navPos = myPicker->PickNavMeshPos(FVector3(windowMouseX, windowMouseY, 0.0f));
		myPlacingManager->UpdateMousePos(navPos);
	}
	
	std::vector<FVector3> pathPoints = FNavMeshManager::GetInstance()->FindPath(vNavStart, vNavEnd);


	// Update world
	for (FGameEntity* entity : myEntityContainer)
	{
		entity->Update(aDeltaTime);
	}

	// Step physics
	if (myInput->IsKeyDown(VK_SPACE))
	{
		myPhysics->TogglePaused();
	}

	myPhysics->Update(aDeltaTime);

	if (myInput->IsKeyDown(VK_SHIFT))
	{
		FNavMeshManager::GetInstance()->DebugDraw(myRenderer->GetDebugDrawer());
		//myPhysics->DebugDrawWorld();
	}

	for (FGameEntity* entity : myEntityContainer)
	{
		entity->PostPhysicsUpdate();
	}

	// update FPS
	char buff[128];
	sprintf_s(buff, "%s %f\0", "Fps: ", 1.0f/static_cast<float>(aDeltaTime));
	myFpsCounter->SetText(buff);

	return true;
}

void FGame::Render()
{
	
	{
		FPROFILE_FUNCTION("FGame Render");
		
		//FD3d12Renderer::GetInstance()->GetCommandAllocator()->Reset();
		myRenderWindow->CheckForQuit();
		myHighlightManager->Render();
		myRenderer->Render();
	}

	FProfiler::GetInstance()->Render();
}
