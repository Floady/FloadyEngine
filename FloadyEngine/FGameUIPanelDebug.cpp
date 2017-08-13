#include "FGameUIPanelDebug.h"
#include "FGUIButtonToggle.h"
#include "FLightManager.h"
#include "FNavMeshManagerRecast.h"
#include "FBulletPhysics.h"
#include "FGame.h"
#include "FD3d12Renderer.h"
#include "FProfiler.h"

FGameUIPanelDebug::FGameUIPanelDebug()
{
	FVector3 pos(0,0,0);
	FVector3 size(0.1, 0.05, 0);
	FVector3 offset(0, 0.05, 0);

	FGUIButtonToggle* button = new FGUIButtonToggle(pos, pos + size, "buttonBlanco.png", FDelegate2<void(bool)>::from<FLightManager, &FLightManager::SetDebugDrawEnabled>(FLightManager::GetInstance()));
	button->SetDynamicText("Light debug");
	AddObject(button);
	pos += offset;

	FGUIButtonToggle* button2 = new FGUIButtonToggle(pos, pos + size, "buttonBlanco.png", FDelegate2<void(bool)>::from<FNavMeshManagerRecast, &FNavMeshManagerRecast::SetDebugDrawEnabled>(FNavMeshManagerRecast::GetInstance()));
	button2->SetDynamicText("Navmesh debug");		
	AddObject(button2);								
	pos += offset;
													
	FGUIButtonToggle* button3 = new FGUIButtonToggle(pos, pos + size, "buttonBlanco.png", FDelegate2<void(bool)>::from<FBulletPhysics, &FBulletPhysics::SetDebugDrawEnabled>(FGame::GetInstance()->GetPhysics()));
	button3->SetDynamicText("Phys debug");			 
	AddObject(button3);								 
	pos += offset;
													 
	FGUIButtonToggle* button4 = new FGUIButtonToggle(pos, pos + size, "buttonBlanco.png", FDelegate2<void(bool)>::from<FD3d12Renderer, &FD3d12Renderer::SetDebugDrawEnabled>(FD3d12Renderer::GetInstance()));
	button4->SetDynamicText("AABB debug");			 
	AddObject(button4);								 
	pos += offset;

	FGUIButtonToggle* button5 = new FGUIButtonToggle(pos, pos + size, "buttonBlanco.png", FDelegate2<void(bool)>::from<FBulletPhysics, &FBulletPhysics::SetPaused>(FGame::GetInstance()->GetPhysics()));
	button5->SetDynamicText("Pause phys");
	AddObject(button5);
	pos += offset;

	FGUIButtonToggle* button6 = new FGUIButtonToggle(pos, pos + size, "buttonBlanco.png", FDelegate2<void(bool)>::from<FProfiler, &FProfiler::SetVisible>(FProfiler::GetInstance()));
	button6->SetDynamicText("Show profiler");
	AddObject(button6);
	pos += offset;
}

void FGameUIPanelDebug::Update()
{
}


FGameUIPanelDebug::~FGameUIPanelDebug()
{
}
