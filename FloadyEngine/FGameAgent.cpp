#include "FRenderableObject.h"
#include "FGameAgent.h"
#include "FPathfindComponent.h"
#include "FPhysicsComponent.h"
#include "FRenderMeshComponent.h"
#include "FUtilities.h"
#include "FD3d12Renderer.h"
#include "FDebugDrawer.h"

FGameAgent::FGameAgent()
	: FGameEntity(FVector3(0, 0, 0))
	, myRoll(0.0f, 4.0f)
{
	myWasPathing = false;
	myBobTimer = 0.0f;
	myBobSpeed = 0.5f;
}

FGameAgent::FGameAgent(const FVector3& aPos)
	: FGameEntity(aPos)
	, myRoll(0.0f, 4.0f)
{
	myWasPathing = false;
	myBobTimer = 0.0f;
	myBobSpeed = 0.5f;
}


FGameAgent::~FGameAgent()
{
}

void FGameAgent::Init(const FJsonObject & anObj)
{
	FGameEntity::Init(anObj);
	myAgentPos = myPos;
}

void FGameAgent::Update(double aDeltaTime)
{
	FGameEntity::Update(aDeltaTime);

	myBobTimer += myBobSpeed * aDeltaTime;
	if (myBobTimer > 0.3f)
		myBobSpeed = -0.75f;
	if (myBobTimer < -0.3f)
		myBobSpeed = 0.75f;

	myRoll.Update(aDeltaTime);
}

void FGameAgent::PostPhysicsUpdate()
{
	FVector3 newPos = myAgentPos;
	float yaw = 0.0f;
	if (GetComponentInSlot<FPathfindComponent>(0)->HasPath())
	{
		// snap position to path pos (if there is one) - todo: if we want things to be able to be knocked off course, dont do this - make steering behavior
		myPos = GetComponentInSlot<FPathfindComponent>(0)->GetPosOnPath();
		myAgentPos = myPos;

		GetComponentInSlot<FPhysicsComponent>(0)->SetPos(newPos);
		
		// set yaw to fit direction on path
		FVector3 dir = GetComponentInSlot<FPathfindComponent>(0)->GetDirection();
		float dot = dir.Dot(FVector3(1, 0, 0));
		bool isOnLeft = dir.Dot(FVector3(0,0,-1)) < 0.0f;
		yaw = isOnLeft ? -acos(dot) : acos(dot);
		
		// reset bob timer while following path so we smooth into it
		if (!GetComponentInSlot<FPathfindComponent>(0)->HasFinishedPath())
			myBobTimer = 0;
	}

	if (!GetComponentInSlot<FPathfindComponent>(0)->HasPath() || (GetComponentInSlot<FPathfindComponent>(0)->HasFinishedPath()))
	{
		// bobb
		newPos += FVector3(0, myBobTimer, 0); 
	}

	// apply Y offset
	newPos += FVector3(0, 4.0f, 0);

	GetComponentInSlot<FPhysicsComponent>(0)->SetPos(newPos);
	GetComponentInSlot<FPhysicsComponent>(0)->SetYaw(yaw);

	// set roll depending on angle from desiredDir, lets say 90 deg is max roll
	const FVector3& desiredDir = GetComponentInSlot<FPathfindComponent>(0)->GetDesiredDirection();
	const FVector3& dir = GetComponentInSlot<FPathfindComponent>(0)->GetDirection();
	float dot = dir.Dot(desiredDir);
	float angle = dot >= 1.0f ? 0.0f : acos(dir.Dot(desiredDir));
	
	const float maxAngle = PI / 2;
	float bankPercent = min(1.0f, angle / maxAngle);
	bool isCCW = desiredDir.Cross(dir).y > 0;
	myRoll.SetDesiredValue(bankPercent);
	GetComponentInSlot<FPhysicsComponent>(0)->Roll(isCCW ? myRoll.GetValue() : -myRoll.GetValue());

	FGameEntity::PostPhysicsUpdate();
}

void FGameAgent::SetPos(const FVector3 & aPos)
{
	myAgentPos = aPos;
	FGameEntity::SetPos(aPos);
}

FPathfindComponent * FGameAgent::GetPathFindingComponent()
{
	return GetComponentInSlot<FPathfindComponent>(0);
}
