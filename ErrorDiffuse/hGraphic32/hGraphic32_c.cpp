//hGraphic32_c.cpp

#include "hGraphic32_c.h"
#include "hGraphic32.h"

void pixels32Ref_fillColor(const TPixels32Ref_c* pPixels32Ref,const Color32_c color){
    ((const TPixels32Ref*)pPixels32Ref)->fillColor( *(const Color32*)(&color) );
}

void pixels32Ref_fillAlpha(const TPixels32Ref_c* pPixels32Ref,const unsigned int alpha){
    ((const TPixels32Ref*)pPixels32Ref)->fillAlpha( alpha );
}


TPixels32Handle pixels32_create(){
    try{
        return new TPixels32();
    }
    catch(...){
        return 0;
    }
}

bool pixels32_resizeFast(TPixels32Handle pixels32,long width,long height){
    if (pixels32==0) return false;
    try{
        ((TPixels32*)pixels32)->resizeFast(width,height);
        return true;
    }
    catch(...){
        return false;
    }
}

bool pixels32_loadBmp(TPixels32Handle pixels32,const char* bmpFileName){
    if (pixels32==0) return false;
    try{
        TFileInputStream bmpInputStream(bmpFileName);
        TBmpFile::load(&bmpInputStream,(TPixels32*)pixels32);
        return true;
    }
    catch(...){
        return false;
    }
}
bool pixels32_getRef(const TPixels32Handle pixels32,TPixels32Ref_c* pPixels32Ref){
    if (pixels32==0) return false;
    try{
        (*(TPixels32Ref*)pPixels32Ref)=((TPixels32*)pixels32)->getRef();
        return true;
    }
    catch(...){
        return false;
    }
}

void pixels32_delete(TPixels32Handle pixels32){
    if (pixels32!=0)
        delete (TPixels32*)pixels32;
}


bool pixels32Ref_saveBmp(const TPixels32Ref_c* pPixels32Ref,const char* bmpFileName){
    if ((pPixels32Ref==0)||(bmpFileName==0)) return false;
    try{
        TFileOutputStream bmpOutStream(bmpFileName);
        TBmpFile::save(*(TPixels32Ref*)pPixels32Ref,&bmpOutStream);//±£´æ½á¹ûÍ¼Æ¬
        return true;
    }
    catch(...){
        return false;
    }
}

bool pixels32_saveBmp(TPixels32Handle pixels32,const char* bmpFileName){
    if ((pixels32==0)||(bmpFileName==0)) return false;
    TPixels32Ref_c pixels32Ref;
    pixels32Ref.pdata=0;
    pixels32Ref.height=0;
    if (!pixels32_getRef(pixels32,&pixels32Ref))
        return false;
    return pixels32Ref_saveBmp(&pixels32Ref,bmpFileName);
}