#pragma once
#include "..\FJson\FJsonObject.h"
#include "FGameEntityFactory.h"

class FGameEntity;

class FGameEntityComponent
{
public:

	FGameEntityComponent();
	void SetOwner(FGameEntity* anEntity) { myOwner = anEntity; }
	virtual ~FGameEntityComponent();
	virtual void Init(const FJsonObject& anObj) = 0;
	virtual void Update(double aDeltaTime) = 0;
	virtual void PostPhysicsUpdate() = 0;

protected:
	FGameEntity* myOwner; // if !null, this entity is managed by someone else (for entity picker callbacks etc)
};
