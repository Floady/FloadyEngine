#include "FD3d12Renderer.h"
#include "FTextureManager.h"
#include "FLightManager.h"
#include "FDelegate.h"
#include "FJobSystem.h"
#include "FTextureManager.h"
#include "FLightManager.h"
#include "FPrimitiveGeometry.h"
#include "FDebugDrawer.h"
#include "FProfiler.h"
#include "FCamera.h"
#include <DirectXMath.h>

using namespace DirectX;

FDebugDrawer::FDebugDrawer(FD3d12Renderer* aManager) : FRenderableObject()
{
	myManagerClass = aManager;
	myVertexBufferViewLines = new D3D12_VERTEX_BUFFER_VIEW();
	myVertexBufferViewTriangles = new D3D12_VERTEX_BUFFER_VIEW();

	Line line;
	line.myColor = FVector3(1, 1, 1);
	line.myStart = FVector3(0, 0, 0);
	line.myStart = FVector3(0, 8, 0);

	myPsoLines = nullptr;
	myPsoTriangles = nullptr;
}

FDebugDrawer::~FDebugDrawer()
{
}

void FDebugDrawer::drawLine(const FVector3 & from, const FVector3 & to, const FVector3 & color)
{
	Line line;
	line.myColor = color;
	line.myStart = from;
	line.myEnd = to;
	myLines.push_back(line);
}

void FDebugDrawer::drawAABB(const FVector3& aMin, const FVector3& aMax, const FVector3 & color)
{
	FVector3 halfDim = (aMax - aMin) / 2.0f;
	DrawSphere(aMin + halfDim, halfDim.Length(), color);

	// 
	drawLine(
		FVector3(aMin.x, aMax.y, aMax.z),
		FVector3(aMax.x, aMax.y, aMax.z), color);

	drawLine(
		FVector3(aMax.x, aMin.y, aMax.z),
		FVector3(aMin.x, aMin.y, aMax.z), color);

	drawLine(
		FVector3(aMax.x, aMin.y, aMin.z),
		FVector3(aMin.x, aMin.y, aMin.z), color);

	drawLine(
		FVector3(aMax.x, aMax.y, aMin.z),
		FVector3(aMin.x, aMax.y, aMin.z), color);

	// ~

	// 
	drawLine(
		FVector3(aMax.x, aMax.y, aMin.z),
		FVector3(aMax.x, aMax.y, aMax.z), color);

	drawLine(
		FVector3(aMax.x, aMin.y, aMin.z),
		FVector3(aMax.x, aMin.y, aMax.z), color);

	drawLine(
		FVector3(aMin.x, aMin.y, aMin.z),
		FVector3(aMin.x, aMin.y, aMax.z), color);

	drawLine(
		FVector3(aMin.x, aMax.y, aMin.z),
		FVector3(aMin.x, aMax.y, aMax.z), color);
	// ~

	// 
	drawLine(
		FVector3(aMax.x, aMin.y, aMax.z),
		FVector3(aMax.x, aMax.y, aMax.z), color);

	drawLine(
		FVector3(aMax.x, aMin.y, aMin.z),
		FVector3(aMax.x, aMax.y, aMin.z), color);

	drawLine(
		FVector3(aMin.x, aMin.y, aMin.z),
		FVector3(aMin.x, aMax.y, aMin.z), color);

	drawLine(
		FVector3(aMin.x, aMin.y, aMax.z),
		FVector3(aMin.x, aMax.y, aMax.z), color);
	// ~
}

void FDebugDrawer::drawAABB(const FAABB & anAABB, const FVector3 & color)
{
	drawAABB(anAABB.myMin, anAABB.myMax, color);
}

void FDebugDrawer::DrawTriangle(const FVector3 & aV1, const FVector3 & aV2, const FVector3 & aV3, const FVector3& aColor)
{
	//FPROFILE_FUNCTION("DebugDrawer Tri");
	Triangle t;
	t.myVtx1 = aV1;
	t.myVtx2 = aV2;
	t.myVtx3 = aV3;
	t.myColor = aColor;
	myTriangles.push_back(t);
}

void FDebugDrawer::DrawPoint(const FVector3 & aV, float aSize, const FVector3 & aColor)
{
	Triangle t;
	float size = aSize / 2.0f;
	t.myVtx1 = aV - FVector3(size, 0, 0);
	t.myVtx2 = aV + FVector3(size, 0, 0);
	t.myVtx3 = aV + FVector3(0, 0, size);
	t.myColor = aColor;
	myTriangles.push_back(t);	

	t.myVtx1 = aV - FVector3(-size, 0, 0);
	t.myVtx2 = aV + FVector3(-size, 0, 0);
	t.myVtx3 = aV + FVector3(0, 0, -size);
	t.myColor = aColor;
	myTriangles.push_back(t);
}

void FDebugDrawer::DrawSphere(const FVector3 & aPoint, float aRadius, const FVector3 & aColor)
{
	DrawPoint(aPoint, 1.0f, aColor);

	int segmentsV = 3 * max(5.0f, aRadius);
	FVector3 lastPoint(0, 0, 0);
	for (size_t i = 0; i < segmentsV+1; i++)
	{
		float a = i * (1.0f / segmentsV);
		float x = sin(2 * PI * a);
		float y = cos(2 * PI * a);
		FVector3 newPoint = aPoint + FVector3(x, 0, y) * aRadius;
		if(i > 0)
			drawLine(lastPoint, newPoint, aColor);

		lastPoint = newPoint;
	}

	for (size_t i = 0; i < segmentsV + 1; i++)
	{
		float a = i * (1.0f / segmentsV);
		float x = sin(2 * PI * a);
		float y = cos(2 * PI * a);
		FVector3 newPoint = aPoint + FVector3(x, y, 0) * aRadius;
		if (i > 0)
			drawLine(lastPoint, newPoint, aColor);

		lastPoint = newPoint;
	}

	for (size_t i = 0; i < segmentsV + 1; i++)
	{
		float a = i * (1.0f / segmentsV);
		float x = sin(2 * PI * a);
		float y = cos(2 * PI * a);
		FVector3 newPoint = aPoint + FVector3(0, x, y) * aRadius;
		if (i > 0)
			drawLine(lastPoint, newPoint, aColor);

		lastPoint = newPoint;
	}
}

void FDebugDrawer::Init()
{
	HRESULT hr;
	{
		m_rootSignature = FD3d12Renderer::GetInstance()->GetRootSignature(0, 1);
		m_rootSignature->SetName(L"DebugDrawer");
	}

	// get shader ptr + layouts
	FShaderManager::FShader shader = myManagerClass->GetShaderManager().GetShader("DebugDrawPrimitive.hlsl");
	
	// PSO for lines
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
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R10G10B10A2_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	hr = myManagerClass->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&myPsoLines));

	// PSO for triangles
	psoDesc.InputLayout = { &shader.myInputElementDescs[0], (UINT)shader.myInputElementDescs.size() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	hr = myManagerClass->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&myPsoTriangles));
	
	// VB lines
	CD3DX12_RANGE readRange(0, 0);
	hr = myManagerClass->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(FVector3) * 2 * 512000), // pos + color * nrOfMaxLines(256.000 for now)
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&myVertexBufferLines));
	hr = myVertexBufferLines->Map(0, &readRange, reinterpret_cast<void**>(&myGPUVertexDataLines));

	// VB triangles
	hr = myManagerClass->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(FVector3) * 2 * 512000),  // pos + color * nrOfMaxTriangles(256.000 for now)
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&myVertexBufferTriangles));
	hr = myVertexBufferTriangles->Map(0, &readRange, reinterpret_cast<void**>(&myGPUVertexDataTriangles));

	myHeapOffsetCBV = myManagerClass->GetNextOffset();
	myHeapOffsetAll = myHeapOffsetCBV;
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
	}

	skipNextRender = false;
}
void FDebugDrawer::Render()
{
	ID3D12GraphicsCommandList* cmdList = myManagerClass->GetCommandListForWorkerThread(FJobSystem::ourThreadIdx);

	if (!cmdList)
		return;

	if (skipNextRender)
	{
		skipNextRender = false;
		return;
	}

	cmdList->Reset(myManagerClass->GetCommandAllocatorForWorkerThread(FJobSystem::ourThreadIdx), nullptr);
	PopulateCommandListInternal(cmdList);
	cmdList->Close();
	ID3D12CommandList* ppCommandLists[] = { cmdList };
	myManagerClass->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void FDebugDrawer::PopulateCommandListAsync()
{
	PopulateCommandListInternal(myManagerClass->GetCommandListForWorkerThread(FJobSystem::ourThreadIdx));
}

void FDebugDrawer::PopulateCommandListInternal(ID3D12GraphicsCommandList* aCmdList)
{
	if (!myPsoLines || !myPsoTriangles)
		return;

	if (myLines.size() == 0 && myTriangles.size() == 0)
		return;

	// copy data to GPU
	std::vector<FVector3> verticesLines;
	verticesLines.reserve(myLines.size() * 4);
	for (size_t i = 0; i < myLines.size(); i++)
	{
		FDebugDrawer::Line& line = myLines[i];
		verticesLines.push_back(line.myStart);
		verticesLines.push_back(line.myColor);
		verticesLines.push_back(line.myEnd);
		verticesLines.push_back(line.myColor);
	}
	myLines.clear();

	std::vector<FVector3> verticesTriangles;
	verticesTriangles.reserve(myTriangles.size() * 6);
	for (size_t i = 0; i < myTriangles.size(); i++)
	{
		FDebugDrawer::Triangle& tri = myTriangles[i];
		verticesTriangles.push_back(tri.myVtx1);
		verticesTriangles.push_back(tri.myColor);
		verticesTriangles.push_back(tri.myVtx2);
		verticesTriangles.push_back(tri.myColor);
		verticesTriangles.push_back(tri.myVtx3);
		verticesTriangles.push_back(tri.myColor);
	}
	myTriangles.clear();

	// update buffers
	if (verticesLines.size() > 0)
	{
		int vertexBufferSize = sizeof(FVector3) * verticesLines.size();
		memcpy(myGPUVertexDataLines, &verticesLines[0], vertexBufferSize);
		myVertexBufferViewLines->BufferLocation = myVertexBufferLines->GetGPUVirtualAddress();
		myVertexBufferViewLines->StrideInBytes = sizeof(FVector3) * 2;
		myVertexBufferViewLines->SizeInBytes = vertexBufferSize;
	}

	if(verticesTriangles.size() > 0)
	{
		int vertexBufferSize = sizeof(FVector3) * verticesTriangles.size();
		memcpy(myGPUVertexDataTriangles, &verticesTriangles[0], vertexBufferSize);
		myVertexBufferViewTriangles->BufferLocation = myVertexBufferTriangles->GetGPUVirtualAddress();
		myVertexBufferViewTriangles->StrideInBytes = sizeof(FVector3) * 2;
		myVertexBufferViewTriangles->SizeInBytes = vertexBufferSize;
	}

	// copy modelviewproj data to gpu
	FVector3 myPos(0, 0, 0);
	FVector3 myScale(1, 1, 1);
	XMMATRIX mtxRot = XMMatrixIdentity();
	XMMATRIX scale = XMMatrixScaling(myScale.x, myScale.y, myScale.z);
	XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(myPos.x, myPos.y, myPos.z, 1));
	offset = scale * mtxRot * offset;

	XMFLOAT4X4 ret;
	offset = XMMatrixTranspose(offset);

	XMStoreFloat4x4(&ret, offset);

	float constData[32];
	memcpy(&constData[0], myManagerClass->GetCamera()->GetViewProjMatrixWithOffset2(0, 0, 0).cell, sizeof(float) * 16);
	memcpy(&constData[16], ret.m, sizeof(XMFLOAT4X4));
	memcpy(myConstantBufferPtr, &constData[0], sizeof(float) * 32);

	// Setup for Draw
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
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeap = myManagerClass->GetDSVHandle();

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = myManagerClass->GetRTVHandle();
	aCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHeap);

	// Record commands.
	if (verticesLines.size() > 0)
	{
		aCmdList->SetPipelineState(myPsoLines);
		aCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		aCmdList->IASetVertexBuffers(0, 1, myVertexBufferViewLines);
		aCmdList->DrawInstanced(verticesLines.size() / 2, 1, 0, 0);
	}
	if(verticesTriangles.size() > 0)
	{
		aCmdList->SetPipelineState(myPsoTriangles);
		aCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		aCmdList->IASetVertexBuffers(0, 1, myVertexBufferViewTriangles);
		aCmdList->DrawInstanced(verticesTriangles.size() / 2, 1, 0, 0);
	}
	
	aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetDepthBuffer(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}
