#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <dxgi1_4.h>
#include "d3dx12.h"
#include "FVector3.h"

using namespace DirectX;
class FCamera;
class FD3DClass;

class FFontRenderer
{
public:
	struct Vertex
	{
		DirectX::XMFLOAT3 position;
		XMFLOAT2 uv;
	};

	FFontRenderer(UINT width, UINT height, FVector3 aPos, const char* aText);
	~FFontRenderer();
	void Init(ID3D12CommandAllocator* aCmdAllocator, ID3D12Device* aDevice, D3D12_CPU_DESCRIPTOR_HANDLE& anRTVHandle, ID3D12CommandQueue* aCmdQueue, ID3D12DescriptorHeap* anSRVHeap, ID3D12RootSignature* aRootSig, FD3DClass* aManager);
	void Render(ID3D12Resource* aRenderTarget, ID3D12CommandAllocator* aCmdAllocator, ID3D12CommandQueue* aCmdQueue, D3D12_CPU_DESCRIPTOR_HANDLE& anRTVHandle, D3D12_CPU_DESCRIPTOR_HANDLE& aDSVHandle, ID3D12DescriptorHeap* anSRVHeap, FCamera* aCam);

private:
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
	UINT8* myConstantBufferPtr;

	// App resources.
	ID3D12Resource* m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	ID3D12Resource* m_texture;

	ID3D12Resource* m_ModelProjMatrix;

	float m_aspectRatio;


	FVector3 myPos;
	const char* myText;
	FD3DClass* myManagerClass; // for getting SRVHeap stuff
	int myHeapOffsetCBV;
	int myHeapOffsetText;
	int myHeapOffsetAll;

	// font stuff
	UINT TextureWidth ;
	UINT TextureHeight;
	size_t wordLength;
	UINT largestBearing;
};

