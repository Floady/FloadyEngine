#include "FGameLightEntity.h"
#include "FLightManager.h"
#include "FGame.h"
#include "FD3d12Input.h"
#include "FD3d12Renderer.h"
#include "FPrimitiveBox.h"
#include "FBulletPhysics.h"
#include "BulletDynamics\Dynamics\btRigidBody.h"
#include "FGameEntity.h"
#include "FPathfindComponent.h"

REGISTER_GAMEENTITY2(FGameLightEntity);

FGameLightEntity::FGameLightEntity()
{
	myLightId = 0;
	myColorAlpha = 0;
	myAlphaStep = 0.3;
}

FGameLightEntity::~FGameLightEntity()
{
}

void FGameLightEntity::Init(const FJsonObject & anObj)
{
	const FJsonObject* entObj = anObj.GetChildByName("FGameEntity");
	assert(entObj);
	FGameEntity::Init(*entObj);

	FVector3 dir;
	dir.x = anObj.GetItem("dirX").GetAs<double>();
	dir.y = anObj.GetItem("dirY").GetAs<double>();
	dir.z = anObj.GetItem("dirZ").GetAs<double>();

	FVector3 color;
	color.x = anObj.GetItem("r").GetAs<double>();
	color.y = anObj.GetItem("g").GetAs<double>();
	color.z = anObj.GetItem("b").GetAs<double>();

	FVector3 offset;
	offset.x = anObj.GetItem("offsetX").GetAs<double>();
	offset.y = anObj.GetItem("offsetY").GetAs<double>();
	offset.z = anObj.GetItem("offsetZ").GetAs<double>();

	double type = anObj.GetItem("b").GetAs<double>();

	if (type == 1.0)
	{
		double radius = anObj.GetItem("radius").GetAs<double>();
		myLightId = FLightManager::GetInstance()->AddSpotlight(myPos + offset, dir, radius, color);
	}
	else if (type == 2.0)
		myLightId = FLightManager::GetInstance()->AddDirectionalLight(myPos + offset, dir, color);
}

void FGameLightEntity::Update(double aDeltaTime)
{
	if (!myLightId)
		return;

	
	if (myColorAlpha >= 0.0f)
		myAlphaStep = -0.3;
	else if(myColorAlpha <= -1.0f)
		myAlphaStep = 0.3;

	// todo: convert this stuff in behaviors or styles that lights can play (movement, color, shuttering)
	myColorAlpha += myAlphaStep * aDeltaTime;

	//FLightManager::GetInstance()->SetLightColor(myLightId, FVector3(myColorAlpha, myColorAlpha, myColorAlpha));
	//FLightManager::GetInstance()->GetLight(myLightId)->myColor = FVector3(myColorAlpha, myColorAlpha, myColorAlpha);
	FLightManager::GetInstance()->GetLight(myLightId)->myDir.y = sin(myColorAlpha * PI);
	FLightManager::GetInstance()->GetLight(myLightId)->myDir.x = cos(myColorAlpha * PI);
}