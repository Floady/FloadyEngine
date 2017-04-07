#include "FD3d12Renderer.h"
#include "FTextureManager.h"
#include "FLightManager.h"
#include "FDelegate.h"
#include "FFontManager.h"
#include "FJobSystem.h"
#include "FTextureManager.h"
#include "FLightManager.h"
#include "FPrimitiveGeometry.h"
#include "FDebugDrawer.h"

using namespace DirectX;

FDebugDrawer::FDebugDrawer(FD3d12Renderer* aManager) : btIDebugDraw(), FRenderableObject()
{
	m_commandList = nullptr;
	myManagerClass = aManager;
	myVertexBufferViewLines = new D3D12_VERTEX_BUFFER_VIEW();
	myVertexBufferViewTriangles = new D3D12_VERTEX_BUFFER_VIEW();

	Line line;
	line.myColor = FVector3(1, 1, 1);
	line.myStart = FVector3(0, 0, 0);
	line.myStart = FVector3(0, 8, 0);
}


FDebugDrawer::~FDebugDrawer()
{
}

void FDebugDrawer::drawLine(const btVector3 & from, const btVector3 & to, const btVector3 & color)
{
	Line line;
	line.myColor = FVector3(color.getX(), color.getY(), color.getZ());
	line.myStart = FVector3(from.getX(), from.getY(), from.getZ());
	line.myEnd = FVector3(to.getX(), to.getY(), to.getZ());
	myLines.push_back(line);
}

void FDebugDrawer::drawLine(const FVector3 & from, const FVector3 & to, const FVector3 & color)
{
	Line line;
	line.myColor = color;
	line.myStart = from;
	line.myEnd = to;
	myLines.push_back(line);
}

void FDebugDrawer::reportErrorWarning(const char * warningString)
{
}

void FDebugDrawer::draw3dText(const btVector3 & location, const char * textString)
{
}

void FDebugDrawer::setDebugMode(int debugMode)
{
}

//
//enum	DebugDrawModes
//{
//	DBG_NoDebug = 0,
//	DBG_DrawWireframe = 1,
//	DBG_DrawAabb = 2,
//	DBG_DrawFeaturesText = 4,
//	DBG_DrawContactPoints = 8,
//	DBG_NoDeactivation = 16,
//	DBG_NoHelpText = 32,
//	DBG_DrawText = 64,
//	DBG_ProfileTimings = 128,
//	DBG_EnableSatComparison = 256,
//	DBG_DisableBulletLCP = 512,
//	DBG_EnableCCD = 1024,
//	DBG_DrawConstraints = (1 << 11),
//	DBG_DrawConstraintLimits = (1 << 12),
//	DBG_FastWireframe = (1 << 13),
//	DBG_DrawNormals = (1 << 14),
//	DBG_DrawFrames = (1 << 15),
//	DBG_MAX_DEBUG_DRAW_MODE
//};
int FDebugDrawer::getDebugMode() const
{
	return 1;
}

void FDebugDrawer::drawContactPoint(const btVector3 & PointOnB, const btVector3 & normalOnB, btScalar distance, int lifeTime, const btVector3 & color)
{
}

void FDebugDrawer::DrawTriangle(const FVector3 & aV1, const FVector3 & aV2, const FVector3 & aV3, const FVector3& aColor)
{
	Triangle t;
	t.myVtx1 = aV1;
	t.myVtx2 = aV2;
	t.myVtx3 = aV3;
	t.myColor = aColor;
	myTriangles.push_back(t);
}

void FDebugDrawer::Init()
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

		CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

		CD3DX12_ROOT_PARAMETER1 rootParameters[1];
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

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
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	hr = myManagerClass->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&myPsoLines));

	// PSO for triangles
	psoDesc.InputLayout = { &shader.myInputElementDescs[0], (UINT)shader.myInputElementDescs.size() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	hr = myManagerClass->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&myPsoTriangles));

	// command list
	hr = myManagerClass->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, myManagerClass->GetCommandAllocator(), myPsoLines, IID_PPV_ARGS(&m_commandList));
	m_commandList->Close();

	// VB lines
	CD3DX12_RANGE readRange(0, 0);
	hr = myManagerClass->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(FVector3) * 2 * 256000), // pos + color * nrOfMaxLines(256.000 for now)
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&myVertexBufferLines));
	hr = myVertexBufferLines->Map(0, &readRange, reinterpret_cast<void**>(&myGPUVertexDataLines));

	// VB triangles
	hr = myManagerClass->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(FVector3) * 2 * 256000),  // pos + color * nrOfMaxTriangles(256.000 for now)
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
	if (!m_commandList)
		return;

	if (skipNextRender)
	{
		skipNextRender = false;
		return;
	}

	m_commandList->Reset(myManagerClass->GetCommandAllocator(), nullptr);
	PopulateCommandListInternal(m_commandList);
	m_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_commandList };
	myManagerClass->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void FDebugDrawer::PopulateCommandListAsync()
{
	PopulateCommandListInternal(myManagerClass->GetCommandListForWorkerThread(FJobSystem::ourThreadIdx));
}

void FDebugDrawer::PopulateCommandListInternal(ID3D12GraphicsCommandList* aCmdList)
{
	// copy data to GPU
	std::vector<FVector3> verticesLines;
	for (size_t i = 0; i < myLines.size(); i++)
	{
		verticesLines.push_back(myLines[i].myStart);
		verticesLines.push_back(myLines[i].myColor);
		verticesLines.push_back(myLines[i].myEnd);
		verticesLines.push_back(myLines[i].myColor);
	}
	myLines.clear();

	std::vector<FVector3> verticesTriangles;
	for (size_t i = 0; i < myTriangles.size(); i++)
	{
		verticesTriangles.push_back(myTriangles[i].myVtx1);
		verticesTriangles.push_back(myTriangles[i].myColor);
		verticesTriangles.push_back(myTriangles[i].myVtx2);
		verticesTriangles.push_back(myTriangles[i].myColor);
		verticesTriangles.push_back(myTriangles[i].myVtx3);
		verticesTriangles.push_back(myTriangles[i].myColor);
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
	memcpy(&constData[0], myManagerClass->GetCamera()->GetViewProjMatrixWithOffset(0, 0, 0).m, sizeof(XMFLOAT4X4));
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
	aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
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

	// Indicate that the back buffer will now be used to present.
	aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	aCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myManagerClass->GetDepthBuffer(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}