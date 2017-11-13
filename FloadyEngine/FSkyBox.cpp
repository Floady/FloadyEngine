
#include "FSkyBox.h"
#include "FGame.h"
#include "FProfiler.h"
#include "FCamera.h"
#include "FD3d12Renderer.h"
#include "FRenderMeshComponent.h"

REGISTER_GAMEENTITY2(FSkyBox);

FSkyBox::FSkyBox()
{
}


FSkyBox::~FSkyBox()
{
}

void FSkyBox::Init(const FJsonObject & anObj)
{
	FGameEntity::Init(anObj);

	myGraphicsObject = GetComponentInSlot<FRenderMeshComponent>(0)->GetGraphicsObject();

	myGraphicsObject->myRenderCCW = true;
	myGraphicsObject->SetShader("skyboxShader.hlsl");
	myGraphicsObject->SetCastsShadows(false);
}

void FSkyBox::Update(double aDeltaTime)
{
	FGameEntity::Update(aDeltaTime);
	myPos = FGame::GetInstance()->GetRenderer()->GetCamera()->GetPos();
}
