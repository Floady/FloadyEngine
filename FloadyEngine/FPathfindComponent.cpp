#include "FPathfindComponent.h"
#include "FNavMeshManager.h"
#include "FGameEntity.h"
#include "FNavMeshManagerRecast.h"
#include "FD3d12Renderer.h"
#include "FDebugDrawer.h"

REGISTER_GAMEENTITYCOMPONENT2(FPathfindComponent);

#pragma optimize("", off)

FPathfindComponent::FPathfindComponent()
{
	mySpeed = 1.0f;
	myCurTargetIdx = -1;
	myDirection = FVector3(1, 0, 0);
	myDesiredDirection = FVector3(1, 0, 0);
	myMaxTurnSpeed = 1.0f;
}

void FPathfindComponent::FindPath(FVector3 aStart, FVector3 anEnd)
{
	// old 2d pathfinder
	// myPath = FNavMeshManager::GetInstance()->FindPath(aStart, anEnd); 

	myPath = FNavMeshManagerRecast::GetInstance()->FindPath(aStart, anEnd);
	myPathLength = 0.0f;
	myCurPosOnPath = 0.0f;
	myCurTargetIdx = -1;

	if (myPath.size() < 2)
		return;

	myCurTargetIdx = 0;

	for (int i = 0; i < myPath.size() - 1; i++)
	{
		myPathLength += (myPath[i + 1] - myPath[i]).Length();
	}
}

FPathfindComponent::~FPathfindComponent()
{
}

void FPathfindComponent::Init(const FJsonObject & anObj)
{
	if(anObj.HasItem("speed"))
		mySpeed = anObj.GetItem("speed").GetAs<double>();
}

void FPathfindComponent::Update(double aDeltaTime)
{
	// adjust speed based on rotation
	float dotDirection = myDesiredDirection.Dot(myDirection);
	float angleFromDesiredDirection = dotDirection >= 1.0f ? 0.0f : acos(dotDirection);
	float speed = mySpeed * ((PI - angleFromDesiredDirection) / (PI)); // 180 turn = 0 speed, if we are heading towards direction, its max speed
	
	myCurPosOnPath += aDeltaTime * speed;

	if(myCurTargetIdx >= 0 && myCurTargetIdx < myPath.size())
	{
		FVector3 pos = myOwner->GetPos();
		FVector3 nextPathPoint = myPath[myCurTargetIdx];
		nextPathPoint.y = 0;
		pos.y = 0;
		if ((pos - nextPathPoint).Length() < 1.0f)
		{
			myCurTargetIdx++;
		}
	}

	// Debug draw path
	if (false)
	{
		for (int i = 1; i < myPath.size(); i++)
		{
			float shade = 0.5f + (i * (0.5f / myPath.size()));
			FVector3 from = myPath[i - 1];
			FVector3 to = myPath[i];
			FD3d12Renderer::GetInstance()->GetDebugDrawer()->drawLine(from, to, FVector3(shade, shade, shade));
		}

		FVector3 offset = FVector3(0, 0.3f, 0);
		FD3d12Renderer::GetInstance()->GetDebugDrawer()->drawLine(GetPosOnPath() + offset, GetPosOnPath() + offset + myDirection, FVector3(1.0f, 0, 0));
		offset = FVector3(0, 0.33f, 0);
		FD3d12Renderer::GetInstance()->GetDebugDrawer()->drawLine(GetPosOnPath() + offset, GetPosOnPath() + offset + myDesiredDirection, FVector3(0, 1.0f, 0));
	}

	
	// update direction if on path
	if (myPath.size() > 1)
	{
		// we arrived, return direction of last point to finish - todo: later other behaviors can override this
		if (myCurTargetIdx == myPath.size())
			myDesiredDirection = (myPath[myCurTargetIdx - 1] - myPath[myCurTargetIdx - 2]).Normalized();
		else if (myCurTargetIdx > 0)
			myDesiredDirection = (myPath[myCurTargetIdx] - myPath[myCurTargetIdx - 1]).Normalized();
		else
			myDesiredDirection = (myPath[1] - myPath[0]).Normalized();
	}

	// spring direction
	bool isCCW = myDesiredDirection.Cross(myDirection).y > 0;
	const float turnSpeed = 2.0f;
	float newAngle = angleFromDesiredDirection - aDeltaTime * turnSpeed;
	if(newAngle > 0.01)
	{
		btVector3 a =  btVector3(myDesiredDirection.x, myDesiredDirection.y, myDesiredDirection.z);
		a = a.rotate(btVector3(0,1,0), !isCCW ? 2 * PI - newAngle : newAngle); // rotate over up
		myDirection.x = a.getX();
		myDirection.y = a.getY();
		myDirection.z = a.getZ();
	}
}

void FPathfindComponent::PostPhysicsUpdate()
{
}

FVector3 FPathfindComponent::GetPosOnPath()
{
	if (myPath.size() < 2)
		return FVector3();
	
	float pathTraversed = 0.0f;
	for (int i = 0; i < myPath.size() - 1; i++)
	{
		float segmentLength = (myPath[i + 1] - myPath[i]).Length();
		pathTraversed += (myPath[i + 1] - myPath[i]).Length();
		if (pathTraversed > myCurPosOnPath)
		{
			float remainder = segmentLength - (pathTraversed - myCurPosOnPath);
			return myPath[i] + (myPath[i+1] - myPath[i]).Normalized() * remainder;
		}
	}

	return myPath[myPath.size()-1];
}

const FVector3& FPathfindComponent::GetDirection()
{
	return myDirection;
}

const FVector3& FPathfindComponent::GetDesiredDirection()
{
	return myDesiredDirection;
}
