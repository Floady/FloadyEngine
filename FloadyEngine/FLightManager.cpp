#include "FLightManager.h"

using namespace DirectX;

FLightManager* FLightManager::ourInstance = nullptr;
FVector3 FLightManager::ourLightPos = FVector3(0.0, 5.0f, -10.0); // debugdraw frustrum with lines todo


FVector3 FLightManager::GetLightPos()
{
	return ourLightPos;
}

XMFLOAT4X4 FLightManager::GetLightViewProjMatrix(float x, float y, float z)
{
	float aspectRatio = 800.0f / 600.0f;
	float fov = 60.0f;
	float fovAngleY = fov * XM_PI / 180.0f;

	//XMMATRIX myProjMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, 100.0f, 0.01f);
	XMMATRIX myProjMatrix = XMMatrixOrthographicLH(20.0f, 15.0f, 20.0f, 0.01f);
	// calc viewproj from lightpos

	FXMVECTOR eye = XMVectorSet(ourLightPos.x, ourLightPos.y, ourLightPos.z, 1);
	FXMVECTOR at = XMVectorSet(0, 0, 1, 0);
	FXMVECTOR up = XMVectorSet(0, 1, 0, 1);

	XMMATRIX mtxRot = XMMatrixRotationRollPitchYaw(PI/8.0f, 0, 0);

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
}


FLightManager::~FLightManager()
{
}
