#include "FPhysicsDebugDrawer.h"
#include "FDebugDrawer.h"
#include "FProfiler.h"


FPhysicsDebugDrawer::FPhysicsDebugDrawer(FDebugDrawer* aDebugDrawer)
	: btIDebugDraw()
	, myDebugDrawer(aDebugDrawer)
{

}


FPhysicsDebugDrawer::~FPhysicsDebugDrawer()
{
}

// TODO: optimize, with many lines this takes about 5ms+
void FPhysicsDebugDrawer::drawLine(const btVector3 & from, const btVector3 & to, const btVector3 & color)
{
	FVector3 fcolor(color.getX(), color.getY(), color.getZ());
	FVector3 start(from.getX(), from.getY(), from.getZ());
	FVector3 end(to.getX(), to.getY(), to.getZ());
	myDebugDrawer->drawLine(start, end, fcolor);
}

void FPhysicsDebugDrawer::reportErrorWarning(const char * warningString)
{
}

void FPhysicsDebugDrawer::draw3dText(const btVector3 & location, const char * textString)
{
}

void FPhysicsDebugDrawer::setDebugMode(int debugMode)
{
}

//
//enum	DebugDrawModes
//{
//	DBG_NoDebug = 0,
//	DBG_DrawWireframe = 1,
//	DBG_DrawAabb = 2,
//	DBG_DrawFeaturesText = 4,
//	DBG_DrawContactPoints = 8,
//	DBG_NoDeactivation = 16,
//	DBG_NoHelpText = 32,
//	DBG_DrawText = 64,
//	DBG_ProfileTimings = 128,
//	DBG_EnableSatComparison = 256,
//	DBG_DisableBulletLCP = 512,
//	DBG_EnableCCD = 1024,
//	DBG_DrawConstraints = (1 << 11),
//	DBG_DrawConstraintLimits = (1 << 12),
//	DBG_FastWireframe = (1 << 13),
//	DBG_DrawNormals = (1 << 14),
//	DBG_DrawFrames = (1 << 15),
//	DBG_MAX_DEBUG_DRAW_MODE
//};
int FPhysicsDebugDrawer::getDebugMode() const
{
	return DBG_DrawWireframe;
}

void FPhysicsDebugDrawer::drawContactPoint(const btVector3 & PointOnB, const btVector3 & normalOnB, btScalar distance, int lifeTime, const btVector3 & color)
{
}

void FPhysicsDebugDrawer::DrawTriangle(const FVector3 & aV1, const FVector3 & aV2, const FVector3 & aV3, const FVector3 & aColor)
{
	myDebugDrawer->DrawTriangle(aV1, aV2, aV3, aColor);
}
