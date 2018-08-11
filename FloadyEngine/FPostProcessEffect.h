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
	FPostProcessEffect(const std::vector<BindInfo>& aResourcesToBind, const char* aShaderName, int aNrOfCBV = 0, const char* aDebugName = nullptr);
	~FPostProcessEffect();
	void Render();
	void RenderAsync(ID3D12GraphicsCommandList* aCmdList, bool aRenderToBackBuffer = false);
	void Init(int aPostEffectBufferIdx);
	void SetShader();
	void WriteConstBuffer(int i, float* aData, int aSize);

	const char* myDebugName;
protected:
	ID3D12RootSignature* myRootSignature;
	ID3D12PipelineState* myPipelineState;
	ID3D12PipelineState* myPipelineStateRT; // retarded copy for having a different rendertarget format
	ID3D12GraphicsCommandList* myCommandList;
	
	ID3D12Resource* myVertexBuffer;
	UINT8* myVertexDataBegin;
	D3D12_VERTEX_BUFFER_VIEW myVertexBufferView;
	
	bool skipNextRender;
	int myHeapOffsetAll;
	BindBufferMask myBindMask;
	const char* myShaderName;

	bool myUseResource;
	struct BindResource
	{
		ID3D12Resource* myResource;
		CD3DX12_CPU_DESCRIPTOR_HANDLE myResourceHandle;
		DXGI_FORMAT myResourceFormat;
	};
	std::vector<BindResource> myResources;

	int myNrOfCBV;
	
	struct ConstBuffer
	{
		ID3D12Resource* myConstBuffer;
		UINT8* myConstantBufferPtr;
	};

	std::vector<ConstBuffer> myConstBuffers;
};

