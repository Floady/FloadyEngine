#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>

class FCamera;

class FD3DClass
{
public:
	FD3DClass();
	~FD3DClass();

	bool Initialize(int screenHeight, int screenWidth, HWND hwnd, bool vsync, bool fullscreen);
	void Shutdown();
	void SetCamera(FCamera* aCam) { myCamera = aCam;  }
	bool Render();
	int GetNextOffset() { int val = myCurrentHeapOffset;  myCurrentHeapOffset++; return val; }

private:
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
	ID3D12GraphicsCommandList* m_commandList;
	ID3D12PipelineState* m_pipelineState;
	ID3D12Fence* m_fence;
	HANDLE m_fenceEvent;
	unsigned long long m_fenceValue;


	ID3D12DescriptorHeap* m_srvHeap;

	FCamera* myCamera;
	int myCurrentHeapOffset; //for SRV CBV UAV heap
};

