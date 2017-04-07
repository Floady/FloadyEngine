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

void FGUIManager::Update(float aMouseX, float aMouseY, bool aIsLMouseDown, bool aIsRMouseDown) // normalized screen coords
{
	for (FGUIObjectStatus& objStatus : myGuiItems)
	{
		FGUIObject* obj = objStatus.myObject;
		
		if (obj->IsInside(aMouseX,aMouseY))
		{
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