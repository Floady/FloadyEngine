#include "FPostProcessEffect.h"
#include "FD3d12Renderer.h"
#include "FShaderManager.h"
#include "FPrimitiveGeometry.h"
#include "FJobSystem.h"
#include "FUtilities.h"
#include "FProfiler.h"
#include "comdef.h"

FPostProcessEffect::FPostProcessEffect(BindBufferMask aBindMask, const char* aShaderName, const char* aDebugName)
{
	myBindMask = aBindMask;
	myShaderName = aShaderName;
	myPipelineState = nullptr;
	myRootSignature = nullptr;
	myCommandList = nullptr;
	myDebugName = aDebugName;
	myUseResource = false;
	myNrOfCBV = 0;
}

FPostProcessEffect::FPostProcessEffect(const std::vector<BindInfo>& aResourcesToBind, const char * aShaderName, int aNrOfCBV, const char * aDebugName)
{
	myUseResource = true;
	myShaderName = aShaderName;
	myPipelineState = nullptr;
	myRootSignature = nullptr;
	myCommandList = nullptr;
	myDebugName = aDebugName;
	myNrOfCBV = aNrOfCBV;

	for (const BindInfo& resource : aResourcesToBind)
	{
		BindResource bindResource;
		bindResource.myResource = resource.myResource;
		//bindResource.myResourceHandle = FD3d12Renderer::GetInstance()->CreateHeapDescriptorHandleSRV(resource.myResource, resource.myResourceFormat);
		bindResource.myResourceFormat = resource.myResourceFormat;
		myResources.push_back(bindResource);
	}
}


FPostProcessEffect::~FPostProcessEffect()
{
}

void FPostProcessEffect::Init(int aPostEffectBufferIdx)
{
	myHeapOffsetAll = 0;
	if (myUseResource)
	{
		myHeapOffsetAll = FD3d12Renderer::GetInstance()->GetCurHeapOffset();
		BindResource bindResource;
		bindResource.myResource = FD3d12Renderer::GetInstance()->GetPostProcessBuffer(aPostEffectBufferIdx);
		bindResource.myResourceFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		myResources.push_back(bindResource);

		// get handles in a linear fashion so we can bind to shader
		for (BindResource& resource : myResources)
		{
			resource.myResourceHandle = FD3d12Renderer::GetInstance()->CreateHeapDescriptorHandleSRV(resource.myResource, resource.myResourceFormat);
		}
	}

	for (int i = 0; i < myNrOfCBV; i++)
	{
		ConstBuffer constBuffer;
		FD3d12Renderer::GetInstance()->CreateConstantBuffer(constBuffer.myConstBuffer, constBuffer.myConstantBufferPtr);
		constBuffer.myConstBuffer->SetName(FUtilities::ConvertFromUtf8ToUtf16(std::string(myDebugName).append("Const")).c_str());
		myConstBuffers.push_back(constBuffer);
		float data[16] = { 0 };
		WriteConstBuffer(i, &data[0], 16 * 4);
	}

	HRESULT hr;
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

		// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

		if (FAILED(FD3d12Renderer::GetInstance()->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		int nrOfSRVs = 0;
		for (int i = 0; i < Buffer_Count; i++)
		{
			if (myBindMask & (1 << i) != 0)
				nrOfSRVs++;
		}

		if (myUseResource)
			nrOfSRVs = myResources.size();

		//*
		CD3DX12_DESCRIPTOR_RANGE1 ranges[2];

		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, nrOfSRVs, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, myNrOfCBV, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

		CD3DX12_ROOT_PARAMETER1 rootParameters[1];
		rootParameters[0].InitAsDescriptorTable(myNrOfCBV > 0 ? 2 : 1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

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
		FUtilities::OutputShaderErrorMessage(error);
		hr = FD3d12Renderer::GetInstance()->GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&myRootSignature));
		/*/
		myRootSignature = FD3d12Renderer::GetInstance()->GetRootSignature(nrOfSRVs, myNrOfCBV);
		//*/
		myRootSignature->SetName(FUtilities::ConvertFromUtf8ToUtf16(std::string(myDebugName)).c_str());
	}

	SetShader();
	FD3d12Renderer::GetInstance()->GetShaderManager().RegisterForHotReload(myShaderName, this, FDelegate2<void()>::from<FPostProcessEffect, &FPostProcessEffect::SetShader>(this));

	const UINT vertexBufferSize = sizeof(FPrimitiveGeometry::Vertex) * 6;
	// setup vertexbuffer for screenquad
	hr = FD3d12Renderer::GetInstance()->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&myVertexBuffer));

	// Map the buffer
	CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
	hr = myVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&myVertexDataBegin));

	// Initialize the vertex buffer view.
	myVertexBufferView.BufferLocation = myVertexBuffer->GetGPUVirtualAddress();
	myVertexBufferView.StrideInBytes = sizeof(FPrimitiveGeometry::Vertex);
	
	myVertexBufferView.SizeInBytes = vertexBufferSize;


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
			uvTL.uv.x = 0;
			uvTL.uv.y = 0;
			uvBR.uv.x = 1;
			uvBR.uv.y = 1;
			const float width = 1.0f;
			const float height = 1.0f;

			// draw quad
			triangleVertices[vtxIdx++] = FPrimitiveGeometry::Vertex(-1, height, quadZ, 0, 0, -1, uvTL.uv.x, uvTL.uv.y);
			triangleVertices[vtxIdx++] = FPrimitiveGeometry::Vertex(width, -1, quadZ, 0, 0, -1, uvBR.uv.x, uvBR.uv.y);
			triangleVertices[vtxIdx++] = FPrimitiveGeometry::Vertex(-1, -1, quadZ, 0, 0, -1, uvTL.uv.x, uvBR.uv.y);

			triangleVertices[vtxIdx++] = FPrimitiveGeometry::Vertex(-1, height, quadZ, 0, 0, -1, uvTL.uv.x, uvTL.uv.y);
			triangleVertices[vtxIdx++] = FPrimitiveGeometry::Vertex(width, height, quadZ, 0, 0, -1, uvBR.uv.x, uvTL.uv.y);
			triangleVertices[vtxIdx++] = FPrimitiveGeometry::Vertex(width, -1, quadZ, 0, 0, -1, uvBR.uv.x, uvBR.uv.y);

		}

		memcpy(myVertexDataBegin, &triangleVertices[0], vertexBufferSize);

		delete[] triangleVertices;
	}
}

void FPostProcessEffect::SetShader()
{
	FShaderManager::FShader shader = FD3d12Renderer::GetInstance()->GetShaderManager().GetShader(myShaderName);

	if (myPipelineState)
		myPipelineState->Release();

	if (myCommandList)
		myCommandList->Release();

	// you can make a root sig out of the shadder blob - https://stackoverflow.com/questions/35785271/id3d12devicecreatecomputepipelinestate-fails-e-invalidargs
	// FD3d12Renderer::GetInstance()->GetDevice()->CreateRootSignature(0, shader.myPixelShader, signature->GetBufferSize(), IID_PPV_ARGS(&myRootSignature));
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { &shader.myInputElementDescs[0], (UINT)shader.myInputElementDescs.size() };
	psoDesc.pRootSignature = myRootSignature;
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(shader.myVertexShader);
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(shader.myPixelShader);
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleDesc.Count = 1;
	psoDesc.DepthStencilState.DepthEnable = false;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	HRESULT hr = FD3d12Renderer::GetInstance()->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&myPipelineState));

	if (FAILED(hr))
	{
		_com_error err(hr);
		LPCTSTR errMsg = err.ErrorMessage();
		FUtilities::FLog(FUtilities::ConvertFromUtf16ToUtf8(std::wstring(errMsg).append(L"\n")).c_str());
	}

	hr = FD3d12Renderer::GetInstance()->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, FD3d12Renderer::GetInstance()->GetCommandAllocator(), myPipelineState, IID_PPV_ARGS(&myCommandList));
	myCommandList->Close();
	myCommandList->SetName(FUtilities::ConvertFromUtf8ToUtf16(std::string(myDebugName)).c_str());
}

void FPostProcessEffect::WriteConstBuffer(int i, float* aData, int aSize)
{
	if(i < myConstBuffers.size() && myConstBuffers[i].myConstantBufferPtr)
		memcpy(myConstBuffers[i].myConstantBufferPtr, aData, aSize);
}

void FPostProcessEffect::Render()
{
	if (skipNextRender)
	{
		skipNextRender = false;
		return;
	}

	// populate cmd list
	HRESULT hr = myCommandList->Reset(FD3d12Renderer::GetInstance()->GetCommandAllocator(), myPipelineState);

	// Set necessary state.
	myCommandList->SetGraphicsRootSignature(myRootSignature);

	ID3D12DescriptorHeap* ppHeaps[] = { FD3d12Renderer::GetInstance()->GetSRVHeap() };
	myCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// Get the size of the memory location for the render target view descriptors.
	unsigned int srvSize = FD3d12Renderer::GetInstance()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_GPU_DESCRIPTOR_HANDLE handle(FD3d12Renderer::GetInstance()->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart(), myHeapOffsetAll, srvSize);
	myCommandList->SetGraphicsRootDescriptorTable(0, handle);

	// set viewport/scissor
	myCommandList->RSSetViewports(1, &FD3d12Renderer::GetInstance()->GetViewPort());
	myCommandList->RSSetScissorRects(1, &FD3d12Renderer::GetInstance()->GetScissorRect());

	// Indicate that the back buffer will be used as a render target.
	myCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(FD3d12Renderer::GetInstance()->GetPostProcessBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
	myCommandList->OMSetRenderTargets(1, &FD3d12Renderer::GetInstance()->GetPostProcessScratchBufferHandle(), FALSE, nullptr);

	// Record commands.
	myCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	myCommandList->IASetVertexBuffers(0, 1, &myVertexBufferView);
	myCommandList->DrawInstanced(6, 1, 0, 0);

	// Indicate that the back buffer will now be used to present.
	myCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(FD3d12Renderer::GetInstance()->GetPostProcessBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	hr = myCommandList->Close();

	ID3D12CommandList* ppCommandLists[] = { myCommandList };
	FD3d12Renderer::GetInstance()->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// wait for cmdlist to be done before returning
	ID3D12Fence* m_fence;
	HANDLE m_fenceEvent;
	m_fenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
	int fenceToWaitFor = 2; // what value?
	HRESULT result = FD3d12Renderer::GetInstance()->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_fence);
	result = FD3d12Renderer::GetInstance()->GetCommandQueue()->Signal(m_fence, fenceToWaitFor);
	m_fence->SetEventOnCompletion(1, m_fenceEvent);
	WaitForSingleObject(m_fenceEvent, INFINITE);
	m_fence->Release();
	CloseHandle(m_fenceEvent);
}

void FPostProcessEffect::RenderAsync()
{
	if (skipNextRender)
	{
		skipNextRender = false;
		return;
	}

	//FPROFILE_FUNCTION(myDebugName);

	ID3D12GraphicsCommandList* cmdList = FD3d12Renderer::GetInstance()->GetCommandListForWorkerThread(FJobSystem::ourThreadIdx);

	// Set necessary state.
	cmdList->SetGraphicsRootSignature(myRootSignature);

	ID3D12DescriptorHeap* ppHeaps[] = { FD3d12Renderer::GetInstance()->GetSRVHeap() };
	cmdList->SetPipelineState(myPipelineState);

	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// Get the size of the memory location for the render target view descriptors.
	unsigned int srvSize = FD3d12Renderer::GetInstance()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_GPU_DESCRIPTOR_HANDLE handle(FD3d12Renderer::GetInstance()->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart(), myHeapOffsetAll, srvSize);
	cmdList->SetGraphicsRootDescriptorTable(0, handle);

	// set viewport/scissor
	cmdList->RSSetViewports(1, &FD3d12Renderer::GetInstance()->GetViewPort());
	cmdList->RSSetScissorRects(1, &FD3d12Renderer::GetInstance()->GetScissorRect());


	// Indicate that the back buffer will be used as a render target.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(FD3d12Renderer::GetInstance()->GetPostProcessBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
	cmdList->OMSetRenderTargets(1, &FD3d12Renderer::GetInstance()->GetPostProcessScratchBufferHandle(), FALSE, nullptr);

	// Record commands.
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &myVertexBufferView);
	cmdList->DrawInstanced(6, 1, 0, 0);

	// Indicate that the back buffer will now be used to present.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(FD3d12Renderer::GetInstance()->GetPostProcessBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

}