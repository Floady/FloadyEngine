#pragma once
#include <vector>
#include "d3d12.h"
#include "FD3d12Renderer.h"

class FGameEntity;

// different types of highlights and colors?

class FGameHighlightManager
{
public:
	FGameHighlightManager();
	void Render();
	void AddSelectableObject(FGameEntity* anObject);
	void RemoveSelectableObject(FGameEntity* anObject);
	~FGameHighlightManager();
private:
	std::vector<FGameEntity*> myObjects;
	ID3D12Resource* myScratchBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE myScratchBufferView;
	ID3D12RootSignature * myRootSignature;
	ID3D12PipelineState* myPSO;
	ID3D12GraphicsCommandList* myCommandList;

	ID3D12Resource* myVertexBuffer;
	UINT8* myVertexDataBegin;
	D3D12_VERTEX_BUFFER_VIEW myVertexBufferView;

	ID3D12Resource* myProjectionMatrix;
	UINT8* myConstantBufferPtrProjMatrix;
	int heapOffset;

	FD3d12Renderer::GPUMutex myMutex;
};

