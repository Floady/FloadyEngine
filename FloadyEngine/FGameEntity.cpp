#include "FGame.h"
#include "FD3d12Input.h"
#include "FD3d12Renderer.h"
#include "FPrimitiveBox.h"
#include "FBulletPhysics.h"
#include "BulletDynamics\Dynamics\btRigidBody.h"
#include "FGameEntity.h"
using namespace DirectX;
FGameEntity::FGameEntity(FVector3 aPos, FVector3 aScale, FGameEntity::ModelType aType, float aMass /* = 0.0f */, bool aIsNavBlocker /* = false */)
{
	Init(aPos, aScale, aType, aMass, aIsNavBlocker);
}

void FGameEntity::Init(FVector3 aPos, FVector3 aScale, FGameEntity::ModelType aType, float aMass /* = 0.0f */, bool aIsNavBlocker /* = false */)
{
	myPhysicsObject = nullptr;

	if(aType == ModelType::Sphere)
	{
		myGraphicsObject = new FPrimitiveBox(FGame::GetInstance()->GetRenderer(), aPos, aScale, FPrimitiveBox::PrimitiveType::Sphere);
		myPhysicsObject = FGame::GetInstance()->GetPhysics()->AddObject(aMass, aPos, aScale, FBulletPhysics::CollisionPrimitiveType::Sphere, aIsNavBlocker);
	}
	else
	{
		myGraphicsObject = new FPrimitiveBox(FGame::GetInstance()->GetRenderer(), aPos, aScale, FPrimitiveBox::PrimitiveType::Box); // default to box
		myPhysicsObject = FGame::GetInstance()->GetPhysics()->AddObject(aMass, aPos, aScale, FBulletPhysics::CollisionPrimitiveType::Box, aIsNavBlocker);
	}
	
	FGame::GetInstance()->GetRenderer()->GetSceneGraph().AddObject(myGraphicsObject, false);
}

FGameEntity::FGameEntity(const FJsonObject & anObj)
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

	type = static_cast<FGameEntity::ModelType>(anObj.GetItem("type").GetAs<int>()); // 0 spehere, 1 box

	Init(pos, scale, type, mass, isNavBlocking);

	string tex = anObj.GetItem("tex").GetAs<string>();
	myGraphicsObject->SetTexture(tex.c_str());
}

void FGameEntity::Update()
{
}

void FGameEntity::PostPhysicsUpdate()
{
	if (!myPhysicsObject)
		return;

	// update graphics from physics if needed

	btVector3 boxPhysPos = myPhysicsObject->getWorldTransform().getOrigin();
	float m[16];
	myPhysicsObject->getWorldTransform().getOpenGLMatrix(m);

	// kill translation - we store it seperately
	m[12] = 0;
	m[13] = 0;
	m[14] = 0;

	XMFLOAT4X4 matrix(m);
	myGraphicsObject->SetRotMatrix(m);
	myGraphicsObject->SetPos(FVector3(boxPhysPos.getX(), boxPhysPos.getY(), boxPhysPos.getZ()));
}


FGameEntity::~FGameEntity()
{
}
