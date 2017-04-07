#pragma once
#include "FVector3.h"

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
	virtual void SetPos(FVector3 aPos) {  }
	virtual void SetRotMatrix(float* m) { }
	FRenderableObject();
	virtual ~FRenderableObject();
};

