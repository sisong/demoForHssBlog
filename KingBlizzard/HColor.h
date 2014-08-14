//HColor.h
#ifndef __HColor_H_
#define __HColor_H_

#define must_inline __forceinline

//颜色查表
extern const unsigned char* color_table;

typedef unsigned long Color32Data;

struct Color32
{
	union
	{
        Color32Data   xrgb;
		struct
		{
			unsigned char  b;
			unsigned char  g;
			unsigned char  r;
			unsigned char  x;
		};
	};
    must_inline Color32(){}
    must_inline Color32(Color32Data color32):xrgb(color32){}
    must_inline Color32(const Color32& color32):xrgb(color32.xrgb){}
    must_inline Color32(unsigned long r8,unsigned long g8,unsigned long b8):xrgb(PackColor32Data(r8,g8,b8)){}
    must_inline static Color32Data PackColor32Data(unsigned long r8,unsigned long g8,unsigned long b8) { return b8|(g8<<8)|(r8<<16);  }
};

struct Context32
{
private:
    static inline long _min(long x,long y)  { if (x<y) return x; else return y; }
public:
    Color32*    pdata;        //颜色数据首地址
    long        byte_width;   //一行数据宽度(字节宽度)
	long        width;        //像素宽度
	long        height;       //像素高度

    must_inline Color32& Pixels(const long x,const long y)
    {
        return ( (Color32*)((unsigned char*)pdata+byte_width*y) )[x];
    }
    inline void fill(const Color32 Color)
    {
        Color32* pdst=this->pdata;
        for (long y=0;y<this->height;++y)
        {
            for (long x=0;x<this->width;++x)
                pdst[x]=Color;
            (unsigned char*&)pdst+=this->byte_width;
        }
    }
    inline void copy(const Context32& src)
    {
        Color32* pdst=this->pdata;
        Color32* psrc=src.pdata;
        for (long y=0;y<_min(this->height,src.height);++y)
        {
            for (long x=0;x<_min(this->width,src.width);++x)
                pdst[x]=psrc[x];
            (unsigned char*&)pdst+=this->byte_width;
            (unsigned char*&)psrc+=src.byte_width;
        }
    }
    inline void darkle(const long del_color)
    {
        Color32* pdst=this->pdata;
        for (long y=0;y<this->height;++y)
        {
            for (long x=0;x<this->width;++x)
            {
                Color32 Color=pdst[x];
                if (Color.xrgb!=0)
                {
                    Color.r=color_table[Color.r-del_color];
                    Color.g=color_table[Color.g-del_color];
                    Color.b=color_table[Color.b-del_color];
                    pdst[x]=Color;
                }
            }
            (unsigned char*&)pdst+=this->byte_width;
        }
    }
};

#endif //__HColor_H_