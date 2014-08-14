//ZoomTest.cpp
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <time.h>
//(  警告:代码移植到其他体系的CPU时需要重新考虑字节顺序的大端小端问题! )

#include "../hGraphic32/hGraphic32.h" //简易图像处理基础框架

#ifdef _MSC_VER     
    #define must_inline __forceinline 
#else
    #ifdef __GNUC__
        #define must_inline __attribute__((always_inline)) 
    #else
        #define must_inline inline 
    #endif
#endif 


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
    sprintf(buff, "%.3f", fps);
    std::string result(buff);
    const long fpsStrBestSize=8;
    if (result.size()<fpsStrBestSize)
        result=std::string(fpsStrBestSize-result.size(),' ')+result;
    return result;
}

typedef void (*T_threshold_proc)(const TPixels32Ref& dst,const TPixels32Ref& src);
typedef void (*T_threshold_localWidth_proc)(const TPixels32Ref& dst,const TPixels32Ref& src,long localWidth);

void test(const char* proc_name,const T_threshold_localWidth_proc threshold_proc,const long csRunCount,long localWidth=0){
    TPixels32 srcPic;
    TPixels32 dstPic;
#if defined(__APPLE__) && defined(__MACH__)
    TFileInputStream bmpInputStream("/Users/hss/Desktop/GraphicDemo/ColorToGray/test0.bmp"); //我的xcode测试目录
#else
    TFileInputStream bmpInputStream("test0.bmp");
#endif
    TBmpFile::load(&bmpInputStream,&srcPic);//加载源图片
    dstPic.resizeFast(srcPic.getWidth(),srcPic.getHeight());

    std::cout<<proc_name<<": ";
    clock_t t0=clock();
    for (long c=0;c<csRunCount;++c){
        if (localWidth>0)
            threshold_proc(dstPic.getRef(),srcPic.getRef(),localWidth);
        else
            ((T_threshold_proc)threshold_proc)(dstPic.getRef(),srcPic.getRef());        
    }
    t0=clock()-t0;
    double fps=csRunCount/(t0*1.0/CLOCKS_PER_SEC);
    std::cout<<fpsToStr(fps)<<" 帧/秒"<<"  localWidth: "<<localWidth<<std::endl;

    if (true){ 
#if defined(__APPLE__) && defined(__MACH__)
        TFileOutputStream bmpOutStream("/Users/hss/Desktop/GraphicDemo/ColorToGray/testResult.bmp");
#else
        TFileOutputStream bmpOutStream("testResult.bmp");
#endif
        TBmpFile::save(dstPic.getRef(),&bmpOutStream);//保存结果图片
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////

    
    const double cs_gray_red  =0.299;
    const double cs_gray_green=0.587;
    const double cs_gray_blue =0.114;

    must_inline long getGray0(const Color32& color)
    {   
        return (long)(color.r*cs_gray_red + color.g*cs_gray_green + color.b*cs_gray_blue);
    }
 
void threshold0(const TPixels32Ref& dst,const TPixels32Ref& src)
{
    long width=dst.width;
    if (src.width<width) width=src.width;
    long height=dst.height;
    if  (src.height<height) height=src.height;
    Color32* srcLine=src.pdata;
    Color32* dstLine=dst.pdata;
    for (long y=0;y<height;++y)
    {
        for (long x=0;x<width;++x)
        {
            long light=getGray0(srcLine[x]);
            if (light>127)
                dstLine[x].argb=0xFFFFFFFF;//白色
            else
                dstLine[x].argb=0xFF000000;//黑色
        }
        (UInt8*&)srcLine+=src.byte_width;
        (UInt8*&)dstLine+=dst.byte_width;
    }
}



    const long cs_gray_red_16  =(long)(cs_gray_red*(1<<16));
    const long cs_gray_green_16=(long)(cs_gray_green*(1<<16));
    const long cs_gray_blue_16 =(long)(cs_gray_blue*(1<<16));

    must_inline long getGrayInt(const Color32& color)
    {   
        return (color.r*cs_gray_red_16 + color.g*cs_gray_green_16 + color.b*cs_gray_blue_16)>>16;
    }

 
void threshold1(const TPixels32Ref& dst,const TPixels32Ref& src)
{
    long width=dst.width;
    if (src.width<width) width=src.width;
    long height=dst.height;
    if (src.height<height) height=src.height;
    Color32* srcLine=src.pdata;
    Color32* dstLine=dst.pdata;
    for (long y=0;y<height;++y)
    {
        for (long x=0;x<width;++x)
        {
            long light=getGrayInt(srcLine[x]);
            UInt32 color=((127-light)>>31);//利用了整数的编码方式来消除了分支
            ((UInt32*)dstLine)[x]=(color|0xFF000000);  //一次写4个字节
        }
        (UInt8*&)srcLine+=src.byte_width;
        (UInt8*&)dstLine+=dst.byte_width;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    must_inline const Color32& getMapBorderColor(const TPixels32Ref& src,long x,long y)
    {
        if (x<0) x=-x-1;
        long width2=src.width*2;
        while (x>=width2) x-=width2;
        if (x>=src.width) x=width2-x-1;
        if (y<0) y=-y-1;
        long height2=src.height*2;
        while (y>=height2) y-=height2;
        if (y>=src.height) y=height2-y-1;
        return src.pixels(x,y);
    }

    long getLocalLight_quadratic(const TPixels32Ref& src,long x0,long y0,long localHalfWidth)
    {
        long sumLight=0;
        for (long y=y0-localHalfWidth;y<=y0+localHalfWidth;++y)
        {
            for (long x=x0-localHalfWidth;x<=x0+localHalfWidth;++x)
            {
                const Color32& mapBorderColor=getMapBorderColor(src,x,y);
                sumLight+=getGrayInt(mapBorderColor);
            }
        }
        return sumLight;
    }

   
void localAdaptiveThreshold_quadratic(const TPixels32Ref& dst,const TPixels32Ref& src,long localWidth)
{
    long width=dst.width;
    if (src.width<width) width=src.width;
    long height=dst.height;
    if (src.height<height) height=src.height;
    Color32* srcLine=src.pdata;
    Color32* dstLine=dst.pdata;
    long localHalfWidth=localWidth/2;
    long tLocalWidth=localHalfWidth*2+1;
    long tLocalWidthSqr=tLocalWidth*tLocalWidth;
    for (long y=0;y<height;++y)
    {
        for (long x=0;x<width;++x)
        {
            long sumLight=getLocalLight_quadratic(src,x,y,localHalfWidth);
            long light=getGrayInt(srcLine[x]);
            UInt32 color=((sumLight-light*tLocalWidthSqr)>>31);  //localWidth^2*256<=2^31 =>  localWidth<=2896
            ((UInt32*)dstLine)[x]=(color|0xFF000000); 
        } 
        (UInt8*&)srcLine+=src.byte_width;
        (UInt8*&)dstLine+=dst.byte_width;
    }
}


    long getLocalLight_linearV(const TPixels32Ref& src,long x,long y0,long localHalfWidth)
    {
        long sumYLight=0;
        for (long y=y0-localHalfWidth;y<=y0+localHalfWidth;++y)
        {
            const Color32& mapBorderColor=getMapBorderColor(src,x,y);
            sumYLight+=getGrayInt(mapBorderColor);
        }
        return sumYLight;
    }
    long getLocalLight_linearH(const TPixels32Ref& src,long x0,long y,long localHalfWidth)
    {
        long sumXLight=0;
        for (long x=x0-localHalfWidth;x<=x0+localHalfWidth;++x)
        {
            const Color32& mapBorderColor=getMapBorderColor(src,x,y);
            sumXLight+=getGrayInt(mapBorderColor);
        }
        return sumXLight;
    }
void localAdaptiveThreshold_linear(const TPixels32Ref& dst,const TPixels32Ref& src,long localWidth)
{
    long width=dst.width;
    if (src.width<width) width=src.width;
    long height=dst.height;
    if (src.height<height) height=src.height;
    Color32* srcLine=src.pdata;
    Color32* dstLine=dst.pdata;
    long localHalfWidth=localWidth/2;
    long tLocalWidth=localHalfWidth*2+1;
    long tLocalWidthSqr=tLocalWidth*tLocalWidth;
    long sumLight0=getLocalLight_quadratic(src,-1,-1,localHalfWidth);
    for (long y=0;y<height;++y)
    {
        sumLight0=sumLight0 
                +getLocalLight_linearH(src,-1,y+localHalfWidth,localHalfWidth)
                -getLocalLight_linearH(src,-1,y-localHalfWidth-1,localHalfWidth);
        long sumLight=sumLight0;
        for (long x=0;x<width;++x)
        {
            sumLight=sumLight
                    +getLocalLight_linearV(src,x+localHalfWidth,y,localHalfWidth)
                    -getLocalLight_linearV(src,x-localHalfWidth-1,y,localHalfWidth);
            long light=getGrayInt(srcLine[x]);
            UInt32 color=((sumLight-light*tLocalWidthSqr)>>31);
            ((UInt32*)dstLine)[x]=(color|0xFF000000); 
        }
        (UInt8*&)srcLine+=src.byte_width;
        (UInt8*&)dstLine+=dst.byte_width;
    }
}


    must_inline long getLocalLight_constant(const TPixels32Ref& src,long x,long y)
    {
        return getGrayInt(getMapBorderColor(src,x,y));
    }
void localAdaptiveThreshold_constant(const TPixels32Ref& dst,const TPixels32Ref& src,long localWidth)
{
    long width=dst.width;
    if (src.width<width) width=src.width;
    if (width<=0) return;
    long height=dst.height;
    if (src.height<height) height=src.height;
    Color32* srcLine=src.pdata;
    Color32* dstLine=dst.pdata;
    long localHalfWidth=localWidth/2;
    long tLocalWidth=localHalfWidth*2+1;
    long tLocalWidthSqr=tLocalWidth*tLocalWidth;

    long* _sumLightArray=new long[width+1];
    long* sumLightArray=&_sumLightArray[1];
    sumLightArray[-1]=getLocalLight_quadratic(src,-1,-1,localHalfWidth);
    for (long x=0;x<width;++x)
    {
        sumLightArray[x] = sumLightArray[x-1]
                +getLocalLight_linearV(src,x+localHalfWidth,-1,localHalfWidth)
                -getLocalLight_linearV(src,x-localHalfWidth-1,-1,localHalfWidth);
    }

    for (long y=0;y<height;++y)
    {
        long sumLight0=sumLightArray[-1]
                +getLocalLight_linearH(src,-1,y+localHalfWidth,localHalfWidth)
                -getLocalLight_linearH(src,-1,y-localHalfWidth-1,localHalfWidth);
        for (long x=0;x<width;++x)
        {
            long sumLight=sumLight0+sumLightArray[x]-sumLightArray[x-1]
                     +getLocalLight_constant(src,x-localHalfWidth-1,y-localHalfWidth-1)
                     +getLocalLight_constant(src,x+localHalfWidth,y+localHalfWidth)
                     -getLocalLight_constant(src,x+localHalfWidth,y-localHalfWidth-1)
                     -getLocalLight_constant(src,x-localHalfWidth-1,y+localHalfWidth);

            sumLightArray[x-1]=sumLight0;
            sumLight0=sumLight;

            long light=getGrayInt(srcLine[x]);
            UInt32 color=((sumLight-light*tLocalWidthSqr)>>31);
            ((UInt32*)dstLine)[x]=(color|0xFF000000); 
        }
        sumLightArray[width-1]=sumLight0;
        (UInt8*&)srcLine+=src.byte_width;
        (UInt8*&)dstLine+=dst.byte_width;
    }

    delete []_sumLightArray;
}


///////////


        must_inline const Color32& getMapBorderColor_fastY(const TPixels32Ref& src,long x,long y)
        {
            if (x<0) x=-x-1;
            long width2=src.width*2;
            while (x>=width2) x-=width2;
            if (x>=src.width) x=width2-x-1;
            return src.pixels(x,y);
        }
    must_inline long getLocalLight_constant_fastY(const TPixels32Ref& src,long x,long y)
    {
        return getGrayInt(getMapBorderColor_fastY(src,x,y));
    }
    must_inline long getLocalLight_constant_fast(const Color32* src,long x)
    {
        return getGrayInt(src[x]);
    }
    long getLocalLight_linearH_fastY(const TPixels32Ref& src,long x0,long y,long localHalfWidth)
    {
        long sumXLight=0;
        for (long x=x0-localHalfWidth;x<=x0+localHalfWidth;++x)
        {
            const Color32& mapBorderColor=getMapBorderColor_fastY(src,x,y);
            sumXLight+=getGrayInt(mapBorderColor);
        }
        return sumXLight;
    }

void localAdaptiveThreshold(const TPixels32Ref& dst,const TPixels32Ref& src,long localWidth)
{
    long width=dst.width;
    if (src.width<width) width=src.width;
    if (width<=0) return;
    long height=dst.height;
    if (src.height<height) height=src.height;
    if (height<=0) return;
    long localHalfWidth=localWidth/2;
    long tLocalWidth=localHalfWidth*2+1;
    long tLocalWidthSqr=tLocalWidth*tLocalWidth;

    long* _sumLightRegion=new long[width*(height+localHalfWidth*2+1)];
    ////
    {
        //0<=fastX-localHalfWidth-1; fastX+localHalfWidth<width;
        long fastXBegin=localHalfWidth+1;
        long fastXEnd=width-localHalfWidth;
        if (fastXEnd<=fastXBegin)
        {
            fastXBegin=width;
            fastXEnd=width;
        }

        long* sumLightRegion=&_sumLightRegion[width*(localHalfWidth+1)];
        Color32* srcLine=src.pdata;
        for (long y=0;y<height;++y)
        {
            long x;
            long sumLight0=getLocalLight_linearH_fastY(src,-1,y,localHalfWidth);
            for (x=0;x<fastXBegin;++x) //border
            {
                sumLight0=sumLight0
                        +getLocalLight_constant_fastY(src,x+localHalfWidth,y)
                        -getLocalLight_constant_fastY(src,x-localHalfWidth-1,y);
                sumLightRegion[x] = sumLight0;
            }
            for (x=fastXBegin;x<fastXEnd;++x) //fast
            {
                sumLight0=sumLight0
                        +getLocalLight_constant_fast(srcLine,x+localHalfWidth)
                        -getLocalLight_constant_fast(srcLine,x-localHalfWidth-1);
                sumLightRegion[x] = sumLight0;
            }
            for (x=fastXEnd;x<width;++x) //border
            {
                sumLight0=sumLight0
                        +getLocalLight_constant_fastY(src,x+localHalfWidth,y)
                        -getLocalLight_constant_fastY(src,x-localHalfWidth-1,y);
                sumLightRegion[x] = sumLight0;
            }
            (UInt8*&)srcLine+=src.byte_width;
            sumLightRegion=&sumLightRegion[width];//next line
        }

        //map copy
        {
            
            long* sumLightRegionMap=&_sumLightRegion[width*(localHalfWidth+1-1)];
            for (long yi=0;yi<localHalfWidth+1;++yi)
            {
                long y=yi;
                long height2=height*2;
                while (y>=height2) y-=height2;
                if (y>=height) y=height2-y-1;
                long* sumLightRegion=&_sumLightRegion[width*(localHalfWidth+1+y)];

                for (long x=0;x<width;++x)
                    sumLightRegionMap[x]=sumLightRegion[x];
                sumLightRegionMap=&sumLightRegionMap[-width];
            }

            sumLightRegionMap=&_sumLightRegion[width*(localHalfWidth+1+height)];
            for (long yi=height-1;yi>=height-localHalfWidth;--yi)
            {
                long y=yi;
                if (y<0) y=-y-1;
                long height2=height*2;
                while (y>=height2) y-=height2;
                if (y>=height) y=height2-y-1;
                long* sumLightRegion=&_sumLightRegion[width*(localHalfWidth+1+y)];

                for (long x=0;x<width;++x)
                    sumLightRegionMap[x]=sumLightRegion[x];
                sumLightRegionMap=&sumLightRegionMap[width];
            }
        }
    }

    ////
    {
        long src_byte_width=src.byte_width;
        long dst_byte_width=dst.byte_width;
        for (long x=0;x<width;++x)
        {
            long* sumLightRegion=&_sumLightRegion[x];
            long sumLight0=0;
            for (long y=-localHalfWidth-1;y<=localHalfWidth-1;++y)
            {
                sumLight0+=sumLightRegion[0];
                sumLightRegion=&sumLightRegion[width];
            }

            Color32* dstLine=&dst.pdata[x];
            Color32* srcLine=&src.pdata[x];
            sumLightRegion=&_sumLightRegion[width*(localHalfWidth+1)+x];
            for (long y=0;y<height;++y)
            {
                sumLight0=sumLight0
                         +sumLightRegion[width*localHalfWidth]
                         -sumLightRegion[-width*(localHalfWidth+1)];

                long light=getGrayInt(srcLine[0]);
                UInt32 color=((sumLight0-light*tLocalWidthSqr)>>31);
                ((UInt32*)dstLine)[0]=color; 
                (UInt8*&)srcLine+=src_byte_width;
                (UInt8*&)dstLine+=dst_byte_width;
                sumLightRegion=&sumLightRegion[width];
            }
        }
    }
    delete []_sumLightRegion;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////

int main()
{
    std::cout<<" 请输入回车键开始测试(可以把进程优先级设置为“实时”)> ";
    waitInputChar();
    std::cout<<std::endl;

    //*                                                                                 //AMD64X2 4200+ 2.33G   Q8200 2.50G
    test("threshold0     ",(T_threshold_localWidth_proc)threshold0,300);                //153.61 fps            256.41 fps
    test("threshold1     ",(T_threshold_localWidth_proc)threshold1,800);                //396.83 fps            666.11 fps  
    std::cout<<std::endl;                                                                                       
    test("localAdaptiveThreshold_quadratic  ",localAdaptiveThreshold_quadratic,20,5);   //  5.82 fps             11.34 fps
    test("localAdaptiveThreshold_quadratic  ",localAdaptiveThreshold_quadratic,2,17);   //  0.59 fps              1.04 fps
    test("localAdaptiveThreshold_quadratic  ",localAdaptiveThreshold_quadratic,1,51);   //  0.07 fps              0.12 fps
    test("localAdaptiveThreshold_quadratic  ",localAdaptiveThreshold_quadratic,1,151);  //  0.009fps              0.013fps
                                                                                                                
    test("localAdaptiveThreshold_linear     ",localAdaptiveThreshold_linear,40,5);      // 14.46 fps             25.89 fps
    test("localAdaptiveThreshold_linear     ",localAdaptiveThreshold_linear,15,17);     //  5.30 fps              8.51 fps
    test("localAdaptiveThreshold_linear     ",localAdaptiveThreshold_linear,5,51);      //  1.69 fps              2.91 fps
    test("localAdaptiveThreshold_linear     ",localAdaptiveThreshold_linear,2,151);     //  0.59 fps              0.93 fps
                                                                                                                
    test("localAdaptiveThreshold_constant   ",localAdaptiveThreshold_constant,100,5);   // 42.39 fps             63.45 fps
    test("localAdaptiveThreshold_constant   ",localAdaptiveThreshold_constant,100,17);  // 41.82 fps             62.85 fps
    test("localAdaptiveThreshold_constant   ",localAdaptiveThreshold_constant,100,51);  // 39.75 fps             60.50 fps
    test("localAdaptiveThreshold_constant   ",localAdaptiveThreshold_constant,100,151); // 34.78 fps             54.32 fps
                                                                                                                
    test("localAdaptiveThreshold",localAdaptiveThreshold,100,5);                        // 41.56 fps             83.26 fps
    test("localAdaptiveThreshold",localAdaptiveThreshold,100,17);                       // 41.03 fps             83.26 fps
    test("localAdaptiveThreshold",localAdaptiveThreshold,100,51);                       // 37.88 fps             80.13 fps
    test("localAdaptiveThreshold",localAdaptiveThreshold,100,151);                      // 33.33 fps             67.48 fps
    //*/

    std::cout<<std::endl<<" 测试完成. ";
    waitInputChar();
    return 0;
}
