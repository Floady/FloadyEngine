#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include "d3dx12.h"
#include "FVector3.h"
#include "FCamera.h"
#include <vector>

class FLightManager
{
public:
	struct PointLight
	{
		FVector3 myPos;
		float myRadius;
	};

	DirectX::XMFLOAT4X4 GetSpotlightViewProjMatrix(int i);
	DirectX::XMFLOAT4X4 GetCurrentActiveLightViewProjMatrix();
	DirectX::XMFLOAT4X4 GetLightViewProjMatrixOrtho(float x = 0.0f, float y = 0.0f, float z = 0.0f);
	FVector3 GetLightPos();
	void AddLight(FVector3 aPos, float aRadius);
	const std::vector<PointLight>& GetPointLights() { return myPointLights; }
	static FLightManager* GetInstance() {
		if (!ourInstance)
			ourInstance = new FLightManager();
		return ourInstance;
	}
	void SetActiveLight(int anId) { myActiveLight = anId; }
	int GetActiveLight() { return myActiveLight == -1 ? 0 : myActiveLight + 1; }

private:
	FLightManager();
	~FLightManager();
	static FLightManager* ourInstance;
	FVector3 myLightPos;
	std::vector<PointLight> myPointLights;
	int myActiveLight;
};

