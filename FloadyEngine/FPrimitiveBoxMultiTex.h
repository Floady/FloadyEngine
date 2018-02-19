#pragma once
#include "FPrimitiveBox.h"
#include "FPrimitiveBoxInstanced.h"
#include "FObjLoader.h"

struct ID3D12Resource;
class FD3d12Renderer;

class FPrimitiveBoxMultiTex : public FPrimitiveBoxInstanced
{
public:
	void Init() override;
	void ObjectLoadingDone(const FObjLoader::FObjMesh& anObj);
	FPrimitiveBoxMultiTex(FD3d12Renderer* aRenderer, FVector3 aPos, FVector3 aScale, FPrimitiveBoxInstanced::PrimitiveType aType, int aNrOfInstances);
	~FPrimitiveBoxMultiTex();

	FObjLoader::FObjMesh myObjMesh;
	int myTexOffset;
};

