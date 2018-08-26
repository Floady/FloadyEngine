#pragma once
#include <vector>
#include "FVector3.h"

class btDiscreteDynamicsWorld;
class btBroadphaseInterface;
class btCollisionDispatcher;
class btConstraintSolver;
class btDefaultCollisionConfiguration;
class btCollisionShape;
class FPhysicsObject;
class FIPhysicsDebugDrawer;
class FPhysicsDebugDrawer;
class btRigidBody;

template <class T>
class btAlignedObjectArray;

class FPhysicsWorld
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

	FPhysicsWorld();
	~FPhysicsWorld();
	void Init(FIPhysicsDebugDrawer* aRendererForDebug = nullptr);
	void Update(double aDeltaTime);
	void SetDebugDrawEnabled(bool anEnabled);
	FPhysicsObject* AddObject(float aMass, FVector3 aPos, FVector3 aScale, CollisionPrimitiveType aPrim = CollisionPrimitiveType::Default, bool aShouldBlockNav = false, void* anEntity = nullptr);
	void AddTerrain(const std::vector<FVector3>& aTriangleList, void* anOwner);
	void RemoveObject(FPhysicsObject* aBody);
	FPhysicsDebugDrawer* GetDebugDrawer() { return myDebugDrawer; }
	void* GetFirstEntityHit(FVector3 aStart, FVector3 anEnd);
	void SetPaused(bool aPause);
	void TogglePaused() { myEnabled = !myEnabled; }
	bool RayCast(FVector3 aStart, FVector3 anEnd, RayCastHit& outHitResult);
	bool HasNewNavBlockers() const { return myHasNewNavBlockers; }
	void ResetHasNewNavBlockers() { myHasNewNavBlockers = false; }

	std::vector<FPhysicsWorld::AABB> GetAABBs();
private:
	btBroadphaseInterface*	m_broadphase;
	btCollisionDispatcher*	m_dispatcher;
	btConstraintSolver*	m_solver;
	btDefaultCollisionConfiguration* m_collisionConfiguration;
	btDiscreteDynamicsWorld* m_dynamicsWorld;
	FPhysicsDebugDrawer* myDebugDrawer;
	btAlignedObjectArray<btCollisionShape*>*	m_collisionShapes;
	bool myEnabled;
	struct FPhysicsBody
	{
		FPhysicsObject* myRigidBody; // mem leaks?
		bool myShouldBlockNavMesh; // should this be generic flags?
		void* myUserData; // myGameEntity
		btCollisionShape* myCollisionEntity;
	};
	btAlignedObjectArray<FPhysicsBody>*	myRigidBodies;
	bool myDebugDrawEnabled;
	bool myHasNewNavBlockers;
};

