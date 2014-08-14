//hDibImport.cpp
#include "hDibImport.h"
#include "assert.h"
#include "windows.h"

class CDib
{
private:
	HBITMAP     m_hDIB;
	HDC		    m_MemDC;
	HGDIOBJ	    m_OldObject;
    Context32   m_Pixels32BufData;
protected:
    void new_pixel_buf(Context32& result,const long width,const long height);
    void del_pixel_buf(Context32& aPixels32BufData) ;
public:
    CDib(const long width,const long height):m_MemDC(0),m_hDIB(0),m_OldObject(0){ new_pixel_buf(m_Pixels32BufData,width,height); }
	~CDib(){ unlock_DC(); del_pixel_buf(m_Pixels32BufData); }
	HDC  lock_DC();
	void unlock_DC();
    bool lock_Data(Context32& out_dst) { out_dst=m_Pixels32BufData; return (m_Pixels32BufData.pdata!=0); }
    void unlock_Data(Context32& dst) { assert(m_Pixels32BufData.pdata=dst.pdata); }
};

HBITMAP	create_DIB(int nWidth , int nHeight,int nBitCount,BYTE*&	out_pBits,long& out_WidthBytes)
{
	out_WidthBytes=0;
	out_pBits=0;

	BITMAPINFOHEADER	_BI;
	memset(&_BI,0,sizeof(BITMAPINFOHEADER));
	_BI.biSize= sizeof(BITMAPINFOHEADER);
	_BI.biWidth=nWidth;
	_BI.biHeight=nHeight;
	_BI.biPlanes=1;
	_BI.biBitCount=nBitCount;
	_BI.biCompression=0;
	_BI.biSizeImage=0;
	_BI.biXPelsPerMeter=0;
	_BI.biYPelsPerMeter=0;
	_BI.biClrUsed=0;
	_BI.biClrImportant=0;
	HBITMAP _hBmp=CreateDIBSection(0,(BITMAPINFO*)&_BI,DIB_RGB_COLORS,(void**)&out_pBits,0,0);
	if (_hBmp!=0)
	{
		assert(out_pBits!=0);
		BITMAP bm;	
		::GetObject(_hBmp,sizeof(BITMAP),&bm);
		_BI.biSizeImage=bm.bmHeight*bm.bmWidthBytes;
		_BI.biClrUsed=1<<nBitCount;
		out_WidthBytes=bm.bmWidthBytes;
	}
	else
		assert(out_pBits==0);

	return _hBmp;
}

void delete_DIB(HBITMAP hDIB)
{
	if (hDIB!=0) DeleteObject(hDIB);
}

HDC create_DIB_DC(HBITMAP hDIB,HGDIOBJ& out_OldObject)
{
	assert(hDIB!=0);
	HDC MemDC=0;

	MemDC=CreateCompatibleDC(0);
	SetMapMode(MemDC,MM_TEXT);
	assert(out_OldObject==0);
	out_OldObject=SelectObject(MemDC,hDIB);

	assert(MemDC!=0);
	return MemDC;
}

void delete_DIB_DC(HDC MemDC,HGDIOBJ& out_OldObject)
{
	if (MemDC!=0)
	{
		SelectObject(MemDC,out_OldObject);
		out_OldObject=0;
		DeleteDC(MemDC);
	}
}


//////////////////////////////////////////

void CDib::new_pixel_buf(Context32& result,const long width,const long height)
{
	assert(m_MemDC==0);
	assert(m_hDIB==0);

	if ((width>0)&&(height>0))
	{
		BYTE*	out_pBits=0;
		long    out_WidthBytes;
		m_hDIB=create_DIB(width,height,sizeof(Color32)*8,out_pBits,out_WidthBytes);
		assert(m_hDIB!=0);
		result.pdata=(Color32*)out_pBits;
		result.byte_width=out_WidthBytes;
		result.width=width;
		result.height=height;
	}
    else
    {
		result.pdata=0;
		result.byte_width=0;
		result.width=0;
		result.height=0;
    }
}

void CDib::del_pixel_buf(Context32& aPixels32BufData)
{
	assert(m_MemDC==0);
	if (m_hDIB==0)
	{
		assert(aPixels32BufData.pdata==0);
	}
	else
	{
		delete_DIB(m_hDIB);
		m_hDIB=0;
	}

	aPixels32BufData.pdata=0; 
	aPixels32BufData.byte_width=0;
	aPixels32BufData.width=0;
	aPixels32BufData.height=0;
}

HDC  CDib::lock_DC()
{
	assert(m_hDIB!=0);
	assert(m_MemDC==0);

	m_MemDC=create_DIB_DC(m_hDIB,m_OldObject);
	return m_MemDC;
}

void CDib::unlock_DC()
{
	if (m_MemDC!=0)
	{
		delete_DIB_DC(m_MemDC,m_OldObject);
		m_MemDC=0;
	}
}


///////////////////

void CDibImport::Clear()
{
    if (m_Import!=0)
    {
        delete (CDib*)m_Import;
        m_Import=0;
    }
}

void CDibImport::Inti(const long width,const long height)
{
    Clear();
    m_Import=new CDib(width,height);
}

void*  CDibImport::lock_DC()
{
    return ((CDib*)m_Import)->lock_DC();
}

void   CDibImport::unlock_DC()
{
    ((CDib*)m_Import)->unlock_DC();
}

bool  CDibImport::lock_Data(Context32& out_dst)
{
    return ((CDib*)m_Import)->lock_Data(out_dst);
}

void  CDibImport::unlock_Data(Context32& dst)
{
    ((CDib*)m_Import)->unlock_Data(dst);
}