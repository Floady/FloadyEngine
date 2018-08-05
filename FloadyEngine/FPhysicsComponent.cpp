#include "FPhysicsComponent.h"
#include "FGame.h"
#include "FBulletPhysics.h"
#include "BulletDynamics\Dynamics\btRigidBody.h"


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
	FBulletPhysics::CollisionPrimitiveType primType = type ? FBulletPhysics::CollisionPrimitiveType::Sphere : FBulletPhysics::CollisionPrimitiveType::Box;

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
	btVector3 boxPhysPos = myPhysicsObject->getWorldTransform().getOrigin();
	float m[16];
	myPhysicsObject->getWorldTransform().getOpenGLMatrix(m);

	// kill translation - we store it seperately
	m[12] = 0;
	m[13] = 0;
	m[14] = 0;

	//myGraphicsObject->SetRotMatrix(m);
	return FVector3(boxPhysPos.getX(), boxPhysPos.getY(), boxPhysPos.getZ());
}

void FPhysicsComponent::GetTransform(float* aMatrix)
{
	btQuaternion rot = myPhysicsObject->getWorldTransform().getRotation();
	btMatrix3x3 mat(rot);
	mat = mat.inverse();
	int idx = 0;
	for (int i = 0; i < 3; i++)
	{
		btVector3 row = mat.getRow(i);
		aMatrix[idx + 0] = row.x();
		aMatrix[idx + 1] = row.y();
		aMatrix[idx + 2] = row.z();
		aMatrix[idx + 3] = 0.0f;
		
		idx += 4;
	}
	aMatrix[idx + 0] = 0.0f;
	aMatrix[idx + 1] = 0.0f;
	aMatrix[idx + 2] = 0.0f;
	aMatrix[idx + 3] = 1.0f;
}

void FPhysicsComponent::SetPos(const FVector3 & aPos)
{
	btTransform groundTransform;
	groundTransform.setIdentity();
	groundTransform.setOrigin(btVector3(aPos.x + myOffset.x, aPos.y + myOffset.y, aPos.z + myOffset.z));

	if (myPhysicsObject)
		myPhysicsObject->setWorldTransform(groundTransform);
}
