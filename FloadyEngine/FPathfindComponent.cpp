#include "FPathfindComponent.h"
#include "FNavMeshManager.h"
#include "FGameEntity.h"

FPathfindComponent::FPathfindComponent(FGameEntity* aGameEntity)
{
	myGameEntity = aGameEntity;
	mySpeed = 1.0f;
	myCurTargetIdx = -1;
}

void FPathfindComponent::FindPath(FVector3 aStart, FVector3 anEnd)
{
	myPath = FNavMeshManager::GetInstance()->FindPath(aStart, anEnd);
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

void FPathfindComponent::Update(double aDeltaTime)
{
	myCurPosOnPath += aDeltaTime * mySpeed;

	if(myCurTargetIdx >= 0 && myCurTargetIdx < myPath.size())
	{
		if ((myGameEntity->GetPos() - myPath[myCurTargetIdx]).Length() < 0.5f)
		{
			myCurTargetIdx++;
		}
	}
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

FVector3 FPathfindComponent::GetDirection()
{
	if (myPath.size() <= 1)
		return FVector3();

	if (myCurTargetIdx < 0 || myCurTargetIdx > myPath.size() - 1 )
		return FVector3();

	return (myPath[myCurTargetIdx] - myGameEntity->GetPos()).Normalized();

	return FVector3();
}