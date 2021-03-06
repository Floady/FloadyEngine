#include "FRenderableObject.h"

#include "FDebugDrawer.h"
#include "FD3d12Renderer.h"

FAABB FRenderableObject::GetAABB() const
{
	FAABB aabb = myAABB;
	aabb.myMax += myPos;
	aabb.myMin += myPos;
	return aabb;
}

FAABB FRenderableObject::GetLocalAABB() const
{
	FAABB aabb = myAABB;
	return aabb;
}

FRenderableObject::FRenderableObject()
{
	m_ModelProjMatrix = nullptr;
	myIsVisible = true;
	myCastsShadows = true;
	myRenderCCW = false;
	myScale = FVector3(1,1,1);
	myPos = FVector3(0,0,0);
}


FRenderableObject::~FRenderableObject()
{
	if (m_ModelProjMatrix)
		m_ModelProjMatrix->Release();
}

bool FAABB::IsInside(const FVector3& aPoint) const
{
	if (aPoint.x > myMin.x && aPoint.x < myMax.x && aPoint.y > myMin.y && aPoint.y < myMax.y && aPoint.z > myMin.z && aPoint.z < myMax.z)
		return true;

	return false;
}

bool FAABB::IsInside(const FAABB& anAABB) const
{
	bool isInside = false;
	isInside |= IsInside(anAABB.myMin);
	isInside |= IsInside(anAABB.myMax);
	return isInside;
}

FRenderableObjectInstanceData::FRenderableObjectInstanceData()
{
	memset(myModelMatrix, 0, sizeof(float) * 16);
	memset(myRotMatrix, 0, sizeof(float) * 16);
	FVector3 myPos = FVector3(0, 0, 0);
	myScale = FVector3(1,1,1);
	myAABB = FAABB();
	myAABB.Grow(-myScale);
	myAABB.Grow(myScale);
	myIsVisible = false;;
}
