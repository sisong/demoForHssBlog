

#ifndef _PerspectiveCamera_h_
#define _PerspectiveCamera_h_

#include "Ray3.h"

struct PerspectiveCamera{
	Vector3 eye;
	Vector3 front;
	Vector3 refUp;
	float   fov;

	Vector3 right;
	Vector3 up;
	float   fovScale;

	inline PerspectiveCamera(const Vector3& _eye,const Vector3& _front,const Vector3& _refUp,float _fov)
		:eye(_eye),front(_front),refUp(_refUp),fov(_fov),
		right(Vector3::zero()),up(Vector3::zero()),fovScale(0) { }

	inline void initialize(){
        right = front.cross(refUp);
        up = right.cross(front);
        fovScale = tan(fov* (PI  * 0.5f / 180)) * 2;
    }

    inline Ray3 generateRay(float x,float y)const{
        Vector3 r = right.multiply((x - 0.5f) * fovScale);
        Vector3 u = up.multiply((y - 0.5f) * fovScale);
        return Ray3(eye,front.add(r).add(u).normalize());
    }
};


#endif