//Surface.h

#ifndef __Surface_h_
#define __Surface_h_

#include "HColor.h"

class CSurface
{
private:
    Context32 m_data;
public:
    CSurface() { m_data.pdata=0; Clear(); }
    void Inti(const unsigned long width,const unsigned long height) {
        if ((width*height)!=(m_data.width*m_data.height))
        {
            Clear();
            if ((width>=1)&&(height>=1))
            {
                m_data.pdata=new Color32[width*height];
            }
        }
        m_data.byte_width=width*sizeof(Color32);
        m_data.width=width;
        m_data.height=height;
    }
    void Clear() { 
        if (m_data.pdata!=0) delete[]m_data.pdata;
        m_data.pdata=0;
        m_data.byte_width=0;
        m_data.width=0;
        m_data.height=0;
    }
    ~CSurface()  { Clear(); }
    inline bool    lock_Data(Context32& out_dst) { out_dst=m_data; return true; }
    inline void    unlock_Data(Context32& dst)   {}
};

#endif //__Surface_h_
