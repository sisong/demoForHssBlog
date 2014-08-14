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
    #ifndef _WIN64
        //启用内联x86汇编  x64时vc编译器不支持内联汇编 并且没有MMX寄存器 
        #define asm __asm
        //使用MMX  
        #define MMX_ACTIVE
    #endif
    typedef    __int64             Int64;
    typedef    unsigned __int64    UInt64;
#else
    #ifdef __GNUC__
        #define must_inline __attribute__((always_inline)) 
    #else
        #define must_inline inline 
    #endif
    typedef    long long            Int64;
    typedef    unsigned long long   UInt64;
#endif 


const double PI=3.1415926535897932384626433832795;


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

typedef void (*TRotaryProc)(const TPixels32Ref& Dst,const TPixels32Ref& Src,double RotaryAngle,double ZoomX,double ZoomY,double move_x,double move_y);

void test(const char* proc_name,const TRotaryProc fproc,const long csRunCount,bool isSaveResultPic=false){
    TPixels32 srcPic;
    TPixels32 dstPic;
#if defined(__APPLE__) && defined(__MACH__)
    TFileInputStream bmpInputStream("/Users/hss/Desktop/GraphicDemo/ColorToGray/test1.bmp"); //我的xcode测试目录
#else
    TFileInputStream bmpInputStream("test1.bmp");
#endif
    TBmpFile::load(&bmpInputStream,&srcPic);//加载源图片
        long dst_wh=(long)( ::sqrt(1.0*srcPic.getWidth()*srcPic.getWidth()+srcPic.getHeight()*srcPic.getHeight()) +4 +0.5);
    dstPic.resizeFast(dst_wh,dst_wh);
    dstPic.getRef().fillColor(Color32(0,0,255));
    const long rCount=360; //分成rCount个角度测试速度

    std::cout<<proc_name<<": ";
    clock_t t0=clock();
    for (long c=0;c<csRunCount;++c){
        for (long r=0;r<rCount;++r){
            double rotaryAngle=(PI*2)*(r*1.0/rCount+30.0/360);
            fproc(dstPic.getRef(),srcPic.getRef(),rotaryAngle,1,1,(dstPic.getWidth()-srcPic.getWidth())*0.5,(dstPic.getHeight()-srcPic.getHeight())*0.5);
        }
    }
    t0=clock()-t0;
    double fps=csRunCount*rCount/(t0*1.0/CLOCKS_PER_SEC);
    std::cout<<fpsToStr(fps)<<" 帧/秒"<<std::endl;

    if (isSaveResultPic){ 
#if defined(__APPLE__) && defined(__MACH__)
        TFileOutputStream bmpOutStream("/Users/hss/Desktop/GraphicDemo/ColorToGray/testResult.bmp");
#else
        TFileOutputStream bmpOutStream("testResult.bmp");
#endif
        TBmpFile::save(dstPic.getRef(),&bmpOutStream);//保存结果图片
    }
}


void testEffect(const TRotaryProc fproc,bool isUseAlpha=false){
    const char* srcFileName=0;
#if defined(__APPLE__) && defined(__MACH__)
    if (isUseAlpha)
        srcFileName="/Users/hss/Desktop/GraphicDemo/ColorToGray/zaka_ARGB32bit.bmp"; //我的xcode测试目录
    else
        srcFileName="/Users/hss/Desktop/GraphicDemo/ColorToGray/zaka.bmp"; 
#else
    if (isUseAlpha)
        srcFileName="zaka_ARGB32bit.bmp";
    else
        srcFileName="zaka.bmp";
#endif
    TPixels32 srcPic;
    TPixels32 dstPic;
    TFileInputStream bmpInputStream(srcFileName);
    TBmpFile::load(&bmpInputStream,&srcPic);//加载源图片
    long dst_wh=1004;
    dstPic.resizeFast(dst_wh,dst_wh);
    dstPic.getRef().fillColor(Color32(0,255,0));

    clock_t t0=clock();
    const long testCount=2000;
    for (int i=0;i<testCount;++i)
    {
        double zoom=rand()*(1.0/RAND_MAX)+0.5;
        fproc(dstPic.getRef(),srcPic.getRef(),rand()*(PI*2/RAND_MAX),zoom,zoom,((dst_wh+srcPic.getWidth())*rand()*(1.0/RAND_MAX)-srcPic.getWidth()),(dst_wh+srcPic.getHeight())*rand()*(1.0/RAND_MAX)-srcPic.getHeight());
    }

    t0=clock()-t0;
    double fps=1/(t0*1.0/CLOCKS_PER_SEC);
    std::cout<<fpsToStr(fps)<<" 帧/秒"<<std::endl;

    if (true){ 
#if defined(__APPLE__) && defined(__MACH__)
        TFileOutputStream bmpOutStream("/Users/hss/Desktop/GraphicDemo/ColorToGray/testEffectResult.bmp");
#else
        TFileOutputStream bmpOutStream("testEffectResult.bmp");
#endif
        TBmpFile::save(dstPic.getRef(),&bmpOutStream);//保存结果图片
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//RotaryAngle为逆时针旋转的角度;
//ZoomX,ZoomY为x轴y轴的缩放系数(支持负的系数,相当于图像翻转);
//move_x,move_y为x轴y轴的平移量;
void PicRotary0(const TPixels32Ref& Dst,const TPixels32Ref& Src,double RotaryAngle,double ZoomX,double ZoomY,double move_x,double move_y)
{
    if ( (fabs(ZoomX*Src.width)<1.0e-4) || (fabs(ZoomY*Src.height)<1.0e-4) ) return; //太小的缩放比例认为已经不可见
    double rx0=Src.width*0.5;  //(rx0,ry0)为旋转中心 
    double ry0=Src.height*0.5; 
    for (long y=0;y<Dst.height;++y)
    {
        for (long x=0;x<Dst.width;++x)
        {
            long srcx=(long)((x- (move_x+rx0))/ZoomX*cos(RotaryAngle) - (y- (move_y+ry0))/ZoomY*sin(RotaryAngle) + rx0) ;
            long srcy=(long)((x- (move_x+rx0))/ZoomX*sin(RotaryAngle) + (y- (move_y+ry0))/ZoomY*cos(RotaryAngle) + ry0) ;
            if (Src.getIsInPic(srcx,srcy))
                Dst.pixels(x,y)=Src.pixels(srcx,srcy);
        }
    }
}

void PicRotary1(const TPixels32Ref& Dst,const TPixels32Ref& Src,double RotaryAngle,double ZoomX,double ZoomY,double move_x,double move_y)
{
    if ( (fabs(ZoomX*Src.width)<1.0e-4) || (fabs(ZoomY*Src.height)<1.0e-4) ) return; //太小的缩放比例认为已经不可见
    double rZoomX=1.0/ZoomX;
    double rZoomY=1.0/ZoomY;
    double sinA=sin(RotaryAngle);
    double cosA=cos(RotaryAngle);
    double Ax=(rZoomX*cosA); 
    double Ay=(rZoomX*sinA); 
    double Bx=(-rZoomY*sinA); 
    double By=(rZoomY*cosA); 
    double rx0=Src.width*0.5;  //(rx0,ry0)为旋转中心 
    double ry0=Src.height*0.5; 
    double Cx=(-(rx0+move_x)*rZoomX*cosA+(ry0+move_y)*rZoomY*sinA+rx0);
    double Cy=(-(rx0+move_x)*rZoomX*sinA-(ry0+move_y)*rZoomY*cosA+ry0); 

    Color32* pDstLine=Dst.pdata;
    double srcx0_f=(Cx);
    double srcy0_f=(Cy);
    for (long y=0;y<Dst.height;++y)
    {
        double srcx_f=srcx0_f;
        double srcy_f=srcy0_f;
        for (long x=0;x<Dst.width;++x)
        {
            long srcx=(long)(srcx_f);
            long srcy=(long)(srcy_f);
            srcx_f+=Ax;
            srcy_f+=Ay;
            if (Src.getIsInPic(srcx,srcy))
                pDstLine[x]=Src.pixels(srcx,srcy);
        }
        srcx0_f+=Bx;
        srcy0_f+=By;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
}

#ifdef asm   
    //SinCos:同时计算sin(Angle)和cos(Angle)的内嵌x86汇编函数
    void __declspec(naked) __stdcall SinCos(const double Angle,double& sina,double& cosa) 
    { 
        asm
        {
            fld  qword ptr [esp+4]//Angle   
            mov  eax,[esp+12]//&sina
            mov  edx,[esp+16]//&cosa
            fsincos   
            fstp qword ptr [edx]   
            fstp qword ptr [eax]  
            ret 16
        }
    } 
#else
    void SinCos(const double Angle,double& sina,double& cosa) 
    { 
        sina=sin(Angle);
        cosa=cos(Angle);
    } 
#endif


void PicRotary2(const TPixels32Ref& Dst,const TPixels32Ref& Src,double RotaryAngle,double ZoomX,double ZoomY,double move_x,double move_y)
{
    if ( (fabs(ZoomX*Src.width)<1.0e-4) || (fabs(ZoomY*Src.height)<1.0e-4) ) return; //太小的缩放比例认为已经不可见
    double tmprZoomXY=1.0/(ZoomX*ZoomY);  
    double rZoomX=tmprZoomXY*ZoomY;
    double rZoomY=tmprZoomXY*ZoomX;
    double sinA,cosA;
    SinCos(RotaryAngle,sinA,cosA);
    long Ax_16=(long)(rZoomX*cosA*(1<<16)); 
    long Ay_16=(long)(rZoomX*sinA*(1<<16)); 
    long Bx_16=(long)(-rZoomY*sinA*(1<<16)); 
    long By_16=(long)(rZoomY*cosA*(1<<16)); 
    double rx0=Src.width*0.5;  //(rx0,ry0)为旋转中心 
    double ry0=Src.height*0.5; 
    long Cx_16=(long)((-(rx0+move_x)*rZoomX*cosA+(ry0+move_y)*rZoomY*sinA+rx0)*(1<<16));
    long Cy_16=(long)((-(rx0+move_x)*rZoomX*sinA-(ry0+move_y)*rZoomY*cosA+ry0)*(1<<16)); 

    Color32* pDstLine=Dst.pdata;
    long srcx0_16=(Cx_16);
    long srcy0_16=(Cy_16);
    for (long y=0;y<Dst.height;++y)
    {
        long srcx_16=srcx0_16;
        long srcy_16=srcy0_16;
        for (long x=0;x<Dst.width;++x)
        {
            long srcx=(srcx_16>>16);
            long srcy=(srcy_16>>16);
            srcx_16+=Ax_16;
            srcy_16+=Ay_16;
            if (Src.getIsInPic(srcx,srcy))
                pDstLine[x]=Src.pixels(srcx,srcy);
        }
        srcx0_16+=Bx_16;
        srcy0_16+=By_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
}

struct TRotaryClipData{
public:
    long src_width;
    long src_height;
    long dst_width;
    long dst_height;
    long Ax_16; 
    long Ay_16; 
    long Bx_16; 
    long By_16; 
    long Cx_16;
    long Cy_16; 
    long border_width;//插值边界宽度
private:
    long cur_dst_up_x0;
    long cur_dst_up_x1;
    long cur_dst_down_x0;
    long cur_dst_down_x1;
    must_inline bool is_border_src(long src_x_16,long src_y_16)
    {
         return ( ( (src_x_16>=(-(border_width<<16)))&&((src_x_16>>16)<(src_width +border_width)) )
               && ( (src_y_16>=(-(border_width<<16)))&&((src_y_16>>16)<(src_height+border_width)) ) );
    }
    must_inline bool is_in_src(long src_x_16,long src_y_16)
    {
         return ( ( (src_x_16>=(border_width<<16))&&((src_x_16>>16)<(src_width-border_width) ) )
               && ( (src_y_16>=(border_width<<16))&&((src_y_16>>16)<(src_height-border_width)) ) );
    }
    void find_begin_in(long dst_y,long& out_dst_x,long& src_x_16,long& src_y_16)
    {
        src_x_16-=Ax_16;
        src_y_16-=Ay_16;
        while (is_border_src(src_x_16,src_y_16))
        {
            --out_dst_x;
            src_x_16-=Ax_16;
            src_y_16-=Ay_16;
        }
        src_x_16+=Ax_16;
        src_y_16+=Ay_16;
    }
    bool find_begin(long dst_y,long& out_dst_x0,long dst_x1)
    {
        long test_dst_x0=out_dst_x0-1;
        long src_x_16=Ax_16*test_dst_x0 + Bx_16*dst_y + Cx_16;
        long src_y_16=Ay_16*test_dst_x0 + By_16*dst_y + Cy_16;
        for (long i=test_dst_x0;i<=dst_x1;++i)
        {
            if (is_border_src(src_x_16,src_y_16))
            {
                out_dst_x0=i;
                if (i==test_dst_x0)
                    find_begin_in(dst_y,out_dst_x0,src_x_16,src_y_16);
                if (out_dst_x0<0)
                {
                    src_x_16-=(Ax_16*out_dst_x0);
                    src_y_16-=(Ay_16*out_dst_x0);
                }
                out_src_x0_16=src_x_16;
                out_src_y0_16=src_y_16;
                return true;
            }
            else
            {
                src_x_16+=Ax_16;
                src_y_16+=Ay_16;
            }
        }
        return false;
    }
    void find_end(long dst_y,long dst_x0,long& out_dst_x1)
    {
        long test_dst_x1=out_dst_x1;
        if (test_dst_x1<dst_x0) test_dst_x1=dst_x0;

        long src_x_16=Ax_16*test_dst_x1 + Bx_16*dst_y + Cx_16;
        long src_y_16=Ay_16*test_dst_x1 + By_16*dst_y + Cy_16;
        if (is_border_src(src_x_16,src_y_16))
        {
            ++test_dst_x1;
            src_x_16+=Ax_16;
            src_y_16+=Ay_16;
            while (is_border_src(src_x_16,src_y_16))
            {
                ++test_dst_x1;
                src_x_16+=Ax_16;
                src_y_16+=Ay_16;
            }
            out_dst_x1=test_dst_x1;
        }
        else
        {
            src_x_16-=Ax_16;
            src_y_16-=Ay_16;
            while (!is_border_src(src_x_16,src_y_16))
            {
                --test_dst_x1;
                src_x_16-=Ax_16;
                src_y_16-=Ay_16;
            }
            out_dst_x1=test_dst_x1;
        }
    }

    must_inline void update_out_dst_x_in()
    {
        if ((0==border_width)||(out_dst_x0_boder>=out_dst_x1_boder) )
        {
            out_dst_x0_in=out_dst_x0_boder;
            out_dst_x1_in=out_dst_x1_boder;
        }
        else
        {
            long src_x_16=out_src_x0_16;
            long src_y_16=out_src_y0_16;
            long i=out_dst_x0_boder;
            while (i<out_dst_x1_boder)
            {
                if (is_in_src(src_x_16,src_y_16)) break;
                src_x_16+=Ax_16;
                src_y_16+=Ay_16;
                ++i;
            }
            out_dst_x0_in=i;

            src_x_16=out_src_x0_16+(out_dst_x1_boder-out_dst_x0_boder)*Ax_16;
            src_y_16=out_src_y0_16+(out_dst_x1_boder-out_dst_x0_boder)*Ay_16;
            i=out_dst_x1_boder;
            while (i>out_dst_x0_in)
            {
                src_x_16-=Ax_16;
                src_y_16-=Ay_16;
                if (is_in_src(src_x_16,src_y_16)) break;
                --i;
            }
            out_dst_x1_in=i;
        }
    }
    must_inline void update_out_dst_up_x()
    {
        if (cur_dst_up_x0<0)
            out_dst_x0_boder=0;
        else
            out_dst_x0_boder=cur_dst_up_x0;
        if (cur_dst_up_x1>=dst_width)
            out_dst_x1_boder=dst_width;
        else
            out_dst_x1_boder=cur_dst_up_x1;
        update_out_dst_x_in();
    }
    must_inline void update_out_dst_down_x()
    {
        if (cur_dst_down_x0<0)
            out_dst_x0_boder=0;
        else
            out_dst_x0_boder=cur_dst_down_x0;
        if (cur_dst_down_x1>=dst_width)
            out_dst_x1_boder=dst_width;
        else
            out_dst_x1_boder=cur_dst_down_x1;
        update_out_dst_x_in();
    }

public:
    long out_src_x0_16;
    long out_src_y0_16;

    long out_dst_up_y;
    long out_dst_down_y;

    long out_dst_x0_boder;
    long out_dst_x0_in;
    long out_dst_x1_in;
    long out_dst_x1_boder;

public:
    bool inti_clip(double move_x,double move_y,unsigned long aborder_width)
    {
        border_width=aborder_width;

        //计算src中心点映射到dst后的坐标
        out_dst_down_y=(long)(src_height*0.5+move_y);
        cur_dst_down_x0=(long)(src_width*0.5+move_x);
        cur_dst_down_x1=cur_dst_down_x0;
        //得到初始扫描线
        if (find_begin(out_dst_down_y,cur_dst_down_x0,cur_dst_down_x1))
            find_end(out_dst_down_y,cur_dst_down_x0,cur_dst_down_x1);
        out_dst_up_y=out_dst_down_y;
        cur_dst_up_x0=cur_dst_down_x0;
        cur_dst_up_x1=cur_dst_down_x1;
        update_out_dst_up_x();
        return (cur_dst_down_x0<cur_dst_down_x1);
    }
    bool next_clip_line_down()
    {
        ++out_dst_down_y;
        if (!find_begin(out_dst_down_y,cur_dst_down_x0,cur_dst_down_x1)) return false;
        find_end(out_dst_down_y,cur_dst_down_x0,cur_dst_down_x1);
        update_out_dst_down_x();
        return (cur_dst_down_x0<cur_dst_down_x1);
    }
    bool next_clip_line_up()
    {
        --out_dst_up_y;
        if (!find_begin(out_dst_up_y,cur_dst_up_x0,cur_dst_up_x1)) return false;
        find_end(out_dst_up_y,cur_dst_up_x0,cur_dst_up_x1);
        update_out_dst_up_x();
        return (cur_dst_up_x0<cur_dst_up_x1);
    }
};


void PicRotary3_CopyLine(Color32* pDstLine,long dstCount,long Ax_16,long Ay_16,
                        long srcx0_16,long srcy0_16,const TPixels32Ref& SrcPic)
{
    Color32* pSrcLine=(Color32*)SrcPic.pdata;
    long src_byte_width=SrcPic.byte_width;
    for (long x=0;x<dstCount;++x)
    {
        pDstLine[x]= ((Color32*)( ((UInt8*)pSrcLine) + src_byte_width*(srcy0_16>>16))) [srcx0_16>>16];
        srcx0_16+=Ax_16;
        srcy0_16+=Ay_16;
    }
}

void PicRotary3(const TPixels32Ref& Dst,const TPixels32Ref& Src,double RotaryAngle,double ZoomX,double ZoomY,double move_x,double move_y)
{
    if ( (fabs(ZoomX*Src.width)<1.0e-4) || (fabs(ZoomY*Src.height)<1.0e-4) ) return; //太小的缩放比例认为已经不可见
    double tmprZoomXY=1.0/(ZoomX*ZoomY);  
    double rZoomX=tmprZoomXY*ZoomY;
    double rZoomY=tmprZoomXY*ZoomX;
    double sinA,cosA;
    SinCos(RotaryAngle,sinA,cosA);
    long Ax_16=(long)(rZoomX*cosA*(1<<16)); 
    long Ay_16=(long)(rZoomX*sinA*(1<<16)); 
    long Bx_16=(long)(-rZoomY*sinA*(1<<16)); 
    long By_16=(long)(rZoomY*cosA*(1<<16)); 
    double rx0=Src.width*0.5;  //(rx0,ry0)为旋转中心 
    double ry0=Src.height*0.5; 
    long Cx_16=(long)((-(rx0+move_x)*rZoomX*cosA+(ry0+move_y)*rZoomY*sinA+rx0)*(1<<16));
    long Cy_16=(long)((-(rx0+move_x)*rZoomX*sinA-(ry0+move_y)*rZoomY*cosA+ry0)*(1<<16)); 

    TRotaryClipData rcData;
    rcData.Ax_16=Ax_16;
    rcData.Bx_16=Bx_16;
    rcData.Cx_16=Cx_16;
    rcData.Ay_16=Ay_16;
    rcData.By_16=By_16;
    rcData.Cy_16=Cy_16;
    rcData.dst_width=Dst.width;
    rcData.dst_height=Dst.height;
    rcData.src_width=Src.width;
    rcData.src_height=Src.height;
    if (!rcData.inti_clip(move_x,move_y,0)) return;

    Color32* pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_down_y);
    while (true) //to down
    {
        long y=rcData.out_dst_down_y;
        if (y>=Dst.height) break;
        if (y>=0)
        {
            long x0=rcData.out_dst_x0_in;
            PicRotary3_CopyLine(&pDstLine[x0],rcData.out_dst_x1_in-x0,Ax_16,Ay_16,
                rcData.out_src_x0_16,rcData.out_src_y0_16,Src);
        }
        if (!rcData.next_clip_line_down()) break;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
   
    pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_up_y);
    while (rcData.next_clip_line_up()) //to up
    {
        long y=rcData.out_dst_up_y;
        if (y<0) break;
        ((UInt8*&)pDstLine)-=Dst.byte_width;
        if (y<Dst.height)
        {
            long x0=rcData.out_dst_x0_in;
            PicRotary3_CopyLine(&pDstLine[x0],rcData.out_dst_x1_in-x0,Ax_16,Ay_16,
                rcData.out_src_x0_16,rcData.out_src_y0_16,Src);
        }
    }
}


#ifdef MMX_ACTIVE
#ifdef asm   

void __declspec(naked) __stdcall PicRotarySSE_CopyLine(Color32* pDstLine,long dstCount,long Ax_16,long Ay_16,
                        //                                      [esp+ 4]      [esp+ 8]      [esp+12]   [esp+16]
                        long srcx0_16,long srcy0_16,const TPixels32Ref& SrcPic)
                        //   [esp+20]      [esp+24]                   [esp+28]
{
    //利用SSE的MOVNTQ指令优化写缓冲的汇编实现
    asm
    {
        push        ebx  
        push        esi  
        push        edi   
        push        ebp  
        //esp offset 16
        mov         eax,dword ptr [esp+28+16]//SrcPic
        mov         ebx,dword ptr [esp+ 8+16] 
        mov         edi,[eax]TPixels32Ref.pdata
        mov         esi,[eax]TPixels32Ref.byte_width
        mov         ecx,dword ptr [esp+20+16] 
        mov         eax,dword ptr [esp+24+16] 
        dec         ebx 
        xor         edx,edx 
        test        ebx,ebx 
        mov         dword ptr [esp+ 8+16],ebx 
        jle         loop_bound 

        //jmp   loop_begin
        //align 16
    loop_begin:
            mov         ebx,eax 
            add         eax,dword ptr [esp+16+16] 
            sar         ebx,16 
            imul        ebx,esi 
            add         ebx,edi 
            mov         ebp,ecx 
            add         ecx,dword ptr [esp+12+16] 
            sar         ebp,16 
            MOVD        MM0,dword ptr [ebx+ebp*4] 
            mov         ebx,eax 
            add         eax,dword ptr [esp+16+16] 
            sar         ebx,16 
            imul        ebx,esi 
            mov         ebp,ecx 
            add         ecx,dword ptr [esp+12+16] 
            sar         ebp,16 
            add         ebx,edi 
            MOVD        MM1,dword ptr [ebx+ebp*4] 
            mov         ebp,dword ptr [esp+ 4+16] 
            PUNPCKlDQ   MM0,MM1
            mov         ebx,dword ptr [esp+ 8+16] 
            MOVNTQ      qword ptr [ebp+edx*4],MM0 
            add         edx,2 
            cmp         edx,ebx  
            jl          loop_begin 

            EMMS

    loop_bound:
        cmp         edx,ebx 
        jne         loop_bound_end 
        sar         eax,16 
        imul        eax,esi 
        sar         ecx,16 
        add         eax,edi 
        mov         eax,dword ptr [eax+ecx*4] 
        mov         ecx,dword ptr [esp+ 4+16] 
        mov         dword ptr [ecx+edx*4],eax 
    loop_bound_end: 
        pop         ebp  
        pop         edi  
        pop         esi  
        pop         ebx  
        ret         28  
    }
}

void PicRotarySSE(const TPixels32Ref& Dst,const TPixels32Ref& Src,double RotaryAngle,double ZoomX,double ZoomY,double move_x,double move_y)
{
    if ( (fabs(ZoomX*Src.width)<1.0e-4) || (fabs(ZoomY*Src.height)<1.0e-4) ) return; //太小的缩放比例认为已经不可见
    double tmprZoomXY=1.0/(ZoomX*ZoomY);  
    double rZoomX=tmprZoomXY*ZoomY;
    double rZoomY=tmprZoomXY*ZoomX;
    double sinA,cosA;
    SinCos(RotaryAngle,sinA,cosA);
    long Ax_16=(long)(rZoomX*cosA*(1<<16)); 
    long Ay_16=(long)(rZoomX*sinA*(1<<16)); 
    long Bx_16=(long)(-rZoomY*sinA*(1<<16)); 
    long By_16=(long)(rZoomY*cosA*(1<<16)); 
    double rx0=Src.width*0.5;  //(rx0,ry0)为旋转中心 
    double ry0=Src.height*0.5; 
    long Cx_16=(long)((-(rx0+move_x)*rZoomX*cosA+(ry0+move_y)*rZoomY*sinA+rx0)*(1<<16));
    long Cy_16=(long)((-(rx0+move_x)*rZoomX*sinA-(ry0+move_y)*rZoomY*cosA+ry0)*(1<<16)); 

    TRotaryClipData rcData;
    rcData.Ax_16=Ax_16;
    rcData.Bx_16=Bx_16;
    rcData.Cx_16=Cx_16;
    rcData.Ay_16=Ay_16;
    rcData.By_16=By_16;
    rcData.Cy_16=Cy_16;
    rcData.dst_width=Dst.width;
    rcData.dst_height=Dst.height;
    rcData.src_width=Src.width;
    rcData.src_height=Src.height;
    if (!rcData.inti_clip(move_x,move_y,0)) return;

    Color32* pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_down_y);
    while (true) //to down
    {
        long y=rcData.out_dst_down_y;
        if (y>=Dst.height) break;
        if (y>=0)
        {
            long x0=rcData.out_dst_x0_in;
            PicRotarySSE_CopyLine(&pDstLine[x0],rcData.out_dst_x1_in-x0,Ax_16,Ay_16,
                rcData.out_src_x0_16,rcData.out_src_y0_16,Src);
        }
        if (!rcData.next_clip_line_down()) break;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
   
    pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_up_y);
    while (rcData.next_clip_line_up()) //to up 
    {
        long y=rcData.out_dst_up_y;
        if (y<0) break;
        ((UInt8*&)pDstLine)-=Dst.byte_width;
        if (y<Dst.height)
        {
            long x0=rcData.out_dst_x0_in;
            PicRotarySSE_CopyLine(&pDstLine[x0],rcData.out_dst_x1_in-x0,Ax_16,Ay_16,
                rcData.out_src_x0_16,rcData.out_src_y0_16,Src);
        }
    }

    asm  sfence //刷新写入
}


void __declspec(naked) __stdcall PicRotarySSE2_CopyLine(Color32* pDstLine,long dstCount,long Ax_16,long Ay_16,
                        //                                       [esp+ 4]      [esp+ 8]      [esp+12]   [esp+16]
                        long srcx0_16,long srcy0_16,const TPixels32Ref& SrcPic)
                        //   [esp+20]      [esp+24]                   [esp+28]
{
    //利用SSE2的MOVNTI指令优化写缓冲的汇编实现
    asm
    {
        push        ebx  
        push        esi  
        push        edi   
        push        ebp  
        //esp offset 16
        mov         eax,dword ptr [esp+28+16]//SrcPic
        mov         ebx,dword ptr [esp+ 8+16] 
        mov         edi,[eax]TPixels32Ref.pdata
        mov         esi,[eax]TPixels32Ref.byte_width
        mov         ecx,dword ptr [esp+20+16] 
        mov         eax,dword ptr [esp+24+16] 
        dec         ebx 
        xor         edx,edx 
        test        ebx,ebx 
        mov         dword ptr [esp+ 8+16],ebx 
        jle         loop_bound 

        jmp   loop_begin
        align 16
    loop_begin:
        mov         ebx,eax 
        add         eax,dword ptr [esp+16+16] 
        sar         ebx,16 
        imul        ebx,esi 
        add         ebx,edi 
        mov         ebp,ecx 
        add         ecx,dword ptr [esp+12+16] 
        sar         ebp,16 
        mov         ebx,dword ptr [ebx+ebp*4] 
        mov         ebp,dword ptr [esp+ 4+16] 
        MOVNTI         dword ptr [ebp+edx*4],ebx 
        mov         ebx,eax 
        add         eax,dword ptr [esp+16+16] 
        sar         ebx,16 
        imul        ebx,esi 
        mov         ebp,ecx 
        add         ecx,dword ptr [esp+12+16] 
        sar         ebp,16 
        add         ebx,edi 
        mov         ebx,dword ptr [ebx+ebp*4] 
        mov         ebp,dword ptr [esp+4+16] 
        MOVNTI         dword ptr [ebp+edx*4+4],ebx 
        mov         ebx,dword ptr [esp+ 8+16] 
        add         edx,2 
        cmp         edx,ebx 
        jl          loop_begin 
    loop_bound:
        cmp         edx,ebx 
        jne         loop_bound_end 
        sar         eax,16 
        imul        eax,esi 
        sar         ecx,16 
        add         eax,edi 
        mov         eax,dword ptr [eax+ecx*4] 
        mov         ecx,dword ptr [esp+ 4+16] 
        mov         dword ptr [ecx+edx*4],eax 
    loop_bound_end: 
        pop         ebp  
        pop         edi  
        pop         esi  
        pop         ebx  
        ret         28  
    }
}

void PicRotarySSE2(const TPixels32Ref& Dst,const TPixels32Ref& Src,double RotaryAngle,double ZoomX,double ZoomY,double move_x,double move_y)
{
    if ( (fabs(ZoomX*Src.width)<1.0e-4) || (fabs(ZoomY*Src.height)<1.0e-4) ) return; //太小的缩放比例认为已经不可见
    double tmprZoomXY=1.0/(ZoomX*ZoomY);  
    double rZoomX=tmprZoomXY*ZoomY;
    double rZoomY=tmprZoomXY*ZoomX;
    double sinA,cosA;
    SinCos(RotaryAngle,sinA,cosA);
    long Ax_16=(long)(rZoomX*cosA*(1<<16)); 
    long Ay_16=(long)(rZoomX*sinA*(1<<16)); 
    long Bx_16=(long)(-rZoomY*sinA*(1<<16)); 
    long By_16=(long)(rZoomY*cosA*(1<<16)); 
    double rx0=Src.width*0.5;  //(rx0,ry0)为旋转中心 
    double ry0=Src.height*0.5; 
    long Cx_16=(long)((-(rx0+move_x)*rZoomX*cosA+(ry0+move_y)*rZoomY*sinA+rx0)*(1<<16));
    long Cy_16=(long)((-(rx0+move_x)*rZoomX*sinA-(ry0+move_y)*rZoomY*cosA+ry0)*(1<<16)); 

    TRotaryClipData rcData;
    rcData.Ax_16=Ax_16;
    rcData.Bx_16=Bx_16;
    rcData.Cx_16=Cx_16;
    rcData.Ay_16=Ay_16;
    rcData.By_16=By_16;
    rcData.Cy_16=Cy_16;
    rcData.dst_width=Dst.width;
    rcData.dst_height=Dst.height;
    rcData.src_width=Src.width;
    rcData.src_height=Src.height;
    if (!rcData.inti_clip(move_x,move_y,0)) return;

    Color32* pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_down_y);
    while (true) //to down
    {
        long y=rcData.out_dst_down_y;
        if (y>=Dst.height) break;
        if (y>=0)
        {
            long x0=rcData.out_dst_x0_in;
            PicRotarySSE2_CopyLine(&pDstLine[x0],rcData.out_dst_x1_in-x0,Ax_16,Ay_16,
                rcData.out_src_x0_16,rcData.out_src_y0_16,Src);
        }
        if (!rcData.next_clip_line_down()) break;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
   
    pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_up_y);
    while (rcData.next_clip_line_up()) //to up 
    {
        long y=rcData.out_dst_up_y;
        if (y<0) break;
        ((UInt8*&)pDstLine)-=Dst.byte_width;
        if (y<Dst.height)
        {
            long x0=rcData.out_dst_x0_in;
            PicRotarySSE2_CopyLine(&pDstLine[x0],rcData.out_dst_x1_in-x0,Ax_16,Ay_16,
                rcData.out_src_x0_16,rcData.out_src_y0_16,Src);
        }
    }

    asm sfence //刷新写入
}

#endif
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////

    must_inline void BilInear_Fast(const TPixels32Ref& pic,const long x_16,const long y_16,Color32* result)
    {
        Color32* PColor0=&pic.pixels(x_16>>16,y_16>>16);
        Color32* PColor1=(Color32*)((UInt8*)PColor0+pic.byte_width); 
        unsigned long u_8=(unsigned char)(x_16>>8);
        unsigned long v_8=(unsigned char)(y_16>>8);
        unsigned long pm3_8=(u_8*v_8)>>8;
        unsigned long pm2_8=u_8-pm3_8;
        unsigned long pm1_8=v_8-pm3_8;
        unsigned long pm0_8=256-pm1_8-pm2_8-pm3_8;

        unsigned long Color=*(unsigned long*)(PColor0);
        unsigned long BR=(Color & 0x00FF00FF)*pm0_8;
        unsigned long GA=((Color & 0xFF00FF00)>>8)*pm0_8;
                      Color=((unsigned long*)(PColor0))[1];
                      GA+=((Color & 0xFF00FF00)>>8)*pm2_8;
                      BR+=(Color & 0x00FF00FF)*pm2_8;
                      Color=*(unsigned long*)(PColor1);
                      GA+=((Color & 0xFF00FF00)>>8)*pm1_8;
                      BR+=(Color & 0x00FF00FF)*pm1_8;
                      Color=((unsigned long*)(PColor1))[1];
                      GA+=((Color & 0xFF00FF00)>>8)*pm3_8;
                      BR+=(Color & 0x00FF00FF)*pm3_8;

        *(unsigned long*)result=(GA & 0xFF00FF00)|((BR & 0xFF00FF00)>>8);
    }

    inline void BilInear_Border(const TPixels32Ref& pic,const long x_16,const long y_16,Color32* result)
    {
        unsigned long x0=(x_16>>16);
        unsigned long y0=(y_16>>16);

        Color32 pixel[4];
        pixel[0]=pic.getPixelsBorder(x0,y0);
        pixel[2]=pic.getPixelsBorder(x0,y0+1);
        pixel[1]=pic.getPixelsBorder(x0+1,y0);
        pixel[3]=pic.getPixelsBorder(x0+1,y0+1);
        
        TPixels32Ref npic;
        npic.pdata     =&pixel[0];
        npic.byte_width=2*sizeof(Color32);
        //npic.width     =2;
        //npic.height    =2;
        BilInear_Fast(npic,(unsigned short)x_16,(unsigned short)y_16,result);
    }

    must_inline Color32 AlphaBlend(Color32 dst,Color32 src)
    {
        //AlphaBlend颜色混合公式(对其中的每个颜色分量分别处理)：
        // new_color=(dst_color*(255-src_color.alpha)+src_color*src_color.alpha)/255

        /*版本一：
        //提示：除法指令是很慢的操作，但vc2005可以把x/255编译为完全等价的"(x*M)>>N"类似的快速计算代码；
        unsigned long a=src.a;
        //if (a==0) return dst;
        //else if (a==255) return src;
        unsigned long ra=255-a;
        unsigned long result_b=(dst.b*ra+src.b*a)/255;
        unsigned long result_g=(dst.g*ra+src.g*a)/255;
        unsigned long result_r=(dst.r*ra+src.r*a)/255;
        unsigned long result_a=(dst.a*ra+a*a)/255;
        unsigned long result=(result_b) | (result_g<<8) | (result_r<<16) | (result_a<<24);
        return *(Color32*)&result;
        //*/

        /*版本二：
        //颜色处理中，也可以这样近似计算: x/255  =>  x>>8
        unsigned long a=src.a;
        unsigned long ra=255-a;
        unsigned long result_b=(dst.b*ra+src.b*a)>>8;
        unsigned long result_g=(dst.g*ra+src.g*a)>>8;
        unsigned long result_r=(dst.r*ra+src.r*a)>>8;
        unsigned long result_a=(dst.a*ra+a*a)>>8;
        unsigned long result=(result_b) | (result_g<<8) | (result_r<<16) | (result_a<<24);
        return *(Color32*)&result;
        //*/

        /*版本三：
        // (dst*(255-alpha)+src*alpha)>>8 近似为:(dst*(256-alpha)+src*alpha)>>8 
        //   即 ((dst<<8)+(src-dst)*alpha)>>8  从而用一个移位代替一个乘法 (*256会被优化为移位)
        long a=src.a;
        unsigned long result_b=((unsigned long)(((long)dst.b)*256+((long)src.b-(long)dst.b)*a))>>8;
        unsigned long result_g=((unsigned long)(((long)dst.g)*256+((long)src.g-(long)dst.g)*a))>>8;
        unsigned long result_r=((unsigned long)(((long)dst.r)*256+((long)src.r-(long)dst.r)*a))>>8;
        unsigned long result_a=((unsigned long)(((long)dst.a)*256+((long)a-(long)dst.a)*a))>>8;
        unsigned long result=(result_b) | (result_g<<8) | (result_r<<16) | (result_a<<24);
        return *(Color32*)&result;
        //*/

        //*版本四：
        //同时运行两路颜色分量
        unsigned long Src_ARGB=*(unsigned long*)&src;
        unsigned long Dst_ARGB=*(unsigned long*)&dst;
        unsigned long a=Src_ARGB>>24;
        unsigned long ra=255-a;
        unsigned long result_RB=((Dst_ARGB & 0x00FF00FF)*ra + (Src_ARGB & 0x00FF00FF)*a);
        unsigned long result_AG=(((Dst_ARGB & 0xFF00FF00)>>8)*ra + ((Src_ARGB & 0xFF00FF00)>>8)*a);
        unsigned long result=((result_RB & 0xFF00FF00)>>8) | (result_AG & 0xFF00FF00);
        return *(Color32*)&result;
        //*/
    }

    
void PicRotary_BilInear_CopyLine(Color32* pDstLine,long dst_border_x0,long dst_in_x0,long dst_in_x1,long dst_border_x1,
                        const TPixels32Ref& SrcPic,long srcx0_16,long srcy0_16,long Ax_16,long Ay_16)
{
    long x;
    for (x=dst_border_x0;x<dst_in_x0;++x)
    {
        Color32 src_color;
        BilInear_Border(SrcPic,srcx0_16,srcy0_16,&src_color);
        pDstLine[x]=AlphaBlend(pDstLine[x],src_color);
        srcx0_16+=Ax_16;
        srcy0_16+=Ay_16;
    }
    for (x=dst_in_x0;x<dst_in_x1;++x)
    {
        BilInear_Fast(SrcPic,srcx0_16,srcy0_16,&pDstLine[x]);
        srcx0_16+=Ax_16;
        srcy0_16+=Ay_16;
    }
    for (x=dst_in_x1;x<dst_border_x1;++x)
    {
        Color32 src_color;
        BilInear_Border(SrcPic,srcx0_16,srcy0_16,&src_color);
        pDstLine[x]=AlphaBlend(pDstLine[x],src_color);
        srcx0_16+=Ax_16;
        srcy0_16+=Ay_16;
    }
}

void PicRotaryBilInear(const TPixels32Ref& Dst,const TPixels32Ref& Src,double RotaryAngle,double ZoomX,double ZoomY,double move_x,double move_y)
{
    if ( (fabs(ZoomX*Src.width)<1.0e-4) || (fabs(ZoomY*Src.height)<1.0e-4) ) return; //太小的缩放比例认为已经不可见
    double tmprZoomXY=1.0/(ZoomX*ZoomY);  
    double rZoomX=tmprZoomXY*ZoomY;
    double rZoomY=tmprZoomXY*ZoomX;
    double sinA,cosA;
    SinCos(RotaryAngle,sinA,cosA);
    long Ax_16=(long)(rZoomX*cosA*(1<<16)); 
    long Ay_16=(long)(rZoomX*sinA*(1<<16)); 
    long Bx_16=(long)(-rZoomY*sinA*(1<<16)); 
    long By_16=(long)(rZoomY*cosA*(1<<16)); 
    double rx0=Src.width*0.5;  //(rx0,ry0)为旋转中心 
    double ry0=Src.height*0.5; 
    long Cx_16=(long)((-(rx0+move_x)*rZoomX*cosA+(ry0+move_y)*rZoomY*sinA+rx0)*(1<<16));
    long Cy_16=(long)((-(rx0+move_x)*rZoomX*sinA-(ry0+move_y)*rZoomY*cosA+ry0)*(1<<16)); 

    TRotaryClipData rcData;
    rcData.Ax_16=Ax_16;
    rcData.Bx_16=Bx_16;
    rcData.Cx_16=Cx_16;
    rcData.Ay_16=Ay_16;
    rcData.By_16=By_16;
    rcData.Cy_16=Cy_16;
    rcData.dst_width=Dst.width;
    rcData.dst_height=Dst.height;
    rcData.src_width=Src.width;
    rcData.src_height=Src.height;
    if (!rcData.inti_clip(move_x,move_y,1)) return;

    Color32* pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_down_y);
    while (true) //to down
    {
        long y=rcData.out_dst_down_y;
        if (y>=Dst.height) break;
        if (y>=0)
        {
            PicRotary_BilInear_CopyLine(pDstLine,rcData.out_dst_x0_boder,rcData.out_dst_x0_in,
                rcData.out_dst_x1_in,rcData.out_dst_x1_boder,Src,rcData.out_src_x0_16,rcData.out_src_y0_16,Ax_16,Ay_16);
        }
        if (!rcData.next_clip_line_down()) break;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
   
    pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_up_y);
    while (rcData.next_clip_line_up()) //to up 
    {
        long y=rcData.out_dst_up_y;
        if (y<0) break;
        ((UInt8*&)pDstLine)-=Dst.byte_width;
        if (y<Dst.height)
        {
            PicRotary_BilInear_CopyLine(pDstLine,rcData.out_dst_x0_boder,rcData.out_dst_x0_in,
                rcData.out_dst_x1_in,rcData.out_dst_x1_boder,Src,rcData.out_src_x0_16,rcData.out_src_y0_16,Ax_16,Ay_16);
        }
    }
}


#ifdef MMX_ACTIVE
#ifdef asm   

    must_inline Color32 AlphaBlend_MMX(Color32 dst,Color32 src)
    {
        unsigned long result;
        asm
        {
            PXOR      MM7,MM7
            MOVD      MM0,src
            MOVD      MM2,dst
            PUNPCKLBW MM0,MM7
            PUNPCKLBW MM2,MM7
            MOVQ      MM1,MM0
            PUNPCKHWD MM1,MM1
            PSUBW     MM0,MM2
            PUNPCKHDQ MM1,MM1
            PSLLW     MM2,8
            PMULLW    MM0,MM1
            PADDW     MM2,MM0
            PSRLW     MM2,8
            PACKUSWB  MM2,MM7
            MOVD      result,MM2
        }
        return *(Color32*)&result;
    }

    /*
    void __declspec(naked) __stdcall BilInear_Fast_MMX(const TPixels32Ref& pic,const long x_16,const long y_16,Color32* result)
    {
        asm
        {    
              mov       edx,[esp+12] //y_16
              mov       eax,[esp+8]  //x_16
              PXOR      mm7,mm7
              shl       edx,16
              shl       eax,16
              shr       edx,24 //edx=v_8
              shr       eax,24 //eax=u_8
              MOVD      MM6,edx
              MOVD      MM5,eax
              mov       ecx,[esp+4]//pic
              mov       edx,[esp+12]//y_16
              mov       eax,[ecx]TPixels32Ref.byte_width
              sar       edx,16
              imul      edx,eax
              add       edx,[ecx]TPixels32Ref.pdata
              add       eax,edx

              mov       ecx,[esp+8] //x_16
              sar       ecx,16     //srcx_16>>16

              MOVD         MM2,dword ptr [eax+ecx*4]  //MM2=Color1
              MOVD         MM0,dword ptr [eax+ecx*4+4]//MM0=Color3
              PUNPCKLWD    MM5,MM5
              PUNPCKLWD    MM6,MM6
              MOVD         MM3,dword ptr [edx+ecx*4]  //MM3=Color0
              MOVD         MM1,dword ptr [edx+ecx*4+4]//MM1=Color2
              PUNPCKLDQ    MM5,MM5 //mm5=u_8
              PUNPCKLBW    MM0,MM7
              PUNPCKLBW    MM1,MM7
              PUNPCKLBW    MM2,MM7
              PUNPCKLBW    MM3,MM7
              PSUBw        MM0,MM2
              PSUBw        MM1,MM3
              PSLLw        MM2,8
              PSLLw        MM3,8
              PMULlw       MM0,MM5
              PMULlw       MM1,MM5
              PUNPCKLDQ    MM6,MM6 //mm6=v_8
              PADDw        MM0,MM2
              PADDw        MM1,MM3

              PSRLw        MM0,8
              PSRLw        MM1,8
              PSUBw        MM0,MM1
              PSLLw        MM1,8
              PMULlw       MM0,MM6
              mov       eax,[esp+16]//result
              PADDw        MM0,MM1

              PSRLw        MM0,8
              PACKUSwb     MM0,MM7
              movd      [eax],MM0 

              //emms
              ret 16
        }
    }
    */

    must_inline void  BilInear_Fast_MMX(const TPixels32Ref& pic,const long x_16,const long y_16,Color32* result)
    {
        asm
        {    
              mov       edx,y_16
              mov       eax,x_16
              PXOR      mm7,mm7
              shl       edx,16
              shl       eax,16
              shr       edx,24 //edx=v_8
              shr       eax,24 //eax=u_8
              MOVD      MM6,edx
              MOVD      MM5,eax
              mov       ecx,[pic]
              mov       edx,y_16
              mov       eax,[ecx]TPixels32Ref.byte_width
              sar       edx,16
              imul      edx,eax
              add       edx,[ecx]TPixels32Ref.pdata
              add       eax,edx

              mov       ecx,x_16
              sar       ecx,16     //srcx_16>>16

              MOVD         MM2,dword ptr [eax+ecx*4]  //MM2=Color1
              MOVD         MM0,dword ptr [eax+ecx*4+4]//MM0=Color3
              PUNPCKLWD    MM5,MM5
              PUNPCKLWD    MM6,MM6
              MOVD         MM3,dword ptr [edx+ecx*4]  //MM3=Color0
              MOVD         MM1,dword ptr [edx+ecx*4+4]//MM1=Color2
              PUNPCKLDQ    MM5,MM5 //mm5=u_8
              PUNPCKLBW    MM0,MM7
              PUNPCKLBW    MM1,MM7
              PUNPCKLBW    MM2,MM7
              PUNPCKLBW    MM3,MM7
              PSUBw        MM0,MM2
              PSUBw        MM1,MM3
              PSLLw        MM2,8
              PSLLw        MM3,8
              PMULlw       MM0,MM5
              PMULlw       MM1,MM5
              PUNPCKLDQ    MM6,MM6 //mm6=v_8
              PADDw        MM0,MM2
              PADDw        MM1,MM3

              PSRLw        MM0,8
              PSRLw        MM1,8
              PSUBw        MM0,MM1
              PSLLw        MM1,8
              PMULlw       MM0,MM6
              mov       eax,result
              PADDw        MM0,MM1

              PSRLw        MM0,8
              PACKUSwb     MM0,MM7
              movd      [eax],MM0 
              //emms
        }
    }

    inline void BilInear_Border_MMX(const TPixels32Ref& pic,const long x_16,const long y_16,Color32* result)
    {
        unsigned long x0=(x_16>>16);
        unsigned long y0=(y_16>>16);

        Color32 pixel[4];
        pixel[0]=pic.getPixelsBorder(x0,y0);
        pixel[2]=pic.getPixelsBorder(x0,y0+1);
        pixel[1]=pic.getPixelsBorder(x0+1,y0);
        pixel[3]=pic.getPixelsBorder(x0+1,y0+1);
        
        TPixels32Ref npic;
        npic.pdata     =&pixel[0];
        npic.byte_width=2*sizeof(Color32);
        //npic.width     =2;
        //npic.height    =2;
        BilInear_Fast_MMX(npic,(unsigned short)x_16,(unsigned short)y_16,result);
    }

void PicRotary_BilInear_CopyLine_MMX(Color32* pDstLine,long dst_border_x0,long dst_in_x0,long dst_in_x1,long dst_border_x1,
                        const TPixels32Ref& SrcPic,long srcx0_16,long srcy0_16,long Ax_16,long Ay_16)
{
    long x;
    for (x=dst_border_x0;x<dst_in_x0;++x)
    {
        Color32 src_color;
        BilInear_Border_MMX(SrcPic,srcx0_16,srcy0_16,&src_color);
        pDstLine[x]=AlphaBlend_MMX(pDstLine[x],src_color);        
        srcx0_16+=Ax_16;
        srcy0_16+=Ay_16;
    }
    for (x=dst_in_x0;x<dst_in_x1;++x)
    {
        BilInear_Fast_MMX(SrcPic,srcx0_16,srcy0_16,&pDstLine[x]);
        srcx0_16+=Ax_16;
        srcy0_16+=Ay_16;
    }
    for (x=dst_in_x1;x<dst_border_x1;++x)
    {
        Color32 src_color;
        BilInear_Border_MMX(SrcPic,srcx0_16,srcy0_16,&src_color);
        pDstLine[x]=AlphaBlend_MMX(pDstLine[x],src_color);        
        srcx0_16+=Ax_16;
        srcy0_16+=Ay_16;
    }
    asm  emms
}

void PicRotaryBilInear_MMX(const TPixels32Ref& Dst,const TPixels32Ref& Src,double RotaryAngle,double ZoomX,double ZoomY,double move_x,double move_y)
{
    if ( (fabs(ZoomX*Src.width)<1.0e-4) || (fabs(ZoomY*Src.height)<1.0e-4) ) return; //太小的缩放比例认为已经不可见
    double tmprZoomXY=1.0/(ZoomX*ZoomY);  
    double rZoomX=tmprZoomXY*ZoomY;
    double rZoomY=tmprZoomXY*ZoomX;
    double sinA,cosA;
    SinCos(RotaryAngle,sinA,cosA);
    long Ax_16=(long)(rZoomX*cosA*(1<<16)); 
    long Ay_16=(long)(rZoomX*sinA*(1<<16)); 
    long Bx_16=(long)(-rZoomY*sinA*(1<<16)); 
    long By_16=(long)(rZoomY*cosA*(1<<16)); 
    double rx0=Src.width*0.5;  //(rx0,ry0)为旋转中心 
    double ry0=Src.height*0.5; 
    long Cx_16=(long)((-(rx0+move_x)*rZoomX*cosA+(ry0+move_y)*rZoomY*sinA+rx0)*(1<<16));
    long Cy_16=(long)((-(rx0+move_x)*rZoomX*sinA-(ry0+move_y)*rZoomY*cosA+ry0)*(1<<16)); 

    TRotaryClipData rcData;
    rcData.Ax_16=Ax_16;
    rcData.Bx_16=Bx_16;
    rcData.Cx_16=Cx_16;
    rcData.Ay_16=Ay_16;
    rcData.By_16=By_16;
    rcData.Cy_16=Cy_16;
    rcData.dst_width=Dst.width;
    rcData.dst_height=Dst.height;
    rcData.src_width=Src.width;
    rcData.src_height=Src.height;
    if (!rcData.inti_clip(move_x,move_y,1)) return;

    Color32* pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_down_y);
    while (true) //to down
    {
        long y=rcData.out_dst_down_y;
        if (y>=Dst.height) break;
        if (y>=0)
        {
            PicRotary_BilInear_CopyLine_MMX(pDstLine,rcData.out_dst_x0_boder,rcData.out_dst_x0_in,
                rcData.out_dst_x1_in,rcData.out_dst_x1_boder,Src,rcData.out_src_x0_16,rcData.out_src_y0_16,Ax_16,Ay_16);
        }
        if (!rcData.next_clip_line_down()) break;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
   
    pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_up_y);
    while (rcData.next_clip_line_up()) //to up 
    {
        long y=rcData.out_dst_up_y;
        if (y<0) break;
        ((UInt8*&)pDstLine)-=Dst.byte_width;
        if (y<Dst.height)
        {
            PicRotary_BilInear_CopyLine_MMX(pDstLine,rcData.out_dst_x0_boder,rcData.out_dst_x0_in,
                rcData.out_dst_x1_in,rcData.out_dst_x1_boder,Src,rcData.out_src_x0_16,rcData.out_src_y0_16,Ax_16,Ay_16);
        }
    }
}


    void BilInear_BlendBorder_MMX(const TPixels32Ref& pic,const long x_16,const long y_16,Color32* result)
    {
        unsigned long x0=(x_16>>16);
        unsigned long y0=(y_16>>16);

        Color32 pixel[4];
        pixel[0]=pic.getPixelsBorder(x0,y0);
        pixel[2]=pic.getPixelsBorder(x0,y0+1);
        pixel[1]=pic.getPixelsBorder(x0+1,y0);
        pixel[3]=pic.getPixelsBorder(x0+1,y0+1);
        
        TPixels32Ref npic;
        npic.pdata     =&pixel[0];
        npic.byte_width=2*sizeof(Color32);
        //npic.width     =2;
        //npic.height    =2;
        BilInear_Fast_MMX(npic,(unsigned short)x_16,(unsigned short)y_16,result);
    }

void PicRotary_BilInear_BlendLine_MMX(Color32* pDstLine,long dst_border_x0,long dst_in_x0,long dst_in_x1,long dst_border_x1,
                        const TPixels32Ref& SrcPic,long srcx0_16,long srcy0_16,long Ax_16,long Ay_16)
{
    long x;
    for (x=dst_border_x0;x<dst_in_x0;++x)
    {
        Color32 src_color;
        BilInear_BlendBorder_MMX(SrcPic,srcx0_16,srcy0_16,&src_color);
        if (src_color.a>0)
            pDstLine[x]=AlphaBlend_MMX(pDstLine[x],src_color);        
        srcx0_16+=Ax_16;
        srcy0_16+=Ay_16;
    }
    for (x=dst_in_x0;x<dst_in_x1;++x)
    {
        Color32 src_color;
        BilInear_Fast_MMX(SrcPic,srcx0_16,srcy0_16,&src_color);
        if (src_color.a==255)
            pDstLine[x]=src_color;  
        else if (src_color.a>0)
            pDstLine[x]=AlphaBlend_MMX(pDstLine[x],src_color);
        srcx0_16+=Ax_16;
        srcy0_16+=Ay_16;
    }
    for (x=dst_in_x1;x<dst_border_x1;++x)
    {
        Color32 src_color;
        BilInear_BlendBorder_MMX(SrcPic,srcx0_16,srcy0_16,&src_color);
        if (src_color.a>0)
            pDstLine[x]=AlphaBlend_MMX(pDstLine[x],src_color);        
        srcx0_16+=Ax_16;
        srcy0_16+=Ay_16;
    }
    asm  emms
}

//带alpha blend混合的二次插值旋转
void PicRotaryBlendBilInear_MMX(const TPixels32Ref& Dst,const TPixels32Ref& Src,double RotaryAngle,double ZoomX,double ZoomY,double move_x,double move_y)
{
    if ( (fabs(ZoomX*Src.width)<1.0e-4) || (fabs(ZoomY*Src.height)<1.0e-4) ) return; //太小的缩放比例认为已经不可见
    double tmprZoomXY=1.0/(ZoomX*ZoomY);  
    double rZoomX=tmprZoomXY*ZoomY;
    double rZoomY=tmprZoomXY*ZoomX;
    double sinA,cosA;
    SinCos(RotaryAngle,sinA,cosA);
    long Ax_16=(long)(rZoomX*cosA*(1<<16)); 
    long Ay_16=(long)(rZoomX*sinA*(1<<16)); 
    long Bx_16=(long)(-rZoomY*sinA*(1<<16)); 
    long By_16=(long)(rZoomY*cosA*(1<<16)); 
    double rx0=Src.width*0.5;  //(rx0,ry0)为旋转中心 
    double ry0=Src.height*0.5; 
    long Cx_16=(long)((-(rx0+move_x)*rZoomX*cosA+(ry0+move_y)*rZoomY*sinA+rx0)*(1<<16));
    long Cy_16=(long)((-(rx0+move_x)*rZoomX*sinA-(ry0+move_y)*rZoomY*cosA+ry0)*(1<<16)); 

    TRotaryClipData rcData;
    rcData.Ax_16=Ax_16;
    rcData.Bx_16=Bx_16;
    rcData.Cx_16=Cx_16;
    rcData.Ay_16=Ay_16;
    rcData.By_16=By_16;
    rcData.Cy_16=Cy_16;
    rcData.dst_width=Dst.width;
    rcData.dst_height=Dst.height;
    rcData.src_width=Src.width;
    rcData.src_height=Src.height;
    if (!rcData.inti_clip(move_x,move_y,1)) return;

    Color32* pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_down_y);
    while (true) //to down
    {
        long y=rcData.out_dst_down_y;
        if (y>=Dst.height) break;
        if (y>=0)
        {
            PicRotary_BilInear_BlendLine_MMX(pDstLine,rcData.out_dst_x0_boder,rcData.out_dst_x0_in,
                rcData.out_dst_x1_in,rcData.out_dst_x1_boder,Src,rcData.out_src_x0_16,rcData.out_src_y0_16,Ax_16,Ay_16);
        }
        if (!rcData.next_clip_line_down()) break;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
   
    pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_up_y);
    while (rcData.next_clip_line_up()) //to up 
    {
        long y=rcData.out_dst_up_y;
        if (y<0) break;
        ((UInt8*&)pDstLine)-=Dst.byte_width;
        if (y<Dst.height)
        {
            PicRotary_BilInear_BlendLine_MMX(pDstLine,rcData.out_dst_x0_boder,rcData.out_dst_x0_in,
                rcData.out_dst_x1_in,rcData.out_dst_x1_boder,Src,rcData.out_src_x0_16,rcData.out_src_y0_16,Ax_16,Ay_16);
        }
    }
}

#endif
#endif



//=================================================

        inline double SinXDivX(double x) 
        {
            //该函数计算插值曲线sin(x*PI)/(x*PI)的值 //PI=3.1415926535897932385; 
            //下面是它的近似拟合表达式
            const float a = -1; //a还可以取 a=-2,-1,-0.75,-0.5等等，起到调节锐化或模糊程度的作用

            if (x<0) x=-x; //x=abs(x);
            double x2=x*x;
            double x3=x2*x;
            if (x<=1)
              return (a+2)*x3 - (a+3)*x2 + 1;
            else if (x<=2) 
              return a*x3 - (5*a)*x2 + (8*a)*x - (4*a);
            else
              return 0;
        } 
        must_inline UInt8 ColorBound(long Color)
        {
            if (Color<=0)
                return 0;
            else if (Color>=255)
                return 255;
            else
                return (UInt8)Color;
        }

    static long SinXDivX_Table_8[(2<<8)+1];
    class _CAutoInti_SinXDivX_Table {
    private: 
        void _Inti_SinXDivX_Table()
        {
            for (long i=0;i<=(2<<8);++i)
                SinXDivX_Table_8[i]=long(0.5+256*SinXDivX(i*(1.0/(256))));
        };
    public:
        _CAutoInti_SinXDivX_Table() { _Inti_SinXDivX_Table(); }
    };
    static _CAutoInti_SinXDivX_Table __tmp_CAutoInti_SinXDivX_Table;



    void ThreeOrder_Fast(const TPixels32Ref& pic,const long x_16,const long y_16,Color32* result)
    {
        long u_8=(unsigned char)((x_16)>>8);
        long v_8=(unsigned char)((y_16)>>8);
        const Color32* pixel=&pic.pixels((x_16>>16)-1,(y_16>>16)-1);
        long pic_byte_width=pic.byte_width;

        long au_8[4],av_8[4];
        //
        au_8[0]=SinXDivX_Table_8[(1<<8)+u_8];
        au_8[1]=SinXDivX_Table_8[u_8];
        au_8[2]=SinXDivX_Table_8[(1<<8)-u_8];
        au_8[3]=SinXDivX_Table_8[(2<<8)-u_8];
        av_8[0]=SinXDivX_Table_8[(1<<8)+v_8];
        av_8[1]=SinXDivX_Table_8[v_8];
        av_8[2]=SinXDivX_Table_8[(1<<8)-v_8];
        av_8[3]=SinXDivX_Table_8[(2<<8)-v_8];

        long sR=0,sG=0,sB=0,sA=0;
        for (long i=0;i<4;++i)
        {
            long aA=au_8[0]*pixel[0].a + au_8[1]*pixel[1].a + au_8[2]*pixel[2].a + au_8[3]*pixel[3].a;
            long aR=au_8[0]*pixel[0].r + au_8[1]*pixel[1].r + au_8[2]*pixel[2].r + au_8[3]*pixel[3].r;
            long aG=au_8[0]*pixel[0].g + au_8[1]*pixel[1].g + au_8[2]*pixel[2].g + au_8[3]*pixel[3].g;
            long aB=au_8[0]*pixel[0].b + au_8[1]*pixel[1].b + au_8[2]*pixel[2].b + au_8[3]*pixel[3].b;
            sA+=aA*av_8[i];
            sR+=aR*av_8[i];
            sG+=aG*av_8[i];
            sB+=aB*av_8[i];
            ((UInt8*&)pixel)+=pic_byte_width;
        }

        *(unsigned long*)result=ColorBound(sB>>16) | (ColorBound(sG>>16)<<8) | (ColorBound(sR>>16)<<16)| (ColorBound(sA>>16)<<24);
    }

    void ThreeOrder_Border(const TPixels32Ref& pic,const long x_16,const long y_16,Color32* result)
    {
        unsigned long x0_sub1=(x_16>>16)-1;
        unsigned long y0_sub1=(y_16>>16)-1;
        long u_16_add1=((unsigned short)(x_16))+(1<<16);
        long v_16_add1=((unsigned short)(y_16))+(1<<16);

        Color32 pixel[16];
        long i,j;

        for (i=0;i<4;++i)
        {
            long y=y0_sub1+i;
            for (j=0;j<4;++j)
            {
                long x=x0_sub1+j;
                pixel[i*4+j]=pic.getPixelsBorder(x,y);
            }
        }
        
        TPixels32Ref npic;
        npic.pdata     =&pixel[0];
        npic.byte_width=4*sizeof(Color32);
        //npic.width     =4;
        //npic.height    =4;
        ThreeOrder_Fast(npic,u_16_add1,v_16_add1,result);
    }

void PicRotary_ThreeOrder_CopyLine(Color32* pDstLine,long dst_border_x0,long dst_in_x0,long dst_in_x1,long dst_border_x1,
                        const TPixels32Ref& SrcPic,long srcx0_16,long srcy0_16,long Ax_16,long Ay_16)
{
    long x;
    for (x=dst_border_x0;x<dst_in_x0;++x)
    {

        Color32 src_color;
        ThreeOrder_Border(SrcPic,srcx0_16,srcy0_16,&src_color);
        pDstLine[x]=AlphaBlend(pDstLine[x],src_color);
        srcx0_16+=Ax_16;
        srcy0_16+=Ay_16;
    }
    for (x=dst_in_x0;x<dst_in_x1;++x)
    {
        ThreeOrder_Fast(SrcPic,srcx0_16,srcy0_16,&pDstLine[x]);
        srcx0_16+=Ax_16;
        srcy0_16+=Ay_16;
    }
    for (x=dst_in_x1;x<dst_border_x1;++x)
    {
        Color32 src_color;
        ThreeOrder_Border(SrcPic,srcx0_16,srcy0_16,&src_color);
        pDstLine[x]=AlphaBlend(pDstLine[x],src_color);
        srcx0_16+=Ax_16;
        srcy0_16+=Ay_16;
    }
}

void PicRotaryThreeOrder(const TPixels32Ref& Dst,const TPixels32Ref& Src,double RotaryAngle,double ZoomX,double ZoomY,double move_x,double move_y)
{
    if ( (fabs(ZoomX*Src.width)<1.0e-4) || (fabs(ZoomY*Src.height)<1.0e-4) ) return; //太小的缩放比例认为已经不可见
    double tmprZoomXY=1.0/(ZoomX*ZoomY);  
    double rZoomX=tmprZoomXY*ZoomY;
    double rZoomY=tmprZoomXY*ZoomX;
    double sinA,cosA;
    SinCos(RotaryAngle,sinA,cosA);
    long Ax_16=(long)(rZoomX*cosA*(1<<16)); 
    long Ay_16=(long)(rZoomX*sinA*(1<<16)); 
    long Bx_16=(long)(-rZoomY*sinA*(1<<16)); 
    long By_16=(long)(rZoomY*cosA*(1<<16)); 
    double rx0=Src.width*0.5;  //(rx0,ry0)为旋转中心 
    double ry0=Src.height*0.5; 
    long Cx_16=(long)((-(rx0+move_x)*rZoomX*cosA+(ry0+move_y)*rZoomY*sinA+rx0)*(1<<16));
    long Cy_16=(long)((-(rx0+move_x)*rZoomX*sinA-(ry0+move_y)*rZoomY*cosA+ry0)*(1<<16)); 

    TRotaryClipData rcData;
    rcData.Ax_16=Ax_16;
    rcData.Bx_16=Bx_16;
    rcData.Cx_16=Cx_16;
    rcData.Ay_16=Ay_16;
    rcData.By_16=By_16;
    rcData.Cy_16=Cy_16;
    rcData.dst_width=Dst.width;
    rcData.dst_height=Dst.height;
    rcData.src_width=Src.width;
    rcData.src_height=Src.height;
    if (!rcData.inti_clip(move_x,move_y,2)) return;

    Color32* pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_down_y);
    while (true) //to down
    {
        long y=rcData.out_dst_down_y;
        if (y>=Dst.height) break;
        if (y>=0)
        {
            PicRotary_ThreeOrder_CopyLine(pDstLine,rcData.out_dst_x0_boder,rcData.out_dst_x0_in,
                rcData.out_dst_x1_in,rcData.out_dst_x1_boder,Src,rcData.out_src_x0_16,rcData.out_src_y0_16,Ax_16,Ay_16);
        }
        if (!rcData.next_clip_line_down()) break;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
   
    pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_up_y);
    while (rcData.next_clip_line_up()) //to up 
    {
        long y=rcData.out_dst_up_y;
        if (y<0) break;
        ((UInt8*&)pDstLine)-=Dst.byte_width;
        if (y<Dst.height)
        {
            PicRotary_ThreeOrder_CopyLine(pDstLine,rcData.out_dst_x0_boder,rcData.out_dst_x0_in,
                rcData.out_dst_x1_in,rcData.out_dst_x1_boder,Src,rcData.out_src_x0_16,rcData.out_src_y0_16,Ax_16,Ay_16);
        }
    }
}

#ifdef MMX_ACTIVE
#ifdef asm   

    typedef   unsigned long TMMXData32;
    static TMMXData32 SinXDivX_Table_MMX[(2<<8)+1];
    class _CAutoInti_SinXDivX_Table_MMX {
    private: 
        void _Inti_SinXDivX_Table_MMX()
        {
            for (long i=0;i<=(2<<8);++i)
            {
                unsigned short t=(unsigned short)(0.5+(1<<14)*SinXDivX(i*(1.0/(256))));
                unsigned long tl=t | (((unsigned long)t)<<16);
                SinXDivX_Table_MMX[i]=tl;
            }
        };
    public:
        _CAutoInti_SinXDivX_Table_MMX() { _Inti_SinXDivX_Table_MMX(); }
    };
    static _CAutoInti_SinXDivX_Table_MMX __tmp_CAutoInti_SinXDivX_Table_MMX;


    void __declspec(naked) _private_ThreeOrder_Fast_MMX()
    {
        asm
        {
            movd        mm1,dword ptr [edx]
            movd        mm2,dword ptr [edx+4]
            movd        mm3,dword ptr [edx+8]
            movd        mm4,dword ptr [edx+12]
            movd        mm5,dword ptr [(offset SinXDivX_Table_MMX)+256*4+eax*4]
            movd        mm6,dword ptr [(offset SinXDivX_Table_MMX)+eax*4]
            punpcklbw   mm1,mm7
            punpcklbw   mm2,mm7
            punpcklwd   mm5,mm5
            punpcklwd   mm6,mm6
            psllw       mm1,7
            psllw       mm2,7
            pmulhw      mm1,mm5
            pmulhw      mm2,mm6
            punpcklbw   mm3,mm7
            punpcklbw   mm4,mm7
            movd        mm5,dword ptr [(offset SinXDivX_Table_MMX)+256*4+ecx*4]
            movd        mm6,dword ptr [(offset SinXDivX_Table_MMX)+512*4+ecx*4]
            punpcklwd   mm5,mm5
            punpcklwd   mm6,mm6
            psllw       mm3,7
            psllw       mm4,7
            pmulhw      mm3,mm5
            pmulhw      mm4,mm6
            paddsw      mm1,mm2
            paddsw      mm3,mm4
            movd        mm6,dword ptr [ebx] //v
            paddsw      mm1,mm3
            punpcklwd   mm6,mm6

            pmulhw      mm1,mm6
            add     edx,esi  //+pic.byte_width
            paddsw      mm0,mm1

            ret
        }
    }

    must_inline void ThreeOrder_Fast_MMX(const TPixels32Ref& pic,const long x_16,const long y_16,Color32* result)
    {
        asm
        {
            mov     ecx,pic
            mov     eax,y_16
            mov     ebx,x_16
            movzx   edi,ah //v_8
            mov     edx,[ecx]TPixels32Ref.pdata
            shr     eax,16
            mov     esi,[ecx]TPixels32Ref.byte_width
            dec     eax
            movzx   ecx,bh //u_8
            shr     ebx,16
            imul    eax,esi
            lea     edx,[edx+ebx*4-4]
            add     edx,eax //pixel

            mov     eax,ecx
            neg     ecx

            pxor    mm7,mm7  //0
            //mov     edx,pixel
            pxor    mm0,mm0  //result=0
            //lea     eax,auv_7

            lea    ebx,[(offset SinXDivX_Table_MMX)+256*4+edi*4]
            call  _private_ThreeOrder_Fast_MMX
            lea    ebx,[(offset SinXDivX_Table_MMX)+edi*4]
            call  _private_ThreeOrder_Fast_MMX
            neg    edi
            lea    ebx,[(offset SinXDivX_Table_MMX)+256*4+edi*4]
            call  _private_ThreeOrder_Fast_MMX
            lea    ebx,[(offset SinXDivX_Table_MMX)+512*4+edi*4]
            call  _private_ThreeOrder_Fast_MMX

            psraw     mm0,3
            mov       eax,result
            packuswb  mm0,mm7
            movd      [eax],mm0

            //emms
        }
    }

    void ThreeOrder_Border_MMX(const TPixels32Ref& pic,const long x_16,const long y_16,Color32* result)
    {
        unsigned long x0_sub1=(x_16>>16)-1;
        unsigned long y0_sub1=(y_16>>16)-1;
        long u_16_add1=((unsigned short)(x_16))+(1<<16);
        long v_16_add1=((unsigned short)(y_16))+(1<<16);

        Color32 pixel[16];
        long i,j;

        for (i=0;i<4;++i)
        {
            long y=y0_sub1+i;
            for (j=0;j<4;++j)
            {
                long x=x0_sub1+j;
                pixel[i*4+j]=pic.getPixelsBorder(x,y);
            }
        }
        
        TPixels32Ref npic;
        npic.pdata     =&pixel[0];
        npic.byte_width=4*sizeof(Color32);
        //npic.width     =4;
        //npic.height    =4;
        ThreeOrder_Fast_MMX(npic,u_16_add1,v_16_add1,result);
    }

void PicRotary_ThreeOrder_CopyLine_MMX(Color32* pDstLine,long dst_border_x0,long dst_in_x0,long dst_in_x1,long dst_border_x1,
                        const TPixels32Ref& SrcPic,long srcx0_16,long srcy0_16,long Ax_16,long Ay_16)
{
    long x;
    for (x=dst_border_x0;x<dst_in_x0;++x)
    {
        Color32 src_color;
        ThreeOrder_Border_MMX(SrcPic,srcx0_16,srcy0_16,&src_color);
        pDstLine[x]=AlphaBlend_MMX(pDstLine[x],src_color);
        srcx0_16+=Ax_16;
        srcy0_16+=Ay_16;
    }
    for (x=dst_in_x0;x<dst_in_x1;++x)
    {
        ThreeOrder_Fast_MMX(SrcPic,srcx0_16,srcy0_16,&pDstLine[x]);
        srcx0_16+=Ax_16;
        srcy0_16+=Ay_16;
    }
    for (x=dst_in_x1;x<dst_border_x1;++x)
    {
        Color32 src_color;
        ThreeOrder_Border_MMX(SrcPic,srcx0_16,srcy0_16,&src_color);
        pDstLine[x]=AlphaBlend_MMX(pDstLine[x],src_color);
        srcx0_16+=Ax_16;
        srcy0_16+=Ay_16;
    }
    asm  emms
}

void PicRotaryThreeOrder_MMX(const TPixels32Ref& Dst,const TPixels32Ref& Src,double RotaryAngle,double ZoomX,double ZoomY,double move_x,double move_y)
{
    if ( (fabs(ZoomX*Src.width)<1.0e-4) || (fabs(ZoomY*Src.height)<1.0e-4) ) return; //太小的缩放比例认为已经不可见
    double tmprZoomXY=1.0/(ZoomX*ZoomY);  
    double rZoomX=tmprZoomXY*ZoomY;
    double rZoomY=tmprZoomXY*ZoomX;
    double sinA,cosA;
    SinCos(RotaryAngle,sinA,cosA);
    long Ax_16=(long)(rZoomX*cosA*(1<<16)); 
    long Ay_16=(long)(rZoomX*sinA*(1<<16)); 
    long Bx_16=(long)(-rZoomY*sinA*(1<<16)); 
    long By_16=(long)(rZoomY*cosA*(1<<16)); 
    double rx0=Src.width*0.5;  //(rx0,ry0)为旋转中心 
    double ry0=Src.height*0.5; 
    long Cx_16=(long)((-(rx0+move_x)*rZoomX*cosA+(ry0+move_y)*rZoomY*sinA+rx0)*(1<<16));
    long Cy_16=(long)((-(rx0+move_x)*rZoomX*sinA-(ry0+move_y)*rZoomY*cosA+ry0)*(1<<16)); 

    TRotaryClipData rcData;
    rcData.Ax_16=Ax_16;
    rcData.Bx_16=Bx_16;
    rcData.Cx_16=Cx_16;
    rcData.Ay_16=Ay_16;
    rcData.By_16=By_16;
    rcData.Cy_16=Cy_16;
    rcData.dst_width=Dst.width;
    rcData.dst_height=Dst.height;
    rcData.src_width=Src.width;
    rcData.src_height=Src.height;
    if (!rcData.inti_clip(move_x,move_y,2)) return;

    Color32* pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_down_y);
    while (true) //to down
    {
        long y=rcData.out_dst_down_y;
        if (y>=Dst.height) break;
        if (y>=0)
        {
            PicRotary_ThreeOrder_CopyLine_MMX(pDstLine,rcData.out_dst_x0_boder,rcData.out_dst_x0_in,
                rcData.out_dst_x1_in,rcData.out_dst_x1_boder,Src,rcData.out_src_x0_16,rcData.out_src_y0_16,Ax_16,Ay_16);
        }
        if (!rcData.next_clip_line_down()) break;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
   
    pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_up_y);
    while (rcData.next_clip_line_up()) //to up 
    {
        long y=rcData.out_dst_up_y;
        if (y<0) break;
        ((UInt8*&)pDstLine)-=Dst.byte_width;
        if (y<Dst.height)
        {
            PicRotary_ThreeOrder_CopyLine_MMX(pDstLine,rcData.out_dst_x0_boder,rcData.out_dst_x0_in,
                rcData.out_dst_x1_in,rcData.out_dst_x1_boder,Src,rcData.out_src_x0_16,rcData.out_src_y0_16,Ax_16,Ay_16);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
#include <vector>

    struct TBlockLineWork
    {
    public:
        Color32*    pdst;
        long        width_border0;
        long        width_in;
        long        width_border1;
        long        src_x0_16;
        long        src_y0_16;
        TBlockLineWork(Color32* _pdst,long _width_in,long _src_x0_16,long _src_y0_16)
            :pdst(_pdst),width_in(_width_in),src_x0_16(_src_x0_16),src_y0_16(_src_y0_16),width_border0(0),width_border1(0) {}
        TBlockLineWork(Color32* _pdst,long _width_border0,long _width_in,long _width_border1,long _src_x0_16,long _src_y0_16)
            :pdst(_pdst),width_in(_width_in),src_x0_16(_src_x0_16),src_y0_16(_src_y0_16),width_border0(_width_border0),width_border1(_width_border1) {}
    };
    typedef std::vector<TBlockLineWork> TBlockWork;

    void do_PicRotarySSE_Block(TBlockWork& BlockWork,const TPixels32Ref& Src,long Ax_16,long Ay_16)
    {
        const long rotary_block_width=64;  //128 
        const long rotary_block_height=rotary_block_width;
        long height=BlockWork.size();
        for (long y=0;y<height;y+=rotary_block_height)
        { 
            long cur_block_height;
            if (rotary_block_height<=(height-y))
                cur_block_height=rotary_block_height;
            else 
                cur_block_height=(height-y);
            bool is_line_filish=false;
            while (!is_line_filish)
            {
                is_line_filish=true;
                for (long yi=y;yi<y+cur_block_height;++yi)
                {
                    TBlockLineWork* BlockLine=&BlockWork[yi];
                    long cur_block_width=BlockLine->width_in;
                    if (cur_block_width>0)
                    {
                        is_line_filish=false;
                        if (cur_block_width>rotary_block_width)
                           cur_block_width=rotary_block_width;
                        PicRotarySSE_CopyLine(BlockLine->pdst,cur_block_width,Ax_16,Ay_16,
                            BlockLine->src_x0_16,BlockLine->src_y0_16,Src);
                        BlockLine->pdst=&BlockLine->pdst[cur_block_width];
                        BlockLine->width_in-=cur_block_width;
                        BlockLine->src_x0_16+=(Ax_16*cur_block_width);
                        BlockLine->src_y0_16+=(Ay_16*cur_block_width);
                    }
                }
            }
        }
    }

void PicRotarySSE_Block(const TPixels32Ref& Dst,const TPixels32Ref& Src,double RotaryAngle,double ZoomX,double ZoomY,double move_x,double move_y)
{
    if ( (fabs(ZoomX*Src.width)<1.0e-4) || (fabs(ZoomY*Src.height)<1.0e-4) ) return; //太小的缩放比例认为已经不可见
    double tmprZoomXY=1.0/(ZoomX*ZoomY);  
    double rZoomX=tmprZoomXY*ZoomY;
    double rZoomY=tmprZoomXY*ZoomX;
    double sinA,cosA;
    SinCos(RotaryAngle,sinA,cosA);
    long Ax_16=(long)(rZoomX*cosA*(1<<16)); 
    long Ay_16=(long)(rZoomX*sinA*(1<<16)); 
    long Bx_16=(long)(-rZoomY*sinA*(1<<16)); 
    long By_16=(long)(rZoomY*cosA*(1<<16)); 
    double rx0=Src.width*0.5;  //(rx0,ry0)为旋转中心 
    double ry0=Src.height*0.5; 
    long Cx_16=(long)((-(rx0+move_x)*rZoomX*cosA+(ry0+move_y)*rZoomY*sinA+rx0)*(1<<16));
    long Cy_16=(long)((-(rx0+move_x)*rZoomX*sinA-(ry0+move_y)*rZoomY*cosA+ry0)*(1<<16)); 

    TRotaryClipData rcData;
    rcData.Ax_16=Ax_16;
    rcData.Bx_16=Bx_16;
    rcData.Cx_16=Cx_16;
    rcData.Ay_16=Ay_16;
    rcData.By_16=By_16;
    rcData.Cy_16=Cy_16;
    rcData.dst_width=Dst.width;
    rcData.dst_height=Dst.height;
    rcData.src_width=Src.width;
    rcData.src_height=Src.height;
    if (!rcData.inti_clip(move_x,move_y,0)) return;


    TBlockWork BlockWork;

    Color32* pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_down_y);
    while (true) //to down
    {
        long y=rcData.out_dst_down_y;
        if (y>=Dst.height) break;
        if (y>=0)
        {
            long x0=rcData.out_dst_x0_in;
            BlockWork.push_back(TBlockLineWork(&pDstLine[x0],rcData.out_dst_x1_in-x0,rcData.out_src_x0_16,rcData.out_src_y0_16));
        }
        if (!rcData.next_clip_line_down()) break;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
    for (long sleft=0,sright=BlockWork.size()-1;sleft<sright;++sleft,--sright)
        std::swap(BlockWork[sleft],BlockWork[sright]);
   
    pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_up_y);
    while (rcData.next_clip_line_up()) //to up 
    {
        long y=rcData.out_dst_up_y;
        if (y<0) break;
        ((UInt8*&)pDstLine)-=Dst.byte_width;
        if (y<Dst.height)
        {
            long x0=rcData.out_dst_x0_in;
            BlockWork.push_back(TBlockLineWork(&pDstLine[x0],rcData.out_dst_x1_in-x0,rcData.out_src_x0_16,rcData.out_src_y0_16));
        }
    }
    do_PicRotarySSE_Block(BlockWork,Src,Ax_16,Ay_16);

    asm  sfence //刷新写入
}

    inline void PicRotary_BilInear_CopyLine_Fast_MMX(Color32* pDstLine,long width,
                            long Ax_16,long Ay_16,long srcx0_16,long srcy0_16,const TPixels32Ref& SrcPic)
    {
        for (long x=0;x<width;++x)
        {
            BilInear_Fast_MMX(SrcPic,srcx0_16,srcy0_16,&pDstLine[x]);
            srcx0_16+=Ax_16;
            srcy0_16+=Ay_16;
        }
    }
    inline void PicRotary_BilInear_CopyLine_Border_MMX(Color32* pDstLine,long width,
                            long Ax_16,long Ay_16,long srcx0_16,long srcy0_16,const TPixels32Ref& SrcPic)
    {
        for (long x=0;x<width;++x)
        {
            Color32 src_color;
            BilInear_Border_MMX(SrcPic,srcx0_16,srcy0_16,&src_color);
            pDstLine[x]=AlphaBlend_MMX(pDstLine[x],src_color);        
            srcx0_16+=Ax_16;
            srcy0_16+=Ay_16;
        }
    }

    void do_PicRotary_BilInear_MMX_Block(TBlockWork& BlockWork,const TPixels32Ref& Src,long Ax_16,long Ay_16)
    {
        const long rotary_block_width=64;  //128 
        const long rotary_block_height=rotary_block_width;
        long height=BlockWork.size();
        for (long y=0;y<height;y+=rotary_block_height)
        { 
            long cur_block_height;
            if (rotary_block_height<=(height-y))
                cur_block_height=rotary_block_height;
            else 
                cur_block_height=(height-y);

            for (long yi=y;yi<y+cur_block_height;++yi)
            {
                TBlockLineWork* BlockLine=&BlockWork[yi];
                long cur_block_width=BlockLine->width_border0;
                if (cur_block_width>0)
                {
                    PicRotary_BilInear_CopyLine_Border_MMX(BlockLine->pdst,cur_block_width,Ax_16,Ay_16,
                        BlockLine->src_x0_16,BlockLine->src_y0_16,Src);
                    BlockLine->pdst=&BlockLine->pdst[cur_block_width];
                    BlockLine->src_x0_16+=(Ax_16*cur_block_width);
                    BlockLine->src_y0_16+=(Ay_16*cur_block_width);
                }
            }

            bool is_line_filish=false;
            while (!is_line_filish)
            {
                is_line_filish=true;
                for (long yi=y;yi<y+cur_block_height;++yi)
                {
                    TBlockLineWork* BlockLine=&BlockWork[yi];
                    long cur_block_width=BlockLine->width_in;
                    if (cur_block_width>0)
                    {
                        is_line_filish=false;
                        if (cur_block_width>rotary_block_width)
                           cur_block_width=rotary_block_width;
                        PicRotary_BilInear_CopyLine_Fast_MMX(BlockLine->pdst,cur_block_width,Ax_16,Ay_16,
                            BlockLine->src_x0_16,BlockLine->src_y0_16,Src);
                        BlockLine->pdst=&BlockLine->pdst[cur_block_width];
                        BlockLine->width_in-=cur_block_width;
                        BlockLine->src_x0_16+=(Ax_16*cur_block_width);
                        BlockLine->src_y0_16+=(Ay_16*cur_block_width);
                    }
                }
            }

            for (long yi=y;yi<y+cur_block_height;++yi)
            {
                TBlockLineWork* BlockLine=&BlockWork[yi];
                long cur_block_width=BlockLine->width_border1;
                if (cur_block_width>0)
                {
                    PicRotary_BilInear_CopyLine_Border_MMX(BlockLine->pdst,cur_block_width,Ax_16,Ay_16,
                        BlockLine->src_x0_16,BlockLine->src_y0_16,Src);
                    //BlockLine->pdst=&BlockLine->pdst[cur_block_width];
                    //BlockLine->src_x0_16+=(Ax_16*cur_block_width);
                    //BlockLine->src_y0_16+=(Ay_16*cur_block_width);
                }
            }
        }
    
    
        asm  emms
    }

void PicRotaryBilInear_MMX_Block(const TPixels32Ref& Dst,const TPixels32Ref& Src,double RotaryAngle,double ZoomX,double ZoomY,double move_x,double move_y)
{
    if ( (fabs(ZoomX*Src.width)<1.0e-4) || (fabs(ZoomY*Src.height)<1.0e-4) ) return; //太小的缩放比例认为已经不可见
    double tmprZoomXY=1.0/(ZoomX*ZoomY);  
    double rZoomX=tmprZoomXY*ZoomY;
    double rZoomY=tmprZoomXY*ZoomX;
    double sinA,cosA;
    SinCos(RotaryAngle,sinA,cosA);
    long Ax_16=(long)(rZoomX*cosA*(1<<16)); 
    long Ay_16=(long)(rZoomX*sinA*(1<<16)); 
    long Bx_16=(long)(-rZoomY*sinA*(1<<16)); 
    long By_16=(long)(rZoomY*cosA*(1<<16)); 
    double rx0=Src.width*0.5;  //(rx0,ry0)为旋转中心 
    double ry0=Src.height*0.5; 
    long Cx_16=(long)((-(rx0+move_x)*rZoomX*cosA+(ry0+move_y)*rZoomY*sinA+rx0)*(1<<16));
    long Cy_16=(long)((-(rx0+move_x)*rZoomX*sinA-(ry0+move_y)*rZoomY*cosA+ry0)*(1<<16)); 

    TRotaryClipData rcData;
    rcData.Ax_16=Ax_16;
    rcData.Bx_16=Bx_16;
    rcData.Cx_16=Cx_16;
    rcData.Ay_16=Ay_16;
    rcData.By_16=By_16;
    rcData.Cy_16=Cy_16;
    rcData.dst_width=Dst.width;
    rcData.dst_height=Dst.height;
    rcData.src_width=Src.width;
    rcData.src_height=Src.height;
    if (!rcData.inti_clip(move_x,move_y,1)) return;

    TBlockWork BlockWork;

    Color32* pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_down_y);
    while (true) //to down
    {
        long y=rcData.out_dst_down_y;
        if (y>=Dst.height) break;
        if (y>=0)
        {
            BlockWork.push_back(TBlockLineWork(&pDstLine[rcData.out_dst_x0_boder],
                rcData.out_dst_x0_in-rcData.out_dst_x0_boder,rcData.out_dst_x1_in-rcData.out_dst_x0_in,
                rcData.out_dst_x1_boder-rcData.out_dst_x1_in,rcData.out_src_x0_16,rcData.out_src_y0_16));
        }
        if (!rcData.next_clip_line_down()) break;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
   
    for (long sleft=0,sright=BlockWork.size()-1;sleft<sright;++sleft,--sright)
        std::swap(BlockWork[sleft],BlockWork[sright]);

    pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_up_y);
    while (rcData.next_clip_line_up()) //to up 
    {
        long y=rcData.out_dst_up_y;
        if (y<0) break;
        ((UInt8*&)pDstLine)-=Dst.byte_width;
        if (y<Dst.height)
        {
            BlockWork.push_back(TBlockLineWork(&pDstLine[rcData.out_dst_x0_boder],
                rcData.out_dst_x0_in-rcData.out_dst_x0_boder,rcData.out_dst_x1_in-rcData.out_dst_x0_in,
                rcData.out_dst_x1_boder-rcData.out_dst_x1_in,rcData.out_src_x0_16,rcData.out_src_y0_16));
        }
    }

    do_PicRotary_BilInear_MMX_Block(BlockWork,Src,Ax_16,Ay_16);
}


    inline void PicRotary_ThreeOrder_CopyLine_Fast_MMX(Color32* pDstLine,long width,
                            long Ax_16,long Ay_16,long srcx0_16,long srcy0_16,const TPixels32Ref& SrcPic)
    {
        for (long x=0;x<width;++x)
        {
            ThreeOrder_Fast_MMX(SrcPic,srcx0_16,srcy0_16,&pDstLine[x]);
            srcx0_16+=Ax_16;
            srcy0_16+=Ay_16;
        }
    }
    inline void PicRotary_ThreeOrder_CopyLine_Border_MMX(Color32* pDstLine,long width,
                            long Ax_16,long Ay_16,long srcx0_16,long srcy0_16,const TPixels32Ref& SrcPic)
    {
        for (long x=0;x<width;++x)
        {
            Color32 src_color;
            ThreeOrder_Border_MMX(SrcPic,srcx0_16,srcy0_16,&src_color);
            pDstLine[x]=AlphaBlend_MMX(pDstLine[x],src_color);        
            srcx0_16+=Ax_16;
            srcy0_16+=Ay_16;
        }
    }

    void do_PicRotary_ThreeOrder_MMX_Block(TBlockWork& BlockWork,const TPixels32Ref& Src,long Ax_16,long Ay_16)
    {
        const long rotary_block_width=64;  //128 
        const long rotary_block_height=rotary_block_width;
        long height=BlockWork.size();
        for (long y=0;y<height;y+=rotary_block_height)
        { 
            long cur_block_height;
            if (rotary_block_height<=(height-y))
                cur_block_height=rotary_block_height;
            else 
                cur_block_height=(height-y);

            for (long yi=y;yi<y+cur_block_height;++yi)
            {
                TBlockLineWork* BlockLine=&BlockWork[yi];
                long cur_block_width=BlockLine->width_border0;
                if (cur_block_width>0)
                {
                    PicRotary_ThreeOrder_CopyLine_Border_MMX(BlockLine->pdst,cur_block_width,Ax_16,Ay_16,
                        BlockLine->src_x0_16,BlockLine->src_y0_16,Src);
                    BlockLine->pdst=&BlockLine->pdst[cur_block_width];
                    BlockLine->src_x0_16+=(Ax_16*cur_block_width);
                    BlockLine->src_y0_16+=(Ay_16*cur_block_width);
                }
            }

            bool is_line_filish=false;
            while (!is_line_filish)
            {
                is_line_filish=true;
                for (long yi=y;yi<y+cur_block_height;++yi)
                {
                    TBlockLineWork* BlockLine=&BlockWork[yi];
                    long cur_block_width=BlockLine->width_in;
                    if (cur_block_width>0)
                    {
                        is_line_filish=false;
                        if (cur_block_width>rotary_block_width)
                           cur_block_width=rotary_block_width;
                        PicRotary_ThreeOrder_CopyLine_Fast_MMX(BlockLine->pdst,cur_block_width,Ax_16,Ay_16,
                            BlockLine->src_x0_16,BlockLine->src_y0_16,Src);
                        BlockLine->pdst=&BlockLine->pdst[cur_block_width];
                        BlockLine->width_in-=cur_block_width;
                        BlockLine->src_x0_16+=(Ax_16*cur_block_width);
                        BlockLine->src_y0_16+=(Ay_16*cur_block_width);
                    }
                }
            }

            for (long yi=y;yi<y+cur_block_height;++yi)
            {
                TBlockLineWork* BlockLine=&BlockWork[yi];
                long cur_block_width=BlockLine->width_border1;
                if (cur_block_width>0)
                {
                    PicRotary_ThreeOrder_CopyLine_Border_MMX(BlockLine->pdst,cur_block_width,Ax_16,Ay_16,
                        BlockLine->src_x0_16,BlockLine->src_y0_16,Src);
                    //BlockLine->pdst=&BlockLine->pdst[cur_block_width];
                    //BlockLine->src_x0_16+=(Ax_16*cur_block_width);
                    //BlockLine->src_y0_16+=(Ay_16*cur_block_width);
                }
            }
        }
    
    
        asm  emms
    }

void PicRotaryThreeOrder_MMX_Block(const TPixels32Ref& Dst,const TPixels32Ref& Src,double RotaryAngle,double ZoomX,double ZoomY,double move_x,double move_y)
{
    if ( (fabs(ZoomX*Src.width)<1.0e-4) || (fabs(ZoomY*Src.height)<1.0e-4) ) return; //太小的缩放比例认为已经不可见
    double tmprZoomXY=1.0/(ZoomX*ZoomY);  
    double rZoomX=tmprZoomXY*ZoomY;
    double rZoomY=tmprZoomXY*ZoomX;
    double sinA,cosA;
    SinCos(RotaryAngle,sinA,cosA);
    long Ax_16=(long)(rZoomX*cosA*(1<<16)); 
    long Ay_16=(long)(rZoomX*sinA*(1<<16)); 
    long Bx_16=(long)(-rZoomY*sinA*(1<<16)); 
    long By_16=(long)(rZoomY*cosA*(1<<16)); 
    double rx0=Src.width*0.5;  //(rx0,ry0)为旋转中心 
    double ry0=Src.height*0.5; 
    long Cx_16=(long)((-(rx0+move_x)*rZoomX*cosA+(ry0+move_y)*rZoomY*sinA+rx0)*(1<<16));
    long Cy_16=(long)((-(rx0+move_x)*rZoomX*sinA-(ry0+move_y)*rZoomY*cosA+ry0)*(1<<16)); 

    TRotaryClipData rcData;
    rcData.Ax_16=Ax_16;
    rcData.Bx_16=Bx_16;
    rcData.Cx_16=Cx_16;
    rcData.Ay_16=Ay_16;
    rcData.By_16=By_16;
    rcData.Cy_16=Cy_16;
    rcData.dst_width=Dst.width;
    rcData.dst_height=Dst.height;
    rcData.src_width=Src.width;
    rcData.src_height=Src.height;
    if (!rcData.inti_clip(move_x,move_y,2)) return;

    TBlockWork BlockWork;

    Color32* pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_down_y);
    while (true) //to down
    {
        long y=rcData.out_dst_down_y;
        if (y>=Dst.height) break;
        if (y>=0)
        {
            BlockWork.push_back(TBlockLineWork(&pDstLine[rcData.out_dst_x0_boder],
                rcData.out_dst_x0_in-rcData.out_dst_x0_boder,rcData.out_dst_x1_in-rcData.out_dst_x0_in,
                rcData.out_dst_x1_boder-rcData.out_dst_x1_in,rcData.out_src_x0_16,rcData.out_src_y0_16));
        }
        if (!rcData.next_clip_line_down()) break;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
   
    for (long sleft=0,sright=BlockWork.size()-1;sleft<sright;++sleft,--sright)
        std::swap(BlockWork[sleft],BlockWork[sright]);

    pDstLine=Dst.pdata;
    ((UInt8*&)pDstLine)+=(Dst.byte_width*rcData.out_dst_up_y);
    while (rcData.next_clip_line_up()) //to up 
    {
        long y=rcData.out_dst_up_y;
        if (y<0) break;
        ((UInt8*&)pDstLine)-=Dst.byte_width;
        if (y<Dst.height)
        {
            BlockWork.push_back(TBlockLineWork(&pDstLine[rcData.out_dst_x0_boder],
                rcData.out_dst_x0_in-rcData.out_dst_x0_boder,rcData.out_dst_x1_in-rcData.out_dst_x0_in,
                rcData.out_dst_x1_boder-rcData.out_dst_x1_in,rcData.out_src_x0_16,rcData.out_src_y0_16));
        }
    }

    do_PicRotary_ThreeOrder_MMX_Block(BlockWork,Src,Ax_16,Ay_16);
}

#endif
#endif


//////////////////////////////////////////////////////////////////////////////////////////////////////

int main()
{
    std::cout<<" 请输入回车键开始测试(可以把进程优先级设置为“实时”)> ";
    waitInputChar();
    std::cout<<std::endl;

    //*                                                                                //AMD64X2 4200+ 2.33G
    test("PicRotary0"                         ,PicRotary0                         ,  1,true);//   35.07 FPS
    test("PicRotary1"                         ,PicRotary1                         ,  1,true);//   63.29 FPS
    test("PicRotary2"                         ,PicRotary2                         ,  1,true);//  143.08 FPS
    test("PicRotary3"                         ,PicRotary3                         ,  2,true);//  284.47 FPS
#ifdef MMX_ACTIVE
#ifdef asm   
    test("PicRotarySSE"                       ,PicRotarySSE                       ,  2,true);//  303.16 FPS
    test("PicRotarySSE2"                      ,PicRotarySSE2                      ,  2,true);//  303.16 FPS
    test("PicRotarySSE_Block"                 ,PicRotarySSE_Block                 ,  2,true);//  313.45 FPS
#endif
#endif
 
    test("PicRotaryBilInear"                  ,PicRotaryBilInear                  ,  1,true);//   88.60 FPS
#ifdef MMX_ACTIVE
#ifdef asm   
    test("PicRotaryBilInear_MMX"              ,PicRotaryBilInear_MMX              ,  1,true);//  108.17 FPS
    test("PicRotaryBilInear_MMX_Block"        ,PicRotaryBilInear_MMX_Block        ,  1,true);//  113.53 FPS
    test("PicRotaryBlendBilInear_MMX"         ,PicRotaryBlendBilInear_MMX         ,  1,true);//   90.00 FPS
#endif
#endif

    test("PicRotaryThreeOrder"                ,PicRotaryThreeOrder                ,  1,true);//  23.11 FPS
#ifdef MMX_ACTIVE
#ifdef asm   
    test("PicRotaryThreeOrder_MMX"            ,PicRotaryThreeOrder_MMX            ,  1,true);//  44.48 FPS
    test("PicRotaryThreeOrder_MMX_Block"      ,PicRotaryThreeOrder_MMX_Block      ,  1,true);//  43.31 FPS
#endif
#endif
    //*/



    //testEffect(PicRotary3                 );
    //testEffect(PicRotaryBilInear          );
    testEffect(PicRotaryThreeOrder        );
#ifdef MMX_ACTIVE
#ifdef asm   
    //testEffect(PicRotaryBlendBilInear_MMX ,true);
#endif
#endif



    std::cout<<std::endl<<" 测试完成. ";
    waitInputChar();
    return 0;
}
