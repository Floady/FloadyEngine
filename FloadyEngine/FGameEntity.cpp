#include "FGame.h"
#include "FD3d12Input.h"
#include "FD3d12Renderer.h"
#include "FPrimitiveBox.h"
#include "FBulletPhysics.h"
#include "BulletDynamics\Dynamics\btRigidBody.h"
#include "FGameEntity.h"

FGameEntity::FGameEntity(FVector3 aPos, FVector3 aScale, FGameEntity::ModelType aType, float aMass /* = 0.0f */)
{
	if(aType == ModelType::Sphere)
	{
		myGraphicsObject = new FPrimitiveBox(FGame::GetInstance()->GetRenderer(), aPos, aScale, FPrimitiveBox::PrimitiveType::Sphere);
		myPhysicsObject = FGame::GetInstance()->GetPhysics()->AddObject(aMass, aPos, aScale, FBulletPhysics::CollisionPrimitiveType::Capsule);
	}
	else
	{
		myGraphicsObject = new FPrimitiveBox(FGame::GetInstance()->GetRenderer(), aPos, aScale, FPrimitiveBox::PrimitiveType::Box); // default to box
		myPhysicsObject = FGame::GetInstance()->GetPhysics()->AddObject(aMass, aPos, aScale, FBulletPhysics::CollisionPrimitiveType::Box);
	}
	
	FGame::GetInstance()->GetRenderer()->GetSceneGraph().AddObject(myGraphicsObject, false);
}

void FGameEntity::Update()
{
}

void FGameEntity::PostPhysicsUpdate()
{
	// update graphics from physics if needed

	btVector3 boxPhysPos = myPhysicsObject->getWorldTransform().getOrigin();
	float m[16];
	myPhysicsObject->getWorldTransform().getOpenGLMatrix(m);

	// kill translation - we store it seperately
	m[12] = 0;
	m[13] = 0;
	m[14] = 0;

	XMFLOAT4X4 matrix(m);
	myGraphicsObject->SetRotMatrix(&matrix);
	myGraphicsObject->SetPos(FVector3(boxPhysPos.getX(), boxPhysPos.getY(), boxPhysPos.getZ()));
}


FGameEntity::~FGameEntity()
{
}
