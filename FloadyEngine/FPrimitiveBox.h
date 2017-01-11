#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <dxgi1_4.h>
#include "d3dx12.h"
#include "FVector3.h"

using namespace DirectX;
class FCamera;
class FD3DClass;

class FPrimitiveBox
{
public:
	struct Vertex
	{
		DirectX::XMFLOAT4 position;
		XMFLOAT4 uv;
	};

	FPrimitiveBox(FD3DClass* aManager, FVector3 aPos);
	~FPrimitiveBox();
	void Init();
	void Render();
	void PopulateCommandListAsync();
	void PopulateCommandListInternal(ID3D12GraphicsCommandList* aCmdList);
	void SetShader();

private:
	ID3D12RootSignature* m_rootSignature;
	ID3D12PipelineState* m_pipelineState;
	ID3D12GraphicsCommandList* m_commandList;
	
	UINT8* myConstantBufferPtr;
	UINT8* pVertexDataBegin;
	UINT8* pIndexDataBegin;

	ID3D12Resource* m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	
	ID3D12Resource* m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	ID3D12Resource* m_ModelProjMatrix;
	
	FVector3 myPos;
	FD3DClass* myManagerClass;

	int myHeapOffsetCBV;
	int myHeapOffsetText;
	int myHeapOffsetAll;

	UINT myWordLength;
	ID3D12Resource* m_texture;

	bool skipNextRender;
	bool firstFrame;
};

