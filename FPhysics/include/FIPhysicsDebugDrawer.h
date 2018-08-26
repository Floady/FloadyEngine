#pragma once

#include "FVector3.h"

class FIPhysicsDebugDrawer
{
public:
	FIPhysicsDebugDrawer() {}
	virtual ~FIPhysicsDebugDrawer() {}

	virtual void DrawTriangle(const FVector3& aV1, const FVector3& aV2, const FVector3& aV3, const FVector3& aColor) = 0;
	virtual void drawLine(const FVector3 & from, const FVector3 & to, const FVector3 & color) = 0;
};