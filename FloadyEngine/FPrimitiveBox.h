#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <dxgi1_4.h>
#include "d3dx12.h"
#include "FVector3.h"
#include "FRenderableObject.h"
#include <string>


class FCamera;
class FD3d12Renderer;

class FPrimitiveBox : public FRenderableObject
{
public:
	enum PrimitiveType
	{
		Box = 0,
		Sphere
	};

	FPrimitiveBox(FD3d12Renderer* aManager, FVector3 aPos, FVector3 aScale, PrimitiveType aType);
	~FPrimitiveBox();
	void Init() override;
	void Render() override;
	void RenderShadows() override;
	void PopulateCommandListAsync() override;
	void PopulateCommandListAsyncShadows() override;
	void PopulateCommandListInternal(ID3D12GraphicsCommandList* aCmdList);
	void PopulateCommandListInternalShadows(ID3D12GraphicsCommandList* aCmdList);
	void SetShader();
	virtual void RecalcModelMatrix() override;
	void SetRotMatrix(float* m) override;

	void SetTexture(const char* aFilename) override;
	const char* GetTexture() override { return myTexName.c_str(); };
	void SetShader(const char* aFilename) override;
	virtual void SetPos(FVector3 aPos) override;

	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() { return m_vertexBufferView; }
	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() { return m_indexBufferView; }
	int GetIndicesCount() { return myIndicesCount; }
	bool IsInitialized() { return myIsInitialized; }

protected:
	ID3D12RootSignature* m_rootSignature;
	ID3D12RootSignature* m_rootSignatureShadows;
	ID3D12PipelineState* m_pipelineState;
	ID3D12PipelineState* m_pipelineStateShadows;
	ID3D12GraphicsCommandList* m_commandList;
	
	float myRotMatrix[16];
	DirectX::XMFLOAT4X4 myModelMatrix;

	UINT8* myConstantBufferPtr;
	UINT8* myConstantBufferShadowsPtr;
	bool myIsMatrixDirty;
	bool myIsGPUConstantDataDirty;
	std::string myTexName;

	ID3D12Resource* m_ModelProjMatrixShadow;
	
	FD3d12Renderer* myManagerClass;
	
	int myHeapOffsetCBV;
	int myHeapOffsetCBVShadow;
	int myHeapOffsetText;
	int myHeapOffsetAll;
	float myYaw;
	bool skipNextRender;
	PrimitiveType myType;

	const char* shaderfilename;
	const char* shaderfilenameShadow;
	bool myIsInitialized;
};

