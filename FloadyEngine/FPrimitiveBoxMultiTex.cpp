#include "FPrimitiveBoxMultiTex.h"
#include "FVector3.h"
#include "d3dx12.h"
#include "FD3d12Renderer.h"
#include <vector>

#include "FDelegate.h"
#include "FJobSystem.h"
#include "FTextureManager.h"
#include "FLightManager.h"
#include "FPrimitiveGeometry.h"
#include "FProfiler.h"

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

		// get root sig with 32 SRV's
		m_rootSignature = FD3d12Renderer::GetInstance()->GetRootSignature(96, 1);
		m_rootSignature->SetName(L"PrimitiveBoxMultiTex");

		//*
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

		ID3DBlob* signature;
		ID3DBlob* error;

		// shadow root sig
		CD3DX12_DESCRIPTOR_RANGE1 rangesShadows[1];
		rangesShadows[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

		CD3DX12_ROOT_PARAMETER1 rootParametersShadow[2];
		rootParametersShadow[0].InitAsDescriptorTable(1, &rangesShadows[0], D3D12_SHADER_VISIBILITY_ALL);
		rootParametersShadow[1].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC  rootSignatureDescShadows;
		rootSignatureDescShadows.Init_1_1(_countof(rootParametersShadow), rootParametersShadow, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ID3DBlob* signatureShadows;
		hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDescShadows, featureData.HighestVersion, &signatureShadows, &error);
		hr = myManagerClass->GetDevice()->CreateRootSignature(0, signatureShadows->GetBufferPointer(), signatureShadows->GetBufferSize(), IID_PPV_ARGS(&m_rootSignatureShadows));
		m_rootSignatureShadows->SetName(L"PrimitiveBoxInstancedShadow");
		/*/
		m_rootSignatureShadows = FD3d12Renderer::GetInstance()->GetRootSignature(0, 1);
		m_rootSignatureShadows->SetName(L"PrimitiveBoxMultiTexShadows");

		//*/
	}

	// Create vertex + index buffer

	// create constant buffer for modelview

	const int buff_size = sizeof(float) * (myNrOfInstances + 1) * 16;
	myHeapOffsetCBVShadow = myManagerClass->CreateConstantBuffer(m_ModelProjMatrixShadow, myConstantBufferShadowsPtr, sizeof(float) * 32 * 16);
	myManagerClass->CreateConstantBuffer(myShadowPerInstanceData, myShadowPerInstanceDataPtr, sizeof(PerDrawCallData) * 16); // nrOfDrawcalls or nrOfLights TODO
	myHeapOffsetCBV = myManagerClass->CreateConstantBuffer(m_ModelProjMatrix, myConstantBufferPtr, buff_size);
	m_ModelProjMatrix->SetName(L"PrimitiveBoxInstancedConst");
	m_ModelProjMatrixShadow->SetName(L"PrimitiveBoxInstancedConstShadows");
	myShadowPerInstanceData->SetName(L"PrimitiveBoxMultiTexConstShadowsInstanceData");
	myHeapOffsetAll = myHeapOffsetCBV;
	myHeapOffsetText = myHeapOffsetAll;

	memset(myConstantBufferShadowsPtr, 0, sizeof(float) * 32 * 16);
	memset(myShadowPerInstanceDataPtr, 0, sizeof(PerDrawCallData) * 16);

	skipNextRender = false;
	myIsInitialized = true;

	RecalcModelMatrix();

	// set to multitex shader
	SetShader("primitiveshader_deferredMultiTex.hlsl");
	SetShader();
	myManagerClass->GetShaderManager().RegisterForHotReload(shaderfilename, this, FDelegate2<void()>::from<FPrimitiveBoxInstanced, &FPrimitiveBoxInstanced::SetShader>(this));
	
	myTexOffset = FD3d12Renderer::GetInstance()->BindTexture("");
	for (size_t i = 0; i < 95; i++) // 63 + 1 = 64 :^)
	{
		FD3d12Renderer::GetInstance()->BindTexture("");
	}
}

void FPrimitiveBoxMultiTex::ObjectLoadingDone(const FMeshManager::FMeshLoadObject& anObj)
{
	FLOG("ObjectLoadingDone called");

	// update GPU buffers
	myIndicesCount = anObj.myObject->myIndicesCount;
	m_vertexBufferView = anObj.myObject->myVertexBufferView;
	m_indexBufferView = anObj.myObject->myIndexBufferView;
	

	// map all textures to the slots	
	std::string name;
	for (size_t i = 0; i < anObj.myModel.myMaterials.size(); i++)
	{
		const F3DModel::FMaterial& mat = anObj.myModel.myMaterials[i];

		if (!mat.myDiffuseTexture.empty())
		{
			name = mat.myDiffuseTexture.substr(mat.myDiffuseTexture.find('\\') + 1, mat.myDiffuseTexture.length());
		}

		FD3d12Renderer::GetInstance()->BindTextureToSlot(name, myTexOffset + i);
	}

	for (size_t i = 0; i < anObj.myModel.myMaterials.size(); i++)
	{
		const F3DModel::FMaterial& mat = anObj.myModel.myMaterials[i];

		if (!mat.myNormalTexture.empty())
		{
			name = mat.myNormalTexture.substr(mat.myNormalTexture.find('\\') + 1, mat.myNormalTexture.length());
		}

		FD3d12Renderer::GetInstance()->BindTextureToSlot(name, myTexOffset + i + 32);
	}

	for (size_t i = 0; i < anObj.myModel.myMaterials.size(); i++)
	{
		const F3DModel::FMaterial& mat = anObj.myModel.myMaterials[i];

		if (!mat.mySpecularTexture.empty())
		{
			name = mat.mySpecularTexture.substr(mat.mySpecularTexture.find('\\') + 1, mat.mySpecularTexture.length());
		}

		FD3d12Renderer::GetInstance()->BindTextureToSlot(name, myTexOffset + i + 64);
	}

	// update AABB with latest vertex info
	for (const FPrimitiveGeometry::Vertex2& vert : anObj.myObject->myVertices)
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
