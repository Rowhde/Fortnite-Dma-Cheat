#pragma once

#include <corecrt_math_defines.h>
#include <cmath>
#include <math.h>
#include <algorithm>

inline float _sqrt(float fNumber)
{
	long i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = fNumber * 0.5F;
	y = fNumber;
	i = *(long*)&y;
	i = 0x5f3759df - (i >> 1);
	y = *(float*)&i;
	y = y * (threehalfs - (x2 * y * y));
	return 1 / y;
}

struct vec2_t
{
	float x, y;

	vec2_t operator+(const vec2_t& v) const
	{
		return vec2_t{ x + v.x, y + v.y };
	}

	vec2_t operator-(const vec2_t& v) const
	{
		return vec2_t{ x - v.x, y - v.y };
	}

	vec2_t operator*(float s) const
	{
		return vec2_t{ x * s, y * s };
	}

	vec2_t operator/(float s) const
	{
		return vec2_t{ x / s, y / s };
	}

	vec2_t& operator*=(float s)
	{
		x *= s;
		y *= s;
		return *this;
	}

	vec2_t& operator+=(const vec2_t& v)
	{
		x += v.x;
		y += v.y;
		return *this;
	}

	vec2_t& operator-=(const vec2_t& v)
	{
		x -= v.x;
		y -= v.y;
		return *this;
	}

	float length() const
	{
		return _sqrt(x * x + y * y);
	}

	

	void normalize()
	{
		float fLength = length();
		if (fLength != 0)
		{
			x /= fLength;
			y /= fLength;
		}
	}
};
class FRotator
{
public:
	FRotator() : Pitch(0.f), Yaw(0.f), Roll(0.f)
	{

	}

	FRotator(double _Pitch, double _Yaw, double _Roll) : Pitch(_Pitch), Yaw(_Yaw), Roll(_Roll)
	{

	}
	~FRotator()
	{

	}

	double Pitch;
	double Yaw;
	double Roll;

};
struct vec3_t
{
	double x, y, z;
	inline double Distance(const vec3_t& v) const
	{
		return sqrt(pow(v.x - x, 2.0) + pow(v.y - y, 2.0) + pow(v.z - z, 2.0));
	}
};
class FVector
{

public:
	FVector() : x(0.f), y(0.f), z(0.f)
	{

	}

	FVector(float _x, float _y, float _z) : x(_x), y(_y), z(_z)
	{

	}
	~FVector()
	{

	}

	double x;
	double y;
	double z;

	inline float Dot(FVector v)
	{
		return x * v.x + y * v.y + z * v.z;
	}

	inline double Distance(FVector v)
	{
		return double(sqrtf(powf(v.x - x, 2.0) + powf(v.y - y, 2.0) + powf(v.z - z, 2.0)));
	}

	inline bool is_valid(FVector v)
	{
		return (x != 0 && y != 0 && z != 0);
	}

	FVector operator+(FVector v)
	{
		return FVector(x + v.x, y + v.y, z + v.z);
	}

	FVector operator-(FVector v)
	{
		return FVector(x - v.x, y - v.y, z - v.z);
	}

	FVector operator*(float number) const
	{
		return FVector(x * number, y * number, z * number);
	}

};
inline float dot(vec3_t a, vec3_t b)
{
	return (a.x * b.x + a.y * b.y + a.z * b.z);
}