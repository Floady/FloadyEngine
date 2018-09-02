#include "FTextureManager.h"
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <vector>
#include <stdio.h>
#include "FUtilities.h"
#include "FProfiler.h"
#include "FImage.h"

static std::string ourFallbackTextureName = "reserved_fallback.reserved";
//#pragma optimize("", off)

namespace
{
	std::string ConvertFromUtf16ToUtf8(const std::wstring& wstr)
	{
		std::string convertedString;
		int requiredSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, 0, 0, 0, 0);
		if (requiredSize > 0)
		{
			std::vector<char> buffer(requiredSize);
			WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &buffer[0], requiredSize, 0, 0);
			convertedString.assign(buffer.begin(), buffer.end() - 1);
		}
		return convertedString;
	}

	std::wstring ConvertFromUtf8ToUtf16(const std::string& str)
	{
		std::wstring convertedString;
		int requiredSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, 0, 0);
		if (requiredSize > 0)
		{
			std::vector<wchar_t> buffer(requiredSize);
			MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &buffer[0], requiredSize);
			convertedString.assign(buffer.begin(), buffer.end() - 1);
		}

		return convertedString;
	}

}

// Generate a simple black and white checkerboard texture.

UINT8* FTextureManager::GenerateCheckerBoard()
{
	UINT TextureWidth = 256;
	UINT TextureHeight = 256;
	UINT TexturePixelSize = 4;

	const UINT rowPitch = TextureWidth * TexturePixelSize;
	const UINT cellPitch = rowPitch >> 3;		// The width of a cell in the checkboard texture.
	const UINT cellHeight = TextureWidth >> 3;	// The height of a cell in the checkerboard texture.
	const UINT textureSize = rowPitch * TextureHeight;

	//data.resize(textureSize);
	UINT8* pData = new UINT8[textureSize];

	for (UINT n = 0; n < textureSize; n += TexturePixelSize)
	{
		UINT x = n % rowPitch;
		UINT y = n / rowPitch;
		UINT i = x / cellPitch;
		UINT j = y / cellHeight;

		if (i % 2 == j % 2)
		{
			pData[n] = 0xff;		// R
			pData[n + 1] = 0x00;	// G
			pData[n + 2] = 0x00;	// B
			pData[n + 3] = 0xff;	// A
		}
		else
		{
			pData[n] = 0xff;		// R
			pData[n + 1] = 0xff;	// G
			pData[n + 2] = 0xff;	// B
			pData[n + 3] = 0xff;	// A
		}
	}

	return pData;
}

FTextureManager* FTextureManager::ourInstance = nullptr;

void FTextureManager::ReloadTextures()
{
	HANDLE hFind;
	WIN32_FIND_DATA data;

	//myTextures.clear();
	//myTextureMutex.WaitFor();
	//myTextureMutex.Lock();

	LPCWSTR folderFilters[] = { L"Textures//*.png" ,L"Textures//*.jpg" , L"Textures//sponza//*.png", L"Textures//sponza2//*.tga" };
	LPCWSTR folderPrefix[] = { L"Textures//" ,  L"Textures//" , L"Textures//sponza//" , L"Textures//sponza2//" };

	for (size_t i = 0; i < _countof(folderFilters); i++)
	{
		hFind = FindFirstFileW(folderFilters[i], &data);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
//				FLOG("%ws", data.cFileName);

				std::wstring mywstring(data.cFileName);
				std::wstring concatted_stdstr = folderPrefix[i] + mywstring;

				std::string cStrFilename = ConvertFromUtf16ToUtf8(mywstring);
				std::string cStrFilenameConcat = ConvertFromUtf16ToUtf8(concatted_stdstr);

				{
					FImage* image = new FImage();
					{
					//	FPROFILE_FUNCTION("FAssetImg Load");
						image->Load(cStrFilenameConcat.c_str());
					}

					myTextures[cStrFilename].myWidth = image->GetWidth();
					myTextures[cStrFilename].myHeight = image->GetHeight();
					myTextures[cStrFilename].myRawPixelData = image->GetPixelData();
//					FLOG("Taking load data from FreeImage for: %ws", data.cFileName);
				}

				myTextures[cStrFilename].myD3DResource = nullptr;	// will be filled later (init with device) this way you can pre-empt png loading while device is being made
																	// also disconnects from render device, so we can use texture + resource managing with other devices
			} while (FindNextFileW(hFind, &data));
			FindClose(hFind);
		}
	}

	//myTextureMutex.Unlock();
}

void FTextureManager::ReleaseTextures()
{
	for (std::map<std::string, TextureInfo>::iterator it = myTextures.begin(); it != myTextures.end(); ++it)
	{
		if((*it).second.myD3DResource)
			(*it).second.myD3DResource->Release();
	}
}

ID3D12Resource * FTextureManager::GetTextureD3D(const std::string& aTextureName) const
{
	assert(myInitializedD3D && "NOT INIT D3D");

	if (myTextures.find(aTextureName) != myTextures.end())
		return myTextures.find(aTextureName)->second.myD3DResource;

	return nullptr;
}

ID3D12Resource * FTextureManager::GetTextureD3DFallback() const
{
	assert(myInitializedD3D && "NOT INIT D3D");
	return myTextures.find(ourFallbackTextureName)->second.myD3DResource;
}

ID3D12Resource * FTextureManager::GetTextureD3D(const char * aTextureName) const
{
	return GetTextureD3D(std::string(aTextureName));
}

FTextureManager::FTextureManager()
{
//	HANDLE hFind;
//	WIN32_FIND_DATA data;
	void* tex = static_cast<void*>(FTextureManager::GenerateCheckerBoard());
	
	myTextures[ourFallbackTextureName].myRawPixelData = tex;
	myTextures[ourFallbackTextureName].myWidth = 256;
	myTextures[ourFallbackTextureName].myHeight = 256;
	myTextures[ourFallbackTextureName].myD3DResource = nullptr;
	
	myInitializedD3D = false;

	myTextureMutex.Init();
}

void FTextureManager::InitD3DResources(ID3D12Device* aDevice, ID3D12GraphicsCommandList* aCommandList)
{
	//myTextureMutex.WaitFor();
	//myTextureMutex.Lock();

	const int maxItems = 5;
	int nrOfItemsDone = 0;
	for (std::map<std::string, TextureInfo>::iterator it = myTextures.begin(); it != myTextures.end(); ++it)
	{
		if (nrOfItemsDone >= maxItems)
		{
			break;
		}

		TextureInfo& texture = it->second;
		if (texture.myRawPixelData && !texture.myD3DResource)
		{
			nrOfItemsDone++;
			FLOG("init: %s", it->first.c_str());
			// Describe and create a Texture2D.
			D3D12_RESOURCE_DESC textureDesc = {};
			textureDesc.MipLevels = 1;
			textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			textureDesc.Width = texture.myWidth;
			textureDesc.Height = texture.myHeight;
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
				IID_PPV_ARGS(&texture.myD3DResource));

			if (FAILED(hr))
			{
				HRESULT hr2 = FD3d12Renderer::GetInstance()->GetDevice()->GetDeviceRemovedReason();
				FLOG("CreateCommitedResource failed %x [%x]", hr, hr2);
			}

			// Create the GPU upload buffer.
			ID3D12Resource* textureUploadHeap;
			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.myD3DResource, 0, 1);
			hr = aDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&textureUploadHeap));

			if (FAILED(hr))
			{
				HRESULT hr2 = FD3d12Renderer::GetInstance()->GetDevice()->GetDeviceRemovedReason();
				FLOG("CreateCommitedResource failed %x [%x]", hr, hr2);
			}

			// Copy data to the intermediate upload heap and then schedule a copy 
			// from the upload heap to the Texture2D.
			void* texturePixelData = texture.myRawPixelData;
			D3D12_SUBRESOURCE_DATA textureData = {};
			textureData.pData = texturePixelData;
			textureData.RowPitch = texture.myWidth * 4;
			textureData.SlicePitch = textureData.RowPitch * texture.myHeight;

			UpdateSubresources(aCommandList, texture.myD3DResource, textureUploadHeap, 0, 0, 1, &textureData);
			aCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture.myD3DResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
		}
	}

	myInitializedD3D = true;
	
//	myTextureMutex.Unlock();
}

FTextureManager::~FTextureManager()
{
}

FTextureManager * FTextureManager::GetInstance()
{
	if (!ourInstance)
		ourInstance = new FTextureManager();

	return ourInstance;
}
