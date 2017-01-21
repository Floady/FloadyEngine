#include "FLightManager.h"


FLightManager* FLightManager::ourInstance = nullptr;

XMFLOAT4X4 FLightManager::GetLightViewProjMatrix(float x, float y, float z)
{
	float aspectRatio = 800.0f / 600.0f;
	float fov = 60.0f;
	float fovAngleY = fov * XM_PI / 180.0f;

	XMMATRIX myProjMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, 100.0f, 0.01f);
	// calc viewproj from lightpos
	float lightpos[3] = { 8.0, 0, 3.0 };

	FXMVECTOR eye = XMVectorSet(lightpos[0], lightpos[1], lightpos[2], 1);
	FXMVECTOR at = XMVectorSet(0, 0, 1, 1);
	FXMVECTOR up = XMVectorSet(0, 1, 0, 1);

	XMMATRIX mtxRot = XMMatrixRotationRollPitchYaw(0, -3.14 / 2.0f, 0);

	XMVECTOR vUp = XMVector3Transform(up, mtxRot);
	XMVECTOR vAt = XMVector3Transform(at, mtxRot);
	vAt += eye;

	XMMATRIX _viewMatrix = XMMatrixLookAtLH(eye, vAt, vUp);

	XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(x, y, z, 1));

	// combine with stored projection matrix
	XMMATRIX  _viewProjMatrix = XMMatrixTranspose(offset * XMMatrixMultiply(_viewMatrix, myProjMatrix));
	XMFLOAT4X4  myProjMatrixFloatVersion;
	XMStoreFloat4x4(&myProjMatrixFloatVersion, _viewProjMatrix);

	return myProjMatrixFloatVersion;
}

XMFLOAT4X4 FLightManager::GetInvLightViewProjMatrix()
{
	float aspectRatio = 800.0f / 600.0f;
	float fov = 60.0f;
	float fovAngleY = fov * XM_PI / 180.0f;

	XMMATRIX myProjMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, 100.0f, 0.01f);
	// calc viewproj from lightpos
	float lightpos[3] = { 8.0, 0, 3.0 };

	FXMVECTOR eye = XMVectorSet(lightpos[0], lightpos[1], lightpos[2], 1);
	FXMVECTOR at = XMVectorSet(0, 0, 1, 1);
	FXMVECTOR up = XMVectorSet(0, 1, 0, 1);

	XMMATRIX mtxRot = XMMatrixRotationRollPitchYaw(0, -3.14 / 2.0f, 0);

	XMVECTOR vUp = XMVector3Transform(up, mtxRot);
	XMVECTOR vAt = XMVector3Transform(at, mtxRot);
	vAt += eye;

	XMMATRIX _viewMatrix = XMMatrixLookAtLH(eye, vAt, vUp);

	// combine with stored projection matrix
	XMMATRIX  _viewProjMatrix = XMMatrixTranspose(XMMatrixMultiply(_viewMatrix, myProjMatrix));
	XMFLOAT4X4  myProjMatrixFloatVersion;

	XMMATRIX invProj = _viewProjMatrix;
	invProj = XMMatrixInverse(nullptr, invProj);
	XMStoreFloat4x4(&myProjMatrixFloatVersion, invProj);

	return myProjMatrixFloatVersion;
}

FLightManager::FLightManager()
{
}


FLightManager::~FLightManager()
{
}
