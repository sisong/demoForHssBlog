#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <time.h>
//(  警告:代码移植到其他体系的CPU时需要重新考虑字节顺序的大端小端问题! )

#include "../hGraphic32/hGraphic32.h" //简易图像处理基础框架

#include <vector>

    struct TViewRect{
        double x0,y0,r,seta;
    };

//---------------------------------------------------------------------------------

    inline double loglog(const double x){
        return log(abs(log(x)));
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

//---------------------------------------------------------------------------------

inline double mandelbrot9(double* xList,double* yList,const long max_iter,long& out_i){
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
    out_i=i;
    if (i!=max_iter){
        const double lnln_Z=loglog(x*x+y*y);
        const double lnln_Zbak=loglog(x_bck*x_bck+y_bck*y_bck);
        return i-2-(lnln_Z-lnln_M)/(lnln_Z-lnln_Zbak);
    }else
        return i;
}

inline Color32 coloring9_s(const double iter,long i,const long max_iter,double* xList,double* yList,const Colorf& errorColorIn,Colorf& errorColorOut){
    Colorf color=errorColorIn;
    if (iter==max_iter){
        const double x=xList[max_iter];
        const double y=yList[max_iter];
        double z=sqrt(x*x+y*y);
        double zd=z-sqrt(xList[max_iter-1]*xList[max_iter-1]+yList[max_iter-1]*yList[max_iter-1]);
        color.addColor(Colorf(sinColorf(z*2000),sinColorf(y*x*1000),sinColorf(zd*1000)));
    }else{ 
        const double x=xList[i];
        const double y=yList[i];
        double z=sqrt(x*x+y*y);
        color.addColor(Colorf(sinColorf(z*20/20-236),sinColorf(z*15/20+221),sinColorf(z*30/20-254)));
    }
    Color32 resultColor=color.toColor32();
    errorColorOut=color;
    errorColorOut.subColor(resultColor);
    return resultColor;
}

void draw_mandelbrot9_s(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
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
            long i;
            double iter=mandelbrot9(&xList[0],&yList[0],max_iter,i);
            dst.pixels(x,y)=coloring9_s(iter,i,max_iter,&xList[0],&yList[0],errorColor,errorColor);
        }
    }
}

    inline double smoothXY9(double iter,long i,const double* datas){
        const double s=iter+5-(long)(iter+5); //用5避免iter可能为负的情况
        return datas[i]*s+datas[i-1]*(1-s); //线性关系插值
        //return pow(abs(datas[i]),s)*pow(abs(datas[i-1]),(1-s)); //指数关系插值
    }
inline Color32 coloring9(const double iter,long i,const long max_iter,double* xList,double* yList,const Colorf& errorColorIn,Colorf& errorColorOut){
    Colorf color=errorColorIn;
    if (iter==max_iter){
        const double x=xList[max_iter];
        const double y=yList[max_iter];
        double z=sqrt(x*x+y*y);
        double zd=z-sqrt(xList[max_iter-1]*xList[max_iter-1]+yList[max_iter-1]*yList[max_iter-1]);
        color.addColor(Colorf(sinColorf(z*2000),sinColorf(y*x*1000),sinColorf(zd*1000)));
    }else{ 
        const double x=smoothXY9(iter,i,xList);
        const double y=smoothXY9(iter,i,yList);
        double z=sqrt(x*x+y*y);
        color.addColor(Colorf(sinColorf(z*20/3-236),sinColorf(z*15/3+221),sinColorf(z*30/3-254)));
    }
    Color32 resultColor=color.toColor32();
    errorColorOut=color;
    errorColorOut.subColor(resultColor);
    return resultColor;
}

void draw_mandelbrot9(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
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
            long i;
            double iter=mandelbrot9(&xList[0],&yList[0],max_iter,i);
            dst.pixels(x,y)=coloring9(iter,i,max_iter,&xList[0],&yList[0],errorColor,errorColor);
        }
    }
}

//---------------------------------------------------------------------------------


        inline double SinXDivX(double x) {
            if (x<0) x=-x; //x=abs(x);
            double x2=x*x;
            double x3=x2*x;

            //该函数计算插值曲线sin(x*PI)/(x*PI)的值 //PI=3.1415926535897932385; 
            //下面是它的近似拟合表达式
            /////////////////////////////////////////////////////
            //一元立方插值的插值核(一元三次的方程)
            //        (a+2)*x^3 - (a+3)*x^2 +1       (0<=x<1)  
            // h(x)=  a*x^3 - 5*a*x^2 + 8*a*x - 4*a  (1<=x<2)
            //        0                              (2<=x)
            const float a = -0.5; //a可以取各种值效果都可以试试
            if (x<=1)
              return (a+2)*x3 - (a+3)*x2 + 1;
            else if (x<=2) 
              return a*x3 - (5*a)*x2 + (8*a)*x - (4*a);
            else
              return 0;
        } 
    inline double smoothXY10(double iter,long i,const double* datas){
        const double s=iter+5-(long)(iter+5);
        return datas[i]*SinXDivX(2-s)+datas[i-1]*SinXDivX(1-s)+datas[i-2]*SinXDivX(s)+datas[i-3]*SinXDivX(1+s);
    }

inline Color32 coloring10(const double iter,long i,const long max_iter,double* xList,double* yList,const Colorf& errorColorIn,Colorf& errorColorOut){
    Colorf color=errorColorIn;
    if (iter==max_iter){
        const double x=xList[max_iter];
        const double y=yList[max_iter];
        double z=sqrt(x*x+y*y);
        double zd=z-sqrt(xList[max_iter-1]*xList[max_iter-1]+yList[max_iter-1]*yList[max_iter-1]);
        color.addColor(Colorf(sinColorf(z*2000),sinColorf(y*x*1000),sinColorf(zd*1000)));
    }else{ 
        const double x=smoothXY10(iter,i,xList);
        const double y=smoothXY10(iter,i,yList);
        double z=sqrt(x*x+y*y);
        color.addColor(Colorf(sinColorf(z*20*1.5-236),sinColorf(z*15*1.5+221),sinColorf(z*30*1.5-254)));
    }
    Color32 resultColor=color.toColor32();
    errorColorOut=color;
    errorColorOut.subColor(resultColor);
    return resultColor;
}

void draw_mandelbrot10(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
    const long safe_border=4;
    std::vector<double> xList;
    std::vector<double> yList;
    xList.resize(max_iter+1+safe_border);
    yList.resize(max_iter+1+safe_border);
    for (long y=0;y<dst.height;++y){
        Colorf errorColor(0,0,0);
        for (long x=0;x<dst.width;++x) {
            double x0=(2*rect.r)*x/dst.width+rect.x0-rect.r;
            double yr=rect.r*dst.height/dst.width;
            double y0=(2*yr)*y/dst.height+rect.y0-yr;
            for (long f=0;f<=safe_border;++f){
                xList[f]=x0;
                yList[f]=y0;
            }
            long i;
            double iter=mandelbrot9(&xList[safe_border],&yList[safe_border],max_iter,i);
            dst.pixels(x,y)=coloring10(iter,i,max_iter,&xList[safe_border],&yList[safe_border],errorColor,errorColor);
        }
    }
}


        inline double BSpline(double x){
            if (x<0) x=-x; //x=abs(x);
            double x2=x*x;
            double x3=x2*x;
            //B样条插值的插值核
            //       x^3/2 - x^2 + 2/3               (0<=x<1)
            //h(x) = (2-x)^3/6 ==(8-12x+6x^2-x^3)/6  (1<=x<2)
            //       0                               (2<=x)
            if (x<1)
              return x3*0.5 - x2 + (2.0/3.0);
            else if (x<2)
              return (1.0/6.0)*(-x3+6*x2-12*x+8);
            else
              return 0;
        } 
        inline double smoothMap(double x){
            return x;
            //return log(abs(x)+1e-100);
            //return sqrt(abs(x));
            //return pow(abs(x),1.15);
        }
    inline double smoothXY10B(double iter,long i,const double* datas){
        const double s=(iter+5-(long)(iter+5));
        return smoothMap(datas[i  ])*BSpline(2-s)
              +smoothMap(datas[i-1])*BSpline(1-s)
              +smoothMap(datas[i-2])*BSpline(s)
              +smoothMap(datas[i-3])*BSpline(1+s);
    }

inline Color32 coloring10B(const double iter,long i,const long max_iter,double* xList,double* yList,const Colorf& errorColorIn,Colorf& errorColorOut){
    Colorf color=errorColorIn;
    if (iter==max_iter){
        const double x=xList[max_iter];
        const double y=yList[max_iter];
        double z=sqrt(x*x+y*y);
        double zd=z-sqrt(xList[max_iter-1]*xList[max_iter-1]+yList[max_iter-1]*yList[max_iter-1]);
        color.addColor(Colorf(sinColorf(z*2000),sinColorf(y*x*1000),sinColorf(zd*1000)));
    }else{ 
        const double x=smoothXY10B(iter,i,xList);
        const double y=smoothXY10B(iter,i,yList);
        double z=sqrt(x*x+y*y);
        color.addColor(Colorf(sinColorf(z*20*2-236),sinColorf(z*15*2+221),sinColorf(z*30*2-254)));
    }
    Color32 resultColor=color.toColor32();
    errorColorOut=color;
    errorColorOut.subColor(resultColor);
    return resultColor;
}

void draw_mandelbrot10B(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
    const long safe_border=4;
    std::vector<double> xList;
    std::vector<double> yList;
    xList.resize(max_iter+1+safe_border);
    yList.resize(max_iter+1+safe_border);
    for (long y=0;y<dst.height;++y){
        Colorf errorColor(0,0,0);
        for (long x=0;x<dst.width;++x) {
            double x0=(2*rect.r)*x/dst.width+rect.x0-rect.r;
            double yr=rect.r*dst.height/dst.width;
            double y0=(2*yr)*y/dst.height+rect.y0-yr;
            for (long f=0;f<=safe_border;++f){
                xList[f]=x0;
                yList[f]=y0;
            }
            long i;
            double iter=mandelbrot9(&xList[safe_border],&yList[safe_border],max_iter,i);
            dst.pixels(x,y)=coloring10B(iter,i,max_iter,&xList[safe_border],&yList[safe_border],errorColor,errorColor);
        }
    }
}
inline Color32 coloring10B1(const double iter,long i,const long max_iter,double* xList,double* yList,const Colorf& errorColorIn,Colorf& errorColorOut){
    Colorf color=errorColorIn;
    if (iter==max_iter){
        const double x=xList[max_iter];
        const double y=yList[max_iter];
        double z=sqrt(x*x+y*y);
        color.addColor(Colorf(sinColorf(z*20*50-236),sinColorf(z*15*50+221),sinColorf(z*30*50-254)));
    }else{ 
        const double x=smoothXY10B(iter,i,xList);
        const double y=smoothXY10B(iter,i,yList);
        double z=sqrt(x*x+y*y);
        color.addColor(Colorf(sinColorf(x*20*2-236),sinColorf(y*15*2+221),sinColorf((x*y/sqrt(z))*30*2-254)));
    }
    Color32 resultColor=color.toColor32();
    errorColorOut=color;
    errorColorOut.subColor(resultColor);
    return resultColor;
}

void draw_mandelbrot10B1(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
    const long safe_border=4;
    std::vector<double> xList;
    std::vector<double> yList;
    xList.resize(max_iter+1+safe_border);
    yList.resize(max_iter+1+safe_border);
    for (long y=0;y<dst.height;++y){
        Colorf errorColor(0,0,0);
        for (long x=0;x<dst.width;++x) {
            double x0=(2*rect.r)*x/dst.width+rect.x0-rect.r;
            double yr=rect.r*dst.height/dst.width;
            double y0=(2*yr)*y/dst.height+rect.y0-yr;
            for (long f=0;f<=safe_border;++f){
                xList[f]=x0;
                yList[f]=y0;
            }
            long i;
            double iter=mandelbrot9(&xList[safe_border],&yList[safe_border],max_iter,i);
            dst.pixels(x,y)=coloring10B1(iter,i,max_iter,&xList[safe_border],&yList[safe_border],errorColor,errorColor);
        }
    }
}

//---------------------------------------------------------------------------------
    const double PI=3.14159265358979;
    void rotaryMap(const TViewRect& rect,double x0,double y0,double& out_x,double& out_y){
        const double rsin=sin(rect.seta*(PI/180));
        const double rcos=cos(rect.seta*(PI/180));
        double x= (x0-rect.x0)*rcos + (y0-rect.y0)*rsin + rect.x0;
        double y=-(x0-rect.x0)*rsin + (y0-rect.y0)*rcos + rect.y0;
        out_x=x;
        out_y=y;
    }

void draw_mandelbrot11(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
    const long safe_border=4;
    std::vector<double> xList;
    std::vector<double> yList;
    xList.resize(max_iter+1+safe_border);
    yList.resize(max_iter+1+safe_border);
    for (long y=0;y<dst.height;++y){
        Colorf errorColor(0,0,0);
        for (long x=0;x<dst.width;++x) {
            double x0=(2*rect.r)*x/dst.width+rect.x0-rect.r;
            double yr=rect.r*dst.height/dst.width;
            double y0=(2*yr)*y/dst.height+rect.y0-yr;
            rotaryMap(rect,x0,y0,x0,y0);
            for (long f=0;f<=safe_border;++f){
                xList[f]=x0;
                yList[f]=y0;
            }
            long i;
            double iter=mandelbrot9(&xList[safe_border],&yList[safe_border],max_iter,i);
            dst.pixels(x,y)=coloring10B(iter,i,max_iter,&xList[safe_border],&yList[safe_border],errorColor,errorColor);
        }
    }
}

//test
void draw_mandelbrot11_1(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
    TViewRect newRect;
    newRect.x0=-1.9414077;
    newRect.y0=0;
    newRect.r=0.0025;
    newRect.seta=0;
    draw_mandelbrot11(dst,newRect,max_iter);
}
void draw_mandelbrot11_2(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
    TViewRect newRect;
    newRect.x0=-1.9414077;
    newRect.y0=0;
    newRect.r=0.0025;
    newRect.seta=45;
    draw_mandelbrot11(dst,newRect,max_iter);
}

//---------------------------------------------------------------------------------

    void tanMap(const TViewRect& rect,double x0,double y0,double& out_x,double& out_y){
        double x=tan(x0*2);
        double y=tan(y0*2);
        out_x=x;
        out_y=y;
    }

void draw_mandelbrot12t1(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
    const long safe_border=4;
    std::vector<double> xList;
    std::vector<double> yList;
    xList.resize(max_iter+1+safe_border);
    yList.resize(max_iter+1+safe_border);
    for (long y=0;y<dst.height;++y){
        Colorf errorColor(0,0,0);
        for (long x=0;x<dst.width;++x) {
            double x0=(2*rect.r)*x/dst.width+rect.x0-rect.r;
            double yr=rect.r*dst.height/dst.width;
            double y0=(2*yr)*y/dst.height+rect.y0-yr;
            rotaryMap(rect,x0,y0,x0,y0);
            tanMap(rect,x0,y0,x0,y0);
            for (long f=0;f<=safe_border;++f){
                xList[f]=x0;
                yList[f]=y0;
            }
            long i;
            double iter=mandelbrot9(&xList[safe_border],&yList[safe_border],max_iter,i);
            dst.pixels(x,y)=coloring10B(iter,i,max_iter,&xList[safe_border],&yList[safe_border],errorColor,errorColor);
        }
    }
}


    void divMap2(const TViewRect& rect,double x0,double y0,double& out_x,double& out_y){
        //C=1/C;
        x0=(x0+1.4)*1.6;
        y0=y0*1.6;
        const double z=(x0*x0+y0*y0+1e-100);
        double x=x0/z;
        double y=-y0/z;
        out_x=x;
        out_y=y;
    }

void draw_mandelbrot12t2(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
    const long safe_border=4;
    std::vector<double> xList;
    std::vector<double> yList;
    xList.resize(max_iter+1+safe_border);
    yList.resize(max_iter+1+safe_border);
    for (long y=0;y<dst.height;++y){
        Colorf errorColor(0,0,0);
        for (long x=0;x<dst.width;++x) {
            double x0=(2*rect.r)*x/dst.width+rect.x0-rect.r;
            double yr=rect.r*dst.height/dst.width;
            double y0=(2*yr)*y/dst.height+rect.y0-yr;
            rotaryMap(rect,x0,y0,x0,y0);
            divMap2(rect,x0,y0,x0,y0);
            for (long f=0;f<=safe_border;++f){
                xList[f]=x0;
                yList[f]=y0;
            }
            long i;
            double iter=mandelbrot9(&xList[safe_border],&yList[safe_border],max_iter,i);
            dst.pixels(x,y)=coloring10B(iter,i,max_iter,&xList[safe_border],&yList[safe_border],errorColor,errorColor);
        }
    }
}


    inline double julia12(double* xList,double* yList,const long max_iter,double jx0,double jy0,long& out_i){  
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
        out_i=i;
        if (i!=max_iter){  
            const double lnln_Z=loglog(x*x+y*y);  
            const double lnln_Zbak=loglog(x_bck*x_bck+y_bck*y_bck);  
            return i-2-(lnln_Z-lnln_M)/(lnln_Z-lnln_Zbak);  
        }else  
            return i;  
    }  

    void juliaMap(const TViewRect& rect,double x0,double y0,const long max_iter,double jx0,double jy0,double& out_x,double& out_y){
        const long safe_border=4;
        std::vector<double> xList;
        std::vector<double> yList;
        xList.resize(max_iter+1+safe_border);
        yList.resize(max_iter+1+safe_border);
        for (long f=0;f<=safe_border;++f){
            xList[f]=x0;
            yList[f]=y0;
        }
        long i;
        const double iter=julia12(&xList[safe_border],&yList[safe_border],max_iter,jx0,jy0,i);
        const double x=smoothXY10B(iter,i,&xList[safe_border]);
        const double y=smoothXY10B(iter,i,&yList[safe_border]);
        out_x=x;
        out_y=y;
    }

void draw_mandelbrot12j(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter,const long jmax_iter,double jx0,double jy0){
    const long safe_border=4;
    std::vector<double> xList;
    std::vector<double> yList;
    xList.resize(max_iter+1+safe_border);
    yList.resize(max_iter+1+safe_border);
    for (long y=0;y<dst.height;++y){
        Colorf errorColor(0,0,0);
        for (long x=0;x<dst.width;++x) {
            double x0=(2*rect.r)*x/dst.width+rect.x0-rect.r;
            double yr=rect.r*dst.height/dst.width;
            double y0=(2*yr)*y/dst.height+rect.y0-yr;
            rotaryMap(rect,x0,y0,x0,y0);
            juliaMap(rect,x0,y0,jmax_iter,jx0,jy0,x0,y0);
            for (long f=0;f<=safe_border;++f){
                xList[f]=x0;
                yList[f]=y0;
            }
            long i;
            double iter=mandelbrot9(&xList[safe_border],&yList[safe_border],max_iter,i);
            dst.pixels(x,y)=coloring10B(iter,i,max_iter,&xList[safe_border],&yList[safe_border],errorColor,errorColor);
        }
    }
}

//test
void draw_mandelbrot12j1(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
    TViewRect newRect;
    newRect.x0=0;
    newRect.y0=0;
    newRect.r=1.5;
    newRect.seta=120;
    draw_mandelbrot12j(dst,newRect,max_iter,3,0.28888,0.012325);
}
void draw_mandelbrot12j2(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
    TViewRect newRect;
    newRect.x0=0;
    newRect.y0=0;
    newRect.r=1.7;
    newRect.seta=0;
    draw_mandelbrot12j(dst,newRect,max_iter,16,-0.81442,0.19633);
}

//---------------------------------------------------------------------------------

typedef void (*TDrawMandelbrotProc)(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter);

void test(const char* proc_name,const TDrawMandelbrotProc fproc){
    const long max_iter=1000;
    TViewRect rect;
    rect.x0=-0.5;
    rect.y0=0;
    rect.r=2;
    rect.seta=0;

    TPixels32 dstPic;
    dstPic.resizeFast(640,480);

    std::cout<<proc_name<<": ";
    fproc(dstPic.getRef(),rect,max_iter);
    std::cout<<" ok"<<std::endl;

    { //save pic
        std::string dstFileName(proc_name);
        dstFileName+=".bmp";
        TFileOutputStream bmpOutStream(dstFileName.c_str());
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

    test("draw_mandelbrot9_s" ,draw_mandelbrot9_s);
    test("draw_mandelbrot9"   ,draw_mandelbrot9);
    test("draw_mandelbrot10"  ,draw_mandelbrot10);
    test("draw_mandelbrot10B" ,draw_mandelbrot10B);
    test("draw_mandelbrot10B1",draw_mandelbrot10B1);
    test("draw_mandelbrot11_1",draw_mandelbrot11_1);
    test("draw_mandelbrot11_2",draw_mandelbrot11_2);
    test("draw_mandelbrot12t1",draw_mandelbrot12t1);
    test("draw_mandelbrot12t2",draw_mandelbrot12t2);
    test("draw_mandelbrot12j1",draw_mandelbrot12j1);
    test("draw_mandelbrot12j2",draw_mandelbrot12j2);
    

    std::cout<<std::endl<<" 运行完成. ";
    //waitInputChar();
    return 0;
}
