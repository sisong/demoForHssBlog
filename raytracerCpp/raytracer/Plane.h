
#ifndef _Plane_h_
#define _Plane_h_

#include "IGeometry.h"

struct Plane:public IGeometry{
 	Vector3 normal;
	float   d;

	Vector3 position;

	inline Plane(const Plane& p):normal(p.normal),d(p.d), position(Vector3::zero()) {  }
	inline Plane(const Vector3& _normal,float _d):normal(_normal),d(_d), position(Vector3::zero()) {  }

    inline void initialize() {
        position = normal.multiply(d);
    }

    virtual IntersectResult intersect(const Ray3& ray){ 
        float a = ray.direction.dot(this->normal);
        if (a >= 0)
			return IntersectResult::noHit();

        float b = this->normal.dot(ray.origin.subtract(this->position));
        IntersectResult result;
        result.geometry = this;
        result.distance = -b / a;
        result.position = ray.getPoint(result.distance);
        result.normal = this->normal;
        return result;
	}
	
};

#endif