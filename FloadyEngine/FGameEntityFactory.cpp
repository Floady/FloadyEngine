#include "FGameEntityFactory.h"
#include "FGameEntity.h"

FGameEntityFactory* FGameEntityFactory::myInstance = nullptr;

FGameEntityFactory * FGameEntityFactory::GetInstance()
{
	if (!myInstance)
		myInstance = new FGameEntityFactory();

	return myInstance;
}

FGameEntityFactory::FGameEntityFactory()
{
}


FGameEntityFactory::~FGameEntityFactory()
{
}
