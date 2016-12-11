#include "FFontManager.h"
#include "d3dx12.h"
#include "D3dCompiler.h"
#include "FD3DClass.h"
#include "FCamera.h"
#include <vector>

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

		//int top = (texHeight - bitmap.rows);
		int glyphHeight = (aFace->glyph->metrics.height >> 6);
		int top = largestBearing - (aFace->glyph->metrics.horiBearingY >> 6);


		// kerning?
		if (h > 0)
		{
			FT_UInt prev;
			prev = FT_Get_Char_Index(aFace, aText[h - 1]);
			FT_UInt next = FT_Get_Char_Index(aFace, aText[h]);
			FT_Vector delta;
			FT_Get_Kerning(aFace, prev, next, FT_KERNING_DEFAULT, &delta);
			xoffset += (delta.x >> 6) * TexturePixelSize;
			int breakhere = 0;
		}

		xoffset += (aFace->glyph)->bitmap_left * TexturePixelSize;
		int offset = xoffset + top*TextureWidth * TexturePixelSize;

		for (unsigned int i = 0; i < bitmap.rows; i++)
		{
			for (int j = 0; j < bitmap.pitch; j++)
			{
				data[offset + counter++] = bitmap.buffer[i*bitmap.pitch + j];
				data[offset + counter++] = bitmap.buffer[i*bitmap.pitch + j];
				data[offset + counter++] = bitmap.buffer[i*bitmap.pitch + j];
				data[offset + counter++] = 255;
			}

			offset += (TextureWidth - bitmap.pitch) * TexturePixelSize;
		}

		// advance.x = whitespace before glyph + charWidth + whitespace after glyph, we already moved the whitespace before and the charWidth, so we move: advance.x - whiteSpace before (bitmap_left) = whitespace after glyph
		xoffset += ((aFace->glyph)->advance.x >> 6) * TexturePixelSize - ((aFace->glyph)->bitmap_left * TexturePixelSize);
	}

	return data;
}

FFontManager::FFontManager()
{
	myFontMap[FFontManager::FFONT_TYPE::Arial] = "C:/Windows/Fonts/Arial.ttf";

	FT_Error error = FT_Init_FreeType(&myLibrary);

	for (size_t i = 0; i < FFontManager::FFONT_TYPE::Count; i++)
	{
		error = FT_New_Face(myLibrary, myFontMap[i], 0, &myFontFaces[i]);
	}
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
		if (myFonts[i].myType == aType && myFonts[i].mySize == aSize)
		{
			return myFonts[i];
		}
	}

	return myFonts.back(); // this should return a fallback font
}

void FFontManager::InitFont(FFontManager::FFONT_TYPE aType, int aSize, const char * aSupportedChars, ID3D12Device * aDevice, ID3D12CommandQueue * aCmdQueue, FD3DClass * aManager, ID3D12GraphicsCommandList* aCommandList, ID3D12DescriptorHeap* anSRVHeap)
{
	FFont newFont;
	newFont.myType = aType;
	newFont.mySize = aSize;
	newFont.myCharacters = aSupportedChars;

	unsigned int TextureWidth = 0;
	unsigned int TextureHeight = 0;
	size_t wordLength = strlen(aSupportedChars);
	unsigned int largestBearing = 0;
	int texWidth = 0;
	int texHeight = 0;

	// setup uv buffer
	unsigned int allSupportedLength = strlen(aSupportedChars);
	newFont.myUVs.resize(allSupportedLength + 1);

	newFont.myUVs[0].x = 0;
	newFont.myUVs[0].y = 0;

	FT_UInt prev;
	FT_Error error;

	error = FT_Set_Char_Size(myFontFaces[aType], 0, aSize * 32, 100, 100);

	// calculate buffer dimensions
	for (int i = 0; i < allSupportedLength; i++)
	{
		error = FT_Load_Char(myFontFaces[aType], aSupportedChars[i], 0);
		const char* errorString = getErrorMessage(error);

		//// kerning?
		// no kerning for atlas tex
		/*	if (i > 0)
		{
		prev = FT_Get_Char_Index(myFontFaces[aType], aSupportedChars[i - 1]);
		FT_UInt next = FT_Get_Char_Index(myFontFaces[aType], aSupportedChars[i]);
		FT_Vector delta;
		FT_Get_Kerning(myFontFaces[aType], prev, next, FT_KERNING_DEFAULT, &delta);
		texWidth += delta.x >> 6;
		int breakhere = 0;
		}*/

		int glyphWidth = (myFontFaces[aType]->glyph->advance.x >> 6);
		texWidth += glyphWidth;
		int glyphHeight = (myFontFaces[aType]->glyph->metrics.height >> 6);

		largestBearing = max(largestBearing, myFontFaces[aType]->glyph->metrics.horiBearingY >> 6);
		texHeight = max(texHeight, glyphHeight);

		newFont.myUVs[i + 1].x = texWidth;
		newFont.myUVs[i + 1].y = glyphHeight;
	}

	TextureWidth = texWidth;
	TextureHeight = texHeight + 1; // some sizes require + 1 here in the past.. 

								   // scale UVs
	for (int i = 0; i < allSupportedLength + 1; i++)
	{
		newFont.myUVs[i].x /= TextureWidth;
		newFont.myUVs[i].y /= TextureHeight;
	}

	newFont.myHeight = TextureHeight;
	newFont.myWidth = TextureWidth;
	
	std::vector<UINT8> texture = GenerateTextureData(myFontFaces[aType], aSupportedChars, newFont.myWidth, newFont.myHeight, wordLength, largestBearing);
	

	//
	// You can do this with a temp commandlist i think
	// so all you need is device, commandQueue and managerclass?
	
	// Describe and create a Texture2D.
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Width = TextureWidth;
	textureDesc.Height = TextureHeight;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	HRESULT hr = aDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&newFont.myTexture));

	// these indices are also wrong -> need to be global for upload heap? get from device i guess
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(newFont.myTexture, 0, 1);

	ID3D12Resource* textureUploadHeap;

	// Create the GPU upload buffer.
	aDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureUploadHeap));


	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = &texture[0];
	textureData.RowPitch = TextureWidth * TexturePixelSize;
	textureData.SlicePitch = textureData.RowPitch * TextureHeight;

	UpdateSubresources(aCommandList, newFont.myTexture, textureUploadHeap, 0, 0, 1, &textureData);

	aCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(newFont.myTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	// Describe and create a SRV for the texture.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	// Get the size of the memory location for the render target view descriptors.
	unsigned int srvSize = aDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	UINT myHeapOffsetText = aManager->GetNextOffset();
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle0(anSRVHeap->GetCPUDescriptorHandleForHeapStart(), myHeapOffsetText, srvSize);
	aDevice->CreateShaderResourceView(newFont.myTexture, &srvDesc, srvHandle0);

	aCommandList->Close();

	// do we need this?
	ID3D12CommandList* ppCommandLists[] = { aCommandList };
	aCmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	myFonts.push_back(newFont);
}
