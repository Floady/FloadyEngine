#include "FPrimitiveBox.h"
#include "d3dx12.h"
#include "FD3DClass.h"
#include "FCamera.h"
#include <vector>

#include "FDelegate.h"
#include "FFontManager.h"
#include "FJobSystem.h"
#include "FTextureManager.h"
#include "FLightManager.h"

extern std::vector<UINT8> GenerateTextureData2();
#define DEFERRED 1
#if DEFERRED
const char* shaderfilename = "primitiveshader_deferred.hlsl";
#else
const char* shaderfilename = "primitiveshader.hlsl";
#endif

FPrimitiveBox::FPrimitiveBox(FD3DClass* aManager, FVector3 aPos)
{
	myManagerClass = aManager;

	m_ModelProjMatrix = nullptr;
	m_ModelProjMatrixShadow = nullptr;
	m_vertexBuffer = nullptr;
	m_indexBuffer = nullptr;
	
	myPos.x = aPos.x;
	myPos.y = aPos.y;
	myPos.z = aPos.z;

	m_pipelineState = nullptr;
	m_pipelineStateShadows = nullptr;
	m_commandList = nullptr;
}

FPrimitiveBox::~FPrimitiveBox()
{
}

static int isfloor = false;
void FPrimitiveBox::Init()
{
	firstFrame = true;
	
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
	myManagerClass->GetShaderManager().RegisterForHotReload("primitiveshader_deferred.hlsl", this, FDelegate::from_method<FPrimitiveBox, &FPrimitiveBox::SetShader>(this));

	// Create the vertex buffer.
	{
		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.
		hr = myManagerClass->GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vertex) * 128 * 6), // allocate enough for a max size string (128 characters)
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer));

		// Map the buffer
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		hr = m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));

		// Initialize the vertex buffer view.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(Vertex);

		// Create the vertex buffer.
		{
			float width = 30.0f;
			float height = 1.0f;
			float depth = 30.0f;

			if (!isfloor)
			{
				isfloor = true;
				width = 1.0f;
				height = 1.0f;
				depth = 1.0f;
			}

			float w2 = 0.5f*width;
			float h2 = 0.5f*height;
			float d2 = 0.5f*depth;
			
			float m_aspectRatio = 1.0f;
			
			Vertex triangleVertices[] =
			{
				{ { -w2, -h2, -d2, 1.0f }, { 0, 0, -1, 0}, { 0.0f, 1.0f, 0.0f, 0.0f } },
				{ { -w2, +h2, -d2, 1.0f }, { 0, 0, -1, 0}, { 0.0f, 0.0f, 0.0f, 0.0f } },
				{ { +w2, +h2, -d2, 1.0f }, { 0, 0, -1, 0}, { 1.0f, 0.0f, 0.0f, 0.0f } },
				{ { +w2, -h2, -d2, 1.0f }, { 0, 0, -1, 0}, { 1.0f, 1.0f, 0.0f, 0.0f } },
										   
				{ { -w2, -h2, +d2, 1.0f }, { 0, 0, 1, 0 },{ 1.0f, 1.0f, 0.0f, 0.0f } },
				{ { +w2, -h2, +d2, 1.0f },{ 0, 0, 1, 0 },{ 0.0f, 1.0f, 0.0f, 0.0f } },
				{ { +w2, +h2, +d2, 1.0f },{ 0, 0, 1, 0 },{ 0.0f, 0.0f, 0.0f, 0.0f } },
				{ { -w2, +h2, +d2, 1.0f },{ 0, 0, 1, 0 }, { 1.0f, 0.0f, 0.0f, 0.0f } },
										   
				{ { -w2, +h2, -d2, 1.0f }, { 0, 1, 0, 0 },{ 0.0f, 1.0f, 0.0f, 0.0f } },
				{ { -w2, +h2, +d2, 1.0f },{ 0, 1, 0, 0 },{ 0.0f, 0.0f, 0.0f, 0.0f } },
				{ { +w2, +h2, +d2, 1.0f },{ 0, 1, 0, 0 },{ 1.0f, 0.0f, 0.0f, 0.0f } },
				{ { +w2, +h2, -d2, 1.0f },{ 0, 1, 0, 0 }, { 1.0f, 1.0f, 0.0f, 0.0f } },
										   
				{ { -w2, -h2, -d2, 1.0f }, { 0, -1, 0,  0 },{ 1.0f, 1.0f, 0.0f, 0.0f } },
				{ { +w2, -h2, -d2, 1.0f },{ 0, -1, 0,  0 },{ 0.0f, 1.0f, 0.0f, 0.0f } },
				{ { +w2, -h2, +d2, 1.0f },{ 0, -1, 0,  0 },{ 0.0f, 0.0f, 0.0f, 0.0f } },
				{ { -w2, -h2, +d2, 1.0f },{ 0, -1, 0,  0 }, { 1.0f, 0.0f, 0.0f, 0.0f } },
										   
				{ { -w2, -h2, +d2, 1.0f }, { -1, 0, 0, 0 },{ 0.0f, 1.0f, 0.0f, 0.0f } },
				{ { -w2, +h2, +d2, 1.0f },{ -1, 0, 0, 0 },{ 0.0f, 0.0f, 0.0f, 0.0f } },
				{ { -w2, +h2, -d2, 1.0f },{ -1, 0, 0, 0 },{ 1.0f, 0.0f, 0.0f, 0.0f } },
				{ { -w2, -h2, -d2, 1.0f },{ -1, 0, 0, 0 }, { 1.0f, 1.0f, 0.0f, 0.0f } },
										   
				{ { +w2, -h2, -d2, 1.0f }, { 1, 0, 0,0 },{ 0.0f, 1.0f, 0.0f, 0.0f } },
				{ { +w2, +h2, -d2, 1.0f },{ 1, 0, 0,0 },{ 0.0f, 0.0f, 0.0f, 0.0f } },
				{ { +w2, +h2, +d2, 1.0f },{ 1, 0, 0,0 },{ 1.0f, 0.0f, 0.0f, 0.0f } },
				{ { +w2, -h2, +d2, 1.0f },{ 1, 0, 0,0 }, { 1.0f, 1.0f, 0.0f, 0.0f } }
			};

			const UINT vertexBufferSize = sizeof(Vertex) * _countof(triangleVertices);
			m_vertexBufferView.SizeInBytes = vertexBufferSize;

			memcpy(pVertexDataBegin, &triangleVertices[0], vertexBufferSize);

			// index buffer
			hr = myManagerClass->GetDevice()->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(sizeof(int) * 128 * 6), // allocate enough for a max size string (128 characters)
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_indexBuffer));

			// Map the buffer
			CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
			hr = m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));

			// Initialize the vertex buffer view.
			m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
			m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;

			int indices[] = {
				0,1,2,0,2,3,
				4,5,6,4,6,7,
				8,9,10,8,10,11,
				12,13,14,12,14,15,
				16,17,18,16,18,19,
				20,21,22,20,22,23
			};

			const UINT indexBufferSize = sizeof(int) * _countof(indices);
			m_indexBufferView.SizeInBytes = indexBufferSize;
			memcpy(pIndexDataBegin, &indices[0], indexBufferSize);
		}
	}

	myHeapOffsetCBV = myManagerClass->GetNextOffset();
	myHeapOffsetAll = myHeapOffsetCBV;
	myHeapOffsetText = myManagerClass->GetNextOffset();
	myHeapOffsetCBVShadow = myManagerClass->GetNextOffset();
	// create constant buffer for modelviewproj
	{
		hr = myManagerClass->GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(256),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_ModelProjMatrix));

		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		hr = m_ModelProjMatrix->Map(0, &readRange, reinterpret_cast<void**>(&myConstantBufferPtr));

		// Describe and create a constant buffer view.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc[1] = {};
		cbvDesc[0].BufferLocation = m_ModelProjMatrix->GetGPUVirtualAddress();
		cbvDesc[0].SizeInBytes = 256; // required to be 256 bytes aligned -> (sizeof(ConstantBuffer) + 255) & ~255
		unsigned int srvSize = myManagerClass->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle0(myManagerClass->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), myHeapOffsetCBV, srvSize);
		myManagerClass->GetDevice()->CreateConstantBufferView(cbvDesc, cbvHandle0);

		// need seperate memory for shadowmodelviewproj cause gpu executes deferred and copying mem is not part of cmd list (move to light manager?)
		hr = myManagerClass->GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(256),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_ModelProjMatrixShadow));

		hr = m_ModelProjMatrixShadow->Map(0, &readRange, reinterpret_cast<void**>(&myConstantBufferShadowsPtr));

		// Describe and create a constant buffer view.
		cbvDesc[0].BufferLocation = m_ModelProjMatrixShadow->GetGPUVirtualAddress();
		srvSize = myManagerClass->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandleShadow(myManagerClass->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), myHeapOffsetCBVShadow, srvSize);
		myManagerClass->GetDevice()->CreateConstantBufferView(cbvDesc, cbvHandleShadow);
	}
	
	// create SRV to global font texture
	{		
		// Describe and create a SRV for the texture.
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // get format from font?
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		// Get the size of the memory location for the render target view descriptors.
		unsigned int srvSize = myManagerClass->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				
		// Get material

		ID3D12Resource* textureUploadHeap;
		// Create the texture.
		{
			// Describe and create a Texture2D.
			D3D12_RESOURCE_DESC textureDesc = {};
			textureDesc.MipLevels = 1;
			textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			textureDesc.Width = 256;
			textureDesc.Height = 256;
			textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			textureDesc.DepthOrArraySize = 1;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

			myManagerClass->GetDevice()->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&textureDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&m_texture));

			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture, 0, 1);

			// Create the GPU upload buffer.
			myManagerClass->GetDevice()->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&textureUploadHeap));

			// Copy data to the intermediate upload heap and then schedule a copy 
			// from the upload heap to the Texture2D.

			void* texture = FTextureManager::GetInstance()->GetTexture("testtexture2.png");
			D3D12_SUBRESOURCE_DATA textureData = {};
			textureData.pData = texture;
			textureData.RowPitch = 256 * 4;
			textureData.SlicePitch = textureData.RowPitch * 256;

			UpdateSubresources(m_commandList, m_texture, textureUploadHeap, 0, 0, 1, &textureData);
			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

			// Get the size of the memory location for the render target view descriptors.
			unsigned int srvSize = myManagerClass->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle0(myManagerClass->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), myHeapOffsetText, srvSize);
			myManagerClass->GetDevice()->CreateShaderResourceView(m_texture, &srvDesc, srvHandle0);
		}

		m_commandList->Close();

		// do we need this?
		ID3D12CommandList* ppCommandLists[] = { m_commandList };
		myManagerClass->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}
	
	skipNextRender = false;
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
}

void FPrimitiveBox::PopulateCommandListAsync()
{
	ID3D12GraphicsCommandList* cmdList = myManagerClass->GetCommandListForWorkerThread(FJobSystem::ourThreadIdx);
	ID3D12CommandAllocator* cmdAllocator = myManagerClass->GetCommandAllocatorForWorkerThread(FJobSystem::ourThreadIdx);
	PopulateCommandListInternal(cmdList);
}

void FPrimitiveBox::PopulateCommandListInternal(ID3D12GraphicsCommandList* aCmdList)
{
	aCmdList->SetPipelineState(m_pipelineState);

	// copy modelviewproj data to gpu
	memcpy(myConstantBufferPtr, myManagerClass->GetCamera()->GetViewProjMatrixWithOffset(myPos.x, myPos.y, myPos.z).m, sizeof(XMFLOAT4X4));

#if DEFERRED
	for (size_t i = 0; i < FD3DClass::GbufferType::Gbuffer_count; i++)
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
	
	//D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = myManagerClass->GetRTVHandle();
	//aCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHeap);
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
	aCmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);

	// Indicate that the back buffer will now be used to present.
	aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetDepthBuffer(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
#if DEFERRED
	for (size_t i = 0; i < FD3DClass::GbufferType::Gbuffer_count; i++)
	{
		aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetGBufferTarget(i), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}
#endif
}

void FPrimitiveBox::PopulateCommandListInternalShadows(ID3D12GraphicsCommandList* aCmdList)
{
	XMFLOAT4X4  myProjMatrixFloatVersion = FLightManager::GetLightViewProjMatrix(myPos.x, myPos.y, myPos.z);

	// copy modelviewproj data to gpu
	memcpy(myConstantBufferShadowsPtr, myProjMatrixFloatVersion.m, sizeof(XMFLOAT4X4));
	
	// set pipeline  to shadow shaders
	aCmdList->SetPipelineState(m_pipelineStateShadows);

	aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetShadowMapBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE));

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
	aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetGBufferTarget(FD3DClass::GbufferType::Gbuffer_color), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeap = myManagerClass->GetShadowMapHandle();

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = myManagerClass->GetGBufferHandle(FD3DClass::GbufferType::Gbuffer_color);
	aCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHeap);

	// Record commands.
	aCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	aCmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	aCmdList->IASetIndexBuffer(&m_indexBufferView);
	aCmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);

	float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	aCmdList->ClearRenderTargetView(myManagerClass->GetGBufferHandle(FD3DClass::GbufferType::Gbuffer_color), color, 0, NULL);

	// Indicate that the back buffer will now be used to present.
	aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetGBufferTarget(FD3DClass::GbufferType::Gbuffer_color), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetShadowMapBuffer(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}

void FPrimitiveBox::SetShader()
{	
	skipNextRender = true;

	// get shader ptr + layouts
	FShaderManager::FShader shader = myManagerClass->GetShaderManager().GetShader("primitiveshader_deferred.hlsl");
	FShaderManager::FShader shaderShadows = myManagerClass->GetShaderManager().GetShader("primitiveshader_deferredShadows.hlsl");

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
	psoDesc.NumRenderTargets = myManagerClass->Gbuffer_count;
	for (size_t i = 0; i < myManagerClass->Gbuffer_count; i++)
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
	m_commandList->SetName(L"PrimitiveBox");
	if (!firstFrame)
	{
		m_commandList->Close();
		ID3D12CommandList* ppCommandLists[] = { m_commandList };
		myManagerClass->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	firstFrame = false;
}
