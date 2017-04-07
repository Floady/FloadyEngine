#pragma once
#include "..\FJson\FJsonObject.h"

class FRenderableObject;
class btRigidBody;

class FGameEntity
{
public:
	enum class ModelType : char
	{
		Sphere = 0,
		Box = 1
	};

	FGameEntity(FVector3 aPos, FVector3 aScale, ModelType aType, float aMass = 0.0f, bool aIsNavBlocker = false);
	FGameEntity(const FJsonObject& anObj);
	void Update();
	void PostPhysicsUpdate();

	~FGameEntity();

private:
	void Init(FVector3 aPos, FVector3 aScale, ModelType aType, float aMass = 0.0f, bool aIsNavBlocker = false);
	
	FRenderableObject* myGraphicsObject;
	btRigidBody* myPhysicsObject;
};

