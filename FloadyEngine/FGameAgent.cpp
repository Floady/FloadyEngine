#include "FGameAgent.h"
#include "FBulletPhysics.h"
#include "FRenderableObject.h"


FGameAgent::FGameAgent()
	: FGameEntity(FVector3(0,0,0), FVector3(1,1,1), FGameEntity::ModelType::Sphere, 10.0f, false)
{
	myPathFindingComponent = new FPathfindComponent(this);
	myWasPathing = false;
}


FGameAgent::~FGameAgent()
{
}

void FGameAgent::Update(double aDeltaTime)
{
	FGameEntity::Update(aDeltaTime);

	if (myPathFindingComponent)
		myPathFindingComponent->Update(aDeltaTime);
}

void FGameAgent::PostPhysicsUpdate()
{
	const btVector3& linVel = myPhysicsObject->getLinearVelocity(); // add or reset
	if (myPathFindingComponent)
	{
		FVector3 dir = myPathFindingComponent->GetDirection();
		if (dir.Length() > 0)
		{
			myWasPathing = true;
			myPos = myPathFindingComponent->GetPosOnPath();
			myPhysicsObject->activate(true);
			myPhysicsObject->setLinearVelocity(btVector3(dir.x, dir.y, dir.z));
		}
		else if (myWasPathing)
		{
			myPhysicsObject->setLinearVelocity(btVector3(0, 0, 0));
			myWasPathing = false;
		}
	}

	FGameEntity::PostPhysicsUpdate();
}
