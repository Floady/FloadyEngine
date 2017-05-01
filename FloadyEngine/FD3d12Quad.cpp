#include "FD3d12Quad.h"
#include "d3dx12.h"
#include "D3dCompiler.h"
#include "FD3d12Renderer.h"
#include "FShaderManager.h"
#include "FCamera.h"
#include "FVector3.h"
#include <vector>
#include <DirectXMath.h>
#include "FLightManager.h"

static const UINT TextureWidth = 256;
static const UINT TextureHeight = 256;
static const UINT TexturePixelSize = 4;	// The number of bytes used to represent a pixel in the texture.
using namespace DirectX;
FD3d12Quad::FD3d12Quad(FD3d12Renderer* aManager, FVector3 aPos)
{
	myManagerClass = aManager;
	int tmp = aManager->GetNextOffset();
}

FD3d12Quad::~FD3d12Quad()
{
}

void FD3d12Quad::Init()
{
	firstFrame = true;
	skipNextRender = false;
	m_pipelineState = nullptr;
	m_commandList = nullptr;

	m_device = myManagerClass->GetDevice();

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
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		
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
		m_rootSignature->SetName(L"FD3d12Quad Root");
	}

		// get shader ptr + layouts

	SetShader();
	myManagerClass->GetShaderManager().RegisterForHotReload("lightshader.hlsl", this, FDelegate2<void()>::from<FD3d12Quad, &FD3d12Quad::SetShader>(this));


	// Create the vertex buffer.
	{
		m_aspectRatio = 1.0f;
		const float size = 1.0f;
		const float z = 0.0f;
		Vertex triangleVertices[] =
		{
			{ { -size, size, z },{ 0.0f, 0.0f } },
			{ { size, -size, z },{ 1.0f, 1.0f } },
			{ { -size, -size,z },{ 0.0f, 1.0f } },
					   
			{ { -size, size, z },{ 0.0f, 0.0f } },
			{ { size, size, z},{ 1.0f, 0.0f } },
			{ { size, -size, z },{ 1.0f, 1.0f } }
		};


		const UINT vertexBufferSize = sizeof(triangleVertices);

		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.
		hr = m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		hr = m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
		memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
		m_vertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(Vertex);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;
		
		m_commandList->Close();
	}

	myHeapOffsetCBV = myManagerClass->GetNextOffset();
	unsigned int srvSize = myManagerClass->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// create continuous handles to gbuffer for shader
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	int myHeapOffsetBuffers = myHeapOffsetCBV;
	srvDesc.Format = myManagerClass->gbufferFormat[FD3d12Renderer::GbufferType::Gbuffer_color];
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle0(myManagerClass->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), myHeapOffsetBuffers, srvSize);
	myManagerClass->GetDevice()->CreateShaderResourceView(myManagerClass->GetGBufferTarget(FD3d12Renderer::GbufferType::Gbuffer_color), &srvDesc, srvHandle0);

	myHeapOffsetBuffers = myManagerClass->GetNextOffset();
	srvDesc.Format = myManagerClass->gbufferFormat[FD3d12Renderer::GbufferType::Gbuffer_normals];
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle1(myManagerClass->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), myHeapOffsetBuffers, srvSize);
	myManagerClass->GetDevice()->CreateShaderResourceView(myManagerClass->GetGBufferTarget(FD3d12Renderer::GbufferType::Gbuffer_normals), &srvDesc, srvHandle1);

	myHeapOffsetBuffers = myManagerClass->GetNextOffset();
	srvDesc.Format = myManagerClass->gbufferFormat[FD3d12Renderer::GbufferType::Gbuffer_Depth];
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle2(myManagerClass->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), myHeapOffsetBuffers, srvSize);
	myManagerClass->GetDevice()->CreateShaderResourceView(myManagerClass->GetGBufferTarget(FD3d12Renderer::GbufferType::Gbuffer_Depth), &srvDesc, srvHandle2);

	myHeapOffsetBuffers = myManagerClass->GetNextOffset();
	srvDesc.Format = myManagerClass->gbufferFormat[FD3d12Renderer::GbufferType::Gbuffer_Shadow];
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle3(myManagerClass->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), myHeapOffsetBuffers, srvSize);
	myManagerClass->GetDevice()->CreateShaderResourceView(myManagerClass->GetShadowMapBuffer(), &srvDesc, srvHandle3);

	// buffer for invproj + 3 lights (4x4 float)
	{
		hr = myManagerClass->GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(256),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&myConstDataShader));

		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		hr = myConstDataShader->Map(0, &readRange, reinterpret_cast<void**>(&myConstBufferShaderPtr));

		// Describe and create a constant buffer view.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc[1] = {};
		cbvDesc[0].BufferLocation = myConstDataShader->GetGPUVirtualAddress();
		cbvDesc[0].SizeInBytes = 256; // required to be 256 bytes aligned -> (sizeof(ConstantBuffer) + 255) & ~255
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle0(myManagerClass->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), myManagerClass->GetNextOffset(), srvSize);
		myManagerClass->GetDevice()->CreateConstantBufferView(cbvDesc, cbvHandle0);
	}

	// buffer for invproj + 3 lights (4x4 float)
	{
		hr = myManagerClass->GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(256),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&myInvProjData));

		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		hr = myInvProjData->Map(0, &readRange, reinterpret_cast<void**>(&myInvProjDataShaderPtr));

		// Describe and create a constant buffer view.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc[1] = {};
		cbvDesc[0].BufferLocation = myInvProjData->GetGPUVirtualAddress();
		cbvDesc[0].SizeInBytes = 256; // required to be 256 bytes aligned -> (sizeof(ConstantBuffer) + 255) & ~255
		unsigned int srvSize = myManagerClass->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		int nextOffset = myManagerClass->GetNextOffset();
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle0(myManagerClass->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), nextOffset, srvSize);
		myManagerClass->GetDevice()->CreateConstantBufferView(cbvDesc, cbvHandle0);
	}
}

void FD3d12Quad::Render()
{

	if (skipNextRender)
	{
		skipNextRender = false;
		return;
	}

	HRESULT hr;

	// set const data
	XMFLOAT4X4 invProjMatrix = myManagerClass->GetCamera()->GetInvViewProjMatrix();
	FVector3 lightPos = FLightManager::GetLightPos();
	XMFLOAT4X4 lightViewProj = FLightManager::GetLightViewProjMatrix();
	XMFLOAT4X4 invProjMatrix2 = myManagerClass->GetCamera()->GetViewProjMatrixTransposed();
		
	float shaderConstData2[40];
	memcpy(&shaderConstData2, invProjMatrix.m, sizeof(invProjMatrix.m));
	memcpy(&shaderConstData2[16], lightViewProj.m, sizeof(lightViewProj.m));
	//memcpy(&shaderConstData2[16], invProjMatrix2.m, sizeof(invProjMatrix2.m));
	shaderConstData2[32] = lightPos.x;
	shaderConstData2[33] = lightPos.y;
	shaderConstData2[34] = lightPos.z;
	shaderConstData2[35] = 1.0f;
	FVector3 camPos = myManagerClass->GetCamera()->GetPos();
	shaderConstData2[36] = camPos.x;
	shaderConstData2[37] = camPos.y;
	shaderConstData2[38] = camPos.z;
	shaderConstData2[39] = 1.0f;

	memcpy(myConstBufferShaderPtr, shaderConstData2, sizeof(shaderConstData2));

	// populate cmd list
	hr = m_commandList->Reset(myManagerClass->GetCommandAllocator(), m_pipelineState);

	// Set necessary state.
	m_commandList->SetGraphicsRootSignature(m_rootSignature);

	// is this how we bind textures?	
	ID3D12DescriptorHeap* ppHeaps[] = { myManagerClass->GetSRVHeap() };
	m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// Get the size of the memory location for the render target view descriptors.
	unsigned int srvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_GPU_DESCRIPTOR_HANDLE handle(myManagerClass->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart(), myHeapOffsetCBV, srvSize);
	m_commandList->SetGraphicsRootDescriptorTable(0, handle);

	// set viewport/scissor
	m_commandList->RSSetViewports(1, &myManagerClass->GetViewPort());
	m_commandList->RSSetScissorRects(1, &myManagerClass->GetScissorRect());


	// Indicate that the back buffer will be used as a render target.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetGBufferTarget(FD3d12Renderer::GbufferType::Gbuffer_Combined), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

	m_commandList->OMSetRenderTargets(1, &myManagerClass->GetGBufferHandle(FD3d12Renderer::GbufferType::Gbuffer_Combined), FALSE, nullptr);
	//D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = myManagerClass->GetRTVHandle();
	//m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Record commands.
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->DrawInstanced(6, 1, 0, 0);

	// Indicate that the back buffer will now be used to present.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetGBufferTarget(FD3d12Renderer::GbufferType::Gbuffer_Combined), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	hr = m_commandList->Close();

	ID3D12CommandList* ppCommandLists[] = { m_commandList };
	myManagerClass->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// wait for cmdlist to be done before returning
	ID3D12Fence* m_fence;
	HANDLE m_fenceEvent;
	m_fenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
	int fenceToWaitFor = 1; // what value?
	HRESULT result = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_fence);
	result = myManagerClass->GetCommandQueue()->Signal(m_fence, fenceToWaitFor);
	m_fence->SetEventOnCompletion(1, m_fenceEvent);
	WaitForSingleObject(m_fenceEvent, INFINITE);
	m_fence->Release();
	CloseHandle(m_fenceEvent);
}

void FD3d12Quad::SetShader()
{
	skipNextRender = true;

	// get shader ptr + layouts
	FShaderManager::FShader shader = myManagerClass->GetShaderManager().GetShader("lightshader.hlsl");

	if (m_pipelineState)
		m_pipelineState->Release();

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
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	HRESULT hr = myManagerClass->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));

	hr = myManagerClass->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, myManagerClass->GetCommandAllocator(), m_pipelineState, IID_PPV_ARGS(&m_commandList));
	m_commandList->SetName(L"FD3d12Quad");

	if (!firstFrame)
	{
		m_commandList->Close();
		ID3D12CommandList* ppCommandLists[] = { m_commandList };
		myManagerClass->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	firstFrame = false;
}
