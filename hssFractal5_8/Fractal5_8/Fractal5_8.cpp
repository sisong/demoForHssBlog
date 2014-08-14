#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <time.h>
//(  警告:代码移植到其他体系的CPU时需要重新考虑字节顺序的大端小端问题! )

#include "../hGraphic32/hGraphic32.h" //简易图像处理基础框架

#include <vector>

    struct TViewRect{
        double x0,y0,r;
    };

    static const double _divLog2=1.0/log(2.0);
    inline double log2(double x){
        return log(x)*_divLog2;
    } 

    //color=sin(i)
    inline long sinColor(double iter){
        return (long)( (sin(iter*2*3.1415926/510-3.1415926*0.5)+1)*0.5*255 );
    }

inline Color32 coloring4(const double iter,const long max_iter){
    if (iter==max_iter){
        return Color32(255,0,0);
    }else{ 
        return Color32(sinColor(iter*20),sinColor(iter*15+85),sinColor(iter*30+171));
    }
}

//---------------------------------------------------------------------------------

    inline double loglog(const double x){
        return log(abs(log(x)));
    }

inline double mandelbrot5(const double x0,const double y0,const long max_iter){
    const static double M=256;
    const static double lnln_M=loglog(M);
    double x_bck=0;
    double y_bck=0;
    double x=x0;
    double y=y0;
    long i=0;
    for (;i<max_iter;++i){
        if (x*x+y*y>=M)
            break;
        x_bck=x;
        y_bck=y;
        double tmp=x*x-y*y+x0;
        y=x*y*2+y0;
        x=tmp;
    }
    if (i!=max_iter){
        const double lnln_Z=loglog(x*x+y*y);
        const double lnln_Zbak=loglog(x_bck*x_bck+y_bck*y_bck);
        return i-2-(lnln_Z-lnln_M)/(lnln_Z-lnln_Zbak);
    }else
        return i;
}

void draw_mandelbrot5(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
    for (long y=0;y<dst.height;++y){
        for (long x=0;x<dst.width;++x) {
            double x0=(2*rect.r)*x/dst.width+rect.x0-rect.r;
            double yr=rect.r*dst.height/dst.width;
            double y0=(2*yr)*y/dst.height+rect.y0-yr;
            double iter=mandelbrot5(x0,y0,max_iter);
            dst.pixels(x,y)=coloring4(iter,max_iter);
        }
    }
}

inline double mandelbrot4_e(const double x0,const double y0,const long max_iter){
    const static double M=4;
    double x=x0;
    double y=y0;
    long i=0;
    for (;i<max_iter;++i){
        if (x*x+y*y>=M)
            break;
        double tmp=x*x-y*y+x0;
        y=x*y*2+y0;
        x=tmp;
    }
    if (i!=max_iter){
        return i +1-log2(log2(x*x+y*y));
    }else
        return i;
}

void draw_mandelbrot4_e(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
    for (long y=0;y<dst.height;++y){
        for (long x=0;x<dst.width;++x) {
            double x0=(2*rect.r)*x/dst.width+rect.x0-rect.r;
            double yr=rect.r*dst.height/dst.width;
            double y0=(2*yr)*y/dst.height+rect.y0-yr;
            double iter=mandelbrot4_e(x0,y0,max_iter);
            dst.pixels(x,y)=coloring4(iter,max_iter);
        }
    }
}

inline double mandelbrot5_e(const double x0,const double y0,const long max_iter){
    const static double M=4;
    const static double lnln_M=loglog(M);
    double x_bck=0;
    double y_bck=0;
    double x=x0;
    double y=y0;
    long i=0;
    for (;i<max_iter;++i){
        if (x*x+y*y>=M)
            break;
        x_bck=x;
        y_bck=y;
        double tmp=x*x-y*y+x0;
        y=x*y*2+y0;
        x=tmp;
    }
    if (i!=max_iter){
        const double lnln_Z=loglog(x*x+y*y);
        const double lnln_Zbak=loglog(x_bck*x_bck+y_bck*y_bck);
        return i-(lnln_Z-lnln_M)/(lnln_Z-lnln_Zbak);
    }else
        return i;
}

void draw_mandelbrot5_e(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
    for (long y=0;y<dst.height;++y){
        for (long x=0;x<dst.width;++x) {
            double x0=(2*rect.r)*x/dst.width+rect.x0-rect.r;
            double yr=rect.r*dst.height/dst.width;
            double y0=(2*yr)*y/dst.height+rect.y0-yr;
            double iter=mandelbrot5_e(x0,y0,max_iter);
            dst.pixels(x,y)=coloring4(iter,max_iter);
        }
    }
}
//---------------------------------------------------------------------------------

//浮点RGB颜色
struct Colorf{
    double R;
    double G;
    double B;
    inline Colorf(){}
    inline Colorf(const Colorf& c):R(c.R),G(c.G),B(c.B){}
    inline Colorf(const Color32& c):R(c.r*(1.0/255)),G(c.g*(1.0/255)),B(c.b*(1.0/255)){}
    inline Colorf(const double _R,const double _G,const double _B):R(_R),G(_G),B(_B){}
    inline void addColor(const Colorf& c){ R+=c.R; G+=c.G; B+=c.B; }
    inline void subColor(const Colorf& c){ R-=c.R; G-=c.G; B-=c.B; }
    inline void mulColor(const double p){ R*=p; G*=p; B*=p; }
    inline static long fToIColor(const double fc){
        if (fc<=0) 
            return 0; 
        else if (fc>=1)
            return 255; 
        else 
            return (long)(fc*255+0.49999);
    }
    inline Color32 toColor32()const{  return Color32(fToIColor(R),fToIColor(G),fToIColor(B));  }
};

inline double sinColorf(double iter){
    return (sin(iter*2*3.1415926/510-3.1415926*0.5)+1)*0.5;
}

inline Color32 coloring6(const double iter,const long max_iter,const Colorf& errorColorIn,Colorf& errorColorOut){
    Colorf color=errorColorIn;
    if (iter==max_iter){
        color.addColor(Color32(255,0,0));
    }else{ 
        color.addColor(Colorf(sinColorf(iter*20),sinColorf(iter*15+85),sinColorf(iter*30+171)));
    }
    Color32 resultColor=color.toColor32();
    errorColorOut=color;
    errorColorOut.subColor(resultColor);
    return resultColor;
}


void draw_mandelbrot6(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
    for (long y=0;y<dst.height;++y){
        Colorf errorColor(0,0,0);
        for (long x=0;x<dst.width;++x) {
            double x0=(2*rect.r)*x/dst.width+rect.x0-rect.r;
            double yr=rect.r*dst.height/dst.width;
            double y0=(2*yr)*y/dst.height+rect.y0-yr;
            double iter=mandelbrot5(x0,y0,max_iter);
            dst.pixels(x,y)=coloring6(iter,max_iter,errorColor,errorColor);
        }
    }
}


//---------------------------------------------------------------------------------

inline double mandelbrot7(double* xList,double* yList,const long max_iter){
    const static double M=256;
    const static double lnln_M=loglog(M);
    const double& x0=xList[0];
    const double& y0=yList[0];
    double x_bck=0;
    double y_bck=0;
    double x=x0;
    double y=y0;
    long i=0;
    for (;i<max_iter;++i){
        if (x*x+y*y>=M)
            break;
        x_bck=x;
        y_bck=y;
        double tmp=x*x-y*y+x0;
        y=x*y*2+y0;
        x=tmp;
        xList[i+1]=x;
        yList[i+1]=y;
    }
    if (i!=max_iter){
        const double lnln_Z=loglog(x*x+y*y);
        const double lnln_Zbak=loglog(x_bck*x_bck+y_bck*y_bck);
        return i-2-(lnln_Z-lnln_M)/(lnln_Z-lnln_Zbak);
    }else
        return i;
}
inline Color32 coloring7(const double iter,const long max_iter,double* xList,double* yList,const Colorf& errorColorIn,Colorf& errorColorOut,double k=1){
    Colorf color=errorColorIn;
    if (iter==max_iter){
        const double x=xList[max_iter];
        const double y=yList[max_iter];
        double z=sqrt(x*x+y*y);
        double zd=z-sqrt(xList[max_iter-1]*xList[max_iter-1]+yList[max_iter-1]*yList[max_iter-1]);
        color.addColor(Colorf(sinColorf(z*2000*k),sinColorf(y*x*1000*k),sinColorf(zd*1000*k)));
    }else{ 
        color.addColor(Colorf(sinColorf(iter*20*k),sinColorf(iter*15*k+85),sinColorf(iter*30*k+171)));
    }
    Color32 resultColor=color.toColor32();
    errorColorOut=color;
    errorColorOut.subColor(resultColor);
    return resultColor;
}

void draw_mandelbrot7(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
    std::vector<double> xList;
    std::vector<double> yList;
    xList.resize(max_iter+1);
    yList.resize(max_iter+1);
    for (long y=0;y<dst.height;++y){
        Colorf errorColor(0,0,0);
        for (long x=0;x<dst.width;++x) {
            double x0=(2*rect.r)*x/dst.width+rect.x0-rect.r;
            double yr=rect.r*dst.height/dst.width;
            double y0=(2*yr)*y/dst.height+rect.y0-yr;
            xList[0]=x0;
            yList[0]=y0;
            double iter=mandelbrot7(&xList[0],&yList[0],max_iter);
            dst.pixels(x,y)=coloring7(iter,max_iter,&xList[0],&yList[0],errorColor,errorColor);
        }
    }
}


//---------------------------------------------------------------------------------
long julia(const double x0,const double y0,const long max_iter,double jx0,double jy0){
    double x=x0;
    double y=y0;
    long i=0;
    for (;i<max_iter;++i){
        if (x*x+y*y>=4)
            break;
        double tmp=x*x-y*y+jx0;
        y=x*y*2+jy0;
        x=tmp;
    }
    return i;
}

inline double julia8(double* xList,double* yList,const long max_iter,double jx0,double jy0){
    const static double M=256;
    const static double lnln_M=loglog(M);
    const double& x0=xList[0];
    const double& y0=yList[0];
    double x_bck=0;
    double y_bck=0;
    double x=x0;
    double y=y0;
    long i=0;
    for (;i<max_iter;++i){
        if (x*x+y*y>=M)
            break;
        x_bck=x;
        y_bck=y;
        double tmp=x*x-y*y+jx0;
        y=x*y*2+jy0;
        x=tmp;
        xList[i+1]=x;
        yList[i+1]=y;
    }
    if (i!=max_iter){
        const double lnln_Z=loglog(x*x+y*y);
        const double lnln_Zbak=loglog(x_bck*x_bck+y_bck*y_bck);
        return i-2-(lnln_Z-lnln_M)/(lnln_Z-lnln_Zbak);
    }else
        return i;
}

void draw_julia8(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter,double jx0,double jy0){
    std::vector<double> xList;
    std::vector<double> yList;
    xList.resize(max_iter+1);
    yList.resize(max_iter+1);
    for (long y=0;y<dst.height;++y){
        Colorf errorColor(0,0,0);
        for (long x=0;x<dst.width;++x) {
            double x0=(2*rect.r)*x/dst.width+rect.x0-rect.r;
            double yr=rect.r*dst.height/dst.width;
            double y0=(2*yr)*y/dst.height+rect.y0-yr;
            xList[0]=x0;
            yList[0]=y0;
            //double iter=julia(xList[0],yList[0],max_iter,jx0,jy0);
            double iter=julia8(&xList[0],&yList[0],max_iter,jx0,jy0);
            dst.pixels(x,y)=coloring7(iter,max_iter,&xList[0],&yList[0],errorColor,errorColor,0.5);
        }
    }
}


//---------------------------------------------------------------------------------

typedef void (*TDrawMandelbrotProc)(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter);
typedef void (*TDrawJuliaProc)(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter,double jx0,double jy0);

void test(const char* proc_name,const void* fproc,bool isJulia=false,double jx0=0,double jy0=0){
    const long max_iter=1000;
    TViewRect rect;
    if (!isJulia){
        rect.x0=-0.5;
        rect.y0=0;
        rect.r=2;
    }else{
        rect.x0=0;
        rect.y0=0;
        rect.r=1.6;
    }

    TPixels32 dstPic;
    dstPic.resizeFast(640,480);

    std::cout<<proc_name<<": ";
    if (!isJulia)
        ((TDrawMandelbrotProc)fproc)(dstPic.getRef(),rect,max_iter);
    else
        ((TDrawJuliaProc)fproc)(dstPic.getRef(),rect,max_iter,jx0,jy0);
    std::cout<<" ok"<<std::endl;

    { //save pic
        std::string dstFileName(proc_name);
        dstFileName+=".bmp";
#if defined(__APPLE__) && defined(__MACH__)
        TFileOutputStream bmpOutStream("/Users/hss/Desktop/GraphicDemo/ColorToGray/"+dstFileName);
#else
        TFileOutputStream bmpOutStream(dstFileName.c_str());
#endif
        TBmpFile::save(dstPic.getRef(),&bmpOutStream);//保存结果图片
    }
}

//等待一个回车输入
static void waitInputChar(){
    while (true){
        int c=getchar();
        if (c=='\n')
            break;
    }
}
//---------------------------------------------------------------------------------

int main(){

    test("draw_mandelbrot4_e" ,draw_mandelbrot4_e);
    test("draw_mandelbrot5_e" ,draw_mandelbrot5_e);
    test("draw_mandelbrot5"   ,draw_mandelbrot5);
    test("draw_mandelbrot6"   ,draw_mandelbrot6);
    test("draw_mandelbrot7"   ,draw_mandelbrot7);
    test("draw_julia8_0"      ,draw_julia8,true,-0.74543,0.11301);
    test("draw_julia8_1"      ,draw_julia8,true,0.28888,0.012325);
    test("draw_julia8_2"      ,draw_julia8,true,-0.81442,0.19633);

    std::cout<<std::endl<<" 运行完成. ";
    //waitInputChar();
    return 0;
}
