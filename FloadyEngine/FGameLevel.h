#pragma once
#include <vector>

class FGameEntity;

class FGameLevel
{
public:
	FGameLevel(const char* aLevelName);
	void Update(double aDeltaTime);
	void AddEntity(FGameEntity* anEntity) { myEntityContainer.push_back(anEntity); }
	void PostPhysicsUpdate();
	~FGameLevel();
private:
	std::vector<FGameEntity*> myEntityContainer;
};

