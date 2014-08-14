//hPixels32.cpp

#include "hPixels32.h"

void TPixels32::getNewMemory(const long width,const long height,TPixels32Ref& out_ref){
    TPixels32Ref result;
    if ((width>0)&&(height>0)){
        long byte_width=width*sizeof(Color32);
        result.pdata=(Color32*)(new UInt8[byte_width*height]);
        result.byte_width=byte_width;
        result.width=width;
        result.height=height;
    }
    out_ref=result;
}
void TPixels32::delMemory(TPixels32Ref& ref){
    UInt8* pMemData=((UInt8*)ref.pdata);
    ref.pdata=0;
    ref.byte_width=0;
    ref.width=0;
    ref.height=0;
    if (pMemData!=0)
        delete []pMemData; 
}
