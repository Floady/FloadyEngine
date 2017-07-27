#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include "FShaderManager.h"
#include "FSceneGraph.h"

class FCamera;
class FDebugDrawer;
class FPostProcessEffect;

class FD3d12Renderer
{
public:
	struct GPUMutex
	{
		GPUMutex();
		void Reset();
		~GPUMutex();
		void Init(ID3D12Device* aDevice, const char* aDebugName);
		void Signal(ID3D12CommandQueue* aCmdQueue);
		void WaitFor();
	private:
		ID3D12Fence* myFence;
		ID3D12CommandQueue* myCmdQueue;
		HANDLE myFenceEvent;
		const char* myDebugName;
		LONG myFenceValue;
	};

	enum GbufferType
	{
		Gbuffer_color = 0,
		Gbuffer_normals,
		Gbuffer_Depth,
		Gbuffer_Shadow,
		Gbuffer_Combined,
		Gbuffer_count
	};
	DXGI_FORMAT gbufferFormat[Gbuffer_count] = { DXGI_FORMAT_R8G8B8A8_UNORM , DXGI_FORMAT_R8G8B8A8_UNORM , DXGI_FORMAT_R32_FLOAT , DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R8G8B8A8_UNORM };
	LPCWSTR gbufferFormatName[Gbuffer_count] = { L"GBufferColor" , L"GBufferNormals", L"GBufferDepth", L"GBufferShadow", L"GBufferCombined" };

	FD3d12Renderer();
	~FD3d12Renderer();

	static FD3d12Renderer* GetInstance();
	int CreateConstantBuffer(ID3D12Resource*& aResource, UINT8*& aMapToPtr); // returns heapoffset
	void CreateRenderTarget(ID3D12Resource*& aResource, D3D12_CPU_DESCRIPTOR_HANDLE& aHandle);
	CD3DX12_CPU_DESCRIPTOR_HANDLE CreateHeapDescriptorHandleSRV(ID3D12Resource* aResource, DXGI_FORMAT aFormat);
	ID3D12PipelineState* GetPsoObject(int aNrOfRenderTargets, DXGI_FORMAT* aFormats, const char* aShader, ID3D12RootSignature* aRootSignature, bool aDepthEnabled);
	ID3D12RootSignature* GetRootSignature(int aNumberOfSamplers, int aNumberOfCBs);
	ID3D12GraphicsCommandList* CreateCommandList();
	bool Initialize(int screenHeight, int screenWidth, HWND hwnd, bool vsync, bool fullscreen);
	void Shutdown();
	void SetCamera(FCamera* aCam) { myCamera = aCam; }
	FCamera* GetCamera() { return myCamera; }
	bool Render();
	void RegisterPostEffect(FPostProcessEffect* aPostEffect) { myPostEffects.push_back(aPostEffect); }
	int GetNextOffset() { int val = myCurrentHeapOffset;  myCurrentHeapOffset++; return val; } // this is for the CBV+SRV heap
	int GetCurHeapOffset() { return myCurrentHeapOffset; }
	
	// Render tasks
	void DoClearBuffers();
	void DoRenderToGBuffer();
	void SetRenderPassDependency(ID3D12CommandQueue* aQueue);
	void SetPostProcessDependency(ID3D12CommandQueue* aQueue);

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
	ID3D12Resource* GetShadowMapBuffer(int i) { return myShadowMap[i]; }
	D3D12_CPU_DESCRIPTOR_HANDLE& GetRTVHandle() { return myRenderTargetViewHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE& GetPostProcessScratchBufferHandle() { return myPostProcessScratchBufferViews[myCurrentPostProcessBufferIdx]; }

	ID3D12Resource* GetPostProcessBuffer() { return myPostProcessScratchBuffers[myCurrentPostProcessBufferIdx]; };
	ID3D12Resource* GetPostProcessBuffer(int aIdx) { return myPostProcessScratchBuffers[aIdx]; };
	D3D12_CPU_DESCRIPTOR_HANDLE& GetGBufferHandle(int anIdx) { return m_gbufferViews[anIdx]; }
	D3D12_CPU_DESCRIPTOR_HANDLE& GetGBufferHandleSRV(int anIdx) { return m_gbufferViewsSRV[anIdx]; }
	D3D12_CPU_DESCRIPTOR_HANDLE& GetDSVHandle() { return m_dsvHeap->GetCPUDescriptorHandleForHeapStart(); }
	D3D12_CPU_DESCRIPTOR_HANDLE& GetShadowMapHandle(int i) { return myShadowMapViewHandle[i]; }
	ID3D12CommandAllocator* GetCommandAllocatorForWorkerThread(int aWorkerThreadId) { return m_workerThreadCmdAllocators[aWorkerThreadId]; }
	ID3D12GraphicsCommandList* GetCommandListForWorkerThread(int aWorkerThreadId) { return m_workerThreadCmdLists[aWorkerThreadId]; }
	FDebugDrawer* GetDebugDrawer() { return myDebugDrawer; }
	FSceneGraph& GetSceneGraph() { return mySceneGraph; }

private:
	static FD3d12Renderer* ourInstance;
	FDebugDrawer* myDebugDrawer;
	D3D12_CPU_DESCRIPTOR_HANDLE myRenderTargetViewHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE myShadowMapViewHandle[10];
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
	public:
	unsigned long long m_fenceValue;


	ID3D12DescriptorHeap* m_srvHeap; // shader resource view
	ID3D12DescriptorHeap* m_dsvHeap; //depth stencil view
	ID3D12Resource* m_depthStencil;
	ID3D12Resource* myShadowMap[10];

	ID3D12Resource* m_gbuffer[Gbuffer_count];
	D3D12_CPU_DESCRIPTOR_HANDLE m_gbufferViews[Gbuffer_count];
	D3D12_CPU_DESCRIPTOR_HANDLE m_gbufferViewsSRV[Gbuffer_count];

	ID3D12Resource* myPostProcessScratchBuffers[2];
	D3D12_CPU_DESCRIPTOR_HANDLE myPostProcessScratchBufferViews[2];
	unsigned int myCurrentPostProcessBufferIdx;

	FCamera* myCamera;
	int myCurrentHeapOffset; //for SRV CBV UAV heap
	int myCurrentRTVHeapOffset; //for RTV heap

	FShaderManager myShaderManager;

	FSceneGraph mySceneGraph;

	std::vector<FPostProcessEffect*> myPostEffects;

	GPUMutex myRenderPassGputMtx;
	GPUMutex myPostProcessGpuMtx;
	GPUMutex myTestMutex;
};

