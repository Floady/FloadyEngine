#include "FLightManager.h"

using namespace DirectX;

FLightManager* FLightManager::ourInstance = nullptr;

FVector3 FLightManager::GetLightPos()
{
	return myLightPos;
}

void FLightManager::AddLight(FVector3 aPos, float aRadius)
{
	PointLight p;
	p.myPos = aPos;
	p.myRadius = aRadius;
	myPointLights.push_back(p);
}

XMFLOAT4X4 FLightManager::GetLightViewProjMatrix(float x, float y, float z)
{
	float aspectRatio = 800.0f / 600.0f;
	float fov = 90.0f;
	float fovAngleY = fov * XM_PI / 180.0f;

	XMMATRIX myProjMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, 150.0f, 0.01f);
//	XMMATRIX myProjMatrix = XMMatrixOrthographicLH(20.0f, 15.0f, 200.0f, 0.01f);
	// calc viewproj from lightpos

	FXMVECTOR eye = XMVectorSet(myPointLights[0].myPos.x, myPointLights[0].myPos.y, myPointLights[0].myPos.z, 1);
	//FXMVECTOR eye = XMVectorSet(0, 0, 0, 1);
	FXMVECTOR at = XMVectorSet(0, 0, 1, 0);
	FXMVECTOR up = XMVectorSet(0, 1, 0, 1);
	
	XMMATRIX mtxRot = XMMatrixRotationRollPitchYaw(PI / 2.0f, 0, 0);

	XMVECTOR vUp = XMVector3Transform(up, mtxRot);
	XMVECTOR vAt = XMVector3Transform(at, mtxRot);
	vAt += eye;

	XMMATRIX _viewMatrix = XMMatrixLookAtLH(eye, vAt, vUp);

	XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(x, y, z, 1));

	// combine with stored projection matrix
	XMMATRIX  _viewProjMatrix = XMMatrixTranspose(offset * _viewMatrix * myProjMatrix);
	//XMMATRIX  _viewProjMatrix = XMMatrixTranspose(offset * myProjMatrix);
	XMFLOAT4X4  myProjMatrixFloatVersion;
	XMStoreFloat4x4(&myProjMatrixFloatVersion, _viewProjMatrix);

	return myProjMatrixFloatVersion;
}

DirectX::XMFLOAT4X4 FLightManager::GetCurrentActiveLightViewProjMatrix()
{
	if(myActiveLight == -1)
		return GetLightViewProjMatrixOrtho(0, 0, 0);

	if(myActiveLight >= myPointLights.size())
		return DirectX::XMFLOAT4X4();
	
	float aspectRatio = 800.0f / 600.0f;
	float fov = 90.0f;
	float fovAngleY = fov * XM_PI / 180.0f;

	XMMATRIX myProjMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, 150.0f, 0.01f);
	// calc viewproj from lightpos

	FXMVECTOR eye = XMVectorSet(myPointLights[myActiveLight].myPos.x, myPointLights[myActiveLight].myPos.y, myPointLights[myActiveLight].myPos.z, 1);
	FXMVECTOR at = XMVectorSet(0, 0, 1, 0);
	FXMVECTOR up = XMVectorSet(0, 1, 0, 1);

	XMMATRIX mtxRot = XMMatrixRotationRollPitchYaw(PI / 2.0f, 0, 0);

	XMVECTOR vUp = XMVector3Transform(up, mtxRot);
	XMVECTOR vAt = XMVector3Transform(at, mtxRot);
	vAt += eye;

	XMMATRIX _viewMatrix = XMMatrixLookAtLH(eye, vAt, vUp);

	XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(0,0,0, 1));

	// combine with stored projection matrix
	XMMATRIX  _viewProjMatrix = XMMatrixTranspose(offset * _viewMatrix * myProjMatrix);
	//XMMATRIX  _viewProjMatrix = XMMatrixTranspose(offset * myProjMatrix);
	XMFLOAT4X4  myProjMatrixFloatVersion;
	XMStoreFloat4x4(&myProjMatrixFloatVersion, _viewProjMatrix);

	return myProjMatrixFloatVersion;
}

XMFLOAT4X4 FLightManager::GetLightViewProjMatrixOrtho(float x, float y, float z)
{
	float aspectRatio = 800.0f / 600.0f;
	float fov = 90.0f;
	float fovAngleY = fov * XM_PI / 180.0f;

	//XMMATRIX myProjMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, 150.0f, 0.01f);
	XMMATRIX myProjMatrix = XMMatrixOrthographicLH(20.0f, 15.0f, 200.0f, 0.01f);
	// calc viewproj from lightpos

	FXMVECTOR eye = XMVectorSet(0, 0, 0, 1);
	FXMVECTOR at = XMVectorSet(0, 0, 1, 0);
	FXMVECTOR up = XMVectorSet(0, 1, 0, 1);

	XMMATRIX mtxRot = XMMatrixRotationRollPitchYaw(PI / 8.0f, 0, 0);

	XMVECTOR vUp = XMVector3Transform(up, mtxRot);
	XMVECTOR vAt = XMVector3Transform(at, mtxRot);
	vAt += eye;

	XMMATRIX _viewMatrix = XMMatrixLookAtLH(eye, vAt, vUp);

	XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(x, y, z, 1));

	// combine with stored projection matrix
	XMMATRIX  _viewProjMatrix = XMMatrixTranspose(offset * _viewMatrix * myProjMatrix);
	//XMMATRIX  _viewProjMatrix = XMMatrixTranspose(offset * myProjMatrix);
	XMFLOAT4X4  myProjMatrixFloatVersion;
	XMStoreFloat4x4(&myProjMatrixFloatVersion, _viewProjMatrix);

	return myProjMatrixFloatVersion;
}


FLightManager::FLightManager()
{
	myLightPos = FVector3(0.0, 10.0f, -15.0); // debugdraw frustrum with lines todo
}


FLightManager::~FLightManager()
{
}
