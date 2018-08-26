#pragma once

#include "FVector3.h"

class btRigidBody;

class FPhysicsObject
{
public:
	FPhysicsObject(btRigidBody* aRigidBody);
	~FPhysicsObject();

	FVector3 GetPos() const;
	void GetTransform(float* anOutMatrix) const;
	void SetYaw(float aYaw);
	void Yaw(float aYaw);
	void SetRoll(float aRoll);
	void Roll(float aRoll);
	void SetPos(const FVector3& aPos);

	btRigidBody* GetRigidBody() { return myRigidBody; }

private:
	btRigidBody* myRigidBody;
};

