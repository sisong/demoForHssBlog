
#ifndef _Sphere_h_
#define _Sphere_h_

#include "IGeometry.h"

struct Sphere:public IGeometry{
	Vector3 center;
	float   radius;
	inline Sphere(const Sphere& s):center(s.center),radius(s.radius) {  }
	inline Sphere(const Vector3& _center,float _radius):center(_center),radius(_radius) {  }
    inline float sqrRadius()const{ return radius*radius; }

    virtual IntersectResult intersect(const Ray3& ray){ 
        Vector3 v = ray.origin.subtract(this->center);
        float a0 = v.sqrLength() - this->sqrRadius();
        float DdotV = ray.direction.dot(v);

        if (DdotV <= 0) {
            float discr = DdotV * DdotV - a0;
            if (discr >= 0) {
                IntersectResult result;
                result.geometry = this;
                result.distance = -DdotV - sqrt(discr);
                result.position = ray.getPoint(result.distance);
                result.normal = result.position.subtract(this->center).normalize();
                return result;
            }
        }

		return IntersectResult::noHit();
	}
	
};

#endif