//hBmpFile.h


#ifndef _hBmpFile_h_
#define _hBmpFile_h_

#include "hColor32.h"
#include "hStream.h"
#include <assert.h>


struct TBmpFileError:public std::runtime_error{
    TBmpFileError(const std::string& error) :std::runtime_error(error){}
};


//bmp文件加载和保存
//注意支持1,4,8,15,16,24,32bitRGB颜色 不支持压缩格式等
class TBmpFile{
public:
    static bool checkHeadType(const IInputStream*  aInputStream);
private:
    static inline void throwException(const std::string& error){
        throw TFileInputStreamError(error);
    }
public:
    static void load(IInputStream* aInputStream,IPixels32Buf* dst);
    static void save(const TPixels32Ref& src,IOutputStream* aOutputStream);
    static inline void save(IPixels32Buf* src,IOutputStream* aOutputStream){
        TPixels32Ref ref;
        bool isLockOk=src->lockBuf(ref);
        assert(isLockOk);
        if (isLockOk){
            save(ref,aOutputStream);
            src->unlockBuf();
        }
    }
};


#endif
