#include "FDynamicText.h"
#include "d3dx12.h"
#include "D3dCompiler.h"
#include "FD3DClass.h"
#include "FCamera.h"
#include <vector>

#include <ft2build.h>
#include <ftglyph.h>

#include "FMatrix.h"
#include "FVector3.h"
#include "FDelegate.h"
#include "FFontManager.h"

FDynamicText::FDynamicText(UINT width, UINT height, FVector3 aPos, const char* aText)
	: m_viewport(),
	m_scissorRect()
{
	m_ModelProjMatrix = nullptr;
	m_vertexBuffer = nullptr;
	
	myPos.x = aPos.x;
	myPos.y = aPos.y;
	myPos.z = aPos.z;

	myText = aText;
	m_viewport.Width = static_cast<float>(width);
	m_viewport.Height = static_cast<float>(height);
	m_viewport.MaxDepth = 1.0f;

	m_scissorRect.right = static_cast<LONG>(width);
	m_scissorRect.bottom = static_cast<LONG>(height);
	m_aspectRatio = (float)width / (float)height;

	m_pipelineState = nullptr;
	m_commandList = nullptr;
}

FDynamicText::~FDynamicText()
{
}

void FDynamicText::Init(ID3D12CommandAllocator* aCmdAllocator, ID3D12Device* aDevice, D3D12_CPU_DESCRIPTOR_HANDLE& anRTVHandle, ID3D12CommandQueue* aCmdQueue, ID3D12DescriptorHeap* anSRVHeap, ID3D12RootSignature* aRootSig, FD3DClass* aManager)
{
	firstFrame = true;

	m_device = aDevice;
	myManagerClass = aManager;
	m_commandAllocator = aCmdAllocator;
	m_commandQueue = aCmdQueue;
	HRESULT hr;

	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

		// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

		if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
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
		hr = aDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
	}

	SetShader();
	myManagerClass->GetShaderManager().RegisterForHotReload("shaders.hlsl", this, FDelegate::from_method<FDynamicText, &FDynamicText::SetShader>(this));

	// Create the vertex buffer.
	{
		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.
		hr = aDevice->CreateCommittedResource(
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

		SetText("AT The Quick Brown Fox Jumped over the Lazy Dog");
	}

	// create constant buffer for modelviewproj
	{
		hr = aDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(256),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_ModelProjMatrix));

		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		hr = m_ModelProjMatrix->Map(0, &readRange, reinterpret_cast<void**>(&myConstantBufferPtr));

		myHeapOffsetCBV = myManagerClass->GetNextOffset();
		myHeapOffsetAll = myHeapOffsetCBV;
		// Describe and create a constant buffer view.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc[1] = {};
		cbvDesc[0].BufferLocation = m_ModelProjMatrix->GetGPUVirtualAddress();
		cbvDesc[0].SizeInBytes = 256; // required to be 256 bytes aligned -> (sizeof(ConstantBuffer) + 255) & ~255
		unsigned int srvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle0(anSRVHeap->GetCPUDescriptorHandleForHeapStart(), myHeapOffsetCBV, srvSize);
		aDevice->CreateConstantBufferView(cbvDesc, cbvHandle0);
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
		unsigned int srvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				
		const FFontManager::FFont& font = FFontManager::GetInstance()->GetFont(FFontManager::FFONT_TYPE::Arial, 20, "abcdefghijklmnopqrtsuvwxyz");
		myHeapOffsetText = myManagerClass->GetNextOffset();
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle0(anSRVHeap->GetCPUDescriptorHandleForHeapStart(), myHeapOffsetText, srvSize);
		aDevice->CreateShaderResourceView(font.myTexture, &srvDesc, srvHandle0);
		
		m_commandList->Close();

		// do we need this?
		ID3D12CommandList* ppCommandLists[] = { m_commandList };
		aCmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}
	
	skipNextRender = false;
}

void FDynamicText::Render(ID3D12Resource* aRenderTarget, ID3D12CommandAllocator* aCmdAllocator, ID3D12CommandQueue* aCmdQueue, D3D12_CPU_DESCRIPTOR_HANDLE& anRTVHandle, D3D12_CPU_DESCRIPTOR_HANDLE& aDSVHandle, ID3D12DescriptorHeap* anSRVHeap, FCamera* aCam)
{
	if (skipNextRender)
	{
		skipNextRender = false;
		return;
	}

	HRESULT hr;

	// copy modelviewproj data to gpu
	memcpy(myConstantBufferPtr, aCam->GetViewProjMatrixWithOffset(myPos.x, myPos.y, myPos.z).m, sizeof(XMFLOAT4X4));

	hr = m_commandList->Reset(aCmdAllocator, m_pipelineState);
	
	// Set necessary state.
	m_commandList->SetGraphicsRootSignature(m_rootSignature);

	// is this how we bind textures?
	ID3D12DescriptorHeap* ppHeaps[] = { anSRVHeap };
	m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// Get the size of the memory location for the render target view descriptors.
	unsigned int srvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_GPU_DESCRIPTOR_HANDLE handle = anSRVHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += srvSize*myHeapOffsetAll;
	m_commandList->SetGraphicsRootDescriptorTable(0, handle);
	
	// set viewport/scissor
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// Indicate that the back buffer will be used as a render target.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(aRenderTarget, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	m_commandList->OMSetRenderTargets(1, &anRTVHandle, FALSE, &aDSVHandle);

	// Record commands.
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->DrawInstanced(6 * myWordLength, 1, 0, 0);

	// Indicate that the back buffer will now be used to present.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(aRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	hr = m_commandList->Close();

	ID3D12CommandList* ppCommandLists[] = { m_commandList };
	aCmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void FDynamicText::SetText(const char * aNewText)
{
	myText = aNewText;
	myWordLength = strlen(myText);

	const UINT vertexBufferSize = sizeof(Vertex) * myWordLength * 6;
	m_vertexBufferView.SizeInBytes = vertexBufferSize;

	// Create the vertex buffer.
	{
		const FFontManager::FFont& font = FFontManager::GetInstance()->GetFont(FFontManager::FFONT_TYPE::Arial, 20, "abcdefghijklmnopqrtsuvwxyz");

		float texWidth, texHeight;
		std::vector<XMFLOAT4> uvs = FFontManager::GetInstance()->GetUVsForWord(font, myText, texWidth, texHeight);
		
		// scale to viewport
		float texWidthRescaled = (float)texWidth / m_viewport.Width; 
		float texHeightRescaled = (float)texHeight / m_viewport.Width;

		const float texMultiplierSize = 1.0f;
		texWidthRescaled *= texMultiplierSize;
		texHeightRescaled *= texMultiplierSize;

		//half it
		texWidthRescaled /= 2.0f;
		texHeightRescaled /= 2.0f;

		const float quadZ = 0.0f;
		Vertex uvTL;
		uvTL.uv.x = 0;
		uvTL.uv.y = 0;

		Vertex uvBR;
		uvBR.uv.x = 1;
		uvBR.uv.y = 1;

		Vertex* triangleVertices = new Vertex[myWordLength * 6];
		float xoffset = 0.0f;

		int vtxIdx = 0;
		for (size_t i = 0; i < myWordLength; i++)
		{
			// set uv's
			uvTL.uv.y = 0;
			uvTL.uv.x = uvs[i].x;
			uvBR.uv.x = uvs[i].z;
			uvBR.uv.y = 1;

			float glyphWidth = 0.01;// (uvs[i + 1].x - uvs[i].x)*texWidth;

			xoffset += glyphWidth; // move half

			// draw quad
			triangleVertices[vtxIdx++] = { { xoffset - glyphWidth, texHeightRescaled * m_aspectRatio, quadZ, 1 },{ uvTL.uv.x, uvTL.uv.y, 0, 0 } };
			triangleVertices[vtxIdx++] = { { xoffset + glyphWidth, -texHeightRescaled * m_aspectRatio, quadZ, 1 },{ uvBR.uv.x, uvBR.uv.y, 0, 0 } };
			triangleVertices[vtxIdx++] = { { xoffset - glyphWidth, -texHeightRescaled * m_aspectRatio, quadZ, 1 },{ uvTL.uv.x, uvBR.uv.y, 0, 0 } };

			triangleVertices[vtxIdx++] = { { xoffset - glyphWidth, texHeightRescaled * m_aspectRatio, quadZ, 1 },{ uvTL.uv.x, uvTL.uv.y, 0, 0 } };
			triangleVertices[vtxIdx++] = { { xoffset + glyphWidth, texHeightRescaled * m_aspectRatio, quadZ, 1 },{ uvBR.uv.x, uvTL.uv.y, 0, 0 } };
			triangleVertices[vtxIdx++] = { { xoffset + glyphWidth, -texHeightRescaled * m_aspectRatio, quadZ, 1 },{ uvBR.uv.x, uvBR.uv.y, 0, 0 } };

			xoffset += glyphWidth; // move half
		}

		const UINT vertexBufferSize = sizeof(Vertex) * myWordLength * 6;
		memcpy(pVertexDataBegin, &triangleVertices[0], vertexBufferSize);

		delete[] triangleVertices;
	}
}

void FDynamicText::SetShader()
{	
	skipNextRender = true;

	// get shader ptr + layouts
	FShaderManager::FShader shader = myManagerClass->GetShaderManager().GetShader("shaders.hlsl");

	if(m_pipelineState)
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
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;

	HRESULT hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
	
	hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator, m_pipelineState, IID_PPV_ARGS(&m_commandList));

	if (!firstFrame)
	{
		m_commandList->Close();
		ID3D12CommandList* ppCommandLists[] = { m_commandList };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	firstFrame = false;
}
