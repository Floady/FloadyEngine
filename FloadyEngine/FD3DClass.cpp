#include "FD3DClass.h"
#include "FD3d12Triangle.h"
#include "FD3d12Quad.h"
#include "FFontRenderer.h"
#include "FDynamicText.h"
#include "FCamera.h"
#include "FShaderManager.h"
#include "FFontManager.h"

FD3DClass::FD3DClass()
	: myShaderManager()
{
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
	myCamera = nullptr;

	m_depthStencil = 0;
	m_dsvHeap = 0;
}

FD3DClass::~FD3DClass()
{
}

static FD3d12Triangle* myTriangle = nullptr;
static FD3d12Quad* myQuad = nullptr;
static FFontRenderer* myFontRenderer = nullptr;
static FDynamicText* myFontRenderer2 = nullptr;
static FDynamicText* myFontRenderer3 = nullptr;

bool FD3DClass::Initialize(int screenHeight, int screenWidth, HWND hwnd, bool vsync, bool fullscreen)
{
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
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle;

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
	for (i = 0; i<numModes; i++)
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
	renderTargetViewHeapDesc.NumDescriptors = 2;
	renderTargetViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	renderTargetViewHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	// Create the render target view heap for the back buffers.
	result = m_device->CreateDescriptorHeap(&renderTargetViewHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_renderTargetViewHeap);
	if (FAILED(result))
	{
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 64;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	result = m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap));


	// Get the size of the memory location for the render target view descriptors.
	renderTargetViewDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Get a handle to the starting memory location in the render target view heap to identify where the render target views will be located for the two back buffers.
	renderTargetViewHandle = m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();

	// Get a pointer to the first back buffer from the swap chain.
	result = m_swapChain->GetBuffer(0, __uuidof(ID3D12Resource), (void**)&m_backBufferRenderTarget[0]);
	if (FAILED(result))
	{
		return false;
	}

	// Create a render target view for the first back buffer.
	m_device->CreateRenderTargetView(m_backBufferRenderTarget[0], NULL, renderTargetViewHandle);

	// Increment the view handle to the next descriptor location in the render target view heap.
	renderTargetViewHandle.ptr += renderTargetViewDescriptorSize;

	// Get a pointer to the second back buffer from the swap chain.
	result = m_swapChain->GetBuffer(1, __uuidof(ID3D12Resource), (void**)&m_backBufferRenderTarget[1]);
	if (FAILED(result))
	{
		return false;
	}

	// Create a render target view for the second back buffer.
	m_device->CreateRenderTargetView(m_backBufferRenderTarget[1], NULL, renderTargetViewHandle);

	// Finally get the initial index to which buffer is the current back buffer.
	m_bufferIndex = m_swapChain->GetCurrentBackBufferIndex();

	// SETUP Depth Stencil View
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 2; // 1 or 2? is this double buffered
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	result = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));
	assert(result == S_OK && "ERROR CREATING THE DSV HEAP");

	CD3DX12_RESOURCE_DESC depth_texture(D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0,
		static_cast< UINT >(screenWidth), static_cast< UINT >(screenHeight), 1, 1,
		DXGI_FORMAT_D32_FLOAT, 1, 0, D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);

	D3D12_CLEAR_VALUE clear_value;
	clear_value.Format = DXGI_FORMAT_D32_FLOAT;
	clear_value.DepthStencil.Depth = 1.0f;
	clear_value.DepthStencil.Stencil = 0;

	result = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, &depth_texture, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value,
		IID_PPV_ARGS(&m_depthStencil));
	assert(result == S_OK && "CREATING THE DEPTH STENCIL FAILED");

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	m_device->CreateDepthStencilView(m_depthStencil, &dsvDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	m_depthStencil->SetName(L"m_depthStencil");

	// ~ SETUP DSV
	
	// Create a command allocator.
	result = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&m_commandAllocator);
	if (FAILED(result))
	{
		return false;
	}

	// Create a basic command list.
	result = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&m_commandList);
	if (FAILED(result))
	{
		return false;
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


	result = m_commandList->Reset(m_commandAllocator, m_pipelineState); // how do we deal with this.. passing commandlist around?
	FFontManager::GetInstance()->InitFont(FFontManager::FFONT_TYPE::Arial, 150, "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz1234567890 {}:", m_device, m_commandQueue, this, m_commandList, m_srvHeap);

	// test triangle
	myTriangle = new FD3d12Triangle(screenWidth, screenHeight);
	myQuad = new FD3d12Quad(screenWidth, screenHeight);
	myFontRenderer = new FFontRenderer(screenWidth, screenHeight, FVector3(10, 0, 0), "Piemol");
	myFontRenderer2 = new FDynamicText(screenWidth, screenHeight, FVector3(0, 0, 1.8), "Extra");
	myFontRenderer3 = new FDynamicText(screenWidth, screenHeight, FVector3(0, 2, 0), "Extra22");

	return true;
}

static bool firstFrame = true;
static unsigned int frameCounter = 0;
bool FD3DClass::Render()
{
	HRESULT result;
	D3D12_RESOURCE_BARRIER barrier;
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle;
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
	renderTargetViewHandle = m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
	renderTargetViewDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	if (m_bufferIndex == 1)
	{
		renderTargetViewHandle.ptr += renderTargetViewDescriptorSize;
	}

	// Set the back buffer as the render target.
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart()); // shouldnt DSV be offsetted too (double buffering)?
	m_commandList->OMSetRenderTargets(1, &renderTargetViewHandle, FALSE, &dsvHandle);

	// Then set the color to clear the window to.
	color[0] = 0.2;
	color[1] = 0.2;
	color[2] = 0.2;
	color[3] = 1.0;
	m_commandList->ClearRenderTargetView(renderTargetViewHandle, color, 0, NULL);
	
	m_commandList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(),
		D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

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
		//myTriangle->Init(m_commandAllocator, m_device, renderTargetViewHandle, m_commandQueue, m_srvHeap);
		//myQuad->Init(m_commandAllocator, m_device, renderTargetViewHandle, m_commandQueue, m_srvHeap, myTriangle->GetRootSig());
		myFontRenderer->Init(m_commandAllocator, m_device, renderTargetViewHandle, m_commandQueue, m_srvHeap, myTriangle->GetRootSig(), this);
		myFontRenderer2->Init(m_commandAllocator, m_device, renderTargetViewHandle, m_commandQueue, m_srvHeap, myTriangle->GetRootSig(), this);
		myFontRenderer3->Init(m_commandAllocator, m_device, renderTargetViewHandle, m_commandQueue, m_srvHeap, myTriangle->GetRootSig(), this);
		firstFrame = false;
	}
	else
	{
		//myTriangle->Render(m_backBufferRenderTarget[m_bufferIndex], m_commandAllocator, m_commandQueue, renderTargetViewHandle, m_srvHeap);
		//myQuad->Render(m_backBufferRenderTarget[m_bufferIndex], m_commandAllocator, m_commandQueue, renderTargetViewHandle, m_srvHeap);
		myFontRenderer->Render(m_backBufferRenderTarget[m_bufferIndex], m_commandAllocator, m_commandQueue, renderTargetViewHandle, dsvHandle, m_srvHeap, myCamera);

		// show frame counter for dynamic text testing
		{
			frameCounter++;
			char buff[128];
			sprintf(buff, "%s %d\0", "lol: ", frameCounter);
			myFontRenderer2->SetText(buff);
		}
		for (size_t i = 0; i < 1; i++)
		{
			myFontRenderer2->Render(m_backBufferRenderTarget[m_bufferIndex], m_commandAllocator, m_commandQueue, renderTargetViewHandle, dsvHandle, m_srvHeap, myCamera);
		}
		myFontRenderer3->Render(m_backBufferRenderTarget[m_bufferIndex], m_commandAllocator, m_commandQueue, renderTargetViewHandle, dsvHandle, m_srvHeap, myCamera);
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
	m_bufferIndex == 0 ? m_bufferIndex = 1 : m_bufferIndex = 0;

	return true;
}

void FD3DClass::Shutdown()
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
