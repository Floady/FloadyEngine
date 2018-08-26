#include "FGameUIPanelDebug.h"
#include "FGUIButtonToggle.h"
#include "FLightManager.h"
#include "FNavMeshManagerRecast.h"
#include "FGame.h"
#include "FD3d12Renderer.h"
#include "FProfiler.h"
#include "FSceneGraph.h"
#include "FPhysicsWorld.h"

FGameUIPanelDebug::FGameUIPanelDebug()
{
	FVector3 pos(0,0,0);
	FVector3 size(0.1f, 0.05f, 0);
	FVector3 offset(0, 0.05f, 0);

	FGUIButtonToggle* button = new FGUIButtonToggle(pos, pos + size, "buttonBlanco.png", FDelegate2<void(bool)>::from<FLightManager, &FLightManager::SetDebugDrawEnabled>(FLightManager::GetInstance()));
	button->SetDynamicText("Light debug");
	AddObject(button);
	pos += offset;

	FGUIButtonToggle* button2 = new FGUIButtonToggle(pos, pos + size, "buttonBlanco.png", FDelegate2<void(bool)>::from<FNavMeshManagerRecast, &FNavMeshManagerRecast::SetDebugDrawEnabled>(FNavMeshManagerRecast::GetInstance()));
	button2->SetDynamicText("Navmesh debug");		
	AddObject(button2);								
	pos += offset;
													
	FGUIButtonToggle* button3 = new FGUIButtonToggle(pos, pos + size, "buttonBlanco.png", FDelegate2<void(bool)>::from<FPhysicsWorld, &FPhysicsWorld::SetDebugDrawEnabled>(FGame::GetInstance()->GetPhysics()));
	button3->SetDynamicText("Phys debug");			 
	AddObject(button3);								 
	pos += offset;
													 
	FGUIButtonToggle* button4 = new FGUIButtonToggle(pos, pos + size, "buttonBlanco.png", FDelegate2<void(bool)>::from<FD3d12Renderer, &FD3d12Renderer::SetDebugDrawEnabled>(FD3d12Renderer::GetInstance()));
	button4->SetDynamicText("AABB debug");			 
	AddObject(button4);								 
	pos += offset;

	FGUIButtonToggle* button5 = new FGUIButtonToggle(pos, pos + size, "buttonBlanco.png", FDelegate2<void(bool)>::from<FPhysicsWorld, &FPhysicsWorld::SetPaused>(FGame::GetInstance()->GetPhysics()));
	button5->SetDynamicText("Pause phys");
	AddObject(button5);
	pos += offset;

	FGUIButtonToggle* button6 = new FGUIButtonToggle(pos, pos + size, "buttonBlanco.png", FDelegate2<void(bool)>::from<FProfiler, &FProfiler::SetVisible>(FProfiler::GetInstance()));
	button6->SetDynamicText("Show profiler");
	AddObject(button6);
	pos += offset;

	FGUIButtonToggle* button7 = new FGUIButtonToggle(pos, pos + size, "buttonBlanco.png", FDelegate2<void(bool)>::from<FLightManager, &FLightManager::SetFreezeDebug>(FLightManager::GetInstance()));
	button7->SetDynamicText("Freeze LightDebug");
	AddObject(button7);
	pos += offset;

	FGUIButtonToggle* button8 = new FGUIButtonToggle(pos, pos + size, "buttonBlanco.png", FDelegate2<void(bool)>::from<FCamera, &FCamera::SetOverrideLight>(FD3d12Renderer::GetInstance()->GetCamera()));
	button8->SetDynamicText("Override Light");
	AddObject(button8);
	pos += offset;

	FGUIButtonToggle* button9 = new FGUIButtonToggle(pos, pos + size, "buttonBlanco.png", FDelegate2<void(bool)>::from<FCamera, &FCamera::SetFreezeDebug>(FD3d12Renderer::GetInstance()->GetCamera()));
	button9->SetDynamicText("CamDebug Frz");
	AddObject(button9);
	pos += offset;
	
	FGUIButtonToggle* button10 = new FGUIButtonToggle(pos, pos + size, "buttonBlanco.png", FDelegate2<void(bool)>::from<FCamera, &FCamera::SetDebugDrawEnabled>(FD3d12Renderer::GetInstance()->GetCamera()));
	button10->SetDynamicText("CamDebug Draw");
	AddObject(button10);
	pos += offset;

	pos += offset;
	FVector3 sizePosLabel(0.3f, 0.05f, 0);
	myCameraPositionLabel = new FGUIButton(pos, pos + sizePosLabel, "buttonBlanco.png", FDelegate2<void()>::from<FCamera, &FCamera::UpdateViewProj>(FD3d12Renderer::GetInstance()->GetCamera()));
	AddObject(myCameraPositionLabel);
}

void FGameUIPanelDebug::Update()
{
	char buff[128];
	FVector3 pos = FGame::GetInstance()->GetRenderer()->GetCamera()->GetPos();
	sprintf(buff, "pos: %4.2f, %4.2f, %4.2f", pos.x, pos.y, pos.z);
	myCameraPositionLabel->SetDynamicText(buff);
}


FGameUIPanelDebug::~FGameUIPanelDebug()
{
}
