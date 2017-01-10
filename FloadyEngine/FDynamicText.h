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
	void Render();
	void PopulateCommandList();
	void PopulateCommandListAsync();
	void SetText(const char* aNewText);
	void SetShader();

private:
	bool myUseKerning;

	ID3D12RootSignature* m_rootSignature;
	ID3D12PipelineState* m_pipelineState;
	ID3D12GraphicsCommandList* m_commandList;
	
	UINT8* myConstantBufferPtr;
	UINT8* pVertexDataBegin;

	ID3D12Resource* m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	ID3D12Resource* m_ModelProjMatrix;
	
	FVector3 myPos;
	const char* myText;
	FD3DClass* myManagerClass;

	int myHeapOffsetCBV;
	int myHeapOffsetText;
	int myHeapOffsetAll;

	UINT myWordLength;
	
	bool skipNextRender;
	bool firstFrame;
};

