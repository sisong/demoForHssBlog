
#ifndef _Vector3_h_
#define _Vector3_h_

#include <math.h>		
const float PI=3.141592653589793238462643383;

struct Vector3{
	float x;
	float y;
	float z;

	inline Vector3(const Vector3& v):x(v.x),y(v.y),z(v.z) {}
	inline Vector3(float _x,float _y,float _z):x(_x),y(_y),z(_z) {}
	inline float length()const { return sqrt(sqrLength()); }
	inline float sqrLength()const { return x*x+y*y+z*z; }
	inline Vector3 normalize()const { float inv = 1/length(); return Vector3(x*inv,y*inv,z*inv); }
    inline Vector3 negate()const { return Vector3(-x, -y, -z); }
    inline Vector3 add(const Vector3& v)const { return Vector3(x + v.x, y + v.y, z + v.z); }
    inline Vector3 subtract(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
    inline Vector3 multiply(float f) const { return Vector3(x * f, y * f, z * f); }
    inline Vector3 divide(float f) const { float invf = 1/f; return Vector3(x * invf, y * invf, z * invf); }
    inline float dot(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }
    inline Vector3 cross(const Vector3& v) const { return Vector3(-z * v.y + y * v.z, z * v.x - x * v.z, -y * v.x + x * v.y); }

	static inline Vector3 zero(){ return Vector3(0,0,0); }
};



#endif