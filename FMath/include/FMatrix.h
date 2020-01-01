#include "FVector3.h"
#pragma once

class FMatrix
{
public:
	enum 
	{ 
		TX=3, 
		TY=7, 
		TZ=11, 
		D0=0, D1=5, D2=10, D3=15, 
		SX=D0, SY=D1, SZ=D2, 
		W=D3 
	};
	FMatrix();
	float& operator [] ( int a_N ) { return cell[a_N]; }
	void Identity();
	void Init(const FVector3& a_Pos, float a_RX, float a_RY, float a_RZ );
	void RotateX( float a_RX );
	void RotateY( float a_RY );
	void RotateZ( float a_RZ );
	void Translate(const FVector3& a_Pos );
	void SetTranslation( const FVector3& a_Pos );
	void Concatenate(const FMatrix& m2 );
	FVector3 Transform( const FVector3& v );
	void Multiply(float v);
	void Invert();
	void Invert2();
	void Transpose();
//private:
	float cell[16];
};
