#pragma once

#include <ft2build.h>
#include <ftglyph.h>
#include "FD3d12Renderer.h"
#include <vector>
#include "FFont2.h"

class FFontManager
{
public:
	enum FFONT_TYPE
	{
		Arial = 0,
		Count,
	};

	class FFont
	{
	public:
		FFONT_TYPE myType;
		FFont2::TextureData myTexData;
		FFont2* myFontData;
		ID3D12Resource* myTexture;
	};

	struct FWordInfo
	{
		void clear()
		{
			myDimensions.clear();
			myKerningOffset.clear();
			myUVBR.clear();
			myUVTL.clear();
		}
		std::vector<FVector2> myUVTL;
		std::vector<FVector2> myUVBR;
		std::vector<FVector2> myDimensions;
		std::vector<float> myKerningOffset;
	};

public:
	FFontManager();
	~FFontManager();
	static FFontManager* GetInstance();
	const FFont& GetFont(FFontManager::FFONT_TYPE aType, int aSize, const char* aSupportedChars);
	void InitFont(FFontManager::FFONT_TYPE aType, int aSize, const char* aSupportedChars, FD3d12Renderer* aManager, ID3D12GraphicsCommandList* aCommandList);
	const FWordInfo& GetUVsForWord(const FFontManager::FFont& aFont, const char* aWord, float& aWidthOut, float& aHeightOut, bool aUseKerning);

private:
	std::vector<UINT8> GenerateTextureData(const FT_Face& aFace, const char* aText, int TextureWidth, int TextureHeight, int wordLength, UINT largestBearing);
	std::vector<FFont> myFonts;
};