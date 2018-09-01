
#pragma once
#include "stdio.h"
#include "windows.h"
#include "math.h"
#include "FMath.h"

class FVector2
{
public:
	inline FVector2() : x(0.0f), y(0.0f) {}
	inline FVector2(float a_X, float a_Y) : x(a_X), y(a_Y) {}
	FVector2(const FVector2& other) : x(other.x), y(other.y) {}
	FVector2(FVector2&& other) : x(other.x), y(other.y) {}
	FVector2& operator=(FVector2&& other) { x = other.x; y = other.y; return *this; }
	//FVector2& operator=(FVector2& other) { x = other.x; y = other.y; z = other.z; return *this; }
	FVector2& operator=(const FVector2& other) { x = other.x; y = other.y; return *this; }
	
	void Set(float a_X, float a_Y)
	{
		x = a_X;
		y = a_Y;
	}
	void Normalize()
	{
		float length = 1.0f / Length();
		x *= length;
		y *= length;
	}


	FVector2 Normalized()
	{
		float length = 1.0f / Length();
		float ax = x*length;
		float ay = y*length;
		return FVector2(ax, ay);
	}

	float Length() const
	{
		return (float)sqrt(SqrLength());
	}

	float SqrLength() const
	{
		return x * x + y * y;
	}

	float Dot(const FVector2& a_V) const
	{
		return x * a_V.x + y * a_V.y;
	}

	inline void operator+=(const FVector2& a_V)
	{
		x += a_V.x; y += a_V.y;
	}

	inline void operator+=(FVector2* a_V)
	{
		x += a_V->x; y += a_V->y;
	}

	void operator-=(const FVector2& a_V)
	{
		x -= a_V.x; y -= a_V.y;
	}

	void operator-=(FVector2* a_V)
	{
		x -= a_V->x; y -= a_V->y;
	}

	void operator*=(const float f)
	{
		x *= f; y *= f;
	}

	void operator*=(const FVector2& a_V)
	{
		x *= a_V.x; y *= a_V.y;
	}

	void operator*=(FVector2* a_V)
	{
		x *= a_V->x; y *= a_V->y;
	}

	float& operator[](int a_N)
	{
		return cell[a_N];
	}

	FVector2 operator-() const
	{
		return FVector2(-x, -y);
	}

	friend FVector2 operator-(const FVector2* v1, FVector2& v2)
	{
		return FVector2(v1->x - v2.x, v1->y - v2.y);
	}

	friend FVector2 operator-(const FVector2& v1, FVector2* v2)
	{
		return FVector2(v1.x - v2->x, v1.y - v2->y);
	}

	friend FVector2 operator-(const FVector2& v1, const FVector2& v2)
	{
		return FVector2(v1.x - v2.x, v1.y - v2.y);
	}

	__forceinline FVector2 operator+ (const FVector2& v2)
	{
		FVector2 v1 = *this;
		return FVector2(v1.x + v2.x, v1.y + v2.y);
	}

	__forceinline FVector2 operator+(FVector2* v2)
	{
		FVector2 v1 = *this;
		return FVector2(v1.x + v2->x, v1.y + v2->y);
	}

	friend FVector2 operator+(const FVector2& v1, const FVector2& v2)
	{
		return FVector2(v1.x + v2.x, v1.y + v2.y);
	}
	
	friend FVector2 operator*(const FVector2& v, const float f)
	{
		return FVector2(v.x * f, v.y * f);
	}

	friend FVector2 operator*(const FVector2& v1, const FVector2& v2)
	{
		return FVector2(v1.x * v2.x, v1.y * v2.y);
	}

	friend FVector2 operator*(const float f, const FVector2& v)
	{
		return FVector2(v.x * f, v.y * f);
	}

	friend FVector2 operator/(const FVector2& v, const float f)
	{
		return FVector2(v.x / f, v.y / f);
	}

	friend FVector2 operator/(const FVector2& v1, const FVector2& v2)
	{
		return FVector2(v1.x / v2.x, v1.y / v2.y);
	}

	friend FVector2 operator/(const float f, const FVector2& v)
	{
		return FVector2(v.x / f, v.y / f);
	}

	union
	{
		struct { float x, y; };
		struct { float cell[2]; };
	};

};