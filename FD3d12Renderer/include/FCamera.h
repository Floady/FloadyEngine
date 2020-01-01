#pragma once
#include "FRenderableObject.h"
#include "FVector3.h"
#include "FMatrix.h"

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
	const FMatrix& GetViewProjMatrix() const; 
	const FMatrix& GetViewMatrix() const;
	const FMatrix& GetInvViewProjMatrix2() const;
	const FMatrix& GetInvViewProjMatrix3() const;
	const FMatrix& GetProjMatrix() const;
	FMatrix GetViewProjMatrixWithOffset2(float x, float y, float z);
	void UpdateViewProj();
	bool IsInFrustum(FRenderableObject* anEntity) const;
	bool IsInFrustum(const FVector3& aPoint) const;
	bool IsEncapsulatingFrustum(const FVector3& aPoint, const FVector3& aPoint2) const;
	bool IsInFrustum(float x, float y, float z) const;
	bool IsInFrustum(const FAABB& anABB) const;
	bool LineIntersectsAABB(const FVector3& start, const FVector3& end, const FAABB& anAABB) const;
	void SetOverrideLight(bool aShouldOverride) { myOverrideWithLight = aShouldOverride; }
	
	void SetFreezeDebug(bool aShouldFreeze) { myFreezeDebugInfo = aShouldFreeze; }
	void SetDebugDrawEnabled(bool anEnabled) { myDoDebugDraw = anEnabled; }

private:
	FMatrix myViewProjMatrix;
	FMatrix myViewMatrix2;
	FMatrix myInvViewProjMatrix2;
	FMatrix myInvViewProjMatrix3;
	FMatrix myProjMatrix2;
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
	FVector3 myDebugFrustumCorners[8]; // NTL, NTR, NBL, NBR, FTL, FTR, FBL, FBR
};

