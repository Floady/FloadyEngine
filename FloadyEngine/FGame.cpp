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

FGame::FGame()
{
	// create the basics (renderer, input, camera) - and make the window message handler talk to our input system
	myInput = new FD3d12Input();
	auto someDelegate = FDelegate2<void(UINT, WPARAM, LPARAM)>::from<FD3d12Input, &FD3d12Input::MessageHandler>(myInput);
	myRenderWindow = new FRenderWindow(someDelegate);
	myRenderer = new FD3d12Renderer();
	myCamera = new FGameCamera(myInput, 800.0f, 600.0f);
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

	myRenderer->Initialize(myRenderWindow->GetWindowWidth(), myRenderWindow->GetWindowHeight(), myRenderWindow->GetHWND(), false, false); // vsync + fullscreen here
	myRenderer->SetCamera(myCamera);

	myRenderer->GetSceneGraph().AddObject(new FPrimitiveBox(myRenderer, FVector3(0.0f, 0.0f, 0.0f), FVector3(50, 1, 50), FPrimitiveBox::PrimitiveType::Box), false);
	myRenderer->GetSceneGraph().AddObject(new FPrimitiveBox(myRenderer, FVector3(2.5f, 2.0f, 2.5f), FVector3(1, 1, 1), FPrimitiveBox::PrimitiveType::Box), false);
	myRenderer->GetSceneGraph().AddObject(new FPrimitiveBox(myRenderer, FVector3(5.0f, 2.0f, 2.5f), FVector3(1, 1, 1), FPrimitiveBox::PrimitiveType::Sphere), false);
	myFpsCounter = new FDynamicText(myRenderer, FVector3(0, 2.0f, 0), "FPS Counter", true);
	myRenderer->GetSceneGraph().AddObject(myFpsCounter, false);
}

bool FGame::Update(double aDeltaTime)
{
	// quit on escape
	if (myInput->IsKeyDown(VK_ESCAPE))
		return false;

	// update input if we have focus
	if (myRenderWindow->IsFocussed())
		myInput->Update();

	// update camera
	myCamera->Update(aDeltaTime);

	char buff[128];
	sprintf_s(buff, "%s %f\0", "Fps: ", 1.0f/static_cast<float>(aDeltaTime));
	myFpsCounter->SetText(buff);

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
