#include "FMatrix.h"

FMatrix::FMatrix()
{
	Identity();
}

void FMatrix::Identity()
{
	cell[1] = cell[2] = cell[TX] = cell[4] = cell[6] = cell[TY] =
		cell[8] = cell[9] = cell[TZ] = cell[12] = cell[13] = cell[14] = 0;
	cell[D0] = cell[D1] = cell[D2] = cell[W] = 1;
}

void FMatrix::Init( FVector3& a_Pos, float a_RX, float a_RY, float a_RZ )
{
	FMatrix t;
	t.RotateX( a_RZ );
	RotateY( a_RY );
	Concatenate( t );
	t.RotateZ( a_RX );
	Concatenate( t );
	Translate( a_Pos );
}

void FMatrix::RotateX( float a_RX )
{
	float sx = (float)sin( a_RX * PI / 180 );
	float cx = (float)cos( a_RX * PI / 180 );
	Identity();
	cell[5] = cx, cell[6] = sx, cell[9] = -sx, cell[10] = cx;
}

void FMatrix::RotateY( float a_RY )
{
	float sy = (float)sin( a_RY * PI / 180 );
	float cy = (float)cos( a_RY * PI / 180 );
	Identity ();
	cell[0] = cy, cell[2] = -sy, cell[8] = sy, cell[10] = cy;
}

void FMatrix::RotateZ( float a_RZ )
{
	float sz = (float)sin( a_RZ * PI / 180 );
	float cz = (float)cos( a_RZ * PI / 180 );
	Identity ();
	cell[0] = cz, cell[1] = sz, cell[4] = -sz, cell[5] = cz;
}

void FMatrix::Translate( FVector3& a_Pos )
{
	cell[TX] += a_Pos.x; cell[TY] += a_Pos.y; cell[TZ] += a_Pos.z;
}

void FMatrix::SetTranslation( FVector3& a_Pos )
 { 
	 cell[TX] = a_Pos.x;
	 cell[TY] = a_Pos.y;
	 cell[TZ] = a_Pos.z; 
}

void FMatrix::Concatenate( FMatrix& m2 )
{
	FMatrix res;
	int c;
	for ( c = 0; c < 4; c++ ) for ( int r = 0; r < 4; r++ )
		res.cell[r * 4 + c] = cell[r * 4] * m2.cell[c] +
		cell[r * 4 + 1] * m2.cell[c + 4] +
		cell[r * 4 + 2] * m2.cell[c + 8] +
		cell[r * 4 + 3] * m2.cell[c + 12];
	for ( c = 0; c < 16; c++ ) cell[c] = res.cell[c];
}

FVector3 FMatrix::Transform( FVector3& v )
{
	float x  = cell[0] * v.x + cell[1] * v.y + cell[2] * v.z + cell[3];
	float y  = cell[4] * v.x + cell[5] * v.y + cell[6] * v.z + cell[7];
	float z  = cell[8] * v.x + cell[9] * v.y + cell[10] * v.z + cell[11];
	return FVector3( x, y, z );
}

void FMatrix::Invert()
{
	FMatrix t;
	int h, i;
	float tx = -cell[3], ty = -cell[7], tz = -cell[11];
	for ( h = 0; h < 3; h++ ) for ( int v = 0; v < 3; v++ ) t.cell[h + v * 4] = cell[v + h * 4];
	for ( i = 0; i < 11; i++ ) cell[i] = t.cell[i];
	cell[3] = tx * cell[0] + ty * cell[1] + tz * cell[2];
	cell[7] = tx * cell[4] + ty * cell[5] + tz * cell[6];
	cell[11] = tx * cell[8] + ty * cell[9] + tz * cell[10];
}

void FMatrix::Transpose()
{
	FMatrix t;
	t.cell[0] = cell[0];
	t.cell[1] = cell[4];
	t.cell[2] = cell[8];
	t.cell[3] = cell[12];
	t.cell[4] = cell[1];
	t.cell[5] = cell[5];
	t.cell[6] = cell[9];
	t.cell[7] = cell[13];
	t.cell[8] = cell[2];
	t.cell[9] = cell[6];
	t.cell[10] = cell[10];
	t.cell[11] = cell[14];
	t.cell[12] = cell[3];
	t.cell[13] = cell[7];
	t.cell[14] = cell[11];
	t.cell[15] = cell[15];
	for (int i = 0; i < 16; i++)
	{
		cell[i]=t.cell[i];
	}
}
