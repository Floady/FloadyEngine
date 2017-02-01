#pragma once
#include "FVector3.h"
#include "LinearMath/btAlignedObjectArray.h"

class btDiscreteDynamicsWorld;
class btBroadphaseInterface;
class btCollisionDispatcher;
class btConstraintSolver;
class btDefaultCollisionConfiguration;
class btCollisionShape;
class btRigidBody;

class FBulletPhysics
{
public:
	enum class CollisionPrimitiveType : char
	{
		Default = 0,
		Box = Default,
		Capsule
	};

	FBulletPhysics();
	~FBulletPhysics();
	void Init();
	void Update(double aDeltaTime);
	btRigidBody* AddObject(float aMass, FVector3 aPos, FVector3 aScale, CollisionPrimitiveType aPrim = CollisionPrimitiveType::Default);
private:
	btBroadphaseInterface*	m_broadphase;
	btCollisionDispatcher*	m_dispatcher;
	btConstraintSolver*	m_solver;
	btDefaultCollisionConfiguration* m_collisionConfiguration;
	btDiscreteDynamicsWorld* m_dynamicsWorld;
	btAlignedObjectArray<btCollisionShape*>	m_collisionShapes;
};

