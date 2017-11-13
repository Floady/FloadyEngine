#include "FGameHighlightManager.h"
#include "FD3d12Renderer.h"
#include "d3dx12.h"
#include "FCamera.h"
#include "FGameEntity.h"
#include "FRenderableObject.h"
#include "FPrimitiveGeometry.h"
#include "FPostProcessEffect.h"
#include "FPrimitiveBox.h"
#include "FRenderMeshComponent.h"

using namespace DirectX;

FGameHighlightManager::FGameHighlightManager()
{
	myScratchBuffer = nullptr;
	heapOffset = FD3d12Renderer::GetInstance()->GetCurHeapOffset();
	FD3d12Renderer::GetInstance()->CreateRenderTarget(myScratchBuffer, myScratchBufferView);
	FD3d12Renderer::GetInstance()->CreateConstantBuffer(myProjectionMatrix, myConstantBufferPtrProjMatrix);
	myProjectionMatrix->SetName(L"HighlightMgrProjectionMatrix");
	DXGI_FORMAT formats = { DXGI_FORMAT_R8G8B8A8_UNORM };
	myRootSignature = FD3d12Renderer::GetInstance()->GetRootSignature(0,1);
	myPSO = FD3d12Renderer::GetInstance()->GetPsoObject(1, &formats, "highlightShader.hlsl", myRootSignature, false);
	myCommandList = FD3d12Renderer::GetInstance()->CreateCommandList();
	myCommandList->Close();
	myCommandList->SetName(L"HighlightManagerCmdList");
	myRootSignature->SetName(L"HighlightManagerRootSig");
	myPSO->SetName(L"HighlightManagerPSO");
	myScratchBuffer->SetName(L"HighlightManagerScratchBuff");
	
	// test vtx data
	const UINT vertexBufferSize = sizeof(FPrimitiveGeometry::Vertex) * 6;
	// setup vertexbuffer for screenquad
	HRESULT hr = FD3d12Renderer::GetInstance()->GetDevice()->CreateCommittedResource(
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
			const float width = 0.5f;
			const float height = 0.5f;

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


	std::vector<FPostProcessEffect::BindInfo> resources;
	resources.push_back(FPostProcessEffect::BindInfo(myScratchBuffer, DXGI_FORMAT_R8G8B8A8_UNORM));
	FD3d12Renderer::GetInstance()->RegisterPostEffect(new FPostProcessEffect(resources, "highlightShaderPost.hlsl", 0, "HighlightPost"));
}

void FGameHighlightManager::Render()
{
	//if (myObjects.size() == 0)
	//	return;

	// test with 1 obj
	FRenderMeshComponent* entity = nullptr;

	HRESULT hr = myCommandList->Reset(FD3d12Renderer::GetInstance()->GetCommandAllocator(), myPSO);
	myCommandList->SetPipelineState(myPSO);

	FVector3 pos;// = entity->GetPos();
	FVector3 vecScale = FVector3(1,1,1);// = entity->GetPos();

	if (myObjects.size() == 1)
	{
		entity = myObjects[0]->GetRenderableObject();
		pos = myObjects[0]->GetPos();
		vecScale = entity->GetScale();
	}

	// copy modelviewproj data to gpu
	XMFLOAT4X4 m = entity ? XMFLOAT4X4(entity->GetRotMatrix()) : XMFLOAT4X4();
	XMMATRIX mtxRot = XMLoadFloat4x4(&m);

	XMMATRIX scale = XMMatrixScaling(vecScale.x, vecScale.y, vecScale.z); //XMMatrixScaling(myScale.x, myScale.y, myScale.z);
	XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(pos.x, pos.y, pos.z, 1));
	offset = scale * mtxRot * offset;

	offset = offset * FD3d12Renderer::GetInstance()->GetCamera()->_viewProjMatrix;

	XMFLOAT4X4 ret;
	offset = XMMatrixTranspose(offset);
	

	XMStoreFloat4x4(&ret, offset);

	float constData[32];
	memcpy(&constData[0], FD3d12Renderer::GetInstance()->GetCamera()->GetViewProjMatrixWithOffset(pos.x, pos.y, pos.z).m, sizeof(XMFLOAT4X4));
	//memcpy(&constData[0], FD3d12Renderer::GetInstance()->GetCamera()->GetViewProjMatrixWithOffset(0,0,0).m, sizeof(XMFLOAT4X4));
	memcpy(&constData[0], ret.m, sizeof(XMFLOAT4X4));
	memcpy(myConstantBufferPtrProjMatrix, &constData[0], sizeof(float) * 16);

	// Set necessary state.
	myCommandList->SetGraphicsRootSignature(myRootSignature);

	// is this how we bind textures?
	ID3D12DescriptorHeap* ppHeaps[] = { FD3d12Renderer::GetInstance()->GetSRVHeap() };
	myCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// Get the size of the memory location for the render target view descriptors.
	unsigned int srvSize = FD3d12Renderer::GetInstance()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_GPU_DESCRIPTOR_HANDLE handle = FD3d12Renderer::GetInstance()->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += srvSize*heapOffset;
	myCommandList->SetGraphicsRootDescriptorTable(0, handle);

	// set viewport/scissor
	myCommandList->RSSetViewports(1, &FD3d12Renderer::GetInstance()->GetViewPort());
	myCommandList->RSSetScissorRects(1, &FD3d12Renderer::GetInstance()->GetScissorRect());
	float color[] = {0,0,0,1};
	myCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myScratchBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
	myCommandList->ClearRenderTargetView(myScratchBufferView, color, 0, NULL);
	
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = FD3d12Renderer::GetInstance()->GetRTVHandle();
	myCommandList->OMSetRenderTargets(1, &myScratchBufferView, FALSE, nullptr);

	// Record commands.
	if (entity)
	{
		if (FPrimitiveBox* box = dynamic_cast<FPrimitiveBox*>(entity))
		{
			if(box->IsInitialized())
			{
				myCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				myCommandList->IASetVertexBuffers(0, 1, &box->GetVertexBufferView());
				myCommandList->IASetIndexBuffer(&box->GetIndexBufferView());
				myCommandList->DrawIndexedInstanced(box->GetIndicesCount(), 1, 0, 0, 0);
			}
		}
	}

	// Indicate that the back buffer will now be used to present.
	myCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myScratchBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	hr = myCommandList->Close();

	ID3D12CommandList* ppCommandLists[] = { myCommandList };
	FD3d12Renderer::GetInstance()->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	FD3d12Renderer::GetInstance()->SetPostProcessDependency(FD3d12Renderer::GetInstance()->GetCommandQueue());
}

void FGameHighlightManager::AddSelectableObject(FGameEntity * anObject)
{
	if (!anObject)
		return;

	for (auto it = myObjects.begin(); it != myObjects.end(); ++it)
	{
		if (*it == anObject)
		{
			return;
		}
	}

	if(anObject)
		myObjects.push_back(anObject);
}

void FGameHighlightManager::RemoveSelectableObject(FGameEntity * anObject)
{
	for (auto it = myObjects.begin(); it != myObjects.end(); ++it)
	{
		if (*it == anObject)
		{
			myObjects.erase(it);
			return;
		}
	}
}

FGameHighlightManager::~FGameHighlightManager()
{
}
