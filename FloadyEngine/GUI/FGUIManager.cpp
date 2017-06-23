#include "..\FGame.h"
#include "..\FD3d12Input.h"
#include "..\FD3d12Renderer.h"
#include "..\FPrimitiveBox.h"
#include "..\FBulletPhysics.h"
#include "BulletDynamics\Dynamics\btRigidBody.h"
#include "FGUIManager.h"
#include "..\FScreenQuad.h"

FGUIManager::FGUIManager()
{
}

bool FGUIManager::Update(float aMouseX, float aMouseY, bool aIsLMouseDown, bool aIsRMouseDown) // normalized screen coords
{
	bool isInsideUI = false;
	for (FGUIObjectStatus& objStatus : myGuiItems)
	{
		FGUIObject* obj = objStatus.myObject;
		
		if (obj->IsInside(aMouseX,aMouseY))
		{
			isInsideUI = true;

			if(!objStatus.myIsInside)
				obj->OnMouseEnter(aMouseX - 1.0f, -aMouseY);

			objStatus.myIsInside = true;

			if (aIsLMouseDown)
				obj->OnMouseDown(aMouseX, aMouseY);
			else
				obj->OnMouseUp(aMouseX, aMouseY);
		}
		else
		{
			if (objStatus.myIsInside)
				obj->OnMouseLeave(aMouseX - 1.0f, -aMouseY);

			objStatus.myIsInside = false;
		}
	}

	return isInsideUI;
}

FGUIManager::~FGUIManager()
{
}


FGUIManager* FGUIManager::myInstance = nullptr;
FGUIManager* FGUIManager::GetInstance()
{
	if (!myInstance)
		myInstance = new FGUIManager();

	return myInstance;
}

void FGUIManager::AddObject(FGUIObject * anObject)
{
	myGuiItems.push_back(FGUIObjectStatus(anObject));
	anObject->Show();
}

void FGUIManager::RemoveObject(FGUIObject * anObject)
{
	for (auto it = myGuiItems.begin(); it != myGuiItems.end(); ++it)
	{
		if ((*it).myObject == anObject)
		{
			(*it).myObject->Hide();
			myGuiItems.erase(it);
			return;
		}
	}
}

void FGUIManager::ClearAll()
{
	for (auto it = myGuiItems.begin(); it != myGuiItems.end(); ++it)
	{
		delete (*it).myObject;
	}
	myGuiItems.clear();
}
