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
	bool IsInFrustrum(FRenderableObject* anEntity) const;
	bool IsInFrustrum(const FVector3& aPoint) const;
	bool IsInFrustrum(float x, float y, float z) const;
	bool IsInFrustrum(const FAABB& anABB) const;
	DirectX::XMFLOAT4X4 GetProjMatrix() { return myProjMatrixFloatVersion; }
	void SetOverrideLight(bool aShouldOverride) { myOverrideWithLight = aShouldOverride; }
	DirectX::XMMATRIX _viewProjMatrix;
	DirectX::XMMATRIX _viewMatrix;
	DirectX::XMMATRIX  myProjMatrix;
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
};

