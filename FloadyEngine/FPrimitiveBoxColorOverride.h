#pragma once
#include "FPrimitiveBoxInstanced.h"

struct ID3D12Resource;
class FD3d12Renderer;

class FPrimitiveBoxColorOverride : public FPrimitiveBoxInstanced
{
public:
	void Init() override;
	void SetColor(FVector3 aColor);
	FPrimitiveBoxColorOverride(FD3d12Renderer* aRenderer, FVector3 aPos, FVector3 aScale, FPrimitiveBoxInstanced::PrimitiveType aType);
	~FPrimitiveBoxColorOverride();

protected:
	ID3D12Resource* myConstBuffer;
	UINT8* myConstantBufferPtr2;
};

