#include "FCamera.h"


FCamera::FCamera(float aWidth, float aHeight)
	: myPos(0, 0, 0)
	, myDir(0, 0, 1)
	, myUp(0, 1.0f, 0)
{
	myYaw = 0.0f;
	myPitch = 0.0f;

	// proj matrix
	float aspectRatio = aWidth / aHeight;
	float fov = 60.0f;
	float fovAngleY = fov * XM_PI / 180.0f;

	if (aspectRatio < 1.0f)
		fovAngleY /= aspectRatio;

	// near + far set here
	myProjMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, 0.01f, 18.0f);
	UpdateViewProj();
}

FCamera::~FCamera()
{
}

void FCamera::SetPos(float x, float y, float z)
{
	myPos.x = x;
	myPos.y = y;
	myPos.z = z;
}

void FCamera::Move(float x, float y, float z)
{
	FXMVECTOR up = XMVectorSet(x, y, z, 0);
	XMMATRIX mtxRot = XMMatrixRotationRollPitchYaw(myPitch, myYaw, 0);
	XMVECTOR vUp = XMVector3Transform(up, mtxRot);
	
	// rotate xyz to rotation before doing this (move in camera space, not in world -.-)
	myPos.x += vUp.m128_f32[0];
	myPos.y += vUp.m128_f32[1];
	myPos.z += vUp.m128_f32[2];
}

void FCamera::Yaw(float angle)
{
	myYaw += angle;
}

void FCamera::Pitch(float angle)
{
	myPitch += angle;
}

XMFLOAT4X4 FCamera::GetViewProjMatrixWithOffset(float x, float y, float z)
{
	FXMVECTOR eye = XMVectorSet(myPos.x, myPos.y, myPos.z, 1);
	FXMVECTOR at = XMVectorSet(myDir.x, myDir.y, myDir.z, 1);
	FXMVECTOR up = XMVectorSet(myUp.x, myUp.y, myUp.z, 1);

	XMMATRIX mtxRot = XMMatrixRotationRollPitchYaw(myPitch, myYaw, 0);

	XMVECTOR vUp = XMVector3Transform(up, mtxRot);
	XMVECTOR vAt = XMVector3Transform(at, mtxRot);
	vAt += eye;

	XMMATRIX _viewMatrix = XMMatrixLookAtLH(eye, vAt, vUp);
	XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(x, y, z, 1));

	// combine
	XMMATRIX _viewProjMatrix = XMMatrixTranspose(offset * _viewMatrix * myProjMatrix);
	
	XMFLOAT4X4 ret;
	// store
	XMStoreFloat4x4(&ret, _viewProjMatrix);

	return ret;
}

void FCamera::UpdateViewProj()
{
	// view matrix
	XMFLOAT4X4 viewMatrix;

	FXMVECTOR eye = XMVectorSet(myPos.x, myPos.y, myPos.z, 0);
	FXMVECTOR at = XMVectorSet(myDir.x, myDir.y, myDir.z, 0);
	FXMVECTOR up = XMVectorSet(myUp.x, myUp.y, myUp.z, 0);

	XMMATRIX mtxRot = XMMatrixRotationRollPitchYaw(myPitch, myYaw, 0);

	XMVECTOR vUp = XMVector3Transform(up, mtxRot);
	XMVECTOR vAt = XMVector3Transform(at, mtxRot);
	vAt += eye;

	XMMATRIX _tviewMatrix = XMMatrixLookAtLH(eye, vAt, vUp);
	XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(0, 0, 0, 0)); // ? wtf 
	XMStoreFloat4x4(&viewMatrix, offset);
	XMMATRIX _viewMatrix = XMMatrixMultiply(offset, _tviewMatrix);

	// combine with stored projection matrix
	XMMATRIX _viewProjMatrix = XMMatrixMultiply(myProjMatrix, (_viewMatrix));

	// store
	XMStoreFloat4x4(&myViewProjMatrix, _viewProjMatrix);
}
