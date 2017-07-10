#pragma once
#include "FGameEntity.h"

class FGameLightEntity :
	public FGameEntity
{
public:
	REGISTER_GAMEENTITY(FGameLightEntity);
	FGameLightEntity();
	~FGameLightEntity();

	virtual void Init(const FJsonObject& anObj) override;
	virtual void Update(double aDeltaTime) override;

private:
	unsigned int myLightId;
	float myColorAlpha;
	float myAlphaStep;
};

