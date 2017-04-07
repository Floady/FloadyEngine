#include "FBulletPhysics.h"

#include "btBulletDynamicsCommon.h"
#include "LinearMath/btVector3.h"
#include "FDebugDrawer.h"
#include "FD3d12Renderer.h"

FBulletPhysics::FBulletPhysics()
{
}


FBulletPhysics::~FBulletPhysics()
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
	for (int j = 0; j<m_collisionShapes.size(); j++)
	{
		btCollisionShape* shape = m_collisionShapes[j];
		delete shape;
	}
	m_collisionShapes.clear();

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
}

void FBulletPhysics::Init(FD3d12Renderer* aRendererForDebug)
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

	myDebugDrawer = new FDebugDrawer(aRendererForDebug);
	m_dynamicsWorld->setDebugDrawer(myDebugDrawer);
}

void FBulletPhysics::Update(double aDeltaTime)
{
	if (m_dynamicsWorld)
	{
		m_dynamicsWorld->stepSimulation(static_cast<float>(aDeltaTime));
	}
}

void FBulletPhysics::DebugDrawWorld()
{
	if (m_dynamicsWorld)
	{
		m_dynamicsWorld->debugDrawWorld();
	}
}
btRigidBody* FBulletPhysics::AddObject(float aMass, FVector3 aPos, FVector3 aScale, FBulletPhysics::CollisionPrimitiveType aPrim, bool aShouldBlockNav /* = false*/)
{
	btCollisionShape* shape;
	if(aPrim == FBulletPhysics::CollisionPrimitiveType::Box)
		shape = new btBoxShape(btVector3(btScalar(aScale.x), btScalar(aScale.y), btScalar(aScale.z)));
	else if (aPrim == FBulletPhysics::CollisionPrimitiveType::Capsule)
		shape = new btCapsuleShape(aScale.x, aScale.y / 2.0f);
	else if (aPrim == FBulletPhysics::CollisionPrimitiveType::Sphere)
	{
		shape = new btSphereShape(aScale.x);
	}
	else // default
		shape = new btBoxShape(btVector3(btScalar(aScale.x), btScalar(aScale.y), btScalar(aScale.z)));

	btScalar mass(aMass);
	m_collisionShapes.push_back(shape);

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
	//body->setContactProcessingThreshold(m_defaultContactProcessingThreshold);

#else
	btRigidBody* body = new btRigidBody(mass, 0, shape, localInertia);
	body->setWorldTransform(startTransform);
#endif
	body->setUserIndex(-1);
	m_dynamicsWorld->addRigidBody(body);
	
	FBulletPhysics::FPhysicsBody fbody;
	fbody.myRigidBody = body;
	fbody.myShouldBlockNavMesh = aShouldBlockNav;
	myRigidBodies.push_back(fbody);
	return body;
}

std::vector<FBulletPhysics::AABB> FBulletPhysics::GetAABBs()
{
	std::vector<FBulletPhysics::AABB> aabbs;
	
	for (int i = 0; i < myRigidBodies.size(); i++)
	{
		FBulletPhysics::AABB aabb;
		btTransform t;
		btVector3 min;
		btVector3 max;
		btTransform trans = myRigidBodies[i].myRigidBody->getWorldTransform();
		m_collisionShapes[i]->getAabb(trans, min, max);
		
		if (!myRigidBodies[i].myShouldBlockNavMesh) // HACK, anything below 0 x is ignored for navmesh to hack floor in
			continue;
		
		//min = trans * min;
		//max = trans * max;
		aabb.myMax = FVector3(max.getX(), max.getY(), max.getZ());
		aabb.myMin = FVector3(min.getX(), min.getY(), min.getZ());
		aabbs.push_back(aabb);
	}
	
	return aabbs;
}
