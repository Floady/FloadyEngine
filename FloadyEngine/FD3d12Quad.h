#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <dxgi1_4.h>
#include "d3dx12.h"
#include "FVector3.h"

using namespace DirectX;

class FD3d12Renderer;

class FD3d12Quad
{
public:
	struct Vertex
	{
		DirectX::XMFLOAT3 position;
		XMFLOAT2 uv;
	};

	FD3d12Quad(FD3d12Renderer* aManager, FVector3 aPos);
	~FD3d12Quad();
	void Init();
	void Render();
	void SetShader();
private:
	bool skipNextRender;
	bool firstFrame;
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
	IDXGISwapChain3* m_swapChain;
	ID3D12Device* m_device;
	ID3D12Resource* m_renderTargets;
	ID3D12CommandAllocator* m_commandAllocator;
	ID3D12CommandQueue* m_commandQueue;
	ID3D12RootSignature* m_rootSignature;
	ID3D12DescriptorHeap* m_rtvHeap;
	ID3D12DescriptorHeap* m_srvHeap;
	ID3D12PipelineState* m_pipelineState;
	ID3D12GraphicsCommandList* m_commandList;
	UINT m_rtvDescriptorSize;

	ID3D12Resource* myConstDataShader; // invProjMatrix + light pos+dir
	UINT8* myConstBufferShaderPtr;

	ID3D12Resource* myInvProjData; // invProjMatrix + light pos+dir
	UINT8* myInvProjDataShaderPtr;
	int myHeapOffsetCBV;

	int myHeapOffset;

	FD3d12Renderer* myManagerClass;
	// App resources.
	ID3D12Resource* m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	ID3D12Resource* m_texture;

	float m_aspectRatio;

};

