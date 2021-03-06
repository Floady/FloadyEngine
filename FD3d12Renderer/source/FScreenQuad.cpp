#include "FScreenQuad.h"
#include "d3dx12.h"
#include "FD3d12Renderer.h"
#include "FCamera.h"
#include <DirectXMath.h>
#include <vector>
#include "FTextureManager.h"

#include "FDelegate.h"
#include "FJobSystem.h"

// probably wanna pull some things out here, and call init + populatecmdlist with a deferred bool so the renderer controls it from the scenegraphqueue
// object is placed in queue so is not in control of when it is drawn
using namespace DirectX;

FScreenQuad::FScreenQuad(FD3d12Renderer* aManager, FVector3 aPos, const char* aTexture, float aWidth, float aHeight, bool aUseKerning, bool anIs2D)
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

	myText[0] = '\0';

	m_pipelineState = nullptr;
	m_commandList = nullptr;
	pVertexDataBegin = nullptr;
	myShaderName = myIsDeferred ? "primitiveshader_deferred.hlsl" : "shaders.hlsl";

	myWidth = aWidth;
	myHeight = aHeight;
	myTexName = aTexture;

	SetUVOffset(FVector3(0, 0, 0), FVector3(1, 1, 0));
}

FScreenQuad::~FScreenQuad()
{
	myManagerClass->GetShaderManager().UnregisterForHotReload(this);

	if (m_pipelineState)
		m_pipelineState->Release();

	if (m_commandList)
		m_commandList->Release();

	if (m_ModelProjMatrix)
		m_ModelProjMatrix->Release();

	if (m_vertexBuffer)
		m_vertexBuffer->Release();

	if (m_rootSignature)
		m_rootSignature->Release();
}

void FScreenQuad::Init()
{
	firstFrame = true;

	HRESULT hr;
	{
		m_rootSignature = FD3d12Renderer::GetInstance()->GetRootSignature(1, 1);
		m_rootSignature->SetName(L"FScreenQuad");
	}

	SetShader();
	myManagerClass->GetShaderManager().RegisterForHotReload(myShaderName, this, FDelegate2<void()>::from<FScreenQuad, &FScreenQuad::SetShader>(this));

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

		myHeapOffsetText = myManagerClass->GetNextOffset();
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle0(myManagerClass->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), myHeapOffsetText, srvSize);
		myManagerClass->GetDevice()->CreateShaderResourceView(FTextureManager::GetInstance()->GetTextureD3D(myTexName.c_str()), &srvDesc, srvHandle0);

		m_commandList->Close();

		// do we need this?
		ID3D12CommandList* ppCommandLists[] = { m_commandList };
		myManagerClass->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	skipNextRender = false;
}

void FScreenQuad::Render()
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

void FScreenQuad::PopulateCommandList()
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
			memcpy(&constData[0], myManagerClass->GetCamera()->GetViewProjMatrixWithOffset2(0, 0, 0).cell, sizeof(float) * 16);
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
			memcpy(myConstantBufferPtr, myManagerClass->GetCamera()->GetViewProjMatrixWithOffset2(myPos.x, myPos.y, myPos.z).cell, sizeof(float) * 16);

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

	// Indicate that the back buffer will be used as a render target.
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeap = myManagerClass->GetDSVHandle();

	if (myIsDeferred)
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
	m_commandList->DrawInstanced(6, 1, 0, 0);

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

void FScreenQuad::PopulateCommandListAsync()
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
			memcpy(&constData[0], myManagerClass->GetCamera()->GetViewProjMatrixWithOffset2(0, 0, 0).cell, sizeof(float) * 16);
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
			memcpy(myConstantBufferPtr, myManagerClass->GetCamera()->GetViewProjMatrixWithOffset2(myPos.x, myPos.y, myPos.z).cell, sizeof(float) * 16);

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

	// Indicate that the back buffer will be used as a render target.
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
	cmdList->DrawInstanced(6, 1, 0, 0);

	if(!myIs2D)
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetDepthBuffer(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	if (myIsDeferred)
	{
		for (size_t i = 0; i < FD3d12Renderer::GbufferType::Gbuffer_Combined; i++)
		{
			cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetGBufferTarget(i), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
		}
	}
}

void FScreenQuad::SetText(const char * aNewText)
{
	if (!pVertexDataBegin)
		return;

	//myText = aNewText;
	myWordLength = static_cast<UINT>(strlen(aNewText));
	memcpy(&myText, aNewText, myWordLength);
	myText[myWordLength] = 0;
	const UINT vertexBufferSize = sizeof(FPrimitiveGeometry::Vertex) * 6;
	m_vertexBufferView.SizeInBytes = vertexBufferSize;

	const float finalWidth = myWidth;
	const float finalHeight = myHeight;

	// Create the vertex buffer.
	{

		const float quadZ = 0.0f;
		FPrimitiveGeometry::Vertex uvTL;
		uvTL.uv.x = 0;
		uvTL.uv.y = 0;

		FPrimitiveGeometry::Vertex uvBR;
		uvBR.uv.x = 1;
		uvBR.uv.y = 1;

		FPrimitiveGeometry::Vertex* triangleVertices = new FPrimitiveGeometry::Vertex[6];
		float xoffset = 0.0f;

		int vtxIdx = 0;
		for (size_t i = 0; i < 1; i++)
		{
			// set uv's
			uvTL.uv.x = myUVTL.x;
			uvTL.uv.y = myUVTL.y;
			uvBR.uv.x = myUVBR.x;
			uvBR.uv.y = myUVBR.y;

			// draw quad
			triangleVertices[vtxIdx++] = FPrimitiveGeometry::Vertex(0, myHeight, quadZ, 0, 0, -1, uvTL.uv.x, uvTL.uv.y);
			triangleVertices[vtxIdx++] = FPrimitiveGeometry::Vertex(myWidth, 0, quadZ, 0, 0, -1, uvBR.uv.x, uvBR.uv.y);
			triangleVertices[vtxIdx++] = FPrimitiveGeometry::Vertex(0, 0, quadZ, 0, 0, -1, uvTL.uv.x, uvBR.uv.y);

			triangleVertices[vtxIdx++] = FPrimitiveGeometry::Vertex(0, myHeight, quadZ, 0, 0, -1, uvTL.uv.x, uvTL.uv.y);
			triangleVertices[vtxIdx++] = FPrimitiveGeometry::Vertex(myWidth, myHeight, quadZ, 0, 0, -1, uvBR.uv.x, uvTL.uv.y);
			triangleVertices[vtxIdx++] = FPrimitiveGeometry::Vertex(myWidth, 0, quadZ, 0, 0, -1, uvBR.uv.x, uvBR.uv.y);

		}

		const UINT vertexBufferSize = sizeof(FPrimitiveGeometry::Vertex) * 6;
		memcpy(pVertexDataBegin, &triangleVertices[0], vertexBufferSize);

		delete[] triangleVertices;
	}
}

void FScreenQuad::SetShader()
{
	skipNextRender = true;

	// get shader ptr + layouts
	FShaderManager::FShader shader = myManagerClass->GetShaderManager().GetShader(myShaderName);

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
	if (myIs2D)
		psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	if (myIsDeferred)
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
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R10G10B10A2_UNORM; // HERE
	}

	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;

	// enable alpha blend for 2d (non-deferred) - deferred has flickering, should fix it - and then only do alpha test (on/off) not blend
	if (!myIsDeferred)
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
	m_commandList->SetName(L"FScreenQuad");
	if (!firstFrame)
	{
		m_commandList->Close();
		ID3D12CommandList* ppCommandLists[] = { m_commandList };
		myManagerClass->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	firstFrame = false;
}

void FScreenQuad::SetUVOffset(const FVector3& aTL, const FVector3& aBR)
{
	myUVTL = aTL;
	myUVBR = aBR;
	SetText(myTexName.c_str());
}
