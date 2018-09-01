#pragma once

class FAsset
{
public:

	enum class FAssetType : int
	{
		Font = 0,
		Image,
		Model,
	};

	FAsset();
	virtual ~FAsset();
	virtual FAssetType GetType() = 0;
	virtual bool Load(const char* aPath) = 0;
};


class FAssetLoader
{
	static FAsset* Load(const char* aPath);
};