#pragma once

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

public:
	FFontManager();
	~FFontManager();
	static FFontManager* GetInstance();
	const FFont& GetFont(FFontManager::FFONT_TYPE aType, int aSize);
	void InitFont(FFontManager::FFONT_TYPE aType, int aSize, const char* aSupportedChars, FD3d12Renderer* aManager, ID3D12GraphicsCommandList* aCommandList);
	
private:
	std::vector<FFont> myFonts;
};