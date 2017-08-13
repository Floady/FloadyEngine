#pragma once
#include "..\FJson\FJsonObject.h"
#include "FPathfindComponent.h"
#include "FGameEntityFactory.h"

class FRenderableObject;
class btRigidBody;
class FPathfindComponent;

class FGameEntity
{
public:
	REGISTER_GAMEENTITY(FGameEntity);

	enum class ModelType : char
	{
		Sphere = 0,
		Box = 1
	};

	FGameEntity() { myPos = FVector3(0, 0, 0); myPhysicsObject = nullptr;  myGraphicsObject = nullptr; myPhysicsObject = nullptr; myOwner = nullptr; }
	FGameEntity(FVector3 aPos, FVector3 aScale, ModelType aType, float aMass = 0.0f, bool aIsNavBlocker = false);
	virtual void Init(const FJsonObject& anObj);
	virtual void Update(double aDeltaTime);
	virtual void PostPhysicsUpdate();
	void SetPhysicsActive(bool anIsActive) { myIsPhysicsActive = anIsActive; }
	FVector3 GetPos() const { return myPos; }
	void SetOwnerEntity(FGameEntity* anEntity) { myOwner = anEntity; }
	FGameEntity* GetOwnerEntity() { return myOwner ? myOwner->GetOwnerEntity() : this; }
	virtual void SetPos(FVector3 aPos);

	virtual FRenderableObject* GetRenderableObject() { return myGraphicsObject; }

	virtual ~FGameEntity();

protected:
	void Init(FVector3 aPos, FVector3 aScale, ModelType aType, float aMass = 0.0f, bool aIsNavBlocker = false);
	
	FRenderableObject* myGraphicsObject;
	btRigidBody* myPhysicsObject;
	bool myIsPhysicsActive;
	FVector3 myPos;
	FGameEntity* myOwner; // if !null, this entity is managed by someone else (for entity picker callbacks etc)
};

