#pragma once
#include "FGameEntity.h"

class FGameLightEntity :
	public FGameEntity
{
public:
	REGISTER_GAMEENTITY(FGameLightEntity);
	FGameLightEntity();
	virtual ~FGameLightEntity();

	virtual void Init(const FJsonObject& anObj) override;
	virtual void Update(double aDeltaTime) override;
	void SetPos(const FVector3& aPos) override;
private:
	unsigned int myLightId;
	FVector3 myOffset;
	float myColorAlpha;
	float myAlphaStep;
};

