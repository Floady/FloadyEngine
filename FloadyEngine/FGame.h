#pragma once

#include <vector>
#include "FVector3.h"

class FGameLevel;
class FPlacingManager;
class FScreenQuad;
class FGameEntity;
class FBulletPhysics;
class FDynamicText;
class FD3d12Input;
class FGameCamera;
class FTimer;
class FRenderWindow;
class FD3d12Renderer;
class FPrimitiveBox;
class btRigidBody;
class F3DPicker;
class FNavMeshManager;
class FGameAgent;
class FGameHighlightManager;
class FGameBuildingManager;
class FGameUIManager;
class FPostProcessEffect;
class FJobSystem;
struct FJob;

class FGame
{
public:
	void Init();
	bool Update(double aDeltaTime);
	void RenderAsync();
	void RenderPostEffectsAsync();
	void ClearBuffersAsync();
	void RenderWorldAsync();
	void RegenerateNavMesh();
	static FGame* GetInstance() { return ourInstance; }
	FD3d12Renderer* GetRenderer() { return myRenderer; }
	FD3d12Input* GetInput() { return myInput; }
	FBulletPhysics* GetPhysics() { return myPhysics; }
	void ConstructBuilding(const char* aBuildingName);
	FPlacingManager* GetPlacingManager() { return myPlacingManager; }
	void ConstructBuilding(FVector3 aPos);
	FGameBuildingManager* GetBuildingManager() { return myBuildingManager; }
	FGameLevel* GetLevel() { return myLevel; }
	FGame();
	~FGame();
	FGameEntity* GetSelectedEntity() const { return myPickedEntity; }
private:
	const char* myBuildingName;
private:
	FRenderWindow* myRenderWindow;
	FD3d12Renderer* myRenderer;
	FGameLevel* myLevel;
	FD3d12Input* myInput;
	FGameCamera* myCamera;
	FBulletPhysics* myPhysics;

	FTimer* myFrameTimer;
	double myFrameTime;
	FDynamicText* myFpsCounter;

	static FGame* ourInstance;
	bool myIsMouseCaptured;
	F3DPicker* myPicker;
	FGameEntity* myPickedEntity;
	FGameAgent* myPickedAgent;
	FGameHighlightManager* myHighlightManager;
	FPlacingManager* myPlacingManager;
	FGameBuildingManager* myBuildingManager;
	FGameUIManager* myGameUIManager;

	bool myWasRightMouseDown;
	bool myWasLeftMouseDown;

	FPostProcessEffect* myAA;
	FPostProcessEffect* mySSAO;
	FPostProcessEffect* myBlur;
	FJobSystem* myRenderJobSys;

	FJob* myRenderPostEffectsJob;
	FJob* myRegenerateNavMeshJob;
	FJob* myRenderToBuffersJob;
	FJob* myExecuteCommandlistsJob;
};

