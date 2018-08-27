#pragma once

#include <Windows.h>
#include <map>

#include "FD3d12Renderer.h"

class FTextureManager
{
public:
	struct TextureInfo
	{
		void* myRawPixelData;
		ID3D12Resource* myD3DResource;
		float myWidth;
		float myHeight;
	};

	
	void InitD3DResources(ID3D12Device* aDevice, ID3D12GraphicsCommandList* aCommandList);
	static FTextureManager* GetInstance();
	void ReloadTextures();
	void ReleaseTextures();
	void* GetTexture(const char* aTextureName) const { return myTextures.find(std::string(aTextureName))->second.myRawPixelData; }
	ID3D12Resource* GetTextureD3D(const char* aTextureName) const;
	ID3D12Resource* GetTextureD3D(const std::string& aTextureName) const;
	ID3D12Resource* GetTextureD3DFallback() const;
private:
	UINT8* GenerateCheckerBoard();
	FTextureManager();
	~FTextureManager();
	bool myInitializedD3D;
	static FTextureManager* ourInstance;

	std::map<std::string, TextureInfo> myTextures;
	FD3d12Renderer::FMutex myTextureMutex;
};

