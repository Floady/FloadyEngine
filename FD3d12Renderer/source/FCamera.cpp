#include "FCamera.h"
#include "FLightManager.h"
#include "FProfiler.h"
#include "FDebugDrawer.h"
#include "FD3d12Renderer.h"
#include <DirectXMath.h>
#include "FUtilities.h"

using namespace DirectX;
float aspecto = 1.0f;

FCamera::FCamera(float aWidth, float aHeight)
	: myPos(0, 0, 0)
	, myDir(0, 0, 1)
	, myUp(0, 1.0f, 0)
	, myNear(0.01f)
	, myFar(400.0f)
	, myOverrideWithLight(false)
	, myDoDebugDraw(false)
	, myFreezeDebugInfo(false)
{
	myYaw = 0.0f;
	myPitch = 0.0f;

	// proj matrix
	float aspectRatio = aWidth / aHeight;
	myFov = 75.0f;
	myFovY = myFov * XM_PI / 180.0f;

	if (aspectRatio < 1.0f)
		myFovY /= aspectRatio;
	
	aspecto = aspectRatio;

	// near + far set here
	// Flipping near and far makes depth precision way better but the object dissapears sooner o.0 - also still has artifacts anyway
	//myProjMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, 1.0f, 100.0f);
	XMMATRIX projMatrix = XMMatrixPerspectiveFovLH(myFovY, aspectRatio, myFar*2, myNear);

	// store
	XMFLOAT4X4 ret;
	XMStoreFloat4x4(&ret, projMatrix);
	memcpy(myProjMatrix2.cell, ret.m, sizeof(float) * 16);

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

const FMatrix& FCamera::GetViewProjMatrix() const
{
	return myViewProjMatrix;
}

const FMatrix & FCamera::GetViewMatrix() const
{
	return myViewMatrix2;
}

const FMatrix& FCamera::GetInvViewProjMatrix2() const
{
	return myInvViewProjMatrix2;
}

const FMatrix& FCamera::GetInvViewProjMatrix3() const
{
	return myInvViewProjMatrix3;
}

const FMatrix & FCamera::GetProjMatrix() const
{
	return myProjMatrix2;
}

FMatrix FCamera::GetViewProjMatrixWithOffset2(float x, float y, float z)
{
	FXMVECTOR eye = XMVectorSet(myPos.x, myPos.y, myPos.z, 1);
	FXMVECTOR at = XMVectorSet(myDir.x, myDir.y, myDir.z, 1);
	FXMVECTOR up = XMVectorSet(myUp.x, myUp.y, myUp.z, 1);

	XMMATRIX mtxRot = XMMatrixRotationRollPitchYaw(myPitch, myYaw, 0);

	XMVECTOR vUp = XMVector3Transform(up, mtxRot);
	XMVECTOR vAt = XMVector3Transform(at, mtxRot);
	vAt += eye;

	XMMATRIX _viewMatrix = XMMatrixLookAtLH(eye, vAt, vUp);
	// test :)
	//XMMATRIX scale = XMMatrixScaling(3.0f, 1.0f, 1.0f);
	XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(x, y, z, 1));
	//offset = offset * scale;
	XMMATRIX _tempviewProjMatrix;

	// combine
	XMFLOAT4X4 m = XMFLOAT4X4(myProjMatrix2.cell);
	XMMATRIX _projMatrix = XMLoadFloat4x4(&m);
	_tempviewProjMatrix = XMMatrixTranspose(offset * _viewMatrix * _projMatrix); // transpose cause it will be going to HLSL (do this externally in the future)
	
	XMFLOAT4X4 ret;
	// store
	XMStoreFloat4x4(&ret, _tempviewProjMatrix);

	FMatrix result;
	memcpy(result.cell, ret.m, sizeof(float) * 4 * 4);

	return result;
}

FVector3 shortenLength(FVector3 A, float reductionLength)
{
	FVector3 B = A;
	B.Normalize();
	B = B * (A.Length() - reductionLength);
	return B;
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

	XMMATRIX _viewMatrix = XMMatrixLookAtLH(eye, vAt, vUp);

	XMFLOAT4X4 viewTmp;
	XMStoreFloat4x4(&viewTmp, (_viewMatrix));
	memcpy(myViewMatrix2.cell, viewTmp.m, sizeof(float) * 16);

	// combine with stored projection matrix
	XMFLOAT4X4 m = XMFLOAT4X4(myProjMatrix2.cell);
	XMMATRIX _projMatrix = XMLoadFloat4x4(&m);
	XMMATRIX _viewProjMatrix = XMMatrixMultiply(_viewMatrix, _projMatrix);
	XMFLOAT4X4 viewProjTmp;
	XMStoreFloat4x4(&viewProjTmp, (_viewProjMatrix));
	memcpy(myViewProjMatrix.cell, viewProjTmp.m, sizeof(float) * 16);
	
	XMMATRIX invProj = _viewProjMatrix;
	invProj = XMMatrixInverse(nullptr, invProj);

	// store
	XMFLOAT4X4 invViewProjection;
	XMStoreFloat4x4(&invViewProjection, invProj); // this one is for hlsl
	memcpy(myInvViewProjMatrix3.cell, invViewProjection.m, sizeof(float) * 16);
	XMStoreFloat4x4(&invViewProjection, XMMatrixTranspose(invProj)); // this one is for hlsl
	memcpy(&myInvViewProjMatrix2.cell[0], invViewProjection.m, sizeof(float) * 16);

	// Update frustum

	FVector3 nearPlane = myPos + myDir * myNear;
	FVector3 nearPlaneN = myDir;
	FVector3 farPlane = myPos + myDir * myFar;
	FVector3 farPlaneN = -myDir;

	XMMATRIX view = XMMatrixLookAtLH(eye, vAt, vUp);
	XMMATRIX proj = XMMatrixPerspectiveFovLH(myFovY, aspecto, myFar, myNear);
	XMMATRIX viewProj = view * proj;
	XMFLOAT4X4 viewProjection;

	XMStoreFloat4x4(&viewProjection, (viewProj));

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
	
	for (size_t i = 0; i < 6; i++)
	{
		XMVECTOR v = XMVectorSet(myFrustum[i].myNormal.x, myFrustum[i].myNormal.y, myFrustum[i].myNormal.z, myFrustum[i].myDistance);
		v = XMPlaneNormalize(v);
		myFrustum[i].myNormal = FVector3(XMVectorGetX(v), XMVectorGetY(v), XMVectorGetZ(v));
		myFrustum[i].myDistance = XMVectorGetW(v);
	}

	FVector3 P = myDebugPos;
	FVector3 v = myDebugAt;
	FVector3 up2 = myDebugUp;
	float nDis = myNear;
	float fDis = myFar;
	float fov = myFovY;
	float ar = aspecto;
	float Hnear = 2 * tan(fov / 2) * nDis;
	float Wnear = Hnear * ar;
	float Hfar = 2 * tan(fov / 2) * fDis;
	float Wfar = Hfar * ar;
	FVector3 Cnear = P + v * nDis;
	FVector3 Cfar = P + v * fDis;
	FVector3 w = v.Cross(up2);
	float size = 0.6f;
	FVector3 color = FVector3(1, 1, 0.2f);

	myFrustumCorners[0] = Cnear + (up2 * (Hnear / 2)) - (w * (Wnear / 2));
	myFrustumCorners[1] = Cnear + (up2 * (Hnear / 2)) + (w * (Wnear / 2));
	myFrustumCorners[2] = Cnear - (up2 * (Hnear / 2)) - (w * (Wnear / 2));
	myFrustumCorners[3] = Cnear - (up2 * (Hnear / 2)) + (w * (Wnear / 2));
	myFrustumCorners[4] = Cfar + (up2 * (Hfar / 2)) - (w * (Wfar / 2));
	myFrustumCorners[5] = Cfar + (up2 * (Hfar / 2)) + (w * (Wfar / 2));
	myFrustumCorners[6] = Cfar - (up2 * (Hfar / 2)) - (w * (Wfar / 2));
	myFrustumCorners[7] = Cfar - (up2 * (Hfar / 2)) + (w * (Wfar / 2));

	if (!myFreezeDebugInfo)
	{
		for (size_t i = 0; i < 6; i++)
		{
			myDebugFrustum[i] = myFrustum[i];
		}

		myDebugPos = myPos;
		myDebugUp = FVector3(XMVectorGetX(vUp), XMVectorGetY(vUp), XMVectorGetZ(vUp));
		myDebugUp.Normalize();

		XMVECTOR vAt2 = XMVector3Transform(at, mtxRot);
		myDebugAt = FVector3(XMVectorGetX(vAt2), XMVectorGetY(vAt2), XMVectorGetZ(vAt2));
		myDebugAt.Normalize();

		for (int i = 0; i < 8; i++)
		{
			myDebugFrustumCorners[i] = myFrustumCorners[i];
		}
	}

	if (myDoDebugDraw)
	{
		FDebugDrawer* debugDrawer = FD3d12Renderer::GetInstance()->GetDebugDrawer();
		if (debugDrawer)
		{
			// point alternative
			{
				size = 2.0f;
				FVector3 colorCorners = FVector3(0, 1, 0);
				debugDrawer->DrawPoint(myFrustumCorners[4], size, colorCorners);
				debugDrawer->DrawPoint(myFrustumCorners[5], size, colorCorners);
				debugDrawer->DrawPoint(myFrustumCorners[6], size, colorCorners);
				debugDrawer->DrawPoint(myFrustumCorners[7], size, colorCorners);
				size = 0.6f;
			}

			for (size_t i = 0; i < 6; i++)
			{
				const FPlane& frustum = myDebugFrustum[i];
				FVector3 planeCenter = (-frustum.myNormal * frustum.myDistance);
				debugDrawer->drawLine(myDebugPos, planeCenter, FVector3(1, 0, 0));
				FLOG("dist: %f", frustum.myDistance);
				FLOG("normal: %f %f %f", frustum.myNormal.x, frustum.myNormal.y, frustum.myNormal.z);
				FVector3 biNormal = frustum.myNormal;
				FVector3 up(0, 1, 0);
				up = biNormal.Cross(up);
				biNormal = biNormal.Cross(up);
				biNormal.Normalize();
				FVector3 tangent = biNormal.Cross(frustum.myNormal);
				tangent.Normalize();
				float length = 20.0f;
				float dotResult = (planeCenter - myDebugPos).Dot(-biNormal);
				float dotResult2 = (planeCenter - myDebugPos).Dot(-tangent);
				FVector3 planeCenter2 = planeCenter;
				FVector3 comb = shortenLength((biNormal*dotResult + tangent*dotResult2), 5+length/2);
				planeCenter2 += comb;

				FVector3 TR = planeCenter2 + (biNormal * length) + (tangent * length);
				FVector3 TL = planeCenter2 - (biNormal * length) + (tangent * length);
				FVector3 BR = planeCenter2 + (biNormal * length) - (tangent * length);
				FVector3 BL = planeCenter2 - (biNormal * length) - (tangent * length);
				debugDrawer->drawLine(TL, TR, FVector3(1, 1, 0));
				debugDrawer->drawLine(TR, BR, FVector3(1, 1, 0));
				debugDrawer->drawLine(BL, BR, FVector3(1, 1, 0));
				debugDrawer->drawLine(BL, TL, FVector3(1, 1, 0));
				FVector3 normalDebug = frustum.myNormal;
				normalDebug.Normalize();
				debugDrawer->drawLine(planeCenter2, planeCenter2 + normalDebug, FVector3(0, 1, 0));
				debugDrawer->drawLine(planeCenter2, planeCenter2 + tangent, FVector3(1, 0, 0));
				debugDrawer->drawLine(planeCenter2, planeCenter2 + biNormal, FVector3(0, 0, 1));
			}
		}
	}
}

bool FCamera::IsInFrustum(const FVector3& aPoint) const
{
	return IsInFrustum(aPoint.x, aPoint.y, aPoint.z);
}

FVector3 Clamp(const FVector3& aPoint, const FVector3& aMin, const FVector3& aMax)
{
	FVector3 result = aPoint;
	float dotResult = (aMax - aMin).Dot(result - aMin);
	if (dotResult <= 0)
		result = aMin;

	dotResult = (aMin - aMax).Dot(result - aMax);
	if (dotResult <= 0)
		result = aMax;

	return result;
}

bool FCamera::IsEncapsulatingFrustum(const FVector3 & aPoint, const FVector3 & aPoint2) const
{
	int nrOfIntersections = 0;
	FVector3 mappedPos1 = aPoint;
	FVector3 mappedPos2 = aPoint2;
	for (int i = 0; i < 6; i++)
	{
		XMVECTOR v = XMVectorSet(myDebugFrustum[i].myNormal.x, myDebugFrustum[i].myNormal.y, myDebugFrustum[i].myNormal.z, myDebugFrustum[i].myDistance);
		XMVECTOR minx = XMVectorSet(mappedPos1.x, mappedPos1.y, mappedPos1.z, 1.0f);
		XMVECTOR maxx = XMVectorSet(mappedPos2.x, mappedPos2.y, mappedPos2.z, 1.0f);

		XMVECTOR result = XMPlaneDotCoord(v, minx);
		XMVECTOR result2 = XMPlaneDotCoord(v, maxx);

		if (XMVectorGetX(result) < 0.0f)
		{
			XMVECTOR intersectPoint = XMPlaneIntersectLine(v, minx, maxx);
			mappedPos1 = Clamp(FVector3(XMVectorGetX(intersectPoint), XMVectorGetY(intersectPoint), XMVectorGetZ(intersectPoint)), mappedPos1, mappedPos2);
		}

		if (XMVectorGetX(result2) < 0.0f)
		{
			XMVECTOR intersectPoint = XMPlaneIntersectLine(v, minx, maxx);
			mappedPos2 = Clamp(FVector3(XMVectorGetX(intersectPoint), XMVectorGetY(intersectPoint), XMVectorGetZ(intersectPoint)), mappedPos1, mappedPos2);
		}

		if ((XMVectorGetX(result2) < 0.0 != XMVectorGetX(result) < 0.0f))
		{
			nrOfIntersections++;
		}

		if (nrOfIntersections >= 2)
		{
			FDebugDrawer* debugDrawer = FD3d12Renderer::GetInstance()->GetDebugDrawer();
			if (myDoDebugDraw)
			{
				debugDrawer->drawLine(aPoint, aPoint2, FVector3(1, 0, 0.5));
				debugDrawer->drawLine(mappedPos1 + FVector3(0.01f, 0.01f, 0.01f), mappedPos2 + FVector3(0.01f, 0.01f, 0.01f), FVector3(1, 1, 1));
				debugDrawer->DrawPoint(mappedPos1, 1.0f, FVector3(1, 0, 0.5));
				debugDrawer->DrawPoint(mappedPos2, 1.0f, FVector3(1, 0, 0.5));
			}
		}
	}

	if (nrOfIntersections == 0)
		return false;

	for (int i = 0; i < 6; i+=2)
	{
		XMVECTOR v = XMVectorSet(myFrustum[i].myNormal.x, myFrustum[i].myNormal.y, myFrustum[i].myNormal.z, myFrustum[i].myDistance);
		v = XMVectorSet(myDebugFrustum[i].myNormal.x, myDebugFrustum[i].myNormal.y, myDebugFrustum[i].myNormal.z, myDebugFrustum[i].myDistance);
		XMVECTOR v2 = XMVectorSet(myDebugFrustum[i + 1].myNormal.x, myDebugFrustum[i + 1].myNormal.y, myDebugFrustum[i + 1].myNormal.z, myDebugFrustum[i + 1].myDistance);

		XMVECTOR minx = XMVectorSet(aPoint.x, aPoint.y, aPoint.z, 1.0f);
		minx = XMVectorSet(mappedPos1.x, mappedPos1.y, mappedPos1.z, 1.0f);
		XMVECTOR maxx = XMVectorSet(aPoint2.x, aPoint2.y, aPoint2.z, 1.0f);
		maxx = XMVectorSet(mappedPos2.x, mappedPos2.y, mappedPos2.z, 1.0f);
		XMVECTOR result = XMPlaneDotCoord(v, minx);
		XMVECTOR result2 = XMPlaneDotCoord(v2, maxx);
		float len = (mappedPos2 - mappedPos1).Length();
		if ((XMVectorGetX(result2) >= 0.0 && XMVectorGetX(result) >= 0.0))
		{
			int nexti = (i + 2) % 6;
			v = XMVectorSet(myDebugFrustum[nexti].myNormal.x, myDebugFrustum[nexti].myNormal.y, myDebugFrustum[nexti].myNormal.z, myDebugFrustum[nexti].myDistance);
			v2 = XMVectorSet(myDebugFrustum[nexti + 1].myNormal.x, myDebugFrustum[nexti + 1].myNormal.y, myDebugFrustum[nexti + 1].myNormal.z, myDebugFrustum[nexti + 1].myDistance);
			result = XMPlaneDotCoord(v, minx);
			result2 = XMPlaneDotCoord(v2, maxx);
			if ((XMVectorGetX(result2) >= 0.0 && XMVectorGetX(result) >= 0.0))
			{
				nexti = (nexti + 2) % 6;
				v = XMVectorSet(myDebugFrustum[nexti].myNormal.x, myDebugFrustum[nexti].myNormal.y, myDebugFrustum[nexti].myNormal.z, myDebugFrustum[nexti].myDistance);
				v2 = XMVectorSet(myDebugFrustum[nexti + 1].myNormal.x, myDebugFrustum[nexti + 1].myNormal.y, myDebugFrustum[nexti + 1].myNormal.z, myDebugFrustum[nexti + 1].myDistance);
				result = XMPlaneDotCoord(v, minx);
				result2 = XMPlaneDotCoord(v2, maxx);
				if ((XMVectorGetX(result2) >= 0.0 && XMVectorGetX(result) >= 0.0))
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool FCamera::IsInFrustum(float x, float y, float z) const
{
	for (int i = 0; i < 6; i++)
	{
		XMVECTOR v = XMVectorSet(myFrustum[i].myNormal.x, myFrustum[i].myNormal.y, myFrustum[i].myNormal.z, myFrustum[i].myDistance);
		v = XMVectorSet(myDebugFrustum[i].myNormal.x, myDebugFrustum[i].myNormal.y, myDebugFrustum[i].myNormal.z, myDebugFrustum[i].myDistance);
		v = XMPlaneNormalize(v);

		XMVECTOR minx = XMVectorSet(x, y, z, 1.0f);
		XMVECTOR result = XMPlaneDotCoord(v, minx);
		if (XMVectorGetX(result) < 0.0)
		{
			return false;
		}
	}

	FDebugDrawer* debugDrawer = FD3d12Renderer::GetInstance()->GetDebugDrawer();
	if (myDoDebugDraw)
	{
		debugDrawer->DrawPoint(FVector3(x, y, z), 0.2f, FVector3(1, 0.5, 1));
	}

	return true;
}

const float NOHIT = 999999.0f;
float rayBoxIntersect(const FVector3& rpos, const FVector3& rdir, const FVector3& vmin, const FVector3& vmax)
{
	float t[10];
	t[1] = (vmin.x - rpos.x) / rdir.x;
	t[2] = (vmax.x - rpos.x) / rdir.x;
	t[3] = (vmin.y - rpos.y) / rdir.y;
	t[4] = (vmax.y - rpos.y) / rdir.y;
	t[5] = (vmin.z - rpos.z) / rdir.z;
	t[6] = (vmax.z - rpos.z) / rdir.z;
	t[7] = fmax(fmax(fmin(t[1], t[2]), fmin(t[3], t[4])), fmin(t[5], t[6]));
	t[8] = fmin(fmin(fmax(t[1], t[2]), fmax(t[3], t[4])), fmax(t[5], t[6]));
	t[9] = (t[8] < 0 || t[7] > t[8]) ? NOHIT : t[7];
	return t[9];
}

bool FCamera::LineIntersectsAABB(const FVector3& start, const FVector3& end, const FAABB& anAABB) const
{
	float dist = rayBoxIntersect(start, (end - start).Normalized(), anAABB.myMin, anAABB.myMax);
	if (dist > 0 && dist < (end - start).Length())
	{
		if(myDoDebugDraw)
		{
			FDebugDrawer* debugDrawer = FD3d12Renderer::GetInstance()->GetDebugDrawer();
			debugDrawer->DrawPoint(start + (end - start).Normalized() * dist, 1.0f, FVector3(0, 1, 0.5));
		}
		return true;
	}

	return false;
}

bool FCamera::IsInFrustum(const FAABB& anABB) const
{
	FDebugDrawer* debugDrawer = FD3d12Renderer::GetInstance()->GetDebugDrawer();
	if (myDoDebugDraw)
	{
		debugDrawer->drawAABB(anABB, FVector3(0, 0.5, 1));
	}

	for (int i = 0; i < 8; i++)
	{
		if (anABB.IsInside(myDebugFrustumCorners[i]))
			return true;
	}

	// check for frustum lines intersecting AABB
	{
		// near -> far
		FVector3 start = myDebugFrustumCorners[0];  // NTL -> FTL
		FVector3 end = myDebugFrustumCorners[4];
		if (LineIntersectsAABB(start, end, anABB))
			return true;

		start = myDebugFrustumCorners[1];  // NTR -> FTR
		end = myDebugFrustumCorners[5];
		if (LineIntersectsAABB(start, end, anABB))
			return true;
		
		start = myDebugFrustumCorners[2];  // NBL -> FBL
		end = myDebugFrustumCorners[6];
		if (LineIntersectsAABB(start, end, anABB))
			return true;

		start = myDebugFrustumCorners[3];  // NBR -> FBR
		end = myDebugFrustumCorners[7];
		if (LineIntersectsAABB(start, end, anABB))
			return true;

		// near plane lines
		start = myDebugFrustumCorners[0];  // NTL -> NTR
		end = myDebugFrustumCorners[1];
		if (LineIntersectsAABB(start, end, anABB))
			return true;

		start = myDebugFrustumCorners[0]; // NTL -> NBL
		end = myDebugFrustumCorners[2];
		if (LineIntersectsAABB(start, end, anABB))
			return true;

		start = myDebugFrustumCorners[1];  // NTR -> NBR
		end = myDebugFrustumCorners[3];
		if (LineIntersectsAABB(start, end, anABB))
			return true;

		start = myDebugFrustumCorners[2];  // NBL -> NBR
		end = myDebugFrustumCorners[3];
		if (LineIntersectsAABB(start, end, anABB))
			return true;

		// far plane lines
		start = myDebugFrustumCorners[4];  // FTL -> FTR
		end = myDebugFrustumCorners[5];
		if (LineIntersectsAABB(start, end, anABB))
			return true;

		start = myDebugFrustumCorners[4]; // FTL -> FBL
		end = myDebugFrustumCorners[6];
		if (LineIntersectsAABB(start, end, anABB))
			return true;

		start = myDebugFrustumCorners[5];  // FTR -> FBR
		end = myDebugFrustumCorners[7];
		if (LineIntersectsAABB(start, end, anABB))
			return true;

		start = myDebugFrustumCorners[6];  // FBL -> FBR
		end = myDebugFrustumCorners[7];
		if (LineIntersectsAABB(start, end, anABB))
			return true;
	}

	if (IsInFrustum(anABB.myMin))
	{
		return true;
	}
	if (IsInFrustum(anABB.myMax))
	{
		return true;
	}
	if (IsInFrustum(FVector3(anABB.myMin.x, anABB.myMax.y, anABB.myMax.z)))
	{
		return true;
	}
	if (IsInFrustum(FVector3(anABB.myMax.x, anABB.myMin.y, anABB.myMin.z)))
	{
		return true;
	}
	if (IsInFrustum(FVector3(anABB.myMin.x, anABB.myMin.y, anABB.myMax.z)))
	{
		return true;
	}
	if (IsInFrustum(FVector3(anABB.myMax.x, anABB.myMin.y, anABB.myMax.z)))
	{
		return true;
	}
	if (IsInFrustum(FVector3(anABB.myMin.x, anABB.myMax.y, anABB.myMin.z)))
	{
		return true;
	}
	if(IsInFrustum(FVector3(anABB.myMax.x, anABB.myMax.y, anABB.myMin.z)))
	{
		return true;
	}
	

	if (IsEncapsulatingFrustum(anABB.myMin, FVector3(anABB.myMin.x, anABB.myMin.y, anABB.myMax.z)))
	{
		return true;
	}

	if (IsEncapsulatingFrustum(anABB.myMin, FVector3(anABB.myMin.x, anABB.myMax.y, anABB.myMin.z)))
	{
		return true;
	}
	if (IsEncapsulatingFrustum(anABB.myMin, FVector3(anABB.myMax.x, anABB.myMin.y, anABB.myMin.z)))
	{
		return true;
	}
	if (IsEncapsulatingFrustum(FVector3(anABB.myMin.x, anABB.myMax.y, anABB.myMax.z), anABB.myMax))
	{
		return true;
	}
	if (IsEncapsulatingFrustum(anABB.myMax, FVector3(anABB.myMax.x, anABB.myMin.y, anABB.myMax.z)))
	{
		return true;
	}
	if (IsEncapsulatingFrustum(anABB.myMax, FVector3(anABB.myMax.x, anABB.myMax.y, anABB.myMin.z)))
		return true;
	if (IsEncapsulatingFrustum(FVector3(anABB.myMin.x, anABB.myMax.y, anABB.myMin.z), FVector3(anABB.myMax.x, anABB.myMax.y, anABB.myMin.z)))
		return true;
	if (IsEncapsulatingFrustum(FVector3(anABB.myMax.x, anABB.myMin.y, anABB.myMin.z), FVector3(anABB.myMax.x, anABB.myMax.y, anABB.myMin.z)))
		return true;
	if (IsEncapsulatingFrustum(FVector3(anABB.myMax.x, anABB.myMin.y, anABB.myMin.z), FVector3(anABB.myMax.x, anABB.myMin.y, anABB.myMax.z)))
		return true;
	if (IsEncapsulatingFrustum(FVector3(anABB.myMin.x, anABB.myMin.y, anABB.myMax.z), FVector3(anABB.myMin.x, anABB.myMax.y, anABB.myMax.z)))
		return true;
	if (IsEncapsulatingFrustum(FVector3(anABB.myMin.x, anABB.myMax.y, anABB.myMin.z), FVector3(anABB.myMin.x, anABB.myMax.y, anABB.myMax.z)))
		return true;
	if (IsEncapsulatingFrustum(FVector3(anABB.myMin.x, anABB.myMin.y, anABB.myMax.z), FVector3(anABB.myMax.x, anABB.myMin.y, anABB.myMax.z)))
		return true;
	//	*/

	return false; // @todo temp hack until frustum culling is fixed (currently large objects where all points are outside are not correctly included)
}

bool FCamera::IsInFrustum(FRenderableObject * anEntity) const
{
	FPROFILE_FUNCTION("FCamera IsInFrustum");

	if (!anEntity)
		return false;

	return IsInFrustum(anEntity->GetAABB());
}