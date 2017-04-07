#pragma once
#include "..\..\FJson\FJsonObject.h"
#include "..\FDelegate.h"
#include "FGUIManager.h"

class FScreenQuad;
class btRigidBody;

class FGUIButton : public FGUIObject
{
public:

	FGUIButton(FVector3 aTL, FVector3 aBR, const char* aTexture, FDelegate2<void()> aCallback);
	virtual void OnMouseDown(float aX, float aY) override;
	virtual void OnMouseUp(float aX, float aY) override;
	virtual void OnMouseEnter(float aX, float aY) override;
	virtual void OnMouseLeave(float aX, float aY) override;
	
	void Update();

	~FGUIButton();

private:
	
	FScreenQuad* myGraphicsObject;
	FDelegate2<void()> myCallback;
	bool myShouldDoCallback;
};

