#include "..\FGame.h"
#include "..\FD3d12Input.h"
#include "..\FD3d12Renderer.h"
#include "FGUIButton.h"
#include "..\FScreenQuad.h"
#include "..\FDynamicText.h"
#include "..\FSceneGraph.h"

FGUIButton::FGUIButton(FVector3 aTL, FVector3 aBR, const char* aTexture, FDelegate2<void()> aCallback) : FGUIObject(aTL, aBR)
{
	myDynTexLabel = nullptr;
	myShouldDoCallback = true;
	myCallback = aCallback;
	myCallbackConstChar.reset();
	myCallbackInteger.reset();
	aTL = aTL * 2.0f - FVector3(1.0f, 1.0f, 0.0f);
	aBR = aBR * 2.0f - FVector3(1.0f, 1.0f, 0.0f);
	myTL = aTL;
	myBR = aBR;
	myGraphicsObject = new FScreenQuad(FGame::GetInstance()->GetRenderer(), FVector3(aTL.x, -aBR.y, 0.0f), aTexture, aBR.x - aTL.x, (aBR.y - aTL.y), true, true);
	FGame::GetInstance()->GetRenderer()->GetSceneGraph().AddObject(myGraphicsObject, true, true);
	OnMouseLeave(0, 0);
	Show();
}

FGUIButton::FGUIButton(FVector3 aTL, FVector3 aBR, const char* aTexture, FDelegate2<void(const char*)> aCallback, const char* aString) : FGUIObject(aTL, aBR)
{
	myDynTexLabel = nullptr;
	myString = aString;
	myShouldDoCallback = true;
	myCallback.reset();
	myCallbackConstChar = aCallback;
	myCallbackInteger.reset();
	aTL = aTL * 2.0f - FVector3(1.0f, 1.0f, 0.0f);
	aBR = aBR * 2.0f - FVector3(1.0f, 1.0f, 0.0f);
	myTL = aTL;
	myBR = aBR;
	myGraphicsObject = new FScreenQuad(FGame::GetInstance()->GetRenderer(), FVector3(aTL.x, -aBR.y, 0.0f), aTexture, aBR.x - aTL.x, (aBR.y - aTL.y), true, true);
	FGame::GetInstance()->GetRenderer()->GetSceneGraph().AddObject(myGraphicsObject, true, true);
	OnMouseLeave(0, 0);
	Show();
}

FGUIButton::FGUIButton(FVector3 aTL, FVector3 aBR, const char* aTexture, FDelegate2<void(int)> aCallback, int aNumber) : FGUIObject(aTL, aBR)
{
	myDynTexLabel = nullptr;
	myNumber = aNumber;
	myString = nullptr;
	myShouldDoCallback = true;
	myCallback.reset();
	myCallbackInteger = aCallback;
	myCallbackConstChar.reset();
	aTL = aTL * 2.0f - FVector3(1.0f, 1.0f, 0.0f);
	aBR = aBR * 2.0f - FVector3(1.0f, 1.0f, 0.0f);
	myTL = aTL;
	myBR = aBR;
	myGraphicsObject = new FScreenQuad(FGame::GetInstance()->GetRenderer(), FVector3(aTL.x, -aBR.y, 0.0f), aTexture, aBR.x - aTL.x, (aBR.y - aTL.y), true, true);
	FGame::GetInstance()->GetRenderer()->GetSceneGraph().AddObject(myGraphicsObject, true, true);
	OnMouseLeave(0, 0);
	Show();
}

void FGUIButton::SetDynamicText(const char * aText)
{
	if (!myDynTexLabel)
	{
		float distFromEdge = 0.02f;
		myDynTexLabel = new FDynamicText(FGame::GetInstance()->GetRenderer(), FVector3(myTL.x + distFromEdge, -myBR.y + distFromEdge, 0.0f), aText, (myBR.x-myTL.x) - distFromEdge*2, (myBR.y-myTL.y) - distFromEdge*2, true, true);
		FGame::GetInstance()->GetRenderer()->GetSceneGraph().AddObject(myDynTexLabel, true, true);
		if(!myIsVisible)
			FGame::GetInstance()->GetRenderer()->GetSceneGraph().HideObject(myDynTexLabel);
	}
	else
	{
		myDynTexLabel->SetText(aText);
	}
}

void FGUIButton::Update()
{
}

FGUIButton::~FGUIButton()
{
	if(myDynTexLabel)
	{
		FGame::GetInstance()->GetRenderer()->GetSceneGraph().RemoveObject(myDynTexLabel);
		delete myDynTexLabel;
	}

	FGame::GetInstance()->GetRenderer()->GetSceneGraph().RemoveObject(myGraphicsObject);
	delete myGraphicsObject;
}

void FGUIButton::Hide()
{
	//if(myIsVisible)
	{
		FGame::GetInstance()->GetRenderer()->GetSceneGraph().HideObject(myGraphicsObject);

		if (myDynTexLabel)
		{
			FGame::GetInstance()->GetRenderer()->GetSceneGraph().HideObject(myDynTexLabel);
		}
	}

	FGUIObject::Hide();
}

void FGUIButton::Show()
{
	//if (!myIsVisible)
	{
		FGame::GetInstance()->GetRenderer()->GetSceneGraph().AddObject(myGraphicsObject, true, false);

		if (myDynTexLabel)
			FGame::GetInstance()->GetRenderer()->GetSceneGraph().AddObject(myDynTexLabel, true, false);
	}

	FGUIObject::Show();
}

void FGUIButton::OnMouseEnter(float aX, float aY)
{
	myGraphicsObject->SetUVOffset(FVector3(0, 0.33f, 0), FVector3(1, 0.66f, 0));
}

void FGUIButton::OnMouseLeave(float aX, float aY)
{
	myGraphicsObject->SetUVOffset(FVector3(0, 0, 0), FVector3(1, 0.33f, 0));
}

void FGUIButton::OnMouseDown(float aX, float aY)
{
	myGraphicsObject->SetUVOffset(FVector3(0, 0.66f, 0), FVector3(1, 1, 0));

	if(myShouldDoCallback)
	{
		if(myCallback)
			myCallback();
		if (myCallbackConstChar)
			myCallbackConstChar(myString);
		if (myCallbackInteger)
			myCallbackInteger(myNumber);
	}

	myShouldDoCallback = false;
}

void FGUIButton::OnMouseUp(float aX, float aY)
{
	myGraphicsObject->SetUVOffset(FVector3(0.0, 0.33, 0), FVector3(1, 0.66, 0));

	myShouldDoCallback = true;
}