#pragma once
#include "FAsset.h"
#include "FVector2.h"
#include <vector>

struct FT_FaceRec_;
typedef struct FT_FaceRec_*  FT_Face;

class FFont2 :
	public FAsset
{
public:
	struct TextureData
	{
		struct FWordInfo
		{
			void clear()
			{
				myDimensions.clear();
				myUVBR.clear();
				myUVTL.clear();
			}
			std::vector<FVector2> myUVTL;
			std::vector<FVector2> myUVBR;
			std::vector<FVector2> myDimensions;
			int myTexWidth;
			int myTexHeight;
		};

		FWordInfo GetUVsForWord(const char* aWord) const;

		std::vector<UINT8> myPixels;
		std::vector<FVector2> myUVs;
		unsigned int myTextureWidth;
		unsigned int myTextureHeight;
		unsigned int mySize;
		const char* myCharacters;
		FT_Face myFontFace;
		int** myKerningMap;
		int* myGlyphWidth;
		int* myGlyphHeight;
	};

	FAssetType GetType() override { return FAssetType::Font; }

	FFont2();
	virtual ~FFont2();
	bool Load(const char* aPath) override;
	TextureData GetTextureData(int aSize, const char* aSupportedCharacters);

	int mySize;
	FT_Face myFontFace;
	const char* myFontName;
};

