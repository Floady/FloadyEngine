#pragma once
#include "FGameEntityComponent.h"
#include "FVector3.h"
#include <vector>

class FGameEntity;

class FPathfindComponent : public FGameEntityComponent
{
public:
	REGISTER_GAMEENTITYCOMPONENT(FPathfindComponent);

	FPathfindComponent();
	void FindPath(FVector3 aStart, FVector3 anEnd);
	~FPathfindComponent();
	FVector3 GetPosOnPath();
	const FVector3& GetDirection();
	const FVector3& GetDesiredDirection();

	void Init(const FJsonObject& anObj) override;
	void Update(double aDeltaTime) override;
	void PostPhysicsUpdate() override;
	bool HasPath() { return myPath.size() > 1; }
	bool HasFinishedPath() { return myPath.size() == myCurTargetIdx; }
private:
	std::vector<FVector3> myPath;
	float mySpeed;
	float myPathLength;
	float myCurPosOnPath;
	int myCurTargetIdx;
	FVector3 myDirection;
	FVector3 myDesiredDirection;
	float myMaxTurnSpeed;
};

