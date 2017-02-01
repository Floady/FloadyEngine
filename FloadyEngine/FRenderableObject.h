#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <dxgi1_4.h>
#include "d3dx12.h"
#include "FVector3.h"

using namespace DirectX;

class FRenderableObject
{
public:
	virtual void Init() = 0;
	virtual void Render() = 0;
	virtual void RenderShadows() = 0;
	virtual void PopulateCommandListAsync() = 0;
	virtual void PopulateCommandListAsyncShadows() = 0;

	// todo: this needs some base implementation (pull modelmatrix here, any renderable has a matrix to set and to render with)
	virtual void SetPos(FVector3 aPos) {  }
	virtual void SetRotMatrix(XMMATRIX& m) {  }
	virtual void SetRotMatrix(XMFLOAT4X4* m) { }
	FRenderableObject();
	virtual ~FRenderableObject();
};

