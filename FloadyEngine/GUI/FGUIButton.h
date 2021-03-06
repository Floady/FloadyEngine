#pragma once
#include "FDelegate.h"
#include "FGUIManager.h"

class FScreenQuad;
class FDynamicText;

class FGUIButton : public FGUIObject
{
public:

	FGUIButton(FVector3 aTL, FVector3 aBR, const char* aTexture, FDelegate2<void()> aCallback);
	FGUIButton(FVector3 aTL, FVector3 aBR, const char* aTexture, FDelegate2<void(const char*)> aCallback, const char* aString);
	FGUIButton(FVector3 aTL, FVector3 aBR, const char* aTexture, FDelegate2<void(int)> aCallback, int aNumber);
	void SetDynamicText(const char* aText);
	virtual void OnMouseDown(float aX, float aY) override;
	virtual void OnMouseUp(float aX, float aY) override;
	virtual void OnMouseEnter(float aX, float aY) override;
	virtual void OnMouseLeave(float aX, float aY) override;
	virtual void Hide() override;
	virtual void Show() override;
	
	void Update();

	~FGUIButton() override;
protected:
	bool myShouldDoCallback;
	FScreenQuad* myGraphicsObject;

private:
	FDynamicText* myDynTexLabel;
	FDelegate2<void()> myCallback;
	FDelegate2<void(const char*)> myCallbackConstChar;
	FDelegate2<void(int)> myCallbackInteger;
	const char* myString;
	FVector3 myTL;
	FVector3 myBR;
	int myNumber;
};

