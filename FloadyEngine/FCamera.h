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

	const XMFLOAT4X4& GetViewProjMatrix() { return myViewProjMatrix; }
	XMFLOAT4X4 GetViewProjMatrixWithOffset(float x, float y, float z);
	void UpdateViewProj();

private:
	XMMATRIX  myProjMatrix;
	XMMATRIX myViewMatrix;
	XMFLOAT4X4 myViewProjMatrix;
	FVector3 myPos;
	FVector3 myDir;
	FVector3 myUp;
	float myPitch;
	float myYaw;
};

