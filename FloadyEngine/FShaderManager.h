#pragma once

#include "d3dx12.h"
#include <vector>
#include <tchar.h>
#include <stdio.h>
#include <map>
#include "FDelegate.h"

class FShaderManager
{
public:
	struct FShader
	{
		CD3DX12_SHADER_BYTECODE myVertexShader;
		CD3DX12_SHADER_BYTECODE myPixelShader;
		std::vector<D3D12_INPUT_ELEMENT_DESC> myInputElementDescs;
	};

	FShaderManager();
	~FShaderManager();

	void ReloadShaders();
	const FShader& GetShader(const char* aShaderName) const { return myShaders.find(std::string(aShaderName))->second; }
	void RegisterForHotReload(const char* aShaderName, void* anObject, FDelegate aReloadDelegate);
private:
	std::map<std::string, FShader> myShaders;
	std::map<std::string, std::vector<std::pair<void*, FDelegate> > > myHotReloadMap;
	
	wchar_t a;
};

