#include "FPhysicsObject.h"
#include "BulletDynamics\Dynamics\btRigidBody.h"



FPhysicsObject::FPhysicsObject(btRigidBody* aRigidBody)
	: myRigidBody(aRigidBody)
{
}


FPhysicsObject::~FPhysicsObject()
{
}

FVector3 FPhysicsObject::GetPos() const
{
	btVector3 boxPhysPos = myRigidBody->getWorldTransform().getOrigin();
	float m[16];
	myRigidBody->getWorldTransform().getOpenGLMatrix(m);

	// kill translation - we store it seperately
	m[12] = 0;
	m[13] = 0;
	m[14] = 0;

	//myGraphicsObject->SetRotMatrix(m);
	return FVector3(boxPhysPos.getX(), boxPhysPos.getY(), boxPhysPos.getZ());
}

void FPhysicsObject::GetTransform(float * anOutMatrix) const
{
	btQuaternion rot = myRigidBody->getWorldTransform().getRotation();
	btMatrix3x3 mat(rot);
	mat = mat.inverse();
	int idx = 0;
	for (int i = 0; i < 3; i++)
	{
		btVector3 row = mat.getRow(i);
		anOutMatrix[idx + 0] = row.x();
		anOutMatrix[idx + 1] = row.y();
		anOutMatrix[idx + 2] = row.z();
		anOutMatrix[idx + 3] = 0.0f;

		idx += 4;
	}
	anOutMatrix[idx + 0] = 0.0f;
	anOutMatrix[idx + 1] = 0.0f;
	anOutMatrix[idx + 2] = 0.0f;
	anOutMatrix[idx + 3] = 1.0f;
}

void FPhysicsObject::SetYaw(float aYaw)
{
	btQuaternion rot(btVector3(0, 1, 0), aYaw);
	myRigidBody->getWorldTransform().setRotation(rot);
}

void FPhysicsObject::Yaw(float aYaw)
{
	btQuaternion rot(btVector3(0, 1, 0), aYaw);
	myRigidBody->getWorldTransform().setRotation(myRigidBody->getWorldTransform().getRotation() * rot);
}

void FPhysicsObject::SetRoll(float aRoll)
{
	btQuaternion rot(btVector3(1, 0, 0), aRoll);
	myRigidBody->getWorldTransform().setRotation(rot);
}

void FPhysicsObject::Roll(float aRoll)
{
	btQuaternion rot(btVector3(1, 0, 0), aRoll);
	myRigidBody->getWorldTransform().setRotation(myRigidBody->getWorldTransform().getRotation() * rot);
}

void FPhysicsObject::SetPos(const FVector3 & aPos)
{
	myRigidBody->getWorldTransform().setOrigin(btVector3(aPos.x, aPos.y, aPos.z));
}
