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

//高彩色颜色和图片数据定义 (
struct  TRGB16_555   // 16bit 5:5:5 high color
{
   UInt16 b: 5 ;
   UInt16 g: 5 ;
   UInt16 r: 5 ;
   UInt16 x: 1 ;
};

struct  TPicRegion_RGB16_555   // 一块颜色数据区的描述，便于参数传递
{
    TRGB16_555 *      pdata;         // 颜色数据首地址
     long             byte_width;    // 一行数据的物理宽度(字节宽度)
    unsigned  long    width;         // 像素宽度
    unsigned  long    height;        // 像素高度
};
inline TRGB16_555 &  Pixels( const  TPicRegion_RGB16_555 &  pic, const   long  x, const   long  y)
{
     return  ( (TRGB16_555 * )((UInt8 * )pic.pdata + pic.byte_width * y) )[x];
}

class Surface_RGB16_555{
    TPicRegion_RGB16_555 m_data;
public:
    Surface_RGB16_555(long  width,long height){
        m_data.byte_width=sizeof(TRGB16_555)*width;
        m_data.pdata=(TRGB16_555*)(new UInt8[m_data.byte_width*height]);
        m_data.width=width;
        m_data.height=height;
    }
    ~Surface_RGB16_555(){ delete[] (UInt8*)m_data.pdata; }
    const TPicRegion_RGB16_555& getRef()const{ return m_data; }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




inline TRGB16_555 ToColor16( const  Color32 &  color){
    TRGB16_555 result;
    result.r = color.r >> 3 ;
    result.g = color.g >> 3 ;
    result.b = color.b >> 3 ;
     return  result;
}

void  CvsPic32To16_0( const  TPicRegion_RGB16_555 &  dst, const  TPixels32Ref &  src){
     for  ( long  y = 0 ;y < src.height; ++ y){
         for  ( long  x = 0 ;x < src.width; ++ x){
            Pixels(dst,x,y) = ToColor16(src.pixels(x,y));
        }
    }
}

/////////////////////////////////////////////////////////////////////

inline UInt16 ToColor16_1( const  Color32 &  color){
     return  ((color.r >> 3 ) << 10 ) | ((color.g >> 3 ) << 5 ) | (color.b >> 3 );
}

void  CvsPic32To16_1( const  TPicRegion_RGB16_555 &  dst, const  TPixels32Ref &  src){
    UInt16 *  pDst = (UInt16 * )dst.pdata;
     const  Color32 *  pSrc = src.pdata;
     const   long  width = src.width;
     for  ( long  y = 0 ;y < src.height; ++ y){
         for  ( long  x = 0 ;x < width; ++ x){
            pDst[x] = ToColor16_1(pSrc[x]);
        }
        (UInt8 *& )pDst += dst.byte_width;
        (UInt8 *& )pSrc += src.byte_width;
    }
}


/////////////////////////////////////////////////////////////////////
//误差传递系数为：
//  * 2
//1 1 0    /4

     struct  TErrorColor_f{
         float  dR;
         float  dG;
         float  dB;
    };

    inline  long  getBestRGB16_555Color_f( const   float  wantColor){
         float  result = wantColor * ( 31.0f / 255 );
         if  (result <= 0 ) 
             return   0 ;
         else   if  (result >= 31 )
             return   31 ;
         else
             return  ( long )result;
    }

     void  CvsPic32To16_ErrorDiffuse_Line_0(UInt16 *  pDst, const  Color32 *  pSrc, long  width,TErrorColor_f *  PHLineErr){
        TErrorColor_f HErr;
        HErr.dR = 0 ; HErr.dG = 0 ; HErr.dB = 0 ;
        PHLineErr[ - 1 ].dB = 0 ; PHLineErr[ - 1 ].dG = 0 ; PHLineErr[ - 1 ].dR = 0 ; 
         for  ( long  x = 0 ;x < width; ++ x)
        {
             // cB,cG,cR为应该显示的颜色
             float  cB = (pSrc[x].b + HErr.dB * 2 + PHLineErr[x].dB );
             float  cG = (pSrc[x].g + HErr.dG * 2 + PHLineErr[x].dG );
             float  cR = (pSrc[x].r + HErr.dR * 2 + PHLineErr[x].dR );
             // rB,rG,rR为转换后的颜色(也就是实际显示颜色)
             long  rB = getBestRGB16_555Color_f(cB);
             long  rG = getBestRGB16_555Color_f(cG);
             long  rR = getBestRGB16_555Color_f(cR);
            pDst[x] =  rB | (rG << 5 ) | (rR << 10 );
             // 计算两个颜色之间的差异的1/4
            HErr.dB = (cB - (rB * ( 255.0f / 31 ))) * ( 1.0f / 4 );
            HErr.dG = (cG - (rG * ( 255.0f / 31 ))) * ( 1.0f / 4 );
            HErr.dR = (cR - (rR * ( 255.0f / 31 ))) * ( 1.0f / 4 );

            PHLineErr[x - 1 ].dB += HErr.dB;
            PHLineErr[x - 1 ].dG += HErr.dG;
            PHLineErr[x - 1 ].dR += HErr.dR;

            PHLineErr[x] = HErr;
        }
    }

void  CvsPic32To16_ErrorDiffuse_0( const  TPicRegion_RGB16_555 &  dst, const  TPixels32Ref &  src){
    UInt16 *  pDst = (UInt16 * )dst.pdata;
     const  Color32 *  pSrc = src.pdata;
     const   long  width = src.width;

    TErrorColor_f *  _HLineErr = new  TErrorColor_f[width + 2 ]; 
     for  ( long  x = 0 ;x < width + 2 ; ++ x){
        _HLineErr[x].dR = 0 ;
        _HLineErr[x].dG = 0 ;
        _HLineErr[x].dB = 0 ;
    }
    TErrorColor_f *  HLineErr =& _HLineErr[ 1 ];

     for  ( long  y = 0 ;y < src.height; ++ y){
        CvsPic32To16_ErrorDiffuse_Line_0(pDst,pSrc,width,HLineErr);
        (UInt8 *& )pDst += dst.byte_width;
        (UInt8 *& )pSrc += src.byte_width;
    }

    delete[]_HLineErr;
}

/////////////////////////////////////////////////////////////////////
	
	struct  TErrorColor{
         long  dR;
         long  dG;
         long  dB;
    };

    inline  long  getBestRGB16_555Color( const   long  wantColor){
         const   long  rMax = ( 1 << 5 ) - 1 ;
         if  (wantColor <= 0 ) 
             return   0 ;
         else   if  (wantColor >= (rMax << 3 ))
             return  rMax;
         else
             return  wantColor >> 3 ;
    }
    inline  long  getC8Color( const   long  rColor){
         return  rColor*(255*(1<<16)/((1<<5)-1)) >>16; // rColor*255/((1<<5)-1);
    }
     void  CvsPic32To16_ErrorDiffuse_Line_1(UInt16 *  pDst, const  Color32 *  pSrc, long  width,TErrorColor *  PHLineErr){
        TErrorColor HErr;
        HErr.dR = 0 ; HErr.dG = 0 ; HErr.dB = 0 ;
        PHLineErr[ - 1 ].dB = 0 ; PHLineErr[ - 1 ].dG = 0 ; PHLineErr[ - 1 ].dR = 0 ; 
         for  ( long  x = 0 ;x < width; ++ x)
        {
             long  cB = (pSrc[x].b + HErr.dB*2  + PHLineErr[x].dB );
             long  cG = (pSrc[x].g + HErr.dG*2  + PHLineErr[x].dG );
             long  cR = (pSrc[x].r + HErr.dR*2  + PHLineErr[x].dR );
             long  rB = getBestRGB16_555Color(cB);
             long  rG = getBestRGB16_555Color(cG);
             long  rR = getBestRGB16_555Color(cR);
            pDst[x] =  rB | (rG << 5 ) | (rR << 10 );

            HErr.dB = (cB - getC8Color(rB)) >> 2 ;
            HErr.dG = (cG - getC8Color(rG)) >> 2 ;
            HErr.dR = (cR - getC8Color(rR)) >> 2 ;

            PHLineErr[x - 1 ].dB += HErr.dB;
            PHLineErr[x - 1 ].dG += HErr.dG;
            PHLineErr[x - 1 ].dR += HErr.dR;

            PHLineErr[x] = HErr;
        }
    }

void  CvsPic32To16_ErrorDiffuse_1( const  TPicRegion_RGB16_555 &  dst, const  TPixels32Ref &  src){
    UInt16 *  pDst = (UInt16 * )dst.pdata;
     const  Color32 *  pSrc = src.pdata;
     const   long  width = src.width;

    TErrorColor *  _HLineErr = new  TErrorColor[width + 2 ]; 
     for  ( long  x = 0 ;x < width + 2 ; ++ x){
        _HLineErr[x].dR = 0 ;
        _HLineErr[x].dG = 0 ;
        _HLineErr[x].dB = 0 ;
    }
    TErrorColor *  HLineErr =& _HLineErr[ 1 ];

     for  ( long  y = 0 ;y < src.height; ++ y){
        CvsPic32To16_ErrorDiffuse_Line_1(pDst,pSrc,width,HLineErr);
        (UInt8 *& )pDst += dst.byte_width;
        (UInt8 *& )pSrc += src.byte_width;
    }

    delete[]_HLineErr;
} 

/////////////////////////////////////////////////////////////////////

     static  UInt8 _BestRGB16_555Color_Table[ 256 * 5 ];
     const  UInt8 *  BestRGB16_555Color_Table =& _BestRGB16_555Color_Table[ 256 * 2 ];
     struct  _TAutoInit_BestRGB16_555Color_Table{
        _TAutoInit_BestRGB16_555Color_Table(){
             for  ( long  i = 0 ;i < 256 * 5 ; ++ i){
                _BestRGB16_555Color_Table[i] = getBestRGB16_555Color(i - 256 * 2 );
            }
        }
    };
     static  _TAutoInit_BestRGB16_555Color_Table _AutoInit_BestRGB16_555Color_Table;
	 //实际代码中建议预先生成_BestRGB16_555Color_Table的数据,从而避免初始化顺序依赖的问题

     void  CvsPic32To16_ErrorDiffuse_Line_2(UInt16 *  pDst, const  Color32 *  pSrc, long  width,TErrorColor *  PHLineErr){
        TErrorColor HErr;
        HErr.dR = 0 ; HErr.dG = 0 ; HErr.dB = 0 ;
        PHLineErr[ - 1 ].dB = 0 ; PHLineErr[ - 1 ].dG = 0 ; PHLineErr[ - 1 ].dR = 0 ; 
         for  ( long  x = 0 ;x < width; ++ x)
        {
             long  cB = (pSrc[x].b + HErr.dB*2  + PHLineErr[x].dB );
             long  cG = (pSrc[x].g + HErr.dG*2  + PHLineErr[x].dG );
             long  cR = (pSrc[x].r + HErr.dR*2  + PHLineErr[x].dR );
             long  rB = BestRGB16_555Color_Table[cB];
             long  rG = BestRGB16_555Color_Table[cG];
             long  rR = BestRGB16_555Color_Table[cR];
            pDst[x] =  rB | (rG << 5 ) | (rR << 10 );

			//做乘法比较慢的cpu体系下可以尝试把getC8Color也做成一个数组表
            HErr.dB = (cB - getC8Color(rB)) >> 2 ;
            HErr.dG = (cG - getC8Color(rG)) >> 2 ;
            HErr.dR = (cR - getC8Color(rR)) >> 2 ;

            PHLineErr[x - 1 ].dB += HErr.dB;
            PHLineErr[x - 1 ].dG += HErr.dG;
            PHLineErr[x - 1 ].dR += HErr.dR;

            PHLineErr[x] = HErr;
        }
    }
 
void  CvsPic32To16_ErrorDiffuse_2( const  TPicRegion_RGB16_555 &  dst, const  TPixels32Ref &  src){
    UInt16 *  pDst = (UInt16 * )dst.pdata;
     const  Color32 *  pSrc = src.pdata;
     const   long  width = src.width;

    TErrorColor *  _HLineErr = new  TErrorColor[width + 2 ]; 
     for  ( long  x = 0 ;x < width + 2 ; ++ x){
        _HLineErr[x].dR = 0 ;
        _HLineErr[x].dG = 0 ;
        _HLineErr[x].dB = 0 ;
    }
    TErrorColor *  HLineErr =& _HLineErr[ 1 ];

     for  ( long  y = 0 ;y < src.height; ++ y){
        CvsPic32To16_ErrorDiffuse_Line_2(pDst,pSrc,width,HLineErr);
        (UInt8 *& )pDst += dst.byte_width;
        (UInt8 *& )pSrc += src.byte_width;
    }

    delete[]_HLineErr;
}


/////////////////////////////////////////////////////////////////////

//扩散模板
//  * 1   /1


     void  CvsPic32To16_ErrorDiffuse_Line_fast(UInt16 *  pDst, const  Color32 *  pSrc, long  width){
        TErrorColor HErr;
        HErr.dR = 0 ; HErr.dG = 0 ; HErr.dB = 0 ;
         for  ( long  x = 0 ;x < width; ++ x)
        {
             long  cB = (pSrc[x].b + HErr.dB);
             long  cG = (pSrc[x].g + HErr.dG);
             long  cR = (pSrc[x].r + HErr.dR);
             long  rB = BestRGB16_555Color_Table[cB];
             long  rG = BestRGB16_555Color_Table[cG];
             long  rR = BestRGB16_555Color_Table[cR];
            pDst[x] =  rB | (rG << 5 ) | (rR << 10 );

            HErr.dB = (cB - getC8Color(rB)) ;
            HErr.dG = (cG - getC8Color(rG)) ;
            HErr.dR = (cR - getC8Color(rR)) ;
        }
    }
 
void  CvsPic32To16_ErrorDiffuse_fast( const  TPicRegion_RGB16_555 &  dst, const  TPixels32Ref &  src){
    UInt16 *  pDst = (UInt16 * )dst.pdata;
     const  Color32 *  pSrc = src.pdata;
     const   long  width = src.width;
     for  ( long  y = 0 ;y < src.height; ++ y){
        CvsPic32To16_ErrorDiffuse_Line_fast(pDst,pSrc,width);
        (UInt8 *& )pDst += dst.byte_width;
        (UInt8 *& )pSrc += src.byte_width;
    }
}

/////////////////////////////////////////////////////////////////////
#ifdef MMX_ACTIVE


    const UInt64 csMMX_erdf_mul_w  = 0x41CE41CE41CE41CE; //0x41CE=16846=(255*(1<<11)/((1<<5)-1));
    const UInt64 csMMX_erdf_mask_w = 0x0000000000F8F8F8; //0xF8=((1<<5)-1)<<3;

    inline void CvsPic32To16_ErrorDiffuse_Line_MMX(UInt16* pDst,const Color32* pSrc,long width){
        asm{
            //push esi
            //push edi
            //push ebx
            mov  ecx,width
            mov  edi,pDst
            mov  esi,pSrc 
            lea  edi,[edi+ecx*2] //2==sizeof(UInt16)
            lea  esi,[esi+ecx*4] //4==sizeof(Color32)
            neg  ecx
            pxor      mm6,mm6      //HErr=0000000...
            pxor      mm7,mm7      //mm7 =0000000...
            movq      mm3,csMMX_erdf_mul_w
            movq      mm4,csMMX_erdf_mask_w

        loop_begin:
                movd        mm0,[esi+ecx*4]
                punpcklbw   mm0,mm7
                paddw       mm0,mm6  //ok: cB,cG,cR
                movq        mm6,mm0

                packuswb    mm0,mm7
                pand        mm0,mm4
                psrlq       mm0,3   //>>3
                movd    eax,mm0  //00000000 000RRRRR 000GGGGG 000BBBBB
                punpcklbw   mm0,mm7
                movzx   ebx,ah
                movzx   edx,al
                psllw       mm0,5
                shr     eax,16
                shl     ebx,5
                pmulhw      mm0,mm3
                shl     eax,10
                or      edx,ebx
                psubw       mm6,mm0
                or      eax,edx
                mov word ptr[edi+ecx*2],ax //pDst[x]= rB|(rG<<5)|(rR<<10);
                
                inc   ecx
                jnz   loop_begin

            emms
            //pop  ebx
            //pop  edi
            //pop  esi
        }
    }

void  CvsPic32To16_ErrorDiffuse_MMX( const  TPicRegion_RGB16_555 &  dst, const  TPixels32Ref &  src){
    UInt16 *  pDst = (UInt16 * )dst.pdata;
     const  Color32 *  pSrc = src.pdata;
     const   long  width = src.width;
     if (width<=0) return;
     for  ( long  y = 0 ;y < src.height; ++ y){
        CvsPic32To16_ErrorDiffuse_Line_MMX(pDst,pSrc,width);
        (UInt8 *& )pDst += dst.byte_width;
        (UInt8 *& )pSrc += src.byte_width;
    }
}

#endif //MMX_ACTIVE

/////////////////////////////////////////////////////////////////////

//Floyd-Steinberg
//  * 7
//3 5 1   /16

	void  CvsPic32To16_ErrorDiffuse_Line_fs(UInt16 *  pDst, const  Color32 *  pSrc, long  width,TErrorColor *  PHLineErr0,TErrorColor *  PHLineErr1){
        TErrorColor HErr;
        HErr.dR = 0 ; HErr.dG = 0 ; HErr.dB = 0 ;
        PHLineErr1[ - 1 ].dB = 0 ; PHLineErr1[ - 1 ].dG = 0 ; PHLineErr1[ - 1 ].dR = 0 ; 
        PHLineErr1[ 0 ].dB = 0 ; PHLineErr1[ 0 ].dG = 0 ; PHLineErr1[ 0 ].dR = 0 ; 
        for  ( long  x = 0 ;x < width; ++ x)
        {
            long  cB = (pSrc[x].b + ((HErr.dB  + PHLineErr0[x].dB)>>4) );
            long  cG = (pSrc[x].g + ((HErr.dG  + PHLineErr0[x].dG)>>4) );
            long  cR = (pSrc[x].r + ((HErr.dR  + PHLineErr0[x].dR)>>4) );
            long  rB = BestRGB16_555Color_Table[cB];
            long  rG = BestRGB16_555Color_Table[cG];
            long  rR = BestRGB16_555Color_Table[cR];
            pDst[x] =  rB | (rG << 5 ) | (rR << 10 );

            PHLineErr1[x + 1].dB = (cB - getC8Color(rB));
            PHLineErr1[x + 1].dG = (cG - getC8Color(rG));
            PHLineErr1[x + 1].dR = (cR - getC8Color(rR));

            PHLineErr1[x - 1].dB += (PHLineErr1[x + 1].dB*3);
            PHLineErr1[x - 1].dG += (PHLineErr1[x + 1].dG*3);
            PHLineErr1[x - 1].dR += (PHLineErr1[x + 1].dR*3);
            PHLineErr1[x    ].dB += (PHLineErr1[x + 1].dB*5);
            PHLineErr1[x    ].dG += (PHLineErr1[x + 1].dG*5);
            PHLineErr1[x    ].dR += (PHLineErr1[x + 1].dR*5);
            HErr.dB = (PHLineErr1[x + 1].dB*7);
            HErr.dG = (PHLineErr1[x + 1].dG*7);
            HErr.dR = (PHLineErr1[x + 1].dR*7);
        }
    }
 
void  CvsPic32To16_ErrorDiffuse_fs( const  TPicRegion_RGB16_555 &  dst, const  TPixels32Ref &  src){
    UInt16 *  pDst = (UInt16 * )dst.pdata;
     const  Color32 *  pSrc = src.pdata;
     const   long  width = src.width;

    TErrorColor *  _HLineErr = new  TErrorColor[(width + 2)*2 ]; 
     for  ( long  x = 0 ;x < (width + 2)*2 ; ++ x){
        _HLineErr[x].dR = 0 ;
        _HLineErr[x].dG = 0 ;
        _HLineErr[x].dB = 0 ;
    }
    TErrorColor *  HLineErr0 =& _HLineErr[ 1 ];
    TErrorColor *  HLineErr1 =& _HLineErr[ 1 + (width + 2) ];

     for  ( long  y = 0 ;y < src.height; ++ y){
        CvsPic32To16_ErrorDiffuse_Line_fs(pDst,pSrc,width,HLineErr0,HLineErr1);
		std::swap(HLineErr0,HLineErr1);
        (UInt8 *& )pDst += dst.byte_width;
        (UInt8 *& )pSrc += src.byte_width;
    }

    delete[]_HLineErr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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


inline Color32 ToColor32( const  TRGB16_555 &  color){
    Color32 result;
    result.r = color.r << 3 ;
    result.g = color.g << 3 ;
    result.b = color.b << 3 ;
     return  result;
}

void CvsPic16To32(const TPixels32Ref& dst,const TPicRegion_RGB16_555& src){
    for (long y=0;y<(long)src.height;++y){
        for (long x=0;x<(long)src.width;++x){
			dst.pixels(x,y)=ToColor32(Pixels(src,x,y));
        }
    }
}

typedef void (*T_CvsPic32To16_proc)(const TPicRegion_RGB16_555& dst,const TPixels32Ref& src);

void test(const char* proc_name,const T_CvsPic32To16_proc CvsPic32To16_proc,const long csRunCount){
    TPixels32 srcPic;
#if defined(__APPLE__) && defined(__MACH__)
    TFileInputStream bmpInputStream("/Users/hss/Desktop/GraphicDemo/ColorToGray/test0.bmp"); //我的xcode测试目录
#else
    TFileInputStream bmpInputStream("test0.bmp");
#endif
    TBmpFile::load(&bmpInputStream,&srcPic);//加载源图片
    Surface_RGB16_555 dstPic(srcPic.getWidth(),srcPic.getHeight());

    std::cout<<proc_name<<": ";
    clock_t t0=clock();
    for (long c=0;c<csRunCount;++c){
        CvsPic32To16_proc(dstPic.getRef(),srcPic.getRef());
    }
    t0=clock()-t0;
    double fps=csRunCount/(t0*1.0/CLOCKS_PER_SEC);
    std::cout<<fpsToStr(fps)<<" 帧/秒"<<std::endl;

    if (true){ 
#if defined(__APPLE__) && defined(__MACH__)
        TFileOutputStream bmpOutStream("/Users/hss/Desktop/GraphicDemo/ColorToGray/test0Result.bmp");
#else
        TFileOutputStream bmpOutStream("test0Result.bmp");
#endif
		TPixels32 tmpPic(srcPic.getWidth(),srcPic.getHeight());
        CvsPic16To32(tmpPic.getRef(),dstPic.getRef());
        TBmpFile::saveAsColor16_555(tmpPic.getRef(),&bmpOutStream);//保存结果图片
    }
}



//////////////////////////////////////////////////////////////////////////////////////////////////////

int main()
{
    std::cout<<" 请输入回车键开始测试(可以把进程优先级设置为“实时”)> ";
    waitInputChar();
    std::cout<<std::endl;

    //*																				// i7-920 3.44G
    test("CvsPic32To16_0				",CvsPic32To16_0				,700);		// 386.95 fps 
    test("CvsPic32To16_1				",CvsPic32To16_1				,2000);		//1221.00 fps 
    test("CvsPic32To16_ErrorDiffuse_0	",CvsPic32To16_ErrorDiffuse_0	,200);		//  97.13 fps 
    test("CvsPic32To16_ErrorDiffuse_1	",CvsPic32To16_ErrorDiffuse_1	,500);		// 283.77 fps 
    test("CvsPic32To16_ErrorDiffuse_2	",CvsPic32To16_ErrorDiffuse_2	,600);		// 316.62 fps 
    test("CvsPic32To16_ErrorDiffuse_fast",CvsPic32To16_ErrorDiffuse_fast,800);		// 422.83 fps 
	#ifdef MMX_ACTIVE
    test("CvsPic32To16_ErrorDiffuse_MMX	",CvsPic32To16_ErrorDiffuse_MMX	,1200);		// 662.98 fps 
	#endif
    test("CvsPic32To16_ErrorDiffuse_fs",CvsPic32To16_ErrorDiffuse_fs	,400);		// 198.81 fps 
    //*/

    std::cout<<std::endl<<" 测试完成. ";
    waitInputChar();
    return 0;
}
