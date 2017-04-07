#pragma once
#include "..\..\FJson\FJsonObject.h"
#include "..\FDelegate.h"
#include <vector>

class FRenderableObject;
class btRigidBody;

class FGUIObject
{
public:
	FGUIObject(FVector3 aTL, FVector3 aBR) { myTL = aTL; myBR = aBR; }
	virtual bool IsInside(float aX, float aY) { return myTL.x < aX && myBR.x > aX && myTL.y < aY && myBR.y > aY; };
	virtual void OnMouseDown(float aX, float aY) {}
	virtual void OnMouseUp(float aX, float aY) {}
	virtual void OnMouseEnter(float aX, float aY) {}
	virtual void OnMouseLeave(float aX, float aY) {}
protected:
	FVector3 myTL;
	FVector3 myBR;
};

class FGUIManager
{
private:
	struct FGUIObjectStatus
	{
		FGUIObjectStatus(FGUIObject* anObj) { myObject = anObj; myIsInside = false; }
		bool myIsInside;
		FGUIObject* myObject;
	};
public:
	static FGUIManager* GetInstance();
	void AddObject(FGUIObject* anObject) { myGuiItems.push_back(FGUIObjectStatus(anObject)); }
	void Update(float aMouseX, float aMouseY, bool aIsLMouseDown, bool aIsRMouseDown);
	~FGUIManager();

private:
	FGUIManager();
	static FGUIManager* myInstance;
	std::vector<FGUIObjectStatus> myGuiItems;
};

