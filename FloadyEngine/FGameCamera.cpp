#include "FGameCamera.h"
#include "FD3d12Input.h"



FGameCamera::FGameCamera(FD3d12Input* anInputSystem, float aWidth, float aHeight) : FCamera(aWidth, aHeight)
{
	myInputSystem = anInputSystem;
}

FGameCamera::~FGameCamera()
{
}

void FGameCamera::Update(double aDeltaTime)
{
	float movSpeed = static_cast<float>(40 * aDeltaTime);
	if (myInputSystem->IsKeyDown(65))
		Move(-movSpeed, 0, 0);
	if (myInputSystem->IsKeyDown(68))
		Move(movSpeed, 0, 0);
	if (myInputSystem->IsKeyDown(87))
		Move(0, 0, movSpeed);
	if (myInputSystem->IsKeyDown(83))
		Move(0, 0, -movSpeed);
	
	int deltaX =  myInputSystem->GetDeltaMouseX();
	int deltaY = myInputSystem->GetDeltaMouseY();
		
	// don't time correct rotatio nsince the mousemove per frame is shorter when frames are shorter times
	// should be direction normalized times frametime?
	float rotSpeed = 0.01f;
	Yaw(deltaX * rotSpeed);
	Pitch(deltaY * rotSpeed);

	UpdateViewProj();
}