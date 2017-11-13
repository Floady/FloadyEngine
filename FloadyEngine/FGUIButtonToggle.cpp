#include "FGUIButtonToggle.h"
#include "FScreenQuad.h"



FGUIButtonToggle::FGUIButtonToggle(FVector3 aTL, FVector3 aBR, const char* aTexture, FDelegate2<void(bool)> aCallback)
	: FGUIButton(aTL, aBR, aTexture, FDelegate2<void()>::from<FGUIButtonToggle, &FGUIButtonToggle::CallbackInternal>(this))
	, myIsToggled(false)
	, myCallbackToggle(aCallback)
{
}

void FGUIButtonToggle::CallbackInternal()
{
	myIsToggled = !myIsToggled;
	myCallbackToggle(myIsToggled);
}


FGUIButtonToggle::~FGUIButtonToggle()
{
}

void FGUIButtonToggle::OnMouseLeave(float aX, float aY)
{
	if(myIsToggled)
		myGraphicsObject->SetUVOffset(FVector3(0.0f, 0.66f, 0), FVector3(1, 1.0f, 0));
	else
		myGraphicsObject->SetUVOffset(FVector3(0, 0, 0), FVector3(1, 0.33f, 0));
}
