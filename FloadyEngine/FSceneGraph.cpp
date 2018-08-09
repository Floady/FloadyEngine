#include "FSceneGraph.h"
#include "FUtilities.h"



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

	if (myInitObjects.size() > 0)
		FLOG("Initialized new objects");

	myInitObjects.clear();
}

void FSceneGraph::AddObject(FRenderableObject * anObject, bool anIsTransparant, bool callInit)
{
	if (anIsTransparant)
	{
		bool isInList = false;
		for (FRenderableObject* obj : myTransparantObjects)
		{
			if (obj == anObject)
			{
				isInList = true;
				break;
			}
		}

		if (!isInList)
			myTransparantObjects.push_back(anObject);
	}
	else
	{
		bool isInList = false;
		for (FRenderableObject* obj : myObjects)
		{
			if (obj == anObject)
			{
				isInList = true;
				break;
			}
		}

		if (!isInList)
			myObjects.push_back(anObject);
	}

	if(callInit)
	{	
		bool isInList = false;
		for (FRenderableObject* obj : myInitObjects)
		{
			if (obj == anObject)
			{
				isInList = true;
				break;
			}
		}

		if(!isInList)
			myInitObjects.push_back(anObject);
	}
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

void FSceneGraph::HideObject(FRenderableObject * anObject)
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
}

FSceneGraph::~FSceneGraph()
{
	for (std::vector<FRenderableObject*>::iterator it = myObjects.begin(); it != myObjects.end(); ++it)
	{
		delete *it;
	}

	myObjects.clear();
}
