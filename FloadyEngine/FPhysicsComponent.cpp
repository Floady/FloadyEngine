#include "FPhysicsComponent.h"
#include "FGame.h"
#include "BulletDynamics\Dynamics\btRigidBody.h"
#include "FUtilities.h"
#include "FPhysicsWorld.h"
#include "FPhysicsObject.h"


REGISTER_GAMEENTITYCOMPONENT2(FPhysicsComponent);

FPhysicsComponent::FPhysicsComponent(){
	myPhysicsObject = nullptr;
}


FPhysicsComponent::~FPhysicsComponent()
{
	if (myPhysicsObject)
	{
		FGame::GetInstance()->GetPhysics()->RemoveObject(myPhysicsObject);
		myPhysicsObject = nullptr;
	}
}

void FPhysicsComponent::Init(const FJsonObject & anObj)
{
	FVector3 pos;
	float mass;

	pos.x = anObj.GetItem("posX").GetAs<double>();
	pos.y = anObj.GetItem("posY").GetAs<double>();
	pos.z = anObj.GetItem("posZ").GetAs<double>();

	myScale.x = anObj.GetItem("scaleX").GetAs<double>();
	myScale.y = anObj.GetItem("scaleY").GetAs<double>();
	myScale.z = anObj.GetItem("scaleZ").GetAs<double>();

	if (anObj.HasItem("offsetX"))
	{
		myOffset.x = anObj.GetItem("offsetX").GetAs<double>();
		myOffset.y = anObj.GetItem("offsetY").GetAs<double>();
		myOffset.z = anObj.GetItem("offsetZ").GetAs<double>();
	}

	mass = anObj.GetItem("mass").GetAs<double>();

	bool isNavBlocking = anObj.GetItem("isNavBlocker").GetAs<bool>();
	bool canMove = anObj.GetItem("canMove").GetAs<bool>();

	int type = anObj.GetItem("type").GetAs<int>();
	FPhysicsWorld::CollisionPrimitiveType primType = type ? FPhysicsWorld::CollisionPrimitiveType::Sphere : FPhysicsWorld::CollisionPrimitiveType::Box;

	myPhysicsObject = FGame::GetInstance()->GetPhysics()->AddObject(mass, pos + myOffset, myScale, primType, isNavBlocking, myOwner);
}

void FPhysicsComponent::Update(double aDeltaTime)
{
}

void FPhysicsComponent::PostPhysicsUpdate()
{
}

FVector3 FPhysicsComponent::GetPos()
{
	return myPhysicsObject->GetPos();
}

void FPhysicsComponent::GetTransform(float* aMatrix)
{
	myPhysicsObject->GetTransform(aMatrix);
}

void FPhysicsComponent::SetYaw(float aYaw)
{
	myPhysicsObject->SetYaw(aYaw);
}

void FPhysicsComponent::Yaw(float aYaw)
{
	myPhysicsObject->Yaw(aYaw);
}

void FPhysicsComponent::SetRoll(float aRoll)
{
	myPhysicsObject->SetRoll(aRoll);
}

void FPhysicsComponent::Roll(float aRoll)
{
	myPhysicsObject->Roll(aRoll);
}

void FPhysicsComponent::SetPos(const FVector3 & aPos)
{
	if (myPhysicsObject)
		myPhysicsObject->SetPos(myOffset + aPos);
}
