#pragma once

#include <vector>
#include "FVector3.h"

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

class FGame
{
public:
	void Init();
	void Render();
	bool Update(double aDeltaTime);
	static FGame* GetInstance() { return ourInstance; }
	FD3d12Renderer* GetRenderer() { return myRenderer; }
	FD3d12Input* GetInput() { return myInput; }
	FBulletPhysics* GetPhysics() { return myPhysics; }
	void ConstructBuilding(const char* aBuildingName);
	void ConstructBuilding(FVector3 aPos);
	FGameBuildingManager* GetBuildingManager() { return myBuildingManager; }
	void Test();
	void Test(FVector3 aPos);
	void AddEntity(FGameEntity* anEntity);
	FGame();
	~FGame();
	const FGameEntity* GetSelectedEntity() const { return myPickedEntity; }
private:
	const char* myBuildingName;
private:
	FRenderWindow* myRenderWindow;
	FD3d12Renderer* myRenderer;
	FD3d12Input* myInput;
	FGameCamera* myCamera;
	FBulletPhysics* myPhysics;

	FTimer* myFrameTimer;
	double myFrameTime;
	FDynamicText* myFpsCounter;

	static FGame* ourInstance;
	std::vector<FGameEntity*> myEntityContainer;
	bool myIsMouseCaptured;
	F3DPicker* myPicker;
	FGameEntity* myPickedEntity;
	FGameAgent* myPickedAgent;
	FGameHighlightManager* myHighlightManager;
	FPlacingManager* myPlacingManager;
	FGameBuildingManager* myBuildingManager;
	FGameUIManager* myGameUIManager;
};

