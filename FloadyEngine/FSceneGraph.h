#pragma once
#include <vector>
#include "FRenderableObject.h"

class FSceneGraph
{
public:
	FSceneGraph();
	void InitNewObjects();
	void AddObject(FRenderableObject* anObject, bool anIsTransparant);
	const std::vector<FRenderableObject*>& GetObjects() { return myObjects; }
	const std::vector<FRenderableObject*>& GetTransparantObjects() { return myTransparantObjects; }
	~FSceneGraph();
private:
	std::vector<FRenderableObject*> myInitObjects;
	std::vector<FRenderableObject*> myObjects;
	std::vector<FRenderableObject*> myTransparantObjects;
};

