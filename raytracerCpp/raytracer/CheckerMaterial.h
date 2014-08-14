
#ifndef _CheckerMaterial_h_
#define _CheckerMaterial_h_


#include "IMaterial.h"

struct CheckerMaterial:public IMaterial{
	float scale;
	inline CheckerMaterial(float _scale,float _reflectiveness=0):IMaterial(_reflectiveness),scale(_scale) { }

	virtual Color sample(const Ray3& ray,const Vector3& position,const Vector3& normal){
		float d=abs((floor(position.x * this->scale) + floor(position.z * this->scale)));
		d=fmod(d,2);
		return  d < 1 ? Color::black() : Color::white();
    }
};


#endif