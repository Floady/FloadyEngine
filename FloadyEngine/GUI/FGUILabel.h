#pragma once
#include "FDelegate.h"
#include "FGUIManager.h"

class FScreenQuad;
class FDynamicText;

class FGUILabel : public FGUIObject
{
public:

	FGUILabel(FVector3 aTL, FVector3 aBR, const char* aTexture = nullptr);
	void SetDynamicText(const char* aText);
	virtual void Hide() override;
	virtual void Show() override;
	
	void Update();

	~FGUILabel() override;
protected:
	bool myShouldDoCallback;
	FScreenQuad* myGraphicsObject;

private:
	FDynamicText* myDynTexLabel;
	const char* myString;
	FVector3 myTL;
	FVector3 myBR;
	int myNumber;
};

