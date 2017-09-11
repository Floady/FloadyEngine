#pragma once

#include <png.h>
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
	void* GetTexture(const char* aTextureName) const { return myTextures.find(std::string(aTextureName))->second.myRawPixelData; }
	ID3D12Resource* GetTextureD3D(const char* aTextureName) const { assert(myInitializedD3D && "NOT INIT D3D");  return myTextures.find(std::string(aTextureName))->second.myD3DResource; }
private:
	FTextureManager();
	~FTextureManager();
	bool myInitializedD3D;
	static FTextureManager* ourInstance;

	std::map<std::string, TextureInfo> myTextures;
};

