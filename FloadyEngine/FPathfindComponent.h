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
	FVector3 GetDirection();

	void Init(const FJsonObject& anObj) override;
	void Update(double aDeltaTime) override;
	void PostPhysicsUpdate() override;
	bool HasPath() { return myPath.size() > 1; }
private:
	std::vector<FVector3> myPath;
	float mySpeed;
	float myPathLength;
	float myCurPosOnPath;
	int myCurTargetIdx;
};

