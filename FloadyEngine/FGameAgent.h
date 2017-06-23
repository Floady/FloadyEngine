#pragma once
#include "FGameEntity.h"
class FPathfindComponent;

class FGameAgent :
	public FGameEntity
{
public:
	FGameAgent();
	FGameAgent(const FVector3& aPos);
	~FGameAgent();
	virtual void Init(const FJsonObject& anObj) override;
	void Update(double aDeltaTime) override;
	void PostPhysicsUpdate() override;
	FPathfindComponent* GetPathFindingComponent() { return myPathFindingComponent; }

	FPathfindComponent* myPathFindingComponent; // this should be some factory based list
	bool myWasPathing;
};

