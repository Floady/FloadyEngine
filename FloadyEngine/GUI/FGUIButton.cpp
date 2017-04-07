#include "..\FGame.h"
#include "..\FD3d12Input.h"
#include "..\FD3d12Renderer.h"
#include "..\FPrimitiveBox.h"
#include "..\FBulletPhysics.h"
#include "BulletDynamics\Dynamics\btRigidBody.h"
#include "FGUIButton.h"
#include "..\FScreenQuad.h"

FGUIButton::FGUIButton(FVector3 aTL, FVector3 aBR, const char* aTexture, FDelegate2<void()> aCallback) : FGUIObject(aTL, aBR)
{
	myShouldDoCallback = true;
	myCallback = aCallback;
	aTL = aTL * 2.0f - FVector3(1.0f, 1.0f, 0.0f);
	aBR = aBR * 2.0f - FVector3(1.0f, 1.0f, 0.0f);
	myGraphicsObject = new FScreenQuad(FGame::GetInstance()->GetRenderer(), FVector3(aTL.x, -aBR.y, 0.0f), aTexture, aBR.x - aTL.x, (aBR.y - aTL.y), true, true);
	FGame::GetInstance()->GetRenderer()->GetSceneGraph().AddObject(myGraphicsObject, true);
	OnMouseLeave(0, 0);
}

void FGUIButton::Update()
{
}

FGUIButton::~FGUIButton()
{
}

void FGUIButton::OnMouseEnter(float aX, float aY)
{
	myGraphicsObject->SetUVOffset(FVector3(0.0, 0.33, 0), FVector3(1, 0.66, 0));
}

void FGUIButton::OnMouseLeave(float aX, float aY)
{
	myGraphicsObject->SetUVOffset(FVector3(0, 0, 0), FVector3(1, 0.33, 0));
}

void FGUIButton::OnMouseDown(float aX, float aY)
{
	myGraphicsObject->SetUVOffset(FVector3(0.0, 0.66, 0), FVector3(1, 1.0, 0));

	if(myShouldDoCallback)
		myCallback();

	myShouldDoCallback = false;
}

void FGUIButton::OnMouseUp(float aX, float aY)
{
	myGraphicsObject->SetUVOffset(FVector3(0.0, 0.33, 0), FVector3(1, 0.66, 0));

	myShouldDoCallback = true;
}