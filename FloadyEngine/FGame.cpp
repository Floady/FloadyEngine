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
#include "FDelaunayTriangulate.h"
#include "F3DPicker.h"

FGame* FGame::ourInstance = nullptr;

void FGame::Test()
{
	{ OutputDebugString(L"Test"); }

	FGameEntity* gameEntity = new FGameEntity(myCamera->GetPos(), FVector3(1, 1, 1), FGameEntity::ModelType::Box, 15.0f, true);
	myEntityContainer.push_back(gameEntity);
	
}

static FDelaunayTriangulate ourTriangulator;
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
	myRenderer->GetSceneGraph().AddObject(myPhysics->GetDebugDrawer(), true); // nondeferred debug
	
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
		myEntityContainer.push_back(new FGameEntity(*child));
		child = level->GetNextChild();
	}

	// init navmesh
	ourTriangulator.AddBlockingAABB(FVector3(5, 0, 5), FVector3(8, 0, 8));
	ourTriangulator.GenerateMesh(FVector3(0, 0, 0), FVector3(20, 0, 20));
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
		ourTriangulator.RemoveAllBlockingAABB();
		for (FBulletPhysics::AABB& aabb : aabbs)
		{
			ourTriangulator.AddBlockingAABB(aabb.myMin, aabb.myMax);
		}

		ourTriangulator.GenerateMesh(FVector3(-20, 0, -20), FVector3(20, 0, 20));
	}

	static FVector3 PickPosFromNav;
	static FVector3 StartPickPosFromNav;
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
		FVector3 pickPosNear = myPicker->UnProject(FVector3(windowMouseX, windowMouseY, 0.0f));
		FVector3 pickPosFar = myPicker->UnProject(FVector3(windowMouseX, windowMouseY, 1.0f));
		FVector3 direction = (pickPosFar - pickPosNear).Normalized();
		float dist = (pickPosFar - pickPosNear).Length();
		FVector3 line = pickPosNear + (direction * -(pickPosNear.y / direction.y));
		float size = 0.1f;
		myPhysics->GetDebugDrawer()->DrawTriangle(line + FVector3(-size, 0, -size), line + FVector3(-size, 0, size), line + FVector3(size, 0, size), FVector3(1, 1, 0));
		myPhysics->GetDebugDrawer()->drawLine(pickPosNear, line, FVector3(1, 1, 0));
		PickPosFromNav = line;
		if (myInput->IsMouseButtonDown(false))
		{
			StartPickPosFromNav = line;
		}
	}

	for (int i = 0; i < ourTriangulator.pts.size(); i++)
	{
		FVector3 pos = FVector3(ourTriangulator.pts[i].r, 0.1f, ourTriangulator.pts[i].c);
		float size = 0.1f;
		myPhysics->GetDebugDrawer()->DrawTriangle(pos + FVector3(-size, 0, -size), pos + FVector3(-size, 0, size), pos + FVector3(size, 0, size), FVector3(1, 1, 0));
	}

	for (int i = 0; i < ourTriangulator.triads.size(); i++)
	{
		Shx& pointA = ourTriangulator.hull[ourTriangulator.triads[i].a];
		Shx& pointB = ourTriangulator.hull[ourTriangulator.triads[i].b];
		Shx& pointC = ourTriangulator.hull[ourTriangulator.triads[i].c];
		FVector3 vPointA = FVector3(pointA.r, 0, pointA.c);
		FVector3 vPointB = FVector3(pointB.r, 0, pointB.c);
		FVector3 vPointC = FVector3(pointC.r, 0, pointC.c);
		FVector3 vCenter = (vPointA + vPointB + vPointC) / 3.0f;
		float shrinkFact = 0.0f;
		vPointA -= (vPointA - vCenter) * shrinkFact;
		vPointB -= (vPointB - vCenter) * shrinkFact;
		vPointC -= (vPointC - vCenter) * shrinkFact;
		
		FVector3 color = FVector3(0, 0.3f + (i % 20) / 20.0f, 0);
		
		// this was for highlighting the triangle winding
		int highlightTri = 9999; // 4,0,2,8
		if (ourTriangulator.IsBlocked(i))
			color = FVector3(0.3f + ((i % 20) / 20.0f), 0, 0);
		if (highlightTri == i)
			color = FVector3(0, 0, (i % 20) / 20.0f);
		myPhysics->GetDebugDrawer()->DrawTriangle(vPointA, vPointB, vPointC, color);

		if (i == highlightTri)
		{
			float size = 0.3f;
			myPhysics->GetDebugDrawer()->DrawTriangle(vPointA + FVector3(-size, 0, -size), vPointA + FVector3(-size, 0, size), vPointA + FVector3(size, 0, size), FVector3(1, 0, 0));
			myPhysics->GetDebugDrawer()->DrawTriangle(vPointB + FVector3(-size, 0, -size), vPointB + FVector3(-size, 0, size), vPointB + FVector3(size, 0, size), FVector3(0, 1, 0));
			myPhysics->GetDebugDrawer()->DrawTriangle(vPointC + FVector3(-size, 0, -size), vPointC + FVector3(-size, 0, size), vPointC + FVector3(size, 0, size), FVector3(0, 0, 1));
		}

	}

	FVector3 vNavStart = FVector3(5.0f, 0.1f, 2.0f);
	vNavStart = StartPickPosFromNav;
	FVector3 vNavEnd = FVector3(3.5f, 0.1f, 15.0f);
	vNavEnd = PickPosFromNav;
	float size = 0.1f;
	myPhysics->GetDebugDrawer()->DrawTriangle(vNavStart + FVector3(-size, 0, -size), vNavStart + FVector3(-size, 0, size), vNavStart + FVector3(size, 0, size), FVector3(1, 1, 1));
	myPhysics->GetDebugDrawer()->DrawTriangle(vNavEnd + FVector3(-size, 0, -size), vNavEnd + FVector3(-size, 0, size), vNavEnd + FVector3(size, 0, size), FVector3(1, 1, 1));
	std::vector<FVector3> pathPoints = ourTriangulator.FindPath(vNavStart, vNavEnd);

	for (int i = 1; i < pathPoints.size(); i++)
	{
		float shade = 0.5f + (i * (0.5f / pathPoints.size()));
		FVector3 from = FVector3(pathPoints[i - 1].x, 0.1f, pathPoints[i - 1].z);
		FVector3 to = FVector3(pathPoints[i].x, 0.1f, pathPoints[i].z);
		myPhysics->GetDebugDrawer()->drawLine(from, to, FVector3(shade, shade, shade));
	}

	for (int i = 0; i < ourTriangulator.myFunnel.size(); i++)
	{
		float shade = 0.5f + (i * (0.5f / pathPoints.size()));
		float size = 0.1f;
		FVector3 pos = ourTriangulator.myFunnel[i].aLeft;
		pos.y = 0.2f;
		myPhysics->GetDebugDrawer()->DrawTriangle(pos + FVector3(-size, 0, -size), pos + FVector3(-size, 0, size), pos + FVector3(size, 0, size), FVector3(1, 0.5f, 0.5f));
		pos = ourTriangulator.myFunnel[i].aRight;
		pos.y = 0.15f;
		 size = 0.15f;
		myPhysics->GetDebugDrawer()->DrawTriangle(pos + FVector3(-size, 0, -size), pos + FVector3(-size, 0, size), pos + FVector3(size, 0, size), FVector3(0, 1, 0));
	}

	// Update world
	for (FGameEntity* entity : myEntityContainer)
	{
		entity->Update();
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
