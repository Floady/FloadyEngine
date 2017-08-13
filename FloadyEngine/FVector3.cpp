#include "FVector3.h"
#include <DirectXMath.h>

void FVector3::Rotate(float anAngleX, float anAngleY, float anAngleZ)
{
	DirectX::FXMVECTOR pos = DirectX::XMVectorSet(x, y, z, 1);

	DirectX::XMMATRIX mtxRot = DirectX::XMMatrixRotationRollPitchYaw(anAngleX, anAngleY, anAngleZ);
	DirectX::XMVECTOR vUp = DirectX::XMVector3Transform(pos, mtxRot);
	x = DirectX::XMVectorGetX(vUp);
	y = DirectX::XMVectorGetY(vUp);
	z = DirectX::XMVectorGetZ(vUp);
}
