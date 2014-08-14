#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <time.h>
//(  警告:代码移植到其他体系的CPU时需要重新考虑字节顺序的大端小端问题! )

#include "../hGraphic32/hGraphic32.h" //简易图像处理基础框架


struct TViewRect{
    double x0,y0,r;
};

//---------------------------------------------------------------------------------
inline long mandelbrot1(const double x0,const double y0,const long max_iter){
    double x=x0;
    double y=y0;
    long i=0;
    for (;i<max_iter;++i){
        if (x*x+y*y>=4)
            break;
        double tmp=x*x-y*y+x0;
        y=x*y*2+y0;
        x=tmp;
    }
    return i;
}


//color=i%256
//    /  /  /  /  /
//   /  /  /  /  /
//  /  /  /  /  / 
inline Color32 coloring1(const long iter,const long max_iter){
    if (iter==max_iter){
        return Color32(255,0,0);
    }else{
        return Color32((iter*20)%256,(iter*15+85)%256,(iter*30+171)%256);
    }
}

void draw_mandelbrot1(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
    for (long y=0;y<dst.height;++y){
        for (long x=0;x<dst.width;++x) {
            double x0=(2*rect.r)*x/dst.width+rect.x0-rect.r;
            double yr=rect.r*dst.height/dst.width; //y轴半径
            double y0=(2*yr)*y/dst.height+rect.y0-yr;
            long iter=mandelbrot1(x0,y0,max_iter);
            dst.pixels(x,y)=coloring1(iter,max_iter);
        }
    }
}

//---------------------------------------------------------------------------------

//color=(i%510)-255
//    /\    /\    /
//   /  \  /  \  /
//  /    \/    \/ 
inline long modColor2(long iter){
    return abs((iter+255)%510-255);
}
inline Color32 coloring2(const long iter,const long max_iter){
    if (iter==max_iter){
        return Color32(255,0,0);
    }else{
        return Color32(modColor2(iter*20),modColor2(iter*15+85),modColor2(iter*30+171));
    }
}

void draw_mandelbrot2(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
    for (long y=0;y<dst.height;++y){
        for (long x=0;x<dst.width;++x) {
            double x0=(2*rect.r)*x/dst.width+rect.x0-rect.r;
            double yr=rect.r*dst.height/dst.width;
            double y0=(2*yr)*y/dst.height+rect.y0-yr;
            long iter=mandelbrot1(x0,y0,max_iter);
            dst.pixels(x,y)=coloring2(iter,max_iter);
        }
    }
}
inline Color32 coloring3_s(const long iter,const long max_iter){
    if (iter==max_iter){
        return Color32(255,0,0);
    }else{
        return Color32(modColor2(iter),modColor2(iter+85),modColor2(iter+171));
    }
}

void draw_mandelbrot3_s(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
    for (long y=0;y<dst.height;++y){
        for (long x=0;x<dst.width;++x) {
            double x0=(2*rect.r)*x/dst.width+rect.x0-rect.r;
            double yr=rect.r*dst.height/dst.width;
            double y0=(2*yr)*y/dst.height+rect.y0-yr;
            long iter=mandelbrot1(x0,y0,max_iter);
            dst.pixels(x,y)=coloring3_s(iter,max_iter);
        }
    }
}

//---------------------------------------------------------------------------------
   
    static const double _divLog2=1.0/log(2.0);
    inline double log2(double x){
        return log(x)*_divLog2;
    }  

inline double mandelbrot3(const double x0,const double y0,const long max_iter){
    double x=x0;
    double y=y0;
    long i=0;
    for (;i<max_iter;++i){
        if (x*x+y*y>=256)
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

//color=(i%510)-255
inline long modColor3(double iter){
    return (long)(abs(fmod(iter+255,510)-255));
}
inline Color32 coloring3(const double iter,const long max_iter){
    if (iter==max_iter){
        return Color32(255,0,0);
    }else{ 
        return Color32(modColor3(iter*20),modColor3(iter*15+85),modColor3(iter*30+171));
    }
}

void draw_mandelbrot3(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
    for (long y=0;y<dst.height;++y){
        for (long x=0;x<dst.width;++x) {
            double x0=(2*rect.r)*x/dst.width+rect.x0-rect.r;
            double yr=rect.r*dst.height/dst.width;
            double y0=(2*yr)*y/dst.height+rect.y0-yr;
            double iter=mandelbrot3(x0,y0,max_iter);
            dst.pixels(x,y)=coloring3(iter,max_iter);
        }
    }
}

//---------------------------------------------------------------------------------

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

void draw_mandelbrot4(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter){
    for (long y=0;y<dst.height;++y){
        for (long x=0;x<dst.width;++x) {
            double x0=(2*rect.r)*x/dst.width+rect.x0-rect.r;
            double yr=rect.r*dst.height/dst.width;
            double y0=(2*yr)*y/dst.height+rect.y0-yr;
            double iter=mandelbrot3(x0,y0,max_iter);
            dst.pixels(x,y)=coloring4(iter,max_iter);
        }
    }
}

//---------------------------------------------------------------------------------

typedef void (*TDrawMandelbrotProc)(const TPixels32Ref& dst,const TViewRect& rect,const long max_iter);

void test(const char* proc_name,const TDrawMandelbrotProc fproc){
    const long max_iter=1000;
    TViewRect rect;
    rect.x0=-0.5;
    rect.y0=0;
    rect.r=2;

    TPixels32 dstPic;
    dstPic.resizeFast(640,480);

    std::cout<<proc_name<<": ";
    fproc(dstPic.getRef(),rect,max_iter);
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

    test("draw_mandelbrot1"   ,draw_mandelbrot1);
    test("draw_mandelbrot2"   ,draw_mandelbrot2);
    test("draw_mandelbrot2_s" ,draw_mandelbrot3_s);
    test("draw_mandelbrot3"   ,draw_mandelbrot3);
    test("draw_mandelbrot4"   ,draw_mandelbrot4);

    std::cout<<std::endl<<" 运行完成. ";
    //waitInputChar();
    return 0;
}
