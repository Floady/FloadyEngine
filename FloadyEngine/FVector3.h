
#include "stdio.h"
#include "windows.h"
#include "math.h"
#include <emmintrin.h>
#include <xmmintrin.h>
#include <smmintrin.h>

#pragma once
#define PI				3.14159265358979323846264338327950288419716939937510582097494459072381640628620899862803482534211706798f
#define DEGTORAD		PI / 180.0f

#define NONVECTORIZED 0

class FVector3
{
public:
	inline FVector3() : x( 0.0f ), y( 0.0f ), z( 0.0f ) {}
	inline FVector3( float a_X, float a_Y, float a_Z ) : x( a_X ), y( a_Y ), z( a_Z ) {}
	inline FVector3(__m128 vec) : xyz(vec){}
	FVector3(const FVector3& other) : x(other.x), y(other.y), z(other.z) {  }
	FVector3(FVector3&& other) : x(other.x), y(other.y), z(other.z) {  }
	FVector3& operator=(FVector3&& other) { x = other.x; y = other.y; z = other.z; return *this; }
	FVector3& operator=(FVector3& other) { x = other.x; y = other.y; z = other.z; return *this; }
	
	void Set( float a_X, float a_Y, float a_Z )
	{
		x = a_X;
		y = a_Y;
		z = a_Z;
	}
	void Normalize()
	{	
	#if NONVECTORIZED	
		float length = 1.0f / Length();
		x *= length;
		y *= length;
		z *= length;
	#else
		xyz = _mm_mul_ps(xyz, _mm_rsqrt_ps(_mm_dp_ps(xyz, xyz, 0x7F)));
	#endif
	}

	
FVector3 Normalized()
{
#if NONVECTORIZED	
	float length = 1.0f / Length();
	float ax = x*length;
	float ay = y*length;
	float az = z*length;
	return FVector3(ax,ay,az);
#else
	return _mm_mul_ps(xyz, _mm_rsqrt_ps(_mm_dp_ps(xyz, xyz, 0x7F)));
#endif
}

float Length() const
{
#if NONVECTORIZED
	return (float)sqrt( x * x + y * y + z * z );
#else
	return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(xyz, xyz, 0x71)));
#endif
}

float SqrLength() const
{
#if NONVECTORIZED
	return x * x + y * y + z * z;
#else
	return _mm_cvtss_f32(_mm_dp_ps(xyz, xyz, 0x7F));
#endif
}

float Dot( FVector3 a_V ) const
{
#if NONVECTORIZED
	return x * a_V.x + y * a_V.y + z * a_V.z;
#else
	__m128 a = _mm_dp_ps(xyz, a_V.xyz, 0x7F);
	return a.m128_f32[0];
#endif
}

FVector3 Cross( FVector3 v ) const
{
#if NONVECTORIZED
	return FVector3( y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x );
#else
	FVector3 result;
	result.xyz = xyz;
	 result.xyz = _mm_sub_ps(
    _mm_mul_ps(_mm_shuffle_ps(result.xyz, result.xyz, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(v.xyz, v.xyz, _MM_SHUFFLE(3, 1, 0, 2))), 
    _mm_mul_ps(_mm_shuffle_ps(result.xyz, result.xyz, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(v.xyz, v.xyz, _MM_SHUFFLE(3, 0, 2, 1)))
  );
	  return result;
#endif
}

inline void operator+=( const FVector3& a_V )
{
#if NONVECTORIZED
	x += a_V.x; y += a_V.y; z += a_V.z;
#else
	xyz = _mm_add_ps(xyz, a_V.xyz);
#endif
}

inline void operator+=( FVector3* a_V )
{
#if NONVECTORIZED
	x += a_V->x; y += a_V->y; z += a_V->z;
#else
	xyz = _mm_add_ps(xyz, a_V->xyz);
#endif
}

void operator-=( const FVector3& a_V )
{
#if NONVECTORIZED
	x -= a_V.x; y -= a_V.y; z -= a_V.z;
#else
	xyz = _mm_sub_ps(xyz, a_V.xyz);
#endif
}

void operator-=( FVector3* a_V )
{
#if NONVECTORIZED
	x -= a_V->x; y -= a_V->y; z -= a_V->z;
#else
	xyz = _mm_sub_ps(xyz, a_V->xyz);
#endif
}

void operator*=( const float f )
{
#if NONVECTORIZED
	x *= f; y *= f; z *= f;
#else
	const __m128 scalar = _mm_set1_ps(f);
	__m128 result = _mm_mul_ps(xyz, scalar);
#endif
}

void operator*=( const FVector3& a_V )
{
#if NONVECTORIZED
	x *= a_V.x; y *= a_V.y; z *= a_V.z;
#else
	xyz = _mm_mul_ps(xyz, a_V.xyz);
#endif
}

void operator*=( FVector3* a_V )
{
#if NONVECTORIZED
	x *= a_V->x; y *= a_V->y; z *= a_V->z;
#else
	xyz = _mm_mul_ps(xyz, a_V->xyz);
#endif
}

float& operator[]( int a_N )
{
	return cell[a_N];
}

FVector3 operator-() const
{
#if NONVECTORIZED
	return FVector3( -x, -y, -z );
#else
	return _mm_sub_ps(_mm_set1_ps(0.0), xyz);
#endif
}

friend FVector3 operator-( const FVector3* v1, FVector3& v2 )
{
#if NONVECTORIZED
	return FVector3( v1->x - v2.x, v1->y - v2.y, v1->z - v2.z );
#else
	return _mm_sub_ps(v1->xyz, v2.xyz);
#endif
}

friend FVector3 operator-( const FVector3& v1, FVector3* v2 )
{
#if NONVECTORIZED
	return FVector3( v1.x - v2->x, v1.y - v2->y, v1.z - v2->z );
#else
	return _mm_sub_ps(v1.xyz, v2->xyz);
#endif
}

friend FVector3 operator-( const FVector3& v1, const FVector3& v2 )
{
#if NONVECTORIZED
	return FVector3( v1.x - v2.x, v1.y - v2.y, v1.z - v2.z );
#else
	return _mm_sub_ps(v1.xyz, v2.xyz);
#endif
}

__forceinline friend FVector3 operator+ ( const FVector3& v1, const FVector3& v2 )
{
#if NONVECTORIZED
	return FVector3( v1.x + v2.x, v1.y + v2.y, v1.z + v2.z ); 
#else
	return _mm_add_ps(v1.xyz, v2.xyz);
#endif
}

__forceinline friend FVector3 operator+( const FVector3& v1, FVector3* v2 )
{
#if NONVECTORIZED
	return FVector3( v1.x + v2->x, v1.y + v2->y, v1.z + v2->z ); 
#else
	return _mm_add_ps(v1.xyz, v2->xyz);
#endif
}

friend FVector3 operator^( const FVector3& A, const FVector3& B )
{
	return FVector3(A.y*B.z-A.z*B.y,A.z*B.x-A.x*B.z,A.x*B.y-A.y*B.x);
}

friend FVector3 operator^( const FVector3& A, FVector3* B )
{
	return FVector3(A.y*B->z-A.z*B->y,A.z*B->x-A.x*B->z,A.x*B->y-A.y*B->x);
}

friend FVector3 operator*( const FVector3& v, const float f )
{
#if NONVECTORIZED
	return FVector3( v.x * f, v.y * f, v.z * f );
#else
	const __m128 scalar = _mm_set1_ps(f);
	FVector3 result;
	result.xyz = _mm_mul_ps(v.xyz, scalar);
	return result;
#endif
}

friend FVector3 operator*( const FVector3& v1, const FVector3& v2 )
{
#if NONVECTORIZED
	return FVector3( v1.x * v2.x, v1.y * v2.y, v1.z * v2.z );
#else
	return _mm_mul_ps(v1.xyz, v2.xyz);
#endif
}

friend FVector3 operator*( const float f, const FVector3& v )
{
#if NONVECTORIZED
	return FVector3( v.x * f, v.y * f, v.z * f );
#else
	const __m128 scalar = _mm_set1_ps(f);
	return _mm_mul_ps(v.xyz, scalar);
#endif
}

friend FVector3 operator/( const FVector3& v, const float f )
{
#if NONVECTORIZED
	return FVector3( v.x / f, v.y / f, v.z / f );
#else
	const __m128 scalar = _mm_set1_ps(f);
	return _mm_div_ps(v.xyz, scalar);
#endif
}

friend FVector3 operator/( const FVector3& v1, const FVector3& v2 )
{
#if NONVECTORIZED
	return FVector3( v1.x / v2.x, v1.y / v2.y, v1.z / v2.z );
#else
	return _mm_div_ps(v1.xyz, v2.xyz);
#endif
}

friend FVector3 operator/( const float f, const FVector3& v )
{
#if NONVECTORIZED
	return FVector3( v.x / f, v.y / f, v.z / f );
#else
	const __m128 scalar = _mm_set1_ps(f);
	return _mm_div_ps(v.xyz, scalar);
#endif
}

	union
	{
		struct { float x, y, z, w; };
		struct { float cell[4]; };
		__m128 xyz;
	};

};