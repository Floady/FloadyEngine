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
	void GetTransform(float* aMatrix);
	void SetPos(const FVector3& aPos);
protected:
	btRigidBody* myPhysicsObject;
};

