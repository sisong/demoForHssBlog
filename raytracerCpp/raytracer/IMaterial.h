
#ifndef _IMaterial_h_
#define _IMaterial_h_

#include "Vector3.h"
#include "Color.h"

struct IMaterial{
	float   reflectiveness;
	inline IMaterial(float _reflectiveness=0):reflectiveness(_reflectiveness){}

	virtual Color sample(const Ray3& ray,const Vector3& position,const Vector3& normal)=0;
	virtual ~IMaterial(){}
};

#endif