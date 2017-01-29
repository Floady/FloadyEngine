#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include "FShaderManager.h"
#include "FSceneGraph.h"

class FCamera;

class FD3d12Renderer
{
public:
	enum GbufferType
	{
		Gbuffer_color = 0,
		Gbuffer_normals,
		Gbuffer_Depth,
		Gbuffer_Shadow,
		Gbuffer_count
	};
	DXGI_FORMAT gbufferFormat[Gbuffer_count] = { DXGI_FORMAT_R8G8B8A8_UNORM , DXGI_FORMAT_R8G8B8A8_UNORM , DXGI_FORMAT_R32_FLOAT , DXGI_FORMAT_R32_FLOAT };
	LPCWSTR gbufferFormatName[Gbuffer_count] = { L"GBufferColor" , L"GBufferNormals", L"GBufferDepth", L"GBufferShadow" };

	FD3d12Renderer();
	~FD3d12Renderer();

	FD3d12Renderer* GetInstance() { return ourInstance; }

	bool Initialize(int screenHeight, int screenWidth, HWND hwnd, bool vsync, bool fullscreen);
	void Shutdown();
	void SetCamera(FCamera* aCam) { myCamera = aCam; }
	FCamera* GetCamera() { return myCamera; }
	bool Render();
	int GetNextOffset() { int val = myCurrentHeapOffset;  myCurrentHeapOffset++; return val; } // this is for the CBV heap
	
	FShaderManager& GetShaderManager() { return myShaderManager;  }
	ID3D12CommandAllocator* GetCommandAllocator() { return m_commandAllocator; }
	ID3D12DescriptorHeap* GetSRVHeap() { return m_srvHeap; }
	ID3D12DescriptorHeap* GetRTVHeap() { return m_renderTargetViewHeap; }
	D3D12_VIEWPORT& GetViewPort() { return m_viewport; }
	D3D12_RECT& GetScissorRect() { return m_scissorRect; }
	float GetAspectRatio() { return m_aspectRatio; }
	ID3D12Device* GetDevice() { return m_device; }
	ID3D12CommandQueue* GetCommandQueue() { return m_commandQueue; }
	void IncreaseInt();
	ID3D12Resource* GetRenderTarget() { return m_backBufferRenderTarget[m_bufferIndex]; }
	ID3D12Resource* GetGBufferTarget(int i) { return m_gbuffer[i]; }
	ID3D12Resource* GetDepthBuffer() { return m_depthStencil; }
	ID3D12Resource* GetShadowMapBuffer() { return myShadowMap; }
	D3D12_CPU_DESCRIPTOR_HANDLE& GetRTVHandle() { return myRenderTargetViewHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE& GetGBufferHandle(int anIdx) { return m_gbufferViews[anIdx]; }
	D3D12_CPU_DESCRIPTOR_HANDLE& GetGBufferHandleSRV(int anIdx) { return m_gbufferViewsSRV[anIdx]; }
	D3D12_CPU_DESCRIPTOR_HANDLE& GetDSVHandle() { return m_dsvHeap->GetCPUDescriptorHandleForHeapStart(); }
	D3D12_CPU_DESCRIPTOR_HANDLE& GetShadowMapHandle() { return myShadowMapViewHandle; }
	ID3D12CommandAllocator* GetCommandAllocatorForWorkerThread(int aWorkerThreadId) { return m_workerThreadCmdAllocators[aWorkerThreadId]; }
	ID3D12GraphicsCommandList* GetCommandListForWorkerThread(int aWorkerThreadId) { return m_workerThreadCmdLists[aWorkerThreadId]; }

	FSceneGraph& GetSceneGraph() { return mySceneGraph; }

private:
	static FD3d12Renderer* ourInstance;
	D3D12_CPU_DESCRIPTOR_HANDLE myRenderTargetViewHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE myShadowMapViewHandle;
	volatile unsigned int myInt;
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
	float m_aspectRatio;
	int m_videoCardMemory;
	bool m_vsync_enabled;
	ID3D12Device* m_device;
	ID3D12CommandQueue* m_commandQueue;
	char m_videoCardDescription[128];
	IDXGISwapChain3* m_swapChain;
	ID3D12DescriptorHeap* m_renderTargetViewHeap;
	ID3D12Resource* m_backBufferRenderTarget[2];
	unsigned int m_bufferIndex;
	ID3D12CommandAllocator* m_commandAllocator;
	ID3D12CommandAllocator** m_workerThreadCmdAllocators;
	ID3D12GraphicsCommandList** m_workerThreadCmdLists;
	ID3D12GraphicsCommandList* m_commandList;
	ID3D12PipelineState* m_pipelineState;
	ID3D12Fence* m_fence;
	HANDLE m_fenceEvent;
	unsigned long long m_fenceValue;


	ID3D12DescriptorHeap* m_srvHeap; // shader resource view
	ID3D12DescriptorHeap* m_dsvHeap; //depth stencil view
	ID3D12Resource* m_depthStencil;
	ID3D12Resource* myShadowMap;

	ID3D12Resource* m_gbuffer[Gbuffer_count];
	D3D12_CPU_DESCRIPTOR_HANDLE m_gbufferViews[Gbuffer_count];
	D3D12_CPU_DESCRIPTOR_HANDLE m_gbufferViewsSRV[Gbuffer_count];

	FCamera* myCamera;
	int myCurrentHeapOffset; //for SRV CBV UAV heap
	int myCurrentRTVHeapOffset; //for RTV heap

	FShaderManager myShaderManager;

	FSceneGraph mySceneGraph;
};

