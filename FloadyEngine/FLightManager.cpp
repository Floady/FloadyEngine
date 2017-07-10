#include "FLightManager.h"

using namespace DirectX;

FLightManager* FLightManager::ourInstance = nullptr;

unsigned int FLightManager::AddSpotlight(FVector3 aPos, FVector3 aDir, float aRadius, FVector3 aColor, float anAngle)
{
	SpotLight p;
	p.myId = myNextFreeLightId++;
	p.myPos = aPos;
	p.myRange = aRadius;
	p.myColor = aColor;
	p.myAlpha = 1.0f;
	p.myDir = aDir.Normalized();
	p.myAngle = anAngle * 0.0174533f;// deg -> radian
	mySpotlights.push_back(p);
	
	return p.myId;
}

unsigned int FLightManager::AddDirectionalLight(FVector3 aPos, FVector3 aDir, FVector3 aColor, float aRange)
{
	DirectionalLight light;
	light.myId = myNextFreeLightId++;
	light.myPos = aPos;
	light.myAlpha = 1.0f;
	light.myDir = aDir.Normalized();
	light.myColor = aColor;
	light.myRange = aRange;
	myDirectionalLights.push_back(light);

	return light.myId;
}

unsigned int FLightManager::AddLight(FVector3 aPos, float aRadius)
{
	return AddSpotlight(aPos, FVector3(0, -1, 0.1), aRadius);
}

void FLightManager::SetLightColor(unsigned int aLightId, FVector3 aColor)
{
	for (SpotLight& light : mySpotlights)
	{
		if (light.myId == aLightId)
		{
			light.myColor = aColor;
			return;
		}
	}

	for (DirectionalLight& light : myDirectionalLights)
	{
		if (light.myId == aLightId)
		{
			light.myColor = aColor;
			return;
		}
	}
}

FLightManager::Light* FLightManager::GetLight(unsigned int aLightId)
{
	for (SpotLight& light : mySpotlights)
	{
		if (light.myId == aLightId)
		{
			return &light;
		}
	}

	for (DirectionalLight& light : myDirectionalLights)
	{
		if (light.myId == aLightId)
		{
			return &light;
		}
	}
}

XMFLOAT4X4 FLightManager::GetSpotlightViewProjMatrix(int i)
{
	float aspectRatio = 800.0f / 600.0f;
	float fov = 45.0f;
	float fovAngleY = mySpotlights[i].myAngle * 2;

	XMMATRIX myProjMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, 40.0f, 0.01f);
	
	// calc viewproj from lightpos
	FXMVECTOR eye = XMVectorSet(mySpotlights[i].myPos.x, mySpotlights[i].myPos.y, mySpotlights[i].myPos.z, 1);
	FXMVECTOR at = XMVectorSet(mySpotlights[i].myDir.x, mySpotlights[i].myDir.y, mySpotlights[i].myDir.z, 1);
	FXMVECTOR up = XMVectorSet(0, 1, 0, 1);

	XMVECTOR vUp = up;
	XMVECTOR vAt = at;
	vAt += eye;

	XMMATRIX _viewMatrix = XMMatrixLookAtLH(eye, vAt, vUp);

	XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(0, 0, 0, 1));
	XMMATRIX  _viewProjMatrix = XMMatrixTranspose(_viewMatrix * myProjMatrix);
	XMFLOAT4X4  myProjMatrixFloatVersion;
	XMStoreFloat4x4(&myProjMatrixFloatVersion, _viewProjMatrix);

	return myProjMatrixFloatVersion;
}

DirectX::XMFLOAT4X4 FLightManager::GetCurrentActiveLightViewProjMatrix()
{
	if(myActiveLight == -1)
		return GetDirectionalLightViewProjMatrix(0); // todo: fix

	if(myActiveLight >= mySpotlights.size())
		return DirectX::XMFLOAT4X4();
	
	return GetSpotlightViewProjMatrix(myActiveLight);
}

DirectX::XMFLOAT4X4 FLightManager::GetDirectionalLightViewProjMatrix(int i)
{
	XMMATRIX myProjMatrix = XMMatrixOrthographicLH(100.0f, 100.0f, 200.0f, 0.001f);

	FVector3& pos = myDirectionalLights[i].myPos;
	FXMVECTOR eye = XMVectorSet(pos.x, pos.y, pos.z, 1);
	FXMVECTOR at = XMVectorSet(0, 0, 1, 0);
	FXMVECTOR up = XMVectorSet(0, 1, 0, 1);

	XMMATRIX mtxRot = XMMatrixRotationRollPitchYaw(PI / 4.0f, 0, 0);

	XMVECTOR vUp = XMVector3Transform(up, mtxRot);
	XMVECTOR vAt = XMVector3Transform(at, mtxRot);
	vAt += eye;

	XMMATRIX _viewMatrix = XMMatrixLookAtLH(eye, vAt, vUp);

	XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(0, 0, 0, 1));

	// combine with stored projection matrix
	XMMATRIX  _viewProjMatrix = XMMatrixTranspose(offset * _viewMatrix * myProjMatrix);
	XMFLOAT4X4  myProjMatrixFloatVersion;
	XMStoreFloat4x4(&myProjMatrixFloatVersion, _viewProjMatrix);

	return myProjMatrixFloatVersion;
}


FLightManager::FLightManager()
	: myNextFreeLightId(1)
{
}


FLightManager::~FLightManager()
{
}
