#pragma once
#include <map>
#include "FDelegate.h"

class FGameEntity;
class FGameEntityComponent;

class FGameEntityFactory
{
public:
	typedef FGameEntity* RegisterFuncPtr();
	typedef FGameEntityComponent* RegisterComponentFuncPtr();
	static FGameEntityFactory* GetInstance();
	void Register(const char* aName, RegisterFuncPtr* aFuncPtr) { myRegisteredClasses[aName] = aFuncPtr; }
	void RegisterComponent(const char* aName, RegisterComponentFuncPtr* aFuncPtr) { myRegisteredClassesComponents[aName] = aFuncPtr; }
	const char* GetEntityKey(const std::string& aName);
	const char* GetEntityComponentKey(const std::string& aName);
	FGameEntity* Create(const std::string& aName);
	FGameEntityComponent* CreateComponent(const std::string& aName);
protected:
	FGameEntityFactory();
	~FGameEntityFactory();
	static FGameEntityFactory* myInstance;
	std::map<const char*, RegisterFuncPtr*> myRegisteredClasses;
	std::map<const char*, RegisterComponentFuncPtr*> myRegisteredClassesComponents;
};

#define REGISTER_GAMEENTITY(className) static FGameEntity* Create##className() { return new className(); } \
										static const char* GetFClassName() { return #className; }

#define REGISTER_GAMEENTITY2(className) namespace { struct scopeObj { scopeObj() { FGameEntityFactory::GetInstance()->Register(className::GetFClassName(), &className::Create##className); } }; static scopeObj _tempObj; }


#define REGISTER_GAMEENTITYCOMPONENT(className) static FGameEntityComponent* Create##className() { return new className(); } \
												 static const char* GetFClassName() { return #className; }
#define REGISTER_GAMEENTITYCOMPONENT2(className) namespace { struct scopeObj { scopeObj() { FGameEntityFactory::GetInstance()->RegisterComponent(className::GetFClassName(), &className::Create##className); } }; static scopeObj _tempObj; }

