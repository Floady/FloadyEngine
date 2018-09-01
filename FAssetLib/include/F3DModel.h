#pragma once
#include "FAsset.h"
class F3DModel :
	public FAsset
{
public:
	FAssetType GetType() override { return FAssetType::Model; }

	F3DModel();
	~F3DModel();
};

