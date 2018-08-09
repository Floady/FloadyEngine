#include "F3DPicker.h"
#include "FCamera.h"
#include "FRenderWindow.h"
#include "FD3d12Renderer.h"
#include <DirectXMath.h>
#include "FNavMeshManagerRecast.h"


F3DPicker::F3DPicker(FCamera* aCam, FRenderWindow* aWindow)
{
	myCamera = aCam;
	myWindow = aWindow;
}


F3DPicker::~F3DPicker()
{
}

FVector3 F3DPicker::PickNavMeshPos(FVector3 a2DPos)
{
	FVector3 pickPosNear = UnProject(FVector3(a2DPos.x, a2DPos.y, 0.0f));
	FVector3 pickPosFar = UnProject(FVector3(a2DPos.x, a2DPos.y, 1.0f));
	FVector3 direction = (pickPosFar - pickPosNear).Normalized();

	FVector3 line = pickPosNear + (direction * -(pickPosNear.y / direction.y));
	

	return FNavMeshManagerRecast::GetInstance()->GetClosestPointOnNavMesh(FNavMeshManagerRecast::GetInstance()->RayCast(pickPosNear, line));

	//return line;
}

FVector3 F3DPicker::UnProject(FVector3 aMousePos)
{
	// unproject
	// Compute (projection x modelView) ^ -1:
	float winX = aMousePos.x;
	float winY = aMousePos.y;
	float winZ = aMousePos.z;
	
	const DirectX::XMFLOAT4X4& m = myCamera->GetInvViewProjMatrix();

	// Need to invert Y since screen Y-origin point down,
	// while 3D Y-origin points up (this is an OpenGL only requirement):
	winY = 1.0f - winY;

	// Transformation of normalized coordinates between -1 and 1:
	FVector3 in;
	float x = winX  * 2.0f - 1.0f;
	float y = winY * 2.0f - 1.0f;
	float z = 2.0f * winZ - 1.0f;
	float w = 1.0f;

	// To world coordinates:
	DirectX::XMVECTOR vec;
	vec = DirectX::XMVectorSet(x, y, z, w);
//	FVector3 out(m * in);
	
	DirectX::XMMATRIX invProj = XMMatrixInverse(nullptr, myCamera->_viewProjMatrix);
	DirectX::XMVECTOR result = XMVector3Transform(vec, invProj);
	if (result.m128_f32[3] == 0.0) // Avoid a division by zero
	{
		return FVector3(0,0,0);
	}

	FVector3 worldCoordinates;
	result.m128_f32[3] = 1.0f / result.m128_f32[3];
	worldCoordinates.x = result.m128_f32[0] * result.m128_f32[3];
	worldCoordinates.y = result.m128_f32[1] * result.m128_f32[3];
	worldCoordinates.z = result.m128_f32[2] * result.m128_f32[3];
	
	return worldCoordinates;
}
