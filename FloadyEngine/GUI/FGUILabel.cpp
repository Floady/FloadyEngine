#include "..\FGame.h"
#include "..\FD3d12Input.h"
#include "FD3d12Renderer.h"
#include "FGUILabel.h"
#include "FScreenQuad.h"
#include "FDynamicText.h"
#include "FSceneGraph.h"

FGUILabel::FGUILabel(FVector3 aTL, FVector3 aBR, const char* aTexture) : FGUIObject(aTL, aBR)
{
	myGraphicsObject = nullptr;
	myDynTexLabel = nullptr;
	myShouldDoCallback = true;
	aTL = aTL * 2.0f - FVector3(1.0f, 1.0f, 0.0f);
	aBR = aBR * 2.0f - FVector3(1.0f, 1.0f, 0.0f);
	myTL = aTL;
	myBR = aBR;
	if(aTexture)
	{
		myGraphicsObject = new FScreenQuad(FGame::GetInstance()->GetRenderer(), FVector3(aTL.x, -aBR.y, 0.0f), aTexture, aBR.x - aTL.x, (aBR.y - aTL.y), true, true);
		FGame::GetInstance()->GetRenderer()->GetSceneGraph().AddObject(myGraphicsObject, true, true);
	}

	Show();
}

void FGUILabel::SetDynamicText(const char * aText)
{
	if (!myDynTexLabel)
	{
		float distFromEdge = 0.02f;
		myDynTexLabel = new FDynamicText(FGame::GetInstance()->GetRenderer(), FVector3(myTL.x + distFromEdge, -myBR.y + distFromEdge, 0.0f), aText, (myBR.x-myTL.x) - distFromEdge*2, (myBR.y-myTL.y) - distFromEdge*2, true, true);
		myDynTexLabel->SetFitToWidth(false);
		FGame::GetInstance()->GetRenderer()->GetSceneGraph().AddObject(myDynTexLabel, true, true);
		if(!myIsVisible)
			FGame::GetInstance()->GetRenderer()->GetSceneGraph().HideObject(myDynTexLabel);
	}
	else
	{
		myDynTexLabel->SetFitToWidth(false);
		myDynTexLabel->SetText(aText);
	}
}

void FGUILabel::Update()
{
}

FGUILabel::~FGUILabel()
{
	if(myDynTexLabel)
	{
		FGame::GetInstance()->GetRenderer()->GetSceneGraph().RemoveObject(myDynTexLabel);
		delete myDynTexLabel;
	}

	if(myGraphicsObject)
	{
		FGame::GetInstance()->GetRenderer()->GetSceneGraph().RemoveObject(myGraphicsObject);
		delete myGraphicsObject;
	}
}

void FGUILabel::Hide()
{
	//if(myIsVisible)
	{
		if (myGraphicsObject)
		{
			FGame::GetInstance()->GetRenderer()->GetSceneGraph().HideObject(myGraphicsObject);
		}

		if (myDynTexLabel)
		{
			FGame::GetInstance()->GetRenderer()->GetSceneGraph().HideObject(myDynTexLabel);
		}
	}

	FGUIObject::Hide();
}

void FGUILabel::Show()
{
	//if (!myIsVisible)
	{
		if (myGraphicsObject)
		{
			FGame::GetInstance()->GetRenderer()->GetSceneGraph().AddObject(myGraphicsObject, true, false);
		}

		if (myDynTexLabel)
			FGame::GetInstance()->GetRenderer()->GetSceneGraph().AddObject(myDynTexLabel, true, false);
	}

	FGUIObject::Show();
}
