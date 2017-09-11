#pragma once
#include "FVector3.h"
#include "FRenderableObject.h"
#include <DirectXMath.h>


struct FPlane
{
	FVector3 myNormal;
	float myDistance;
};

class FCamera
{
public:
	FCamera(float aWidth, float aHeight);
	~FCamera();
	void SetPos(float x, float y, float z);
	void Move(float x, float y, float z);
	void Yaw(float angle);
	void Pitch(float angle);
	const FVector3& GetPos() const { return myPos;  }
	const DirectX::XMFLOAT4X4& GetViewProjMatrix();
	const DirectX::XMFLOAT4X4& GetViewProjMatrixTransposed() { return myViewProjMatrixTransposed; }
	const DirectX::XMFLOAT4X4& GetInvViewProjMatrix() { return myInvViewProjMatrix; }
	DirectX::XMFLOAT4X4 GetViewProjMatrixWithOffset(float x, float y, float z, bool transpose = true);
	DirectX::XMFLOAT4X4 GetViewProjMatrixWithOffset(const DirectX::XMMATRIX& anObjectMatrix);
	void UpdateViewProj();
	bool IsInFrustum(FRenderableObject* anEntity) const;
	bool IsInFrustum(const FVector3& aPoint) const;
	bool IsEncapsulatingFrustum(const FVector3& aPoint, const FVector3& aPoint2) const;
	bool IsInFrustum(float x, float y, float z) const;
	bool IsInFrustum(const FAABB& anABB) const;
	bool LineIntersectsAABB(const FVector3& start, const FVector3& end, const FAABB& anAABB) const;
	DirectX::XMFLOAT4X4 GetProjMatrix() { return myProjMatrixFloatVersion; }
	void SetOverrideLight(bool aShouldOverride) { myOverrideWithLight = aShouldOverride; }
	DirectX::XMMATRIX _viewProjMatrix;
	DirectX::XMMATRIX _viewMatrix;
	DirectX::XMMATRIX  myProjMatrix;
	bool SphereInFrustum(DirectX::XMVECTOR pPosition, float radius);

	void SetFreezeDebug(bool aShouldFreeze) { myFreezeDebugInfo = aShouldFreeze; }
	void SetDebugDrawEnabled(bool anEnabled) { myDoDebugDraw = anEnabled; }

private:
	DirectX::XMFLOAT4X4  myProjMatrixFloatVersion;
	DirectX::XMMATRIX myViewMatrix;
	DirectX::XMFLOAT4X4 myViewProjMatrix;
	DirectX::XMFLOAT4X4 myViewProjMatrixTransposed;
	DirectX::XMFLOAT4X4 myInvViewProjMatrix;
	FVector3 myPos;
	FVector3 myDir;
	FVector3 myUp;
	float myPitch;
	float myYaw;
	float myFov;
	float myFovY;
	float myNear;
	float myFar;
	bool myOverrideWithLight;

	FPlane myFrustum[6];
	FVector3 myFrustumCorners[8]; // NTL, NTR, NBL, NBR, FTL, FTR, FBL, FBR

	// debug stuff
	bool myDoDebugDraw;
	bool myFreezeDebugInfo;
	FPlane myDebugFrustum[6];
	FVector3 myDebugPos;
	FVector3 myDebugAt;
	FVector3 myDebugUp;
	DirectX::XMMATRIX debugViewProj;
	FVector3 myDebugFrustumCorners[8]; // NTL, NTR, NBL, NBR, FTL, FTR, FBL, FBR
};

