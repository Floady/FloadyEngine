#pragma once
#include "FVector3.h"
#include <DirectXMath.h>


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
	const DirectX::XMFLOAT4X4& GetViewProjMatrix() { return myViewProjMatrix; }
	const DirectX::XMFLOAT4X4& GetInvViewProjMatrix() { return myInvViewProjMatrix; }
	DirectX::XMFLOAT4X4 GetViewProjMatrixWithOffset(float x, float y, float z, bool transpose = true);
	DirectX::XMFLOAT4X4 GetViewProjMatrixWithOffset(const DirectX::XMMATRIX& anObjectMatrix);
	void UpdateViewProj();
	DirectX::XMFLOAT4X4 GetProjMatrix() { return myProjMatrixFloatVersion; }

	DirectX::XMMATRIX _viewProjMatrix;
	DirectX::XMMATRIX _viewMatrix;
	DirectX::XMMATRIX  myProjMatrix;
private:
	DirectX::XMFLOAT4X4  myProjMatrixFloatVersion;
	DirectX::XMMATRIX myViewMatrix;
	DirectX::XMFLOAT4X4 myViewProjMatrix;
	DirectX::XMFLOAT4X4 myInvViewProjMatrix;
	FVector3 myPos;
	FVector3 myDir;
	FVector3 myUp;
	float myPitch;
	float myYaw;
};

