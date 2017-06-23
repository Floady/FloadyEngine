#include "FD3d12Renderer.h"
#include "FD3d12Triangle.h"
#include "FD3d12Quad.h"
#include "FFontRenderer.h"
#include "FCamera.h"
#include "FShaderManager.h"
#include "FFontManager.h"
#include "FJobSystem.h"
#include "FTextureManager.h"
#include "FPrimitiveGeometry.h"
#include "FTimer.h"
#include "FDebugDrawer.h"
#include "FPostProcessEffect.h"
#include "FLightManager.h"
#include <functional>
#include "windows.h"

FD3d12Renderer* FD3d12Renderer::ourInstance = nullptr;

FD3d12Renderer::FD3d12Renderer()
	: myShaderManager()
	, m_viewport()
	, m_scissorRect()
{
	ourInstance = this;
	m_device = 0;
	m_commandQueue = 0;
	m_swapChain = 0;
	m_renderTargetViewHeap = 0;
	m_backBufferRenderTarget[0] = 0;
	m_backBufferRenderTarget[1] = 0;
	m_commandAllocator = 0;
	m_commandList = 0;
	m_pipelineState = 0;
	m_fence = 0;
	m_fenceEvent = 0;

	myCurrentHeapOffset = 0;
	myCurrentRTVHeapOffset = 0;
	myCamera = nullptr;

	m_depthStencil = 0;
	m_dsvHeap = 0;
	
	myInt = 0;
	
	FTextureManager::GetInstance();
	myDebugDrawer = new FDebugDrawer(this);
}

FD3d12Renderer::~FD3d12Renderer()
{
}

static FD3d12Quad* myQuad = nullptr;

void FD3d12Renderer::CreateRenderTarget(ID3D12Resource*& aResource, D3D12_CPU_DESCRIPTOR_HANDLE & aHandle)
{
	CD3DX12_RESOURCE_DESC resourceDesc(D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0,
		static_cast<UINT>(m_viewport.Width), static_cast<UINT>(m_viewport.Height), 1, 1,
		DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0, D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

	D3D12_CLEAR_VALUE clear_value;
	clear_value.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	clear_value.Color[0] = 0.0f;
	clear_value.Color[1] = 0.0f;
	clear_value.Color[2] = 0.0f;
	clear_value.Color[3] = 1.0f;

	HRESULT result = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clear_value,
		IID_PPV_ARGS(&aResource));
	aResource->SetName(L"Runtime RT");
	// Describe and create a SRV for the texture.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	
	int renderTargetViewDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	aHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart(), myCurrentRTVHeapOffset, renderTargetViewDescriptorSize);
	m_device->CreateRenderTargetView(aResource, NULL, aHandle);

	myCurrentRTVHeapOffset++;
	assert(result == S_OK && "CREATING scratch buffer FAILED");
}

int FD3d12Renderer::CreateConstantBuffer(ID3D12Resource *& aResource, UINT8 *& aMapToPtr)
{
	HRESULT hr = GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(256),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&aResource));

	CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
	hr = aResource->Map(0, &readRange, reinterpret_cast<void**>(&aMapToPtr));

	int heapOffset = GetNextOffset();

	// Describe and create a constant buffer view.
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc[1] = {};
	cbvDesc[0].BufferLocation = aResource->GetGPUVirtualAddress();
	cbvDesc[0].SizeInBytes = 256; // required to be 256 bytes aligned -> (sizeof(ConstantBuffer) + 255) & ~255
	unsigned int srvSize = GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle0(GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), heapOffset, srvSize);
	GetDevice()->CreateConstantBufferView(cbvDesc, cbvHandle0);

	return heapOffset;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE FD3d12Renderer::CreateHeapDescriptorHandleSRV(ID3D12Resource * aResource, DXGI_FORMAT aFormat)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = aFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), myCurrentHeapOffset, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	m_device->CreateShaderResourceView(aResource, &srvDesc, handle);

	myCurrentHeapOffset++;

	return handle;
}

ID3D12PipelineState* FD3d12Renderer::GetPsoObject(int aNrOfRenderTargets, DXGI_FORMAT * aFormats, const char * aShader, ID3D12RootSignature * aRootSignature, bool aDepthEnabled)
{
	FShaderManager::FShader shader = myShaderManager.GetShader(aShader);

	ID3D12PipelineState* pso;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { &shader.myInputElementDescs[0], (UINT)shader.myInputElementDescs.size() };
	psoDesc.pRootSignature = aRootSignature;
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(shader.myVertexShader);
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(shader.myPixelShader);
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	if (!aDepthEnabled)
	{
		psoDesc.DepthStencilState.DepthEnable = FALSE;
	}
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	psoDesc.NumRenderTargets = aNrOfRenderTargets;
	for (size_t i = 0; i < aNrOfRenderTargets; i++)
	{
		psoDesc.RTVFormats[i] = aFormats[i];
	}

	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;

	HRESULT hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
	return pso;
}

ID3D12RootSignature* FD3d12Renderer::GetRootSignature(int aNumberOfSamplers, int aNumberOfCBs)
{
	if (aNumberOfSamplers + aNumberOfCBs == 0) // shouldnt happen
		return nullptr;

	int nrOfDescriptors = aNumberOfSamplers > 0 && aNumberOfCBs > 0 ? 2 : 1;
	ID3D12RootSignature* rootSignature = nullptr;

	CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
	if (aNumberOfSamplers)
	{
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, aNumberOfSamplers, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	}
	if (aNumberOfCBs)
	{
		ranges[aNumberOfSamplers == 0 ? 0 : 1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, aNumberOfCBs, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	}

	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	rootParameters[0].InitAsDescriptorTable(nrOfDescriptors, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

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
	
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

	// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	ID3DBlob* signature;
	ID3DBlob* error;
	HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error);
	hr = GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

	return rootSignature;
}

ID3D12GraphicsCommandList * FD3d12Renderer::CreateCommandList()
{
	ID3D12GraphicsCommandList* cmdList;
	HRESULT hr = GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, GetCommandAllocator(), nullptr, IID_PPV_ARGS(&cmdList));

	return cmdList;
}

bool FD3d12Renderer::Initialize(int screenHeight, int screenWidth, HWND hwnd, bool vsync, bool fullscreen)
{
	m_viewport.Width = static_cast<float>(screenWidth);
	m_viewport.Height = static_cast<float>(screenHeight);
	m_viewport.MaxDepth = 1.0f;

	m_scissorRect.right = static_cast<LONG>(screenWidth);
	m_scissorRect.bottom = static_cast<LONG>(screenHeight);
	m_aspectRatio = (float)screenWidth / (float)screenHeight;

	D3D_FEATURE_LEVEL featureLevel;
	HRESULT result;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
	IDXGIFactory4* factory;
	IDXGIAdapter* adapter;
	IDXGIOutput* adapterOutput;
	unsigned int numModes, i, numerator, denominator, renderTargetViewDescriptorSize;
	size_t stringLength;
	DXGI_MODE_DESC* displayModeList;
	DXGI_ADAPTER_DESC adapterDesc;
	int error;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	IDXGISwapChain* swapChain;
	D3D12_DESCRIPTOR_HEAP_DESC renderTargetViewHeapDesc;

	// Store the vsync setting.
	m_vsync_enabled = vsync;

	// Set the feature level to DirectX 12.1 to enable using all the DirectX 12 features.
	// Note: Not all cards support full DirectX 12, this feature level may need to be reduced on some cards to 12.0.
	featureLevel = D3D_FEATURE_LEVEL_11_0;
	{
#ifdef _DEBUG
		ID3D12Debug* debugController;
		result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
		if (SUCCEEDED(result))
		{
			debugController->EnableDebugLayer();
		}
#endif
	}

	// Create the Direct3D 12 device.
	result = D3D12CreateDevice(NULL, featureLevel, __uuidof(ID3D12Device), (void**)&m_device);
	if (FAILED(result))
	{
		MessageBox(hwnd, L"Could not create a DirectX 12.1 device.  The default video card does not support DirectX 12.1.", L"DirectX Device Failure", MB_OK);
		return false;
	}

	// Initialize the description of the command queue.
	ZeroMemory(&commandQueueDesc, sizeof(commandQueueDesc));

	// Set up the description of the command queue.
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask = 0;

	// Create the command queue.
	result = m_device->CreateCommandQueue(&commandQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&m_commandQueue);
	if (FAILED(result))
	{
		HRESULT hr2 = m_device->GetDeviceRemovedReason();
		return false;
	}
	m_commandQueue->SetName(L"RendererCmdQueue");

	// Create a DirectX graphics interface factory.
	result = CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&factory);
	if (FAILED(result))
	{
		return false;
	}

	// Use the factory to create an adapter for the primary graphics interface (video card).
	result = factory->EnumAdapters(0, &adapter);
	if (FAILED(result))
	{
		return false;
	}

	// Enumerate the primary adapter output (monitor).
	result = adapter->EnumOutputs(0, &adapterOutput);
	if (FAILED(result))
	{
		return false;
	}

	// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	if (FAILED(result))
	{
		return false;
	}

	// Create a list to hold all the possible display modes for this monitor/video card combination.
	displayModeList = new DXGI_MODE_DESC[numModes];
	if (!displayModeList)
	{
		return false;
	}

	// Now fill the display mode list structures.
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
	if (FAILED(result))
	{
		return false;
	}

	// Now go through all the display modes and find the one that matches the screen height and width.
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
	for (i = 0; i < numModes; i++)
	{
		if (displayModeList[i].Height == (unsigned int)screenHeight)
		{
			if (displayModeList[i].Width == (unsigned int)screenWidth)
			{
				numerator = displayModeList[i].RefreshRate.Numerator;
				denominator = displayModeList[i].RefreshRate.Denominator;
			}
		}
	}

	// Get the adapter (video card) description.
	result = adapter->GetDesc(&adapterDesc);
	if (FAILED(result))
	{
		return false;
	}

	// Store the dedicated video card memory in megabytes.
	m_videoCardMemory = (int)(adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	// Convert the name of the video card to a character array and store it.
	error = wcstombs_s(&stringLength, m_videoCardDescription, 128, adapterDesc.Description, 128);
	if (error != 0)
	{
		return false;
	}

	// Release the display mode list.
	delete[] displayModeList;
	displayModeList = 0;

	// Release the adapter output.
	adapterOutput->Release();
	adapterOutput = 0;

	// Release the adapter.
	adapter->Release();
	adapter = 0;

	// Initialize the swap chain description.
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	// Set the swap chain to use double buffering.
	swapChainDesc.BufferCount = 2;

	// Set the height and width of the back buffers in the swap chain.
	swapChainDesc.BufferDesc.Height = screenHeight;
	swapChainDesc.BufferDesc.Width = screenWidth;

	// Set a regular 32-bit surface for the back buffers.
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Set the usage of the back buffers to be render target outputs.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the swap effect to discard the previous buffer contents after swapping.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// Set the handle for the window to render to.
	swapChainDesc.OutputWindow = hwnd;

	// Set to full screen or windowed mode.
	if (fullscreen)
	{
		swapChainDesc.Windowed = false;
	}
	else
	{
		swapChainDesc.Windowed = true;
	}

	// Set the refresh rate of the back buffer.
	if (m_vsync_enabled)
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = numerator;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = denominator;
	}
	else
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Turn multisampling off.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Set the scan line ordering and scaling to unspecified.
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Don't set the advanced flags.
	swapChainDesc.Flags = 0;

	// Finally create the swap chain using the swap chain description.	
	result = factory->CreateSwapChain(m_commandQueue, &swapChainDesc, &swapChain);
	if (FAILED(result))
	{
		return false;
	}

	// Next upgrade the IDXGISwapChain to a IDXGISwapChain3 interface and store it in a private member variable named m_swapChain.
	// This will allow us to use the newer functionality such as getting the current back buffer index.
	result = swapChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&m_swapChain);
	if (FAILED(result))
	{
		return false;
	}

	// Clear pointer to original swap chain interface since we are using version 3 instead (m_swapChain).
	swapChain = 0;

	// Release the factory now that the swap chain has been created.
	factory->Release();
	factory = 0;

	// Initialize the render target view heap description for the two back buffers.
	ZeroMemory(&renderTargetViewHeapDesc, sizeof(renderTargetViewHeapDesc));

	// Set the number of descriptors to two for our two back buffers.  Also set the heap tyupe to render target views.
	renderTargetViewHeapDesc.NumDescriptors = 2 + Gbuffer_count + 10; // 2 backbuffers + gbuffers + 2 postprocess scratchbuffs + 8 randoms for game or whatever
	renderTargetViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	renderTargetViewHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	// Create the render target view heap for the back buffers.
	result = m_device->CreateDescriptorHeap(&renderTargetViewHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_renderTargetViewHeap);
	if (FAILED(result))
	{
		return false;
	}

	myCurrentRTVHeapOffset = 2; // offset for backbuffers

	// Get the size of the memory location for the render target view descriptors.
	renderTargetViewDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 64 * 64;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	result = m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap));

	// Get a handle to the starting memory location in the render target view heap to identify where the render target views will be located for the two back buffers.
	myRenderTargetViewHandle = m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();

	// GBuffer init
	{
		for (size_t i = 0; i < Gbuffer_count; i++)
		{
			CD3DX12_RESOURCE_DESC resourceDesc(D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0,
				static_cast<UINT>(screenWidth), static_cast<UINT>(screenHeight), 1, 1,
				gbufferFormat[i], 1, 0, D3D12_TEXTURE_LAYOUT_UNKNOWN,
				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

			D3D12_CLEAR_VALUE clear_value;
			clear_value.Format = gbufferFormat[i];
			clear_value.Color[0] = 0.0f;
			clear_value.Color[1] = 0.0f;
			clear_value.Color[2] = 0.0f;
			clear_value.Color[3] = 1.0f;

			result = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clear_value,
				IID_PPV_ARGS(&m_gbuffer[i]));
			m_gbuffer[i]->SetName(gbufferFormatName[i]);
			// Describe and create a SRV for the texture.
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = gbufferFormat[i];
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;

			m_gbufferViews[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart(), myCurrentRTVHeapOffset, renderTargetViewDescriptorSize);
			m_gbufferViewsSRV[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), myCurrentHeapOffset, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
			m_device->CreateRenderTargetView(m_gbuffer[i], NULL, m_gbufferViews[i]);
			m_device->CreateShaderResourceView(m_gbuffer[i], &srvDesc, m_gbufferViewsSRV[i]);

			myCurrentRTVHeapOffset++;
			myCurrentHeapOffset++;
			assert(result == S_OK && "CREATING GBUFFER FAILED");
		}
	}

	// Get a pointer to the first back buffer from the swap chain.
	result = m_swapChain->GetBuffer(0, __uuidof(ID3D12Resource), (void**)&m_backBufferRenderTarget[0]);
	if (FAILED(result))
	{
		return false;
	}

	// Create a render target view for the first back buffer.
	D3D12_RENDER_TARGET_VIEW_DESC desc;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipSlice = 0;
	desc.Texture2D.PlaneSlice = 0;
	m_device->CreateRenderTargetView(m_backBufferRenderTarget[0], NULL, myRenderTargetViewHandle);

	// Increment the view handle to the next descriptor location in the render target view heap.
	myRenderTargetViewHandle.ptr += renderTargetViewDescriptorSize;

	// Get a pointer to the second back buffer from the swap chain.
	result = m_swapChain->GetBuffer(1, __uuidof(ID3D12Resource), (void**)&m_backBufferRenderTarget[1]);
	if (FAILED(result))
	{
		return false;
	}

	// Create a render target view for the second back buffer.
	m_device->CreateRenderTargetView(m_backBufferRenderTarget[1], NULL, myRenderTargetViewHandle);
	m_backBufferRenderTarget[0]->SetName(L"BackBuffer0");
	m_backBufferRenderTarget[1]->SetName(L"BackBuffer1");

	// Finally get the initial index to which buffer is the current back buffer.
	m_bufferIndex = m_swapChain->GetCurrentBackBufferIndex();

	// SETUP Depth Stencil View
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 12; // how many depth buffers you need? (1 depth, 1 shadow) + 10 shadow 
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	result = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));
	assert(result == S_OK && "ERROR CREATING THE DSV HEAP");

	CD3DX12_RESOURCE_DESC depth_texture(D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0,
		static_cast<UINT>(screenWidth), static_cast<UINT>(screenHeight), 1, 1,
		DXGI_FORMAT_R32_TYPELESS, 1, 0, D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	D3D12_CLEAR_VALUE clear_value;
	clear_value.Format = DXGI_FORMAT_D32_FLOAT;
	clear_value.DepthStencil.Depth = 0.0f;
	clear_value.DepthStencil.Stencil = 0;

	result = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, &depth_texture, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value,
		IID_PPV_ARGS(&m_depthStencil));
	assert(result == S_OK && "CREATING THE DEPTH STENCIL FAILED");

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CPU_DESCRIPTOR_HANDLE depth_handle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	m_device->CreateDepthStencilView(m_depthStencil, &dsvDesc, depth_handle);
	m_depthStencil->SetName(L"m_depthStencil");
	
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = gbufferFormat[Gbuffer_Depth];
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	
	for (int i = 0; i < 10; i++)
	{
		result = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, &depth_texture, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value,
			IID_PPV_ARGS(&myShadowMap[i]));
		assert(result == S_OK && "CREATING THE SHADOW MAP FAILED");

		myShadowMapViewHandle[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), 1+i, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV));
		m_device->CreateDepthStencilView(myShadowMap[i], &dsvDesc, myShadowMapViewHandle[i]); // you cna bind these straight into the gbuffer handles (currently we rebind the gbuffer views to these resources and leave the 2 targets unused
		wchar_t* buff = new wchar_t[20];
		wsprintf(buff, L"shadow map %i", i);
		myShadowMap[i]->SetName(buff);
	}

	//	m_gbufferViewsSRV[Gbuffer_Depth] = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), myCurrentHeapOffset, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	m_device->CreateShaderResourceView(m_depthStencil, &srvDesc, m_gbufferViewsSRV[Gbuffer_Depth]); // Depth tex has better precision than my custom one :(
	//m_device->CreateShaderResourceView(myShadowMap[0], &srvDesc, m_gbufferViewsSRV[Gbuffer_Shadow]);
	//	myCurrentHeapOffset++;
		// ~ SETUP DSV

	// SETUP PostProcess scratch Buffers
	myCurrentPostProcessBufferIdx = 0;

	//*
	for (int i = 0; i < 2; i++)
	{
		CD3DX12_RESOURCE_DESC resourceDesc(D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0,
			static_cast<UINT>(screenWidth), static_cast<UINT>(screenHeight), 1, 1,
			DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0, D3D12_TEXTURE_LAYOUT_UNKNOWN,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		D3D12_CLEAR_VALUE clear_value;
		clear_value.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		clear_value.Color[0] = 0.0f;
		clear_value.Color[1] = 0.0f;
		clear_value.Color[2] = 0.0f;
		clear_value.Color[3] = 1.0f;

		result = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clear_value,
			IID_PPV_ARGS(&myPostProcessScratchBuffers[i]));
		myPostProcessScratchBuffers[i]->SetName(L"PostEffectScratch");
		// Describe and create a SRV for the texture.
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		myPostProcessScratchBufferViews[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart(), myCurrentRTVHeapOffset, renderTargetViewDescriptorSize);
		m_device->CreateRenderTargetView(myPostProcessScratchBuffers[i], NULL, myPostProcessScratchBufferViews[i]);
		
		myCurrentRTVHeapOffset++;
		assert(result == S_OK && "CREATING PostProcess scratch buffer FAILED");
	}
	//*/

		// Create a command allocator.
	result = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&m_commandAllocator);
	if (FAILED(result))
	{
		return false;
	}

	// command allocators for worker threads
	int nrWorkerThreads = FJobSystem::GetInstance()->GetNrWorkerThreads();
	m_workerThreadCmdAllocators = new ID3D12CommandAllocator*[nrWorkerThreads];
	m_workerThreadCmdLists = new ID3D12GraphicsCommandList*[nrWorkerThreads];
	for (int i = 0; i < nrWorkerThreads; i++)
	{
		result = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&m_workerThreadCmdAllocators[i]);
		if (FAILED(result))
		{
			return false;
		}
	}

	// Create a basic command list.
	result = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&m_commandList);
	if (FAILED(result))
	{
		return false;
	}
	m_commandList->SetName(L"D3DClass cmd list");

	// command lists for worker thread
	for (int i = 0; i < nrWorkerThreads; i++)
	{
		result = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_workerThreadCmdAllocators[i], NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&m_workerThreadCmdLists[i]);
		m_workerThreadCmdLists[i]->SetName(L"WorkerThread");
		if (FAILED(result))
		{
			return false;
		}
		result = m_workerThreadCmdLists[i]->Close(); // start closed
	}

	// Initially we need to close the command list during initialization as it is created in a recording state.
	result = m_commandList->Close();
	if (FAILED(result))
	{
		return false;
	}

	// Create a fence for GPU synchronization.
	result = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_fence);
	if (FAILED(result))
	{
		return false;
	}

	// Create an event object for the fence.
	m_fenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (m_fenceEvent == NULL)
	{
		return false;
	}

	// Initialize the starting fence value. 
	m_fenceValue = 1;
	myQuad = new FD3d12Quad(this, FVector3(10, 0, 0));

	result = m_commandList->Reset(m_commandAllocator, m_pipelineState); // how do we deal with this.. passing commandlist around?


	// Init resources for managers, they record in cmd list and execute alltogether
	{
		FFontManager::GetInstance()->InitFont(FFontManager::FFONT_TYPE::Arial, 45, "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz1234567890 {}:.", this, m_commandList);
		FTextureManager::GetInstance()->InitD3DResources(m_device, m_commandList);
		FPrimitiveGeometry::InitD3DResources(m_device, m_commandList);

		m_commandList->Close();
		ID3D12CommandList* ppCommandLists[] = { m_commandList };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	return true;
}

static bool firstFrame = true;
static unsigned int frameCounter = 0;
bool FD3d12Renderer::Render()
{
	HRESULT result;
	D3D12_RESOURCE_BARRIER barrier;
	unsigned int renderTargetViewDescriptorSize;
	float color[4];
	ID3D12CommandList* ppCommandLists[1];
	unsigned long long fenceToWaitFor;

	// Reset (re-use) the memory associated command allocator.
	result = m_commandAllocator->Reset();
	if (FAILED(result))
	{
		return false;
	}

	// Reset the command list, use empty pipeline state for now since there are no shaders and we are just clearing the screen.
	result = m_commandList->Reset(m_commandAllocator, m_pipelineState);
	if (FAILED(result))
	{
		return false;
	}

	// Record commands in the command list now.
	// Start by setting the resource barrier.
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_backBufferRenderTarget[m_bufferIndex];
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	m_commandList->ResourceBarrier(1, &barrier);

	// Get the render target view handle for the current back buffer.
	myRenderTargetViewHandle = m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
	renderTargetViewDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	if (m_bufferIndex == 1)
	{
		myRenderTargetViewHandle.ptr += renderTargetViewDescriptorSize;
	}

	// Set the back buffer as the render target.
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	m_commandList->OMSetRenderTargets(1, &myRenderTargetViewHandle, FALSE, &dsvHandle);

	// Then set the color to clear the window to.
	color[0] = 0.0;
	color[1] = 0.0;
	color[2] = 0.0;
	color[3] = 1.0;
	m_commandList->ClearRenderTargetView(myRenderTargetViewHandle, color, 0, NULL);

	for (int i = 0; i < Gbuffer_count; i++)
	{
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetGBufferTarget(i), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
		m_commandList->ClearRenderTargetView(m_gbufferViews[i], color, 0, NULL);
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetGBufferTarget(i), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}

	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_depthStencil, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	m_commandList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_depthStencil, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	
	for (int i = 0; i < 10; i++)
	{
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myShadowMap[i], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE));
		m_commandList->ClearDepthStencilView(myShadowMapViewHandle[i], D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myShadowMap[i], D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}

	// Indicate that the back buffer will now be used to present.
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	m_commandList->ResourceBarrier(1, &barrier);

	// Close the list of commands.
	result = m_commandList->Close();
	if (FAILED(result))
	{
		return false;
	}

	// Load the command list array (only one command list for now).
	ppCommandLists[0] = m_commandList;

	// Execute the list of commands.
	m_commandQueue->ExecuteCommandLists(1, ppCommandLists);

	if (firstFrame)
	{
		myQuad->Init();

		myDebugDrawer->Init();

		int i = 1;
		for (FPostProcessEffect* postEffect : myPostEffects)
		{
			postEffect->Init(i);
			i = i == 0 ? 1 : 0;
		}
		
		firstFrame = false;
	}
	else
	{
		
		mySceneGraph.InitNewObjects();
		{

			//@todo: only shadowcasting lights.. improve
			const std::vector<FLightManager::PointLight>& pointlights = FLightManager::GetInstance()->GetPointLights();
			FLightManager::GetInstance()->SetActiveLight(-1);
			for (int i = 0; i < pointlights.size() + 1; i++)
			{
				FJobSystem* test = FJobSystem::GetInstance();
				int nrWorkerThreads = test->GetNrWorkerThreads();
				for (int i = 0; i < nrWorkerThreads; i++)
				{
					m_workerThreadCmdAllocators[i]->Reset();
					m_workerThreadCmdLists[i]->Reset(m_workerThreadCmdAllocators[i], nullptr);
				}

				test->Pause();
				test->ResetQueue();

				for (FRenderableObject* object : mySceneGraph.GetObjects())
				{
					test->QueueJob(FDelegate2<void()>::from<FRenderableObject, &FRenderableObject::PopulateCommandListAsyncShadows>(object));
				}

				test->UnPause();

				test->WaitForAllJobs(); // this no longer works if workerthreads start queueing up stuff
				
				for (int i = 0; i < nrWorkerThreads; i++) // you can send commandlists when they are done to let the GPU run ahead a bit (send when queue is at 50%?)
				{
					m_workerThreadCmdLists[i]->Close();
					ID3D12CommandList* ppCommandLists[] = { m_workerThreadCmdLists[i] };
					GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists); // you have to wait for event here I think before combine pass
				}

				// wait for cmdlist to be done before returning
				ID3D12Fence* m_fence2;
				HANDLE m_fenceEvent2;
				m_fenceEvent2 = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
				int fenceToWaitFor = 1; // what value? per-thread counter or something in case you execute multiple ones
				HRESULT result = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_fence2);
				result = GetCommandQueue()->Signal(m_fence2, fenceToWaitFor);
				m_fence2->SetEventOnCompletion(1, m_fenceEvent2);
				WaitForSingleObject(m_fenceEvent2, INFINITE);
				m_fence2->Release();
				CloseHandle(m_fenceEvent2);

				FLightManager::GetInstance()->SetActiveLight(i);
			}

		}

		{
			FJobSystem* test = FJobSystem::GetInstance();
			int nrWorkerThreads = test->GetNrWorkerThreads();
			for (int i = 0; i < nrWorkerThreads; i++)
			{
				m_workerThreadCmdAllocators[i]->Reset();
				m_workerThreadCmdLists[i]->Reset(m_workerThreadCmdAllocators[i], nullptr);
			}

			test->Pause();
			test->ResetQueue();

			for (FRenderableObject* object : mySceneGraph.GetObjects())
			{
				test->QueueJob(FDelegate2<void()>::from<FRenderableObject, &FRenderableObject::PopulateCommandListAsync>(object));
			}

			test->UnPause();

			test->WaitForAllJobs(); // this no longer works if workerthreads start queueing up stuff

			for (int i = 0; i < nrWorkerThreads; i++) // you can send commandlists when they are done to let the GPU run ahead a bit (send when queue is at 50%?)
			{
				m_workerThreadCmdLists[i]->Close();
				ID3D12CommandList* ppCommandLists[] = { m_workerThreadCmdLists[i] };
				GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists); // you have to wait for event here I think before combine pass
			}

			// wait for cmdlist to be done before returning
			ID3D12Fence* m_fence3;
			HANDLE m_fenceEvent3;
			m_fenceEvent3 = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
			int fenceToWaitFor = 1; // what value? per-thread counter or something in case you execute multiple ones
			HRESULT result = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_fence3);
			result = GetCommandQueue()->Signal(m_fence3, fenceToWaitFor);
			m_fence3->SetEventOnCompletion(1, m_fenceEvent3);
			WaitForSingleObject(m_fenceEvent3, INFINITE);
			m_fence3->Release();
			CloseHandle(m_fenceEvent3);
		}

		// deferred combine pass
		myQuad->Render();

		//post effects

		// copy combined onto first scratch
		myCurrentPostProcessBufferIdx = 1; // 0 is the one we render to, 1 is the one we bind as resource
		{
			m_commandList->Reset(m_commandAllocator, m_pipelineState);
			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myPostProcessScratchBuffers[myCurrentPostProcessBufferIdx], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_gbuffer[Gbuffer_Combined], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE));
			m_commandList->CopyResource(myPostProcessScratchBuffers[myCurrentPostProcessBufferIdx], m_gbuffer[Gbuffer_Combined]);
			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_gbuffer[Gbuffer_Combined], D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myPostProcessScratchBuffers[myCurrentPostProcessBufferIdx], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
			m_commandList->Close();
			ID3D12Fence* m_fence3;
			HANDLE m_fenceEvent3;
			m_fenceEvent3 = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
			int fenceToWaitFor = 17; // what value? per-thread counter or something in case you execute multiple ones
			ID3D12CommandList* ppCommandLists[] = { m_commandList };
			GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists); // you have to wait for event here I think before combine pass

			HRESULT result = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_fence3);
			result = GetCommandQueue()->Signal(m_fence3, fenceToWaitFor);
			m_fence3->SetEventOnCompletion(1, m_fenceEvent3);
			WaitForSingleObject(m_fenceEvent3, INFINITE);
			m_fence3->Release();
			CloseHandle(m_fenceEvent3);
		}

		myCurrentPostProcessBufferIdx = 0;

		// render all post effects while swapping back/forth between scratch - copy result over for next posteffect
		for (FPostProcessEffect* postEffect : myPostEffects)
		{
			postEffect->Render();

			{
				int nextScratchBuffIdx = myCurrentPostProcessBufferIdx == 0 ? 1 : 0; // nrOfScratchBuffers
				m_commandList->Reset(m_commandAllocator, m_pipelineState);
				m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myPostProcessScratchBuffers[nextScratchBuffIdx], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
				m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myPostProcessScratchBuffers[myCurrentPostProcessBufferIdx], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE));
				m_commandList->CopyResource(myPostProcessScratchBuffers[nextScratchBuffIdx], myPostProcessScratchBuffers[myCurrentPostProcessBufferIdx]);
				m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myPostProcessScratchBuffers[nextScratchBuffIdx], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
				m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myPostProcessScratchBuffers[myCurrentPostProcessBufferIdx], D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
				m_commandList->Close();
				ID3D12Fence* m_fence3;
				HANDLE m_fenceEvent3;
				m_fenceEvent3 = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
				int fenceToWaitFor = 1; // what value? per-thread counter or something in case you execute multiple ones
				ID3D12CommandList* ppCommandLists[] = { m_commandList };
				GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists); // you have to wait for event here I think before combine pass

				HRESULT result = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_fence3);
				result = GetCommandQueue()->Signal(m_fence3, fenceToWaitFor);
				m_fence3->SetEventOnCompletion(1, m_fenceEvent3);
				WaitForSingleObject(m_fenceEvent3, INFINITE);
				m_fence3->Release();
				CloseHandle(m_fenceEvent3);

				myCurrentPostProcessBufferIdx = nextScratchBuffIdx;
			}
		}

		{
			m_commandList->Reset(m_commandAllocator, m_pipelineState);
			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(FD3d12Renderer::GetInstance()->GetRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST));
			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myPostProcessScratchBuffers[myCurrentPostProcessBufferIdx], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE));
			m_commandList->CopyResource(GetRenderTarget(), myPostProcessScratchBuffers[myCurrentPostProcessBufferIdx]);
			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(FD3d12Renderer::GetInstance()->GetRenderTarget(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT));
			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(myPostProcessScratchBuffers[myCurrentPostProcessBufferIdx], D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
			m_commandList->Close();
			ID3D12Fence* m_fence3;
			HANDLE m_fenceEvent3;
			m_fenceEvent3 = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
			int fenceToWaitFor = 1; // what value? per-thread counter or something in case you execute multiple ones
			ID3D12CommandList* ppCommandLists[] = { m_commandList };
			GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists); // you have to wait for event here I think before combine pass

			HRESULT result = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_fence3);
			result = GetCommandQueue()->Signal(m_fence3, fenceToWaitFor);
			m_fence3->SetEventOnCompletion(1, m_fenceEvent3);
			WaitForSingleObject(m_fenceEvent3, INFINITE);
			m_fence3->Release();
			CloseHandle(m_fenceEvent3);
		}

		// debug draws
		myDebugDrawer->Render();
	}

	for (FRenderableObject* object : mySceneGraph.GetTransparantObjects())
	{
		object->Render();
	}

		
	// Finally present the back buffer to the screen since rendering is complete.
	if (m_vsync_enabled)
	{
		// Lock to screen refresh rate.
		result = m_swapChain->Present(1, 0);
		if (FAILED(result))
		{
			return false;
		}
	}
	else
	{
		// Present as fast as possible.
		result = m_swapChain->Present(0, 0);
		if (FAILED(result))
		{
			return false;
		}
	}
	// Signal and increment the fence value.
	fenceToWaitFor = m_fenceValue;
	result = m_commandQueue->Signal(m_fence, fenceToWaitFor);
	if (FAILED(result))
	{
		return false;
	}
	m_fenceValue++;

	// Wait until the GPU is done rendering.
	if (m_fence->GetCompletedValue() < fenceToWaitFor)
	{
		result = m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent);
		if (FAILED(result))
		{
			return false;
		}
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	// Alternate the back buffer index back and forth between 0 and 1 each frame.
	m_bufferIndex = m_bufferIndex == 0 ? 1 : 0;

	return true;
}

void FD3d12Renderer::IncreaseInt()
{
	// do some actual work to test this (gets compiled out in release obviously)
	for (size_t i = 0; i < 20; i++)
	{
		float x = 100;
		x /= 2.0f;
		x /= 2.0f;
		x /= 2.0f;
	}
	InterlockedIncrement(&myInt);
}

void FD3d12Renderer::Shutdown()
{
	int error;


	// Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
	if (m_swapChain)
	{
		m_swapChain->SetFullscreenState(false, NULL);
	}

	// Close the object handle to the fence event.
	error = CloseHandle(m_fenceEvent);
	if (error == 0)
	{
	}

	// Release the fence.
	if (m_fence)
	{
		m_fence->Release();
		m_fence = 0;
	}

	// Release the empty pipe line state.
	if (m_pipelineState)
	{
		m_pipelineState->Release();
		m_pipelineState = 0;
	}

	// Release the command list.
	if (m_commandList)
	{
		m_commandList->Release();
		m_commandList = 0;
	}

	// Release the command allocator.
	if (m_commandAllocator)
	{
		m_commandAllocator->Release();
		m_commandAllocator = 0;
	}

	// Release the back buffer render target views.
	if (m_backBufferRenderTarget[0])
	{
		m_backBufferRenderTarget[0]->Release();
		m_backBufferRenderTarget[0] = 0;
	}
	if (m_backBufferRenderTarget[1])
	{
		m_backBufferRenderTarget[1]->Release();
		m_backBufferRenderTarget[1] = 0;
	}

	// Release the render target view heap.
	if (m_renderTargetViewHeap)
	{
		m_renderTargetViewHeap->Release();
		m_renderTargetViewHeap = 0;
	}

	// Release the swap chain.
	if (m_swapChain)
	{
		m_swapChain->Release();
		m_swapChain = 0;
	}

	// Release the command queue.
	if (m_commandQueue)
	{
		m_commandQueue->Release();
		m_commandQueue = 0;
	}

	// Release the device.
	if (m_device)
	{
		m_device->Release();
		m_device = 0;
	}

	return;
}
