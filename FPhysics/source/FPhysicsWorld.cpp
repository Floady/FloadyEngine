#include "FPhysicsWorld.h"
#include "LinearMath/btAlignedObjectArray.h"
#include "btBulletDynamicsCommon.h"
#include "LinearMath/btVector3.h"
#include "FIPhysicsDebugDrawer.h"
#include "FPhysicsDebugDrawer.h"
#include "FPhysicsObject.h"

FPhysicsWorld::FPhysicsWorld()
{
	myEnabled = false;
	myDebugDrawEnabled = false;
	myHasNewNavBlockers = false;

	myRigidBodies = new btAlignedObjectArray<FPhysicsBody>();
	m_collisionShapes = new btAlignedObjectArray<btCollisionShape*>();
}

FPhysicsWorld::~FPhysicsWorld()
{
	//remove the rigidbodies from the dynamics world and delete them

	if (m_dynamicsWorld)
	{

		int i;
		for (i = m_dynamicsWorld->getNumConstraints() - 1; i >= 0; i--)
		{
			m_dynamicsWorld->removeConstraint(m_dynamicsWorld->getConstraint(i));
		}
		for (i = m_dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
		{
			btCollisionObject* obj = m_dynamicsWorld->getCollisionObjectArray()[i];
			btRigidBody* body = btRigidBody::upcast(obj);
			if (body && body->getMotionState())
			{
				delete body->getMotionState();
			}
			m_dynamicsWorld->removeCollisionObject(obj);
			delete obj;
		}
	}
	//delete collision shapes
	for (int j = 0; j<m_collisionShapes->size(); j++)
	{
		btCollisionShape* shape = (*m_collisionShapes)[j];
		delete shape;
	}
	m_collisionShapes->clear();

	delete m_dynamicsWorld;
	m_dynamicsWorld = 0;

	delete m_solver;
	m_solver = 0;

	delete m_broadphase;
	m_broadphase = 0;

	delete m_dispatcher;
	m_dispatcher = 0;

	delete m_collisionConfiguration;
	m_collisionConfiguration = 0;

	delete myRigidBodies;
	delete m_collisionShapes;
}

void FPhysicsWorld::Init(FIPhysicsDebugDrawer* aDebugDrawer)
{
	///collision configuration contains default setup for memory, collision setup
	m_collisionConfiguration = new btDefaultCollisionConfiguration();
	//m_collisionConfiguration->setConvexConvexMultipointIterations();

	///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	m_dispatcher = new	btCollisionDispatcher(m_collisionConfiguration);

	m_broadphase = new btDbvtBroadphase();

	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	btSequentialImpulseConstraintSolver* sol = new btSequentialImpulseConstraintSolver;
	m_solver = sol;

	m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_collisionConfiguration);

	m_dynamicsWorld->setGravity(btVector3(0, -10, 0));

	if (aDebugDrawer)
	{
		myDebugDrawer = new FPhysicsDebugDrawer(aDebugDrawer);
		m_dynamicsWorld->setDebugDrawer(myDebugDrawer);
	}
}

void FPhysicsWorld::Update(double aDeltaTime)
{
	if (myEnabled && m_dynamicsWorld)
	{
		m_dynamicsWorld->stepSimulation(static_cast<float>(aDeltaTime));
	}

	if (m_dynamicsWorld && myDebugDrawEnabled)
	{
		m_dynamicsWorld->debugDrawWorld();
	}
}

void FPhysicsWorld::SetDebugDrawEnabled(bool anEnabled)
{
	myDebugDrawEnabled = anEnabled;
}

FPhysicsObject* FPhysicsWorld::AddObject(float aMass, FVector3 aPos, FVector3 aScale, FPhysicsWorld::CollisionPrimitiveType aPrim, bool aShouldBlockNav /* = false*/, void* anEntity /*= nullptr*/)
{
	FVector3 extends = aScale / 2.0f;
	btCollisionShape* shape;
	if (aPrim == FPhysicsWorld::CollisionPrimitiveType::Box)
		shape = new btBoxShape(btVector3(btScalar(extends.x), btScalar(extends.y), btScalar(extends.z)));
	else if (aPrim == FPhysicsWorld::CollisionPrimitiveType::Capsule)
		shape = new btCapsuleShape(extends.x, extends.y / 2.0f);
	else if (aPrim == FPhysicsWorld::CollisionPrimitiveType::Sphere)
	{
		shape = new btSphereShape(extends.x);
	}
	else // default
		shape = new btBoxShape(btVector3(btScalar(extends.x), btScalar(extends.y), btScalar(extends.z)));

	btScalar mass(aMass);

	m_collisionShapes->push_back(shape);

	btTransform groundTransform;
	groundTransform.setIdentity();
	groundTransform.setOrigin(btVector3(aPos.x, aPos.y, aPos.z));

	bool isDynamic = (mass != 0.f);

	btVector3 localInertia(0, 0, 0);
	if (isDynamic)
		shape->calculateLocalInertia(mass, localInertia);

	//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects

#define USE_MOTIONSTATE 1
#ifdef USE_MOTIONSTATE
	btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);

	btRigidBody::btRigidBodyConstructionInfo cInfo(mass, myMotionState, shape, localInertia);

	btRigidBody* body = new btRigidBody(cInfo);
	FPhysicsObject* physObject = new FPhysicsObject(body);

	//body->setGravity(btVector3(0, 0, 0));
	//body->setContactProcessingThreshold(m_defaultContactProcessingThreshold);

#else
	btRigidBody* body = new btRigidBody(mass, 0, shape, localInertia);
	body->setWorldTransform(startTransform);
#endif
	body->setUserIndex(-1);
	m_dynamicsWorld->addRigidBody(body);

	FPhysicsWorld::FPhysicsBody fbody;
	fbody.myRigidBody = physObject;
	fbody.myCollisionEntity = shape;
	fbody.myUserData = anEntity;
	fbody.myShouldBlockNavMesh = aShouldBlockNav;
	myRigidBodies->push_back(fbody);

	if (aShouldBlockNav)
		myHasNewNavBlockers = true;

	return physObject;
}

void FPhysicsWorld::AddTerrain(const std::vector<FVector3>& aTriangleList, void * anOwner)
{
	btTriangleMesh* triangleMeshTerrain = new btTriangleMesh();
	for (int i = 0; i < aTriangleList.size(); i += 3)
	{
		btVector3 posA = btVector3(aTriangleList[i].x, aTriangleList[i].y, aTriangleList[i].z);
		btVector3 posB = btVector3(aTriangleList[i + 1].x, aTriangleList[i + 1].y, aTriangleList[i + 1].z);
		btVector3 posC = btVector3(aTriangleList[i + 2].x, aTriangleList[i + 2].y, aTriangleList[i + 2].z);
		triangleMeshTerrain->addTriangle(posA, posB, posC);
	}

	btCollisionShape* myColShape = new btBvhTriangleMeshShape(triangleMeshTerrain, true);
	btDefaultMotionState* motionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));
	btRigidBody::btRigidBodyConstructionInfo rigidBodyConstructionInfo(0.0f, motionState, myColShape, btVector3(0, 0, 0));
	btRigidBody* terrain = new btRigidBody(rigidBodyConstructionInfo);

	FPhysicsWorld::FPhysicsBody fbody;
	fbody.myRigidBody = new FPhysicsObject(terrain);
	fbody.myCollisionEntity = myColShape;
	fbody.myUserData = anOwner;
	fbody.myShouldBlockNavMesh = false;
	m_collisionShapes->push_back(myColShape);
	myRigidBodies->push_back(fbody);
	m_dynamicsWorld->addRigidBody(terrain);
}

void FPhysicsWorld::RemoveObject(FPhysicsObject * aBody)
{
	int idxToRemove = -1;
	for (int i = 0; i < myRigidBodies->size(); i++)
	{
		if ((*myRigidBodies)[i].myRigidBody == aBody)
		{
			idxToRemove = i;
			break;
		}
	}

	for (int i = m_dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
	{
		btCollisionObject* obj = m_dynamicsWorld->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body == aBody->GetRigidBody())
		{
			m_dynamicsWorld->removeCollisionObject(obj);
			delete obj;
			break;
		}
	}

	if (idxToRemove == -1)
	{
		// todo FLOG("Error: removing a body that doesnt exist in RigidBodies?");
	}
	else
	{
		myRigidBodies->removeAtIndex(idxToRemove);
	}
}

void* FPhysicsWorld::GetFirstEntityHit(FVector3 aStart, FVector3 anEnd)
{
	btVector3 start = btVector3(aStart.x, aStart.y, aStart.z);
	btVector3 end = btVector3(anEnd.x, anEnd.y, anEnd.z);

	btCollisionWorld::ClosestRayResultCallback cb(start, end);
	m_dynamicsWorld->rayTest(start, end, cb);
	if (cb.hasHit())
	{
		for (int i = 0; i < myRigidBodies->size(); i++)
		{
			if ((*myRigidBodies)[i].myCollisionEntity == cb.m_collisionObject->getCollisionShape())
				return (*myRigidBodies)[i].myUserData;
		}
	}

	//myDebugDrawer->drawLine(start, end, btVector3(1, 1, 1));
	//float size = 0.1f;
	//aStart = aStart + (anEnd - aStart).Normalized() * 3.0f;
	//myDebugDrawer->DrawTriangle(aStart + FVector3(-size, 0, -size), aStart + FVector3(-size, 0, size), aStart + FVector3(size, 0, size), FVector3(1, 0, 0));
	//size = 0.5f;
	//myDebugDrawer->DrawTriangle(anEnd + FVector3(-size, 0, -size), anEnd + FVector3(-size, 0, size), anEnd + FVector3(size, 0, size), FVector3(0, 1, 0));

	return nullptr;
}

bool FPhysicsWorld::RayCast(FVector3 aStart, FVector3 anEnd, FPhysicsWorld::RayCastHit& outHitResult)
{
	btVector3 start = btVector3(aStart.x, aStart.y, aStart.z);
	btVector3 end = btVector3(anEnd.x, anEnd.y, anEnd.z);

	btCollisionWorld::ClosestRayResultCallback cb(start, end);
	m_dynamicsWorld->rayTest(start, end, cb);
	if (cb.hasHit())
	{
		outHitResult.myPos = FVector3(cb.m_hitPointWorld.getX(), cb.m_hitPointWorld.getY(), cb.m_hitPointWorld.getZ());
		outHitResult.myNormal = FVector3(cb.m_hitNormalWorld.getX(), cb.m_hitNormalWorld.getY(), cb.m_hitNormalWorld.getZ());

		if (myDebugDrawer)
		{
			myDebugDrawer->drawLine(start, end, btVector3(1, 1, 1));
			float size = 0.1f;
			aStart = aStart + (anEnd - aStart).Normalized() * 3.0f;
			myDebugDrawer->DrawTriangle(aStart + FVector3(-size, 0, -size), aStart + FVector3(-size, 0, size), aStart + FVector3(size, 0, size), FVector3(1, 0, 0));
			size = 0.5f;
			myDebugDrawer->DrawTriangle(anEnd + FVector3(-size, 0, -size), anEnd + FVector3(-size, 0, size), anEnd + FVector3(size, 0, size), FVector3(0, 1, 0));
			myDebugDrawer->DrawTriangle(outHitResult.myPos + FVector3(-size, 0, -size), outHitResult.myPos + FVector3(-size, 0, size), outHitResult.myPos + FVector3(size, 0, size), FVector3(0, 0, 1));
		}

		return true;
	}

	return false;
}

void FPhysicsWorld::SetPaused(bool aPause)
{
	myEnabled = !aPause;
}

std::vector<FPhysicsWorld::AABB> FPhysicsWorld::GetAABBs()
{
	std::vector<FPhysicsWorld::AABB> aabbs;

	for (int i = 0; i < myRigidBodies->size(); i++)
	{
		FPhysicsWorld::AABB aabb;
		btTransform t;
		btVector3 min;
		btVector3 max;
		btTransform trans = (*myRigidBodies)[i].myRigidBody->GetRigidBody()->getWorldTransform();
		(*m_collisionShapes)[i]->getAabb(trans, min, max);

		if (!(*myRigidBodies)[i].myShouldBlockNavMesh)
			continue;

		aabb.myMax = FVector3(max.getX(), max.getY(), max.getZ());
		aabb.myMin = FVector3(min.getX(), min.getY(), min.getZ());
		aabbs.push_back(aabb);
	}

	return aabbs;
}
