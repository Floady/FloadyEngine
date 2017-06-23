#include "FGameUIPanel.h"
#include "GUI\FGUIManager.h"


FGameUIPanel::FGameUIPanel()
{
}

void FGameUIPanel::Hide()
{
	if (myIsHidden)
		return;

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
	for (FGUIObject* obj : myObjects)
	{
		obj->Hide();
		FGUIManager::GetInstance()->RemoveObject(obj);
		delete obj;
	}

	myObjects.clear();
}
