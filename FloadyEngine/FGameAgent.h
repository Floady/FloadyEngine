#pragma once
#include "FGameEntity.h"

template<typename T>
class SpringValue
{
public:
	SpringValue(T aStartingValue) { mySpeed = 1.0f; myValue = aStartingValue; myDesiredValue = aStartingValue; }
	SpringValue(T aStartingValue, float aSpeed) { mySpeed = aSpeed; myValue = aStartingValue; myDesiredValue = aStartingValue; }
	void SetSpeed(float aSpeed) { mySpeed = aSpeed; }
	void Update(double aDeltaTime) { myValue += (myDesiredValue - myValue) * mySpeed * aDeltaTime; }
	void SetDesiredValue(T aValue) { myDesiredValue = aValue; }
	T GetValue() { return myValue; }
private:
	T myValue;
	T myDesiredValue;
	float mySpeed;
};

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
	void SetPos(const FVector3& aPos) override;
	FPathfindComponent* GetPathFindingComponent();

	bool myWasPathing;
	float myBobTimer;
	float myBobSpeed;
	FVector3 myAgentPos;
	SpringValue<float> myRoll;
};

