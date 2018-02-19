#pragma once

#include "FRenderableObject.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include "d3dx12.h"
#include "FVector3.h"
#include "FMeshManager.h"
#include <string>

#include <DirectXMath.h>


class FCamera;
class FD3d12Renderer;

struct PerDrawCallData
{
	int myLightIndex;
	float myPadding[63];
};
class FPrimitiveBoxInstanced : public FRenderableObject
{
public:
	enum PrimitiveType
	{
		Box = 0,
		Sphere
	};

	struct PerInstanceData
	{
		PerInstanceData() { myIsVisible = false; }
		DirectX::XMFLOAT4X4 myModelMatrix;
		bool myIsVisible;
	};

	FPrimitiveBoxInstanced(FD3d12Renderer* aManager, FVector3 aPos, FVector3 aScale, PrimitiveType aType, unsigned int aNrOfInstances = 1);
	~FPrimitiveBoxInstanced();
	bool GetIsVisible() override { return true; }
	virtual void Init() override;
	void Render() override;
	void RenderShadows() override;
	void PopulateCommandListAsync() override;
	void PopulateCommandListAsyncShadows() override;
	void PopulateCommandListInternal(ID3D12GraphicsCommandList* aCmdList) override;
	void PopulateCommandListInternalShadows(ID3D12GraphicsCommandList* aCmdList) override;
	void SetShader();
	void RecalcModelMatrix() override;
	void UpdateConstBuffers() override;
	void SetRotMatrix(float* m) override;

	void SetTexture(const char* aFilename) override;
	const char* GetTexture() override { return myTexName.c_str(); };
	void SetShader(const char* aFilename) override;
	virtual void SetPos(const FVector3& aPos) override;

	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() { return m_vertexBufferView; }
	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() { return m_indexBufferView; }
	int GetIndicesCount() { return myIndicesCount; }
	bool IsInitialized() { return myIsInitialized; }
	PerInstanceData& GetInstanceData(int i) { myUsedIndex = max(myUsedIndex, i); myModelMatrix[i].myIsVisible = true; return myModelMatrix[i]; }
	ID3D12RootSignature* m_rootSignature;
	FMeshManager::FMeshObject* myMesh;

protected:
	ID3D12RootSignature* m_rootSignatureShadows;
	ID3D12PipelineState* m_pipelineState;
	ID3D12PipelineState* m_pipelineStateShadows;
	ID3D12GraphicsCommandList* m_commandList;
	int myUsedIndex;
	
	PerInstanceData* myModelMatrix;

	UINT8* myConstantBufferPtr;
	UINT8* myConstantBufferShadowsPtr;
	bool myIsMatrixDirty;
	bool myIsGPUConstantDataDirty;
	std::string myTexName;

	ID3D12Resource* m_ModelProjMatrixShadow;
	
	FD3d12Renderer* myManagerClass;


	UINT8* myShadowPerInstanceDataPtr;
	ID3D12Resource* myShadowPerInstanceData;
	
	int myHeapOffsetCBV;
	int myHeapOffsetCBVShadow;
public:
	int myHeapOffsetText;

protected:
	int myHeapOffsetAll;
	float myYaw;
	bool skipNextRender;
	PrimitiveType myType;

	const char* shaderfilename;
	const char* shaderfilenameShadow;
	bool myIsInitialized;
	unsigned int myNrOfInstances;
	unsigned int myNrOfVisibleInstances;
	FD3d12Renderer::GPUMutex myMutex;
	PerDrawCallData* myPerDrawCallData;
};

