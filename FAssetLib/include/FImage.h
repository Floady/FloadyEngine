#pragma once
#include "FAsset.h"
class FImage :
	public FAsset
{
public:
	FAssetType GetType() override { return FAssetType::Image; }

	bool Load(const char* aPath) override;
	unsigned char* GetPixelData() { return myPixelData; }
	int GetWidth() { return myWidth; }
	int GetHeight() { return myHeight; }

	FImage();
	~FImage();

private:
	unsigned char* myPixelData;
	int myWidth;
	int myHeight;
};

