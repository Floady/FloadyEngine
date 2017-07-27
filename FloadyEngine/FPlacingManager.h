#pragma once
#include <vector>
#include "d3d12.h"
#include "FRenderableObject.h"
#include "FDelegate.h"

class FGameEntity;

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
	void SetPlacable(bool anIsCube, FVector3 aScale, FDelegate2<void(FVector3)>& aCB); //anIsCube should be replaced with mesh name
	void ClearPlacable();
	bool IsPlacing() const { return myObject != nullptr; }
private:

	FRenderableObject* myObject;
	FDelegate2<void (FVector3)> myPlaceCallback;
};

