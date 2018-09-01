#pragma once
#include "FAsset.h"
class FImage :
	public FAsset
{
public:
	FAssetType GetType() override { return FAssetType::Image; }

	FImage();
	~FImage();
};

