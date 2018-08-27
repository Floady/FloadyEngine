#pragma once

#include <ft2build.h>
#include <DirectXMath.h>
#include <ftglyph.h>
#include "FD3d12Renderer.h"
#include <vector>

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
		int mySize;
		FFONT_TYPE myType;
		int myWidth;
		int myHeight;
		std::vector<DirectX::XMFLOAT2> myUVs;
		const char* myCharacters;
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
		std::vector<DirectX::XMFLOAT2> myUVTL;
		std::vector<DirectX::XMFLOAT2> myUVBR;
		std::vector<DirectX::XMFLOAT2> myDimensions;
		std::vector<float> myKerningOffset;
	};

public:
	FFontManager();
	~FFontManager();
	static FFontManager* GetInstance();
	const FFont& GetFont(FFontManager::FFONT_TYPE aType, int aSize, const char* aSupportedChars);
	void InitFont(FFontManager::FFONT_TYPE aType, int aSize, const char* aSupportedChars, FD3d12Renderer* aManager, ID3D12GraphicsCommandList* aCommandList);
	const FWordInfo& FFontManager::GetUVsForWord(const FFontManager::FFont& aFont, const char* aWord, float& aWidthOut, float& aHeightOut, bool aUseKerning);

private:
	std::vector<UINT8> GenerateTextureData(const FT_Face& aFace, const char* aText, int TextureWidth, int TextureHeight, int wordLength, UINT largestBearing);

	FT_Library  myLibrary;
	const char* myFontMap[FFontManager::FFONT_TYPE::Count];
	FT_Face  myFontFaces[FFontManager::FFONT_TYPE::Count];
	std::vector<FFont> myFonts;
};