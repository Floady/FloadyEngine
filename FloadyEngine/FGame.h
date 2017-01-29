#pragma once

class FDynamicText;
class FD3d12Input;
class FGameCamera;
class FTimer;
class FRenderWindow;
class FD3d12Renderer;

class FGame
{
public:
	void Init();
	void Render();
	bool Update(double aDeltaTime);
	FGame();
	~FGame();

private:
	FRenderWindow* myRenderWindow;
	FD3d12Renderer* myRenderer;
	FD3d12Input* myInput;
	FGameCamera* myCamera;

	FTimer* myFrameTimer;
	double myFrameTime;
	FDynamicText* myFpsCounter;
};

