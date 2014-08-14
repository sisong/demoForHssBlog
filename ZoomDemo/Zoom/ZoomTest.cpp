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
    typedef    __int64                Int64;
    typedef    unsigned __int64    UInt64;
#else
    #ifdef __GNUC__
        #define must_inline __attribute__((always_inline)) 
    #else
        #define must_inline inline 
    #endif
    typedef    long long            Int64;
    typedef    unsigned long long    UInt64;
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
    sprintf(buff, "%.2f", fps);
    std::string result(buff);
    const long fpsStrBestSize=8;
    if (result.size()<fpsStrBestSize)
        result=std::string(fpsStrBestSize-result.size(),' ')+result;
    return result;
}

typedef void (*TZoomProc)(const TPixels32Ref& dst,const TPixels32Ref& src);

void test(const char* proc_name,const TZoomProc fproc,const long csRunCount,bool isSaveResultPic=false){
    TPixels32 srcPic;
    TPixels32 dstPic;
#if defined(__APPLE__) && defined(__MACH__)
    TFileInputStream bmpInputStream("/Users/hss/Desktop/GraphicDemo/ColorToGray/test1.bmp"); //我的xcode测试目录
#else
    TFileInputStream bmpInputStream("test1.bmp");
#endif
    TBmpFile::load(&bmpInputStream,&srcPic);//加载源图片
    dstPic.resizeFast(1024,768);

    std::cout<<proc_name<<": ";
    clock_t t0=clock();
    for (long c=0;c<csRunCount;++c){
        fproc(dstPic.getRef(),srcPic.getRef());
    }
    t0=clock()-t0;
    double fps=csRunCount/(t0*1.0/CLOCKS_PER_SEC);
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


//////////////////////////////////////////////////////////////////////////////////////////////////////

//Src.pdata指向源数据区,Dst.pdata指向目的数据区
//函数将大小为Src.width*Src.height的图片缩放到Dst.width*Dst.height的区域中
void PicZoom0(const TPixels32Ref& Dst,const TPixels32Ref& Src){
    if (  (0==Dst.width)||(0==Dst.height)
        ||(0==Src.width)||(0==Src.height)) return;

    for (long x=0;x<Dst.width;++x)  {
        for (long y=0;y<Dst.height;++y)
        {
            long srcx=(x*Src.width/Dst.width);
            long srcy=(y*Src.height/Dst.height);
            Dst.pixels(x,y)=Src.pixels(srcx,srcy);
        }
    }
}

void PicZoom1(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(0==Src.width)||(0==Src.height)) return;

    for (long y=0;y<Dst.height;++y)
    {
        for (long x=0;x<Dst.width;++x)
        {
            long srcx=(x*Src.width/Dst.width);
            long srcy=(y*Src.height/Dst.height);
            Dst.pixels(x,y)=Src.pixels(srcx,srcy);
        }
    }
}

void PicZoom2(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(0==Src.width)||(0==Src.height)) return;

    //函数能够处理的最大图片尺寸65536*65536
    long xrIntFloat_16=(Src.width<<16)/Dst.width+1; //16.16格式定点数
    long yrIntFloat_16=(Src.height<<16)/Dst.height+1; //16.16格式定点数
    //可证明: (Dst.width-1)*xrIntFloat_16<Src.width成立

    for (long y=0;y<Dst.height;++y)
    {
        for (long x=0;x<Dst.width;++x)
        {
            unsigned long srcx=(x*xrIntFloat_16)>>16;
            unsigned long srcy=(y*yrIntFloat_16)>>16;
            Dst.pixels(x,y)=Src.pixels(srcx,srcy);
        }
    }
}

void PicZoom3(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(0==Src.width)||(0==Src.height)) return;

    long xrIntFloat_16=(Src.width<<16)/Dst.width+1; 
    long yrIntFloat_16=(Src.height<<16)/Dst.height+1;

    long dst_width=Dst.width;
    Color32* pDstLine=Dst.pdata;
    long srcy_16=0;
    for (long y=0;y<Dst.height;++y)
    {
        Color32* pSrcLine=Src.getLinePixels(srcy_16>>16);
        long srcx_16=0;
        for (long x=0;x<dst_width;++x)
        {
            pDstLine[x]=pSrcLine[srcx_16>>16];
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
}

#ifdef asm

void PicZoom3_asm(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(0==Src.width)||(0==Src.height)) return;

    long xrIntFloat_16=(Src.width<<16)/Dst.width+1; 
    long yrIntFloat_16=(Src.height<<16)/Dst.height+1;

    long dst_width=Dst.width;
    Color32* pDstLine=Dst.pdata;
    long srcy_16=0;
    for (long y=0;y<Dst.height;++y)
    {
        Color32* pSrcLine=Src.getLinePixels(srcy_16>>16);
        //把内部的循环改写为汇编实现
        asm
        {
              xor       edx,edx          //srcx_16=0
              mov       ecx,dst_width
              mov       esi,pSrcLine
              mov       edi,pDstLine
              mov       eax,xrIntFloat_16
              lea       edi,[edi+ecx*4]
              neg       ecx

        loop_start:
              mov       ebx,edx
              shr       ebx,16            //srcx_16>>16
              mov       ebx,[esi+ebx*4]
              mov       [edi+ecx*4],ebx

              add       edx,eax           //srcy_16+=yrIntFloat_16
              inc       ecx
              jnz       loop_start
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
}

void PicZoom3_float(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(0==Src.width)||(0==Src.height)) return;

    double xrdouble=1.000000001*Src.width/Dst.width;
    double yrdouble=1.000000001*Src.height/Dst.height;

    unsigned short RC_Old;
    unsigned short RC_Edit;
    asm  //设置FPU的取整方式  为了直接使用fist浮点指令
    {
        FNSTCW  RC_Old           // 保存协处理器控制字,用来恢复
        FNSTCW  RC_Edit          // 保存协处理器控制字,用来修改
        FWAIT
        OR      RC_Edit, 0x0F00  // 使RC场向零取整     改为 RC=11
        FLDCW   RC_Edit          // 载入协处理器控制字,RC场已经修改
    }

    long dst_width=Dst.width;
    Color32* pDstLine=Dst.pdata;
    double srcy=0;
    for (long y=0;y<Dst.height;++y)
    {
        Color32* pSrcLine=((Color32*)((UInt8*)Src.pdata+Src.byte_width*((long)srcy)));
        //double srcx=0;
        //for (unsigned long x=0;x<dst_width;++x)
        //{
        //    pDstLine[x]=pSrcLine[(long)srcx];//浮点取整很慢!
        //    srcx+=xrdouble;
        //}
        asm fld       xrdouble            //st0==xrdouble
        asm fldz                         //st0==0   st1==xrdouble
        unsigned long srcx=0;
        for (long x=0;x<dst_width;++x)
        {
            asm fist dword ptr srcx     //srcx=(long)st0 //优化的浮点取整
            pDstLine[x]=pSrcLine[srcx];
            asm fadd  st,st(1)        //st0+=st1   st1==xrdouble
        }
        asm fstp      st
        asm fstp      st

        srcy+=yrdouble;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }

    asm  //恢复FPU的取整方式
    {
        FWAIT
        FLDCW   RC_Old 
    }
}
#endif


void PicZoom3_Table(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(0==Src.width)||(0==Src.height)) return;

    long dst_width=Dst.width;

    long* SrcX_Table = new long[dst_width];
    for (long x=0;x<dst_width;++x)//生成表 SrcX_Table
    {
        SrcX_Table[x]=(x*Src.width/Dst.width);
    }

    Color32* pDstLine=Dst.pdata;
    for (long y=0;y<Dst.height;++y)
    {
        long srcy=(y*Src.height/Dst.height);
        Color32* pSrcLine=Src.getLinePixels(srcy);;
        for (long x=0;x<dst_width;++x)
            pDstLine[x]=pSrcLine[SrcX_Table[x]];
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }

    delete [] SrcX_Table;
}

void CreateZoomTable_OpMul(const long SrcLength,const long DstLength,long* ZoomTable)
{
    long delta_src=SrcLength;
    long delta_dst=DstLength;

    long inc_src,inc_dst;
    if (delta_src>0) 
        inc_src=1; 
    else if (delta_src<0) {
        delta_src=-delta_src; 
        inc_src=-1; 
    } else  
        inc_src=0;
    if (delta_dst>0) 
        inc_dst=1; 
    else if (delta_dst<0) {
        delta_dst=-delta_dst; 
        inc_dst=-1; 
    }else
        inc_dst=0;

    long MaxLength;
    if (delta_src>=delta_dst)
        MaxLength=delta_src; 
    else 
        MaxLength=delta_dst;

    long SrcPos=0;
    long DstPos=0;
    long mSrcPos=0;
    long mDstPos=0;
    for (long i=0;i<MaxLength;++i) {
        ZoomTable[SrcPos]=DstPos;
        mSrcPos+=delta_src;
        if (mSrcPos>=MaxLength) { 
            mSrcPos-=MaxLength; 
            DstPos+=inc_dst; 
        }
        mDstPos+=delta_dst;
        if (mDstPos>=MaxLength) { 
            mDstPos-=MaxLength; 
            SrcPos+=inc_src;
        }
    }
}

//这个版本在除法和乘法比较昂贵的CPU上可能效果不错 整个实现稍加改动可以做到不用任何乘除法指令
void PicZoom3_Table_OpMul(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(0==Src.width)||(0==Src.height)) return;

    long dst_width=Dst.width;

    long* SrcX_Table = new long[dst_width];
    CreateZoomTable_OpMul(Src.width,Dst.width,SrcX_Table);//生成表 SrcX_Table
    long* SrcY_Table = new long[Dst.height];
    CreateZoomTable_OpMul(Src.height,Dst.height,SrcY_Table);//生成表 SrcX_Table

    Color32* pDstLine=Dst.pdata;
    for (long y=0;y<Dst.height;++y)
    {
        long srcy=SrcY_Table[y];
        Color32* pSrcLine=Src.getLinePixels(srcy);//做些改动也可以去除这里隐含的一个乘法
        for (long x=0;x<dst_width;++x)
            pDstLine[x]=pSrcLine[SrcX_Table[x]];
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }

    delete [] SrcY_Table;
    delete [] SrcX_Table;
}


#ifdef MMX_ACTIVE
#ifdef asm

void PicZoom3_SSE(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    //警告: 函数需要CPU支持MMX和movntq指令

    if (  (0==Dst.width)||(0==Dst.height)
        ||(0==Src.width)||(0==Src.height)) return;

    long xrIntFloat_16=(Src.width<<16)/Dst.width+1; 
    long yrIntFloat_16=(Src.height<<16)/Dst.height+1;

    long dst_width=Dst.width;
    Color32* pDstLine=Dst.pdata;
    long srcy_16=0;
    for (long y=0;y<Dst.height;++y)
    {
        Color32* pSrcLine=Src.getLinePixels(srcy_16>>16);

        asm
        {
            mov       esi,pSrcLine
            mov       edi,pDstLine
            mov       edx,xrIntFloat_16
            mov       ecx,dst_width
            push      ebp
            xor       ebp,ebp           //srcx_16=0

            and    ecx, (not 3)    //循环4次展开
            test   ecx,ecx   //nop
            jle    EndWriteLoop

            lea       edi,[edi+ecx*4]
            neg       ecx

                WriteLoop:
                        mov       eax,ebp
                        shr       eax,16            //srcx_16>>16
                        lea       ebx,[ebp+edx]
                        MOVD      mm0,[esi+eax*4]
                        shr       ebx,16            //srcx_16>>16
                        PUNPCKlDQ mm0,[esi+ebx*4]
                        lea       ebp,[ebp+edx*2]
                       
                        MOVNTQ qword ptr [edi+ecx*4], mm0  //不使用缓存的写入指令
                        //asm _emit 0x0F asm _emit 0xE7 asm _emit 0x04 asm _emit 0x8F  

                        mov       eax,ebp
                        shr       eax,16            //srcx_16>>16
                        lea       ebx,[ebp+edx]
                        MOVD      mm1,[esi+eax*4]
                        shr       ebx,16            //srcx_16>>16
                        PUNPCKlDQ mm1,[esi+ebx*4]
                        lea       ebp,[ebp+edx*2]
                        
                        movntq qword ptr [edi+ecx*4+8], mm1 //不使用缓存的写入指令
                        //asm _emit 0x0F asm _emit 0xE7 asm _emit 0x4C asm _emit 0x8F asm _emit 0x08

                        add ecx, 4
                        jnz WriteLoop

                EndWriteLoop:

            mov    ebx,ebp
            pop    ebp

            //处理边界  循环次数为0,1,2,3；(这个循环可以展开,做一个跳转表,略)
            mov    ecx,dst_width
            and    ecx,3
            test   ecx,ecx
            jle    EndLineZoom
                lea       edi,[edi+ecx*4]
                neg       ecx
          StartBorder:
                mov       eax,ebx
                shr       eax,16            //srcx_16>>16
                mov       eax,[esi+eax*4]
                mov       [edi+ecx*4],eax
                add       ebx,edx
            inc       ECX
            JNZ       StartBorder
      EndLineZoom:
            sfence //刷新写入
            emms //MMX使用结束
        }

        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
}

void PicZoom3_SSE_prefetch(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    //警告: 函数需要CPU支持MMX和movntq\prefetchnta指令

    if (  (0==Dst.width)||(0==Dst.height)
        ||(0==Src.width)||(0==Src.height)) return;

    long xrIntFloat_16=(Src.width<<16)/Dst.width+1; 
    long yrIntFloat_16=(Src.height<<16)/Dst.height+1;

    long dst_width=Dst.width;
    Color32* pDstLine=Dst.pdata;
    long srcy_16=0;
    for (long y=0;y<Dst.height;++y)
    {
        Color32* pSrcLine=Src.getLinePixels(srcy_16>>16);

        asm
        {
            mov       esi,pSrcLine
            mov       edi,pDstLine
            mov       edx,xrIntFloat_16
            mov       ecx,dst_width
            push      ebp
            xor       ebp,ebp           //srcx_16=0

            and    ecx, (not 3)    //循环4次展开
            test   ecx,ecx   //nop
            jle    EndWriteLoop

            lea       edi,[edi+ecx*4]
            neg       ecx

                WriteLoop:
                        prefetchnta [esi+eax*4+256*4] //预读方案   提示:在intel的CPU上不一定更快,没有测试

                        mov       eax,ebp
                        shr       eax,16            //srcx_16>>16
                        lea       ebx,[ebp+edx]
                        MOVD      mm0,[esi+eax*4]
                        shr       ebx,16            //srcx_16>>16
                        PUNPCKlDQ mm0,[esi+ebx*4]
                        lea       ebp,[ebp+edx*2]
                       
                        MOVNTQ qword ptr [edi+ecx*4], mm0  //不使用缓存的写入指令
                        //asm _emit 0x0F asm _emit 0xE7 asm _emit 0x04 asm _emit 0x8F  

                        mov       eax,ebp
                        shr       eax,16            //srcx_16>>16
                        lea       ebx,[ebp+edx]
                        MOVD      mm1,[esi+eax*4]
                        shr       ebx,16            //srcx_16>>16
                        PUNPCKlDQ mm1,[esi+ebx*4]
                        lea       ebp,[ebp+edx*2]
                        
                        movntq qword ptr [edi+ecx*4+8], mm1 //不使用缓存的写入指令
                        //asm _emit 0x0F asm _emit 0xE7 asm _emit 0x4C asm _emit 0x8F asm _emit 0x08

                        add ecx, 4
                        jnz WriteLoop

                EndWriteLoop:

            mov    ebx,ebp
            pop    ebp

            //处理边界  循环次数为0,1,2,3；(这个循环可以展开,做一个跳转表,略)
            mov    ecx,dst_width
            and    ecx,3
            test   ecx,ecx
            jle    EndLineZoom
                lea       edi,[edi+ecx*4]
                neg       ecx
          StartBorder:
                mov       eax,ebx
                shr       eax,16            //srcx_16>>16
                mov       eax,[esi+eax*4]
                mov       [edi+ecx*4],eax
                add       ebx,edx
            inc       ECX
            JNZ       StartBorder
      EndLineZoom:
            sfence //刷新写入
            emms //MMX使用结束
        }

        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
}

#endif


//编译器不支持MMX等的话 请关闭MMX_ACTIVE的定义
#include <mmintrin.h>   //mmx
//#include <mm3dnow.h>    //3dnow
#include <xmmintrin.h>  //sse
//#include <emmintrin.h>  //sse2
//#include <pmmintrin.h>  //sse3
//#include <tmmintrin.h>  //ssse3
//#include <intrin.h>     //sse4a
//#include <smmintrin.h>  //sse4.1
//#include <nmmintrin.h>  //sse4.2

//----------------------------------

void PicZoom3_SSE_mmh(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    //警告: 函数需要CPU支持MMX和movntq指令

    if (  (0==Dst.width)||(0==Dst.height)
        ||(0==Src.width)||(0==Src.height)) return;

    long xrIntFloat_16=(Src.width<<16)/Dst.width+1; 
    long yrIntFloat_16=(Src.height<<16)/Dst.height+1;

    long dst_width=Dst.width;
    Color32* pDstLine=Dst.pdata;
    long srcy_16=0;
    long for4count=dst_width/4*4;
    for (long y=0;y<Dst.height;++y)
    {
        Color32* pSrcLine=Src.getLinePixels(srcy_16>>16);
        long srcx_16=0;
        long x;
        for (x=0;x<for4count;x+=4)//循环4次展开
        {
            __m64 m0=_m_from_int(*(int*)(&pSrcLine[srcx_16>>16]));
            srcx_16+=xrIntFloat_16;
            m0=_m_punpckldq(m0, _m_from_int(*(int*)(&pSrcLine[srcx_16>>16])) );
            srcx_16+=xrIntFloat_16;
            __m64 m1=_m_from_int(*(int*)(&pSrcLine[srcx_16>>16]));
            srcx_16+=xrIntFloat_16;
            m1=_m_punpckldq(m1, _m_from_int(*(int*)(&pSrcLine[srcx_16>>16])) );
            srcx_16+=xrIntFloat_16;
            _mm_stream_pi((__m64 *)&pDstLine[x],m0); //不使用缓存的写入指令
            _mm_stream_pi((__m64 *)&pDstLine[x+2],m1); //不使用缓存的写入指令
        }
        for (x=for4count;x<dst_width;++x)//处理边界
        {
            pDstLine[x]=pSrcLine[srcx_16>>16];
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
    _m_empty();
}

#endif


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 


    must_inline void Bilinear0(const TPixels32Ref& pic,double fx,double fy,Color32* result)
    {
        long x=(long)fx; if (x>fx) --x; //x=floor(fx);    
        long y=(long)fy; if (y>fy) --y; //y=floor(fy);
        
        Color32 Color0=pic.getPixelsBorder(x,y);
        Color32 Color2=pic.getPixelsBorder(x+1,y);
        Color32 Color1=pic.getPixelsBorder(x,y+1);
        Color32 Color3=pic.getPixelsBorder(x+1,y+1);

        double u=fx-x;
        double v=fy-y;
        double pm3=u*v;
        double pm2=u*(1-v);
        double pm1=v*(1-u);
        double pm0=(1-u)*(1-v);

        result->a=(UInt8)(pm0*Color0.a+pm1*Color1.a+pm2*Color2.a+pm3*Color3.a);
        result->r=(UInt8)(pm0*Color0.r+pm1*Color1.r+pm2*Color2.r+pm3*Color3.r);
        result->g=(UInt8)(pm0*Color0.g+pm1*Color1.g+pm2*Color2.g+pm3*Color3.g);
        result->b=(UInt8)(pm0*Color0.b+pm1*Color1.b+pm2*Color2.b+pm3*Color3.b);
    }

void PicZoom_Bilinear0(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(0==Src.width)||(0==Src.height)) return;

    long dst_width=Dst.width;
    Color32* pDstLine=Dst.pdata;
    for (long y=0;y<Dst.height;++y)
    {
        double srcy=(y+0.4999999)*Src.height/Dst.height-0.5;
        for (long x=0;x<dst_width;++x)
        {
            double srcx=(x+0.4999999)*Src.width/Dst.width-0.5;
            Bilinear0(Src,srcx,srcy,&pDstLine[x]);
        }
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
}



    must_inline void Bilinear1(const TPixels32Ref& pic,const long x_16,const long y_16,Color32* result)
    {
        long x=x_16>>16;
        long y=y_16>>16;
        Color32 Color0=pic.getPixelsBorder(x,y);
        Color32 Color2=pic.getPixelsBorder(x+1,y);
        Color32 Color1=pic.getPixelsBorder(x,y+1);
        Color32 Color3=pic.getPixelsBorder(x+1,y+1);

        unsigned long u_8=(x_16 & 0xFFFF)>>8;
        unsigned long v_8=(y_16 & 0xFFFF)>>8;
        unsigned long pm3_16=(u_8*v_8);
        unsigned long pm2_16=(u_8*(unsigned long)(256-v_8));
        unsigned long pm1_16=(v_8*(unsigned long)(256-u_8));
        unsigned long pm0_16=((256-u_8)*(256-v_8));

        result->a=(UInt8)((pm0_16*Color0.a+pm1_16*Color1.a+pm2_16*Color2.a+pm3_16*Color3.a)>>16);
        result->r=(UInt8)((pm0_16*Color0.r+pm1_16*Color1.r+pm2_16*Color2.r+pm3_16*Color3.r)>>16);
        result->g=(UInt8)((pm0_16*Color0.g+pm1_16*Color1.g+pm2_16*Color2.g+pm3_16*Color3.g)>>16);
        result->b=(UInt8)((pm0_16*Color0.b+pm1_16*Color1.b+pm2_16*Color2.b+pm3_16*Color3.b)>>16);
    }

void PicZoom_Bilinear1(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(0==Src.width)||(0==Src.height)) return;

    long xrIntFloat_16=((Src.width)<<16)/Dst.width+1; 
    long yrIntFloat_16=((Src.height)<<16)/Dst.height+1;
    const long csDErrorX=-(1<<15)+(xrIntFloat_16>>1);
    const long csDErrorY=-(1<<15)+(yrIntFloat_16>>1);

    long dst_width=Dst.width;

    Color32* pDstLine=Dst.pdata;
    long srcy_16=csDErrorY;
    long y;
    for (y=0;y<Dst.height;++y)
    {
        long srcx_16=csDErrorX;
        for (long x=0;x<dst_width;++x)
        {
            Bilinear1(Src,srcx_16,srcy_16,&pDstLine[x]); //border
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
} 


///////////

    must_inline void Bilinear2_Fast(Color32* PColor0,Color32* PColor1,unsigned long u_8,unsigned long v_8,Color32* result)
    {
        unsigned long pm3_16=u_8*v_8;
        unsigned long pm2_16=(u_8<<8)-pm3_16;
        unsigned long pm1_16=(v_8<<8)-pm3_16;
        unsigned long pm0_16=(1<<16)-pm1_16-pm2_16-pm3_16;
   
        result->a=(UInt8)((pm0_16*PColor0[0].a+pm2_16*PColor0[1].a+pm1_16*PColor1[0].a+pm3_16*PColor1[1].a)>>16);
        result->r=(UInt8)((pm0_16*PColor0[0].r+pm2_16*PColor0[1].r+pm1_16*PColor1[0].r+pm3_16*PColor1[1].r)>>16);
        result->g=(UInt8)((pm0_16*PColor0[0].g+pm2_16*PColor0[1].g+pm1_16*PColor1[0].g+pm3_16*PColor1[1].g)>>16);
        result->b=(UInt8)((pm0_16*PColor0[0].b+pm2_16*PColor0[1].b+pm1_16*PColor1[0].b+pm3_16*PColor1[1].b)>>16);
    }

    inline void Bilinear2_Border(const TPixels32Ref& pic,const long x_16,const long y_16,Color32* result)
    {
        long x=(x_16>>16);
        long y=(y_16>>16);
        unsigned long u_16=((unsigned short)(x_16));
        unsigned long v_16=((unsigned short)(y_16));

        Color32 pixel[4];
        pixel[0]=pic.getPixelsBorder(x,y);
        pixel[1]=pic.getPixelsBorder(x+1,y);
        pixel[2]=pic.getPixelsBorder(x,y+1);
        pixel[3]=pic.getPixelsBorder(x+1,y+1);
        
        Bilinear2_Fast(&pixel[0],&pixel[2],u_16>>8,v_16>>8,result);
    }

void PicZoom_Bilinear2(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(0==Src.width)||(0==Src.height)) return;

    long xrIntFloat_16=((Src.width)<<16)/Dst.width+1; 
    long yrIntFloat_16=((Src.height)<<16)/Dst.height+1;
    const long csDErrorX=-(1<<15)+(xrIntFloat_16>>1);
    const long csDErrorY=-(1<<15)+(yrIntFloat_16>>1);

    long dst_width=Dst.width;

    //计算出需要特殊处理的边界
    long border_y0=-csDErrorY/yrIntFloat_16+1;              //y0+y*yr>=0; y0=csDErrorY => y>=-csDErrorY/yr
    if (border_y0>=Dst.height) border_y0=Dst.height;
    long border_x0=-csDErrorX/xrIntFloat_16+1;     
    if (border_x0>=Dst.width ) border_x0=Dst.width; 
    long border_y1=(((Src.height-2)<<16)-csDErrorY)/yrIntFloat_16+1; //y0+y*yr<=(height-2) => y<=(height-2-csDErrorY)/yr
    if (border_y1<border_y0) border_y1=border_y0;
    long border_x1=(((Src.width-2)<<16)-csDErrorX)/xrIntFloat_16+1; 
    if (border_x1<border_x0) border_x1=border_x0;

    Color32* pDstLine=Dst.pdata;
    long Src_byte_width=Src.byte_width;
    long srcy_16=csDErrorY;
    long y;
    for (y=0;y<border_y0;++y)
    {
        long srcx_16=csDErrorX;
        for (long x=0;x<dst_width;++x)
        {
            Bilinear2_Border(Src,srcx_16,srcy_16,&pDstLine[x]); //border
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
    for (y=border_y0;y<border_y1;++y)
    {
        long srcx_16=csDErrorX;
        long x;
        for (x=0;x<border_x0;++x)
        {
            Bilinear2_Border(Src,srcx_16,srcy_16,&pDstLine[x]);//border
            srcx_16+=xrIntFloat_16;
        }

        {
            unsigned long v_8=(srcy_16 & 0xFFFF)>>8;
            Color32* PSrcLineColor= (Color32*)((UInt8*)(Src.pdata)+Src_byte_width*(srcy_16>>16)) ;
            for (long x=border_x0;x<border_x1;++x)
            {
                Color32* PColor0=&PSrcLineColor[srcx_16>>16];
                Color32* PColor1=(Color32*)((UInt8*)(PColor0)+Src_byte_width);
                Bilinear2_Fast(PColor0,PColor1,(srcx_16 & 0xFFFF)>>8,v_8,&pDstLine[x]);
                srcx_16+=xrIntFloat_16;
            }
        }

        for (x=border_x1;x<dst_width;++x)
        {
            Bilinear2_Border(Src,srcx_16,srcy_16,&pDstLine[x]);//border
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
    for (y=border_y1;y<Dst.height;++y)
    {
        long srcx_16=csDErrorX;
        for (long x=0;x<dst_width;++x)
        {
            Bilinear2_Border(Src,srcx_16,srcy_16,&pDstLine[x]); //border
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
}


    must_inline void Bilinear_Fast_Common(Color32* PColor0,Color32* PColor1,unsigned long u_8,unsigned long v_8,Color32* result)
    {
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

        *(unsigned long*)(result)=(GA & 0xFF00FF00)|((BR & 0xFF00FF00)>>8);
    }

    inline void Bilinear_Border_Common(const TPixels32Ref& pic,const long x_16,const long y_16,Color32* result)
    {
        long x=(x_16>>16);
        long y=(y_16>>16);
        unsigned long u_16=((unsigned short)(x_16));
        unsigned long v_16=((unsigned short)(y_16));

        Color32 pixel[4];
        pixel[0]=pic.getPixelsBorder(x,y);
        pixel[1]=pic.getPixelsBorder(x+1,y);
        pixel[2]=pic.getPixelsBorder(x,y+1);
        pixel[3]=pic.getPixelsBorder(x+1,y+1);
        
        Bilinear_Fast_Common(&pixel[0],&pixel[2],u_16>>8,v_16>>8,result);
    }

void PicZoom_Bilinear_Common(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(0==Src.width)||(0==Src.height)) return;

    long xrIntFloat_16=((Src.width)<<16)/Dst.width+1; 
    long yrIntFloat_16=((Src.height)<<16)/Dst.height+1;
    const long csDErrorX=-(1<<15)+(xrIntFloat_16>>1);
    const long csDErrorY=-(1<<15)+(yrIntFloat_16>>1);

    long dst_width=Dst.width;

    //计算出需要特殊处理的边界
    long border_y0=-csDErrorY/yrIntFloat_16+1;              //y0+y*yr>=0; y0=csDErrorY => y>=-csDErrorY/yr
    if (border_y0>=Dst.height) border_y0=Dst.height;
    long border_x0=-csDErrorX/xrIntFloat_16+1;     
    if (border_x0>=Dst.width ) border_x0=Dst.width; 
    long border_y1=(((Src.height-2)<<16)-csDErrorY)/yrIntFloat_16+1; //y0+y*yr<=(height-2) => y<=(height-2-csDErrorY)/yr
    if (border_y1<border_y0) border_y1=border_y0;
    long border_x1=(((Src.width-2)<<16)-csDErrorX)/xrIntFloat_16+1; 
    if (border_x1<border_x0) border_x1=border_x0;

    Color32* pDstLine=Dst.pdata;
    long Src_byte_width=Src.byte_width;
    long srcy_16=csDErrorY;
    long y;
    for (y=0;y<border_y0;++y)
    {
        long srcx_16=csDErrorX;
        for (long x=0;x<dst_width;++x)
        {
            Bilinear_Border_Common(Src,srcx_16,srcy_16,&pDstLine[x]); //border
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
    for (y=border_y0;y<border_y1;++y)
    {
        long srcx_16=csDErrorX;
        long x;
        for (x=0;x<border_x0;++x)
        {
            Bilinear_Border_Common(Src,srcx_16,srcy_16,&pDstLine[x]);//border
            srcx_16+=xrIntFloat_16;
        }

        {
            unsigned long v_8=(srcy_16 & 0xFFFF)>>8;
            Color32* PSrcLineColor= (Color32*)((UInt8*)(Src.pdata)+Src_byte_width*(srcy_16>>16)) ;
            for (long x=border_x0;x<border_x1;++x)
            {
                Color32* PColor0=&PSrcLineColor[srcx_16>>16];
                Color32* PColor1=(Color32*)((UInt8*)(PColor0)+Src_byte_width);
                Bilinear_Fast_Common(PColor0,PColor1,(srcx_16 & 0xFFFF)>>8,v_8,&pDstLine[x]);
                srcx_16+=xrIntFloat_16;
            }
        }

        for (x=border_x1;x<dst_width;++x)
        {
            Bilinear_Border_Common(Src,srcx_16,srcy_16,&pDstLine[x]);//border
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
    for (y=border_y1;y<Dst.height;++y)
    {
        long srcx_16=csDErrorX;
        for (long x=0;x<dst_width;++x)
        {
            Bilinear_Border_Common(Src,srcx_16,srcy_16,&pDstLine[x]); //border
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
}


//----------------------------

#ifdef MMX_ACTIVE
#ifdef asm

    must_inline void  Bilinear_Fast_MMX(Color32* PColor0,Color32* PColor1,unsigned long u_8,unsigned long v_8,Color32* result)
    {
        asm
        {    
              MOVD      MM6,v_8
              MOVD      MM5,u_8
              mov       edx,PColor0
              mov       eax,PColor1
              PXOR      mm7,mm7

              MOVD         MM2,dword ptr [eax]  
              MOVD         MM0,dword ptr [eax+4]
              PUNPCKLWD    MM5,MM5
              PUNPCKLWD    MM6,MM6
              MOVD         MM3,dword ptr [edx]  
              MOVD         MM1,dword ptr [edx+4]
              PUNPCKLDQ    MM5,MM5 
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
              PUNPCKLDQ    MM6,MM6 
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

    void Bilinear_Border_MMX(const TPixels32Ref& pic,const long x_16,const long y_16,Color32* result)
    {
        long x=(x_16>>16);
        long y=(y_16>>16);
        unsigned long u_16=((unsigned short)(x_16));
        unsigned long v_16=((unsigned short)(y_16));

        Color32 pixel[4];
        pixel[0]=pic.getPixelsBorder(x,y);
        pixel[1]=pic.getPixelsBorder(x+1,y);
        pixel[2]=pic.getPixelsBorder(x,y+1);
        pixel[3]=pic.getPixelsBorder(x+1,y+1);
        
        Bilinear_Fast_MMX(&pixel[0],&pixel[2],u_16>>8,v_16>>8,result);
    }

void PicZoom_Bilinear_MMX(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(0==Src.width)||(0==Src.height)) return;

    long xrIntFloat_16=((Src.width)<<16)/Dst.width+1; 
    long yrIntFloat_16=((Src.height)<<16)/Dst.height+1;
    const long csDErrorX=-(1<<15)+(xrIntFloat_16>>1);
    const long csDErrorY=-(1<<15)+(yrIntFloat_16>>1);

    long dst_width=Dst.width;

    //计算出需要特殊处理的边界
    long border_y0=-csDErrorY/yrIntFloat_16+1;              //y0+y*yr>=0; y0=csDErrorY => y>=-csDErrorY/yr
    if (border_y0>=Dst.height) border_y0=Dst.height;
    long border_x0=-csDErrorX/xrIntFloat_16+1;     
    if (border_x0>=Dst.width ) border_x0=Dst.width; 
    long border_y1=(((Src.height-2)<<16)-csDErrorY)/yrIntFloat_16+1; //y0+y*yr<=(height-2) => y<=(height-2-csDErrorY)/yr
    if (border_y1<border_y0) border_y1=border_y0;
    long border_x1=(((Src.width-2)<<16)-csDErrorX)/xrIntFloat_16+1; 
    if (border_x1<border_x0) border_x1=border_x0;

    Color32* pDstLine=Dst.pdata;
    long Src_byte_width=Src.byte_width;
    long srcy_16=csDErrorY;
    long y;
    for (y=0;y<border_y0;++y)
    {
        long srcx_16=csDErrorX;
        for (long x=0;x<dst_width;++x)
        {
            Bilinear_Border_MMX(Src,srcx_16,srcy_16,&pDstLine[x]); //border
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
    for (y=border_y0;y<border_y1;++y)
    {
        long srcx_16=csDErrorX;
        long x;
        for (x=0;x<border_x0;++x)
        {
            Bilinear_Border_MMX(Src,srcx_16,srcy_16,&pDstLine[x]);//border
            srcx_16+=xrIntFloat_16;
        }

        {
            unsigned long v_8=(srcy_16 & 0xFFFF)>>8;
            Color32* PSrcLineColor= (Color32*)((UInt8*)(Src.pdata)+Src_byte_width*(srcy_16>>16)) ;
            for (long x=border_x0;x<border_x1;++x)
            {
                Color32* PColor0=&PSrcLineColor[srcx_16>>16];
                Color32* PColor1=(Color32*)((UInt8*)(PColor0)+Src_byte_width);
                Bilinear_Fast_MMX(PColor0,PColor1,(srcx_16 & 0xFFFF)>>8,v_8,&pDstLine[x]);
                srcx_16+=xrIntFloat_16;
            }
        }

        for (x=border_x1;x<dst_width;++x)
        {
            Bilinear_Border_MMX(Src,srcx_16,srcy_16,&pDstLine[x]);//border
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
    for (y=border_y1;y<Dst.height;++y)
    {
        long srcx_16=csDErrorX;
        for (long x=0;x<dst_width;++x)
        {
            Bilinear_Border_MMX(Src,srcx_16,srcy_16,&pDstLine[x]); //border
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
    asm emms
}


void PicZoom_Bilinear_MMX_Ex(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(0==Src.width)||(0==Src.height)) return;

    long xrIntFloat_16=((Src.width)<<16)/Dst.width+1; 
    long yrIntFloat_16=((Src.height)<<16)/Dst.height+1;
    const long csDErrorX=-(1<<15)+(xrIntFloat_16>>1);
    const long csDErrorY=-(1<<15)+(yrIntFloat_16>>1);

    long dst_width=Dst.width;

    //计算出需要特殊处理的边界
    long border_y0=-csDErrorY/yrIntFloat_16+1;              //y0+y*yr>=0; y0=csDErrorY => y>=-csDErrorY/yr
    if (border_y0>=Dst.height) border_y0=Dst.height;
    long border_x0=-csDErrorX/xrIntFloat_16+1;     
    if (border_x0>=Dst.width ) border_x0=Dst.width; 
    long border_y1=(((Src.height-2)<<16)-csDErrorY)/yrIntFloat_16+1; //y0+y*yr<=(height-2) => y<=(height-2-csDErrorY)/yr
    if (border_y1<border_y0) border_y1=border_y0;
    long border_x1=(((Src.width-2)<<16)-csDErrorX)/xrIntFloat_16+1; 
    if (border_x1<border_x0) border_x1=border_x0;

    Color32* pDstLine=Dst.pdata;
    long Src_byte_width=Src.byte_width;
    long srcy_16=csDErrorY;
    long y;
    for (y=0;y<border_y0;++y)
    {
        long srcx_16=csDErrorX;
        for (long x=0;x<dst_width;++x)
        {
            Bilinear_Border_MMX(Src,srcx_16,srcy_16,&pDstLine[x]); //border
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }

    for (y=border_y0;y<border_y1;++y)
    {
        long srcx_16=csDErrorX;
        long x;
        for (x=0;x<border_x0;++x)
        {
            Bilinear_Border_MMX(Src,srcx_16,srcy_16,&pDstLine[x]);//border
            srcx_16+=xrIntFloat_16;
        }

        {
            long dst_width_fast=border_x1-border_x0;
            if (dst_width_fast>0)
            {
                unsigned long v_8=(srcy_16 & 0xFFFF)>>8;
                Color32* PSrcLineColor= (Color32*)((UInt8*)(Src.pdata)+Src_byte_width*(srcy_16>>16)) ;
                Color32* PSrcLineColorNext= (Color32*)((UInt8*)(PSrcLineColor)+Src_byte_width) ;
                Color32* pDstLine_Fast=&pDstLine[border_x0];
                asm
                {
                      movd         mm6,v_8
                      pxor         mm7,mm7 //mm7=0
                      PUNPCKLWD    MM6,MM6
                      PUNPCKLDQ    MM6,MM6//mm6=v_8
                    
                      mov       esi,PSrcLineColor
                      mov       ecx,PSrcLineColorNext
                      mov       edx,srcx_16
                      mov       ebx,dst_width_fast
                      mov       edi,pDstLine_Fast
                      lea       edi,[edi+ebx*4]
                      push      ebp
                      mov       ebp,xrIntFloat_16
                      neg       ebx

                loop_start:

                          mov       eax,edx
                          shl       eax,16
                          shr       eax,24
                          //== movzx       eax,dh  //eax=u_8
                          MOVD      MM5,eax
                          mov       eax,edx
                          shr       eax,16     //srcx_16>>16

                          MOVD         MM2,dword ptr [ecx+eax*4]  
                          MOVD         MM0,dword ptr [ecx+eax*4+4]
                          PUNPCKLWD    MM5,MM5
                          MOVD         MM3,dword ptr [esi+eax*4]  
                          MOVD         MM1,dword ptr [esi+eax*4+4]
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
                          PADDw        MM0,MM2
                          PADDw        MM1,MM3

                          PSRLw        MM0,8
                          PSRLw        MM1,8
                          PSUBw        MM0,MM1
                          PSLLw        MM1,8
                          PMULlw       MM0,MM6
                          PADDw        MM0,MM1

                          PSRLw     MM0,8
                          PACKUSwb  MM0,MM7
                          MOVd   dword ptr    [edi+ebx*4],MM0 //write DstColor
                                      
                          add       edx,ebp //srcx_16+=xrIntFloat_16
                          inc       ebx
                          jnz       loop_start

                      pop       ebp
                      mov       srcx_16,edx
                }
            }
        }

        for (x=border_x1;x<dst_width;++x)
        {
            Bilinear_Border_MMX(Src,srcx_16,srcy_16,&pDstLine[x]);//border
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
    for (y=border_y1;y<Dst.height;++y)
    {
        long srcx_16=csDErrorX;
        for (long x=0;x<dst_width;++x)
        {
            Bilinear_Border_MMX(Src,srcx_16,srcy_16,&pDstLine[x]); //border
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
    asm emms
}






#endif
#endif

//===============================================================================================================================

void PicZoom_ftBilinear_Common(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(2>Src.width)||(2>Src.height)) return;

    long xrIntFloat_16=((Src.width-1)<<16)/Dst.width; 
    long yrIntFloat_16=((Src.height-1)<<16)/Dst.height;

    long dst_width=Dst.width;
    long Src_byte_width=Src.byte_width;
    Color32* pDstLine=Dst.pdata;
    long srcy_16=0;
    for (long y=0;y<Dst.height;++y)
    {
        unsigned long v_8=(srcy_16 & 0xFFFF)>>8;
        Color32* PSrcLineColor= (Color32*)((UInt8*)(Src.pdata)+Src_byte_width*(srcy_16>>16)) ;
        long srcx_16=0;
        for (long x=0;x<dst_width;++x)
        {
            Color32* PColor0=&PSrcLineColor[srcx_16>>16];
            Bilinear_Fast_Common(PColor0,(Color32*)((UInt8*)(PColor0)+Src_byte_width),(srcx_16 & 0xFFFF)>>8,v_8,&pDstLine[x]);
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
}

#ifdef MMX_ACTIVE
#ifdef asm

    //Bilinear_Fast_MMX_expand2(out [edi+ebx*4];mm6=v_8,mm7=0,edx=srcx_16,esi=PSrcLineColor,ecx=PSrcLineColorNext,ebp=xrIntFloat_16)
    void  __declspec(naked) Bilinear_Fast_MMX_expand2()
    {
        asm
        {
              mov       eax,edx
              shl       eax,16
              shr       eax,24
              //== movzx       eax,dh  //eax=u_8
              MOVD      MM5,eax
              mov       eax,edx
              shr       eax,16     //srcx_16>>16

              MOVD         MM0,dword ptr [ecx+eax*4+4]//MM0=Color2
              MOVD         MM2,dword ptr [ecx+eax*4]  //MM2=Color0
              PUNPCKLWD    MM5,MM5
              MOVD         MM1,dword ptr [esi+eax*4+4]//MM1=Color3
              MOVD         MM3,dword ptr [esi+eax*4]  //MM3=Color1
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
              PADDw        MM0,MM2
              PADDw        MM1,MM3

              PSRLw        MM0,8
              PSRLw        MM1,8
                  lea       eax,[edx+ebp]
              PSUBw        MM0,MM1
                  shl       eax,16
              PSLLw        MM1,8
                  shr       eax,24
              PMULlw       MM0,MM6
                  MOVD      MM5,eax 
              PADDw        MM0,MM1
                  lea       eax,[edx+ebp]

              PSRLw     MM0,8
                  shr       eax,16     //srcx_16>>16
              PACKUSwb  MM0,MM7


                  //
                  movq      mm4,mm0

                  MOVD         MM0,dword ptr [ecx+eax*4+4]//MM0=Color2
                  MOVD         MM2,dword ptr [ecx+eax*4]  //MM2=Color0
                  PUNPCKLWD    MM5,MM5
                  MOVD         MM1,dword ptr [esi+eax*4+4]//MM1=Color3
                  MOVD         MM3,dword ptr [esi+eax*4]  //MM3=Color1
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
                  PADDw        MM0,MM2
                  PADDw        MM1,MM3

                  PSRLw        MM0,8
                  PSRLw        MM1,8
                  PSUBw        MM0,MM1
                  PSLLw        MM1,8
                  PMULlw       MM0,MM6
                  PADDw        MM0,MM1

                  PSRLw     MM0,8
                  PACKUSwb  MM0,MM7

              PUNPCKlDQ mm4,mm0
              MOVNTQ qword ptr [edi+ebx*4], mm4//write two DstColor

             ret
        }
    }
    //Bilinear_Fast_MMX_expand1(out [edi+ebx*4];mm6=v_8,mm7=0,edx=srcx_16,esi=PSrcLineColor,ecx=PSrcLineColorNext)
    void  __declspec(naked) Bilinear_Fast_MMX_expand1()
    {
        asm
        {
              mov       eax,edx
              shl       eax,16
              shr       eax,24
              //== movzx       eax,dh  //eax=u_8
              MOVD      MM5,eax
              mov       eax,edx
              shr       eax,16     //srcx_16>>16

              MOVD         MM2,dword ptr [ecx+eax*4]  //MM2=Color0
              MOVD         MM0,dword ptr [ecx+eax*4+4]//MM0=Color2
              PUNPCKLWD    MM5,MM5
              MOVD         MM3,dword ptr [esi+eax*4]  //MM3=Color1
              MOVD         MM1,dword ptr [esi+eax*4+4]//MM1=Color3
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
              PADDw        MM0,MM2
              PADDw        MM1,MM3

              PSRLw        MM0,8
              PSRLw        MM1,8
              PSUBw        MM0,MM1
              PSLLw        MM1,8
              PMULlw       MM0,MM6
              PADDw        MM0,MM1

              PSRLw     MM0,8
              PACKUSwb  MM0,MM7
              MOVd   dword ptr    [edi+ebx*4],MM0 //write DstColor

              ret
        }
    }
    
void PicZoom_ftBilinear_MMX(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(2>Src.width)||(2>Src.height)) return;

    long xrIntFloat_16=((Src.width-1)<<16)/Dst.width+1; 
    long yrIntFloat_16=((Src.height-1)<<16)/Dst.height+1;

    long dst_width=Dst.width;
    long Src_byte_width=Src.byte_width;
    Color32* pDstLine=Dst.pdata;
    long srcy_16=0;
    asm pxor  mm7,mm7 //mm7=0
    for (long y=0;y<Dst.height;++y)
    {
        unsigned long v_8=(srcy_16 & 0xFFFF)>>8;
        Color32* PSrcLineColor= (Color32*)((UInt8*)(Src.pdata)+Src_byte_width*(srcy_16>>16)) ;
        Color32* PSrcLineColorNext= (Color32*)((UInt8*)(PSrcLineColor)+Src_byte_width) ;
        asm
        {
              movd         mm6,v_8
              PUNPCKLWD    MM6,MM6
              PUNPCKLDQ    MM6,MM6//mm6=v_8
            
              mov       esi,PSrcLineColor
              mov       ecx,PSrcLineColorNext
              xor       edx,edx   //srcx_16=0
              mov       ebx,dst_width
              mov       edi,pDstLine
              push      ebp
              mov       ebp,xrIntFloat_16
              lea       edi,[edi+ebx*4]
              neg       ebx

        loop_start:
              call Bilinear_Fast_MMX_expand1
              
              add       edx,ebp //srcx_16+=xrIntFloat_16
              inc       ebx
              jnz       loop_start
              pop       ebp
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
    asm emms  //结束MMX的使用
}

    
void PicZoom_ftBilinear_MMX_expand2(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(2>Src.width)||(2>Src.height)) return;

    long xrIntFloat_16=((Src.width-1)<<16)/Dst.width; 
    long yrIntFloat_16=((Src.height-1)<<16)/Dst.height;

    long dst_width=Dst.width;
    long Src_byte_width=Src.byte_width;
    Color32* pDstLine=Dst.pdata;
    long srcy_16=0;
    asm pxor  mm7,mm7 //mm7=0
    for (long y=0;y<Dst.height;++y)
    {
        unsigned long v_8=(srcy_16 & 0xFFFF)>>8;
        Color32* PSrcLineColor= (Color32*)((UInt8*)(Src.pdata)+Src_byte_width*(srcy_16>>16)) ;
        Color32* PSrcLineColorNext= (Color32*)((UInt8*)(PSrcLineColor)+Src_byte_width) ;
        asm
        {
              movd         mm6,v_8
              PUNPCKLWD    MM6,MM6
              PUNPCKLDQ    MM6,MM6//mm6=v_8
            
              mov       esi,PSrcLineColor
              mov       ecx,PSrcLineColorNext
              xor       edx,edx   //srcx_16=0
              mov       ebx,dst_width
              push      ebp
              mov       edi,pDstLine
              mov       ebp,xrIntFloat_16
              push      ebx
              and       ebx,(not 1)
              test      ebx,ebx   //nop
              jle     end_loop2


              lea       edi,[edi+ebx*4]
              neg       ebx

        loop2_start:
              call Bilinear_Fast_MMX_expand2
              lea       edx,[edx+ebp*2]
              add       ebx,2
              jnz       loop2_start

        end_loop2:
            pop    ebx
            and    ebx,1  
            test   ebx,ebx
            jle    end_write

              lea       edi,[edi+ebx*4]
              neg       ebx
              call Bilinear_Fast_MMX_expand1
        end_write:

              pop       ebp
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
    asm sfence //刷新写入
    asm emms  //结束MMX的使用
}



    //ftBilinear_SSE2(out [edi+ebx*4];xmm6=v_8,xmm7=0,edx=srcx_16,esi=PSrcLineColor,ecx=PSrcLineColorNext,ebp=xrIntFloat_16)
    void __declspec(naked) ftBilinear_SSE2()
    {
        asm
        {
              mov       eax,edx
              shl       eax,16
              shr       eax,24
              //== movzx       eax,dh  //eax=u_8
              MOVD      XMM5,eax
              mov       eax,edx
              shr       eax,16     //srcx_16>>16

              MOVD         XMM0,  dword ptr [ecx+eax*4+4]//XMM0=Color2
              MOVD         XMM2,  dword ptr [ecx+eax*4]  //XMM2=Color0
              PUNPCKLWD    XMM5,XMM5
              MOVD         XMM1,  dword ptr [esi+eax*4+4]//XMM1=Color3
              MOVD         XMM3,  dword ptr [esi+eax*4]  //XMM3=Color1
              PUNPCKLDQ    XMM5,XMM5 //mm5=u_8
              PUNPCKLBW    XMM0,XMM7
              PUNPCKLBW    XMM1,XMM7
              PUNPCKLBW    XMM2,XMM7
              PUNPCKLBW    XMM3,XMM7
              PSUBw        XMM0,XMM2
              PSUBw        XMM1,XMM3
              PSLLw        XMM2,8
              PSLLw        XMM3,8
              PMULlw       XMM0,XMM5
              PMULlw       XMM1,XMM5
              PADDw        XMM0,XMM2
              PADDw        XMM1,XMM3

              PSRLw        XMM0,8
              PSRLw        XMM1,8
              PSUBw        XMM0,MM1
              PSLLw        XMM1,8
              PMULlw       XMM0,XMM6
              PADDw        XMM0,XMM1

              PSRLw     XMM0,8
              PACKUSwb  XMM0,XMM7
              MOVd  dword ptr  [edi+ebx*4],XMM0 //write DstColor

              ret
        }
    }

     //ftBilinear_SSE2_expand2(out [edi+ebx*4];xmm6=v_8,xmm7=0,edx=srcx_16,esi=PSrcLineColor,ecx=PSrcLineColorNext,ebp=xrIntFloat_16)
    void __declspec(naked) ftBilinear_SSE2_expand2()
    {
        asm
        {
              lea       eax,[edx+ebp]
              MOVD      XMM5,edx
              MOVD      XMM4,eax
              PUNPCKLWD XMM5,XMM4
              PSRLW     XMM5,8

              mov       eax,edx
              shr       eax,16     //srcx_16>>16
              PUNPCKLWD    XMM5,XMM5
              MOVQ         XMM2,  qword ptr [ecx+eax*4]//XMM2=0  0  Color0 Color2
              MOVQ         XMM3,  qword ptr [esi+eax*4]//XMM3=0  0  Color1 Color3
              lea       eax,[edx+ebp]
              shr       eax,16     //srcx_16>>16
              PUNPCKLDQ    XMM5,XMM5 //mm5=u_8' u_8' u_8' u_8' u_8 u_8 u_8 u_8 
              movq   xmm4,qword ptr [ecx+eax*4]
              PUNPCKLDQ    XMM2,xmm4//XMM2=Color0' Color0  Color2' Color2
              movq   xmm4,qword ptr [esi+eax*4]
              PUNPCKLDQ    XMM3,xmm4//XMM3=Color1' Color1  Color3' Color3
              MOVHLPS      XMM0,XMM2 //XMM0= X  X  Color0' Color0
              MOVHLPS      XMM1,XMM3 //XMM1= X  X  Color1' Color1

              PUNPCKLBW    XMM0,XMM7
              PUNPCKLBW    XMM1,XMM7
              PUNPCKLBW    XMM2,XMM7
              PUNPCKLBW    XMM3,XMM7
              PSUBw        XMM0,XMM2
              PSUBw        XMM1,XMM3
              PSLLw        XMM2,8
              PSLLw        XMM3,8
              PMULlw       XMM0,XMM5
              PMULlw       XMM1,XMM5
              PADDw        XMM0,XMM2
              PADDw        XMM1,XMM3

              PSRLw        XMM0,8
              PSRLw        XMM1,8
              PSUBw        XMM0,MM1
              PSLLw        XMM1,8
              PMULlw       XMM0,XMM6
              PADDw        XMM0,XMM1

              PSRLw     XMM0,8
              PACKUSwb  XMM0,XMM7

              //MOVQ qword ptr [edi+ebx*4], xmm0//write two DstColor
              MOVDQ2Q   mm4,xmm0
              movntq  qword ptr  [edi+ebx*4],mm4

              ret
        }
    }

    
void PicZoom_ftBilinear_SSE2(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(2>Src.width)||(2>Src.height)) return;

    long xrIntFloat_16=((Src.width-1)<<16)/Dst.width; 
    long yrIntFloat_16=((Src.height-1)<<16)/Dst.height;

    long dst_width=Dst.width;
    long Src_byte_width=Src.byte_width;
    Color32* pDstLine=Dst.pdata;
    long srcy_16=0;
    asm pxor  xmm7,xmm7 //xmm7=0
    for (long y=0;y<Dst.height;++y)
    {
        unsigned long v_8=(srcy_16 & 0xFFFF)>>8;
        Color32* PSrcLineColor= (Color32*)((UInt8*)(Src.pdata)+Src_byte_width*(srcy_16>>16)) ;
        Color32* PSrcLineColorNext= (Color32*)((UInt8*)(PSrcLineColor)+Src_byte_width) ;
        asm
        {
              movd        xmm6,v_8
              PUNPCKLWD   xmm6,xmm6
              PUNPCKLDQ   xmm6,xmm6
              PUNPCKLQDQ  xmm6,xmm6//xmm6=v_8
            
              mov       esi,PSrcLineColor
              mov       ecx,PSrcLineColorNext
              xor       edx,edx   //srcx_16=0
              mov       ebx,dst_width
              mov       edi,pDstLine
              push      ebp
              mov       ebp,xrIntFloat_16
              push      ebx
              and       ebx,(not 1)
              test      ebx,ebx   //nop
              jle     end_loop2


              lea       edi,[edi+ebx*4]
              neg       ebx

        loop2_start:
              call ftBilinear_SSE2_expand2
              lea       edx,[edx+ebp*2]
              add       ebx,2

              jnz       loop2_start

        end_loop2:
            pop    ebx
            and    ebx,1  
            test   ebx,ebx
            jle    end_write

              lea       edi,[edi+ebx*4]
              neg       ebx
              call Bilinear_Fast_MMX_expand1
        end_write:

              pop       ebp
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
    asm emms
}

#endif
 
void PicZoom_ftBilinear_MMX_mmh(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(2>Src.width)||(2>Src.height)) return;

    long xrIntFloat_16=((Src.width-1)<<16)/Dst.width; 
    long yrIntFloat_16=((Src.height-1)<<16)/Dst.height;

    long dst_width=Dst.width;
    long Src_byte_width=Src.byte_width;
    Color32* pDstLine=Dst.pdata;
    long srcy_16=0;
    const __m64 m_0=_mm_setzero_si64();

    for (long y=0;y<Dst.height;++y)
    {
        unsigned long v_8=(srcy_16 & 0xFFFF)>>8;
        //__m64 m_v =_mm_set_pi16(v_8,v_8,v_8,v_8);
        __m64 m_v=_m_from_int(v_8);
        m_v=_m_punpcklwd(m_v,m_v);
        m_v=_m_punpckldq(m_v,m_v);

        Color32* PSrcLineColor= (Color32*)((UInt8*)(Src.pdata)+Src_byte_width*(srcy_16>>16)) ;
        long srcx_16=0;
        for (long x=0;x<dst_width;++x)
        {
            Color32* PColor0=&PSrcLineColor[srcx_16>>16];
             Color32* PColor1=(Color32*)((UInt8*)(PColor0)+Src_byte_width);
            unsigned long u_8=(srcx_16 & 0xFFFF)>>8;
            //Bilinear_MMX_mmh
            {
                //__m64 m_u =_mm_set_pi16(u_8,u_8,u_8,u_8);
                __m64 m_u=_m_from_int(u_8);
                m_u=_m_punpcklwd(m_u,m_u);
                m_u=_m_punpckldq(m_u,m_u);

                __m64 m_color0 =_m_from_int(((int*)PColor1)[1]);
                __m64 m_color2 =_m_from_int(((int*)PColor1)[0]);
                __m64 m_color1 =_m_from_int(((int*)PColor0)[1]);
                __m64 m_color3 =_m_from_int(((int*)PColor0)[0]);
                m_color0=_m_punpcklbw(m_color0,m_0);
                m_color1=_m_punpcklbw(m_color1,m_0);
                m_color2=_m_punpcklbw(m_color2,m_0);
                m_color3=_m_punpcklbw(m_color3,m_0);

                m_color0=_m_psubw(m_color0,m_color2);
                m_color1=_m_psubw(m_color1,m_color3);
                m_color2=_m_psllwi(m_color2,8);
                m_color3=_m_psllwi(m_color3,8);
                m_color0=_m_pmullw(m_color0,m_u);
                m_color1=_m_pmullw(m_color1,m_u);
                m_color0=_m_paddw(m_color0,m_color2);
                m_color1=_m_paddw(m_color1,m_color3);
                m_color0=_m_psrlwi(m_color0,8);
                m_color1=_m_psrlwi(m_color1,8);

                m_color0=_m_psubw(m_color0,m_color1);
                m_color1=_m_psllwi(m_color1,8);
                m_color0=_m_pmullw(m_color0,m_v);
                m_color0=_m_paddw(m_color0,m_color1);
                m_color0=_m_psrlwi(m_color0,8);

                m_color0=_m_packuswb(m_color0,m_0);

                *(int*)(&pDstLine[x])=_m_to_int(m_color0);
            }
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
    _m_empty();
}

#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        inline double SinXDivX(double x) 
        {
            //该函数计算插值曲线sin(x*PI)/(x*PI)的值 //PI=3.1415926535897932385; 
            //下面是它的近似拟合表达式
            const double a = -1; //a还可以取 a=-2,-1,-0.75,-0.5等等，起到调节锐化或模糊程度的作用

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

        must_inline UInt8 border_color(long Color)
        {
            if (Color<=0)
                return 0;
            else if (Color>=255)
                return 255;
            else
                return (UInt8)Color;
        }
        
    void ThreeOrder0(const TPixels32Ref& pic,const double fx,const double fy,Color32* result)
    {
        long x0=(long)fx; if (x0>fx) --x0; //x0=floor(fx);    
        long y0=(long)fy; if (y0>fy) --y0; //y0=floor(fy);
        double fu=fx-x0;
        double fv=fy-y0;

        Color32 pixel[16];
        long i,j;

        for (i=0;i<4;++i)
        {
            for (j=0;j<4;++j)
            {
                long x=x0-1+j;
                long y=y0-1+i;
                pixel[i*4+j]=pic.getPixelsBorder(x,y);
            }
        }

        double afu[4],afv[4];
        //
        afu[0]=SinXDivX(1+fu);
        afu[1]=SinXDivX(fu);
        afu[2]=SinXDivX(1-fu);
        afu[3]=SinXDivX(2-fu);
        afv[0]=SinXDivX(1+fv);
        afv[1]=SinXDivX(fv);
        afv[2]=SinXDivX(1-fv);
        afv[3]=SinXDivX(2-fv);

        double sR=0,sG=0,sB=0,sA=0;
        for (i=0;i<4;++i)
        {
            double aR=0,aG=0,aB=0,aA=0;
            for (long j=0;j<4;++j)
            {
                aA+=afu[j]*pixel[i*4+j].a;
                aR+=afu[j]*pixel[i*4+j].r;
                aG+=afu[j]*pixel[i*4+j].g;
                aB+=afu[j]*pixel[i*4+j].b;
            }
            sA+=aA*afv[i];
            sR+=aR*afv[i];
            sG+=aG*afv[i];
            sB+=aB*afv[i];
        }

        result->a=border_color((long)(sA+0.5));
        result->r=border_color((long)(sR+0.5));
        result->g=border_color((long)(sG+0.5));
        result->b=border_color((long)(sB+0.5));
    }

void PicZoom_ThreeOrder0(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(0==Src.width)||(0==Src.height)) return;


    long dst_width=Dst.width;
    Color32* pDstLine=Dst.pdata;
    for (long y=0;y<Dst.height;++y)
    {
        double srcy=(y+0.4999999)*Src.height/Dst.height-0.5;
        for (long x=0;x<dst_width;++x)
        {
            double srcx=(x+0.4999999)*Src.width/Dst.width-0.5;
            ThreeOrder0(Src,srcx,srcy,&pDstLine[x]);
        }
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
}


//-------------------------------------------------------------------------------------------


    static long SinXDivX_Table_8[(2<<8)+1];
    class _CAutoInti_SinXDivX_Table {
    private: 
        void _Inti_SinXDivX_Table()
        {
            for (long i=0;i<=(2<<8);++i)
                SinXDivX_Table_8[i]=long(0.5+256*SinXDivX(i*(1.0/(256))))*1;
        };
    public:
        _CAutoInti_SinXDivX_Table() { _Inti_SinXDivX_Table(); }
    };
    static _CAutoInti_SinXDivX_Table __tmp_CAutoInti_SinXDivX_Table;


    //颜色查表
    static UInt8 _color_table[256*3];
    static const UInt8* color_table=&_color_table[256];
    class _CAuto_inti_color_table
    {
    public:
        _CAuto_inti_color_table() {
            for (int i=0;i<256*3;++i)
                _color_table[i]=border_color(i-256);
        }
    };
    static _CAuto_inti_color_table _Auto_inti_color_table;

    void ThreeOrder_Fast_Common(const TPixels32Ref& pic,const long x_16,const long y_16,Color32* result)
    {
        unsigned long u_8=(unsigned char)((x_16)>>8);
        unsigned long v_8=(unsigned char)((y_16)>>8);
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

        result->a=color_table[sA>>16];
        result->r=color_table[sR>>16];
        result->g=color_table[sG>>16];
        result->b=color_table[sB>>16];
    }

    void ThreeOrder_Border_Common(const TPixels32Ref& pic,const long x_16,const long y_16,Color32* result)
    {
        long x0_sub1=(x_16>>16)-1;
        long y0_sub1=(y_16>>16)-1;
        unsigned long u_16_add1=((unsigned short)(x_16))+(1<<16);
        unsigned long v_16_add1=((unsigned short)(y_16))+(1<<16);

        Color32 pixel[16];
        long i;

        for (i=0;i<4;++i)
        {
            long y=y0_sub1+i;
            pixel[i*4+0]=pic.getPixelsBorder(x0_sub1+0,y);
            pixel[i*4+1]=pic.getPixelsBorder(x0_sub1+1,y);
            pixel[i*4+2]=pic.getPixelsBorder(x0_sub1+2,y);
            pixel[i*4+3]=pic.getPixelsBorder(x0_sub1+3,y);
        }
        
        TPixels32Ref npic;
        npic.pdata     =&pixel[0];
        npic.byte_width=4*sizeof(Color32);
        //npic.width     =4;
        //npic.height    =4;
        ThreeOrder_Fast_Common(npic,u_16_add1,v_16_add1,result);
    }

void PicZoom_ThreeOrder_Common(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(0==Src.width)||(0==Src.height)) return;

    long xrIntFloat_16=((Src.width)<<16)/Dst.width+1; 
    long yrIntFloat_16=((Src.height)<<16)/Dst.height+1;
    const long csDErrorX=-(1<<15)+(xrIntFloat_16>>1);
    const long csDErrorY=-(1<<15)+(yrIntFloat_16>>1);

    long dst_width=Dst.width;

    //计算出需要特殊处理的边界
    long border_y0=((1<<16)-csDErrorY)/yrIntFloat_16+1;//y0+y*yr>=1; y0=csDErrorY => y>=(1-csDErrorY)/yr
    if (border_y0>=Dst.height) border_y0=Dst.height;
    long border_x0=((1<<16)-csDErrorX)/xrIntFloat_16+1;
    if (border_x0>=Dst.width ) border_x0=Dst.width;
    long border_y1=(((Src.height-3)<<16)-csDErrorY)/yrIntFloat_16+1; //y0+y*yr<=(height-3) => y<=(height-3-csDErrorY)/yr
    if (border_y1<border_y0) border_y1=border_y0;
    long border_x1=(((Src.width-3)<<16)-csDErrorX)/xrIntFloat_16+1;; 
    if (border_x1<border_x0) border_x1=border_x0;

    Color32* pDstLine=Dst.pdata;
    long srcy_16=csDErrorY;
    long y;
    for (y=0;y<border_y0;++y)
    {
        long srcx_16=csDErrorX;
        for (long x=0;x<dst_width;++x)
        {
            ThreeOrder_Border_Common(Src,srcx_16,srcy_16,&pDstLine[x]); //border
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
    for (y=border_y0;y<border_y1;++y)
    {
        long srcx_16=csDErrorX;
        long x;
        for (x=0;x<border_x0;++x)
        {
            ThreeOrder_Border_Common(Src,srcx_16,srcy_16,&pDstLine[x]);//border
            srcx_16+=xrIntFloat_16;
        }
        for (x=border_x0;x<border_x1;++x)
        {
            ThreeOrder_Fast_Common(Src,srcx_16,srcy_16,&pDstLine[x]);//fast  !
            srcx_16+=xrIntFloat_16;
        }
        for (x=border_x1;x<dst_width;++x)
        {
            ThreeOrder_Border_Common(Src,srcx_16,srcy_16,&pDstLine[x]);//border
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
    for (y=border_y1;y<Dst.height;++y)
    {
        long srcx_16=csDErrorX;
        for (long x=0;x<dst_width;++x)
        {
            ThreeOrder_Border_Common(Src,srcx_16,srcy_16,&pDstLine[x]); //border
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
}


//-------------------------------------------------------------------------------------------

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

        for (long i=0;i<4;++i)
        {
            long y=y0_sub1+i;
            pixel[i*4+0]=pic.getPixelsBorder(x0_sub1  ,y);
            pixel[i*4+1]=pic.getPixelsBorder(x0_sub1+1,y);
            pixel[i*4+2]=pic.getPixelsBorder(x0_sub1+2,y);
            pixel[i*4+3]=pic.getPixelsBorder(x0_sub1+3,y);
        }
        
        TPixels32Ref npic;
        npic.pdata     =&pixel[0];
        npic.byte_width=4*sizeof(Color32);
        //npic.width     =4;
        //npic.height    =4;
        ThreeOrder_Fast_MMX(npic,u_16_add1,v_16_add1,result);
    }


void PicZoom_ThreeOrder_MMX(const TPixels32Ref& Dst,const TPixels32Ref& Src)
{
    if (  (0==Dst.width)||(0==Dst.height)
        ||(0==Src.width)||(0==Src.height)) return;

    long xrIntFloat_16=((Src.width)<<16)/Dst.width+1; 
    long yrIntFloat_16=((Src.height)<<16)/Dst.height+1;
    const long csDErrorX=-(1<<15)+(xrIntFloat_16>>1);
    const long csDErrorY=-(1<<15)+(yrIntFloat_16>>1);

    long dst_width=Dst.width;

    //计算出需要特殊处理的边界
    long border_y0=((1<<16)-csDErrorY)/yrIntFloat_16+1;//y0+y*yr>=1; y0=csDErrorY => y>=(1-csDErrorY)/yr
    if (border_y0>=Dst.height) border_y0=Dst.height;
    long border_x0=((1<<16)-csDErrorX)/xrIntFloat_16+1;
    if (border_x0>=Dst.width ) border_x0=Dst.width;
    long border_y1=(((Src.height-3)<<16)-csDErrorY)/yrIntFloat_16+1; //y0+y*yr<=(height-3) => y<=(height-3-csDErrorY)/yr
    if (border_y1<border_y0) border_y1=border_y0;
    long border_x1=(((Src.width-3)<<16)-csDErrorX)/xrIntFloat_16+1;; 
    if (border_x1<border_x0) border_x1=border_x0;

    Color32* pDstLine=Dst.pdata;
    long srcy_16=csDErrorY;
    long y;
    for (y=0;y<border_y0;++y)
    {
        long srcx_16=csDErrorX;
        for (long x=0;x<dst_width;++x)
        {
            ThreeOrder_Border_MMX(Src,srcx_16,srcy_16,&pDstLine[x]); //border
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
    for (y=border_y0;y<border_y1;++y)
    {
        long srcx_16=csDErrorX;
        long x;
        for (x=0;x<border_x0;++x)
        {
            ThreeOrder_Border_MMX(Src,srcx_16,srcy_16,&pDstLine[x]);//border
            srcx_16+=xrIntFloat_16;
        }
        for (x=border_x0;x<border_x1;++x)
        {
            ThreeOrder_Fast_MMX(Src,srcx_16,srcy_16,&pDstLine[x]);//fast MMX !
            srcx_16+=xrIntFloat_16;
        }
        for (x=border_x1;x<dst_width;++x)
        {
            ThreeOrder_Border_MMX(Src,srcx_16,srcy_16,&pDstLine[x]);//border
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
    for (y=border_y1;y<Dst.height;++y)
    {
        long srcx_16=csDErrorX;
        for (long x=0;x<dst_width;++x)
        {
            ThreeOrder_Border_MMX(Src,srcx_16,srcy_16,&pDstLine[x]); //border
            srcx_16+=xrIntFloat_16;
        }
        srcy_16+=yrIntFloat_16;
        ((UInt8*&)pDstLine)+=Dst.byte_width;
    }
    asm emms
}

#endif
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 /*
#include <vector>
typedef std::vector<TPixels32Ref> TMipMap;

double public_mip_bias=0.5; //[0..1] //mip选择偏好 0.5没有偏好 靠近0偏向选择小图片 靠近1偏向选择大图片

long SelectBestPicIndex(const TMipMap& mip,const long dstWidth,const long dstHeight)
{
    long oldS=mip[0].width*mip[0].height;
    long dstS=dstWidth*dstHeight;
    if ( (dstS>=oldS) || (mip.size()==1) )
        return 0;
    else if (dstS<=1)
        return mip.size()-1;
    else
        return (long)(log((double)oldS/dstS)*0.5+public_mip_bias);
}

struct TMipWeight {
  long  BigMip;
  long  SmallMip;
  double BigMipWeight;//[0..1]
};

TMipWeight SelectBestPicIndexEx(const TMipMap& mip,const long dstWidth,const long dstHeight)
{
    long oldS=mip[0].width*mip[0].height;
    long dstS=dstWidth*dstHeight;
    TMipWeight result;
    if ( (dstS>=oldS) || (mip.size()==1) )
    {
        result.BigMip=0;
        result.SmallMip=0;
        result.BigMipWeight=1.0;
    }
    else if (dstS<=1)
    {
        result.BigMip=mip.size()-1;
        result.SmallMip=mip.size()-1;
        result.BigMipWeight=1.0;
    }
    else
    {
        double bestIndex=log((double)oldS/dstS)*0.5+0.5; //or + public_mip_bias
        result.BigMip=(long)bestIndex;
        if (bestIndex==mip.size()-1)
        {
            result.SmallMip=mip.size()-1;
            result.BigMipWeight=1.0;
        }
        else
        {
            result.SmallMip=result.BigMip+1;
            result.BigMipWeight=1.0-(bestIndex-result.BigMip);
        }
    }
    return result;
}
*/


int main()
{
    std::cout<<" 请输入回车键开始测试(可以把进程优先级设置为“实时”)> ";
    waitInputChar();
    std::cout<<std::endl;
                                                                                    //AMD64X2 4200+ 2.33G
      //*
      test("PicZoom0"                       ,PicZoom0                       , 40,true);//   19.25 FPS
      test("PicZoom1"                       ,PicZoom1                       , 60,true);//   30.23 FPS
      test("PicZoom2"                       ,PicZoom2                       ,400,true);//  188.24 FPS
      test("PicZoom3"                       ,PicZoom3                       ,800,true);//  478.47 FPS
#ifdef asm   
      test("PicZoom3_asm"                   ,PicZoom3_asm                   ,800,true);//  478.47 FPS
      test("PicZoom3_float"                 ,PicZoom3_float                 ,600,true);//  290.98 FPS
#endif
      test("PicZoom3_Table"                 ,PicZoom3_Table                 ,800,true);//  449.19 FPS
      test("PicZoom3_Table_OpMul"           ,PicZoom3_Table_OpMul           ,800,true);//  445.19 FPS
#ifdef MMX_ACTIVE
#ifdef asm   
      test("PicZoom3_SSE"                   ,PicZoom3_SSE                   ,1500,true);// 780.44 FPS
      test("PicZoom3_SSE_prefetch"          ,PicZoom3_SSE_prefetch          ,2000,true);//1084.60 FPS
#endif
      test("PicZoom3_SSE_mmh"               ,PicZoom3_SSE_mmh               ,1200,true);// 640.00 FPS
#endif
      //*/
      //-------------------------------------------------------------------------------------------
      //*
      test("PicZoom_Bilinear0"              ,PicZoom_Bilinear0              , 20,true);//   10.00 FPS
      test("PicZoom_Bilinear1"              ,PicZoom_Bilinear1              , 60,true);//   30.98 FPS 
      test("PicZoom_Bilinear2"              ,PicZoom_Bilinear2              ,100,true);//   56.12 FPS 
      test("PicZoom_Bilinear_Common"        ,PicZoom_Bilinear_Common        ,150,true);//   75.60 FPS 
#ifdef MMX_ACTIVE
#ifdef asm   
      test("PicZoom_Bilinear_MMX"           ,PicZoom_Bilinear_MMX           ,250,true);// 133.33 FPS 
      test("PicZoom_Bilinear_MMX_Ex"        ,PicZoom_Bilinear_MMX_Ex        ,300,true);// 157.40 FPS 
#endif
#endif
      test("PicZoom_ftBilinear_Common"      ,PicZoom_ftBilinear_Common      ,150,true);//  79.37 FPS
#ifdef MMX_ACTIVE
#ifdef asm   
      test("PicZoom_ftBilinear_MMX"         ,PicZoom_ftBilinear_MMX         ,300,true);//  156.09 FPS
      test("PicZoom_ftBilinear_MMX_expand2" ,PicZoom_ftBilinear_MMX_expand2 ,300,true);//  168.44 FPS
      test("PicZoom_ftBilinear_SSE2"        ,PicZoom_ftBilinear_SSE2        ,300,true);//  148.88 FPS
#endif
      test("PicZoom_ftBilinear_MMX_mmh"     ,PicZoom_ftBilinear_MMX_mmh     ,150,true);//   69.57 FPS
#endif
      //*/
      //-------------------------------------------------------------------------------------------
      //*
      test("PicZoom_ThreeOrder0"            ,PicZoom_ThreeOrder0            ,  6,true);//   3.34 FPS
      test("PicZoom_ThreeOrder_Common"      ,PicZoom_ThreeOrder_Common      , 35,true);//  16.71 FPS
#ifdef MMX_ACTIVE
#ifdef asm   
      test("PicZoom_ThreeOrder_MMX"         ,PicZoom_ThreeOrder_MMX         , 70,true);//  33.44 FPS
#endif
#endif
      //*/
 
    std::cout<<std::endl<<" 测试完成. ";
    waitInputChar();
    return 0;
}
