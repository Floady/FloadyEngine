#pragma once
#include "GUI\FGUIButton.h"

class FGUIButtonToggle : public FGUIButton
{
public:
	FGUIButtonToggle(FVector3 aTL, FVector3 aBR, const char* aTexture, FDelegate2<void(bool)> aCallback);
	void CallbackInternal();
	~FGUIButtonToggle();

	virtual void OnMouseLeave(float aX, float aY) override;

protected:
	bool myIsToggled;
	FDelegate2<void(bool)> myCallbackToggle;
};

