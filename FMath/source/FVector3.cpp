#include "FVector3.h"
#include "FMatrix.h"

void FVector3::Rotate22(float anAngleX, float anAngleY, float anAngleZ)
{
	FMatrix m;
	m.RotateX(anAngleX);
	m.RotateY(anAngleY);
	m.RotateZ(anAngleZ);

	*this = m.Transform(*this);
}
