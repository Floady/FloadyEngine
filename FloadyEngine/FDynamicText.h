#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <dxgi1_4.h>
#include "d3dx12.h"
#include "FVector3.h"

using namespace DirectX;
class FCamera;
class FD3DClass;

class FDynamicText
{
public:
	struct Vertex
	{
		DirectX::XMFLOAT4 position;
		XMFLOAT4 uv;
	};

	FDynamicText(FD3DClass* aManager, FVector3 aPos, const char* aText, bool aUseKerning);
	~FDynamicText();
	void Init();
	void Render(ID3D12Resource* aRenderTarget, D3D12_CPU_DESCRIPTOR_HANDLE& anRTVHandle, D3D12_CPU_DESCRIPTOR_HANDLE& aDSVHandle);
	void SetText(const char* aNewText);
	void SetShader();

private:
	bool myUseKerning;

	ID3D12RootSignature* m_rootSignature;
	ID3D12PipelineState* m_pipelineState;
	ID3D12GraphicsCommandList* m_commandList;
	
	UINT8* myConstantBufferPtr;
	UINT8* pVertexDataBegin;

	// App resources.
	ID3D12Resource* m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	ID3D12Resource* m_ModelProjMatrix;
	
	FVector3 myPos;
	const char* myText;
	FD3DClass* myManagerClass; // for getting SRVHeap stuff
	int myHeapOffsetCBV;
	int myHeapOffsetText;
	int myHeapOffsetAll;

	size_t myWordLength;
	
	bool skipNextRender;
	bool firstFrame;
};

