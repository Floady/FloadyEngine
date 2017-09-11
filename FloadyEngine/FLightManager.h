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
		virtual FAABB GetAABB() = 0;
		const DirectX::XMFLOAT4X4& GetViewProjMatrix() const { return myViewProjMatrix; }
		float myAlpha;
		unsigned int myId;
		DirectX::XMFLOAT4X4 myViewProjMatrix;
	};

	struct SpotLight: public Light
	{
		FAABB GetAABB();
		float myAngle;
		float myRange;
	};

	struct DirectionalLight : public Light
	{
		FAABB GetAABB();
		float myRange;
	};

	const DirectX::XMFLOAT4X4& GetSpotlightViewProjMatrix(int i) const;
	const DirectX::XMFLOAT4X4& GetCurrentActiveLightViewProjMatrix() const;
	const DirectX::XMFLOAT4X4& GetDirectionalLightViewProjMatrix(int i) const;

	void SortLights();
	void ResetVisibleAABB() { myAABBVisibleFromCam.Reset(); }
	FAABB& GetVisibleAABB() { return myAABBVisibleFromCam; }

	unsigned int AddSpotlight(FVector3 aPos, FVector3 aDir, float aRadius, FVector3 aColor = FVector3(1, 1, 1), float anAngle = 45.0f);
	unsigned int AddDirectionalLight(FVector3 aPos, FVector3 aDir, FVector3 aColor = FVector3(1, 1, 1), float aRange = 0.0f);
	unsigned int AddLight(FVector3 aPos, float aRadius);
	const std::vector<SpotLight>& GetSpotlights() const { return mySpotlights; }
	std::vector<SpotLight>& GetSpotlights() { return mySpotlights; }
	const std::vector<DirectionalLight>& GetDirectionalLights() const { return myDirectionalLights; }
	std::vector<DirectionalLight>& GetDirectionalLights() { return myDirectionalLights; }

	void SetLightColor(unsigned int aLightId, FVector3 aColor);
	void SetLightPos(unsigned int aLightId, FVector3 aPos);
	void RemoveLight(unsigned int aLightId);
	Light* GetLight(unsigned int aLightId);

	void SetDebugDrawEnabled(bool anEnabled) { myDoDebugDraw = anEnabled; }

	static FLightManager* GetInstance() {
		if (!ourInstance)
			ourInstance = new FLightManager();
		return ourInstance;
	}
	void SetActiveLight(int anId) { myActiveLight = anId; }
	int GetActiveLight() { return myActiveLight == -1 ? 0 : myActiveLight + 1; }
	void SetFreezeDebug(bool aShouldFreeze) { myFreezeDebugInfo = aShouldFreeze; }
	void UpdateViewProjMatrices();

private:
	FLightManager();
	~FLightManager();
	static FLightManager* ourInstance;
	std::vector<SpotLight> mySpotlights;
	std::vector<DirectionalLight> myDirectionalLights;
	int myActiveLight;
	unsigned int myNextFreeLightId;
	FAABB myAABBVisibleFromCam;
	bool myDoDebugDraw;

	// debug stuff
	bool myFreezeDebugInfo;
	FVector3 myDebugPos;
	FVector3 myDebugUpvec;
	FVector3 myDebugAtvec;
	FVector3 myDebugDimensions;
	FVector3 myDebugMinCam;
	FVector3 myDebugMaxCam;
	FAABB myDebugAABBVisibleFromCam;

};

