#pragma once

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

	FGameEntity(FVector3 aPos, FVector3 aScale, ModelType aType, float aMass = 0.0f);
	void Update();
	void PostPhysicsUpdate();

	~FGameEntity();
private:
	FRenderableObject* myGraphicsObject;
	btRigidBody* myPhysicsObject;
};

