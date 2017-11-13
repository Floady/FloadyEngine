#include "FGameEntityFactory.h"
#include "FGameEntity.h"

FGameEntityFactory* FGameEntityFactory::myInstance = nullptr;

FGameEntityFactory * FGameEntityFactory::GetInstance()
{
	if (!myInstance)
		myInstance = new FGameEntityFactory();

	return myInstance;
}

const char * FGameEntityFactory::GetEntityKey(const std::string & aName)
{
	for (auto& comp : myRegisteredClasses)
	{
		if (strcmp(comp.first, aName.c_str()) == 0)
		{
			return comp.first;
		}
	}

	return nullptr;
}

const char * FGameEntityFactory::GetEntityComponentKey(const std::string & aName)
{
	for (auto& comp : myRegisteredClassesComponents)
	{
		if (strcmp(comp.first, aName.c_str()) == 0)
		{
			return comp.first;
		}
	}

	return nullptr;
}

FGameEntity * FGameEntityFactory::Create(const std::string & aName)
{
	for (auto& comp : myRegisteredClasses)
	{
		if (strcmp(comp.first, aName.c_str()) == 0)
		{
			return comp.second();
		}
	}

	return nullptr;
}

FGameEntityComponent * FGameEntityFactory::CreateComponent(const std::string & aName)
{
	for (auto& comp : myRegisteredClassesComponents)
	{
		if (strcmp(comp.first, aName.c_str()) == 0)
		{
			return comp.second();
		}
	}

	return nullptr;
}

FGameEntityFactory::FGameEntityFactory()
{
}


FGameEntityFactory::~FGameEntityFactory()
{
}
