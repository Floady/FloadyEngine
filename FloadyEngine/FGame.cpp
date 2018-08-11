#include "FUtilities.h"
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
#include "FGameLevel.h"
#include "FProfiler.h"
#include "FNavMeshManagerRecast.h"
#include "FObjLoader.h"
#include "FGameEntityObjModel.h"
#include <cmath>
#include <cstdlib>
#include "FBulletPhysics.h"
#include "FPathfindComponent.h"
#include "FJobSystem.h"
#include "FRenderMeshComponent.h"
#include "FThrowableTrajectory.h"
#include "FPhysicsComponent.h"

FGame* FGame::ourInstance = nullptr;

FD3d12Renderer::FMutex myMutex;

void * operator new(std::size_t n) throw(std::bad_alloc)
{
	//FPROFILE_FUNCTION("Alloc");
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

	FLOG("ConstructBuilding: %s", myBuildingName);

	FGameBuilding* entity = (myBuildingManager->GetBuildingTemplates().find(myBuildingName))->second->CreateBuilding();
	entity->SetPos(aPos);

	if(myLevel)
		myLevel->AddEntity(entity);
	
	myBuildingName = nullptr;
}

FThrowableTrajectory myThrowableTrajectory;

void FGame::ConstructBuilding(const char * aBuildingName)
{
	FLOG("ConstructBuilding: %s", aBuildingName);
	
	myBuildingName = aBuildingName;
	FGameBuilding* entity = (myBuildingManager->GetBuildingTemplates().find(myBuildingName))->second->CreateBuilding();
	FVector3 scale = entity->GetRepresentation()->GetRenderableObject()->GetScale();
	scale = entity->GetRepresentation()->GetComponentInSlot<FPhysicsComponent>(0) ? entity->GetRepresentation()->GetComponentInSlot<FPhysicsComponent>(0)->GetScale() : scale;
	myPlacingManager->SetPlacable(true, scale, FDelegate2<void(FVector3)>::from<FGame, &FGame::ConstructBuilding>(this), entity->GetRepresentation()->GetRenderableObject()->GetTexture());
	delete entity;
}

extern bool ourShouldRecalc;
FGame::FGame()
{
	// create the basics (renderer, input, camera) - and make the window message handler talk to our input system
	myInput = new FD3d12Input();
	auto someDelegate = FDelegate2<void(UINT, WPARAM, LPARAM)>::from<FD3d12Input, &FD3d12Input::MessageHandler>(myInput);
	myRenderWindow = new FRenderWindow(someDelegate);
	myRenderer = FD3d12Renderer::GetInstance();
	myCamera = new FGameCamera(myInput, 1600, 900);
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
	myLevel = nullptr;
	mySSAO = nullptr;
	myAA = nullptr;
	myFpsCounter = nullptr;

	myRenderPostEffectsJob = nullptr;
	myRenderToBuffersJob = nullptr;
	myExecuteCommandlistsJob = nullptr;
	myRegenerateNavMeshJob = nullptr;

	myRenderJobSys = FJobSystem::GetInstance();
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

	delete myFpsCounter;
	myFpsCounter = nullptr;

	delete FMeshManager::GetInstance();
}

void FGame::Init()
{
	//FPROFILE_FUNCTION("FGame Init");

	bool result = myRenderWindow->Initialize();
	
	if (!result)
	{
		FLOG("Failed to init Render window");
		return;
	}

	// Set up engine basics
	if (!myRenderer->Initialize(myRenderWindow->GetWindowHeight(), myRenderWindow->GetWindowWidth(), myRenderWindow->GetHWND(), false, false))// vsync + fullscreen here
	{
		FLOG("Failed to init Renderer");
		return;
	}
	myRenderer->SetCamera(myCamera);
	myPhysics->Init(myRenderer);
	myPhysics->SetPaused(false);
	
	// setup game basic systems
	myBuildingManager = new FGameBuildingManager();
	myGameUIManager = new FGameUIManager();
	myPlacingManager = new FPlacingManager();

	// fps counter for performance
	if(true)
	{
	myFpsCounter = new FDynamicText(myRenderer, FVector3(-1.0f, -1.0f, 0.0f),"FPS Counter", 0.3f, 0.05f , true, true);
	myRenderer->GetSceneGraph().AddObject(myFpsCounter, true); // transparant == nondeferred for now..
	}

	// Load level and add a sunlight
	myLevel = new FGameLevel("Configs//level3.txt");
	FLightManager::GetInstance()->AddDirectionalLight(FVector3(0, 5, -1), FVector3(0, -1, 1), FVector3(1.75f, 1.65f, 1.55f), 0.0f);

	// init navmesh	- old 2d navmesh
//	FNavMeshManager::GetInstance()->AddBlockingAABB(FVector3(5, 0, 5), FVector3(8, 0, 8));
//	FNavMeshManager::GetInstance()->GenerateMesh(FVector3(0, 0, 0), FVector3(20, 0, 20));

	// setup post process chain
	std::vector<FPostProcessEffect::BindInfo> resources;
	resources.push_back(FPostProcessEffect::BindInfo(myRenderer->GetGBufferTarget(0), myRenderer->gbufferFormat[0]));
	resources.push_back(FPostProcessEffect::BindInfo(myRenderer->GetGBufferTarget(1), myRenderer->gbufferFormat[1]));
	resources.push_back(FPostProcessEffect::BindInfo(myRenderer->GetGBufferTarget(2), myRenderer->gbufferFormat[2]));
	resources.push_back(FPostProcessEffect::BindInfo(myRenderer->GetGBufferTarget(3), myRenderer->gbufferFormat[3]));
	resources.push_back(FPostProcessEffect::BindInfo(myRenderer->GetGBufferTarget(4), myRenderer->gbufferFormat[4]));
	mySSAO = new FPostProcessEffect(resources, "SSAOShaderPost.hlsl", 1, "SSAO");
	myBlur = new FPostProcessEffect(resources, "SSAOBlurPost.hlsl", 0, "PostSSAOBlur");
	myAA = new FPostProcessEffect(resources, "lightshader2.hlsl", 0, "FXAA");
	FD3d12Renderer::GetInstance()->RegisterPostEffect(mySSAO);
	FD3d12Renderer::GetInstance()->RegisterPostEffect(myBlur);
	// this registers a post effect, we want it at the end cause SSAO + SSAOBlur needs to be first (it doesnt correctly propagate previous results)
	myHighlightManager = new FGameHighlightManager();
	FD3d12Renderer::GetInstance()->RegisterPostEffect(myAA);

	// set UI to in game for now
	myGameUIManager->SetState(FGameUIManager::GuiState::InGame);
}

bool FGame::Update(double aDeltaTime)
{
	FThrowableTrajectory::TrajectoryPath path;
	FVector3 startPos = FVector3(0, 5, 0);
	FVector3 dir = FVector3(0.6, 1, 1); dir.Normalize();
	FVector3 lastPos = startPos;
	static float trajectoryVelocity = 10.0f;

	FJob* clearBufferJob = nullptr;
	FD3d12Renderer::GetInstance()->InitFrame();
	clearBufferJob = myRenderJobSys->QueueJob((FDelegate2<void()>(this, &FGame::ClearBuffersAsync)));

	//myThrowableTrajectory.Simulate(startPos, FVector3(0.6, 1, 1), trajectoryVelocity, path, 1000.0f);
	//
	//for (FThrowableTrajectory::TrajectoryPathSegment& segment : path.mySegments)
	//{
	//	FD3d12Renderer::GetInstance()->GetDebugDrawer()->drawLine(lastPos, segment.myPosition, FVector3(1.0f, 1.0f, 1.0f));
	//	lastPos = segment.myPosition;
	//}

	myRenderJobSys->QueueJob(FDelegate2<void()>(FMeshManager::GetInstance(), &FMeshManager::InitLoadedMeshesD3D));
	myRenderJobSys->QueueJob(FDelegate2<void()>(FD3d12Renderer::GetInstance(), &FD3d12Renderer::InitNewTextures));

	{
		// update FPS
		if(myFpsCounter)
		{
			char buff[128];
			sprintf_s(buff, "%s %f\0", "Fps: ", 1.0f / static_cast<float>(aDeltaTime));
			myFpsCounter->SetText(buff);
		}
		
		{
			//myRenderJobSys->WaitForAllJobs();
			//clearBufferJob = myRenderJobSys->QueueJob((FDelegate2<void()>(this, &FGame::ClearBuffersAsync)));
		}

		FPROFILE_FUNCTION("FGame Update");

		//*
		// this should have dependencies - but works because 1 workerthread
		FJob* lightManagerQueue = nullptr;
		lightManagerQueue = myRenderJobSys->QueueJob(FDelegate2<void()>(FLightManager::GetInstance(), &FLightManager::UpdateViewProjMatrices));
		lightManagerQueue = myRenderJobSys->QueueJob(FDelegate2<void()>(FLightManager::GetInstance(), &FLightManager::ResetVisibleAABB), false, lightManagerQueue);
		lightManagerQueue = myRenderJobSys->QueueJob(FDelegate2<void()>(FLightManager::GetInstance(), &FLightManager::ResetHasMoved), false, lightManagerQueue);
		// myRenderJobSys->WaitForAllJobs(); // moved a bit lower, before level update
		/*/
		FLightManager::GetInstance()->UpdateViewProjMatrices();
		FLightManager::GetInstance()->ResetVisibleAABB();
		FLightManager::GetInstance()->ResetHasMoved();
		//*/
		
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
			myRenderJobSys->QueueJob((FDelegate2<void()>(this, &FGame::RegenerateNavMesh)));
		}

		if (myInput->IsKeyDown(VK_F4))
		{
			myGameUIManager->SetState(FGameUIManager::GuiState::InGame);
			ourShouldRecalc = false;
		}

		if (myInput->IsKeyDown(VK_F5))
		{
			myGameUIManager->SetState(FGameUIManager::GuiState::Debug);
			ourShouldRecalc = true;
		}
		if (myInput->IsKeyDown(VK_F6))
		{
			myGameUIManager->SetState(FGameUIManager::GuiState::No_UI);
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
			bool isonUI = FGUIManager::GetInstance()->Update(windowMouseX, windowMouseY, myInput->IsMouseButtonDown(true), myInput->IsMouseButtonDown(false));
		
			// example entity picking
			if (!myWasLeftMouseDown && myInput->IsMouseButtonDown(true))
			{
				FVector3 navPos = myPicker->PickNavMeshPos(FVector3(windowMouseX, windowMouseY, 0.0f));
			
				if(!isonUI)
				{
					bool wasPlacing = myPlacingManager->IsPlacing();
					myPlacingManager->MouseDown(navPos);

					if(!wasPlacing)
					{
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

			if(myPlacingManager->IsPlacing())
			{
				FVector3 navPos = myPicker->PickNavMeshPos(FVector3(windowMouseX, windowMouseY, 0.0f));
				myPlacingManager->UpdateMousePos(navPos);
			}
		}

		myWasLeftMouseDown = myInput->IsMouseButtonDown(true);
		myWasRightMouseDown = myInput->IsMouseButtonDown(true);


		myMutex.WaitFor();
	
		{
			FPROFILE_FUNCTION_CUSTOM("Wait for lightmanager", 0xFFFFAAAA);
			lightManagerQueue->WaitForFinish();
		}
		
		// Update world
		{
			FPROFILE_FUNCTION_CUSTOM("Level update", 0xFFFFFF00);
			myLevel->Update(aDeltaTime);
		}
		// Step physics
		if (myInput->IsKeyDown(VK_SPACE))
		{
			myPhysics->TogglePaused();
		}

		myPhysics->Update(aDeltaTime);

		if (myInput->IsKeyDown(VK_SHIFT))
		{
			//myRenderer->GetCamera()->SetOverrideLight(true);
		}
		else
		{
			//myRenderer->GetCamera()->SetOverrideLight(false);
		}

		FNavMeshManagerRecast::GetInstance()->DebugDraw();

		myLevel->PostPhysicsUpdate();

		myGameUIManager->Update();
	}

	// Render world
	{
		FPROFILE_FUNCTION("FGame Render");
		
		// record cmdlists for render world
		myRenderJobSys->QueueJob((FDelegate2<void()>(this, &FGame::RenderWorldAsync)));
		

		// update SSAO buffers
		float constData[80];
		memcpy(&constData[0], myCamera->GetInvViewProjMatrix().m, 16 * sizeof(float));

		DirectX::XMFLOAT4X4 projMatrix;
		XMStoreFloat4x4(&projMatrix, XMMatrixTranspose(myCamera->myProjMatrix));
		memcpy(&constData[16], projMatrix.m, 16 * sizeof(float));

		DirectX::XMFLOAT4X4 invProjMatrix;
		XMStoreFloat4x4(&invProjMatrix, XMMatrixTranspose(XMMatrixInverse(nullptr, myCamera->myProjMatrix)));
		memcpy(&constData[32], invProjMatrix.m, 16 * sizeof(float));

		DirectX::XMFLOAT4X4 viewMtx;
		XMStoreFloat4x4(&viewMtx, XMMatrixTranspose(myCamera->_viewMatrix));
		memcpy(&constData[48], viewMtx.m, 16 * sizeof(float));

		DirectX::XMFLOAT4X4 invViewMtx;
		XMStoreFloat4x4(&invViewMtx, XMMatrixTranspose(XMMatrixInverse(nullptr, myCamera->_viewMatrix)));
		memcpy(&constData[64], invViewMtx.m, 16 * sizeof(float));

		mySSAO->WriteConstBuffer(0, &constData[0], 80 * sizeof(float));

		myRenderWindow->CheckForQuit();
	}

	{
		FPROFILE_FUNCTION_CUSTOM("WaitFor clearbuff renderworld", 0xFFFFAAAA);
		myRenderJobSys->WaitForAllJobs();
	}

	{
		FPROFILE_FUNCTION_CUSTOM("HighlightManager", 0xFFFFAAAA);
		myHighlightManager->Render();
	}

	{
		FPROFILE_FUNCTION_CUSTOM("Profiler", 0xFFFFAAAA);
		FProfiler::GetInstance()->Render();
		FProfiler::GetInstance()->StartFrame();
	}

	myMutex.Lock();

	FD3d12Renderer::GetInstance()->WaitForRender();
	myExecuteCommandlistsJob = myRenderJobSys->QueueJob((FDelegate2<void()>(this, &FGame::RenderAsync)));
	myRenderPostEffectsJob = myRenderJobSys->QueueJob((FDelegate2<void()>(this, &FGame::RenderPostEffectsAsync)), false, myExecuteCommandlistsJob);

	if (!myRegenerateNavMeshJob || (myPhysics->HasNewNavBlockers() && myRegenerateNavMeshJob->myFinished))
	{
		myPhysics->ResetHasNewNavBlockers();
		myRegenerateNavMeshJob = myRenderJobSys->QueueJob((FDelegate2<void()>(this, &FGame::RegenerateNavMesh)), true);
	}

	// wait for short jobs before processing next frame
	{
		FPROFILE_FUNCTION_CUSTOM("EndFrame", 0xFFFFAAAA);
		{
			FPROFILE_FUNCTION_CUSTOM("WaitForJobs", 0xFFFFAAAA);
			myRenderJobSys->WaitForAllJobs();
			myRenderJobSys->ResetQueue();
		}

		{
			FPROFILE_FUNCTION_CUSTOM("WaitForRender", 0xFFFFAAAA);
			FD3d12Renderer::GetInstance()->WaitForRender();
		}
	}

	return true;
}

void FGame::RenderAsync()
{
	FPROFILE_FUNCTION("RenderAsync");
	myRenderer->Render();
	myMutex.Unlock();
}

void FGame::RenderPostEffectsAsync()
{
	FPROFILE_FUNCTION("RenderPostEffectsAsync");
	myRenderer->RenderPostEffects();
}

void FGame::ClearBuffersAsync()
{
	FPROFILE_FUNCTION("Clear buffers");
	FD3d12Renderer::GetInstance()->RecordClearBuffers();
	FD3d12Renderer::GetInstance()->DoClearBuffers();
}

void FGame::RenderWorldAsync()
{
	FPROFILE_FUNCTION("RenderWorldAsync");
	FD3d12Renderer::GetInstance()->RecordRenderToGBuffer();
	FD3d12Renderer::GetInstance()->RecordShadowPass();
	FD3d12Renderer::GetInstance()->RecordPostProcess();
	FD3d12Renderer::GetInstance()->RecordDebugDrawer();
}

void FGame::RegenerateNavMesh()
{
	// re-init navmesh
	std::vector<FBulletPhysics::AABB> aabbs = myPhysics->GetAABBs();
	FNavMeshManagerRecast::GetInstance()->RemoveAllBlockingAABB();
	for (FBulletPhysics::AABB& aabb : aabbs)
	{
		FNavMeshManagerRecast::GetInstance()->AddBlockingAABB(aabb.myMin, aabb.myMax);
	}

	FNavMesh* newNavMesh = new FNavMesh();
	newNavMesh->myAABBList = FNavMeshManagerRecast::GetInstance()->myAABBList;
	newNavMesh->Generate(FNavMeshManagerRecast::GetInstance()->GetInputMesh());

	FNavMesh* curNavMesh = FNavMeshManagerRecast::GetInstance()->GetActiveNavMesh();
	FNavMeshManagerRecast::GetInstance()->SetActiveNavMesh(newNavMesh);

	delete curNavMesh;
}
