#include "FPrimitiveBoxMultiTex.h"
#include "FVector3.h"
#include "d3dx12.h"
#include "FD3d12Renderer.h"
#include "FCamera.h"
#include <vector>

#include "FDelegate.h"
#include "FFontManager.h"
#include "FJobSystem.h"
#include "FTextureManager.h"
#include "FLightManager.h"
#include "FPrimitiveGeometry.h"
#include "FProfiler.h"

#include "FObjLoader.h"

void FPrimitiveBoxMultiTex::Init()
{
	FPrimitiveBox::Init();

	// get root sig with 32 SRV's
	m_rootSignature = FD3d12Renderer::GetInstance()->GetRootSignature(32, 1);
	m_rootSignature->SetName(L"PrimitiveBoxMultiTex");

	// set to multitex shader
	SetShader("primitiveshader_deferredMultiTex.hlsl");
	SetShader();
	myManagerClass->GetShaderManager().RegisterForHotReload(shaderfilename, this, FDelegate2<void()>::from<FPrimitiveBox, &FPrimitiveBox::SetShader>(this));
	
	// map all textures to the slots
	const FObjLoader::FObjMesh& m = myObjMesh;
	int matcounter = 0;
	std::string name;
	for (size_t i = 0; i < m.myMaterials.size(); i++)
	{
		const tinyobj::material_t& mat = m.myMaterials[i];

		{
			if (!mat.diffuse_texname.empty())
			{
				name = mat.diffuse_texname.substr(mat.diffuse_texname.find('\\') + 1, mat.diffuse_texname.length());
			}

			matcounter++;
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;
			unsigned int srvSize = FD3d12Renderer::GetInstance()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			FD3d12Renderer::GetInstance()->GetNextOffset();
			CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle0(FD3d12Renderer::GetInstance()->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), myHeapOffsetText + matcounter, srvSize);
			ID3D12Resource* texData = FTextureManager::GetInstance()->GetTextureD3D(name.c_str());
			FD3d12Renderer::GetInstance()->GetDevice()->CreateShaderResourceView(texData, &srvDesc, srvHandle0);
		}
	}
}


FPrimitiveBoxMultiTex::FPrimitiveBoxMultiTex(FD3d12Renderer* aRenderer, FVector3 aPos, FVector3 aScale, FPrimitiveBox::PrimitiveType aType)
	: FPrimitiveBox(aRenderer, aPos, aScale, aType)
{
}


FPrimitiveBoxMultiTex::~FPrimitiveBoxMultiTex()
{
}
