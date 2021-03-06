#include "FLightManager.h"
#include "FD3d12Renderer.h"
#include "FDebugDrawer.h"
#include "FCamera.h"
#include "FProfiler.h"
#include <DirectXMath.h>

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
	p.myHasMoved = true;
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
	light.myHasMoved = true;
	myDirectionalLights.push_back(light);

	return light.myId;
}

unsigned int FLightManager::AddLight(FVector3 aPos, float aRadius)
{
	return AddSpotlight(aPos, FVector3(0, -1, 0.1f), aRadius);
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

void FLightManager::SetLightPos(unsigned int aLightId, FVector3 aPos)
{
	for (SpotLight& light : mySpotlights)
	{
		if (light.myId == aLightId)
		{
			light.myPos = aPos;
			light.myHasMoved = true;
			return;
		}
	}

	for (DirectionalLight& light : myDirectionalLights)
	{
		if (light.myId == aLightId)
		{
			light.myPos = aPos;
			light.myHasMoved = true;
			return;
		}
	}
}

void FLightManager::SetLightDir(unsigned int aLightId, FVector3 aDir)
{
	for (SpotLight& light : mySpotlights)
	{
		if (light.myId == aLightId)
		{
			light.myDir = aDir;
			light.myHasMoved = true;
			return;
		}
	}

	for (DirectionalLight& light : myDirectionalLights)
	{
		if (light.myId == aLightId)
		{
			light.myDir = aDir;
			light.myHasMoved = true;
			return;
		}
	}
}

void FLightManager::RemoveLight(unsigned int aLightId)
{
	for (std::vector<SpotLight>::iterator it = mySpotlights.begin(); it != mySpotlights.end(); ++it)
	{
		if ((*it).myId == aLightId)
		{
			mySpotlights.erase(it);
			return;
		}
	}

	for (std::vector<DirectionalLight>::iterator it = myDirectionalLights.begin(); it != myDirectionalLights.end(); ++it)
	{
		if ((*it).myId == aLightId)
		{
			myDirectionalLights.erase(it);
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

	return nullptr;
}

static DirectX::XMFLOAT4X4 ourIdentityMatrix = DirectX::XMFLOAT4X4();

const FMatrix & FLightManager::GetSpotlightViewProjMatrix2(int i) const
{
	return mySpotlights[i].GetViewProjMatrix2();
}

const FMatrix & FLightManager::GetCurrentActiveLightViewProjMatrix2() const
{
	if (myActiveLight == -1)
		return GetDirectionalLightViewProjMatrix2(0); // todo: fix

	if (myActiveLight >= mySpotlights.size())
		return FMatrix();

	return GetSpotlightViewProjMatrix2(myActiveLight);
}

const FMatrix & FLightManager::GetDirectionalLightViewProjMatrix2(int i) const
{
	return myDirectionalLights[i].GetViewProjMatrix2();
}

void FLightManager::SortLights()
{
	//FPROFILE_FUNCTION("Sort lights");

	const FVector3& camPos = FD3d12Renderer::GetInstance()->GetCamera()->GetPos();

	// bubble sort lol
	bool swapped = true;
	while (swapped)
	{
		swapped = false;
		for (int i = 1; i < mySpotlights.size(); i++)
		{
			FAABB worldAABB = mySpotlights[i].GetAABB();
			FAABB worldAABB2 = mySpotlights[i-1].GetAABB();
			bool isInFrustumA = FD3d12Renderer::GetInstance()->GetCamera()->IsInFrustum(worldAABB);
			bool isInFrustumB = FD3d12Renderer::GetInstance()->GetCamera()->IsInFrustum(worldAABB2);
			if (isInFrustumA && !isInFrustumB)
			{
				SpotLight p = mySpotlights[i];
				mySpotlights[i] = mySpotlights[i - 1];
				mySpotlights[i - 1] = p;
				swapped = true;
			}
			else if (isInFrustumA && (mySpotlights[i].myPos - camPos).SqrLength() < (mySpotlights[i - 1].myPos - camPos).SqrLength())
			{
				SpotLight p = mySpotlights[i];
				mySpotlights[i] = mySpotlights[i - 1];
				mySpotlights[i-1] = p;
				swapped = true;
			}
		}
	}
}

extern bool ourShouldRecalc;

void FLightManager::UpdateViewProjMatrices()
{
	FDebugDrawer* debugDrawer = FD3d12Renderer::GetInstance()->GetDebugDrawer();

	{
		float aspectRatio = 1600 / 900.0f;
		float fov = 45.0f;
		FXMVECTOR up = XMVectorSet(0, 1, 0, 1);
		XMVECTOR vUp = up;

		for (int i = 0; i < mySpotlights.size(); i++)
		{
			if(mySpotlights[i].myHasMoved || ourShouldRecalc)
			{
				float fovAngleY = mySpotlights[i].myAngle * 2;

				XMMATRIX projMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, 40.0f, 0.01f);

				// calc viewproj from lightpos
				FXMVECTOR eye = XMVectorSet(mySpotlights[i].myPos.x, mySpotlights[i].myPos.y, mySpotlights[i].myPos.z, 1);
				FXMVECTOR at = XMVectorSet(mySpotlights[i].myDir.x, mySpotlights[i].myDir.y, mySpotlights[i].myDir.z, 1);
				XMVECTOR vAt = at;
				vAt += eye;

				XMMATRIX _viewMatrix = XMMatrixLookAtLH(eye, vAt, vUp);

				XMMATRIX  _viewProjMatrix = XMMatrixTranspose(_viewMatrix * projMatrix);
				XMMATRIX  _transposedProjMatrix = XMMatrixTranspose(projMatrix);
				XMFLOAT4X4 _storageViewProjMtx;
				XMFLOAT4X4 _storageProjMtxTrans;
				XMStoreFloat4x4(&_storageViewProjMtx, _viewProjMatrix);
				XMStoreFloat4x4(&_storageProjMtxTrans, _transposedProjMatrix);
				memcpy(&mySpotlights[i].myViewProjMatrix2.cell, _storageViewProjMtx.m, sizeof(float) * 16);
				memcpy(&mySpotlights[i].myProjMatrix2.cell, _storageProjMtxTrans.m, sizeof(float) * 16);
			}

			if (myDoDebugDraw)
			{
				debugDrawer->drawAABB(mySpotlights[i].GetAABB(), FVector3(1, 1, 1));
			}
		}
	}

	for (int i = 0; i < myDirectionalLights.size(); i++)
	{
		if (myDirectionalLights[i].myHasMoved || ourShouldRecalc)
		{
			// add camera frustum corners to scene
			const FMatrix& viewProjMtx = FD3d12Renderer::GetInstance()->GetCamera()->GetViewProjMatrix();
			XMFLOAT4X4 m = XMFLOAT4X4(viewProjMtx.cell);
			XMMATRIX invProj = XMLoadFloat4x4(&m);
			invProj = XMMatrixInverse(nullptr, invProj);
			XMVECTOR maxCam = XMVector3Transform(XMVectorSet(1, 1, 1, 1), invProj);
			XMVECTOR minCam = XMVector3Transform(XMVectorSet(0, 0, 0, 1), invProj);

			FVector3 positionOBB;
			FVector3 dimensions;
			FVector3 maxVal;
			FVector3 up = myDirectionalLights[i].myDir.Cross(FVector3(0, 1, 0).Cross(myDirectionalLights[i].myDir)).Normalized();
			FVector3 right = FVector3(0, 1, 0).Cross(myDirectionalLights[i].myDir).Normalized();

			{
				float mindepth = 999999;
				float maxdepth = -999999;
				//up = FVector3(0, 1, 0);
				mindepth = min(mindepth, (up.Dot(myAABBVisibleFromCam.myMax)));
				mindepth = min(mindepth, (up.Dot(FVector3(myAABBVisibleFromCam.myMin.x, myAABBVisibleFromCam.myMax.y, myAABBVisibleFromCam.myMax.z))));
				mindepth = min(mindepth, (up.Dot(FVector3(myAABBVisibleFromCam.myMax.x, myAABBVisibleFromCam.myMin.y, myAABBVisibleFromCam.myMin.z))));
				mindepth = min(mindepth, (up.Dot(FVector3(myAABBVisibleFromCam.myMin.x, myAABBVisibleFromCam.myMin.y, myAABBVisibleFromCam.myMax.z))));
				mindepth = min(mindepth, (up.Dot(myAABBVisibleFromCam.myMin)));
				mindepth = min(mindepth, (up.Dot(FVector3(myAABBVisibleFromCam.myMax.x, myAABBVisibleFromCam.myMin.y, myAABBVisibleFromCam.myMax.z))));
				mindepth = min(mindepth, (up.Dot(FVector3(myAABBVisibleFromCam.myMin.x, myAABBVisibleFromCam.myMax.y, myAABBVisibleFromCam.myMin.z))));
				mindepth = min(mindepth, (up.Dot(FVector3(myAABBVisibleFromCam.myMax.x, myAABBVisibleFromCam.myMax.y, myAABBVisibleFromCam.myMin.z))));

				maxdepth = max(maxdepth, (up.Dot(myAABBVisibleFromCam.myMax)));
				maxdepth = max(maxdepth, (up.Dot(FVector3(myAABBVisibleFromCam.myMin.x, myAABBVisibleFromCam.myMax.y, myAABBVisibleFromCam.myMax.z))));
				maxdepth = max(maxdepth, (up.Dot(FVector3(myAABBVisibleFromCam.myMax.x, myAABBVisibleFromCam.myMin.y, myAABBVisibleFromCam.myMin.z))));
				maxdepth = max(maxdepth, (up.Dot(FVector3(myAABBVisibleFromCam.myMin.x, myAABBVisibleFromCam.myMin.y, myAABBVisibleFromCam.myMax.z))));
				maxdepth = max(maxdepth, (up.Dot(myAABBVisibleFromCam.myMin)));
				maxdepth = max(maxdepth, (up.Dot(FVector3(myAABBVisibleFromCam.myMax.x, myAABBVisibleFromCam.myMin.y, myAABBVisibleFromCam.myMax.z))));
				maxdepth = max(maxdepth, (up.Dot(FVector3(myAABBVisibleFromCam.myMin.x, myAABBVisibleFromCam.myMax.y, myAABBVisibleFromCam.myMin.z))));
				maxdepth = max(maxdepth, (up.Dot(FVector3(myAABBVisibleFromCam.myMax.x, myAABBVisibleFromCam.myMax.y, myAABBVisibleFromCam.myMin.z))));
				positionOBB.y = mindepth;
				dimensions.y = maxdepth - mindepth;
				maxVal.y = maxdepth;
			}

			{
				float mindepth = 999999;
				float maxdepth = -999999;
				//right = FVector3(1, 0, 0);
				mindepth = min(mindepth, (right.Dot(myAABBVisibleFromCam.myMax)));
				mindepth = min(mindepth, (right.Dot(FVector3(myAABBVisibleFromCam.myMin.x, myAABBVisibleFromCam.myMax.y, myAABBVisibleFromCam.myMax.z))));
				mindepth = min(mindepth, (right.Dot(FVector3(myAABBVisibleFromCam.myMax.x, myAABBVisibleFromCam.myMin.y, myAABBVisibleFromCam.myMin.z))));
				mindepth = min(mindepth, (right.Dot(FVector3(myAABBVisibleFromCam.myMin.x, myAABBVisibleFromCam.myMin.y, myAABBVisibleFromCam.myMax.z))));
				mindepth = min(mindepth, (right.Dot(myAABBVisibleFromCam.myMin)));
				mindepth = min(mindepth, (right.Dot(FVector3(myAABBVisibleFromCam.myMax.x, myAABBVisibleFromCam.myMin.y, myAABBVisibleFromCam.myMax.z))));
				mindepth = min(mindepth, (right.Dot(FVector3(myAABBVisibleFromCam.myMin.x, myAABBVisibleFromCam.myMax.y, myAABBVisibleFromCam.myMin.z))));
				mindepth = min(mindepth, (right.Dot(FVector3(myAABBVisibleFromCam.myMax.x, myAABBVisibleFromCam.myMax.y, myAABBVisibleFromCam.myMin.z))));

				maxdepth = max(maxdepth, (right.Dot(myAABBVisibleFromCam.myMax)));
				maxdepth = max(maxdepth, (right.Dot(FVector3(myAABBVisibleFromCam.myMin.x, myAABBVisibleFromCam.myMax.y, myAABBVisibleFromCam.myMax.z))));
				maxdepth = max(maxdepth, (right.Dot(FVector3(myAABBVisibleFromCam.myMax.x, myAABBVisibleFromCam.myMin.y, myAABBVisibleFromCam.myMin.z))));
				maxdepth = max(maxdepth, (right.Dot(FVector3(myAABBVisibleFromCam.myMin.x, myAABBVisibleFromCam.myMin.y, myAABBVisibleFromCam.myMax.z))));
				maxdepth = max(maxdepth, (right.Dot(myAABBVisibleFromCam.myMin)));
				maxdepth = max(maxdepth, (right.Dot(FVector3(myAABBVisibleFromCam.myMax.x, myAABBVisibleFromCam.myMin.y, myAABBVisibleFromCam.myMax.z))));
				maxdepth = max(maxdepth, (right.Dot(FVector3(myAABBVisibleFromCam.myMin.x, myAABBVisibleFromCam.myMax.y, myAABBVisibleFromCam.myMin.z))));
				maxdepth = max(maxdepth, (right.Dot(FVector3(myAABBVisibleFromCam.myMax.x, myAABBVisibleFromCam.myMax.y, myAABBVisibleFromCam.myMin.z))));
				positionOBB.x = mindepth;
				dimensions.x = maxdepth - mindepth;
				maxVal.x = maxdepth;
			}

			{
				float mindepth = 999999;
				float maxdepth = -999999;
				mindepth = min(mindepth, (myDirectionalLights[i].myDir.Dot(myAABBVisibleFromCam.myMax)));
				mindepth = min(mindepth, (myDirectionalLights[i].myDir.Dot(FVector3(myAABBVisibleFromCam.myMin.x, myAABBVisibleFromCam.myMax.y, myAABBVisibleFromCam.myMax.z))));
				mindepth = min(mindepth, (myDirectionalLights[i].myDir.Dot(FVector3(myAABBVisibleFromCam.myMax.x, myAABBVisibleFromCam.myMin.y, myAABBVisibleFromCam.myMin.z))));
				mindepth = min(mindepth, (myDirectionalLights[i].myDir.Dot(FVector3(myAABBVisibleFromCam.myMin.x, myAABBVisibleFromCam.myMin.y, myAABBVisibleFromCam.myMax.z))));
				mindepth = min(mindepth, (myDirectionalLights[i].myDir.Dot(myAABBVisibleFromCam.myMin)));
				mindepth = min(mindepth, (myDirectionalLights[i].myDir.Dot(FVector3(myAABBVisibleFromCam.myMax.x, myAABBVisibleFromCam.myMin.y, myAABBVisibleFromCam.myMax.z))));
				mindepth = min(mindepth, (myDirectionalLights[i].myDir.Dot(FVector3(myAABBVisibleFromCam.myMin.x, myAABBVisibleFromCam.myMax.y, myAABBVisibleFromCam.myMin.z))));
				mindepth = min(mindepth, (myDirectionalLights[i].myDir.Dot(FVector3(myAABBVisibleFromCam.myMax.x, myAABBVisibleFromCam.myMax.y, myAABBVisibleFromCam.myMin.z))));

				maxdepth = max(maxdepth, (myDirectionalLights[i].myDir.Dot(myAABBVisibleFromCam.myMax)));
				maxdepth = max(maxdepth, (myDirectionalLights[i].myDir.Dot(FVector3(myAABBVisibleFromCam.myMin.x, myAABBVisibleFromCam.myMax.y, myAABBVisibleFromCam.myMax.z))));
				maxdepth = max(maxdepth, (myDirectionalLights[i].myDir.Dot(FVector3(myAABBVisibleFromCam.myMax.x, myAABBVisibleFromCam.myMin.y, myAABBVisibleFromCam.myMin.z))));
				maxdepth = max(maxdepth, (myDirectionalLights[i].myDir.Dot(FVector3(myAABBVisibleFromCam.myMin.x, myAABBVisibleFromCam.myMin.y, myAABBVisibleFromCam.myMax.z))));
				maxdepth = max(maxdepth, (myDirectionalLights[i].myDir.Dot(myAABBVisibleFromCam.myMin)));
				maxdepth = max(maxdepth, (myDirectionalLights[i].myDir.Dot(FVector3(myAABBVisibleFromCam.myMax.x, myAABBVisibleFromCam.myMin.y, myAABBVisibleFromCam.myMax.z))));
				maxdepth = max(maxdepth, (myDirectionalLights[i].myDir.Dot(FVector3(myAABBVisibleFromCam.myMin.x, myAABBVisibleFromCam.myMax.y, myAABBVisibleFromCam.myMin.z))));
				maxdepth = max(maxdepth, (myDirectionalLights[i].myDir.Dot(FVector3(myAABBVisibleFromCam.myMax.x, myAABBVisibleFromCam.myMax.y, myAABBVisibleFromCam.myMin.z))));
				positionOBB.z = mindepth;
				dimensions.z = maxdepth - mindepth;
				maxVal.z = maxdepth;
			}

			FVector3 pos = myAABBVisibleFromCam.myMin + (myAABBVisibleFromCam.myMax - myAABBVisibleFromCam.myMin) / 2;
			pos -= myDirectionalLights[i].myDir * dimensions.z / 2;

			
			FVector3 up22 = myDirectionalLights[i].myDir.Cross(FVector3(0, 1, 0).Cross(myDirectionalLights[i].myDir)).Normalized();
			FVector3 upVec = up22;
			FVector3 atVec = myDirectionalLights[i].myDir;

			if (!myFreezeDebugInfo)
			{
				myDebugPos = pos;
				myDebugDimensions = dimensions;
				myDebugAtvec = atVec;
				myDebugUpvec = upVec;
				myDebugMinCam = FVector3(XMVectorGetX(minCam), XMVectorGetY(minCam), XMVectorGetZ(minCam));
				myDebugMaxCam = FVector3(XMVectorGetX(maxCam), XMVectorGetY(maxCam), XMVectorGetZ(maxCam));
				myDebugAABBVisibleFromCam = myAABBVisibleFromCam;
			}

			if (myDoDebugDraw)
			{
				//debugDrawer->DrawPoint(myDebugPos, 1.0f, FVector3(1, 1, 0));
				//debugDrawer->drawAABB(myDebugPos - FVector3(myDebugDimensions.x / 2, 0, 0), myDebugPos + FVector3(myDebugDimensions.x / 2, -myDebugDimensions.y, myDebugDimensions.z), FVector3(1, 1, 1));
				//debugDrawer->drawLine(myDebugPos, myDebugPos + myDebugUpvec * myDebugDimensions.y / 2.0f, FVector3(0, 1, 0));
				//debugDrawer->drawLine(myDebugPos, myDebugPos + myDebugAtvec * myDebugDimensions.z, FVector3(0, 1, 0));
				//debugDrawer->DrawPoint(myDebugMinCam, 1.0f, FVector3(1, 1, 1));
				debugDrawer->drawAABB(myDebugAABBVisibleFromCam.myMin, myDebugAABBVisibleFromCam.myMax, FVector3(1, 0, 0));
			}

			FXMVECTOR eye = XMVectorSet(myDebugPos.x, myDebugPos.y, myDebugPos.z, 1);
			XMVECTOR vAt = XMVectorSet(myDebugAtvec.x, myDebugAtvec.y, myDebugAtvec.z, 1);
			vAt += eye;
			XMVECTOR vUp = XMVectorSet(myDebugUpvec.x, myDebugUpvec.y, myDebugUpvec.z, 1);
			XMMATRIX _viewMatrix = XMMatrixLookAtLH(eye, vAt, vUp);
			XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(0, 0, 0, 1));

			// combine with stored projection matrix
			XMMATRIX myProjMatrix = XMMatrixOrthographicLH(myDebugDimensions.x, myDebugDimensions.y, myDebugDimensions.z, 0.001f);
			XMMATRIX  _viewProjMatrix = XMMatrixTranspose(offset * _viewMatrix * myProjMatrix);
			if (myDoDebugDraw)
			{
				debugDrawer->drawLine(myDebugPos + myDebugUpvec * myDebugDimensions.y / 2.0f, myDebugPos + myDebugUpvec * myDebugDimensions.y / 2.0f + myDebugAtvec * myDebugDimensions.z, FVector3(0, 1, 0));
				debugDrawer->drawLine(myDebugPos - myDebugUpvec * myDebugDimensions.y / 2.0f, myDebugPos - myDebugUpvec * myDebugDimensions.y / 2.0f + myDebugAtvec * myDebugDimensions.z, FVector3(0, 1, 0));
				debugDrawer->drawLine(myDebugPos - myDebugUpvec * myDebugDimensions.y / 2.0f, myDebugPos + myDebugUpvec * myDebugDimensions.y / 2.0f, FVector3(0, 1, 0));
				debugDrawer->drawLine(myDebugPos + myDebugUpvec * myDebugDimensions.y / 2.0f + myDebugAtvec * myDebugDimensions.z, myDebugPos - myDebugUpvec * myDebugDimensions.y / 2.0f + myDebugAtvec * myDebugDimensions.z, FVector3(0, 1, 0));
			}

			if (!myFreezeDebugInfo)
			{
				XMFLOAT4X4 _storageViewProjMtx;
				XMFLOAT4X4 _storageProjMtxTrans;
				XMStoreFloat4x4(&_storageViewProjMtx, _viewProjMatrix);
				XMStoreFloat4x4(&_storageProjMtxTrans, XMMatrixTranspose(myProjMatrix));
				memcpy(&myDirectionalLights[i].myViewProjMatrix2.cell, _storageViewProjMtx.m, sizeof(float) * 16);
				memcpy(&myDirectionalLights[i].myProjMatrix2.cell, _storageProjMtxTrans.m, sizeof(float) * 16);
			}
		}
	}
}

void FLightManager::ResetHasMoved()
{
	for (int i = 0; i < mySpotlights.size(); i++)
	{
		mySpotlights[i].myHasMoved = false;
	}


	for (int i = 0; i < myDirectionalLights.size(); i++)
	{
		myDirectionalLights[i].myHasMoved = false;
	}
}

FLightManager::FLightManager()
	: myNextFreeLightId(1)
	, myDoDebugDraw(false)
	, myFreezeDebugInfo(false)
{
}


FLightManager::~FLightManager()
{
}

FAABB FLightManager::SpotLight::GetAABB()
{
	FAABB aabb;
	aabb.Grow(myRange * myDir);

	float theta = myAngle;

	float cs = cos(theta);
	float sn = sin(theta);

	FVector3 rotated = myDir;
	rotated.Rotate22(theta, 0, theta); 
	

	FVector3 rotated2 = myDir;
	rotated2.Rotate22(-theta, 0, -theta);

	aabb.Grow(myRange * rotated);
	aabb.Grow(myRange * rotated2);

	rotated.y = 0;
	rotated2.y = 0;

	aabb.Grow(myRange * rotated);
	aabb.Grow(myRange * rotated2);

	// debug draw light AABB
	FDebugDrawer* debugDrawer = FD3d12Renderer::GetInstance()->GetDebugDrawer();
	//debugDrawer->drawAABB(myPos + aabb.myMin, myPos + aabb.myMax, FVector3(1, 0, 1));

	aabb.myMin += myPos;
	aabb.myMax += myPos;
	
	return aabb;
}

FAABB FLightManager::DirectionalLight::GetAABB()
{
	FAABB aabb;
	aabb.Grow(FVector3(-5000, -5000, -5000));
	aabb.Grow(FVector3(5000, 5000, 5000));
	return aabb;
}
