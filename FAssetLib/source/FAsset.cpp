#include "FAsset.h"
#include "FFont2.h"

FAsset::FAsset()
{
}


FAsset::~FAsset()
{
}

FAsset* FAssetLoader::Load(const char * aPath)
{
	FAsset* asset = new FFont2();
	asset->Load(aPath);
	return asset;
}
