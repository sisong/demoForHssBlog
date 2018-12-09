// YuvToRGB32.cpp

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>


#if _MSC_VER >= 1300
    #define _IS_USE_MM_H_
    #include   <mmintrin.h>
    #include   <xmmintrin.h> 
#endif

#include <iostream>
#include "..\..\..\h2DEngine\hImport.h"

#define _MY_TIME_CLOCK

#ifdef _MY_TIME_CLOCK
    #define clock_t __int64
    clock_t CLOCKS_PER_SEC=0;
    inline clock_t clock() {
        __int64 result;
        if (CLOCKS_PER_SEC==0)
        {
            QueryPerformanceFrequency((LARGE_INTEGER *)&result);
            CLOCKS_PER_SEC=(clock_t)result;
        }
        QueryPerformanceCounter((LARGE_INTEGER *)&result);
        return (clock_t)result;
    }
#else
  #include <time.h>
#endif


using namespace perception_plan_image;

#define asm __asm

typedef unsigned char  TUInt8; // [0..255]
typedef unsigned short TUInt16;
typedef unsigned long  TUInt32;

/*
struct TARGB32      //32 bit color
{
    TUInt8  b,g,r,a;          // a is alpha
};

struct TPicRegion  //一块颜色数据区的描述，便于参数传递
{
    TARGB32*        pdata;        //颜色数据首地址
    long            byte_width;   //一行数据的物理宽度(字节宽度)；注意: abs(byte_width)有可能大于等于width*sizeof(TARGB32);
    unsigned long   width;        //像素宽度
    unsigned long   height;       //像素高度
};
*/
typedef Context32 TPicRegion;
typedef Color32 TARGB32;

//那么访问一个点的函数可以写为：
__forceinline TARGB32& Pixels(const TPicRegion& pic,const long x,const long y)
{
    return ( (TARGB32*)((TUInt8*)pic.pdata+pic.byte_width*y) )[x];
}

///////////////////////////////////

__forceinline void RGB32ToYUV(const TARGB32 rgb,TUInt8& Y,TUInt8& U,TUInt8& V)
{
    Y= 0.256788*rgb.r + 0.504129*rgb.g + 0.097906*rgb.b +  16;
    U=-0.148223*rgb.r - 0.290993*rgb.g + 0.439216*rgb.b + 128;
    V= 0.439216*rgb.r - 0.367788*rgb.g - 0.071427*rgb.b + 128;
}

    __forceinline long border_color(long color)
    {
        if (color>255)
            return 255;
        else if (color<0)
            return 0;
        else
            return color;
    }


void Encode_RGB32ToYUYV(const TPicRegion& Src,TUInt8* pYUYV)
{
    assert((Src.width & 1)==0);  //要求数据宽度必须是2的倍数

    for (long y=0;y<Src.height;++y)
    {
        for (long x=0;x<Src.width/2;++x)
        {
            TUInt8 Y0,U0,V0,Y1,U1,V1;
            RGB32ToYUV(Pixels(Src,x*2+0,y),Y0,U0,V0);
            RGB32ToYUV(Pixels(Src,x*2+1,y),Y1,U1,V1);
            pYUYV[0]=Y0;
            pYUYV[1]=(U0+U1)/2;
            pYUYV[2]=Y1;
            pYUYV[3]=(V0+V1)/2;
            pYUYV+=4;
        }
    }
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//浮点实现
    __forceinline TARGB32 YUVToRGB32_float(const TUInt8 Y,const TUInt8 U,const TUInt8 V)
    {
        TARGB32 result;
        result.b= border_color( 1.164383 * (Y - 16) + 2.017232*(U - 128) );
        result.g= border_color( 1.164383 * (Y - 16) - 0.391762*(U - 128) - 0.812968*(V - 128) );
        result.r= border_color( 1.164383 * (Y - 16) + 1.596027*(V - 128) );
        result.a= 255;
        return result;
    }

void DECODE_YUYV_Float(const TUInt8* pYUYV,const TPicRegion& DstPic)
{
    assert((DstPic.width & 1)==0); 
    
    TARGB32* pDstLine=DstPic.pdata; 
    for (long y=0;y<DstPic.height;++y)
    {
        for (long x=0;x<DstPic.width;x+=2)
        {
            pDstLine[x+0]=YUVToRGB32_float(pYUYV[0],pYUYV[1],pYUYV[3]);
            pDstLine[x+1]=YUVToRGB32_float(pYUYV[2],pYUYV[1],pYUYV[3]);
            pYUYV+=4;
        }
        ((TUInt8*&)pDstLine)+=DstPic.byte_width;
    }    
}

    __forceinline long border_color1(long color)
    {
        color &=~(color>>31);  
        return (color | ((255-color)>>31) );
    }

        const int csY_coeff_16 = 1.164383*(1<<16);
        const int csU_blue_16  = 2.017232*(1<<16);
        const int csU_green_16 = (-0.391762)*(1<<16);
        const int csV_green_16 = (-0.812968)*(1<<16);
        const int csV_red_16   = 1.596027*(1<<16);

//整数实现(定点数实现)
    __forceinline TARGB32 YUVToRGB32_Int(const TUInt8 Y,const TUInt8 U,const TUInt8 V)
    {

        TARGB32 result;
        int Ye=csY_coeff_16 * (Y - 16); 
        int Ue=U-128;
        int Ve=V-128;
        result.b= border_color( ( Ye + csU_blue_16 * Ue )>>16 );
        result.g= border_color( ( Ye + csU_green_16 * Ue + csV_green_16 * Ve )>>16 );
        result.r= border_color( ( Ye + csV_red_16 * Ve )>>16 );
        result.a= 255;
        return result;
    }

void DECODE_YUYV_Int(const TUInt8* pYUYV,const TPicRegion& DstPic)
{
    assert((DstPic.width & 1)==0); 
    
    TARGB32* pDstLine=DstPic.pdata; 
    for (long y=0;y<DstPic.height;++y)
    {
        for (long x=0;x<DstPic.width;x+=2)
        {
            pDstLine[x+0]=YUVToRGB32_Int(pYUYV[0],pYUYV[1],pYUYV[3]);
            pDstLine[x+1]=YUVToRGB32_Int(pYUYV[2],pYUYV[1],pYUYV[3]);
            pYUYV+=4;
        }
        ((TUInt8*&)pDstLine)+=DstPic.byte_width;
    }  
}


//颜色查表
static TUInt8 _color_table[256*3];
static const TUInt8* color_table=&_color_table[256];
class _CAuto_inti_color_table
{
public:
    _CAuto_inti_color_table() {
        for (int i=0;i<256*3;++i)
            _color_table[i]=border_color(i-256);
    }
};
static _CAuto_inti_color_table _Auto_inti_color_table;

    __forceinline TARGB32 YUVToRGB32_RGBTable(const TUInt8 Y,const TUInt8 U,const TUInt8 V)
    {

        TARGB32 result;
        int Ye=csY_coeff_16 * (Y - 16); 
        int Ue=U-128;
        int Ve=V-128;
        result.b= color_table[ ( Ye + csU_blue_16 * Ue )>>16 ];
        result.g= color_table[ ( Ye + csU_green_16 * Ue + csV_green_16 * Ve )>>16 ];
        result.r= color_table[ ( Ye + csV_red_16 * Ve )>>16 ];
        result.a= 255;
        return result;
    }


void DECODE_YUYV_RGBTable(const TUInt8* pYUYV,const TPicRegion& DstPic)
{
    assert((DstPic.width & 1)==0); 
    
    TARGB32* pDstLine=DstPic.pdata; 
    for (long y=0;y<DstPic.height;++y)
    {
        for (long x=0;x<DstPic.width;x+=2)
        {
            pDstLine[x+0]=YUVToRGB32_RGBTable(pYUYV[0],pYUYV[1],pYUYV[3]);
            pDstLine[x+1]=YUVToRGB32_RGBTable(pYUYV[2],pYUYV[1],pYUYV[3]);
            pYUYV+=4;
        }
        ((TUInt8*&)pDstLine)+=DstPic.byte_width;
    }    
}

//全查表
static int Ym_table[256];
static int Um_blue_table[256];
static int Um_green_table[256];
static int Vm_green_table[256];
static int Vm_red_table[256];

class _CAuto_inti_yuv_table
{
public:
    _CAuto_inti_yuv_table() {
        for (int i=0;i<256;++i)
        {
            Ym_table[i]=csY_coeff_16 * (i - 16);
            Um_blue_table[i]=csU_blue_16 * (i - 128);
            Um_green_table[i]=csU_green_16 * (i - 128);
            Vm_green_table[i]=csV_green_16 * (i - 128);
            Vm_red_table[i]=csV_red_16 * (i - 128);
        }
    }
};
static _CAuto_inti_yuv_table _Auto_inti_yuv_table;

    __forceinline TARGB32 YUVToRGB32_Table(const TUInt8 Y,const TUInt8 U,const TUInt8 V)
    {
        TARGB32 result;
        int Ye=Ym_table[Y]; 
        result.b= color_table[ ( Ye + Um_blue_table[U] )>>16 ];
        result.g= color_table[ ( Ye + Um_green_table[U] + Vm_green_table[V] )>>16 ];
        result.r= color_table[ ( Ye + Vm_red_table[V] )>>16 ];
        result.a= 255;
        return result;
    }


void DECODE_YUYV_Table(const TUInt8* pYUYV,const TPicRegion& DstPic)
{
    assert((DstPic.width & 1)==0); 
    
    TARGB32* pDstLine=DstPic.pdata; 
    for (long y=0;y<DstPic.height;++y)
    {
        for (long x=0;x<DstPic.width;x+=2)
        {
            pDstLine[x+0]=YUVToRGB32_Table(pYUYV[0],pYUYV[1],pYUYV[3]);
            pDstLine[x+1]=YUVToRGB32_Table(pYUYV[2],pYUYV[1],pYUYV[3]);
            pYUYV+=4;
        }
        ((TUInt8*&)pDstLine)+=DstPic.byte_width;
    }    
}

////////////
//全颜色查表
static TUInt32 _color_table_Blue[256*3];
static TUInt32 _color_table_Green[256*3];
static TUInt32 _color_table_Red_FF[256*3];
static const TUInt32* color_table_Blue=&_color_table_Blue[256];
static const TUInt32* color_table_Green=&_color_table_Green[256];
static const TUInt32* color_table_Red_FF=&_color_table_Red_FF[256];

class _CAuto_inti_color_table_ALL
{
public:
    _CAuto_inti_color_table_ALL() {
        for (int i=0;i<256*3;++i)
        {
            _color_table_Blue[i]=color_table[i];
            _color_table_Green[i]=color_table[i]<<8;
            _color_table_Red_FF[i]=(color_table[i]<<16) | (0xFF<<24);
        }
    }
};
static _CAuto_inti_color_table_ALL _Auto_inti_color_table_ALL;



//全查表
static int Ym_tableEx[256];
static int Um_blue_tableEx[256];
static int Um_green_tableEx[256];
static int Vm_green_tableEx[256];
static int Vm_red_tableEx[256];

class _CAuto_inti_yuv_tableEx
{
public:
    _CAuto_inti_yuv_tableEx() {
        for (int i=0;i<256;++i)
        {
            Ym_tableEx[i]=(csY_coeff_16 * (i - 16) )>>16;
            Um_blue_tableEx[i]=(csU_blue_16 * (i - 128) )>>16;
            Um_green_tableEx[i]=(csU_green_16 * (i - 128) )>>16;
            Vm_green_tableEx[i]=(csV_green_16 * (i - 128) )>>16;
            Vm_red_tableEx[i]=(csV_red_16 * (i - 128) )>>16;
        }
    }
};
static _CAuto_inti_yuv_tableEx _Auto_inti_yuv_tableEx;

    __forceinline void YUVToRGB32_Two_TableEx(TARGB32* pDst,const TUInt8 Y0,const TUInt8 Y1,const TUInt8 U,const TUInt8 V)
    {
        int Ye0=Ym_tableEx[Y0]; 
        int Ye1=Ym_tableEx[Y1];
        int Ue_blue=Um_blue_tableEx[U];
        int Ue_green=Um_green_tableEx[U];
        int Ve_green=Vm_green_tableEx[V];
        int Ve_red=Vm_red_tableEx[V];
        int UeVe_green=Ue_green+Ve_green;

        ((TUInt32*)pDst)[0]=color_table[ ( Ye0 + Ue_blue ) ] 
                    | ( color_table[ ( Ye0 + UeVe_green )]<<8 )
                    | ( color_table[ ( Ye0 + Ve_red )]<<16 )
                    | ( 255<<24);
        ((TUInt32*)pDst)[1]=color_table[ ( Ye1 + Ue_blue ) ] 
                    | ( color_table[ ( Ye1 + UeVe_green )]<<8 )
                    | ( color_table[ ( Ye1 + Ve_red )]<<16 )
                    | ( 255<<24);
    }

    void DECODE_YUYV_TableEx_line(TARGB32* pDstLine,const TUInt8* pYUYV,long width)
    {
        for (long x=0;x<width;x+=2)
        {
            YUVToRGB32_Two_TableEx(&pDstLine[x],pYUYV[0],pYUYV[2],pYUYV[1],pYUYV[3]);
            pYUYV+=4;
        }
    }


void DECODE_YUYV_TableEx(const TUInt8* pYUYV,const TPicRegion& DstPic)
{
    assert((DstPic.width & 1)==0); 
    
    long YUV_byte_width=(DstPic.width>>1)<<2;
    TARGB32* pDstLine=DstPic.pdata; 
    for (long y=0;y<DstPic.height;++y)
    {
        DECODE_YUYV_TableEx_line(pDstLine,pYUYV,DstPic.width);
        pYUYV+=YUV_byte_width;
        ((TUInt8*&)pDstLine)+=DstPic.byte_width;
    }    
}



//普通版的终版

    __forceinline void YUVToRGB32_Two(TARGB32* pDst,const TUInt8 Y0,const TUInt8 Y1,const TUInt8 U,const TUInt8 V)
    {
        int Ye0=csY_coeff_16 * (Y0 - 16); 
        int Ye1=csY_coeff_16 * (Y1 - 16);
        int Ue=(U-128);
        int Ue_blue=csU_blue_16 *Ue;
        int Ue_green=csU_green_16 *Ue;
        int Ve=(V-128);
        int Ve_green=csV_green_16 *Ve;
        int Ve_red=csV_red_16 *Ve;
        int UeVe_green=Ue_green+Ve_green;

        ((TUInt32*)pDst)[0]=color_table[ ( Ye0 + Ue_blue )>>16 ] 
                    | ( color_table[ ( Ye0 + UeVe_green )>>16]<<8 )
                    | ( color_table[ ( Ye0 + Ve_red )>>16]<<16 )
                    | ( 255<<24);
        ((TUInt32*)pDst)[1]=color_table[ ( Ye1 + Ue_blue )>>16 ] 
                    | ( color_table[ ( Ye1 + UeVe_green )>>16]<<8 )
                    | ( color_table[ ( Ye1 + Ve_red )>>16]<<16 )
                    | ( 255<<24);
                    
        /*
        ((TUInt32*)pDst)[0]=color_table_Blue[ ( Ye0 + Ue_blue )>>16 ] 
                    | ( color_table_Green[ ( Ye0 + Ue_green+Ve_green )>>16] )
                    | ( color_table_Red_FF[ ( Ye0 + Ve_red )>>16] );
        ((TUInt32*)pDst)[1]=color_table_Blue[ ( Ye1 + Ue_blue )>>16 ] 
                    | ( color_table_Green[ ( Ye1 + Ue_green+Ve_green )>>16] )
                    | ( color_table_Red_FF[ ( Ye1 + Ve_red )>>16] );*/
    }

    void DECODE_YUYV_Common_line(TARGB32* pDstLine,const TUInt8* pYUYV,long width)
    {
        for (long x=0;x<width;x+=2)
        {
            YUVToRGB32_Two(&pDstLine[x],pYUYV[0],pYUYV[2],pYUYV[1],pYUYV[3]);
            pYUYV+=4;
        }
    }

void DECODE_YUYV_Common(const TUInt8* pYUYV,const TPicRegion& DstPic)
{
    assert((DstPic.width & 1)==0); 
    
    long YUV_byte_width=(DstPic.width>>1)<<2;
    TARGB32* pDstLine=DstPic.pdata; 
    for (long y=0;y<DstPic.height;++y)
    {
        DECODE_YUYV_Common_line(pDstLine,pYUYV,DstPic.width);
        pYUYV+=YUV_byte_width;
        ((TUInt8*&)pDstLine)+=DstPic.byte_width;
    }    
}


//MMX版

typedef unsigned __int64  UInt64;

const  UInt64   csMMX_16_b      = 0x1010101010101010; // byte{16,16,16,16,16,16,16,16}
const  UInt64   csMMX_128_w     = 0x0080008000800080; //short{  128,  128,  128,  128}
const  UInt64   csMMX_0x00FF_w  = 0x00FF00FF00FF00FF; //掩码
const  UInt64   csMMX_Y_coeff_w = 0x2543254325432543; //short{ 9539, 9539, 9539, 9539} =1.164383*(1<<13)
const  UInt64   csMMX_U_blue_w  = 0x408D408D408D408D; //short{16525,16525,16525,16525} =2.017232*(1<<13)
const  UInt64   csMMX_U_green_w = 0xF377F377F377F377; //short{-3209,-3209,-3209,-3209} =(-0.391762)*(1<<13)
const  UInt64   csMMX_V_green_w = 0xE5FCE5FCE5FCE5FC; //short{-6660,-6660,-6660,-6660} =(-0.812968)*(1<<13)
const  UInt64   csMMX_V_red_w   = 0x3313331333133313; //short{13075,13075,13075,13075} =1.596027*(1<<13)

//一次处理8个颜色输出
#define YUV422ToRGB32_MMX(out_RGB_reg,WriteCode)                                                 \
   /*input :  mm0 = Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0    */                                                \
   /*         mm1 = 00 u3 00 u2 00 u1 00 u0    */                                                \
   /*         mm2 = 00 v3 00 v2 00 v1 00 v0    */                                                \
   /*output : [out_RGB_reg -- out_RGB_reg+8*4]                 */                                \
                                                                                                 \
          asm   psubusb     mm0,csMMX_16_b        /* mm0 : Y -= 16                       */      \
          asm   psubsw      mm1,csMMX_128_w       /* mm1 : u -= 128                      */      \
          asm   movq        mm7,mm0                                                              \
          asm   psubsw      mm2,csMMX_128_w       /* mm2 : v -= 128                      */      \
          asm   pand        mm0,csMMX_0x00FF_w    /* mm0 = 00 Y6 00 Y4 00 Y2 00 Y0       */      \
          asm   psllw       mm1,3                 /* mm1 : u *= 8                        */      \
          asm   psllw       mm2,3                 /* mm2 : v *= 8                        */      \
          asm   psrlw       mm7,8                 /* mm7 = 00 Y7 00 Y5 00 Y3 00 Y1       */      \
          asm   movq        mm3,mm1                                                              \
          asm   movq        mm4,mm2                                                              \
                                                                                                 \
          asm   pmulhw      mm1,csMMX_U_green_w   /* mm1 = u * U_green                   */      \
          asm   psllw       mm0,3                 /* y*=8                                */      \
          asm   pmulhw      mm2,csMMX_V_green_w   /* mm2 = v * V_green                   */      \
          asm   psllw       mm7,3                 /* y*=8                                */      \
          asm   pmulhw      mm3,csMMX_U_blue_w                                                   \
          asm   paddsw      mm1,mm2                                                              \
          asm   pmulhw      mm4,csMMX_V_red_w                                                    \
          asm   movq        mm2,mm3                                                              \
          asm   pmulhw      mm0,csMMX_Y_coeff_w                                                  \
          asm   movq        mm6,mm4                                                              \
          asm   pmulhw      mm7,csMMX_Y_coeff_w                                                  \
          asm   movq        mm5,mm1                                                              \
          asm   paddsw      mm3,mm0               /* mm3 = B6 B4 B2 B0       */                  \
          asm   paddsw      mm2,mm7               /* mm2 = B7 B5 B3 B1       */                  \
          asm   paddsw      mm4,mm0               /* mm4 = R6 R4 R2 R0       */                  \
          asm   paddsw      mm6,mm7               /* mm6 = R7 R5 R3 R1       */                  \
          asm   paddsw      mm1,mm0               /* mm1 = G6 G4 G2 G0       */                  \
          asm   paddsw      mm5,mm7               /* mm5 = G7 G5 G3 G1       */                  \
                                                                                                 \
          asm   packuswb    mm3,mm4               /* mm3 = R6 R4 R2 R0 B6 B4 B2 B0 to [0-255] */ \
          asm   packuswb    mm2,mm6               /* mm2 = R7 R5 R3 R1 B7 B5 B3 B1 to [0-255] */ \
          asm   packuswb    mm5,mm1               /* mm5 = G6 G4 G2 G0 G7 G5 G3 G1 to [0-255] */ \
          asm   movq        mm4,mm3                                                              \
          asm   punpcklbw   mm3,mm2               /* mm3 = B7 B6 B5 B4 B3 B2 B1 B0     */        \
          asm   punpckldq   mm1,mm5               /* mm1 = G7 G5 G3 G1 xx xx xx xx     */        \
          asm   punpckhbw   mm4,mm2               /* mm4 = R7 R6 R5 R4 R3 R2 R1 R0     */        \
          asm   punpckhbw   mm5,mm1               /* mm5 = G7 G6 G5 G4 G3 G2 G1 G0     */        \
                                                                                                 \
                /*out*/                                                                          \
          asm   pcmpeqb     mm2,mm2               /* mm2 = FF FF FF FF FF FF FF FF     */        \
                                                                                                 \
          asm   movq        mm0,mm3                                                              \
          asm   movq        mm7,mm4                                                              \
          asm   punpcklbw   mm0,mm5             /* mm0 = G3 B3 G2 B2 G1 B1 G0 B0       */        \
          asm   punpcklbw   mm7,mm2             /* mm7 = FF R3 FF R2 FF R1 FF R0       */        \
          asm   movq        mm1,mm0                                                              \
          asm   movq        mm6,mm3                                                              \
          asm   punpcklwd   mm0,mm7             /* mm0 = FF R1 G1 B1 FF R0 G0 B0       */        \
          asm   punpckhwd   mm1,mm7             /* mm1 = FF R3 G3 B3 FF R2 G2 B2       */        \
          asm   WriteCode   [out_RGB_reg],mm0                                                    \
          asm   movq        mm7,mm4                                                              \
          asm   punpckhbw   mm6,mm5             /* mm6 = G7 B7 G6 B6 G5 B5 G4 B4       */        \
          asm   WriteCode   [out_RGB_reg+8],mm1                                                  \
          asm   punpckhbw   mm7,mm2             /* mm7 = FF R7 FF R6 FF R5 FF R4      */         \
          asm   movq        mm0,mm6                                                              \
          asm   punpcklwd   mm6,mm7             /* mm6 = FF R5 G5 B5 FF R4 G4 B4      */         \
          asm   punpckhwd   mm0,mm7             /* mm0 = FF R7 G7 B7 FF R6 G6 B6      */         \
          asm   WriteCode  [out_RGB_reg+8*2],mm6                                                 \
          asm   WriteCode  [out_RGB_reg+8*3],mm0                       
              

    #define YUYV_Loader_MMX(in_yuv_reg)                                                         \
          asm   movq        mm0,[in_yuv_reg  ] /*mm0=V1 Y3 U1 Y2 V0 Y1 U0 Y0  */                \
          asm   movq        mm4,[in_yuv_reg+8] /*mm4=V3 Y7 U3 Y6 V2 Y5 U2 Y4  */                \
          asm   movq        mm1,mm0                                                             \
          asm   movq        mm5,mm4                                                             \
          asm   psrlw       mm1,8              /*mm1=00 V1 00 U1 00 V0 00 U0  */                \
          asm   psrlw       mm5,8              /*mm5=00 V3 00 U3 00 V2 00 U2  */                \
          asm   pand        mm0,csMMX_0x00FF_w /*mm0=00 Y3 00 Y2 00 Y1 00 Y0  */                \
          asm   pand        mm4,csMMX_0x00FF_w /*mm4=00 Y7 00 Y6 00 Y5 00 Y4  */                \
          asm   packuswb    mm1,mm5            /*mm1=V3 U3 V2 U2 V1 U1 V0 U0  */                \
          asm   movq        mm2,mm1                                                             \
          asm   packuswb    mm0,mm4            /*mm0=Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0  */                \
          asm   psllw       mm1,8              /*mm1=U3 00 U2 00 U1 00 U0 00  */                \
          asm   psrlw       mm2,8              /*mm2=00 V3 00 V2 00 V1 00 V0  */                \
          asm   psrlw       mm1,8              /*mm1=00 U3 00 U2 00 U1 00 U0  */                
  
 
    void DECODE_YUYV_MMX_line(TARGB32* pDstLine,const TUInt8* pYUYV,long width)
    {
        long expand8_width=(width>>3)<<3;
        
        if (expand8_width>0)
        {
            asm
            {
                mov     ecx,expand8_width
                mov     eax,pYUYV
                mov     edx,pDstLine
                lea     eax,[eax+ecx*2]
                lea     edx,[edx+ecx*4]
                neg     ecx
                
              loop_beign:
                YUYV_Loader_MMX(eax+ecx*2)
                YUV422ToRGB32_MMX(edx+ecx*4,movq)

                add     ecx,8
                jnz     loop_beign

                mov     pYUYV,eax
                mov     pDstLine,edx
            }
        }

        //处理边界
        DECODE_YUYV_Common_line(pDstLine,pYUYV,width-expand8_width);
    }

void DECODE_YUYV_MMX(const TUInt8* pYUYV,const TPicRegion& DstPic)
{
    assert((DstPic.width & 1)==0); 
    
    long YUV_byte_width=(DstPic.width>>1)<<2;
    TARGB32* pDstLine=DstPic.pdata; 
    for (long y=0;y<DstPic.height;++y)
    {
        DECODE_YUYV_MMX_line(pDstLine,pYUYV,DstPic.width);
        pYUYV+=YUV_byte_width;
        ((TUInt8*&)pDstLine)+=DstPic.byte_width;
    }    
    asm emms
}


    //使用软件预读和禁止写缓存优化
    #define  YUV422ToRGB32_SSE(out_RGB_reg) YUV422ToRGB32_MMX(out_RGB_reg,movntq)

    void DECODE_YUYV_SSE_line(TARGB32* pDstLine,const TUInt8* pYUYV,long width)
    {
        long expand8_width=(width>>3)<<3;
        
        if (expand8_width>0)
        {
            asm
            {
                mov     ecx,expand8_width
                mov     eax,pYUYV
                mov     edx,pDstLine
                lea     eax,[eax+ecx*2]
                lea     edx,[edx+ecx*4]
                neg     ecx
                
              loop_beign:
                YUYV_Loader_MMX(eax+ecx*2)
                prefetchnta [eax+ecx*2+64*4*2]  //预读
                YUV422ToRGB32_SSE(edx+ecx*4)

                add     ecx,8
                jnz     loop_beign

                mov     pYUYV,eax
                mov     pDstLine,edx
            }
        }

        //处理边界
        DECODE_YUYV_Common_line(pDstLine,pYUYV,width-expand8_width);
    }

void DECODE_YUYV_SSE(const TUInt8* pYUYV,const TPicRegion& DstPic)
{
    assert((DstPic.width & 1)==0); 
    
    long YUV_byte_width=(DstPic.width>>1)<<2;
    TARGB32* pDstLine=DstPic.pdata; 
    for (long y=0;y<DstPic.height;++y)
    {
        DECODE_YUYV_SSE_line(pDstLine,pYUYV,DstPic.width);
        pYUYV+=YUV_byte_width;
        ((TUInt8*&)pDstLine)+=DstPic.byte_width;
    } 
    asm sfence
    asm emms
}

    void DECODE_YUYV_SSE_Ex_line(TARGB32* pDstLine,const TUInt8* pYUYV,long width)
    {
        long expand32_width=(width>>5)<<5;
        
        if (expand32_width>0)
        {
            asm
            {
                mov     ecx,expand32_width
                mov     eax,pYUYV
                mov     edx,pDstLine
                lea     eax,[eax+ecx*2]
                lea     edx,[edx+ecx*4]
                neg     ecx
                
              loop_beign:
                YUYV_Loader_MMX(eax+ecx*2)
                YUV422ToRGB32_SSE(edx+ecx*4)
                prefetchnta [eax+ecx*2+64*4] 
                YUYV_Loader_MMX(eax+ecx*2+8*2)
                YUV422ToRGB32_SSE(edx+ecx*4+8*4)
                YUYV_Loader_MMX(eax+ecx*2+8*2*2)
                YUV422ToRGB32_SSE(edx+ecx*4+8*4*2)
                YUYV_Loader_MMX(eax+ecx*2+8*2*3)
                YUV422ToRGB32_SSE(edx+ecx*4+8*4*3)

                add     ecx,8*4
                jnz     loop_beign

                mov     pYUYV,eax
                mov     pDstLine,edx
            }
        }

        //处理边界
        DECODE_YUYV_SSE_line(pDstLine,pYUYV,width-expand32_width);
    }

void DECODE_YUYV_SSE_Ex(const TUInt8* pYUYV,const TPicRegion& DstPic)
{
    assert((DstPic.width & 1)==0); 
    
    long YUV_byte_width=(DstPic.width>>1)<<2;
    TARGB32* pDstLine=DstPic.pdata; 
    for (long y=0;y<DstPic.height;++y)
    {
        DECODE_YUYV_SSE_Ex_line(pDstLine,pYUYV,DstPic.width);
        pYUYV+=YUV_byte_width;
        ((TUInt8*&)pDstLine)+=DstPic.byte_width;
    } 
    asm sfence
    asm emms
}


__declspec(align(16))
typedef UInt64 TSSE2Int[2];
const  TSSE2Int   csSSE2_16_b      = {csMMX_16_b     ,csMMX_16_b     }; 
const  TSSE2Int   csSSE2_128_w     = {csMMX_128_w    ,csMMX_128_w    }; 
const  TSSE2Int   csSSE2_0x00FF_w  = {csMMX_0x00FF_w ,csMMX_0x00FF_w }; 
const  TSSE2Int   csSSE2_Y_coeff_w = {csMMX_Y_coeff_w,csMMX_Y_coeff_w};
const  TSSE2Int   csSSE2_U_blue_w  = {csMMX_U_blue_w ,csMMX_U_blue_w };
const  TSSE2Int   csSSE2_U_green_w = {csMMX_U_green_w,csMMX_U_green_w};
const  TSSE2Int   csSSE2_V_green_w = {csMMX_V_green_w,csMMX_V_green_w};
const  TSSE2Int   csSSE2_V_red_w   = {csMMX_V_red_w  ,csMMX_V_red_w  };

#define YUV422ToRGB32_SSE2                                                                          \
   /*input :  mm0 = Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0   */                                                    \
   /*         mm1 = 00 u3 00 u2 00 u1 00 u0   */                                                    \
   /*         mm2 = 00 v3 00 v2 00 v1 00 v0   */                                                    \
   /*output : [edx -- edx+16*4]               */                                                    \
                                                                                                    \
          asm   psubusb     xmm0,csSSE2_16_b        /* xmm0 : Y -= 16                       */      \
          asm   psubsw      xmm1,csSSE2_128_w       /* xmm1 : u -= 128                      */      \
          asm   movdqa      xmm7,xmm0                                                               \
          asm   psubsw      xmm2,csSSE2_128_w       /* xmm2 : v -= 128                      */      \
          asm   pand        xmm0,csSSE2_0x00FF_w    /* xmm0 =  0  Y6  0  Y4  0  Y2  0  Y0   */      \
          asm   psllw       xmm1,3                 /* xmm1 : u *= 8                        */       \
          asm   psllw       xmm2,3                 /* xmm2 : v *= 8                        */       \
          asm   psrlw       xmm7,8                 /* xmm7 =  0  Y7  0  Y5  0  Y3  0  Y1   */       \
          asm   movdqa      xmm3,xmm1                                                               \
          asm   movdqa      xmm4,xmm2                                                               \
                                                                                                    \
          asm   pmulhw      xmm1,csSSE2_U_green_w   /* xmm1 = u * U_green                   */      \
          asm   psllw       xmm0,3                 /* y*=8                                 */       \
          asm   pmulhw      xmm2,csSSE2_V_green_w   /* xmm2 = v * V_green                   */      \
          asm   psllw       xmm7,3                 /* y*=8                                 */       \
          asm   pmulhw      xmm3,csSSE2_U_blue_w                                                    \
          asm   paddsw      xmm1,xmm2                                                               \
          asm   pmulhw      xmm4,csSSE2_V_red_w                                                     \
          asm   movdqa      xmm2,xmm3                                                               \
          asm   pmulhw      xmm0,csSSE2_Y_coeff_w                                                   \
          asm   movdqa      xmm6,xmm4                                                               \
          asm   pmulhw      xmm7,csSSE2_Y_coeff_w                                                   \
          asm   movdqa      xmm5,xmm1                                                               \
          asm   paddsw      xmm3,xmm0               /* xmm3 = B6 B4 B2 B0       */                  \
          asm   paddsw      xmm2,xmm7               /* xmm2 = B7 B5 B3 B1       */                  \
          asm   paddsw      xmm4,xmm0               /* xmm4 = R6 R4 R2 R0       */                  \
          asm   paddsw      xmm6,xmm7               /* xmm6 = R7 R5 R3 R1       */                  \
          asm   paddsw      xmm1,xmm0               /* xmm1 = G6 G4 G2 G0       */                  \
          asm   paddsw      xmm5,xmm7               /* xmm5 = G7 G5 G3 G1       */                  \
                                                                                                    \
          asm   packuswb    xmm3,xmm4               /* xmm3 = R6 R4 R2 R0 B6 B4 B2 B0 to [0-255] */ \
          asm   packuswb    xmm2,xmm6               /* xmm2 = R7 R5 R3 R1 B7 B5 B3 B1 to [0-255] */ \
          asm   packuswb    xmm5,xmm1               /* xmm5 = G6 G4 G2 G0 G7 G5 G3 G1 to [0-255] */ \
          asm   movdqa      xmm4,xmm3                                                               \
          asm   punpcklbw   xmm3,xmm2               /* xmm3 = B7 B6 B5 B4 B3 B2 B1 B0     */        \
          asm   punpckldq   xmm1,xmm5               /* xmm1 = G7 G5 G3 G1 xx xx xx xx     */        \
          asm   punpckhbw   xmm4,xmm2               /* xmm4 = R7 R6 R5 R4 R3 R2 R1 R0     */        \
          asm   punpckhbw   xmm5,xmm1               /* xmm5 = G7 G6 G5 G4 G3 G2 G1 G0     */        \
                                                                                                    \
                /*out*/                                                                             \
          asm   pcmpeqb     xmm2,xmm2               /* xmm2 = FF FF FF FF FF FF FF FF     */        \
                                                                                                    \
          asm   movdqa      xmm0,xmm3                                                               \
          asm   movdqa      xmm7,xmm4                                                               \
          asm   punpcklbw   xmm0,xmm5             /* xmm0 = G3 B3 G2 B2 G1 B1 G0 B0       */        \
          asm   punpcklbw   xmm7,xmm2             /* xmm7 = FF R3 FF R2 FF R1 FF R0       */        \
          asm   movdqa      xmm1,xmm0                                                               \
          asm   movdqa      xmm6,xmm3                                                               \
          asm   punpcklwd   xmm0,xmm7             /* xmm0 = FF R1 G1 B1 FF R0 G0 B0       */        \
          asm   punpckhwd   xmm1,xmm7             /* xmm1 = FF R3 G3 B3 FF R2 G2 B2       */        \
          asm   movdqu  [edx],xmm0                                                                  \
          asm   movdqu      xmm7,xmm4                                                               \
          asm   punpckhbw   xmm6,xmm5             /* xmm6 = G7 B7 G6 B6 G5 B5 G4 B4       */        \
          asm   movdqu  [edx+16],xmm1                                                               \
          asm   punpckhbw   xmm7,xmm2             /* xmm7 = FF R7 FF R6 FF R5 FF R4      */         \
          asm   movdqa      xmm0,xmm6                                                               \
          asm   punpcklwd   xmm6,xmm7             /* xmm6 = FF R5 G5 B5 FF R4 G4 B4      */         \
          asm   punpckhwd   xmm0,xmm7             /* xmm0 = FF R7 G7 B7 FF R6 G6 B6      */         \
          asm   movdqu  [edx+16*2],xmm6                                                             \
          asm   movdqu  [edx+16*3],xmm0                       



//一次处理16个颜色输出

    void DECODE_YUYV_SSE2_line(TARGB32* pDstLine,const TUInt8* pYUYV,long width)
    {
        long expand16_width=(width>>4)<<4;
        
        if (expand16_width>0)
        {
            asm
            {
                mov     ecx,expand16_width
                mov     eax,pYUYV
                mov     edx,pDstLine
                
              loop_beign:
                //int 3
                movdqu      xmm0,[eax   ]       //xmm0=V3 Y07 U3 Y06 V2 Y05 U2 Y04 V1 Y03 U1 Y02 V0 Y01 U0 Y00
                movdqu      xmm4,[eax+16]       //xmm4=V7 Y15 U7 Y14 V6 Y13 U6 Y12 V5 Y11 U5 Y10 V4 Y09 U4 Y08
                movdqa      xmm1,xmm0
                movdqa      xmm5,xmm4
                psrlw       xmm1,8              //xmm1=00 V3 00 U3 00 V2 00 U2 00 V1 00 U1 00 V0 00 U0
                psrlw       xmm5,8              //xmm5=00 V7 00 U7 00 V6 00 U6 00 V5 00 U5 00 V4 00 U4
                pand        xmm0,csSSE2_0x00FF_w//xmm0=00 Y07 00 Y06 00 Y05 00 Y04 00 Y03 00 Y02 00 Y01 00 Y00 
                pand        xmm4,csSSE2_0x00FF_w//xmm4=00 Y15 00 Y14 00 Y13 00 Y12 00 Y11 00 Y10 00 Y09 00 Y08 
                packuswb    xmm1,xmm5           //xmm1=V7 U7 V6 U6 V5 U5 V4 U4 V3 U3 V2 U2 V1 U1 V0 U0 
                movdqa      xmm2,xmm1            
                packuswb    xmm0,xmm4           //xmm0=Y15 Y14 Y13 Y12 Y11 Y10 Y09 Y08 Y07 Y06 Y05 Y04 Y03 Y02 Y01 Y00
                psllw       xmm1,8              //xmm1=U7 00 U6 00 U5 00 U4 00 U3 00 U2 00 U1 00 U0 00
                psrlw       xmm2,8              //xmm2=00 V7 00 V6 00 V5 00 V4 00 V3 00 V2 00 V1 00 V0
                psrlw       xmm1,8              //xmm1=00 U7 00 U6 00 U5 00 U4 00 U3 00 U2 00 U1 00 U0

                //input :  xmm0 = Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0
                //         xmm1 = 00 u3 00 u2 00 u1 00 u0
                //         xmm2 = 00 v3 00 v2 00 v1 00 v0
                //output : [edx -- edx+16*4] 
                //YUV422ToRGB32_SSE2
                /////////////////////////

          asm   psubusb     xmm0,csSSE2_16_b        /* xmm0 : Y -= 16                       */   
          asm   psubsw      xmm1,csSSE2_128_w       /* xmm1 : u -= 128                      */   
          asm   movdqa        xmm7,xmm0                                                          
          asm   psubsw      xmm2,csSSE2_128_w       /* xmm2 : v -= 128                      */   
          asm   pand        xmm0,csSSE2_0x00FF_w    /* xmm0 = 00 Y6 00 Y4 00 Y2 00 Y0       */   
          asm   psllw       xmm1,3                 /* xmm1 : u *= 8                        */    
          asm   psllw       xmm2,3                 /* xmm2 : v *= 8                        */    
          asm   psrlw       xmm7,8                 /* xmm7 = 00 Y7 00 Y5 00 Y3 00 Y1       */    
          asm   movdqa        xmm3,xmm1                                                          
          asm   movdqa        xmm4,xmm2                                                          
                                                                                                 
          asm   pmulhw      xmm1,csSSE2_U_green_w   /* xmm1 = u * U_green                   */   
          asm   psllw       xmm0,3                 /* y*=8                                */     
          asm   pmulhw      xmm2,csSSE2_V_green_w   /* xmm2 = v * V_green                   */   
          asm   psllw       xmm7,3                 /* y*=8                                */     
          asm   pmulhw      xmm3,csSSE2_U_blue_w                                                 
          asm   paddsw      xmm1,xmm2                                                            
          asm   pmulhw      xmm4,csSSE2_V_red_w                                                  
          asm   movdqa        xmm2,xmm3                                                          
          asm   pmulhw      xmm0,csSSE2_Y_coeff_w                                                
          asm   movdqa        xmm6,xmm4                                                          
          asm   pmulhw      xmm7,csSSE2_Y_coeff_w                                                
          asm   movdqa        xmm5,xmm1                                                          
          asm   paddsw      xmm3,xmm0               /* xmm3 = B6 B4 B2 B0       */               
          asm   paddsw      xmm2,xmm7               /* xmm2 = B7 B5 B3 B1       */               
          asm   paddsw      xmm4,xmm0               /* xmm4 = R6 R4 R2 R0       */               
          asm   paddsw      xmm6,xmm7               /* xmm6 = R7 R5 R3 R1       */               
          asm   paddsw      xmm1,xmm0               /* xmm1 = G6 G4 G2 G0       */               
          asm   paddsw      xmm5,xmm7               /* xmm5 = G7 G5 G3 G1       */               
                                                                                                 
          asm   packuswb    xmm3,xmm4               /* xmm3 = R6 R4 R2 R0 B6 B4 B2 B0 to [0-255] */ 
          asm   packuswb    xmm2,xmm6               /* xmm2 = R7 R5 R3 R1 B7 B5 B3 B1 to [0-255] */ 
          asm   packuswb    xmm5,xmm1               /* xmm5 = G6 G4 G2 G0 G7 G5 G3 G1 to [0-255] */ 
          asm   movdqa        xmm4,xmm3                                                             
          asm   punpcklbw   xmm3,xmm2               /* xmm3 = B7 B6 B5 B4 B3 B2 B1 B0     */        
          asm   punpckldq   xmm1,xmm5               /* xmm1 = G7 G5 G3 G1 xx xx xx xx     */        
          asm   punpckhbw   xmm4,xmm2               /* xmm4 = R7 R6 R5 R4 R3 R2 R1 R0     */        
          asm   punpckhbw   xmm5,xmm1               /* xmm5 = G7 G6 G5 G4 G3 G2 G1 G0     */    
                                                                                                
                /*out*/                                                                         
          asm   pcmpeqb     xmm2,xmm2               /* xmm2 = FF FF FF FF FF FF FF FF     */    
                                                                                                
          asm   movdqa        xmm0,xmm3                                                         
          asm   movdqa        xmm7,xmm4                                                         
          asm   punpcklbw   xmm0,xmm5             /* xmm0 = G3 B3 G2 B2 G1 B1 G0 B0       */    
          asm   punpcklbw   xmm7,xmm2             /* xmm7 = FF R3 FF R2 FF R1 FF R0       */    
          asm   movdqa        xmm1,xmm0                                                         
          asm   movdqa        xmm6,xmm3                                                         
          asm   punpcklwd   xmm0,xmm7             /* xmm0 = FF R1 G1 B1 FF R0 G0 B0       */    
          asm   punpckhwd   xmm1,xmm7             /* xmm1 = FF R3 G3 B3 FF R2 G2 B2       */    
          asm   movdqu   [edx],xmm0                                                             
          asm   movdqa        xmm7,xmm4                                                         
          asm   punpckhbw   xmm6,xmm5             /* xmm6 = G7 B7 G6 B6 G5 B5 G4 B4       */    
          asm   movdqu   [edx+16],xmm1                                                          
          asm   punpckhbw   xmm7,xmm2             /* xmm7 = FF R7 FF R6 FF R5 FF R4      */     
          asm   movdqa        xmm0,xmm6                                                         
          asm   punpcklwd   xmm6,xmm7             /* xmm6 = FF R5 G5 B5 FF R4 G4 B4      */     
          asm   punpckhwd   xmm0,xmm7             /* xmm0 = FF R7 G7 B7 FF R6 G6 B6      */     
          asm   movdqu  [edx+16*2],xmm6                                                         
          asm   movdqu  [edx+16*3],xmm0                       




                /////////////////////////


                add     eax,16*2
                add     edx,16*4
                sub     ecx,16
                jnz     loop_beign
                //
            }
        }

        //处理边界
        pYUYV+=( expand16_width<<1 );
        for (long x=expand16_width;x<width;x+=2)
        {
            YUVToRGB32_Two(&pDstLine[x],pYUYV[0],pYUYV[2],pYUYV[1],pYUYV[3]);
            pYUYV+=4;
        }
    }

void DECODE_YUYV_SSE2(const TUInt8* pYUYV,const TPicRegion& DstPic)
{
    assert((DstPic.width & 1)==0); 
    
    long YUV_byte_width=(DstPic.width>>1)<<2;
    TARGB32* pDstLine=DstPic.pdata; 
    for (long y=0;y<DstPic.height;++y)
    {
        DECODE_YUYV_SSE2_line(pDstLine,pYUYV,DstPic.width);
        pYUYV+=YUV_byte_width;
        ((TUInt8*&)pDstLine)+=DstPic.byte_width;
    }    
    asm sfence
    asm emms 
}


    bool  _CPUSupportCPUID()
    {
        long int CPUIDInfOld=0;
        long int CPUIDInfNew=0;

      try
      {
        asm
        {
            pushfd                   // 保存原 EFLAGS
            pop     eax
            mov     edx,eax
            mov     CPUIDInfOld,eax  //

            xor     eax,00200000h    // 改写 第21位
            push    eax
            popfd                    // 改写 EFLAGS

            pushfd                   // 保存新 EFLAGS
            pop     eax              
            mov     CPUIDInfNew,eax

            push    edx              // 恢复原 EFLAGS
            popfd
        }
        return (CPUIDInfOld!=CPUIDInfNew);  // EFLAGS 第21位 可以改写
      }
      catch(...)
      {
	    return false;
      }
    }

    bool  _CPUSupportMMX()  //判断CPU是否支持MMX指令
    {

      if (!_CPUSupportCPUID())
        return false;

      long int MMXInf=0;

      try
      {
        asm
	    {
          push  ebx
          mov   eax,1
          cpuid
          mov   MMXInf,edx
          pop   ebx
        }
        MMXInf=MMXInf & (1 << 23);  //检测edx第23位
        return (MMXInf==(1 << 23));
      }
      catch(...)
      {
        return false;
      }
    }

    bool  _CPUSupportSSE()  //判断CPU是否支持SSE指令
    {

      if (!_CPUSupportCPUID())
        return false;

      long int SSEInf=0;
      try
      {
        asm
	    {
          push  ebx
          mov   eax,1
          cpuid
          mov   SSEInf,edx
          pop   ebx
        }
        SSEInf=SSEInf & (1 << 25);  //检测edx第25位
        return  (SSEInf==(1 << 25));
      }
      catch(...)
      {
        return false;
      }
    }


    bool  _SystemSupportSSE()  //判断操作系统是否支持SSE指令
    {
      //触发异常来判断
      try
      {
        asm
        {
            //movups     xmm0,xmm0
            asm _emit 0x0F asm _emit 0x10 asm _emit 0xC0
        }
        return true;
      }
      catch(...)
      {
        return false;
      }
    }


const bool _IS_MMX_ACTIVE=_CPUSupportMMX();
const bool _IS_SSE_ACTIVE=_CPUSupportSSE() && _SystemSupportSSE();

typedef void (*TDECODE_YUYV_line_proc)(TARGB32* pDstLine,const TUInt8* pYUYV,long width);

const TDECODE_YUYV_line_proc DECODE_YUYV_Auto_line=
        ( _IS_MMX_ACTIVE ? (_IS_SSE_ACTIVE ? DECODE_YUYV_SSE_line : DECODE_YUYV_MMX_line) : DECODE_YUYV_Common_line );

__forceinline void DECODE_filish()
{
    if (_IS_MMX_ACTIVE)
    {
        if (_IS_SSE_ACTIVE) {  asm sfence }
        asm emms
    }
}

void DECODE_YUYV_Auto(const TUInt8* pYUYV,const TPicRegion& DstPic)
{
    assert((DstPic.width & 1)==0); 
    
    long YUV_byte_width=(DstPic.width>>1)<<2;
    TARGB32* pDstLine=DstPic.pdata; 
    for (long y=0;y<DstPic.height;++y)
    {
        DECODE_YUYV_Auto_line(pDstLine,pYUYV,DstPic.width);
        pYUYV+=YUV_byte_width;
        ((TUInt8*&)pDstLine)+=DstPic.byte_width;
    } 
    DECODE_filish();
}


////////////
//
#include "..\..\..\Common\WorkThreadPool.h"


struct TDECODE_YUYV_Parallel_WorkData
{
    const TUInt8* pYUYV;
    TPicRegion    DstPic;
};


void DECODE_YUYV_Parallel_callback(void* wd)
{
    TDECODE_YUYV_Parallel_WorkData* WorkData=(TDECODE_YUYV_Parallel_WorkData*)wd;
    DECODE_YUYV_Auto(WorkData->pYUYV,WorkData->DstPic);
}

void DECODE_YUYV_Parallel(const TUInt8* pYUYV,const TPicRegion& DstPic)
{
    long work_count=CWorkThreadPool::best_work_count();
    std::vector<TDECODE_YUYV_Parallel_WorkData>   work_list(work_count);
    std::vector<TDECODE_YUYV_Parallel_WorkData*>  pwork_list(work_count);
    long cheight=DstPic.height / work_count; 
    for (long i=0;i<work_count;++i)
    {
        work_list[i].pYUYV=pYUYV+i*cheight*(DstPic.width*2);
        work_list[i].DstPic.pdata=DstPic.pixel_pos(0,cheight*i);
        work_list[i].DstPic.byte_width=DstPic.byte_width;
        work_list[i].DstPic.width=DstPic.width;
        work_list[i].DstPic.height=cheight;
        pwork_list[i]=&work_list[i];
    }
    work_list[work_count-1].DstPic.height=DstPic.height-cheight*(work_count-1);
    CWorkThreadPool::work_execute(DECODE_YUYV_Parallel_callback,(void**)&pwork_list[0],work_count);
}

/////////////


    __forceinline void DECODE_YUYV_AutoLock_line(TARGB32* pDstLine,const TUInt8* pYUYV,long width,volatile long*  Lock)
    {
        //任务领取
        if ((*Lock)!=0) return;
        long lock_value=InterlockedIncrement(Lock);//也可以用带lock前缀的inc指令来代替这个windows调用 
        //警告: 在以后更多个核的电脑上，这里的lock造成的潜在冲突没有测试过
        if (lock_value>=2) return;
        //lock_value==1时，任务领取成功
        
        //执行任务
        DECODE_YUYV_Auto_line(pDstLine,pYUYV,width);
    }

    __forceinline void DECODE_YUYV_AutoEx(const TUInt8* pYUYV,const TPicRegion& DstPic,volatile long* LockList,long  begin_y0)
    {
        assert((DstPic.width & 1)==0); 
        
        long YUV_byte_width=(DstPic.width>>1)<<2;
        TARGB32* pDstLine=DstPic.pdata; 
        long y;

        const TUInt8* pYUYV_b=pYUYV+(YUV_byte_width*begin_y0);
        TARGB32* pDstLine_b=(TARGB32*)(((TUInt8*)DstPic.pdata)+(DstPic.byte_width*begin_y0));
        for (y=begin_y0;y<DstPic.height;++y)
        {
            DECODE_YUYV_AutoLock_line(pDstLine_b,pYUYV_b,DstPic.width,&LockList[y]);
            pYUYV_b+=YUV_byte_width;
            ((TUInt8*&)pDstLine_b)+=DstPic.byte_width;
        } 
        for (y=0;y<begin_y0;++y)
        {
            DECODE_YUYV_AutoLock_line(pDstLine,pYUYV,DstPic.width,&LockList[y]);
            pYUYV+=YUV_byte_width;
            ((TUInt8*&)pDstLine)+=DstPic.byte_width;
        } 
        /*for (y=0;y<DstPic.height;++y)
        {
            DECODE_YUYV_AutoLock_line(pDstLine,pYUYV,DstPic.width,&LockList[y]);
            pYUYV+=YUV_byte_width;
            ((TUInt8*&)pDstLine)+=DstPic.byte_width;
        }
        */
        DECODE_filish();
    }

struct TDECODE_YUYV_ParallelEx_WorkData
{
    const TUInt8*   pYUYV;
    TPicRegion      DstPic;
    long*           LockList;
    long            begin_y0;
};

void DECODE_YUYV_ParallelEx_callback(void* wd)
{
    TDECODE_YUYV_ParallelEx_WorkData* WorkData=(TDECODE_YUYV_ParallelEx_WorkData*)wd;
    DECODE_YUYV_AutoEx(WorkData->pYUYV,WorkData->DstPic,(volatile long*)WorkData->LockList,WorkData->begin_y0);
}

void DECODE_YUYV_ParallelEx(const TUInt8* pYUYV,const TPicRegion& DstPic)
{
    long work_count=CWorkThreadPool::best_work_count();
    std::vector<TDECODE_YUYV_ParallelEx_WorkData>   work_list(work_count);
    std::vector<TDECODE_YUYV_ParallelEx_WorkData*>  pwork_list(work_count);
    std::vector<long>  lock_list(DstPic.height);
    for (long y=0;y<DstPic.height;++y)
        lock_list[y]=0;

    long cheight=DstPic.height / work_count; 
    for (long i=0;i<work_count;++i)
    {
        work_list[i].pYUYV=pYUYV;
        work_list[i].DstPic=DstPic;
        work_list[i].begin_y0=i*cheight;
        work_list[i].LockList=&lock_list[0];
        pwork_list[i]=&work_list[i];
    }
    CWorkThreadPool::work_execute(DECODE_YUYV_ParallelEx_callback,(void**)&pwork_list[0],work_count);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const long ptBMP=0;
const long ptJPG=1;
void LoadPic(Surface32& pic,const char* picname,long pictype=0)
{
   //loadbmp
    if (ptBMP==pictype)
    {
	    TBmpFileColor32 bmp(pic);
	    bmp.load_from(picname,true);
    }
    else if (ptJPG==pictype)
    {
	    TJpegFileColor32 jpg(pic);
	    jpg.load_from(picname);
   }
}
void save_pic(const char* picname,const TPicRegion&  ppic)
{
    Surface32 pic;
    pic.assign(ppic);
	TBmpFileColor32 bmp(pic);
	bmp.save_to(picname);
}

inline double fps_ft(double fps)
{
    fps=((long)(fps*10+0.5))*0.1;
    return fps;
}
inline std::string fps_space(double fps)
{
    fps=((long)(fps*10+0.5))*0.1;
    long spn=0;
    if (fps<10)   ++spn;
    if (fps<100)  ++spn;
    if (fps<1000) ++spn;
    std::string space_f(spn,' ');
    return space_f;
}

typedef void (*T_DECODE_YUYV_proc)(const TUInt8* pYUYV,const TPicRegion& DstPic);

  void run_test(const T_DECODE_YUYV_proc DECODE_YUYV,const char* srcFileName,const long csRunCount)
  {
	Surface32 picSrc;
	Surface32 picDst;
    TUInt8* pYUYV=0;

    LoadPic(picSrc,srcFileName);
    TPicRegion Src=picSrc.as_context();
    //Src.width-=2;

    pYUYV=new TUInt8[(Src.width*Src.height) *2];
    picDst.resize_and_clear(Size(Src.width,Src.height));
    Encode_RGB32ToYUYV(Src,pYUYV);
    TPicRegion Dst=picDst.as_context();
    

    clock_t t0=clock();

    for (long c=0;c<csRunCount;++c)
    {
        DECODE_YUYV(pYUYV,Dst);
    }
    t0=clock()-t0;

    save_pic("Pic\\DECODE_YUYV_result.bmp",Dst);

    double fps=csRunCount/(t0*1.0/CLOCKS_PER_SEC);
	std::cout<<fps_space(fps)<<fps_ft(fps)<<" FPS";

    delete []pYUYV;
  }

void test(const char* proc_name,T_DECODE_YUYV_proc DECODE_YUYV,const long csRunCount)
{
    std::cout<<proc_name<<" ( 1024x576) ";
    run_test(DECODE_YUYV,"Pic\\1024x576.bmp",csRunCount*4);
    std::cout<<std::endl;
    std::cout<<proc_name<<" (1920x1080) ";
    run_test(DECODE_YUYV,"Pic\\1920x1080.bmp",csRunCount);
    std::cout<<std::endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Encode_RGB32ToPlanarYUV411(const TPicRegion& Src,
                                TUInt8* pY,const long Y_byte_width,
                                TUInt8* pU,const long U_byte_width,
                                TUInt8* pV,const long V_byte_width)
{
    assert((Src.width & 1)==0);  //要求数据宽度必须是2的倍数

    for (long y=0;y<Src.height/2;++y)
    {
        TUInt8* pYn=&pY[Y_byte_width];
        for (long x=0;x<Src.width/2;++x)
        {
            TUInt8 U[4],V[4];
            RGB32ToYUV(Pixels(Src,x*2+0,y*2+0),pY [x*2+0],U[0],V[0]);
            RGB32ToYUV(Pixels(Src,x*2+1,y*2+0),pY [x*2+1],U[1],V[1]);
            RGB32ToYUV(Pixels(Src,x*2+0,y*2+1),pYn[x*2+0],U[2],V[2]);
            RGB32ToYUV(Pixels(Src,x*2+1,y*2+1),pYn[x*2+1],U[3],V[3]);
            
            pU[x]=(U[0]+U[1]+U[2]+U[3])/4;
            pV[x]=(V[0]+V[1]+V[2]+V[3])/4;
        }
        pY=&pY[Y_byte_width*2];
        pU=&pU[U_byte_width];
        pV=&pV[V_byte_width];
    }
}

////////////

    void DECODE_PlanarYUV111_Common_line(TARGB32* pDstLine,const TUInt8* pY,
                                         const TUInt8* pU,const TUInt8* pV,long width)
    {
        for (long x=0;x<width;++x)
            pDstLine[x]=YUVToRGB32_Int(pY[x],pU[x],pV[x]);
    }

void DECODE_PlanarYUV111_Common(const TUInt8* pY,const long Y_byte_width,
                                const TUInt8* pU,const long U_byte_width,
                                const TUInt8* pV,const long V_byte_width,
                                const TPicRegion& DstPic)
{
    assert((DstPic.width & 1)==0); 
    TARGB32* pDstLine=DstPic.pdata; 
    for (long y=0;y<DstPic.height;++y)
    {
        DECODE_PlanarYUV111_Common_line(pDstLine,pY,pU,pV,DstPic.width);
        ((TUInt8*&)pDstLine)+=DstPic.byte_width;
        pY+=Y_byte_width;
        pU+=U_byte_width;
        pV+=V_byte_width;
    }    
}

    void DECODE_PlanarYUV211_Common_line(TARGB32* pDstLine,const TUInt8* pY,
                                         const TUInt8* pU,const TUInt8* pV,long width)
    {
        for (long x=0;x<width;x+=2)
        {
            long x_uv=x>>1;
            YUVToRGB32_Two(&pDstLine[x],pY[x],pY[x+1],pU[x_uv],pV[x_uv]);
        }
    }

void DECODE_PlanarYUV211_Common(const TUInt8* pY,const long Y_byte_width,
                                const TUInt8* pU,const long U_byte_width,
                                const TUInt8* pV,const long V_byte_width,
                                const TPicRegion& DstPic)
{
    assert((DstPic.width & 1)==0); 
    TARGB32* pDstLine=DstPic.pdata; 
    for (long y=0;y<DstPic.height;++y)
    {
        DECODE_PlanarYUV211_Common_line(pDstLine,pY,pU,pV,DstPic.width);
        ((TUInt8*&)pDstLine)+=DstPic.byte_width;
        pY+=Y_byte_width;
        pU+=U_byte_width;
        pV+=V_byte_width;
    }
}

void DECODE_PlanarYUV411_Common(const TUInt8* pY,const long Y_byte_width,
                                const TUInt8* pU,const long U_byte_width,
                                const TUInt8* pV,const long V_byte_width,
                                const TPicRegion& DstPic)
{
    assert((DstPic.width & 1)==0); 
    TARGB32* pDstLine=DstPic.pdata; 
    for (long y=0;y<DstPic.height;++y)
    {
        DECODE_PlanarYUV211_Common_line(pDstLine,pY,pU,pV,DstPic.width);
        ((TUInt8*&)pDstLine)+=DstPic.byte_width;
        pY+=Y_byte_width;
        if ((y&1)==1)  //这里做了特殊处理，使Y下移两行的时候U、V才会下移一行
        {
            pU+=U_byte_width;
            pV+=V_byte_width;
        }
    }    
}


    #define PlanarYUV211_Loader_MMX(in_y_reg,in_u_reg,in_v_reg)                                 \
          asm   movd        mm1,[in_u_reg]     /*mm1=00 00 00 00 U3 U2 U1 U0  */                \
          asm   movd        mm2,[in_v_reg]     /*mm2=00 00 00 00 V3 V2 V1 V0  */                \
          asm   pxor        mm4,mm4            /*mm4=00 00 00 00 00 00 00 00  */                \
          asm   movq        mm0,[in_y_reg]     /*mm0=Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0  */                \
          asm   punpcklbw   mm1,mm4            /*mm1=00 U3 00 U2 00 U1 00 U0  */                \
          asm   punpcklbw   mm2,mm4            /*mm2=00 V3 00 V2 00 V1 00 V0  */                



    void DECODE_PlanarYUV211_MMX_line(TARGB32* pDstLine,const TUInt8* pY,
                                      const TUInt8* pU,const TUInt8* pV,long width)
    {
        long expand8_width=(width>>3)<<3;
        
        if (expand8_width>0)
        {
            asm
            {
                push    esi
                push    edi

                mov     ecx,expand8_width
                shr     ecx,1
                mov     eax,pY
                mov     esi,pU
                mov     edi,pV
                mov     edx,pDstLine
                lea     eax,[eax+ecx*2]
                lea     esi,[esi+ecx]
                lea     edi,[edi+ecx]
                neg     ecx
                
              loop_beign:
                PlanarYUV211_Loader_MMX(eax+ecx*2,esi+ecx,edi+ecx)
                YUV422ToRGB32_MMX(edx,movq)

                add     edx,8*4
                add     ecx,4
                jnz     loop_beign

                mov     pY,eax
                mov     pU,esi
                mov     pV,edi
                mov     pDstLine,edx

                pop     edi
                pop     esi
            }
        }

        //处理边界
        DECODE_PlanarYUV211_Common_line(pDstLine,pY,pU,pV,width-expand8_width);
    }

void DECODE_PlanarYUV411_MMX(const TUInt8* pY,const long Y_byte_width,
                             const TUInt8* pU,const long U_byte_width,
                             const TUInt8* pV,const long V_byte_width,
                             const TPicRegion& DstPic)
{
    assert((DstPic.width & 1)==0); 
    TARGB32* pDstLine=DstPic.pdata; 
    for (long y=0;y<DstPic.height;++y)
    {
        DECODE_PlanarYUV211_MMX_line(pDstLine,pY,pU,pV,DstPic.width);
        ((TUInt8*&)pDstLine)+=DstPic.byte_width;
        pY+=Y_byte_width;
        if ((y&1)==1)
        {
            pU+=U_byte_width;
            pV+=V_byte_width;
        }
    }    
    asm emms
}

    void DECODE_PlanarYUV211_SSE_line(TARGB32* pDstLine,const TUInt8* pY,
                                      const TUInt8* pU,const TUInt8* pV,long width)
    {
        long expand8_width=(width>>3)<<3;
        
        if (expand8_width>0)
        {
            asm
            {
                push    esi
                push    edi

                mov     ecx,expand8_width
                shr     ecx,1
                mov     eax,pY
                mov     esi,pU
                mov     edi,pV
                mov     edx,pDstLine
                lea     eax,[eax+ecx*2]
                lea     esi,[esi+ecx]
                lea     edi,[edi+ecx]
                neg     ecx
                
              loop_beign:
                PlanarYUV211_Loader_MMX(eax+ecx*2,esi+ecx,edi+ecx)
                YUV422ToRGB32_SSE(edx)

                add     edx,8*4
                add     ecx,4
                jnz     loop_beign

                mov     pY,eax
                mov     pU,esi
                mov     pV,edi
                mov     pDstLine,edx

                pop     edi
                pop     esi
            }
        }

        //处理边界
        DECODE_PlanarYUV211_Common_line(pDstLine,pY,pU,pV,width-expand8_width);
    }

void DECODE_PlanarYUV411_SSE(const TUInt8* pY,const long Y_byte_width,
                             const TUInt8* pU,const long U_byte_width,
                             const TUInt8* pV,const long V_byte_width,
                             const TPicRegion& DstPic)
{
    assert((DstPic.width & 1)==0); 
    TARGB32* pDstLine=DstPic.pdata; 
    for (long y=0;y<DstPic.height;++y)
    {
        DECODE_PlanarYUV211_SSE_line(pDstLine,pY,pU,pV,DstPic.width);
        ((TUInt8*&)pDstLine)+=DstPic.byte_width;
        pY+=Y_byte_width;
        if ((y&1)==1)
        {
            pU+=U_byte_width;
            pV+=V_byte_width;
        }
    }    
    asm emms
}


///////////////////////////////


typedef void (*T_DECODE_PlanarYUV_proc)(const TUInt8* pY,const long Y_byte_width,
                                        const TUInt8* pU,const long U_byte_width,
                                        const TUInt8* pV,const long V_byte_width,
                                        const TPicRegion& DstPic);

  void run_test(const T_DECODE_PlanarYUV_proc DECODE_PlanarYUV,const char* srcFileName,const long csRunCount)
  {
	Surface32 picSrc;
	Surface32 picDst;
    TUInt8* pPlanarYUV=0;

    LoadPic(picSrc,srcFileName);
    TPicRegion Src=picSrc.as_context();

    pPlanarYUV=new TUInt8[(Src.width*Src.height) *6/4];
    picDst.resize_and_clear(Size(Src.width,Src.height));
    TUInt8* pY=pPlanarYUV;  long Y_byte_width=Src.width;
    TUInt8* pU=&pPlanarYUV[Y_byte_width*Src.height];  long U_byte_width=Src.width/2;
    TUInt8* pV=&pPlanarYUV[Y_byte_width*Src.height*5/4];  long V_byte_width=Src.width/2;
    Encode_RGB32ToPlanarYUV411(Src,pY,Y_byte_width,pU,U_byte_width,pV,V_byte_width);
    TPicRegion Dst=picDst.as_context();
    

    clock_t t0=clock();

    for (long c=0;c<csRunCount;++c)
    {
        DECODE_PlanarYUV(pY,Y_byte_width,pU,U_byte_width,pV,V_byte_width,Dst);
    }
    t0=clock()-t0;

    save_pic("Pic\\DECODE_PlanarYUV_result.bmp",Dst);

    double fps=csRunCount/(t0*1.0/CLOCKS_PER_SEC);
	std::cout<<fps_space(fps)<<fps_ft(fps)<<" FPS";

    delete []pPlanarYUV;
  }

void test(const char* proc_name,T_DECODE_PlanarYUV_proc DECODE_PlanarYUV,const long csRunCount)
{
    std::cout<<proc_name<<" ( 1024x576) ";
    run_test(DECODE_PlanarYUV,"Pic\\1024x576.bmp",csRunCount*4);
    std::cout<<std::endl;
    std::cout<<proc_name<<" (1920x1080) ";
    run_test(DECODE_PlanarYUV,"Pic\\1920x1080.bmp",csRunCount);
    std::cout<<std::endl;
}


void main()
{
	char tmpchar;
	std::cin>>tmpchar;

    /*
    test("DECODE_YUYV_Float   ",DECODE_YUYV_Float   , 20);  //  55.0 FPS   15.6 FPS   
    test("DECODE_YUYV_Int     ",DECODE_YUYV_Int     , 40);  // 136.9 FPS   39.1 FPS   
    test("DECODE_YUYV_RGBTable",DECODE_YUYV_RGBTable, 70);  // 164.6 FPS   47.0 FPS
    test("DECODE_YUYV_Table   ",DECODE_YUYV_Table   , 60);  // 146.4 FPS   41.8 FPS   

    test("DECODE_YUYV_TableEx ",DECODE_YUYV_TableEx ,100);  // 236.7 FPS   67.7 FPS  
    test("DECODE_YUYV_Common  ",DECODE_YUYV_Common  ,100);  // 250.5 FPS   72.0 FPS   
    test("DECODE_YUYV_MMX     ",DECODE_YUYV_MMX     ,200);  // 585.4 FPS  169.8 FPS   
    test("DECODE_YUYV_SSE     ",DECODE_YUYV_SSE     ,200);  // 770.3 FPS  220.0 FPS   
        ////test("DECODE_YUYV_SSE_Ex  ",DECODE_YUYV_SSE_Ex  ,200);  //慢 
            //test("DECODE_YUYV_SSE2    ",DECODE_YUYV_SSE2    ,200);  //慢     

    test("DECODE_YUYV_Auto      ",DECODE_YUYV_Auto      ,200); 
    test("DECODE_YUYV_Parallel  ",DECODE_YUYV_Parallel  ,400); //1433.9 FPS  414.1 FPS
    test("DECODE_YUYV_ParallelEx",DECODE_YUYV_ParallelEx,400); //1387.5 FPS  409.9 FPS
    //*/

    test("DECODE_PlanarYUV411_Common  ",DECODE_PlanarYUV411_Common  ,100);  // 236.1 FPS   67.5 FPS   
    test("DECODE_PlanarYUV411_MMX     ",DECODE_PlanarYUV411_MMX     ,200);  // 650.1 FPS  187.3 FPS   
    test("DECODE_PlanarYUV411_SSE     ",DECODE_PlanarYUV411_SSE     ,200);  // 864.6 FPS  249.5 FPS   



	std::cin>>tmpchar;
}