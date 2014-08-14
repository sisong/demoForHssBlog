
#include "DDrawImport.h"
#include "hDibImport.h"

#include <ddraw.h>
#include "ColorMover.h"
#include "MMTime.h"

class CDDraw
{
private:
    // DirectDraw stuff
    LPDIRECTDRAW4         lpdd4        ;   // DD3 对象
    LPDIRECTDRAWSURFACE4  lpddsprimary ;   // dd 主表面
    LPDIRECTDRAWSURFACE4  lpddsback    ;   // dd 缓冲表面
    DDSURFACEDESC2        ddsd;            // 一个 Direct Draw 表面 描述 struct
    CDibImport            m_DibSurface;      //DIB绘制缓冲

    //帧数计算
    bool                  m_IsShowFPS;
    long                  m_OldDrawTime;
    double                m_FPS;
    CColorMover           m_FPSColor;
public:
    CDDraw():lpdd4(0),lpddsprimary(0),lpddsback(0),m_IsShowFPS(false) { }
    ~CDDraw()  { Clear(); }
    void IsShowFPS(bool isShowFPS) { m_IsShowFPS=isShowFPS; }

    void  Clear()
    {
        if (lpddsback!=0) // 释放缓冲表面
        {
            lpddsback->Release();
            lpddsback = 0;
        }
        if (lpddsprimary!=0)//释放主表面
        {
            lpddsprimary->Release();
            lpddsprimary = 0;
        }
        if (lpdd4!=0)// 释放 IDirectDraw4 界面
        {
            lpdd4->Release();
            lpdd4 = 0;
        }
        m_DibSurface.Clear();
    }

    bool setScreenMode(long ScreenWidth,long ScreenHeight,long ScreenColorBit)
    {
        HRESULT result=lpdd4->SetDisplayMode(ScreenWidth,ScreenHeight, ScreenColorBit,0,0);
        return !(FAILED(result));
    }
    bool trySetScreenMode(long ScreenWidth,long ScreenHeight,long ScreenColorBit)
    {
        //尝试32,24,16三种颜色模式
        if ( (ScreenColorBit!=32)&&(setScreenMode(ScreenWidth,ScreenHeight, 32)))  return true;
        if ( (ScreenColorBit!=24)&&(setScreenMode(ScreenWidth,ScreenHeight, 24)))  return true;
        if ( (ScreenColorBit!=16)&&(setScreenMode(ScreenWidth,ScreenHeight, 16)))  return true;
        return false;
    }

    bool IntiScreen(long ScreenWidth,long ScreenHeight,long ScreenColorBit,void * main_window_handle)
    {
        Clear();

        // 该函数在初次创建完窗体和进入事件循环时调用
        // 进行DDraw的初始化

        LPDIRECTDRAW lpdd_temp;

        // 创建基本 IDirectDraw 界面
        if (FAILED(DirectDrawCreate(0, &lpdd_temp, 0)))
            return false;

        // 查询 IDirectDraw4 界面
        if (FAILED(lpdd_temp->QueryInterface(IID_IDirectDraw4,(LPVOID *)&lpdd4)))
            return false;

        // 设置协作等级为全屏幕
        if ( FAILED(lpdd4->SetCooperativeLevel((HWND)main_window_handle, 
            DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE|
            DDSCL_ALLOWREBOOT  
            //|DDSCL_FPUSETUP  
            )) )
            return false;

        //设置显示模式
        if (!setScreenMode(ScreenWidth,ScreenHeight, ScreenColorBit))//设置默认模式
        {
            if (!trySetScreenMode(ScreenWidth,ScreenHeight, ScreenColorBit))
                return false;
        }

        // 清除Direct Draw 表面 描述，设置大小
        memset(&ddsd,0,sizeof(ddsd)); 
        ddsd.dwSize=sizeof(ddsd); 
        // 设置正确的属性标记
        ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
        // 设置缓冲表面数目（不包括主表面）
        ddsd.dwBackBufferCount = 1;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
        // 创建主表面
        if (FAILED(lpdd4->CreateSurface(&ddsd,&lpddsprimary, 0)))
            return false;

        // 现在请求主表面的附加表面
        ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
        // 得到附加的缓冲表面  不是必需的
        if (FAILED(lpddsprimary->GetAttachedSurface(&ddsd.ddsCaps, &lpddsback)))
            lpddsback=0; //附加表面申请不成功 但还可以执行

        m_DibSurface.Inti(ScreenWidth,ScreenHeight);
        m_FPSColor.Inti(20,100,10.0/1000,80.0/1000);

        return true;
    }

    bool DrawBegin(Context32& out_dst)
    {
        if (lpddsprimary==0) return false;

        if (lpdd4->TestCooperativeLevel()==DD_OK)//是否需要恢复表面
	        lpdd4->RestoreAllSurfaces(); 
        else
            return false;
        return m_DibSurface.lock_Data(out_dst);
    }

    void DrawEnd(Context32& dst)
    {
        long ScreenWidth=dst.width;
        long ScreenHeight=dst.height;
        m_DibSurface.unlock_Data(dst);

        LPDIRECTDRAWSURFACE4  dd;
        if (lpddsback!=0)
            dd=lpddsback;
        else
            dd=lpddsprimary;

        //拷贝数据到显存
        HDC dstDC=0;  
        if (FAILED(dd->GetDC(&dstDC))) return;
        HDC srcDC=(HDC)m_DibSurface.lock_DC();
        BitBlt(dstDC,0,0,ScreenWidth,ScreenHeight,srcDC,0,0,SRCCOPY);
        long newDrawTime=mmGetTime();
        if (m_OldDrawTime>0)
        {
            //HDC dstDC=srcDC;
            long stepTime=newDrawTime-m_OldDrawTime;
            if ((this->m_IsShowFPS)&&(stepTime>0))
            {
                this->m_FPS=1000.0/stepTime;
                m_FPSColor.Update(stepTime);
                char str[50];
	            gcvt(m_FPS,4,str);
	            SetBkColor(dstDC,RGB(0,0,0));
                SetTextColor(dstDC,m_FPSColor.getColor32());
                TextOut(dstDC,5,5,str,lstrlen(str));
	            TextOut(dstDC,45,5,"fps",3);
            }
        }
        m_OldDrawTime=newDrawTime;
        m_DibSurface.unlock_DC();
        dd->ReleaseDC(dstDC);
        
        if (lpddsback!=0)
        {
            while (FAILED(lpddsprimary->Flip(0, DDFLIP_WAIT)));// 交换表面
        }

    }
};

/////////////////////////////////////////////////////////////////////////////////////////


CDDrawImprot::CDDrawImprot() 
: m_Import(0) 
{
}


bool CDDrawImprot::IntiScreen(long ScreenWidth,long ScreenHeight,long ScreenColorBit,void * main_window_handle)
{
    if (m_Import==0)
        m_Import=new CDDraw();
    bool result=((CDDraw*)m_Import)->IntiScreen(ScreenWidth,ScreenHeight,ScreenColorBit,main_window_handle);
    if (!result)
        ClearImportPointer();
    return result;
}

void CDDrawImprot::ClearImportPointer()
{
    if (m_Import!=0)
    {
        CDDraw* Import=((CDDraw*)m_Import);
        m_Import=0;
        delete Import;
    }
}


CDDrawImprot::~CDDrawImprot()
{
    ClearImportPointer();
}

bool CDDrawImprot::DrawBegin(Context32& dst)
{
    if (m_Import==0)
        return false;
    else
        return ((CDDraw*)m_Import)->DrawBegin(dst);

}

void CDDrawImprot::DrawEnd(Context32& dst)
{
    if (m_Import!=0)
        ((CDDraw*)m_Import)->DrawEnd(dst);
}


void  CDDrawImprot::IsShowFPS(bool isShowFPS)
{
    if (m_Import!=0)
        ((CDDraw*)m_Import)->IsShowFPS(isShowFPS);
}

