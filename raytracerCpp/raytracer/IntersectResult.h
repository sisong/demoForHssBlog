
#ifndef _IntersectResult_h_
#define _IntersectResult_h_

#include "Vector3.h"
//#include "IGeometry.h"
struct IGeometry;

struct IntersectResult{
	IGeometry*	geometry;
	float		distance;
	Vector3		position;
	Vector3		normal;
	inline IntersectResult():geometry(0),distance(0),position(Vector3::zero()),normal(Vector3::zero()){ }

	static inline IntersectResult noHit() { return IntersectResult(); }
};


#endif