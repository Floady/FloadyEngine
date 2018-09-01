#include "FFont2.h"
#include <ft2build.h>
#include <ftglyph.h>
#include <vector>

#pragma optimize("", off)

class FFont2_FreeTypeLoader
{
public:

	static const char* FFont2_FreeTypeLoader::getErrorMessage(FT_Error err)
	{
#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  case e: return s;
#define FT_ERROR_START_LIST     switch (err) {
#define FT_ERROR_END_LIST       }
#include FT_ERRORS_H
		return "(Unknown error)";
	}


	static FFont2_FreeTypeLoader* GetInstance() {
		if (!ourInstance) 
			ourInstance	= new FFont2_FreeTypeLoader();
		return ourInstance;
	}
	static const UINT TexturePixelSize = 4;	// The number of bytes used to represent a pixel in the texture.
	
	std::vector<UINT8> FFont2_FreeTypeLoader::GenerateTextureData(const FT_Face& aFace, const char* aText, int TextureWidth, int TextureHeight, int wordLength, UINT largestBearing)
	{
		const UINT rowPitch = TextureWidth * TexturePixelSize;
		const UINT textureSize = rowPitch * TextureHeight;

		std::vector<UINT8> data(textureSize);
		UINT8* pData = &data[0];

		// fill buffer
		int xoffset = 0;
		for (int h = 0; h < wordLength; h++)
		{
			int counter = 0;
			FT_Error error = FT_Load_Char(aFace, aText[h], 0);

			error = FT_Render_Glyph(aFace->glyph, FT_RENDER_MODE_NORMAL);
			FT_Bitmap bitmap = aFace->glyph->bitmap;

			int glyphHeight = (aFace->glyph->metrics.height >> 6);
			int top = largestBearing - (aFace->glyph->metrics.horiBearingY >> 6);

			//xoffset += (aFace->glyph)->bitmap_left * TexturePixelSize;
			int offset = xoffset + top*TextureWidth * TexturePixelSize;

			for (unsigned int i = 0; i < bitmap.rows; i++)
			{
				for (int j = 0; j < bitmap.pitch; j++)
				{
					data[offset + counter++] = bitmap.buffer[i*bitmap.pitch + j];
					data[offset + counter++] = bitmap.buffer[i*bitmap.pitch + j];
					data[offset + counter++] = bitmap.buffer[i*bitmap.pitch + j];
					data[offset + counter++] = bitmap.buffer[i*bitmap.pitch + j]; // set alpha to 0 if there is no color value
				}

				offset += (TextureWidth - bitmap.pitch) * TexturePixelSize;
			}

			// advance.x = whitespace before glyph + charWidth + whitespace after glyph, we already moved the whitespace before and the charWidth, so we move: advance.x - whiteSpace before (bitmap_left) = whitespace after glyph
			xoffset += ((aFace->glyph)->advance.x >> 6) * TexturePixelSize;// -((aFace->glyph)->bitmap_left * TexturePixelSize);
		}

		return data;
	}

	FT_Library& GetLibrary() { return myLibrary; }

private:
	FFont2_FreeTypeLoader()
	{
		FT_Error error = FT_Init_FreeType(&myLibrary);

	}
	FT_Library  myLibrary;
	static FFont2_FreeTypeLoader* ourInstance;
};

FFont2_FreeTypeLoader* FFont2_FreeTypeLoader::ourInstance = nullptr;

FFont2::FFont2()
{
}

FFont2::~FFont2()
{
	delete myFontFace;
}

bool FFont2::Load(const char * aPath)
{
	FT_Error error = FT_New_Face(FFont2_FreeTypeLoader::GetInstance()->GetLibrary(), aPath, 0, &(myFontFace));

	return false;
}

FFont2::TextureData FFont2::GetTextureData(int aSize, const char * aSupportedCharacters)
{
	mySize = aSize;

	unsigned int TextureWidth = 0;
	unsigned int TextureHeight = 0;
	int wordLength = static_cast<int>(strlen(aSupportedCharacters));
	int largestBearing = 0;
	int texWidth = 0;
	int texHeight = 0;

	FT_Error error;
	TextureData texData;


	// setup uv buffer
	unsigned int allSupportedLength = static_cast<unsigned int>(strlen(aSupportedCharacters));
	texData.myUVs.resize(allSupportedLength + 1);

	texData.myUVs[0].x = 0;
	texData.myUVs[0].y = 0;

	FT_Bool hasKerning = FT_HAS_KERNING(myFontFace);

	texData.myKerningMap = new int*[allSupportedLength];

	// test kerning
	for (int i = 0; i < allSupportedLength - 1; i++)
	{
		FT_UInt prev = FT_Get_Char_Index(myFontFace, aSupportedCharacters[i]);

		texData.myKerningMap[i] = new int[allSupportedLength];

		for (int j = 0; j < allSupportedLength; j++)
		{
			FT_UInt next = FT_Get_Char_Index(myFontFace, aSupportedCharacters[j]);
			FT_Vector delta;
			FT_Error a = FT_Get_Kerning(myFontFace, prev, next, FT_KERNING_DEFAULT, &delta);
			int kerningOffset = 0;

			if (delta.x != 0)
			{
				kerningOffset = delta.x >> 6;
				char buff[128];
				sprintf_s(buff, "Kerning found: %c %c = %i\n", aSupportedCharacters[i], aSupportedCharacters[j], kerningOffset);
				OutputDebugStringA(buff);
			}

			texData.myKerningMap[i][j] = kerningOffset;
		}
	}

	error = FT_Set_Char_Size(myFontFace, 0, mySize * 32, 1200, 1080);

	texData.myGlyphWidth = new int[allSupportedLength];
	texData.myGlyphHeight = new int[allSupportedLength];

	// calculate buffer dimensions
	for (unsigned int i = 0; i < allSupportedLength; i++)
	{
		error = FT_Load_Char(myFontFace, aSupportedCharacters[i], 0);
		const char* errorString = FFont2_FreeTypeLoader::getErrorMessage(error);

		int glyphWidth = (myFontFace->glyph->advance.x >> 6) - (myFontFace->glyph)->bitmap_left;
		texWidth += glyphWidth;
		int glyphHeight = (myFontFace->glyph->metrics.height >> 6);

		largestBearing = max(largestBearing, myFontFace->glyph->metrics.horiBearingY >> 6);
		texHeight = max(texHeight, glyphHeight);

		texData.myGlyphWidth[i] = glyphWidth;
		texData.myGlyphHeight[i] = glyphHeight;

		texData.myUVs[i + 1].x = static_cast<float>(texWidth);
		texData.myUVs[i + 1].y = static_cast<float>(glyphHeight);
	}

	texData.myTextureWidth = texWidth;
	texData.myTextureHeight = texHeight + 10; // some sizes require + 1 here in the past.. 

	// scale UVs
	for (unsigned int i = 0; i < allSupportedLength + 1; i++)
	{
		texData.myUVs[i].x /= texData.myTextureWidth;
		texData.myUVs[i].y /= texData.myTextureHeight;
	}
		
	texData.myPixels = FFont2_FreeTypeLoader::GetInstance()->GenerateTextureData(myFontFace, aSupportedCharacters, texData.myTextureWidth, texData.myTextureHeight, wordLength, largestBearing);
	texData.mySize = mySize;
	texData.myCharacters = aSupportedCharacters;
	texData.myFontFace = myFontFace;

	return texData;
}

FFont2::TextureData::FWordInfo FFont2::TextureData::GetUVsForWord(const char * aWord)
{
	size_t wordLength = strlen(aWord);
	int texWidth = 0;
	int texHeight = 0;

	FFont2::TextureData::FWordInfo wordInfo;

	// setup word+kerning buffers
	//static FWordInfo wordInfo;
	wordInfo.clear();
	wordInfo.myDimensions.resize(wordLength);
	wordInfo.myUVTL.resize(wordLength + 1);
	wordInfo.myUVBR.resize(wordLength + 1);

	// copy UVs and modify to take kerning into acocunt
	for (size_t i = 0; i < wordLength; i++)
	{
		int charIdx = 0; // should be an invalid char so you can see its missing

						 // lookup char idx in all char set
		for (int j = 0; j < strlen(myCharacters); j++)
		{
			if (myCharacters[j] == aWord[i])
			{
				charIdx = j;
				break;
			}
		}

		wordInfo.myUVTL[i].x = myUVs[charIdx].x;
		wordInfo.myUVTL[i].y = myUVs[charIdx].y;
		wordInfo.myUVBR[i].x = myUVs[charIdx + 1].x;
		wordInfo.myUVBR[i].y = myUVs[charIdx + 1].y;
		
		wordInfo.myDimensions[i].x = static_cast<float>(myGlyphWidth[charIdx]);
		wordInfo.myDimensions[i].y = static_cast<float>(myGlyphHeight[charIdx]);

		texWidth += myGlyphWidth[charIdx];
		texHeight = max(myGlyphWidth[charIdx], texHeight);
	}

	// write tex dimensions to the caller so it can be scaled however they want
	wordInfo.myTexWidth = texWidth;
	wordInfo.myTexHeight = texHeight;

	return wordInfo;
}
