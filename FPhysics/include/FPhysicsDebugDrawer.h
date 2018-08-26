#pragma once
#include "FVector3.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btIDebugDraw.h"
#include "FIPhysicsDebugDrawer.h"

class FD3d12Renderer;
class FDebugDrawer;

class FPhysicsDebugDrawer : public btIDebugDraw
{
public:
	FPhysicsDebugDrawer(FIPhysicsDebugDrawer* aDebugDrawer);
	~FPhysicsDebugDrawer();

	void	drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;
	void	reportErrorWarning(const char* warningString) override;
	void	draw3dText(const btVector3& location, const char* textString) override;
	void	setDebugMode(int debugMode) override;
	int		getDebugMode() const override;
	void	drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override;

	void	DrawTriangle(const FVector3& aV1, const FVector3& aV2, const FVector3& aV3, const FVector3& aColor);
private:
	FIPhysicsDebugDrawer* myDebugDrawer;
};

