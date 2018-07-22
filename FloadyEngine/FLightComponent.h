#pragma once
#include "FGameEntityComponent.h"
#include "FVector3.h"

class FLightComponent : public FGameEntityComponent
{
public:
	REGISTER_GAMEENTITYCOMPONENT(FLightComponent);


	FLightComponent();
	~FLightComponent();

	void Init(const FJsonObject& anObj) override;
	void Update(double aDeltaTime) override;
	virtual void PostPhysicsUpdate() override {}

private:
	unsigned int myLightId;
	FVector3 myOffset;
	float myColorAlpha;
	float myAlphaStep;
	// parent?
};

