#pragma once
#include "..\..\FJson\FJsonObject.h"
#include "..\FDelegate.h"
#include <vector>
#include "..\FVector3.h"

class FRenderableObject;
class btRigidBody;

class FGUIObject
{
public:
	FGUIObject(FVector3 aTL, FVector3 aBR) {
		myTL = aTL; myBR = aBR; myIsVisible = false;
	}
	virtual ~FGUIObject() {}
	virtual bool IsInside(float aX, float aY) { return myTL.x < aX && myBR.x > aX && myTL.y < aY && myBR.y > aY; };
	virtual void OnMouseDown(float aX, float aY) {}
	virtual void OnMouseUp(float aX, float aY) {}
	virtual void OnMouseEnter(float aX, float aY) {}
	virtual void OnMouseLeave(float aX, float aY) {}
	virtual void Hide() { myIsVisible = false; };
	virtual void Show() { myIsVisible = true; };
protected:
	FVector3 myTL;
	FVector3 myBR;
	bool myIsVisible;
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
	void AddObject(FGUIObject* anObject); 
	void RemoveObject(FGUIObject* anObject);
	void ClearAll();
	bool Update(float aMouseX, float aMouseY, bool aIsLMouseDown, bool aIsRMouseDown); // returns wether mouse is on a UI item
	~FGUIManager();

private:
	FGUIManager();
	static FGUIManager* myInstance;
	std::vector<FGUIObjectStatus> myGuiItems;
};

