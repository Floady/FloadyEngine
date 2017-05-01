#include "FGameTerrain.h"

REGISTER_GAMEENTITY2(FGameTerrain);

FGameTerrain::FGameTerrain()
{
}

void FGameTerrain::Init(const FJsonObject & anObj)
{
	FGameEntity::Init(anObj);

	OutputDebugStringA("Terrain made \n");
}


FGameTerrain::~FGameTerrain()
{
}
