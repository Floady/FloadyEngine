#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include "d3dx12.h"
#include "FVector3.h"
#include "FCamera.h"

class FLightManager
{
public:
	static DirectX::XMFLOAT4X4 GetLightViewProjMatrix(float x = 0.0f, float y = 0.0f, float z = 0.0f);
	static FVector3 GetLightPos();
private:
	FLightManager();
	~FLightManager();
	static FLightManager* ourInstance;
	static FVector3 ourLightPos;
};

