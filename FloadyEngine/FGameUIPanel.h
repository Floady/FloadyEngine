#pragma once
#include <vector>
class FGUIObject;

class FGameUIPanel
{
public:
	FGameUIPanel();
	virtual void Update() = 0;
	virtual void Hide();
	virtual void Show();
	void AddObject(FGUIObject* anObject);
	~FGameUIPanel();
	void DestroyAllItems();
private:
	std::vector<FGUIObject*> myObjects;
	bool myIsHidden;
};

