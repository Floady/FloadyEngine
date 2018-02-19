#include "FPrimitiveBox.h"
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

using namespace DirectX;
#define DEFERRED 1

FPrimitiveBox::FPrimitiveBox(FD3d12Renderer* aManager, FVector3 aPos, FVector3 aScale, FPrimitiveBox::PrimitiveType aType)
{
#if DEFERRED
	shaderfilename = "primitiveshader_deferred.hlsl";
#else
	shaderfilename = "primitiveshader.hlsl";
#endif
	shaderfilenameShadow = "primitiveshader_deferredShadows.hlsl";

	myTexName = "testtexture2.png"; // something default

	myManagerClass = aManager;
	myYaw = 0.0f; // test
	m_ModelProjMatrix = nullptr;
	m_ModelProjMatrixShadow = nullptr;
	myIsMatrixDirty = true;
	myIsGPUConstantDataDirty = true;

	myPos.x = aPos.x;
	myPos.y = aPos.y;
	myPos.z = aPos.z;

	myScale = aScale;

	m_pipelineState = nullptr;
	m_pipelineStateShadows = nullptr;
	m_commandList = nullptr;
	
	myConstantBufferPtr = nullptr;

	myMesh = nullptr;

	myType = aType;

	XMMATRIX mtxRot = XMMatrixIdentity();
	XMFLOAT4X4 m;
	XMStoreFloat4x4(&m, mtxRot);

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			myRotMatrix[i * 4 + j] = m.m[i][j];
		}
	}

	myIsInitialized = false;

	{
		if (myType == PrimitiveType::Sphere)
		{
			myIndicesCount = FPrimitiveGeometry::Box2::GetIndices().size();
			m_vertexBufferView.BufferLocation = FPrimitiveGeometry::Box2::GetVerticesBuffer()->GetGPUVirtualAddress();
			m_vertexBufferView.StrideInBytes = FPrimitiveGeometry::Box2::GetVerticesBufferStride();
			m_vertexBufferView.SizeInBytes = FPrimitiveGeometry::Box2::GetVertexBufferSize();

			m_indexBufferView.BufferLocation = FPrimitiveGeometry::Box2::GetIndicesBuffer()->GetGPUVirtualAddress();
			m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;  // get from primitive manager
			m_indexBufferView.SizeInBytes = FPrimitiveGeometry::Box2::GetIndicesBufferSize();
		}
		else if (myType == PrimitiveType::Box)
		{
			myIndicesCount = FPrimitiveGeometry::Box::GetIndices().size();
			m_vertexBufferView.BufferLocation = FPrimitiveGeometry::Box::GetVerticesBuffer()->GetGPUVirtualAddress();
			m_vertexBufferView.StrideInBytes = FPrimitiveGeometry::Box::GetVerticesBufferStride();
			m_vertexBufferView.SizeInBytes = FPrimitiveGeometry::Box::GetVertexBufferSize();

			m_indexBufferView.BufferLocation = FPrimitiveGeometry::Box::GetIndicesBuffer()->GetGPUVirtualAddress();
			m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;  // get from primitive manager
			m_indexBufferView.SizeInBytes = FPrimitiveGeometry::Box::GetIndicesBufferSize();
		}
	}

	// setup AABB
	std::vector<FPrimitiveGeometry::Vertex>& verts = FPrimitiveGeometry::Box2::GetVertices();
	for (FPrimitiveGeometry::Vertex& vert  : verts)
	{
		myAABB.myMax.x = max(myAABB.myMax.x, vert.position.x * GetScale().x);
		myAABB.myMax.y = max(myAABB.myMax.y, vert.position.y * GetScale().y);
		myAABB.myMax.z = max(myAABB.myMax.z, vert.position.z * GetScale().z);

		myAABB.myMin.x = min(myAABB.myMin.x, vert.position.x * GetScale().x);
		myAABB.myMin.y = min(myAABB.myMin.y, vert.position.y * GetScale().y);
		myAABB.myMin.z = min(myAABB.myMin.z, vert.position.z * GetScale().z);
	}
}

FPrimitiveBox::~FPrimitiveBox()
{
	myManagerClass->GetShaderManager().UnregisterForHotReload(this);
}

void FPrimitiveBox::Init()
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
		m_rootSignature->SetName(L"PrimitiveBox");

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
		m_rootSignatureShadows->SetName(L"PrimitiveBoxShadow");
	}


	SetShader();
	myManagerClass->GetShaderManager().RegisterForHotReload(shaderfilename, this, FDelegate2<void()>::from<FPrimitiveBox, &FPrimitiveBox::SetShader>(this));

	// Create vertex + index buffer

	// create constant buffer for modelview

	myHeapOffsetCBVShadow = myManagerClass->CreateConstantBuffer(m_ModelProjMatrixShadow, myConstantBufferShadowsPtr, sizeof(float) * 32 * 16 );
	myHeapOffsetCBV = myManagerClass->CreateConstantBuffer(m_ModelProjMatrix, myConstantBufferPtr);
	m_ModelProjMatrix->SetName(L"PrimitiveBoxConst");
	m_ModelProjMatrixShadow->SetName(L"PrimitiveBoxConstShadows");
	myHeapOffsetAll = myHeapOffsetCBV;
	myHeapOffsetText = myHeapOffsetAll;

	memset(myConstantBufferShadowsPtr, 0, sizeof(float) * 32 * 16);

	// create SRV for texture
	{		
		myManagerClass->BindTexture(myTexName);
	}
	
	skipNextRender = false;
	myIsInitialized = true;

	RecalcModelMatrix();
}

void FPrimitiveBox::Render()
{
	if (skipNextRender)
	{
		skipNextRender = false;
		return;
	}

	HRESULT hr;
	hr = m_commandList->Reset(myManagerClass->GetCommandAllocator(), m_pipelineState);
	PopulateCommandListInternal(m_commandList);
	hr = m_commandList->Close();

	ID3D12CommandList* ppCommandLists[] = { m_commandList };
	myManagerClass->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// wait for cmdlist to be done before returning
	ID3D12Fence* m_fence;
	HANDLE m_fenceEvent;
	m_fenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
	int fenceToWaitFor = 1; // what value?
	HRESULT result = myManagerClass->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_fence);
	result = myManagerClass->GetCommandQueue()->Signal(m_fence, fenceToWaitFor);
	m_fence->SetEventOnCompletion(1, m_fenceEvent);
	WaitForSingleObject(m_fenceEvent, INFINITE);
	m_fence->Release();
}

void FPrimitiveBox::RenderShadows()
{
	if (skipNextRender)
	{
		return;
	}

	HRESULT hr;
	hr = m_commandList->Reset(myManagerClass->GetCommandAllocator(), m_pipelineStateShadows);
	PopulateCommandListInternalShadows(m_commandList);
	hr = m_commandList->Close();

	ID3D12CommandList* ppCommandLists[] = { m_commandList };
	myManagerClass->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// wait for cmdlist to be done before returning
	ID3D12Fence* m_fence;
	HANDLE m_fenceEvent;
	m_fenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
	int fenceToWaitFor = 1; // what value?
	HRESULT result = myManagerClass->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_fence);
	result = myManagerClass->GetCommandQueue()->Signal(m_fence, fenceToWaitFor);
	m_fence->SetEventOnCompletion(1, m_fenceEvent);
	WaitForSingleObject(m_fenceEvent, INFINITE);
	m_fence->Release();
}

void FPrimitiveBox::PopulateCommandListAsync()
{
	ID3D12GraphicsCommandList* cmdList = myManagerClass->GetCommandListForWorkerThread(FJobSystem::ourThreadIdx);
	PopulateCommandListInternal(cmdList);
}

void FPrimitiveBox::PopulateCommandListAsyncShadows()
{
	ID3D12GraphicsCommandList* cmdList = myManagerClass->GetCommandListForWorkerThread(FJobSystem::ourThreadIdx);
	PopulateCommandListInternalShadows(cmdList);
}

void FPrimitiveBox::PopulateCommandListInternal(ID3D12GraphicsCommandList* aCmdList)
{
	if (skipNextRender)
	{
		skipNextRender = false;
		return;
	}

	if (!myIsInitialized)
		return;

	aCmdList->SetPipelineState(m_pipelineState);
	aCmdList->SetGraphicsRootSignature(m_rootSignature);

	// Get the size of the memory location for the render target view descriptors.
	unsigned int srvSize = myManagerClass->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_GPU_DESCRIPTOR_HANDLE handle = myManagerClass->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += srvSize*myHeapOffsetAll;
	aCmdList->SetGraphicsRootDescriptorTable(0, handle);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeap = myManagerClass->GetDSVHandle();
	
#if DEFERRED
#if !THE_NEW_WAY
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = { myManagerClass->GetGBufferHandle(0), myManagerClass->GetGBufferHandle(1), myManagerClass->GetGBufferHandle(2) , myManagerClass->GetGBufferHandle(3) };
	aCmdList->OMSetRenderTargets(4, rtvHandles, FALSE, &dsvHeap);
	aCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
#endif
#else
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = myManagerClass->GetRTVHandle();
	aCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHeap);
#endif

	if (myMesh)
	{
		aCmdList->IASetVertexBuffers(0, 1, &myMesh->myVertexBufferView);
		aCmdList->IASetIndexBuffer(&myMesh->myIndexBufferView);
		aCmdList->DrawIndexedInstanced(myMesh->myIndicesCount, 1, 0, 0, 0);
	}
	else
	{
		aCmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		aCmdList->IASetIndexBuffer(&m_indexBufferView);
		aCmdList->DrawIndexedInstanced(myIndicesCount, 1, 0, 0, 0);
	}

}

void FPrimitiveBox::PopulateCommandListInternalShadows(ID3D12GraphicsCommandList* aCmdList)
{
	//FPROFILE_FUNCTION("Box Shadow");
	if (!m_pipelineStateShadows)
		return;

	// set pipeline  to shadow shaders
	aCmdList->SetPipelineState(m_pipelineStateShadows);
	aCmdList->SetGraphicsRootSignature(m_rootSignatureShadows);

#if !THE_NEW_WAY
	aCmdList->RSSetViewports(1, &myManagerClass->GetViewPort());
	aCmdList->RSSetScissorRects(1, &myManagerClass->GetScissorRect());
	// is this how we bind textures?
	ID3D12DescriptorHeap* ppHeaps[] = { myManagerClass->GetSRVHeap() };
	aCmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
#endif

	// Get the size of the memory location for the render target view descriptors.
	unsigned int srvSize = myManagerClass->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_GPU_DESCRIPTOR_HANDLE handle = myManagerClass->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += srvSize*myHeapOffsetCBVShadow;
	aCmdList->SetGraphicsRootDescriptorTable(0, handle);

#if !THE_NEW_WAY
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeap = myManagerClass->GetShadowMapHandle(FLightManager::GetInstance()->GetActiveLight());
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = myManagerClass->GetGBufferHandle(FD3d12Renderer::GbufferType::Gbuffer_color);
	aCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHeap);
	aCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
#endif

	if (myMesh)
	{
		aCmdList->IASetVertexBuffers(0, 1, &myMesh->myVertexBufferView);
		aCmdList->IASetIndexBuffer(&myMesh->myIndexBufferView);
		aCmdList->DrawIndexedInstanced(myMesh->myIndicesCount, 1, 0, 0, 0);
	}
	else
	{
		aCmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		aCmdList->IASetIndexBuffer(&m_indexBufferView);
		aCmdList->DrawIndexedInstanced(myIndicesCount, 1, 0, 0, 0);
	}
}

#define DEPTH_BIAS_D32_FLOAT(d) (d/(1/pow(2,23)))
void FPrimitiveBox::SetShader()
{	
	skipNextRender = true;

	// get shader ptr + layouts
	FShaderManager::FShader shader = myManagerClass->GetShaderManager().GetShader(shaderfilename);
	FShaderManager::FShader shaderShadows = myManagerClass->GetShaderManager().GetShader(shaderfilenameShadow);

	if (m_pipelineState)
		m_pipelineState->Release();

	if (m_pipelineStateShadows)
		m_pipelineStateShadows->Release();

	if (m_commandList)
		m_commandList->Release();

	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { &shader.myInputElementDescs[0], (UINT)shader.myInputElementDescs.size() };
	psoDesc.pRootSignature = m_rootSignature;
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(shader.myVertexShader);
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(shader.myPixelShader);
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FrontCounterClockwise = myRenderCCW;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	//psoDesc.BlendState.DepthWriteMask
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	//psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	if(myRenderCCW)
		psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
#if DEFERRED
	psoDesc.NumRenderTargets = myManagerClass->Gbuffer_Combined;
	for (size_t i = 0; i < myManagerClass->Gbuffer_Combined; i++)
	{
		psoDesc.RTVFormats[i] = myManagerClass->gbufferFormat[i];
	}
#else
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
#endif
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;

	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescShadows = {};
	psoDescShadows.InputLayout = { &shaderShadows.myInputElementDescs[0], (UINT)shaderShadows.myInputElementDescs.size() };
	psoDescShadows.pRootSignature = m_rootSignatureShadows;
	psoDescShadows.VS = CD3DX12_SHADER_BYTECODE(shaderShadows.myVertexShader);
	psoDescShadows.PS = CD3DX12_SHADER_BYTECODE(shaderShadows.myPixelShader);
	psoDescShadows.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDescShadows.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	//psoDescShadows.RasterizerState.DepthBias = DEPTH_BIAS_D32_FLOAT(-0.006);
	psoDescShadows.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDescShadows.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDescShadows.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	psoDescShadows.SampleMask = UINT_MAX;
	psoDescShadows.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDescShadows.NumRenderTargets = 1;
	psoDescShadows.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDescShadows.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDescShadows.SampleDesc.Count = 1;

	HRESULT hr = myManagerClass->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
	hr = myManagerClass->GetDevice()->CreateGraphicsPipelineState(&psoDescShadows, IID_PPV_ARGS(&m_pipelineStateShadows));
	
	hr = myManagerClass->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, myManagerClass->GetCommandAllocator(), m_pipelineState, IID_PPV_ARGS(&m_commandList));
	m_commandList->Close();
	m_commandList->SetName(L"PrimitiveBox");
}

void FPrimitiveBox::RecalcModelMatrix()
{
	if (myIsMatrixDirty)
	{
		XMFLOAT4X4 m = XMFLOAT4X4(myRotMatrix);
		XMMATRIX mtxRot = XMLoadFloat4x4(&m);
		XMMATRIX scale = XMMatrixScaling(myScale.x, myScale.y, myScale.z);
		XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(myPos.x, myPos.y, myPos.z, 1));
		offset = scale * mtxRot * offset;

		offset = XMMatrixTranspose(offset);
		XMStoreFloat4x4(&myModelMatrix, offset);
		myIsMatrixDirty = false;
		myIsGPUConstantDataDirty = true;
	}
}

void FPrimitiveBox::SetRotMatrix(float * m)
{
	bool bSame = true;
	for (int i = 0; i < 4 && bSame; i++)
	{
		for (int j = 0; j < 4 && bSame; j++)
		{
			if (m[i * 4 + j] != myRotMatrix[i * 4 + j])
				bSame = false;
		}
	}

	if (bSame)
		return;

	myIsMatrixDirty = true;
	FRenderableObject::SetRotMatrix(m);
	memcpy(myRotMatrix, m, sizeof(float) * 16);
}

extern bool ourShouldRecalc;
void FPrimitiveBox::UpdateConstBuffers()
{
	// copy modelviewproj data to gpu
	// set on gpu
	if (myIsGPUConstantDataDirty || ourShouldRecalc)
	{
		float constData[32];
		memcpy(&constData[0], myManagerClass->GetCamera()->GetViewProjMatrixWithOffset(0, 0, 0).m, sizeof(XMFLOAT4X4));
		memcpy(&constData[16], myModelMatrix.m, sizeof(XMFLOAT4X4));
		memcpy(myConstantBufferPtr, &constData[0], sizeof(float) * 32);
	}
	else
	{
		float constData[16];
		memcpy(&constData[0], myManagerClass->GetCamera()->GetViewProjMatrixWithOffset(0, 0, 0).m, sizeof(XMFLOAT4X4));
		memcpy(myConstantBufferPtr, &constData[0], sizeof(float) * 16);
	}

	float constData[32 * 16];
	int offsetInConstData = 0;
	const std::vector<FLightManager::DirectionalLight>& dirLights = FLightManager::GetInstance()->GetDirectionalLights();
	for (size_t i = 0; i < dirLights.size(); i++)
	{
		const XMFLOAT4X4& m = FLightManager::GetInstance()->GetDirectionalLightViewProjMatrix(i);
		memcpy(&constData[offsetInConstData], m.m, sizeof(XMFLOAT4X4));
		offsetInConstData += 16;
	}

	const std::vector<FLightManager::SpotLight>& spotlights = FLightManager::GetInstance()->GetSpotlights();
	for (size_t i = 0; i < spotlights.size(); i++)
	{
		const XMFLOAT4X4& m = FLightManager::GetInstance()->GetSpotlightViewProjMatrix(i);
		memcpy(&constData[offsetInConstData], m.m, sizeof(XMFLOAT4X4));
		offsetInConstData += 16;
	}

	offsetInConstData = 16 * 16; // jump to light matrix offsets

	if (myIsGPUConstantDataDirty || ourShouldRecalc)
	{
		// copy modelviewproj data to gpu
		//XMMATRIX mtxRot = XMMatrixRotationRollPitchYaw(0, myYaw, 0);
		//XMFLOAT4X4 m = XMFLOAT4X4(myRotMatrix);
		//XMMATRIX mtxRot = XMLoadFloat4x4(&m);
		//XMMATRIX scale = XMMatrixScaling(myScale.x, myScale.y, myScale.z);
		//XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(myPos.x, myPos.y, myPos.z, 1));
		//offset = scale * mtxRot * offset;

		//XMFLOAT4X4 ret;
		//offset = XMMatrixTranspose(offset);
		//XMStoreFloat4x4(&ret, offset);

		//memcpy(&constData[offsetInConstData], ret.m, sizeof(XMFLOAT4X4));
	
		memcpy(&constData[offsetInConstData], myModelMatrix.m, sizeof(XMFLOAT4X4));
		offsetInConstData += 16;
	}

	//memcpy(&constData[16], FLightManager::GetInstance()->GetCurrentActiveLightViewProjMatrix().m, sizeof(XMFLOAT4X4));
	//memcpy(&constData[16], myModelMatrix.m, sizeof(XMFLOAT4X4)); // we need rotation too..

	if(myIsGPUConstantDataDirty || ourShouldRecalc)
		memcpy(myConstantBufferShadowsPtr, &constData[0], sizeof(float) * (offsetInConstData));
	else
		memcpy(myConstantBufferShadowsPtr + sizeof(float) * 16, &constData[1], sizeof(float) * (offsetInConstData - 16));

	myIsGPUConstantDataDirty = false;
}

void FPrimitiveBox::SetTexture(const char * aFilename)
{
	myTexName = aFilename;
}

void FPrimitiveBox::SetShader(const char * aFilename)
{
	shaderfilename = aFilename;
}

void FPrimitiveBox::SetPos(const FVector3& aPos)
{
	if (aPos.x == myPos.x && aPos.y == myPos.y && aPos.z == myPos.z)
		return;

	FRenderableObject::SetPos(aPos);
	myIsMatrixDirty = true;
}
