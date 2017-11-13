#pragma once
#include "FGameEntity.h"

class FSkyBox : public FGameEntity
{
public:
	REGISTER_GAMEENTITY(FSkyBox);
	
	FSkyBox();
	~FSkyBox();

	virtual void Init(const FJsonObject& anObj) override;
	void Update(double aDeltaTime) override;

	FRenderableObject* myGraphicsObject;
};

