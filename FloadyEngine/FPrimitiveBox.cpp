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
	
	myPos.x = aPos.x;
	myPos.y = aPos.y;
	myPos.z = aPos.z;

	myScale = aScale;

	m_pipelineState = nullptr;
	m_pipelineStateShadows = nullptr;
	m_commandList = nullptr;

	myType = aType;

	myRotMatrix = XMMatrixIdentity();
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
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
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
	{
		myHeapOffsetCBV = myManagerClass->CreateConstantBuffer(m_ModelProjMatrix, myConstantBufferPtr);
		myHeapOffsetAll = myHeapOffsetCBV;
		myHeapOffsetText = myManagerClass->GetNextOffset();
		myHeapOffsetCBVShadow = myManagerClass->CreateConstantBuffer(m_ModelProjMatrixShadow, myConstantBufferShadowsPtr);
	}
	
	// create SRV for texture
	{		
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; 
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		unsigned int srvSize = myManagerClass->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle0(myManagerClass->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), myHeapOffsetText, srvSize);
		myManagerClass->GetDevice()->CreateShaderResourceView(FTextureManager::GetInstance()->GetTextureD3D(myTexName.c_str()), &srvDesc, srvHandle0);
	}
	
	skipNextRender = false;
	myIsInitialized = true;
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
	ID3D12CommandAllocator* cmdAllocator = myManagerClass->GetCommandAllocatorForWorkerThread(FJobSystem::ourThreadIdx);
	PopulateCommandListInternal(cmdList);
}

void FPrimitiveBox::PopulateCommandListAsyncShadows()
{
	ID3D12GraphicsCommandList* cmdList = myManagerClass->GetCommandListForWorkerThread(FJobSystem::ourThreadIdx);
	ID3D12CommandAllocator* cmdAllocator = myManagerClass->GetCommandAllocatorForWorkerThread(FJobSystem::ourThreadIdx);
	PopulateCommandListInternalShadows(cmdList);
}

void FPrimitiveBox::PopulateCommandListInternal(ID3D12GraphicsCommandList* aCmdList)
{
	aCmdList->SetPipelineState(m_pipelineState);

	// copy modelviewproj data to gpu
	XMMATRIX mtxRot = myRotMatrix;
	XMMATRIX scale = XMMatrixScaling(myScale.x, myScale.y, myScale.z);
	XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(myPos.x, myPos.y, myPos.z, 1));
	offset = scale * mtxRot * offset;

	XMFLOAT4X4 ret;
	offset = XMMatrixTranspose(offset);

	XMStoreFloat4x4(&ret, offset);

	float constData[32];
	memcpy(&constData[0], myManagerClass->GetCamera()->GetViewProjMatrixWithOffset(0,0,0).m, sizeof(XMFLOAT4X4));
	memcpy(&constData[16], ret.m, sizeof(XMFLOAT4X4));
	memcpy(myConstantBufferPtr, &constData[0], sizeof(float) * 32);

#if DEFERRED
	for (size_t i = 0; i < FD3d12Renderer::GbufferType::Gbuffer_Combined; i++)
	{
		aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetGBufferTarget(i), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
	}
#endif
	aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetDepthBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// Set necessary state.
	aCmdList->SetGraphicsRootSignature(m_rootSignature);

	// is this how we bind textures?
	ID3D12DescriptorHeap* ppHeaps[] = { myManagerClass->GetSRVHeap() };
	aCmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// Get the size of the memory location for the render target view descriptors.
	unsigned int srvSize = myManagerClass->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_GPU_DESCRIPTOR_HANDLE handle = myManagerClass->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += srvSize*myHeapOffsetAll;
	aCmdList->SetGraphicsRootDescriptorTable(0, handle);

	// set viewport/scissor
	aCmdList->RSSetViewports(1, &myManagerClass->GetViewPort());
	aCmdList->RSSetScissorRects(1, &myManagerClass->GetScissorRect());

	// Indicate that the back buffer will be used as a render target.
	aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeap = myManagerClass->GetDSVHandle();
	
#if DEFERRED
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = { myManagerClass->GetGBufferHandle(0), myManagerClass->GetGBufferHandle(1), myManagerClass->GetGBufferHandle(2) , myManagerClass->GetGBufferHandle(3) };
	aCmdList->OMSetRenderTargets(4, rtvHandles, FALSE, &dsvHeap);
#else
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = myManagerClass->GetRTVHandle();
	aCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHeap);
#endif

	// Record commands.
	aCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	aCmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	aCmdList->IASetIndexBuffer(&m_indexBufferView);
	aCmdList->DrawIndexedInstanced(myIndicesCount, 1, 0, 0, 0);

	// Indicate that the back buffer will now be used to present.
	aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetDepthBuffer(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
#if DEFERRED
	for (size_t i = 0; i < FD3d12Renderer::GbufferType::Gbuffer_Combined; i++)
	{
		aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetGBufferTarget(i), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}
#endif
}

void FPrimitiveBox::PopulateCommandListInternalShadows(ID3D12GraphicsCommandList* aCmdList)
{
	FPROFILE_FUNCTION("Box Shadow");
	// copy modelviewproj data to gpu
	XMMATRIX mtxRot = XMMatrixRotationRollPitchYaw(0, myYaw, 0);
	XMMATRIX scale = XMMatrixScaling(myScale.x, myScale.y, myScale.z);
	XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(myPos.x, myPos.y, myPos.z, 1));
	offset = scale * mtxRot * offset;

	XMFLOAT4X4 ret;
	offset = XMMatrixTranspose(offset);
	XMStoreFloat4x4(&ret, offset);

	float constData[32];
	memcpy(&constData[0], FLightManager::GetInstance()->GetCurrentActiveLightViewProjMatrix().m, sizeof(XMFLOAT4X4));
	memcpy(&constData[16], ret.m, sizeof(XMFLOAT4X4));
	memcpy(myConstantBufferShadowsPtr, &constData[0], sizeof(float) * 32);
	
	// set pipeline  to shadow shaders
	aCmdList->SetPipelineState(m_pipelineStateShadows);

	aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetShadowMapBuffer(FLightManager::GetInstance()->GetActiveLight()), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// Set necessary state.
	aCmdList->SetGraphicsRootSignature(m_rootSignatureShadows);

	// is this how we bind textures?
	ID3D12DescriptorHeap* ppHeaps[] = { myManagerClass->GetSRVHeap() };
	aCmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// Get the size of the memory location for the render target view descriptors.
	unsigned int srvSize = myManagerClass->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_GPU_DESCRIPTOR_HANDLE handle = myManagerClass->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += srvSize*myHeapOffsetCBVShadow;
	aCmdList->SetGraphicsRootDescriptorTable(0, handle);

	// set viewport/scissor
	aCmdList->RSSetViewports(1, &myManagerClass->GetViewPort());
	aCmdList->RSSetScissorRects(1, &myManagerClass->GetScissorRect());

	// Indicate that the back buffer will be used as a render target.
	aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetGBufferTarget(FD3d12Renderer::GbufferType::Gbuffer_color), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeap = myManagerClass->GetShadowMapHandle(FLightManager::GetInstance()->GetActiveLight());

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = myManagerClass->GetGBufferHandle(FD3d12Renderer::GbufferType::Gbuffer_color);
	aCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHeap);

	// Record commands.
	aCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	aCmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	aCmdList->IASetIndexBuffer(&m_indexBufferView);
	aCmdList->DrawIndexedInstanced(myIndicesCount, 1, 0, 0, 0);

	float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	aCmdList->ClearRenderTargetView(myManagerClass->GetGBufferHandle(FD3d12Renderer::GbufferType::Gbuffer_color), color, 0, NULL);

	// Indicate that the back buffer will now be used to present.
	aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetGBufferTarget(FD3d12Renderer::GbufferType::Gbuffer_color), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetShadowMapBuffer(FLightManager::GetInstance()->GetActiveLight()), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
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
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
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

void FPrimitiveBox::SetTexture(const char * aFilename)
{
	/*skipNextRender = true;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	unsigned int srvSize = myManagerClass->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle0(myManagerClass->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), myHeapOffsetText, srvSize);
	myManagerClass->GetDevice()->CreateShaderResourceView(FTextureManager::GetInstance()->GetTextureD3D(aFilename), &srvDesc, srvHandle0);
*/
	myTexName = aFilename;
}

void FPrimitiveBox::SetShader(const char * aFilename)
{
	shaderfilename = aFilename;
}
