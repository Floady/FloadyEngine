#include "FGameUIPanel.h"
#include "GUI\FGUIManager.h"
#include "FUtilities.h"


FGameUIPanel::FGameUIPanel()
{
}

void FGameUIPanel::Hide()
{
	myIsHidden = true;
	for (FGUIObject* obj : myObjects)
	{
		obj->Hide();
		FGUIManager::GetInstance()->RemoveObject(obj);
	}
}

void FGameUIPanel::Show()
{
	if (!myIsHidden)
		return;

	myIsHidden = false;
	for (FGUIObject* obj : myObjects)
	{
		FGUIManager::GetInstance()->AddObject(obj);
	}
}

void FGameUIPanel::AddObject(FGUIObject * anObject)
{
	myObjects.push_back(anObject);
	if (!myIsHidden)
	{
		anObject->Show();
		FGUIManager::GetInstance()->AddObject(anObject);
	}
}

FGameUIPanel::~FGameUIPanel()
{
	DestroyAllItems();
}

void FGameUIPanel::DestroyAllItems()
{
	FLOG("DestroyAllItems called");
	for (FGUIObject* obj : myObjects)
	{
		obj->Hide();
		FGUIManager::GetInstance()->RemoveObject(obj);
		delete obj;
	}

	myObjects.clear();
}
