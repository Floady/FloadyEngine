#pragma once
#include "FVector3.h"
#include <vector>

class FGameEntity;

class FPathfindComponent
{
public:
	FPathfindComponent(FGameEntity* aGameEntity);
	void FindPath(FVector3 aStart, FVector3 anEnd);
	~FPathfindComponent();
	void Update(double aDeltaTime);
	FVector3 GetPosOnPath();
	FVector3 GetDirection();
private:
	std::vector<FVector3> myPath;
	float mySpeed;
	float myPathLength;
	float myCurPosOnPath;
	int myCurTargetIdx;
	FGameEntity* myGameEntity;
};

