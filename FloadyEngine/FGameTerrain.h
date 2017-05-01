#pragma once

#include "FGameEntity.h"

class FGameTerrain : public FGameEntity
{
public:
	REGISTER_GAMEENTITY(FGameTerrain);

	FGameTerrain();
	virtual void Init(const FJsonObject& anObj);
	~FGameTerrain();
};

