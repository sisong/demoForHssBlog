//hColor32.h
#ifndef _hColor32_h_
#define _hColor32_h_

//base type
typedef char            Int8;
typedef unsigned char   UInt8;
typedef short           Int16;
typedef unsigned short  UInt16;
typedef long            Int32; 
typedef unsigned long   UInt32;

//32bitARGB颜色类型
struct Color32 {
    union {
        UInt32   argb;
        struct
        {
            UInt8  b;
            UInt8  g;
            UInt8  r;
            UInt8  a;
        };
    };
    inline static UInt32 packColor32Data(unsigned long r8,unsigned long g8,unsigned long b8,unsigned long a8=255) { return b8|(g8<<8)|(r8<<16)|(a8<<24);  }
    inline Color32(){}
    inline Color32(const Color32& color32):argb(color32.argb){}
    inline explicit Color32(const UInt32 color32):argb(color32){}
    inline explicit Color32(unsigned long r8,unsigned long g8,unsigned long b8,unsigned long a8=255):argb(packColor32Data(r8,g8,b8,a8)){}
    inline bool operator ==(const Color32& color32) const { return argb==color32.argb; }
    inline bool operator !=(const Color32& color32) const{ return !((*this)==color32); }
};


//图像数据区的描述信息
struct TPixels32Ref{
    Color32*    pdata;        //图像数据区首地址  即 y==0行的颜色首地址
    long        byte_width;   //一行图像数据的字节宽度  正负值都有可能 
    long        width;        //图像宽度
    long        height;       //图像高度
    inline TPixels32Ref() :pdata(0),byte_width(0),width(0),height(0){}
    inline TPixels32Ref(const TPixels32Ref& ref) :pdata(ref.pdata),byte_width(ref.byte_width),width(ref.width),height(ref.height){}
    
    //访问(x,y)坐标处的颜色
    inline Color32& pixels(const long x,const long y) const { return getLinePixels(y)[x]; }
    //得到y行的颜色首地址
    inline Color32* getLinePixels(const long y) const { return (Color32*) ( ((UInt8*)pdata) + byte_width*y ); }

    //是否是空图像区
    inline bool getIsEmpty()const { return ((width<=0)||(height<=0)); }
    //将pline指向下一行颜色
    inline void nextLine(Color32*& pline)const {  ((UInt8*&)pline)+=byte_width;   }

    //坐标边界饱和  如果(x,y)坐标在图片数据区外,(x,y)值会被设置到图片最近的边界内,并返回false(否则什么也不做,返回true) //警告! 图片区域不能为空
    inline bool clipToBorder(long& x, long& y)const{
        bool isIn = true;
        if (x < 0) { 
            isIn = false; x = 0;
        } else if (x >= width) { 
            isIn = false; x = width - 1;
        }
        if (y < 0) {
            isIn = false; y = 0;
        } else if (y >= height) {
            isIn = false; y = height - 1;
        }
        return isIn;
    }
    //获取一个点的颜色,默认执行边界饱和测试  当坐标超出区域的时候返回的颜色为最近的边界上的颜色值并且其alpha通道置零  //警告! 图片区域不能为空 速度很慢 
    inline Color32 getPixelsBorder(long x, long y) const {
        bool isInPic = clipToBorder(x,y);
        Color32 result = pixels(x,y);
        if (!isInPic)
            result.a=0;
        return result;
    }
    inline bool getIsInPic(long x, long y)const{
        //return (x>=0)&&(y>=0)&&(x<width)&&(y<height);
        //利用负数的编码优化为:
        return ((unsigned long)x<(unsigned long)width)&&((unsigned long)y<(unsigned long)height);
    }

    //填充颜色
    void fillColor(const Color32 color)const{
        Color32* pDstLine=(Color32*)pdata;
        for (long y=0;y<height;++y){
            for (long x=0;x<width;++x){
                pDstLine[x]=color;
            }
            nextLine(pDstLine);
        }
    }
    void fillAlpha(const unsigned int alpha)const {
        Color32* pDstLine=(Color32*)pdata;
        for (long y=0;y<height;++y){
            for (long x=0;x<width;++x){
                pDstLine[x].a=alpha;
            }
            nextLine(pDstLine);
        }
    }
    void copyColor(const TPixels32Ref& src)const{
        Color32* pDstLine=(Color32*)pdata;
        Color32* pSrcLine=(Color32*)src.pdata;
		long mHeight=height;
		if (mHeight>src.height) mHeight=src.height;
		long mWidth=width;
		if (mWidth>src.width) mWidth=src.width;
        for (long y=0;y<mHeight;++y){
            for (long x=0;x<mWidth;++x){
                pDstLine[x]=pSrcLine[x];
            }
            nextLine(pDstLine);
            src.nextLine(pSrcLine);
        }
    }
};

//图像数据区接口
class IPixels32Buf{
public:
    virtual void resizeFast(long width,long height)=0;    //设置图像数据区大小
    virtual bool lockBuf(TPixels32Ref& out_color32Ref)=0;  //锁住表面 返回其描述信息
    virtual void unlockBuf()=0; //解锁表面
    virtual ~IPixels32Buf(){}
};

#endif
