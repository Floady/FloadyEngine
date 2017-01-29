#pragma once
#include "FCamera.h"

class FD3d12Input;

class FGameCamera :
	public FCamera
{
public:
	FGameCamera(FD3d12Input* anInputSystem, float aWidth, float aHeight);
	~FGameCamera();
	void Update(double aDeltaTime);
private:
	FD3d12Input* myInputSystem;
};

