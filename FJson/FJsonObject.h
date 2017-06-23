#pragma once

#include "reader.h"
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include "FAny.h"

class FJsonObject
{
private:
	FJsonObject(const FJsonObject& anOther) {  }
public:
	FJsonObject(const std::string& aName) { myName = aName; myParent = nullptr; myCurrentChildId = 0; }
	FJsonObject(const std::string& aName, FJsonObject& someObject) { myName = aName; myParent = &someObject; }
	FJsonObject(const std::string& aName, FJsonObject* someObject) { myName = aName; myParent = someObject; }
	bool IsRoot() { return myParent == nullptr; }
	void AddItem(std::string aKey, FAny& anItem) { myItems[aKey] = anItem; }
	const FAny& GetItem(std::string aKey) const { return (myItems.find(aKey))->second; }
	FJsonObject* AddChild(const std::string& aName) { myChildren.push_back(new FJsonObject(aName, this)); return myChildren[myChildren.size() - 1]; }
	FJsonObject* GetParent() { return myParent; }
	const FJsonObject* GetFirstChild() const { myCurrentChildId = 0;  return myChildren.size() ? myChildren[0] : nullptr; }
	const FJsonObject* GetChildByName(const char* aName) const;
	const FJsonObject* GetNextChild() const { myCurrentChildId++;  return myChildren.size() > myCurrentChildId ? myChildren[myCurrentChildId] : nullptr; }
	~FJsonObject();
	const std::string& GetName() const { return myName; }
private:
	std::string myName;
	FJsonObject* myParent;
	std::vector<FJsonObject*> myChildren;
	std::map<std::string, FAny> myItems;
	mutable unsigned int myCurrentChildId;
};