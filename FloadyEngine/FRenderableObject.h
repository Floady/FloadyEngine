#pragma once
#include "FVector3.h"
#include <d3d12.h>

struct ID3D12Resource;
class FRenderableObject
{
public:
	virtual void Init() = 0;
	virtual void Render() = 0;
	virtual void RenderShadows() = 0;
	virtual void PopulateCommandListAsync() = 0;
	virtual void PopulateCommandListAsyncShadows() = 0;

	virtual void SetTexture(const char* aFilename) = 0;
	virtual void SetShader(const char* aFilename) = 0;

	// todo: this needs some base implementation (pull modelmatrix here, any renderable has a matrix to set and to render with)
	virtual void SetPos(FVector3 aPos) { myPos = aPos; }
	FVector3 GetPos() { return myPos; }
	FVector3 GetScale() { return myScale; }
	virtual void SetRotMatrix(float* m) { }
	FRenderableObject();
	virtual ~FRenderableObject();

	ID3D12Resource* GetModelViewMatrix() { return m_ModelProjMatrix; }
public:
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	int myIndicesCount;

protected:
	ID3D12Resource* m_ModelProjMatrix;
	FVector3 myPos;
	FVector3 myScale;
};

