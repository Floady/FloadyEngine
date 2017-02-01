#pragma once

#include <vector>

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
	FGame();
	~FGame();

private:
	FRenderWindow* myRenderWindow;
	FD3d12Renderer* myRenderer;
	FD3d12Input* myInput;
	FGameCamera* myCamera;
	FBulletPhysics* myPhysics;

	FTimer* myFrameTimer;
	double myFrameTime;
	FDynamicText* myFpsCounter;
	FDynamicText* myYoloSign;

	static FGame* ourInstance;
	std::vector<FGameEntity*> myEntityContainer;
};

