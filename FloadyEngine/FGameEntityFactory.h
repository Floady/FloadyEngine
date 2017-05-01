#pragma once
#include <map>
#include "FDelegate.h"

class FGameEntity;
class FGameEntityFactory
{
public:
	typedef FGameEntity* RegisterFuncPtr();
	static FGameEntityFactory* GetInstance();
	void Register(std::string aName, RegisterFuncPtr* aFuncPtr) { myRegisteredClasses[aName] = aFuncPtr; }
	FGameEntity* Create(const std::string& aName) { return myRegisteredClasses[aName](); }
protected:
	FGameEntityFactory();
	~FGameEntityFactory();
	static FGameEntityFactory* myInstance;
	std::map<std::string, RegisterFuncPtr*> myRegisteredClasses;
};

#define REGISTER_GAMEENTITY(className) static FGameEntity* Create##className() { return new className(); } //
#define REGISTER_GAMEENTITY2(className) namespace { struct scopeObj { scopeObj() { FGameEntityFactory::GetInstance()->Register(#className, &className::Create##className); } }; static scopeObj _tempObj; }

