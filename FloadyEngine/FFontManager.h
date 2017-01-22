#pragma once

#include <ft2build.h>
#include <DirectXMath.h>
#include <dxgi1_4.h>
#include "d3dx12.h"
#include <ftglyph.h>
#include "FD3DClass.h"
#include <vector>

using namespace DirectX;

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
		std::vector<XMFLOAT2> myUVs;
		const char* myCharacters;
		ID3D12Resource* myTexture;
	};

	struct FWordInfo
	{
		std::vector<XMFLOAT2> myUVTL;
		std::vector<XMFLOAT2> myUVBR;
		std::vector<XMFLOAT2> myDimensions;
		std::vector<float> myKerningOffset;
	};

public:
	FFontManager();
	~FFontManager();
	static FFontManager* GetInstance();
	const FFont& GetFont(FFontManager::FFONT_TYPE aType, int aSize, const char* aSupportedChars);
	void InitFont(FFontManager::FFONT_TYPE aType, int aSize, const char* aSupportedChars, FD3DClass* aManager, ID3D12GraphicsCommandList* aCommandList);
	FWordInfo FFontManager::GetUVsForWord(const FFontManager::FFont& aFont, const char* aWord, float& aWidthOut, float& aHeightOut, bool aUseKerning);

private:
	std::vector<UINT8> GenerateTextureData(const FT_Face& aFace, const char* aText, int TextureWidth, int TextureHeight, int wordLength, UINT largestBearing);

	FT_Library  myLibrary;
	const char* myFontMap[FFontManager::FFONT_TYPE::Count];
	FT_Face  myFontFaces[FFontManager::FFONT_TYPE::Count];
	std::vector<FFont> myFonts;
};