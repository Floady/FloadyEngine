#include "FShaderManager.h"
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "D3dCompiler.h"

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

FShaderManager::FShaderManager()
{
	ReloadShaders();

	// use: ReadDirectoryChangesW
	// run in own thread
	//HANDLE  ChangeHandle = FindFirstChangeNotification(_T("C:\\\MyNewFolder"), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
	//for (;;)
	//{
	//	DWORD Wait = WaitForSingleObject(ChangeHandle, INFINITE);
	//	if (Wait == WAIT_OBJECT_0)
	//	{
	//		MessageBox(NULL, _T("Change"), _T("Change"), MB_OK);
	//		FindNextChangeNotification(ChangeHandle);
	//	}
	//	else
	//	{
	//		break;
	//	}
	//}
}


FShaderManager::~FShaderManager()
{
}

void FShaderManager::ReloadShaders()
{
	HANDLE hFind;
	WIN32_FIND_DATA data;
	
	myShaders.clear();

	hFind = FindFirstFileW(L"Shaders//*.hlsl", &data);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			printf("%ws\n", data.cFileName);

			ID3DBlob* vertexShader;
			ID3DBlob* pixelShader;

			UINT compileFlags = 0;
#ifdef _DEBUG
			compileFlags |= D3DCOMPILE_DEBUG;
#endif
			std::wstring mywstring(data.cFileName);
			std::wstring concatted_stdstr = L"Shaders//" + mywstring;
			D3DCompileFromFile(concatted_stdstr.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr);
			D3DCompileFromFile(concatted_stdstr.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr);


			ID3D12ShaderReflection* lVertexShaderReflection = nullptr;
			HRESULT hr = D3DReflect(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(), IID_ID3D12ShaderReflection, (void**)&lVertexShaderReflection);

			D3D12_SHADER_DESC lShaderDesc;
			lVertexShaderReflection->GetDesc(&lShaderDesc);

			FShader shader;
			shader.myVertexShader = CD3DX12_SHADER_BYTECODE(vertexShader);
			shader.myPixelShader = CD3DX12_SHADER_BYTECODE(pixelShader);

			for (unsigned i = 0; i < lShaderDesc.InputParameters; i++)
			{
				D3D12_SIGNATURE_PARAMETER_DESC lParamDesc;
				lVertexShaderReflection->GetInputParameterDesc(i, &lParamDesc);
				int breakhere = 0;

				D3D12_INPUT_ELEMENT_DESC desc;
				desc.SemanticName = lParamDesc.SemanticName;
				desc.SemanticIndex = lParamDesc.SemanticIndex;
				desc.InputSlot = 0;
				desc.AlignedByteOffset = i == 0 ? 0 : D3D12_APPEND_ALIGNED_ELEMENT;
				desc.InstanceDataStepRate = 0;
				desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;

				if (lParamDesc.Mask == 1)
				{
					if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) desc.Format = DXGI_FORMAT_R32_UINT;
					else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) desc.Format = DXGI_FORMAT_R32_SINT;
					else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) desc.Format = DXGI_FORMAT_R32_FLOAT;
				}
				else if (lParamDesc.Mask <= 3)
				{
					if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) desc.Format = DXGI_FORMAT_R32G32_UINT;
					else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) desc.Format = DXGI_FORMAT_R32G32_SINT;
					else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) desc.Format = DXGI_FORMAT_R32G32_FLOAT;
				}
				else if (lParamDesc.Mask <= 7)
				{
					if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) desc.Format = DXGI_FORMAT_R32G32B32_UINT;
					else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) desc.Format = DXGI_FORMAT_R32G32B32_SINT;
					else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
				}
				else if (lParamDesc.Mask <= 15)
				{
					if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) desc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
					else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) desc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
					else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				}

				shader.myInputElementDescs.push_back(desc);
			}

			myShaders[std::string(ConvertFromUtf16ToUtf8(data.cFileName))] = shader;


			if (myHotReloadMap.find(std::string(ConvertFromUtf16ToUtf8(data.cFileName))) != myHotReloadMap.end())
			{
				for each(const std::pair<void*, FDelegate>& item in myHotReloadMap[std::string(ConvertFromUtf16ToUtf8(data.cFileName))])
				{
					item.second();
				}
			}

		} while (FindNextFileW(hFind, &data));
		FindClose(hFind);
	}
}

void FShaderManager::RegisterForHotReload(const char * aShaderName, void * anObject, FDelegate aReloadDelegate)
{
	myHotReloadMap[std::string(aShaderName)].push_back(std::make_pair(anObject, aReloadDelegate));
}
