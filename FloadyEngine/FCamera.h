#pragma once
#include "FVector3.h"
#include <DirectXMath.h>
using namespace DirectX;


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
	const XMFLOAT4X4& GetViewProjMatrix() { return myViewProjMatrix; }
	const XMFLOAT4X4& GetInvViewProjMatrix() { return myInvViewProjMatrix; }
	XMFLOAT4X4 GetViewProjMatrixWithOffset(float x, float y, float z, bool transpose = true);
	XMFLOAT4X4 GetViewProjMatrixWithOffset(const XMMATRIX& anObjectMatrix);
	void UpdateViewProj();
	XMFLOAT4X4 GetProjMatrix() { return myProjMatrixFloatVersion; }

	XMMATRIX _viewProjMatrix;
	XMMATRIX _viewMatrix;
	XMMATRIX  myProjMatrix;
private:
	XMFLOAT4X4  myProjMatrixFloatVersion;
	XMMATRIX myViewMatrix;
	XMFLOAT4X4 myViewProjMatrix;
	XMFLOAT4X4 myInvViewProjMatrix;
	FVector3 myPos;
	FVector3 myDir;
	FVector3 myUp;
	float myPitch;
	float myYaw;
};

