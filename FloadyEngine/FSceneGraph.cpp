#include "FSceneGraph.h"



FSceneGraph::FSceneGraph()
{
}

void FSceneGraph::InitNewObjects()
{
	for (FRenderableObject* object : myInitObjects)
	{
		static int once = 1;
	//	if (once > 0)
		{
			object->Init();
			once--;
		}
	}

	myInitObjects.clear();
}

void FSceneGraph::AddObject(FRenderableObject * anObject, bool anIsTransparant)
{
	if (anIsTransparant)
		myTransparantObjects.push_back(anObject);
	else
		myObjects.push_back(anObject);

	myInitObjects.push_back(anObject);
}

void FSceneGraph::RemoveObject(FRenderableObject * anObject)
{
	for (std::vector<FRenderableObject*>::iterator it = myTransparantObjects.begin(); it != myTransparantObjects.end(); ++it)
	{
		if (*it == anObject)
		{
			myTransparantObjects.erase(it);
			break;
		}
	}

	for (std::vector<FRenderableObject*>::iterator it = myObjects.begin(); it != myObjects.end(); ++it)
	{
		if (*it == anObject)
		{
			myObjects.erase(it);
			break;
		}
	}

	for (std::vector<FRenderableObject*>::iterator it = myInitObjects.begin(); it != myInitObjects.end(); ++it)
	{
		if (*it == anObject)
		{
			myInitObjects.erase(it);
			break;
		}
	}
}

FSceneGraph::~FSceneGraph()
{
}
