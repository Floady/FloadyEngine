#include "FCamera.h"
#include "FLightManager.h"
#include "FProfiler.h"

using namespace DirectX;

FCamera::FCamera(float aWidth, float aHeight)
	: myPos(0, 0, 0)
	, myDir(0, 0, 1)
	, myUp(0, 1.0f, 0)
	, myNear(0.01f)
	, myFar(400.0f)
	, myOverrideWithLight(false)
{
	myYaw = 0.0f;
	myPitch = 0.0f;

	// proj matrix
	float aspectRatio = aWidth / aHeight;
	myFov = 60.0f;
	myFovY = myFov * XM_PI / 180.0f;

	if (aspectRatio < 1.0f)
		myFovY /= aspectRatio;

	// near + far set here
	// Flipping near and far makes depth precision way better but the object dissapears sooner o.0 - also still has artifacts anyway
	//myProjMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, 1.0f, 100.0f);
	myProjMatrix = XMMatrixPerspectiveFovLH(myFovY, aspectRatio, myFar, myNear);
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

const DirectX::XMFLOAT4X4 & FCamera::GetViewProjMatrix()
{
	return myViewProjMatrix;
}

XMFLOAT4X4 FCamera::GetViewProjMatrixWithOffset(float x, float y, float z, bool transpose)
{
	if(myOverrideWithLight)
		return FLightManager::GetInstance()->GetDirectionalLightViewProjMatrix(0);

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
	XMStoreFloat4x4(&myViewProjMatrixTransposed, XMMatrixTranspose(_viewProjMatrix));
	XMStoreFloat4x4(&myInvViewProjMatrix, XMMatrixTranspose(invProj)); // this one is for hlsl

	// Update frustum

	FVector3 nearPlane = myPos + myDir * myNear;
	FVector3 nearPlaneN = myDir;
	FVector3 farPlane = myPos + myDir * myFar;
	FVector3 farPlaneN = -myDir;

	XMFLOAT4X4& viewProjection = myViewProjMatrix;

	// Left plane
	myFrustum[0].myNormal.x = viewProjection._14 + viewProjection._11;
	myFrustum[0].myNormal.y = viewProjection._24 + viewProjection._21;
	myFrustum[0].myNormal.z = viewProjection._34 + viewProjection._31;
	myFrustum[0].myDistance = viewProjection._44 + viewProjection._41;

	// Right plane
	myFrustum[1].myNormal.x = viewProjection._14 - viewProjection._11;
	myFrustum[1].myNormal.y = viewProjection._24 - viewProjection._21;
	myFrustum[1].myNormal.z = viewProjection._34 - viewProjection._31;
	myFrustum[1].myDistance = viewProjection._44 - viewProjection._41;

	// Top plane
	myFrustum[2].myNormal.x = viewProjection._14 - viewProjection._12;
	myFrustum[2].myNormal.y = viewProjection._24 - viewProjection._22;
	myFrustum[2].myNormal.z = viewProjection._34 - viewProjection._32;
	myFrustum[2].myDistance = viewProjection._44 - viewProjection._42;

	// Bottom plane
	myFrustum[3].myNormal.x = viewProjection._14 + viewProjection._12;
	myFrustum[3].myNormal.y = viewProjection._24 + viewProjection._22;
	myFrustum[3].myNormal.z = viewProjection._34 + viewProjection._32;
	myFrustum[3].myDistance = viewProjection._44 + viewProjection._42;

	// Near plane
	myFrustum[4].myNormal.x = viewProjection._13;
	myFrustum[4].myNormal.y = viewProjection._23;
	myFrustum[4].myNormal.z = viewProjection._33;
	myFrustum[4].myDistance = viewProjection._43;

	// Far plane
	myFrustum[5].myNormal.x = viewProjection._14 - viewProjection._13;
	myFrustum[5].myNormal.y = viewProjection._24 - viewProjection._23;
	myFrustum[5].myNormal.z = viewProjection._34 - viewProjection._33;
	myFrustum[5].myDistance = viewProjection._44 - viewProjection._43;
}

bool FCamera::IsInFrustrum(const FVector3& aPoint) const
{
	return IsInFrustrum(aPoint.x, aPoint.y, aPoint.z);
}

bool FCamera::IsInFrustrum(float x, float y, float z) const
{
	// Check
	for (int i = 0; i < 6; i++)
	{
		//XMVECTOR v = XMPlaneFromPointNormal(XMVectorSet(-myFrustum[i].myDistance * myFrustum[i].myNormal.x, -myFrustum[i].myDistance * myFrustum[i].myNormal.y, -myFrustum[i].myDistance * myFrustum[i].myNormal.z, 1.0f), XMVectorSet(myFrustum[i].myNormal.x, myFrustum[i].myNormal.y, myFrustum[i].myNormal.z, 1.0f));
		XMVECTOR v = XMVectorSet(myFrustum[i].myNormal.x, myFrustum[i].myNormal.y, myFrustum[i].myNormal.z, myFrustum[i].myDistance);
		v = XMPlaneNormalize(v);

		XMVECTOR minx = XMVectorSet(x, y, z, 1.0f);
		XMVECTOR result = XMPlaneDotCoord(v, minx);
		if (XMVectorGetX(result) < 0.0)
		{
			return false;
		}
	}

	return true;
}

bool FCamera::IsInFrustrum(const FAABB& anABB) const
{
	if (IsInFrustrum(anABB.myMin))
		return true;
	if (IsInFrustrum(anABB.myMax))
		return true;
	if (IsInFrustrum(FVector3(anABB.myMin.x, anABB.myMax.y, anABB.myMax.z)))
		return true;
	if (IsInFrustrum(FVector3(anABB.myMax.x, anABB.myMin.y, anABB.myMin.z)))
		return true;
	if (IsInFrustrum(FVector3(anABB.myMin.x, anABB.myMin.y, anABB.myMax.z)))
		return true;						 
	if (IsInFrustrum(FVector3(anABB.myMax.x, anABB.myMin.y, anABB.myMax.z)))
		return true;
	if (IsInFrustrum(FVector3(anABB.myMin.x, anABB.myMax.y, anABB.myMin.z)))
		return true;
	if(IsInFrustrum(FVector3(anABB.myMax.x, anABB.myMax.y, anABB.myMin.z)))
		return true;

	return false; // @todo temp hack until frustum culling is fixed (currently large objects where all points are outside are not correctly included)
}

bool FCamera::IsInFrustrum(FRenderableObject * anEntity) const
{
	FPROFILE_FUNCTION("FCamera IsInFrustrum");

	if (!anEntity)
		return false;

	return IsInFrustrum(anEntity->GetAABB());
}