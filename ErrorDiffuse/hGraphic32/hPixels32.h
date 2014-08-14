//hPixels32.h


#ifndef _hPixels32_h_
#define _hPixels32_h_

#include "hColor32.h"


class TPixels32:public IPixels32Buf{
private:
    TPixels32Ref m_data;
protected:
    virtual void getNewMemory(const long width,const long height,TPixels32Ref& out_ref);
    virtual void delMemory(TPixels32Ref& ref);
    inline void setNewSize(int width, int height) {
        delMemory(m_data);
        getNewMemory(width, height,m_data);
    }
public:
    inline explicit TPixels32():m_data() {}
    inline explicit TPixels32(const long width,const long height):m_data() { resizeFast(width,height); }
    virtual  ~TPixels32() { delMemory(m_data); }

    virtual void resizeFast(long width,long height){
        if ((width > 0) && (height > 0)) {
            if ((width != m_data.width) || (height != m_data.height))
                setNewSize(width,height);
        } else
            delMemory(m_data);
    }
    
    inline long getWidth() const { return m_data.width; }
    inline long getHeight() const { return m_data.height; }
    inline const TPixels32Ref& getRef() const { return m_data; }
    //
    virtual bool lockBuf(TPixels32Ref& out_color32Ref){ out_color32Ref=getRef(); return true;}
    virtual void unlockBuf(){}
};


#endif //hPixels32_h_
