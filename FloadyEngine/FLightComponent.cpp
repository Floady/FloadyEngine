#include "FLightComponent.h"
#include "FLightManager.h"
#include "FGameEntity.h"

REGISTER_GAMEENTITYCOMPONENT2(FLightComponent);

FLightComponent::FLightComponent()
{
	myLightId = 0;
	myColorAlpha = 0;
	myAlphaStep = 0.3f;
	myOffset.x = 0; myOffset.y = 0; myOffset.z = 0;
}

FLightComponent::~FLightComponent()
{
	if(myLightId)
		FLightManager::GetInstance()->RemoveLight(myLightId);
}

void FLightComponent::Init(const FJsonObject & anObj)
{
	const FJsonObject* entObj = anObj.GetChildByName("FGameEntity");
	assert(entObj);

	FVector3 dir;
	dir.x = anObj.GetItem("dirX").GetAs<double>();
	dir.y = anObj.GetItem("dirY").GetAs<double>();
	dir.z = anObj.GetItem("dirZ").GetAs<double>();

	FVector3 color;
	color.x = anObj.GetItem("r").GetAs<double>();
	color.y = anObj.GetItem("g").GetAs<double>();
	color.z = anObj.GetItem("b").GetAs<double>();

	myOffset;
	myOffset.x = anObj.GetItem("offsetX").GetAs<double>();
	myOffset.y = anObj.GetItem("offsetY").GetAs<double>();
	myOffset.z = anObj.GetItem("offsetZ").GetAs<double>();

	double type = anObj.GetItem("type").GetAs<double>();

	if (type == 1.0)
	{
		double radius = anObj.GetItem("radius").GetAs<double>();
		myLightId = FLightManager::GetInstance()->AddSpotlight(myOwner->GetPos() + myOffset, dir, radius, color);
	}
	else if (type == 2.0)
		myLightId = FLightManager::GetInstance()->AddDirectionalLight(myOwner->GetPos() + myOffset, dir, color);
}

void FLightComponent::Update(double aDeltaTime)
{
	if (!myLightId)
		return;

	static float light_dir = 0.0f;
	static float light_speed = 0.1f;
	static bool light_positive = true;

	if (light_dir > 1.0f)
		light_positive = false;
	else if (light_dir < -1.0f)
		light_positive = true;

	if (light_positive)
		light_dir += aDeltaTime * light_speed;
	else
		light_dir -= aDeltaTime * light_speed;

	FLightManager::GetInstance()->SetLightDir(myLightId, FVector3(0, -1, light_dir));

	if (myColorAlpha >= 0.0f)
		myAlphaStep = -0.3f;
	else if (myColorAlpha <= -1.0f)
		myAlphaStep = 0.3f;

	// todo: convert this stuff in behaviors or styles that lights can play (movement, color, shuttering)
	myColorAlpha += myAlphaStep * aDeltaTime;

	FVector3 pos = myOffset;
	if (myOwner)
		pos += myOwner->GetPos();

	FLightManager::GetInstance()->SetLightPos(myLightId, pos + myOffset);
}
