#pragma once
#include "FPrimitiveBoxInstanced.h"

struct ID3D12Resource;
class FD3d12Renderer;

class FPrimitiveBoxMultiTex : public FPrimitiveBoxInstanced
{
public:
	void Init() override;
	void ObjectLoadingDone(const FMeshManager::FMeshLoadObject& anObj);
	FPrimitiveBoxMultiTex(FD3d12Renderer* aRenderer, FVector3 aPos, FVector3 aScale, FPrimitiveBoxInstanced::PrimitiveType aType, int aNrOfInstances);
	~FPrimitiveBoxMultiTex();

	int myTexOffset;
};

