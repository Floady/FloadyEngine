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
#include "FUtilities.h"

void FPrimitiveBoxMultiTex::Init()
{
	HRESULT hr;
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

		// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

		if (FAILED(myManagerClass->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

		CD3DX12_ROOT_PARAMETER1 rootParameters[1];
		rootParameters[0].InitAsDescriptorTable(2, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC  rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ID3DBlob* signature;
		ID3DBlob* error;
		hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error);
		hr = myManagerClass->GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
		m_rootSignature->SetName(L"PrimitiveBoxInstanced");

		// shadow root sig
		CD3DX12_DESCRIPTOR_RANGE1 rangesShadows[1];
		rangesShadows[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

		CD3DX12_ROOT_PARAMETER1 rootParametersShadow[1];
		rootParametersShadow[0].InitAsDescriptorTable(1, &rangesShadows[0], D3D12_SHADER_VISIBILITY_ALL);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC  rootSignatureDescShadows;
		rootSignatureDescShadows.Init_1_1(_countof(rootParametersShadow), rootParametersShadow, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ID3DBlob* signatureShadows;
		hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDescShadows, featureData.HighestVersion, &signatureShadows, &error);
		hr = myManagerClass->GetDevice()->CreateRootSignature(0, signatureShadows->GetBufferPointer(), signatureShadows->GetBufferSize(), IID_PPV_ARGS(&m_rootSignatureShadows));
		m_rootSignatureShadows->SetName(L"PrimitiveBoxInstancedShadow");
	}


	SetShader();
	myManagerClass->GetShaderManager().RegisterForHotReload(shaderfilename, this, FDelegate2<void()>::from<FPrimitiveBoxInstanced, &FPrimitiveBoxInstanced::SetShader>(this));

	// Create vertex + index buffer

	// create constant buffer for modelview

	const int buff_size = sizeof(float) * (myNrOfInstances + 1) * 16;
	myHeapOffsetCBVShadow = myManagerClass->CreateConstantBuffer(m_ModelProjMatrixShadow, myConstantBufferShadowsPtr, sizeof(float) * 32 * 16);
	myHeapOffsetCBV = myManagerClass->CreateConstantBuffer(m_ModelProjMatrix, myConstantBufferPtr, buff_size);
	m_ModelProjMatrix->SetName(L"PrimitiveBoxInstancedConst");
	m_ModelProjMatrixShadow->SetName(L"PrimitiveBoxInstancedConstShadows");
	myHeapOffsetAll = myHeapOffsetCBV;
	myHeapOffsetText = myHeapOffsetAll;

	memset(myConstantBufferShadowsPtr, 0, sizeof(float) * 32 * 16);

	skipNextRender = false;
	myIsInitialized = true;

	RecalcModelMatrix();

	// get root sig with 32 SRV's
	m_rootSignature = FD3d12Renderer::GetInstance()->GetRootSignature(64, 1);
	m_rootSignature->SetName(L"PrimitiveBoxMultiTex");

	// set to multitex shader
	SetShader("primitiveshader_deferredMultiTex.hlsl");
	SetShader();
	myManagerClass->GetShaderManager().RegisterForHotReload(shaderfilename, this, FDelegate2<void()>::from<FPrimitiveBoxInstanced, &FPrimitiveBoxInstanced::SetShader>(this));
	
	myTexOffset = FD3d12Renderer::GetInstance()->BindTexture("");
	for (size_t i = 0; i < 63; i++) // 63 + 1 = 64 :^)
	{
		FD3d12Renderer::GetInstance()->BindTexture("");
	}
}

void FPrimitiveBoxMultiTex::ObjectLoadingDone(const FMeshManager::FMeshObject& anObj)
{
	FLOG("ObgjectLOadingDone called");

	// map all textures to the slots
	const FObjLoader::FObjMesh& m = anObj.myMeshData;

	int matcounter = 0;
	std::string name;
	for (size_t i = 0; i < m.myMaterials.size(); i++)
	{
		const tinyobj::material_t& mat = m.myMaterials[i];

		if (!mat.diffuse_texname.empty())
		{
			name = mat.diffuse_texname.substr(mat.diffuse_texname.find('\\') + 1, mat.diffuse_texname.length());
		}

		FD3d12Renderer::GetInstance()->BindTextureToSlot(name, myTexOffset + i);
	}

	for (size_t i = 0; i < m.myMaterials.size(); i++)
	{
		const tinyobj::material_t& mat = m.myMaterials[i];
		if (!mat.bump_texname.empty())
		{
			name = mat.bump_texname.substr(mat.bump_texname.find('\\') + 1, mat.bump_texname.length());
		}

		FD3d12Renderer::GetInstance()->BindTextureToSlot(name, myTexOffset + i + m.myMaterials.size());
	}

	for (const FPrimitiveGeometry::Vertex2& vert : anObj.myVertices)
	{
		myAABB.myMax.x = max(myAABB.myMax.x, vert.position.x * GetScale().x);
		myAABB.myMax.y = max(myAABB.myMax.y, vert.position.y * GetScale().y);
		myAABB.myMax.z = max(myAABB.myMax.z, vert.position.z * GetScale().z);

		myAABB.myMin.x = min(myAABB.myMin.x, vert.position.x * GetScale().x);
		myAABB.myMin.y = min(myAABB.myMin.y, vert.position.y * GetScale().y);
		myAABB.myMin.z = min(myAABB.myMin.z, vert.position.z * GetScale().z);
	}
}


FPrimitiveBoxMultiTex::FPrimitiveBoxMultiTex(FD3d12Renderer* aRenderer, FVector3 aPos, FVector3 aScale, FPrimitiveBoxInstanced::PrimitiveType aType, int aNrOfInstances)
	: FPrimitiveBoxInstanced(aRenderer, aPos, aScale, aType, aNrOfInstances)
{
}


FPrimitiveBoxMultiTex::~FPrimitiveBoxMultiTex()
{
}
