#include "FRenderableObject.h"
#include "FGameAgent.h"
#include "FPathfindComponent.h"
#include "FPhysicsComponent.h"
#include "FRenderMeshComponent.h"


FGameAgent::FGameAgent()
	: FGameEntity(FVector3(0, 0, 0))
{
	myWasPathing = false;
}

FGameAgent::FGameAgent(const FVector3& aPos)
	: FGameEntity(aPos)
{
	myWasPathing = false;
}


FGameAgent::~FGameAgent()
{
}

void FGameAgent::Init(const FJsonObject & anObj)
{
	FGameEntity::Init(anObj);
}

void FGameAgent::Update(double aDeltaTime)
{
	FGameEntity::Update(aDeltaTime);
}

void FGameAgent::PostPhysicsUpdate()
{
	if(GetComponentInSlot<FPathfindComponent>(0)->HasPath())
	{
		myPos = GetComponentInSlot<FPathfindComponent>(0)->GetPosOnPath();
		GetComponentInSlot<FPhysicsComponent>(0)->SetPos(myPos);
	}

	FGameEntity::PostPhysicsUpdate();
}

FPathfindComponent * FGameAgent::GetPathFindingComponent()
{
	return GetComponentInSlot<FPathfindComponent>(0);
}
