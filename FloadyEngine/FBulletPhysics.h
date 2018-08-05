#pragma once
#include "LinearMath/btAlignedObjectArray.h"
#include <vector>
#include "FVector3.h"

class btDiscreteDynamicsWorld;
class btBroadphaseInterface;
class btCollisionDispatcher;
class btConstraintSolver;
class btDefaultCollisionConfiguration;
class btCollisionShape;
class btRigidBody;
class FDebugDrawer;
class FD3d12Renderer;
class FGameEntity;
class FPhysicsDebugDrawer;

class FBulletPhysics
{
public:

	struct RayCastHit
	{
		FVector3 myPos;
		FVector3 myNormal;
	};

	struct AABB
	{
		FVector3 myMin;
		FVector3 myMax;
	};

	enum class CollisionPrimitiveType : char
	{
		Default = 0,
		Box = Default,
		Capsule,
		Sphere
	};

	FBulletPhysics();
	~FBulletPhysics();
	void Init(FD3d12Renderer* aRendererForDebug);
	void Update(double aDeltaTime);
	void SetDebugDrawEnabled(bool anEnabled);
	btRigidBody* AddObject(float aMass, FVector3 aPos, FVector3 aScale, CollisionPrimitiveType aPrim = CollisionPrimitiveType::Default, bool aShouldBlockNav = false, FGameEntity* anEntity = nullptr);
	void AddTerrain(btRigidBody* aBody, btCollisionShape* aCollisionShape, FGameEntity* anOwner);
	void RemoveObject(btRigidBody* aBody);
	FPhysicsDebugDrawer* GetDebugDrawer() { return myDebugDrawer; }
	FGameEntity* GetFirstEntityHit(FVector3 aStart, FVector3 anEnd);
	void SetPaused(bool aPause);
	void TogglePaused() { myEnabled = !myEnabled; }
	bool RayCast(FVector3 aStart, FVector3 anEnd, RayCastHit& outHitResult);
	bool HasNewNavBlockers() const { return myHasNewNavBlockers; }
	void ResetHasNewNavBlockers() { myHasNewNavBlockers = false; }

	std::vector<FBulletPhysics::AABB> GetAABBs();
private:
	btBroadphaseInterface*	m_broadphase;
	btCollisionDispatcher*	m_dispatcher;
	btConstraintSolver*	m_solver;
	btDefaultCollisionConfiguration* m_collisionConfiguration;
	btDiscreteDynamicsWorld* m_dynamicsWorld;
	FPhysicsDebugDrawer* myDebugDrawer;
	btAlignedObjectArray<btCollisionShape*>	m_collisionShapes;
	bool myEnabled;
	struct FPhysicsBody
	{
		btRigidBody* myRigidBody;
		bool myShouldBlockNavMesh; // should this be generic flags?
		FGameEntity* myGameEntity;
		btCollisionShape* myCollisionEntity;
	};
	btAlignedObjectArray<FPhysicsBody>	myRigidBodies;
	bool myDebugDrawEnabled;
	bool myHasNewNavBlockers;
};

