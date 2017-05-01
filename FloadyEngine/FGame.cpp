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

void FGame::Test()
{
	{ OutputDebugString(L"Test"); }

	//FGameEntity* gameEntity = new FGameEntity(myCamera->GetPos(), FVector3(1, 1, 1), FGameEntity::ModelType::Box, 15.0f, true);
	//myEntityContainer.push_back(gameEntity);

	FGameAgent* newAgent = new FGameAgent();
	myEntityContainer.push_back(newAgent);	
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
	
	FJson jsonObject;

	// Add some objects to the scene
	myFpsCounter = new FDynamicText(myRenderer, FVector3(-1.0f, -1.0f, 0.0),"FPS Counter", 0.3f, 0.05f , true, true);
	FGUIButton* button = new FGUIButton(FVector3(0.3f, 0.95f, 0.0), FVector3(0.5f, 1.0f, 0.0), "button.png", FDelegate2<void()>::from<FGame, &FGame::Test>(this));
	FGUIManager::GetInstance()->AddObject(button);
	myRenderer->GetSceneGraph().AddObject(myFpsCounter, true); // transparant == nondeferred for now..

	FJsonObject* level = FJson::Parse("Configs//level.txt");
	const FJsonObject* child = level->GetFirstChild();
	while (child)
	{
		FGameEntity* newEntity = FGameEntityFactory::GetInstance()->Create(child->GetName());
		newEntity->Init(*child);
		myEntityContainer.push_back(newEntity);
		child = level->GetNextChild();
	}

	// init navmesh
	
	FNavMeshManager::GetInstance()->AddBlockingAABB(FVector3(5, 0, 5), FVector3(8, 0, 8));
	FNavMeshManager::GetInstance()->GenerateMesh(FVector3(0, 0, 0), FVector3(20, 0, 20));

	FGameEntity* newEnt = FGameEntityFactory::GetInstance()->Create("FGameEntity");
	myRenderer->RegisterPostEffect(new FPostProcessEffect(FPostProcessEffect::BindBufferMaskAll, "lightshader2.hlsl", "PostProcess1"));
}

bool FGame::Update(double aDeltaTime)
{
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
		// re-init navmesh
		std::vector<FBulletPhysics::AABB> aabbs = myPhysics->GetAABBs();
		FNavMeshManager::GetInstance()->RemoveAllBlockingAABB();
		for (FBulletPhysics::AABB& aabb : aabbs)
		{
			FNavMeshManager::GetInstance()->AddBlockingAABB(aabb.myMin, aabb.myMax);
		}

		FNavMeshManager::GetInstance()->GenerateMesh(FVector3(-20, 0, -20), FVector3(20, 0, 20));
	}

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
		FGUIManager::GetInstance()->Update(windowMouseX, windowMouseY, myInput->IsMouseButtonDown(true), myInput->IsMouseButtonDown(false));

		// Picker
		FVector3 line = myPicker->PickNavMeshPos(FVector3(windowMouseX, windowMouseY, 0.0f));
		if (myInput->IsMouseButtonDown(false))
		{
			vNavStart = line;
		}

		// example entity picking
		if (myInput->IsMouseButtonDown(true))
		{
			FVector3 pickPosNear = myPicker->UnProject(FVector3(windowMouseX, windowMouseY, 0.0f));
			FVector3 pickPosFar = myPicker->UnProject(FVector3(windowMouseX, windowMouseY, 100.0f));
			FGameEntity* entity = myPhysics->GetFirstEntityHit(pickPosNear, pickPosNear + (pickPosFar - pickPosNear).Normalized() * 100.0f);
			if (entity)
			{
				entity->SetPhysicsActive(false);
				myPickedEntity = dynamic_cast<FGameAgent*>(entity);
				vNavStart = entity->GetPos();
				vNavEnd = vNavStart; // reset, or make sure no pathfinding is done
			}
			else if(myPickedEntity) 			// if something is picked and there is no other entity under mouse, find navpos
			{
				vNavEnd = line;
				if(FPathfindComponent* pathFindComponent = myPickedEntity->GetPathFindingComponent())
				{ 
					vNavStart = myPickedEntity->GetPos();
					pathFindComponent->FindPath(vNavStart, vNavEnd);
				}
			}
		}
	}
	
	std::vector<FVector3> pathPoints = FNavMeshManager::GetInstance()->FindPath(vNavStart, vNavEnd);

	//FNavMeshManager::GetInstance()->DebugDraw(myRenderer->GetDebugDrawer());

	// Update world
	for (FGameEntity* entity : myEntityContainer)
	{
		entity->Update(aDeltaTime);
	}

	// Step physics
	if (myInput->IsKeyDown(VK_SPACE))
	{
		myPhysics->Update(aDeltaTime);
	}

	if (myInput->IsKeyDown(VK_SHIFT))
		myPhysics->DebugDrawWorld();

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
	myRenderWindow->CheckForQuit();
	myRenderer->Render();
}
