#include "FSceneGraph.h"



FSceneGraph::FSceneGraph()
{
}

void FSceneGraph::InitNewObjects()
{
	for (FRenderableObject* object : myInitObjects)
	{
		object->Init();
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


FSceneGraph::~FSceneGraph()
{
}
