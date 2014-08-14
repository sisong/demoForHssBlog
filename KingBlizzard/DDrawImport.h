//DDrawImport.h
#ifndef __DDraw_Import_H_
#define __DDraw_Import_H_

#include "HColor.h"

class CDDrawImprot
{
private:
    void* m_Import;
    void  ClearImportPointer();
public:
    CDDrawImprot();
    ~CDDrawImprot();
    void IsShowFPS(bool isShowFPS);

    bool IntiScreen(long ScreenWidth,long ScreenHeight,long ScreenColorBit,void * main_window_handle); 
    bool DrawBegin(Context32& out_dst);
    void DrawEnd(Context32& dst);
};



#endif //__DDraw_Import_H_