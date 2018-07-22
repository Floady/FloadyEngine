#include "FDynamicText.h"
#include "d3dx12.h"
#include "FD3d12Renderer.h"
#include "FCamera.h"
#include <vector>

#include "FDelegate.h"
#include "FFontManager.h"
#include "FJobSystem.h"

// probably wanna pull some things out here, and call init + populatecmdlist with a deferred bool so the renderer controls it from the scenegraphqueue
// object is placed in queue so is not in control of when it is drawn

using namespace DirectX;

FDynamicText::FDynamicText(FD3d12Renderer* aManager, FVector3 aPos, const char* aText, float aWidth, float aHeight, bool aUseKerning, bool anIs2D)
{
	myManagerClass = aManager;
	myUseKerning = aUseKerning;
	myIs2D = anIs2D;
	myIsDeferred = !anIs2D;

	m_ModelProjMatrix = nullptr;
	m_vertexBuffer = nullptr;
	
	myPos.x = aPos.x;
	myPos.y = aPos.y;
	myPos.z = aPos.z;

	memcpy(myText, aText, strlen(aText));
	myText[strlen(aText)] = '\0';

	m_pipelineState = nullptr;
	m_commandList = nullptr;
	pVertexDataBegin = nullptr;
	myShaderName = myIsDeferred ? "primitiveshader_deferred.hlsl" : "shaders.hlsl";

	myWidth = aWidth;
	myHeight = aHeight;

	myMutex.Init(aManager->GetDevice(), "FDynamicTest");
}

FDynamicText::~FDynamicText()
{
	myManagerClass->GetShaderManager().UnregisterForHotReload(this);

}

void FDynamicText::Init()
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
		m_rootSignature->SetName(L"FDynamicText");
	}

	SetShader();
	myManagerClass->GetShaderManager().RegisterForHotReload(myShaderName, this, FDelegate2<void()>::from<FDynamicText, &FDynamicText::SetShader>(this));

	// Create the vertex buffer.
	{
		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.
		hr = myManagerClass->GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(sizeof(FPrimitiveGeometry::Vertex) * 128 * 6), // allocate enough for a max size string (128 characters)
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer));

		// Map the buffer
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		hr = m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));

		// Initialize the vertex buffer view.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(FPrimitiveGeometry::Vertex);

		SetText(myText);
	}

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

		myHeapOffsetCBV = myManagerClass->GetNextOffset();
		myHeapOffsetAll = myHeapOffsetCBV;
		// Describe and create a constant buffer view.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc[1] = {};
		cbvDesc[0].BufferLocation = m_ModelProjMatrix->GetGPUVirtualAddress();
		cbvDesc[0].SizeInBytes = 256; // required to be 256 bytes aligned -> (sizeof(ConstantBuffer) + 255) & ~255
		unsigned int srvSize = myManagerClass->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle0(myManagerClass->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), myHeapOffsetCBV, srvSize);
		myManagerClass->GetDevice()->CreateConstantBufferView(cbvDesc, cbvHandle0);
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
				
		const FFontManager::FFont& font = FFontManager::GetInstance()->GetFont(FFontManager::FFONT_TYPE::Arial, 20, "abcdefghijklmnopqrtsuvwxyz-+");
		myHeapOffsetText = myManagerClass->GetNextOffset();
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle0(myManagerClass->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), myHeapOffsetText, srvSize);
		myManagerClass->GetDevice()->CreateShaderResourceView(font.myTexture, &srvDesc, srvHandle0);
		
		m_commandList->Close();

		// do we need this?
		ID3D12CommandList* ppCommandLists[] = { m_commandList };
		myManagerClass->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	// wait for cmdlist to be done before returning
	myMutex.Signal(myManagerClass->GetCommandQueue());
	myMutex.WaitFor();

	skipNextRender = false;
}

void FDynamicText::Render()
{
	if (skipNextRender)
	{
		skipNextRender = false;
		return;
	}

	if (!m_commandList)
		return;

	PopulateCommandList();

	ID3D12CommandList* ppCommandLists[] = { m_commandList };
	myManagerClass->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void FDynamicText::PopulateCommandList()
{
	HRESULT hr;
	hr = m_commandList->Reset(myManagerClass->GetCommandAllocator(), m_pipelineState);
	if (myIsDeferred)
	{
		// copy modelviewproj data to gpu
		XMMATRIX mtxRot = XMMatrixRotationRollPitchYaw(0, 0, 0);
		XMMATRIX scale = XMMatrixScaling(1, 1, 1);
		XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(myPos.x, myPos.y, myPos.z, 1));
		offset = scale * mtxRot * offset;

		XMFLOAT4X4 ret;
		offset = XMMatrixTranspose(offset);

		XMStoreFloat4x4(&ret, offset);

		float constData[32];
		if (myIs2D)
		{
			XMMATRIX viewproj = XMMatrixTranslationFromVector(XMVectorSet(myPos.x, myPos.y, myPos.z, 1));
			XMFLOAT4X4 viewProjIdentity;
			XMStoreFloat4x4(&viewProjIdentity, XMMatrixTranspose(viewproj));
			memcpy(&constData[0], viewProjIdentity.m, sizeof(XMFLOAT4X4)); // ortho
		}
		else
		{
			memcpy(&constData[0], myManagerClass->GetCamera()->GetViewProjMatrixWithOffset(0, 0, 0).m, sizeof(XMFLOAT4X4));
		}

		memcpy(&constData[16], ret.m, sizeof(XMFLOAT4X4));
		memcpy(myConstantBufferPtr, &constData[0], sizeof(float) * 32);

		for (size_t i = 0; i < FD3d12Renderer::GbufferType::Gbuffer_Combined; i++)
		{
			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetGBufferTarget(i), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
		}
	}
	else
	{
		// copy modelviewproj data to gpu
		if (myIs2D)
		{
			XMMATRIX viewproj = XMMatrixTranslationFromVector(XMVectorSet(myPos.x, myPos.y, myPos.z, 1));
			XMFLOAT4X4 viewProjIdentity;
			XMStoreFloat4x4(&viewProjIdentity, XMMatrixTranspose(viewproj));
			memcpy(myConstantBufferPtr, viewProjIdentity.m, sizeof(XMFLOAT4X4)); // ortho
		}
		else
			memcpy(myConstantBufferPtr, myManagerClass->GetCamera()->GetViewProjMatrixWithOffset(myPos.x, myPos.y, myPos.z).m, sizeof(XMFLOAT4X4));

		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetDepthBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	}

	// Set necessary state.
	m_commandList->SetGraphicsRootSignature(m_rootSignature);

	// is this how we bind textures?
	ID3D12DescriptorHeap* ppHeaps[] = { myManagerClass->GetSRVHeap() };
	m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// Get the size of the memory location for the render target view descriptors.
	unsigned int srvSize = myManagerClass->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_GPU_DESCRIPTOR_HANDLE handle = myManagerClass->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += srvSize*myHeapOffsetAll;
	m_commandList->SetGraphicsRootDescriptorTable(0, handle);

	// set viewport/scissor
	m_commandList->RSSetViewports(1, &myManagerClass->GetViewPort());
	m_commandList->RSSetScissorRects(1, &myManagerClass->GetScissorRect());

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeap = myManagerClass->GetDSVHandle();

	if(myIsDeferred)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = { myManagerClass->GetGBufferHandle(0), myManagerClass->GetGBufferHandle(1), myManagerClass->GetGBufferHandle(2) , myManagerClass->GetGBufferHandle(3) };
		if (myIs2D)
			m_commandList->OMSetRenderTargets(4, rtvHandles, FALSE, NULL);
		else
			m_commandList->OMSetRenderTargets(4, rtvHandles, FALSE, &dsvHeap);
	}
	else
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = myManagerClass->GetRTVHandle();
		if (myIs2D)
			m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, NULL);
		else
			m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHeap);
	}

	// Record commands.
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->DrawInstanced(6 * myWordLength, 1, 0, 0);

	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetDepthBuffer(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	if (myIsDeferred)
	{
		for (size_t i = 0; i < FD3d12Renderer::GbufferType::Gbuffer_Combined; i++)
		{
			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetGBufferTarget(i), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
		}
	}

	hr = m_commandList->Close();
}

void FDynamicText::PopulateCommandListAsync()
{
	if (skipNextRender)
	{
		skipNextRender = false;
		return;
	}

	if (!m_commandList)
		return;

	ID3D12GraphicsCommandList* cmdList = myManagerClass->GetCommandListForWorkerThread(FJobSystem::ourThreadIdx);
	cmdList->SetPipelineState(m_pipelineState);
	
	if (myIsDeferred)
	{
		// copy modelviewproj data to gpu
		XMMATRIX mtxRot = XMMatrixRotationRollPitchYaw(0, 0, 0);
		XMMATRIX scale = XMMatrixScaling(1, 1, 1);
		XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(myPos.x, myPos.y, myPos.z, 1));
		offset = scale * mtxRot * offset;

		XMMATRIX viewproj = XMMatrixIdentity();
		XMFLOAT4X4 viewProjIdentity;

		XMStoreFloat4x4(&viewProjIdentity, viewproj);

		XMFLOAT4X4 ret;
		offset = XMMatrixTranspose(offset);

		XMStoreFloat4x4(&ret, offset);

		float constData[32];
		if (myIs2D)
			memcpy(&constData[0], viewProjIdentity.m, sizeof(XMFLOAT4X4)); // ortho
		else
		{
			memcpy(&constData[0], myManagerClass->GetCamera()->GetViewProjMatrixWithOffset(0, 0, 0).m, sizeof(XMFLOAT4X4));
		}

		memcpy(&constData[16], ret.m, sizeof(XMFLOAT4X4));
		memcpy(myConstantBufferPtr, &constData[0], sizeof(float) * 32);

		for (size_t i = 0; i < FD3d12Renderer::GbufferType::Gbuffer_Combined; i++)
		{
			cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetGBufferTarget(i), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
		}
	}
	else
	{
		// copy modelviewproj data to gpu
		if (myIs2D)
		{
			XMMATRIX viewproj = XMMatrixTranslationFromVector(XMVectorSet(myPos.x, myPos.y, myPos.z, 1));
			XMFLOAT4X4 viewProjIdentity;
			XMStoreFloat4x4(&viewProjIdentity, XMMatrixTranspose(viewproj));
			memcpy(myConstantBufferPtr, viewProjIdentity.m, sizeof(XMFLOAT4X4)); // ortho
		}
		else
			memcpy(myConstantBufferPtr, myManagerClass->GetCamera()->GetViewProjMatrixWithOffset(myPos.x, myPos.y, myPos.z).m, sizeof(XMFLOAT4X4));

		if (!myIs2D)
			cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetDepthBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	}
	// Set necessary state.
	cmdList->SetGraphicsRootSignature(m_rootSignature);

	// is this how we bind textures?
	ID3D12DescriptorHeap* ppHeaps[] = { myManagerClass->GetSRVHeap() };
	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// Get the size of the memory location for the render target view descriptors.
	unsigned int srvSize = myManagerClass->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_GPU_DESCRIPTOR_HANDLE handle = myManagerClass->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += srvSize*myHeapOffsetAll;
	cmdList->SetGraphicsRootDescriptorTable(0, handle);

	// set viewport/scissor
	cmdList->RSSetViewports(1, &myManagerClass->GetViewPort());
	cmdList->RSSetScissorRects(1, &myManagerClass->GetScissorRect());

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeap = myManagerClass->GetDSVHandle();

	if (myIsDeferred)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = { myManagerClass->GetGBufferHandle(0), myManagerClass->GetGBufferHandle(1), myManagerClass->GetGBufferHandle(2) , myManagerClass->GetGBufferHandle(3) };
		if (myIs2D)
			cmdList->OMSetRenderTargets(4, rtvHandles, FALSE, NULL);
		else
			cmdList->OMSetRenderTargets(4, rtvHandles, FALSE, &dsvHeap);
	}
	else
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = myManagerClass->GetRTVHandle();
		if (myIs2D)
			cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, NULL);
		else
			cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHeap);
	}

	// Record commands.
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	cmdList->DrawInstanced(6 * myWordLength, 1, 0, 0);

	if (!myIs2D)
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetDepthBuffer(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	if (myIsDeferred)
	{
		for (size_t i = 0; i < FD3d12Renderer::GbufferType::Gbuffer_Combined; i++)
		{
			cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetGBufferTarget(i), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
		}
	}
}

void FDynamicText::SetText(const char * aNewText)
{
	if (!pVertexDataBegin)
		return;

	//myText = aNewText;
	myWordLength = static_cast<UINT>(strlen(aNewText));
	memcpy(&myText, aNewText, myWordLength);
	myText[myWordLength] = 0;
	const UINT vertexBufferSize = sizeof(FPrimitiveGeometry::Vertex) * myWordLength * 6;
	m_vertexBufferView.SizeInBytes = vertexBufferSize;

	const float finalWidth = myWidth;
	const float finalHeight = myHeight;

	// Create the vertex buffer.
	{
		const FFontManager::FFont& font = FFontManager::GetInstance()->GetFont(FFontManager::FFONT_TYPE::Arial, 20, "-abcdefghijklmnopqrtsuvwxyz123456789.0");

		float texWidth, texHeight;
		const FFontManager::FWordInfo& wordInfo = FFontManager::GetInstance()->GetUVsForWord(font, myText, texWidth, texHeight, myUseKerning);
		float scaleFactorWidth = finalWidth / (texWidth);
		float scaleFactorHeight = finalHeight / (texHeight);

		const float quadZ = 0.0f;
		FPrimitiveGeometry::Vertex uvTL;
		uvTL.uv.x = 0;
		uvTL.uv.y = 0;

		FPrimitiveGeometry::Vertex uvBR;
		uvBR.uv.x = 1;
		uvBR.uv.y = 1;

		myTriangleVertices.reserve(myWordLength * 6);
		float xoffset = 0.0f;

		int vtxIdx = 0;
		for (size_t i = 0; i < myWordLength; i++)
		{
			// set uv's
			uvTL.uv.y = 0;
			uvTL.uv.x = wordInfo.myUVTL[i].x;
			uvBR.uv.x = wordInfo.myUVBR[i].x;
			uvBR.uv.y = 1;

			//why do we scale? should it be uniform or viewport? - viewproj matrix should handle viewport already
			float halfGlyphWidth = wordInfo.myDimensions[i].x / 2.0f;
			float glyphHeight = texHeight;
			halfGlyphWidth = halfGlyphWidth * scaleFactorWidth;
			glyphHeight = glyphHeight * scaleFactorHeight;

			xoffset += halfGlyphWidth; // move half
			xoffset += wordInfo.myKerningOffset[i] * scaleFactorWidth; // todo fix this with the scaling .. :) did an attempt, but current font has no kerning to double check with

			// draw quad
			myTriangleVertices.push_back(FPrimitiveGeometry::Vertex(xoffset - halfGlyphWidth, glyphHeight, quadZ, 0, 0, -1, uvTL.uv.x, uvTL.uv.y));
			myTriangleVertices.push_back(FPrimitiveGeometry::Vertex(xoffset + halfGlyphWidth, 0, quadZ, 0, 0, -1, uvBR.uv.x, uvBR.uv.y));
			myTriangleVertices.push_back(FPrimitiveGeometry::Vertex(xoffset - halfGlyphWidth, 0, quadZ, 0, 0, -1, uvTL.uv.x, uvBR.uv.y));

			myTriangleVertices.push_back(FPrimitiveGeometry::Vertex(xoffset - halfGlyphWidth, glyphHeight, quadZ, 0, 0, -1, uvTL.uv.x, uvTL.uv.y));
			myTriangleVertices.push_back(FPrimitiveGeometry::Vertex(xoffset + halfGlyphWidth, glyphHeight, quadZ, 0, 0, -1, uvBR.uv.x, uvTL.uv.y));
			myTriangleVertices.push_back(FPrimitiveGeometry::Vertex(xoffset + halfGlyphWidth, 0, quadZ, 0, 0, -1, uvBR.uv.x, uvBR.uv.y));

			xoffset += halfGlyphWidth; // move half
			//xoffset += 0.1; //custom spacing
		}

		const UINT vertexBufferSize = sizeof(FPrimitiveGeometry::Vertex) * myWordLength * 6;
		memcpy(pVertexDataBegin, &myTriangleVertices[0], vertexBufferSize);

		myTriangleVertices.clear();
	}
}

void FDynamicText::SetShader()
{	
	skipNextRender = true;

	// get shader ptr + layouts
	FShaderManager::FShader shader = myManagerClass->GetShaderManager().GetShader(myShaderName);

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
	if(myIs2D)
		psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	
	if(myIsDeferred)
	{
		psoDesc.NumRenderTargets = myManagerClass->Gbuffer_Combined;
		for (size_t i = 0; i < myManagerClass->Gbuffer_Combined; i++)
		{
			psoDesc.RTVFormats[i] = myManagerClass->gbufferFormat[i];
		}
	}
	else
	{
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;

	// enable alpha blend for 2d (non-deferred) - deferred has flickering, should fix it - and then only do alpha test (on/off) not blend
	if(!myIsDeferred)
	{
		psoDesc.BlendState.RenderTarget[0] =
		{
			TRUE, FALSE,
			D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};
	}
	HRESULT hr = myManagerClass->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
	
	hr = myManagerClass->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, myManagerClass->GetCommandAllocator(), m_pipelineState, IID_PPV_ARGS(&m_commandList));
	m_commandList->SetName(L"FDynamicText");
	if (!firstFrame)
	{
		m_commandList->Close();
		ID3D12CommandList* ppCommandLists[] = { m_commandList };
		myManagerClass->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	myMutex.Signal(myManagerClass->GetCommandQueue());
	myMutex.WaitFor();

	firstFrame = false;
}
