#pragma once
#include "FVector3.h"

class FRenderWindow;
class FCamera;

class F3DPicker
{
public:
	F3DPicker(FCamera* aCam, FRenderWindow* aWindow);
	~F3DPicker();
	FVector3 PickNavMeshPos(FVector3 aMousePos); // navmesh is y = 0 and with x/z bounds, otherwsie we can do full triangle test with ray
	FVector3 UnProject(FVector3 aMousePos);
private:
	FCamera* myCamera;
	FRenderWindow* myWindow;
};

