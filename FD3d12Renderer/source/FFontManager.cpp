#include "FFontManager.h"
#include "d3dx12.h"
#include "D3dCompiler.h"
#include "FD3d12Renderer.h"
#include "FCamera.h"
#include <vector>

FFontManager* myInstance = nullptr;

static const UINT TexturePixelSize = 4;	// The number of bytes used to represent a pixel in the texture.

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

const FFontManager::FFont& FFontManager::GetFont(FFontManager::FFONT_TYPE aType, int aSize)
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
