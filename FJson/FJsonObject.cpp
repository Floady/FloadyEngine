#include "FJsonObject.h"



const FJsonObject * FJsonObject::GetChildByName(const char * aName) const
{
	for (int i = 0; i < myChildren.size(); i++)
	{
		if (strcmp(myChildren[i]->GetName().c_str(), aName) == 0)
			return myChildren[i];
	}
	return nullptr;
}

FJsonObject::~FJsonObject()
{
}
