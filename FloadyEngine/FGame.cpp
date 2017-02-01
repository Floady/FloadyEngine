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

FGame* FGame::ourInstance = nullptr;

FGame::FGame()
{
	// create the basics (renderer, input, camera) - and make the window message handler talk to our input system
	myInput = new FD3d12Input();
	auto someDelegate = FDelegate2<void(UINT, WPARAM, LPARAM)>::from<FD3d12Input, &FD3d12Input::MessageHandler>(myInput);
	myRenderWindow = new FRenderWindow(someDelegate);
	myRenderer = new FD3d12Renderer();
	myCamera = new FGameCamera(myInput, 800.0f, 600.0f);
	myPhysics = new FBulletPhysics();

	ourInstance = this;
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
		OutputDebugStringA("Failed to init Render window, quitting");
		return;
	}

	// Init engine basics
	myRenderer->Initialize(myRenderWindow->GetWindowHeight(), myRenderWindow->GetWindowWidth(), myRenderWindow->GetHWND(), false, false); // vsync + fullscreen here
	myRenderer->SetCamera(myCamera);
	myPhysics->Init();

	// Add some objects to the scene
	FVector3 boxPos = FVector3(2.5f, 12.0f, 2.5f);
	myEntityContainer.push_back(new FGameEntity(FVector3(0.0f, -1.0f, 0.0f), FVector3(15, 0.1, 15), FGameEntity::ModelType::Box, 0.0f));
	myEntityContainer.push_back(new FGameEntity(FVector3(2.0f, 12.0f, 2.1f), FVector3(1, 1, 1), FGameEntity::ModelType::Box, 10.0f));
	myEntityContainer.push_back(new FGameEntity(FVector3(2.0f, 8.0f, 2.0f), FVector3(1, 1, 1), FGameEntity::ModelType::Sphere, 10.0f));
	myFpsCounter = new FDynamicText(myRenderer, FVector3(-1.0f, -1.0f, 0.0),"FPS Counter", 1.0f, 0.2f , true, true);
	myYoloSign = new FDynamicText(myRenderer, FVector3(0, 2.0f, -2.0), "FPS Counter", 5.0f, 1.0f, true, false);
	myRenderer->GetSceneGraph().AddObject(myFpsCounter, true); // transparant == nondeferred for now..
	myRenderer->GetSceneGraph().AddObject(myYoloSign, false);
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

	// update camera
	myCamera->Update(aDeltaTime);

	for (FGameEntity* entity : myEntityContainer)
	{
		entity->Update();
	}

	// update input if we have focus
	if (myInput->IsKeyDown(VK_SPACE))
	{
		/*myBoxPhys->applyImpulse(btVector3(0.1, 1, 0), btVector3(0, 0, 0));
		myBoxPhys->activate();*/
		myPhysics->Update(aDeltaTime);
	}

	for (FGameEntity* entity : myEntityContainer)
	{
		entity->PostPhysicsUpdate();
	}


	char buff[128];
	sprintf_s(buff, "%s %f\0", "Fps: ", 1.0f/static_cast<float>(aDeltaTime));
	myFpsCounter->SetText(buff);
	myYoloSign->SetText("yolo");
	return true;
}

void FGame::Render()
{
	// capture mouse if in focus
	if (myRenderWindow->IsFocussed())
	{
		int x = myRenderWindow->GetScreenWidth() / 2;
		int y = myRenderWindow->GetScreenHeight() / 2;
		myInput->SetMousePos(x, y);
	}

	myRenderWindow->CheckForQuit();
	myRenderer->Render();
}
