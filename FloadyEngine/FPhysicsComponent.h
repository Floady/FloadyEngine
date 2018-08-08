#pragma once
#include "FGameEntityComponent.h"
#include "FVector3.h"

class btRigidBody;

class FPhysicsComponent : public FGameEntityComponent
{
public:
	REGISTER_GAMEENTITYCOMPONENT(FPhysicsComponent);

	FPhysicsComponent();
	~FPhysicsComponent();
	virtual void Init(const FJsonObject& anObj);
	virtual void Update(double aDeltaTime);
	virtual void PostPhysicsUpdate();
	FVector3 GetPos();
	FVector3 GetScale() { return myScale; };
	void GetTransform(float* aMatrix);
	void SetYaw(float aYaw);
	void Yaw(float aYaw);
	void SetRoll(float aRoll);
	void Roll(float aRoll);
	void SetPos(const FVector3& aPos);
protected:
	btRigidBody* myPhysicsObject;
	FVector3 myScale;
	FVector3 myOffset;
};

