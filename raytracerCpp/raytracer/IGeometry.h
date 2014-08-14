
#ifndef _IGeometry_h_
#define _IGeometry_h_


#include "Ray3.h"
#include "IntersectResult.h"
#include "IMaterial.h"

struct IGeometry{
	IMaterial* material;
	inline IGeometry():material(0){}
	virtual ~IGeometry(){
		if (material) {
			delete material;
			material=0;
		}
	}

	virtual void initialize(){}
    virtual IntersectResult intersect(const Ray3& ray)=0; //todo:做返回值优化
};

#endif