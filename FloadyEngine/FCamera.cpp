#include "FCamera.h"

using namespace DirectX;

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
	// Flipping near and far makes depth precision way better but the object dissapears sooner o.0 - also still has artifacts anyway
	//myProjMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, 1.0f, 100.0f);
	myProjMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, 60.0f, 0.01f);
	XMStoreFloat4x4(&myProjMatrixFloatVersion, myProjMatrix);
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

XMFLOAT4X4 FCamera::GetViewProjMatrixWithOffset(float x, float y, float z, bool transpose)
{
	FXMVECTOR eye = XMVectorSet(myPos.x, myPos.y, myPos.z, 1);
	FXMVECTOR at = XMVectorSet(myDir.x, myDir.y, myDir.z, 1);
	FXMVECTOR up = XMVectorSet(myUp.x, myUp.y, myUp.z, 1);

	XMMATRIX mtxRot = XMMatrixRotationRollPitchYaw(myPitch, myYaw, 0);

	XMVECTOR vUp = XMVector3Transform(up, mtxRot);
	XMVECTOR vAt = XMVector3Transform(at, mtxRot);
	vAt += eye;

	_viewMatrix = XMMatrixLookAtLH(eye, vAt, vUp);
	// test :)
	//XMMATRIX scale = XMMatrixScaling(3.0f, 1.0f, 1.0f);
	XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(x, y, z, 1));
	//offset = offset * scale;
	XMMATRIX _tempviewProjMatrix;
	
	// combine
	if(transpose)
		_tempviewProjMatrix = XMMatrixTranspose(offset * _viewMatrix * myProjMatrix); // transpose cause it will be going to HLSL (do this externally in the future)
	else
		_tempviewProjMatrix = (offset * _viewMatrix * myProjMatrix);

	XMFLOAT4X4 ret;
	// store
	XMStoreFloat4x4(&ret, _tempviewProjMatrix);

	return ret;
}

XMFLOAT4X4 FCamera::GetViewProjMatrixWithOffset(const XMMATRIX& anObjectMatrix)
{
	FXMVECTOR eye = XMVectorSet(myPos.x, myPos.y, myPos.z, 1);
	FXMVECTOR at = XMVectorSet(myDir.x, myDir.y, myDir.z, 1);
	FXMVECTOR up = XMVectorSet(myUp.x, myUp.y, myUp.z, 1);

	XMMATRIX mtxRot = XMMatrixRotationRollPitchYaw(myPitch, myYaw, 0);

	XMVECTOR vUp = XMVector3Transform(up, mtxRot);
	XMVECTOR vAt = XMVector3Transform(at, mtxRot);
	vAt += eye;

	_viewMatrix = XMMatrixLookAtLH(eye, vAt, vUp);
	XMMATRIX _tempviewProjMatrix;

	// combine
	_tempviewProjMatrix = XMMatrixTranspose(anObjectMatrix * _viewMatrix * myProjMatrix); // transpose cause it will be going to HLSL (do this externally in the future)

	XMFLOAT4X4 ret;
	// store
	XMStoreFloat4x4(&ret, _tempviewProjMatrix);

	return ret;
}

void FCamera::UpdateViewProj()
{
	// view matrix
	FXMVECTOR eye = XMVectorSet(myPos.x, myPos.y, myPos.z, 0);
	FXMVECTOR at = XMVectorSet(myDir.x, myDir.y, myDir.z, 0);
	FXMVECTOR up = XMVectorSet(myUp.x, myUp.y, myUp.z, 0);

	XMMATRIX mtxRot = XMMatrixRotationRollPitchYaw(myPitch, myYaw, 0);

	XMVECTOR vUp = XMVector3Transform(up, mtxRot);
	XMVECTOR vAt = XMVector3Transform(at, mtxRot);
	vAt += eye;

	_viewMatrix = XMMatrixLookAtLH(eye, vAt, vUp);

	// combine with stored projection matrix
	_viewProjMatrix = XMMatrixMultiply(_viewMatrix, myProjMatrix);
	XMMATRIX _viewProjMatrix2 = _viewMatrix * myProjMatrix;

	XMMATRIX invProj = _viewProjMatrix;
	invProj = XMMatrixInverse(nullptr, invProj);

	// store
	XMStoreFloat4x4(&myViewProjMatrix, (_viewProjMatrix));
	XMStoreFloat4x4(&myInvViewProjMatrix, XMMatrixTranspose(invProj)); // this one is for hlsl
}
