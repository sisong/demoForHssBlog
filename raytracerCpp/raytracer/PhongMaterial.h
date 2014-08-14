
#ifndef _PhongMaterial_h_
#define _PhongMaterial_h_


#include "IMaterial.h"

// global temp
static Vector3 lightDir = Vector3(1, 1, 1).normalize();
static Color lightColor = Color::white();

struct PhongMaterial:public IMaterial{
	Color   diffuse;
	Color   specular;
	float   shininess;

	inline PhongMaterial(const Color& _diffuse,const Color& _specular,const float& _shininess,float _reflectiveness=0)
		:IMaterial(_reflectiveness),diffuse(_diffuse),specular(_specular),shininess(_shininess){ }

	virtual Color sample(const Ray3& ray,const Vector3& position,const Vector3& normal){
        float NdotL = normal.dot(lightDir);
        Vector3 H = (lightDir.subtract(ray.direction)).normalize();
        float NdotH = normal.dot(H);
		Color diffuseTerm = this->diffuse.multiply(std::max(NdotL, (float)0));
		Color specularTerm = this->specular.multiply(pow(std::max(NdotH, (float)0), this->shininess));
        return lightColor.modulate(diffuseTerm.add(specularTerm));
    }
};


#endif