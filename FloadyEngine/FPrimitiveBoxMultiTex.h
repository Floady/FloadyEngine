#pragma once
#include "FPrimitiveBox.h"
#include "FObjLoader.h"

struct ID3D12Resource;
class FD3d12Renderer;

class FPrimitiveBoxMultiTex : public FPrimitiveBox
{
public:
	void Init() override;
	FPrimitiveBoxMultiTex(FD3d12Renderer* aRenderer, FVector3 aPos, FVector3 aScale, FPrimitiveBox::PrimitiveType aType);
	~FPrimitiveBoxMultiTex();

	FObjLoader::FObjMesh myObjMesh;
};

