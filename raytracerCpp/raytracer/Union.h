

#ifndef _Union_h_
#define _Union_h_

#include "IGeometry.h"
#include <vector>

struct Union:public IGeometry{
	std::vector<IGeometry*> geometries;
	inline Union() { }

	void push(IGeometry* geometry){
		geometries.push_back(geometry);
	}

    inline void initialize() {
		long size=geometries.size();
        for (long i=0;i<size;++i)
            geometries[i]->initialize();
    }

    virtual IntersectResult intersect(const Ray3& ray){ 
		const float Infinity=1e30; 
        float minDistance = Infinity;
		IntersectResult minResult = IntersectResult::noHit();
		long size=this->geometries.size();
		for (long i=0;i<size;++i){
            IntersectResult result = this->geometries[i]->intersect(ray);
            if (result.geometry && (result.distance < minDistance)) {
                minDistance = result.distance;
                minResult = result;
            }
        }
        return minResult;
	}
	
	inline void clear(){
		long size=geometries.size();
        for (long i=0;i<size;++i)
            delete geometries[i];
		geometries.clear();
	}

	virtual ~Union(){
		clear();
	}
};

#endif