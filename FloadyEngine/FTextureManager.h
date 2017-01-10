#pragma once

#include <png.h>
#include <Windows.h>
#include <map>
class FTextureManager
{
public:
	FTextureManager();
	~FTextureManager();
	static FTextureManager* GetInstance();
	void ReloadTextures();
	void* GetTexture(const char* aTextureName) const { return myTextures.find(std::string(aTextureName))->second; }
private:
	static FTextureManager* ourInstance;

	std::map<std::string, void*> myTextures;
};

