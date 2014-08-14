//hGraphic32_c.h
#ifndef _hGraphic32_c_h_
#define _hGraphic32_c_h_

//base type
typedef char            Int8;
typedef unsigned char   UInt8;
typedef short           Int16;
typedef unsigned short  UInt16;
typedef long            Int32; 
typedef unsigned long   UInt32;

//32bitARGB颜色类型
struct Color32_c {
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
};

//图像数据区的描述信息
struct TPixels32Ref_c{
    Color32_c*  pdata;        //图像数据区首地址  即 y==0行的颜色首地址
    long        byte_width;   //一行图像数据的字节宽度  正负值都有可能 
    long        width;        //图像宽度
    long        height;       //图像高度
};

void pixels32Ref_fillColor(const TPixels32Ref_c* pPixels32Ref,const Color32_c color);
void pixels32Ref_fillAlpha(const TPixels32Ref_c* pPixels32Ref,const unsigned int alpha);
bool pixels32Ref_saveBmp(const TPixels32Ref_c* pPixels32Ref,const char* bmpFileName);
#define pixels32Ref_nextLine(pColorLine,byte_width)  ((UInt8*&)pColorLine)+=byte_width

typedef void* TPixels32Handle;

TPixels32Handle pixels32_create();
void pixels32_delete(TPixels32Handle pixels32);
bool pixels32_resizeFast(TPixels32Handle pixels32,long width,long height);
bool pixels32_loadBmp(TPixels32Handle pixels32,const char* bmpFileName);
bool pixels32_getRef(const TPixels32Handle pixels32,TPixels32Ref_c* pPixels32Ref);
bool pixels32_saveBmp(TPixels32Handle pixels32,const char* bmpFileName);

#endif //_hGraphic32_c_h_
