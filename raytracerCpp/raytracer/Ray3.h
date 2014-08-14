
#ifndef _Ray3_h_
#define _Ray3_h_

#include "Vector3.h"

struct Ray3 {
	Vector3 origin;
	Vector3 direction;

	inline Ray3(const Ray3& r):origin(r.origin),direction(r.direction){}
	inline Ray3(const Vector3& _origin,const Vector3& _direction):origin(_origin),direction(_direction){}
	inline Vector3 getPoint(float t)const{ return origin.add(direction.multiply(t)); }
};

#endif