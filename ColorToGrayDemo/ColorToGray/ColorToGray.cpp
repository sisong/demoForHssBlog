// ColorToGray.cpp

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

typedef void (*TColorToGrayProc)(const TPixels32Ref& src,const TPixels32Ref& dst);

void test(const char* proc_name,const TColorToGrayProc fproc,const long csRunCount,bool isSaveResultPic=false){
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
        fproc(srcPic.getRef(),dstPic.getRef());
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

///////////////////////////////////////////////////////////////////////////////////////////

//灰度转换系数
const double gray_r_coeff=0.299;
const double gray_g_coeff=0.587;
const double gray_b_coeff=0.114;

    //处理一个点
    must_inline double toGray_float(const Color32& src){
        return (src.r*gray_r_coeff +src.g*gray_g_coeff +src.b*gray_b_coeff);
    }
    //处理一行
    void colorToGrayLine_float(const Color32* src,Color32* dst,long width){
        for (long x = 0; x < width; ++x){
            int gray=(int)toGray_float(src[x]);
            dst[x]=Color32(gray,gray,gray,src[x].a);//R,G,B都设置为相同的亮度值,A不变
        }
    }

void colorToGray_float(const TPixels32Ref& src,const TPixels32Ref& dst){
    long width=std::min(src.width,dst.width);
    long height=std::min(src.height,dst.height);
    Color32* srcLine=src.pdata;
    Color32* dstLine=dst.pdata;
    for (long y = 0; y < height; ++y){
        colorToGrayLine_float(srcLine,dstLine,width);
        src.nextLine(srcLine);
        dst.nextLine(dstLine);
    }
}

//----------------------------------

    must_inline int toGray_int16(const Color32& src){
        const long bit=16;
        const int gray_r_coeff_int=(int)( gray_r_coeff*(1<<bit)+0.4999999 );
        const int gray_g_coeff_int=(int)( gray_g_coeff*(1<<bit)+0.4999999 );
        const int gray_b_coeff_int=(1<<bit)-gray_r_coeff_int-gray_g_coeff_int;

        return (src.r*gray_r_coeff_int +src.g*gray_g_coeff_int +src.b*gray_b_coeff_int) >> bit;
    }
    inline void colorToGrayLine_int16(const Color32* src,Color32* dst,long width){
        for (long x = 0; x < width; ++x){
            int gray=toGray_int16(src[x]);
            dst[x]=Color32(gray,gray,gray,src[x].a);
        }
    }

void colorToGray_int16(const TPixels32Ref& src,const TPixels32Ref& dst){
    long width=std::min(src.width,dst.width);
    long height=std::min(src.height,dst.height);
    Color32* srcLine=src.pdata;
    Color32* dstLine=dst.pdata;
    for (long y = 0; y < height; ++y){
        colorToGrayLine_int16(srcLine,dstLine,width);
        src.nextLine(srcLine);
        dst.nextLine(dstLine);
    }
}

//----------------------------------

    //四路展开
    void colorToGrayLine_int16_expand4(const Color32* src,Color32* dst,long width){
        long widthFast=width>>2<<2;
        for (long x = 0; x < widthFast; x+=4){
            int gray0=toGray_int16(src[x  ]);
            int gray1=toGray_int16(src[x+1]);
            dst[x  ]=Color32(gray0,gray0,gray0,src[x  ].a);
            dst[x+1]=Color32(gray1,gray1,gray1,src[x+1].a);
            int gray2=toGray_int16(src[x+2]);
            int gray3=toGray_int16(src[x+3]);
            dst[x+2]=Color32(gray2,gray2,gray2,src[x+2].a);
            dst[x+3]=Color32(gray3,gray3,gray3,src[x+3].a);
        }
        //border
        if (width>widthFast)
            colorToGrayLine_int16(&src[widthFast],&dst[widthFast],width-widthFast);
    }

void colorToGray_int16_expand4(const TPixels32Ref& src,const TPixels32Ref& dst){
    long width=std::min(src.width,dst.width);
    long height=std::min(src.height,dst.height);
    Color32* srcLine=src.pdata;
    Color32* dstLine=dst.pdata;
    for (long y = 0; y < height; ++y){
        colorToGrayLine_int16_expand4(srcLine,dstLine,width);
        src.nextLine(srcLine);
        dst.nextLine(dstLine);
    }
}

//----------------------------------

    must_inline UInt32 toGray_int8_opMul(const Color32* src2Color){
        const UInt32 gray_r_coeff_8=(UInt32)( gray_r_coeff*(1<<8)+0.4999999);
        const UInt32 gray_g_coeff_8=(UInt32)( gray_g_coeff*(1<<8)+0.4999999);
        const UInt32 gray_b_coeff_8=(1<<8)-gray_r_coeff_8-gray_g_coeff_8;
        UInt32 RR,GG,BB;

        BB=src2Color[0].b | (src2Color[1].b<<16);
        GG=src2Color[0].g | (src2Color[1].g<<16);
        RR=src2Color[0].r | (src2Color[1].r<<16);
        BB*=gray_b_coeff_8;
        GG*=gray_g_coeff_8;
        RR*=gray_r_coeff_8;
        return BB+GG+RR;
    }

    void colorToGrayLine_int8_opMul(const Color32* src,Color32* dst,long width){
        long widthFast=width>>2<<2;
        for (long x = 0; x < widthFast; x+=4){
            UInt32 gray01=toGray_int8_opMul(&src[x  ]);
            int gray0=(gray01&0x0000FF00)>>8;
            int gray1=gray01>>24;
            dst[x  ]=Color32(gray0,gray0,gray0,src[x  ].a);
            dst[x+1]=Color32(gray1,gray1,gray1,src[x+1].a);
            UInt32 gray23=toGray_int8_opMul(&src[x+2]);
            int gray2=(gray23&0x0000FF00)>>8;
            int gray3=gray23>>24;
            dst[x+2]=Color32(gray2,gray2,gray2,src[x+2].a);
            dst[x+3]=Color32(gray3,gray3,gray3,src[x+3].a);
        }
        //border
        if (width>widthFast)
            colorToGrayLine_int16(&src[widthFast],&dst[widthFast],width-widthFast);
    }

void colorToGray_int8_opMul(const TPixels32Ref& src,const TPixels32Ref& dst){
    long width=std::min(src.width,dst.width);
    long height=std::min(src.height,dst.height);
    Color32* srcLine=src.pdata;
    Color32* dstLine=dst.pdata;
    for (long y = 0; y < height; ++y){
        colorToGrayLine_int8_opMul(srcLine,dstLine,width);
        src.nextLine(srcLine);
        dst.nextLine(dstLine);
    }
}

#ifdef MMX_ACTIVE

//////////////////////////////////////////////////////
#ifdef asm

    void colorToGrayLine_MMX(const Color32* src,Color32* dst,long width){
        //const UInt32 gray_r_coeff_7=(UInt32)( gray_r_coeff*(1<<7)+0.4999999 );
        //const UInt32 gray_g_coeff_7=(UInt32)( gray_g_coeff*(1<<7)+0.4999999 );
        //const UInt32 gray_b_coeff_7=(1<<7)-gray_r_coeff_7-gray_g_coeff_7;
        // csMMX_rgb_coeff_w= short[ 0 , gray_r_coeff_7 , gray_g_coeff_7 , gray_b_coeff_7 ] 
        const  UInt64   csMMX_rgb_coeff_w  = (((UInt64)0x00000026)<<32) | 0x004b000f;

        long widthFast=width>>1<<1;
        if (widthFast>0){
            asm{
                    pcmpeqb        mm5,mm5                // FF  FF  FF  FF  FF  FF  FF  FF
                    mov        ecx,widthFast
                    pxor        mm7,mm7                // 00  00  00  00  00  00  00  00
                    pcmpeqb        mm4,mm4                // FF  FF  FF  FF  FF  FF  FF  FF
                    mov     eax,src
                    mov     edx,dst
                    movq        mm6,csMMX_rgb_coeff_w
                    psrlw        mm5,15                //      1       1       1       1
                    lea     eax,[eax+ecx*4]
                    lea     edx,[edx+ecx*4]    
                    pslld        mm4,24                // FF  00  00  00  FF  00  00  00
                    neg     ecx
                   
                  loop_beign:
                    movq        mm0,[eax+ecx*4]        // A1  R1  G1  B1  A0  R0  G0  B0
                    movq        mm1,mm0
                    movq        mm3,mm0
                    punpcklbw   mm0,mm7                // 00  A0  00  R0  00  G0  00  B0
                    punpckhbw   mm1,mm7                // 00  A1  00  R1  00  G1  00  B1
                    pmaddwd     mm0,mm6             // R0*r_coeff      G0*g_coeff+B0*b_coeff
                    pmaddwd     mm1,mm6             // R1*r_coeff      G1*g_coeff+B1*b_coeff
                    pand        mm3,mm4             // A1  00  00  00  A0  00  00  00
                    packssdw    mm0,mm1             // sR1     sG1+sB1 sR0     sG0+sB0
                    pmaddwd     mm0,mm5             // sR1+sG1+sB1     sR0+sG0+sB0
                    psrld       mm0,7               // 00 00 00 Gray1  00 00 00 Gray0

                    movq        mm1,mm0
                    movq        mm2,mm0
                    pslld        mm1,8                // 00 00 Gray1 00  00 00 Gray0 00
                    por         mm0,mm3
                    pslld        mm2,16                // 00 Gray1 00 00  00 Gray0 00 00
                    por         mm0,mm1
                    por         mm0,mm2             // A1 Gray1 Gray1 Gray1  A0 Gray0 Gray0 Gray0 

                    movq [edx+ecx*4],mm0

                    add     ecx,2                 
                    jnz     loop_beign
            }
        }

        //border
        if (width>widthFast)
            colorToGrayLine_int16(&src[widthFast],&dst[widthFast],width-widthFast);
    }


void colorToGray_MMX(const TPixels32Ref& src,const TPixels32Ref& dst){
    long width=std::min(src.width,dst.width);
    long height=std::min(src.height,dst.height);
    Color32* srcLine=src.pdata;
    Color32* dstLine=dst.pdata;
    for (long y = 0; y < height; ++y){
        colorToGrayLine_MMX(srcLine,dstLine,width);
        src.nextLine(srcLine);
        dst.nextLine(dstLine);
    }
    asm{ 
        emms //MMX使用结束
    }
}

//----------------------------------

    void colorToGrayLine_MMX2(const Color32* src,Color32* dst,long width){
        //const UInt32 gray_r_coeff_7=(UInt32)( gray_r_coeff*(1<<7)+0.4999999 );
        //const UInt32 gray_g_coeff_7=(UInt32)( gray_g_coeff*(1<<7)+0.4999999 );
        //const UInt32 gray_b_coeff_7=(1<<7)-gray_r_coeff_7-gray_g_coeff_7;
        // csMMX_rgb_coeff_w= short[ 0 , gray_r_coeff_7 , gray_g_coeff_7 , gray_b_coeff_7 ] 
        const  UInt64   csMMX_rgb_coeff_w  = (((UInt64)0x00000026)<<32) | 0x004b000f;

        long widthFast=width>>1<<1;
        if (widthFast>0){
            asm{
                    pcmpeqb        mm5,mm5                // FF  FF  FF  FF  FF  FF  FF  FF
                    mov        ecx,widthFast
                    pxor        mm7,mm7                // 00  00  00  00  00  00  00  00
                    pcmpeqb        mm4,mm4                // FF  FF  FF  FF  FF  FF  FF  FF
                    mov     eax,src
                    mov     edx,dst
                    movq        mm6,csMMX_rgb_coeff_w
                    psrlw        mm5,15                //      1       1       1       1
                    lea     eax,[eax+ecx*4]
                    lea     edx,[edx+ecx*4]    
                    pslld        mm4,24                // FF  00  00  00  FF  00  00  00
                    neg     ecx
                   
                  loop_beign:
                    movq        mm0,[eax+ecx*4]        // A1  R1  G1  B1  A0  R0  G0  B0
                    movq        mm1,mm0
                    movq        mm3,mm0
                    punpcklbw   mm0,mm7                // 00  A0  00  R0  00  G0  00  B0
                    punpckhbw   mm1,mm7                // 00  A1  00  R1  00  G1  00  B1
                    pmaddwd     mm0,mm6             // R0*r_coeff      G0*g_coeff+B0*b_coeff
                    pmaddwd     mm1,mm6             // R1*r_coeff      G1*g_coeff+B1*b_coeff
                    pand        mm3,mm4             // A1  00  00  00  A0  00  00  00
                    packssdw    mm0,mm1             // sR1     sG1+sB1 sR0     sG0+sB0
                    pmaddwd     mm0,mm5             // sR1+sG1+sB1     sR0+sG0+sB0
                    psrld       mm0,7               // 00 00 00 Gray1  00 00 00 Gray0

                    movq        mm1,mm0
                    movq        mm2,mm0
                    pslld        mm1,8                // 00 00 Gray1 00  00 00 Gray0 00
                    por         mm0,mm3
                    pslld        mm2,16                // 00 Gray1 00 00  00 Gray0 00 00
                    por         mm0,mm1
                    por         mm0,mm2             // A1 Gray1 Gray1 Gray1  A0 Gray0 Gray0 Gray0 

                    movntq [edx+ecx*4],mm0  //和colorToGrayLine_MMX的不同之处 

                    add     ecx,2                 
                    jnz     loop_beign
            }
        }

        //border
        if (width>widthFast)
            colorToGrayLine_int16(&src[widthFast],&dst[widthFast],width-widthFast);
    }


void colorToGray_MMX2(const TPixels32Ref& src,const TPixels32Ref& dst){
    long width=std::min(src.width,dst.width);
    long height=std::min(src.height,dst.height);
    Color32* srcLine=src.pdata;
    Color32* dstLine=dst.pdata;
    for (long y = 0; y < height; ++y){
        colorToGrayLine_MMX2(srcLine,dstLine,width);
        src.nextLine(srcLine);
        dst.nextLine(dstLine);
    }
    asm{
        sfence  //刷新写入
        emms
    }
}

#endif //asm
//////////////////////////////////////////////////////

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

    void colorToGrayLine_MMX_mmh(const Color32* src,Color32* dst,long width){
        //const UInt32 gray_r_coeff_7=(UInt32)( gray_r_coeff*(1<<7)+0.4999999 );
        //const UInt32 gray_g_coeff_7=(UInt32)( gray_g_coeff*(1<<7)+0.4999999 );
        //const UInt32 gray_b_coeff_7=(1<<7)-gray_r_coeff_7-gray_g_coeff_7;
        // csMMX_rgb_coeff_w= short[ 0 , gray_r_coeff_7 , gray_g_coeff_7 , gray_b_coeff_7 ] 
        long widthFast=width>>1<<1;
        if (widthFast>0){
            const UInt64 csMMX_rgb_coeff_w  =(((UInt64)0x00000026)<<32) | 0x004b000f;
            const __m64 mm6=*(const __m64*)&csMMX_rgb_coeff_w;
            const __m64 mm7=_mm_setzero_si64();     //mm?变量值同colorToGrayLine_MMX中的mmx值一致
            __m64 mm5=_mm_cmpeq_pi8(mm7,mm7);       // ...
            const __m64 mm4=_mm_slli_pi32(mm5,24);  // ...
            mm5=_mm_srli_pi16(mm5,15);              // ...

            for (long x = 0; x < widthFast; x+=2){
                __m64 mm0=*(__m64*)&src[x];
                __m64 mm1=mm0;
                __m64 mm3=mm0;
                mm0=_mm_unpacklo_pi8(mm0,mm7);
                mm1=_mm_unpackhi_pi8(mm1,mm7);
                mm0=_mm_madd_pi16(mm0,mm6);
                mm1=_mm_madd_pi16(mm1,mm6);
                mm3=_mm_and_si64(mm3,mm4);
                mm0=_mm_packs_pi32(mm0,mm1);
                mm0=_mm_madd_pi16(mm0,mm5);
                mm0=_mm_srli_pi32(mm0,7);

                mm1=mm0;
                __m64 mm2=mm0;
                mm1=_mm_slli_pi32(mm1,8);
                mm0=_mm_or_si64(mm0,mm3);
                mm2=_mm_slli_pi32(mm2,16);
                mm0=_mm_or_si64(mm0,mm1);
                mm0=_mm_or_si64(mm0,mm2);

                *(__m64*)&dst[x]=mm0;
            }
        }

        //border
        if (width>widthFast)
            colorToGrayLine_int16(&src[widthFast],&dst[widthFast],width-widthFast);
    }


void colorToGray_MMX_mmh(const TPixels32Ref& src,const TPixels32Ref& dst){
    long width=std::min(src.width,dst.width);
    long height=std::min(src.height,dst.height);
    Color32* srcLine=src.pdata;
    Color32* dstLine=dst.pdata;
    for (long y = 0; y < height; ++y){
        colorToGrayLine_MMX_mmh(srcLine,dstLine,width);
        src.nextLine(srcLine);
        dst.nextLine(dstLine);
    }
    _mm_empty(); //MMX使用结束
}

//----------------------------------

    void colorToGrayLine_MMX2_mmh(const Color32* src,Color32* dst,long width){
        //const UInt32 gray_r_coeff_7=(UInt32)( gray_r_coeff*(1<<7)+0.4999999 );
        //const UInt32 gray_g_coeff_7=(UInt32)( gray_g_coeff*(1<<7)+0.4999999 );
        //const UInt32 gray_b_coeff_7=(1<<7)-gray_r_coeff_7-gray_g_coeff_7;
        // csMMX_rgb_coeff_w= short[ 0 , gray_r_coeff_7 , gray_g_coeff_7 , gray_b_coeff_7 ] 
        long widthFast=width>>1<<1;
        if (widthFast>0){
            const UInt64 csMMX_rgb_coeff_w  =(((UInt64)0x00000026)<<32) | 0x004b000f;
            const __m64 mm6=*(const __m64*)&csMMX_rgb_coeff_w;
            const __m64 mm7=_mm_setzero_si64();     //mm?变量值同colorToGrayLine_MMX中的mmx值一致
            __m64 mm5=_mm_cmpeq_pi8(mm7,mm7);       // ...
            const __m64 mm4=_mm_slli_pi32(mm5,24);  // ...
            mm5=_mm_srli_pi16(mm5,15);              // ...

            for (long x = 0; x < widthFast; x+=2){
                __m64 mm0=*(__m64*)&src[x];
                __m64 mm1=mm0;
                __m64 mm3=mm0;
                mm0=_mm_unpacklo_pi8(mm0,mm7);
                mm1=_mm_unpackhi_pi8(mm1,mm7);
                mm0=_mm_madd_pi16(mm0,mm6);
                mm1=_mm_madd_pi16(mm1,mm6);
                mm3=_mm_and_si64(mm3,mm4);
                mm0=_mm_packs_pi32(mm0,mm1);
                mm0=_mm_madd_pi16(mm0,mm5);
                mm0=_mm_srli_pi32(mm0,7);

                mm1=mm0;
                __m64 mm2=mm0;
                mm1=_mm_slli_pi32(mm1,8);
                mm0=_mm_or_si64(mm0,mm3);
                mm2=_mm_slli_pi32(mm2,16);
                mm0=_mm_or_si64(mm0,mm1);
                mm0=_mm_or_si64(mm0,mm2);

                //*(__m64*)&dst[x]=mm0;
                _mm_stream_pi((__m64*)&dst[x],mm0);
            }
        }

        //border
        if (width>widthFast)
            colorToGrayLine_int16(&src[widthFast],&dst[widthFast],width-widthFast);
    }


void colorToGray_MMX2_mmh(const TPixels32Ref& src,const TPixels32Ref& dst){
    long width=std::min(src.width,dst.width);
    long height=std::min(src.height,dst.height);
    Color32* srcLine=src.pdata;
    Color32* dstLine=dst.pdata;
    for (long y = 0; y < height; ++y){
        colorToGrayLine_MMX2_mmh(srcLine,dstLine,width);
        src.nextLine(srcLine);
        dst.nextLine(dstLine);
    }
    _mm_sfence();//刷新写入
    _mm_empty(); //MMX使用结束
}

#endif //MMX_ACTIVE

//////////////////////////////////////////////////////////////////////////////////////////

int main(){
    //char tmpchar;
    //std::cin>>tmpchar;
    std::cout<<" 请输入回车键开始测试(可以把进程优先级设置为“实时”)> ";
    waitInputChar();
    std::cout<<std::endl;
                                                                                    //AMD64X2 4200+ 2.33G
    test("colorToGray_float         ",colorToGray_float         ,300,true);            //145.49 FPS 
    test("colorToGray_int16         ",colorToGray_int16         ,700,true);            //355.33 FPS 
    test("colorToGray_int16_expand4 ",colorToGray_int16_expand4 ,800,true);            //413.22 FPS 
    test("colorToGray_int8_opMul    ",colorToGray_int8_opMul    ,800,true);            //387.97 FPS
#ifdef MMX_ACTIVE
    test("colorToGray_MMX_mmh       ",colorToGray_MMX_mmh       ,1000,true);        //508.69 FPS 
    test("colorToGray_MMX2_mmh      ",colorToGray_MMX2_mmh      ,1000,true);        //540.78 FPS 
#ifdef asm                                                                                         
    test("colorToGray_MMX           ",colorToGray_MMX           ,1200,true);        //590.84 FPS 
    test("colorToGray_MMX2          ",colorToGray_MMX2          ,1200,true);        //679.50 FPS 
#endif
#endif
 
    std::cout<<std::endl<<" 测试完成. ";
    waitInputChar();
    return 0;
}
