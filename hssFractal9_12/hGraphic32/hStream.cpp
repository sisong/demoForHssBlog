//hStream.cpp
#include "hStream.h"
#include <stdio.h>
//#include <io.h> 
#include <string>

//file import
    #define csNullFileHandle 0

    #pragma warning(disable:4996)
    //declared deprecated
    static inline void* _file_create(const char* fileName){
        FILE* file=fopen(fileName,"wb"); 
        return file;
    }
    static inline void* _file_open_read(const char* fileName){
        FILE* file=fopen(fileName,"rb"); 
        return file;
    }

    static void _file_close(void* fileHandle){
        FILE* file=(FILE*)fileHandle;
        fclose(file);
    }
    static long _get_file_size(void* fileHandle){
        FILE* file=(FILE*)fileHandle;
        if (file==csNullFileHandle)
            return 0;
        else{
            int oldPos = ftell(file);   
            fseek(file,0,SEEK_END);
            int file_length = ftell(file);   
            fseek(file,oldPos,SEEK_SET);
            return file_length;    
        }
    }
    static long _get_file_pos(void* fileHandle){
        FILE* file=(FILE*)fileHandle;
        if (file==csNullFileHandle)
            return 0;
        else{
            return ftell(file);   
        }
    }
    static long _set_file_pos(void* fileHandle,long pos){
        FILE* file=(FILE*)fileHandle;
        if (file==csNullFileHandle)
            return 0;
        else
            return fseek(file,pos,SEEK_SET);
    }


    static long _file_read(void* fileHandle,void* dst,unsigned long readSize){
        FILE* file=(FILE*)fileHandle;
        if (file==csNullFileHandle)
            return 0;
        else{
            return (long)fread(dst,1,readSize,file);
        }    
    }

    static long _file_write(void* fileHandle,const void* src,unsigned long writeSize){
        FILE* file=(FILE*)fileHandle;
        if (file==csNullFileHandle)
            return 0;
        else{
            return (long)fwrite(src,1,writeSize,file);
        }    
    }

///////////////////////////////////////////////////////////////////////////

TFileInputStream::TFileInputStream(const char* fileName)
:m_fileHandle(csNullFileHandle),m_buf(0),m_bufSize(0){
    m_fileHandle=_file_open_read(fileName); 
    if (m_fileHandle==csNullFileHandle)
        throwException("TFileInputStream _file_open_read ERROR!"); 
}

TFileInputStream::~TFileInputStream(){
    if (m_buf!=0) {
        delete[]m_buf;
        m_buf=0;
    }
    if (m_fileHandle!=csNullFileHandle){
        _file_close(m_fileHandle);
        m_fileHandle=csNullFileHandle;
    }
}

void TFileInputStream::resizeBuf(long bufSize){
    if (bufSize>m_bufSize){
        if (m_buf!=0) {
            delete[]m_buf;
            m_buf=0;
        }

        if (bufSize>0)
            m_buf=new unsigned char[bufSize];
        m_bufSize=bufSize;
    }
}

long TFileInputStream::getSize()const{
    return _get_file_size(m_fileHandle);
}
long TFileInputStream::getPos()const{
    return _get_file_pos(m_fileHandle);
}
void TFileInputStream::setPos(long pos){
    _set_file_pos(m_fileHandle,pos);
}

void* TFileInputStream::read(long bCount){
    resizeBuf(bCount);
    read(m_buf,bCount);
    return m_buf;
}

void  TFileInputStream::read(void* Dst,long bCount){
    long oldPos=this->getPos();
    long fileSize=this->getSize();
    if (fileSize-oldPos<bCount)
        throwException("TFileInputStream::read ERROR!"); 
    else
        _file_read(m_fileHandle,Dst,bCount);
}

bool TFileInputStream::try_read(void* Dst,long bCount)const{ //尝试读一小块数据，但不移动读指针
    long oldPos=this->getPos();
    long fileSize=this->getSize();
    if (fileSize-oldPos<bCount)
        return false;

    TFileInputStream* self=(TFileInputStream*)this;
    self->read(Dst,bCount);
    self->setPos(oldPos);
    return true;
}

void TFileInputStream::skip(unsigned long bCount){
    long oldPos=this->getPos();
    long fileSize=this->getSize();
    if (fileSize-oldPos<(long)bCount)
        throwException("TFileInputStream::skip ERROR!"); 
    else
        this->setPos(oldPos+bCount);
}


////////

TFileOutputStream::TFileOutputStream(const char* aFileName)
:m_fileHandle(0){
    m_fileHandle=_file_create(aFileName);
    if (m_fileHandle==csNullFileHandle)
        throwException("TFileOutputStream _file_create ERROR!"); 
}
TFileOutputStream::~TFileOutputStream(){
    if (m_fileHandle!=csNullFileHandle){
        _file_close(m_fileHandle); 
        m_fileHandle=0;
    }
}
void TFileOutputStream::write(const void* Src,long bCount){
    _file_write(m_fileHandle,Src,bCount);
}



