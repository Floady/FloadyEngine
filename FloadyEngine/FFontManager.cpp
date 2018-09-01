#include "FFontManager.h"
#include "d3dx12.h"
#include "D3dCompiler.h"
#include "FD3d12Renderer.h"
#include "FCamera.h"
#include <vector>

#pragma optimize("", off)

using namespace DirectX;

FFontManager* myInstance = nullptr;

const char* getErrorMessage(FT_Error err)
{
#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  case e: return s;
#define FT_ERROR_START_LIST     switch (err) {
#define FT_ERROR_END_LIST       }
#include FT_ERRORS_H
	return "(Unknown error)";
}

static const UINT TexturePixelSize = 4;	// The number of bytes used to represent a pixel in the texture.
std::vector<UINT8> FFontManager::GenerateTextureData(const FT_Face& aFace, const char* aText, int TextureWidth, int TextureHeight, int wordLength, UINT largestBearing)
{
	const UINT rowPitch = TextureWidth * TexturePixelSize;
	const UINT cellPitch = rowPitch >> 3;		// The width of a cell in the checkboard texture.
	const UINT cellHeight = TextureWidth >> 3;	// The height of a cell in the checkerboard texture.
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

FFontManager::FFontManager()
{
}

FFontManager::~FFontManager()
{
}

FFontManager* FFontManager::GetInstance()
{
	if (!myInstance)
		myInstance = new FFontManager();

	return myInstance;
}

const FFontManager::FFont& FFontManager::GetFont(FFontManager::FFONT_TYPE aType, int aSize, const char* aSupportedChars)
{
	for (size_t i = 0; i < myFonts.size(); i++)
	{
		if (myFonts[i].myType == aType && myFonts[i].myTexData.mySize == aSize)
		{
			return myFonts[i];
		}
	}

	return myFonts.back(); // this should return a fallback font
}

void FFontManager::InitFont(FFontManager::FFONT_TYPE aType, int aSize, const char * aSupportedChars, FD3d12Renderer * aManager, ID3D12GraphicsCommandList* aCommandList)
{
	//TEST todo
	FFont2* a = new FFont2();
	a->Load("C:/Windows/Fonts/arial.ttf");

	FFont2::TextureData texture = a->GetTextureData(aSize, aSupportedChars);
	
	// Describe and create a Texture2D.
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Width = texture.myTextureWidth;
	textureDesc.Height = texture.myTextureHeight;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	ID3D12Resource* renderTex = nullptr;

	HRESULT hr = aManager->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&renderTex));

	// these indices are also wrong -> need to be global for upload heap? get from device i guess
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(renderTex, 0, 1);

	ID3D12Resource* textureUploadHeap;

	// Create the GPU upload buffer.
	aManager->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureUploadHeap));


	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = &texture.myPixels[0];
	textureData.RowPitch = texture.myTextureWidth * TexturePixelSize;
	textureData.SlicePitch = textureData.RowPitch * texture.myTextureHeight;

	UpdateSubresources(aCommandList, renderTex, textureUploadHeap, 0, 0, 1, &textureData);

	aCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTex, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	// Describe and create a SRV for the texture.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	// Get the size of the memory location for the render target view descriptors.
	unsigned int srvSize = aManager->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	UINT myHeapOffsetText = aManager->GetNextOffset();
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle0(aManager->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), myHeapOffsetText, srvSize);
	aManager->GetDevice()->CreateShaderResourceView(renderTex, &srvDesc, srvHandle0);

	FFont newFont2;
	newFont2.myTexData = texture;
	newFont2.myFontData = a;
	newFont2.myType = aType;
	newFont2.myTexture = renderTex;

	myFonts.push_back(newFont2);
}

static FFontManager::FWordInfo wordInfo;

const FFontManager::FWordInfo& FFontManager::GetUVsForWord(const FFontManager::FFont& aFont, const char* aWord, float& aWidthOut, float& aHeightOut, bool aUseKerning)
{
	size_t wordLength = strlen(aWord);
	int texWidth = 0;
	int texHeight = 0;

	// setup word+kerning buffers
	//static FWordInfo wordInfo;
	wordInfo.clear();
	wordInfo.myDimensions.resize(wordLength);
	wordInfo.myUVTL.resize(wordLength+1);
	wordInfo.myUVBR.resize(wordLength + 1);
	wordInfo.myKerningOffset.resize(wordLength);
	
	FT_UInt prev;
	FT_Error error;

	FT_Bool hasKerning = FT_HAS_KERNING(aFont.myFontData->myFontFace);
		
	// calculate kerning values and string dimension
	for (int i = 0; i < wordLength; i++)
	{
		error = FT_Load_Char(aFont.myFontData->myFontFace, aWord[i], 0);
		const char* errorString = getErrorMessage(error);
		
		int glyphWidth = (aFont.myFontData->myFontFace->glyph->advance.x >> 6) - (aFont.myFontData->myFontFace->glyph)->bitmap_left;

		// kerning
		if (hasKerning && aUseKerning && i > 0)
		{
			prev = FT_Get_Char_Index(aFont.myFontData->myFontFace, aWord[i - 1]);
			FT_UInt next = FT_Get_Char_Index(aFont.myFontData->myFontFace, aWord[i]);
			FT_Vector delta;
			FT_Error a = FT_Get_Kerning(aFont.myFontData->myFontFace, prev, next, FT_KERNING_DEFAULT, &delta);
			texWidth += delta.x >> 6;
			wordInfo.myKerningOffset[i - 1] = static_cast<float>((delta.x >> 6));
		}

		texWidth += glyphWidth;
		int glyphHeight = (aFont.myFontData->myFontFace->glyph->metrics.height >> 6);
		texHeight = max(texHeight, glyphHeight);
		
		wordInfo.myDimensions[i].x = static_cast<float>(glyphWidth);
		wordInfo.myDimensions[i].y = static_cast<float>(glyphHeight);
	}

	// write tex dimensions to the caller so it can be scaled however they want
	aWidthOut = static_cast<float>(texWidth);
	aHeightOut = static_cast<float>(texHeight);

	// copy UVs and modify to take kerning into acocunt
	for (size_t i = 0; i < wordLength; i++)
	{
		int charIdx = 0; // should be an invalid char so you can see its missing

		// lookup char idx in all char set
		for (int j = 0; j < strlen(aFont.myTexData.myCharacters); j++)
		{
			if (aFont.myTexData.myCharacters[j] == aWord[i])
			{
				charIdx = j;
				break;
			}
		}

		wordInfo.myUVTL[i].x = aFont.myTexData.myUVs[charIdx].x;
		wordInfo.myUVTL[i].y = aFont.myTexData.myUVs[charIdx].y;
		wordInfo.myUVBR[i].x = aFont.myTexData.myUVs[charIdx + 1].x;
		wordInfo.myUVBR[i].y = aFont.myTexData.myUVs[charIdx + 1].y;
	}

	return wordInfo;
}