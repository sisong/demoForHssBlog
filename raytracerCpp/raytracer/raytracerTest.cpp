#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <time.h>
#include <algorithm>


#include "../hGraphic32/hGraphic32.h" //简易图像处理基础框架

#include "Sphere.h"
#include "Plane.h"
#include "Union.h"
#include "CheckerMaterial.h"
#include "PhongMaterial.h"

#include "PerspectiveCamera.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//简易速度测试框架

//等待一个回车输入
static void waitInputChar(){
    while (true){
        int c=getchar();
        if (c=='\n')
            break;
    }
}

static std::string fpsToStr(double fps){
    #pragma warning(disable:4996)
    char buff[64];
    sprintf(buff, "%.2f", fps);
    std::string result(buff);
    const long fpsStrBestSize=8;
    if (result.size()<fpsStrBestSize)
        result=std::string(fpsStrBestSize-result.size(),' ')+result;
    return result;
}


typedef void (*T_proc)(const TPixels32Ref& dst);

void test(const char* procName,const T_proc testPro,const long runCount){
    TPixels32 dstPic;
	dstPic.resizeFast(256,256);
    std::cout<<procName<<": ";
    clock_t t0=clock();
    for (long c=0;c<runCount;++c){
        testPro(dstPic.getRef());
    }
    t0=clock()-t0;
    double fps=runCount/(t0*1.0/CLOCKS_PER_SEC);
    std::cout<<fpsToStr(fps)<<" 帧/秒"<<std::endl;

	{//保存结果图片
		TFileOutputStream bmpOutStream((std::string(procName)+".bmp").c_str());
		TBmpFile::save(dstPic.getRef(),&bmpOutStream);
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void canvasTest(const TPixels32Ref& ctx) {
	if (ctx.getIsEmpty()) 
        return;

	long w = ctx.width;
	long h = ctx.height;
	ctx.fillColor(Color32(0,0,0,0));

	Color32* pixels = ctx.pdata;
    
    for (long y = 0; y < h; ++y){
        for (long x = 0; x < w; ++x){
            pixels[x].r = (UInt8)( x * 255 / w );
			pixels[x].g = (UInt8)( y * 255 / h );
			pixels[x].b = 0;
			pixels[x].a = 255;
        }
		(UInt8*&)pixels+=ctx.byte_width;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void renderDepth(const TPixels32Ref& ctx) {
	if (ctx.getIsEmpty()) 
        return;

	Union scene;
	scene.push(new Sphere(Vector3(0, 10, -10), 10));
	scene.push(new Plane(Vector3(0, 1, 0), 0));

	PerspectiveCamera camera( Vector3(0, 10, 10),Vector3(0, 0, -1),Vector3(0, 1, 0), 90);
	long maxDepth=20;


	long w = ctx.width;
	long h = ctx.height;
	ctx.fillColor(Color32(0,0,0,0));

	Color32* pixels = ctx.pdata;
    
    scene.initialize();
    camera.initialize();

	float dx=1.0f/w;
	float dy=1.0f/h;
	float dD=255.0f/maxDepth;
    for (long y = 0; y < h; ++y){
		float sy = 1 - dy*y;
        for (long x = 0; x < w; ++x){
			float sx =dx*x;            
            Ray3 ray(camera.generateRay(sx, sy));
            IntersectResult result = scene.intersect(ray);
            if (result.geometry) {
				UInt8 depth = (UInt8)( 255 - std::min(result.distance*dD,255.0f) );
				pixels[x].r = depth;
				pixels[x].g = depth;
				pixels[x].b = depth;
				pixels[x].a = 255;
			}
        }
		(UInt8*&)pixels+=ctx.byte_width;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void renderNormal(const TPixels32Ref& ctx) {
	if (ctx.getIsEmpty()) 
        return;

	Sphere scene(Vector3(0, 10, -10), 10);
	PerspectiveCamera camera( Vector3(0, 10, 10),Vector3(0, 0, -1),Vector3(0, 1, 0), 90);
	long maxDepth=20;


	long w = ctx.width;
	long h = ctx.height;
	ctx.fillColor(Color32(0,0,0,0));

	Color32* pixels = ctx.pdata;
    
    scene.initialize();
    camera.initialize();

	float dx=1.0f/w;
	float dy=1.0f/h;
	float dD=255.0f/maxDepth;
    for (long y = 0; y < h; ++y){
		float sy = 1 - dy*y;
        for (long x = 0; x < w; ++x){
			float sx =dx*x;            
            Ray3 ray(camera.generateRay(sx, sy));
            IntersectResult result = scene.intersect(ray);
            if (result.geometry) {
				pixels[x].r = (UInt8)( (result.normal.x + 1) * 128);
				pixels[x].g = (UInt8)( (result.normal.y + 1) * 128);
				pixels[x].b = (UInt8)( (result.normal.z + 1) * 128);
				pixels[x].a = 255;
			}
        }
		(UInt8*&)pixels+=ctx.byte_width;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void rayTrace(const TPixels32Ref& ctx) {
	if (ctx.getIsEmpty()) 
        return;

	Plane*  plane = new Plane(Vector3(0, 1, 0), 0);
	Sphere* sphere1 = new Sphere(Vector3(-10, 10, -10), 10);
	Sphere* sphere2 = new Sphere(Vector3(10, 10, -10), 10);
	plane->material = new CheckerMaterial(0.1f);
	sphere1->material = new PhongMaterial(Color::red(), Color::white(), 16);
	sphere2->material = new PhongMaterial(Color::blue(), Color::white(), 16);
	Union scene;
	scene.push(plane);
	scene.push(sphere1);
	scene.push(sphere2);
	PerspectiveCamera camera( Vector3(0, 5, 15),Vector3(0, 0, -1),Vector3(0, 1, 0), 90);

	long w = ctx.width;
	long h = ctx.height;
	ctx.fillColor(Color32(0,0,0,0));

	Color32* pixels = ctx.pdata;
    
    scene.initialize();
    camera.initialize();

	float dx=1.0f/w;
	float dy=1.0f/h;
    for (long y = 0; y < h; ++y){
		float sy = 1 - dy*y;
        for (long x = 0; x < w; ++x){
			float sx =dx*x;            
            Ray3 ray(camera.generateRay(sx, sy));
            IntersectResult result = scene.intersect(ray);
            if (result.geometry) {
				Color color = result.geometry->material->sample(ray, result.position, result.normal);
				color.saturate();
				pixels[x].r = (UInt8)( color.r*255);
				pixels[x].g = (UInt8)( color.g*255);
				pixels[x].b = (UInt8)( color.b*255);
				pixels[x].a = 255;
			}
        }
		(UInt8*&)pixels+=ctx.byte_width;
    }
}




//////////////////////////////////////////////////////////////////////////////////////////////////////
			Color rayTraceRecursive(IGeometry* scene,const Ray3& ray,long maxReflect) {
				IntersectResult result = scene->intersect(ray);
			    
				if (result.geometry){
					float reflectiveness = result.geometry->material->reflectiveness;
					Color color = result.geometry->material->sample(ray, result.position, result.normal);
					color = color.multiply(1 - reflectiveness);
			        
					if ((reflectiveness > 0) && (maxReflect > 0)) {
						Vector3 r = result.normal.multiply(-2 * result.normal.dot(ray.direction)).add(ray.direction);
						Ray3 ray = Ray3(result.position, r);
						Color reflectedColor = rayTraceRecursive(scene, ray, maxReflect - 1);
						color = color.add(reflectedColor.multiply(reflectiveness));
					}
					return color;
				}else
					return Color::black();
			}

void rayTraceRecursive(const TPixels32Ref& ctx) {
	if (ctx.getIsEmpty()) 
        return;

	Plane*  plane = new Plane(Vector3(0, 1, 0), 0);
	Sphere* sphere1 = new Sphere(Vector3(-10, 10, -10), 10);
	Sphere* sphere2 = new Sphere(Vector3(10, 10, -10), 10);
	plane->material = new CheckerMaterial(0.1f,0.5);
	sphere1->material = new PhongMaterial(Color::red(), Color::white(), 16,0.25);
	sphere2->material = new PhongMaterial(Color::blue(), Color::white(), 16,0.25);
	Union scene;
	scene.push(plane);
	scene.push(sphere1);
	scene.push(sphere2);
	PerspectiveCamera camera( Vector3(0, 5, 15),Vector3(0, 0, -1),Vector3(0, 1, 0), 90);
	long maxReflect=3;

	long w = ctx.width;
	long h = ctx.height;
	ctx.fillColor(Color32(0,0,0,0));

	Color32* pixels = ctx.pdata;
    
    scene.initialize();
    camera.initialize();

	float dx=1.0f/w;
	float dy=1.0f/h;
    for (long y = 0; y < h; ++y){
		float sy = 1 - dy*y;
        for (long x = 0; x < w; ++x){
			float sx =dx*x;            
            Ray3 ray(camera.generateRay(sx, sy));
			Color color = rayTraceRecursive(&scene, ray, maxReflect);
			color.saturate();
			pixels[x].r = (UInt8)( color.r*255);
			pixels[x].g = (UInt8)( color.g*255);
			pixels[x].b = (UInt8)( color.b*255);
			pixels[x].a = 255;
        }
		(UInt8*&)pixels+=ctx.byte_width;
    }
}




//////////////////////////////////////////////////////////////////////////////////////////////////////

int main(){
    std::cout<<" 请输入回车键开始测试(可以把进程优先级设置为“实时”)> ";
    waitInputChar();
    std::cout<<std::endl;

    test("canvasTest"		,canvasTest			,2000);	
    test("renderDepth"		,renderDepth		,100);	
    test("renderNormal"		,renderNormal		,200);	
    test("rayTrace"			,rayTrace			,50);	
    test("rayTraceRecursive",rayTraceRecursive	,50);	
	
	

    std::cout<<std::endl<<" 测试完成. ";
    waitInputChar();
    return 0;
}
