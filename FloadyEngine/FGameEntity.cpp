#include "FGame.h"
#include "FD3d12Input.h"
#include "FD3d12Renderer.h"
#include "FPrimitiveBox.h"
#include "FBulletPhysics.h"
#include "BulletDynamics\Dynamics\btRigidBody.h"
#include "FGameEntity.h"
#include "FPathfindComponent.h"
#include "FProfiler.h"

REGISTER_GAMEENTITY2(FGameEntity);

using namespace DirectX;
FGameEntity::FGameEntity(FVector3 aPos, FVector3 aScale, FGameEntity::ModelType aType, float aMass /* = 0.0f */, bool aIsNavBlocker /* = false */)
{
	myPhysicsObject = nullptr;
	myGraphicsObject = nullptr;
	Init(aPos, aScale, aType, aMass, aIsNavBlocker);
}

void FGameEntity::Init(FVector3 aPos, FVector3 aScale, FGameEntity::ModelType aType, float aMass /* = 0.0f */, bool aIsNavBlocker /* = false */)
{
	myOwner = nullptr;
	myPos = aPos;
	myIsPhysicsActive = true;

	if (myPhysicsObject)
	{
		FGame::GetInstance()->GetPhysics()->RemoveObject(myPhysicsObject);
		myPhysicsObject = nullptr;
	}

	if (myGraphicsObject)
	{
		FGame::GetInstance()->GetRenderer()->GetSceneGraph().RemoveObject(myGraphicsObject);
		delete myGraphicsObject;
		myGraphicsObject = nullptr;
	}

	if(aType == ModelType::Sphere)
	{
		myGraphicsObject = new FPrimitiveBox(FGame::GetInstance()->GetRenderer(), aPos, aScale, FPrimitiveBox::PrimitiveType::Sphere);
		myPhysicsObject = FGame::GetInstance()->GetPhysics()->AddObject(aMass, aPos, aScale, FBulletPhysics::CollisionPrimitiveType::Sphere, aIsNavBlocker, this);
	}
	else
	{
		myGraphicsObject = new FPrimitiveBox(FGame::GetInstance()->GetRenderer(), aPos, aScale, FPrimitiveBox::PrimitiveType::Box); // default to box
		myPhysicsObject = FGame::GetInstance()->GetPhysics()->AddObject(aMass, aPos, aScale, FBulletPhysics::CollisionPrimitiveType::Box, aIsNavBlocker, this);
	}
	
	FGame::GetInstance()->GetRenderer()->GetSceneGraph().AddObject(myGraphicsObject, false);
}

void FGameEntity::Init(const FJsonObject & anObj)
{
	FVector3 pos, scale;
	FGameEntity::ModelType type;
	float mass;

	pos.x = anObj.GetItem("posX").GetAs<double>();
	pos.y = anObj.GetItem("posY").GetAs<double>();
	pos.z = anObj.GetItem("posZ").GetAs<double>();

	scale.x = anObj.GetItem("scaleX").GetAs<double>();
	scale.y = anObj.GetItem("scaleY").GetAs<double>();
	scale.z = anObj.GetItem("scaleZ").GetAs<double>();

	mass = anObj.GetItem("mass").GetAs<double>();
	bool isNavBlocking = anObj.GetItem("isNavBlocker").GetAs<bool>();
	bool canMove = anObj.GetItem("canMove").GetAs<bool>();

	type = static_cast<FGameEntity::ModelType>(anObj.GetItem("type").GetAs<int>()); // 0 spehere, 1 box

//	type = FGameEntity::ModelType::Sphere;
	Init(pos, scale, type, mass, isNavBlocking);
	
	string tex = anObj.GetItem("tex").GetAs<string>();
	myGraphicsObject->SetTexture(tex.c_str());
}

void FGameEntity::Update(double aDeltaTime)
{
	FPROFILE_FUNCTION("FGameEntity Update");

	if(myGraphicsObject)
		myGraphicsObject->RecalcModelMatrix();
}

void FGameEntity::PostPhysicsUpdate()
{
	if (!myPhysicsObject || !myIsPhysicsActive)
	{
		return;
	}

	// update graphics from physics if needed

	btVector3 boxPhysPos = myPhysicsObject->getWorldTransform().getOrigin();
	float m[16];
	myPhysicsObject->getWorldTransform().getOpenGLMatrix(m);

	// kill translation - we store it seperately
	m[12] = 0;
	m[13] = 0;
	m[14] = 0;

	myGraphicsObject->SetRotMatrix(m);
	myPos = FVector3(boxPhysPos.getX(), boxPhysPos.getY(), boxPhysPos.getZ());
	myGraphicsObject->SetPos(FVector3(boxPhysPos.getX(), boxPhysPos.getY(), boxPhysPos.getZ()));
}

void FGameEntity::SetPos(FVector3 aPos)
{
	btTransform groundTransform;
	groundTransform.setIdentity();
	groundTransform.setOrigin(btVector3(aPos.x, aPos.y, aPos.z));

	myPos = aPos;

	if(myPhysicsObject)
		myPhysicsObject->setWorldTransform(groundTransform);
	if (myGraphicsObject)
		myGraphicsObject->SetPos(aPos);
}

FGameEntity::~FGameEntity()
{
}
