#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include "d3dx12.h"
#include <DirectXMath.h>

class FD3d12Triangle
{
public:
	struct Vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT2 uv;
	};

	FD3d12Triangle(UINT width, UINT height);
	~FD3d12Triangle();
	void Init(ID3D12CommandAllocator* aCmdAllocator, ID3D12Device* aDevice, D3D12_CPU_DESCRIPTOR_HANDLE& anRTVHandle, ID3D12CommandQueue* aCmdQueue, ID3D12DescriptorHeap* anSRVHeap);
	void Render(ID3D12Resource* aRenderTarget, ID3D12CommandAllocator* aCmdAllocator, ID3D12CommandQueue* aCmdQueue, D3D12_CPU_DESCRIPTOR_HANDLE& anRTVHandle, ID3D12DescriptorHeap* anSRVHeap);
	ID3D12RootSignature* GetRootSig() {
		return m_rootSignature;
	}
private:
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
	
	ID3D12Device* m_device;
	ID3D12Resource* m_renderTargets;
	ID3D12RootSignature* m_rootSignature;
	ID3D12DescriptorHeap* m_rtvHeap;
	ID3D12DescriptorHeap* m_srvHeap;
	ID3D12PipelineState* m_pipelineState;
	ID3D12GraphicsCommandList* m_commandList;
	UINT m_rtvDescriptorSize;

	// App resources.
	ID3D12Resource* m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	ID3D12Resource* m_texture;

	float m_aspectRatio;

};

