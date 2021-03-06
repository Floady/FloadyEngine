#pragma once
#include <vector>
#include "FRenderableObject.h"
#include "d3d12.h"
#include "FDelegate.h"

class FGameEntity;
class FPrimitiveBoxColorOverride;

class FPlacingManagerObject : public FRenderableObject
{
};

class FPlacingManager
{
public:
	FPlacingManager();
	~FPlacingManager();

	void UpdateMousePos(const FVector3& aPos);
	void MouseDown(const FVector3& aPos);
	void SetPlacable(bool anIsCube, FVector3 aScale, FDelegate2<void(FVector3)>& aCB, const char* aTex = nullptr); //anIsCube should be replaced with mesh name
	void ClearPlacable();
	bool IsPlacing() const { return myObject != nullptr; }
private:

	FPrimitiveBoxColorOverride* myObject;
	FDelegate2<void (FVector3)> myPlaceCallback;
	bool myFitsOnNavMesh;
};

