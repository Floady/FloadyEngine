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
	struct Light
	{
		FVector3 myPos;
		FVector3 myDir;
		FVector3 myColor;
		float myAlpha;
		unsigned int myId;
	};

	struct SpotLight: public Light
	{
		float myAngle;
		float myRange;
	};

	struct DirectionalLight : public Light
	{
		float myRange;
	};

	DirectX::XMFLOAT4X4 GetSpotlightViewProjMatrix(int i);
	DirectX::XMFLOAT4X4 GetCurrentActiveLightViewProjMatrix();
	DirectX::XMFLOAT4X4 GetDirectionalLightViewProjMatrix(int i);
	
	unsigned int AddSpotlight(FVector3 aPos, FVector3 aDir, float aRadius, FVector3 aColor = FVector3(1, 1, 1), float anAngle = 45.0f);
	unsigned int AddDirectionalLight(FVector3 aPos, FVector3 aDir, FVector3 aColor = FVector3(1, 1, 1), float aRange = 0.0f);
	unsigned int AddLight(FVector3 aPos, float aRadius);
	const std::vector<SpotLight>& GetSpotlights() { return mySpotlights; }
	const std::vector<DirectionalLight>& GetDirectionalLights() { return myDirectionalLights; }

	void SetLightColor(unsigned int aLightId, FVector3 aColor);
	Light* GetLight(unsigned int aLightId);

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
	std::vector<SpotLight> mySpotlights;
	std::vector<DirectionalLight> myDirectionalLights;
	int myActiveLight;
	unsigned int myNextFreeLightId;
};

