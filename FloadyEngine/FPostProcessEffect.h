#pragma once
#include <d3d12.h>
#include <vector>

#include <d3d12.h>
#include <dxgi1_4.h>
#include "FShaderManager.h"
#include "FSceneGraph.h"


struct ID3D12RootSignature;
struct ID3D12PipelineState;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;

class FPostProcessEffect
{
public:
	enum BindBufferType
	{
		Buffer_color = 1 << 0,
		Buffer_normals = 1 << 1,
		Buffer_Depth = 1 << 2,
		Buffer_Shadow = 1 << 3,
		Buffer_Present = 1 << 4,
		Buffer_Count = 5 // FIX WHEN PRESENT BUFFER COPY IS DONE
	};

	struct BindInfo
	{
		BindInfo(ID3D12Resource* aResource, DXGI_FORMAT aResourceFormat) : myResource(aResource), myResourceFormat(aResourceFormat) {}
		ID3D12Resource* myResource;
		DXGI_FORMAT myResourceFormat;
	};


	typedef int BindBufferMask;
	static const BindBufferMask BindBufferMaskAll = Buffer_color | Buffer_normals | Buffer_Depth | Buffer_Shadow | Buffer_Present;

	FPostProcessEffect(BindBufferMask aBindMask, const char* aShaderName, const char* aDebugName = nullptr);
	FPostProcessEffect(const std::vector<BindInfo>& aResourcesToBind, const char* aShaderName, const char* aDebugName = nullptr);
	~FPostProcessEffect();
	void Render();
	void Init(int aPostEffectBufferIdx);
	void SetShader();

protected:
	ID3D12RootSignature* myRootSignature;
	ID3D12PipelineState* myPipelineState;
	ID3D12GraphicsCommandList* myCommandList;
	
	ID3D12Resource* myVertexBuffer;
	UINT8* myVertexDataBegin;
	D3D12_VERTEX_BUFFER_VIEW myVertexBufferView;
	
	bool skipNextRender;
	int myHeapOffsetAll;
	BindBufferMask myBindMask;
	const char* myShaderName;
	const char* myDebugName;

	bool myUseResource;
	struct BindResource
	{
		ID3D12Resource* myResource;
		CD3DX12_CPU_DESCRIPTOR_HANDLE myResourceHandle;
		DXGI_FORMAT myResourceFormat;
	};
	std::vector<BindResource> myResources;
};
