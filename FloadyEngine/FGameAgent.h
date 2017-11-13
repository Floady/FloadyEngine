#pragma once
#include "FGameEntity.h"

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
	FPathfindComponent* GetPathFindingComponent();

	bool myWasPathing;
};

