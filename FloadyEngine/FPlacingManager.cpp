#include "FRenderableObject.h"
#include "FPlacingManager.h"
#include "FD3d12Renderer.h"
#include "d3dx12.h"
#include "FCamera.h"
#include "FGameEntity.h"
#include "FPrimitiveGeometry.h"
#include "FPostProcessEffect.h"
#include "FPrimitiveBoxColorOverride.h"
#include "FNavMeshManagerRecast.h"
#include "FUtilities.h"
#include "FSceneGraph.h"

using namespace DirectX;

FPlacingManager::FPlacingManager()
{
	myObject = nullptr;
	myFitsOnNavMesh = false;
}

void FPlacingManager::UpdateMousePos(const FVector3& aPos)
{
	float errorMargin = 0.5f;

	if (myObject)
	{
		// check if aabb fits on navmesh
		// todo: should do isinside check instead ( now you can overlap if aabb is bigger than blocking piece)
		// todo: snap to navmesh edges so it always fits
		myFitsOnNavMesh = true;
		FVector3 newPos = aPos;
		FVector3 delta = FVector3(myObject->GetLocalAABB().myMax.x, 0, myObject->GetLocalAABB().myMax.z);
		FVector3 point = aPos + delta;
		FVector3 point2 = FNavMeshManagerRecast::GetInstance()->GetClosestPointOnNavMesh(point);
		if ((point - point2).SqrLength() > errorMargin)
			myFitsOnNavMesh = false;

		delta = FVector3(myObject->GetLocalAABB().myMin.x, 0, myObject->GetLocalAABB().myMin.z);
		point = newPos + delta;
		point2 = FNavMeshManagerRecast::GetInstance()->GetClosestPointOnNavMesh(point);
		if ((point - point2).SqrLength() > errorMargin)
			myFitsOnNavMesh = false;

		delta = FVector3(myObject->GetLocalAABB().myMin.x, 0, myObject->GetLocalAABB().myMax.z);
		point = newPos + delta;
		point2 = FNavMeshManagerRecast::GetInstance()->GetClosestPointOnNavMesh(point);
		if ((point - point2).SqrLength() > errorMargin)
			myFitsOnNavMesh = false;

		delta = FVector3(myObject->GetLocalAABB().myMax.x, 0, myObject->GetLocalAABB().myMin.z);
		point = newPos + delta;
		point2 = FNavMeshManagerRecast::GetInstance()->GetClosestPointOnNavMesh(point);
		if ((point - point2).SqrLength() > errorMargin)
			myFitsOnNavMesh = false;

		//
		if (myFitsOnNavMesh)
			myObject->SetColor(FVector3(0, 1, 0));
		else
			myObject->SetColor(FVector3(1, 0, 0));
		
		newPos.y += myObject->GetLocalAABB().myMax.y / 2.0f;
		myObject->SetPos(newPos);
		myObject->RecalcModelMatrix();
	}
}

void FPlacingManager::MouseDown(const FVector3& aPos)
{
	if (!myObject)
		return;

	if (!myFitsOnNavMesh)
		return;

	if (myPlaceCallback)
	{
		FVector3 pos = aPos;
		pos.y += myObject->GetLocalAABB().myMax.y / 2.0f;
		myPlaceCallback(pos);
	}

	ClearPlacable();
}

void FPlacingManager::SetPlacable(bool anIsCube, FVector3 aScale, FDelegate2<void(FVector3)>& aCB, const char* aTex)
{
	ClearPlacable();

	if(anIsCube)
		myObject = new FPrimitiveBoxColorOverride(FD3d12Renderer::GetInstance(), FVector3(0, 0, 0), aScale, FPrimitiveBoxInstanced::PrimitiveType::Box);
	else
		myObject = new FPrimitiveBoxColorOverride(FD3d12Renderer::GetInstance(), FVector3(0, 0, 0), aScale, FPrimitiveBoxInstanced::PrimitiveType::Sphere);

	myObject->SetShader("placeable_deferred.hlsl");
	if(aTex)
		myObject->SetTexture(aTex);
	FD3d12Renderer::GetInstance()->GetSceneGraph().AddObject(myObject, false);

	myPlaceCallback = aCB;
}

void FPlacingManager::ClearPlacable()
{
	if (myObject)
	{
		FD3d12Renderer::GetInstance()->GetSceneGraph().RemoveObject(myObject);
		delete myObject;
	}

	myObject = nullptr;
	myPlaceCallback.reset();
}

FPlacingManager::~FPlacingManager()
{
}
