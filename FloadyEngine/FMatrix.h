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
	void Init( FVector3& a_Pos, float a_RX, float a_RY, float a_RZ );
	void RotateX( float a_RX );
	void RotateY( float a_RY );
	void RotateZ( float a_RZ );
	void Translate( FVector3& a_Pos );
	void SetTranslation( FVector3& a_Pos );
	void Concatenate( FMatrix& m2 );
	FVector3 Transform( FVector3& v );
	void Invert();
	void Transpose();
//private:
	float cell[16];
};
