//hBmpFile.cpp

#include "hBmpFile.h"
#include <assert.h>
//#include <Windows.h>

void rgbColorToColor32_line(void* pLinePixels,long biBitCount,const Color32* colorTable,Color32* dstLine,long width);

///////////

typedef UInt16 tWORD;
typedef UInt32 tDWORD;
typedef long   tLONG;
typedef UInt8  tBYTE;

struct tBITMAPFILEHEADER_NO_TYPE {
    //tWORD    bfType;
    tDWORD   bfSize;
    tWORD    bfReserved1;
    tWORD    bfReserved2;
    tDWORD   bfOffBits;
} ;

struct tBITMAPINFOHEADER{
    tDWORD      biSize;
    tLONG       biWidth;
    tLONG       biHeight;
    tWORD       biPlanes;
    tWORD       biBitCount;
    tDWORD      biCompression;
    tDWORD      biSizeImage;
    tLONG       biXPelsPerMeter;
    tLONG       biYPelsPerMeter;
    tDWORD      biClrUsed;
    tDWORD      biClrImportant;
};

struct tRGBQUAD {
    tBYTE    rgbBlue;
    tBYTE    rgbGreen;
    tBYTE    rgbRed;
    tBYTE    rgbReserved;
};

#define csBI_RGB        0L
#define csBI_RLE8       1L
#define csBI_RLE4       2L
#define csBI_BITFIELDS  3L
#define csBI_JPEG       4L
#define csBI_PNG        5L


////////

const UInt16 csBmpTag='B'|('M'<<8);
const long bmpTagSize=sizeof(csBmpTag); //BM

bool TBmpFile::checkHeadType(const IInputStream*  aInputStream){
    char tag[bmpTagSize];
    if (!aInputStream->try_read(&tag[0],bmpTagSize)) return false;
    return ((tag[0]=='b')||(tag[0]=='B')) && ((tag[1]=='m')||(tag[1]=='M'));
}

void TBmpFile::load(IInputStream* aInputStream,IPixels32Buf* dst){
    long ReadPos=0;
    
    if (!checkHeadType(aInputStream))
        throwException("bmp file \"BM\" tag error!");
    aInputStream->skip(bmpTagSize);
    ReadPos+=bmpTagSize;
    
    //位图文件头结构和位图信息结构
    tBITMAPFILEHEADER_NO_TYPE bmFileHeader;//读出BITMAPFILEHEADER结构
    memset(&bmFileHeader,0,sizeof(bmFileHeader));
    aInputStream->read((unsigned char *)&bmFileHeader,sizeof(bmFileHeader));
    ReadPos+=sizeof(bmFileHeader);
    
    tBITMAPINFOHEADER bmpInfo;//
    memset(&bmpInfo,0,sizeof(bmpInfo));
    aInputStream->read((unsigned char *)&bmpInfo.biSize,sizeof(bmpInfo.biSize));
    long biSize=bmpInfo.biSize;
    if (sizeof(bmpInfo)<biSize)
        biSize=sizeof(bmpInfo);
    aInputStream->read((void*)((unsigned char *)&bmpInfo.biSize+sizeof(tDWORD)),(long)(biSize-sizeof(tDWORD)));
    if (sizeof(bmpInfo)<bmpInfo.biSize) 
        aInputStream->skip(bmpInfo.biSize-sizeof(bmpInfo));
    ReadPos+=bmpInfo.biSize;
    
    bool IsRevY=bmpInfo.biHeight>0;
    bmpInfo.biHeight=abs(bmpInfo.biHeight);
    
    tDWORD nPalletteNumColors=0;
    if (bmpInfo.biBitCount<=8){
        nPalletteNumColors=bmpInfo.biClrUsed;
        if (nPalletteNumColors==0 )
            nPalletteNumColors=1 << bmpInfo.biBitCount;
    }
    
    
    long incBmpByteWidth=(((bmpInfo.biWidth*bmpInfo.biBitCount)+31) / (1<<5) ) << 2;
    if (bmpInfo.biSizeImage == 0) 
        bmpInfo.biSizeImage = abs(incBmpByteWidth * bmpInfo.biHeight);
    
    tRGBQUAD Pallette[256];
    //调色板
    if (nPalletteNumColors>0){
        aInputStream->read(&Pallette[0],nPalletteNumColors*sizeof(tRGBQUAD));
        ReadPos+=nPalletteNumColors*sizeof(tRGBQUAD);
        for (long i=0;i<(long)nPalletteNumColors;++i)
            Pallette[i].rgbReserved=0xFF;
    }
    else if (bmpInfo.biBitCount==18 ){
        bmpInfo.biBitCount=18;
    }else if (bmpInfo.biBitCount==32 ){        
        if (bmpInfo.biCompression==csBI_BITFIELDS ){
            bmpInfo.biBitCount=18;
            tDWORD RMask,GMask,BMask;
            aInputStream->read(&RMask,sizeof(tDWORD));
            aInputStream->read(&GMask,sizeof(tDWORD));
            aInputStream->read(&BMask,sizeof(tDWORD));
            ReadPos+=(3*sizeof(tDWORD));
            if (GMask==(((1<<6)-1)<<6) )
                bmpInfo.biBitCount=18;
        }
    }else if (bmpInfo.biBitCount==16 ){
        //区分 RGB 15bit,RGB 16bit
        if (bmpInfo.biCompression==csBI_BITFIELDS ){
            tDWORD RMask,GMask,BMask;
            aInputStream->read(&RMask,sizeof(tDWORD));
            aInputStream->read(&GMask,sizeof(tDWORD));
            aInputStream->read(&BMask,sizeof(tDWORD));
            ReadPos+=(3*sizeof(tDWORD));
            if (GMask==0x03E0 ){
                assert(RMask==0x7C00);
                assert(BMask==0x001F);
                bmpInfo.biBitCount=15;
            }
            else if (GMask==(((1<<4)-1)<<4) )
                bmpInfo.biBitCount=12;
            else if (GMask==(((1<<6)-1)<<6) )
                bmpInfo.biBitCount=18;
            else {
                assert(RMask==0xF800);
                assert(GMask==0x07E0);
                assert(BMask==0x001F);
                bmpInfo.biBitCount=16;
            }
        }
        else
            bmpInfo.biBitCount=15;
    }
    
    assert(bmpInfo.biPlanes==1);
    assert(bmpInfo.biCompression!=csBI_RLE4);//不支持压缩格式
    assert(bmpInfo.biCompression!=csBI_RLE8);//不支持压缩格式
    
    dst->resizeFast(bmpInfo.biWidth,bmpInfo.biHeight);
    
    incBmpByteWidth=abs(incBmpByteWidth);
    
    aInputStream->skip(bmFileHeader.bfOffBits-ReadPos);
    ReadPos=bmFileHeader.bfOffBits;
    UInt8* pLinePixels=(UInt8*)aInputStream->read(bmpInfo.biSizeImage);
    ReadPos+=bmpInfo.biSizeImage;
    
    TPixels32Ref dstRef;
    bool isLockOk=dst->lockBuf(dstRef);
    assert(isLockOk);
    if (isLockOk){
        for (long y=0;y<bmpInfo.biHeight;++y)
        {
            long py=y;
            if (IsRevY) py=bmpInfo.biHeight-1-y;
            rgbColorToColor32_line(pLinePixels,bmpInfo.biBitCount,(Color32*)(&Pallette[0]),dstRef.getLinePixels(py),dstRef.width);
            pLinePixels+=incBmpByteWidth;
        }
        dst->unlockBuf();
    }
}

static long get_bmp_byte_width(long colorLineByteWidth){
    return (colorLineByteWidth+3)/4*4; //四字节对齐
}

void TBmpFile::save(const TPixels32Ref& src,IOutputStream* aOutputStream){
    long colorLineByteWidth=src.width*sizeof(Color32);
    long BmpByteWidth=get_bmp_byte_width(colorLineByteWidth);
    
    tWORD    bfType=csBmpTag;
    //写入类型信息
    aOutputStream->write(&bfType, sizeof(bfType));
    
    //填充BITMAPFILEHEADER结构
    tBITMAPFILEHEADER_NO_TYPE bmFileHeader;//位图文件头结构和位图信息结构
    memset(&bmFileHeader,0,sizeof(bmFileHeader));
    tDWORD dwDibBitsSize=src.height*abs(BmpByteWidth);
    tDWORD dwOffBits =sizeof(bfType)+ sizeof(tBITMAPFILEHEADER_NO_TYPE) + sizeof(tBITMAPINFOHEADER); //+palette_byte_size();
    
    tDWORD dwFileSize = dwOffBits + dwDibBitsSize;
    bmFileHeader.bfSize = dwFileSize;
    bmFileHeader.bfReserved1 = 0;
    bmFileHeader.bfReserved2 = 0;
    bmFileHeader.bfOffBits = dwOffBits;
    
    //写入文件头和位图信息
    aOutputStream->write(&bmFileHeader, sizeof(tBITMAPFILEHEADER_NO_TYPE));
    
    //
    tBITMAPINFOHEADER bmpInfo; 
    memset(&bmpInfo,0,sizeof(bmpInfo));
    bmpInfo.biSize= sizeof(tBITMAPINFOHEADER);
    bmpInfo.biWidth=src.width;
    bmpInfo.biHeight=src.height;
    bmpInfo.biPlanes=1;
    bmpInfo.biBitCount=sizeof(Color32)*8;
    bmpInfo.biCompression=0;
    bmpInfo.biSizeImage=0;
    bmpInfo.biXPelsPerMeter=0;
    bmpInfo.biYPelsPerMeter=0;
    bmpInfo.biClrUsed=0;
    bmpInfo.biClrImportant=0;
    aOutputStream->write(&bmpInfo, sizeof(tBITMAPINFOHEADER));
    
    //write_palette(aOutputStream);
    
    //写入数据             
    for (long y=bmpInfo.biHeight-1;y>=0;--y){
        aOutputStream->write(src.getLinePixels(y),colorLineByteWidth);
        //for (long i=colorLineByteWidth;i<BmpByteWidth;++i){ char tmp=0; aOutputStream->write(&tmp,1);
        assert(colorLineByteWidth==BmpByteWidth);
    }
}

void TBmpFile::saveAsColor16(const TPixels32Ref& src,IOutputStream* aOutputStream,long rbit,long gbit,long bbit){
    long clSumBit=rbit+gbit+bbit;
    long clBit;
    if (clSumBit>16) 
        clBit=32;
    else 
        clBit=16;
    long colorLineByteWidth=src.width*(clBit/8);
    long BmpByteWidth=get_bmp_byte_width(colorLineByteWidth);
    
    tWORD    bfType=csBmpTag;
    //写入类型信息
    aOutputStream->write(&bfType, sizeof(bfType));
    
    //填充BITMAPFILEHEADER结构
    tBITMAPFILEHEADER_NO_TYPE bmFileHeader;//位图文件头结构和位图信息结构
    memset(&bmFileHeader,0,sizeof(bmFileHeader));
    tDWORD dwDibBitsSize=src.height*abs(BmpByteWidth);
    tDWORD dwOffBits =sizeof(bfType)+ sizeof(tBITMAPFILEHEADER_NO_TYPE) + sizeof(tBITMAPINFOHEADER)+3*sizeof(tDWORD); //+palette_byte_size();
    
    tDWORD dwFileSize = dwOffBits + dwDibBitsSize;
    bmFileHeader.bfSize = dwFileSize;
    bmFileHeader.bfReserved1 = 0;
    bmFileHeader.bfReserved2 = 0;
    bmFileHeader.bfOffBits = dwOffBits;

   
    //写入文件头和位图信息
    aOutputStream->write(&bmFileHeader, sizeof(tBITMAPFILEHEADER_NO_TYPE));
    
    //
    tBITMAPINFOHEADER bmpInfo; 
    memset(&bmpInfo,0,sizeof(bmpInfo));
    bmpInfo.biSize= sizeof(tBITMAPINFOHEADER);
    bmpInfo.biWidth=src.width;
    bmpInfo.biHeight=src.height;
    bmpInfo.biPlanes=1;
    bmpInfo.biBitCount=(tWORD)clBit;
    bmpInfo.biCompression=csBI_BITFIELDS;
    bmpInfo.biSizeImage=0;
    bmpInfo.biXPelsPerMeter=0;
    bmpInfo.biYPelsPerMeter=0;
    bmpInfo.biClrUsed=0;
    bmpInfo.biClrImportant=0;
    aOutputStream->write(&bmpInfo, sizeof(tBITMAPINFOHEADER));

    tDWORD RMask=((1<<rbit)-1)<<(gbit+bbit);
    tDWORD GMask=((1<<gbit)-1)<<bbit;
    tDWORD BMask=((1<<bbit)-1);
    aOutputStream->write(&RMask,sizeof(tDWORD));
    aOutputStream->write(&GMask,sizeof(tDWORD));
    aOutputStream->write(&BMask,sizeof(tDWORD));
    //write_palette(aOutputStream);
    
    UInt8* dbuf=new UInt8[BmpByteWidth];
    for (long i=colorLineByteWidth;i<BmpByteWidth;++i)
        dbuf[i]=0;
    UInt16* dbuf_16=0;
    UInt32* dbuf_32=0;
    if (clBit==32)
        dbuf_32=(UInt32*)dbuf;
    else
        dbuf_16=(UInt16*)dbuf;

    //写入数据             
    for (long y=bmpInfo.biHeight-1;y>=0;--y){
        const Color32* s32=src.getLinePixels(y);
        for (long x=0;x<bmpInfo.biWidth;++x){
            //UInt32 cl=(s32[x].r<<(rbit+gbit+bbit-8))|(s32[x].g<<(gbit+bbit-8))|(s32[x].b>>(8-bbit));
            UInt32 cl=(s32[x].r>>(8-rbit)<<(gbit+bbit))|(s32[x].g>>(8-gbit)<<bbit)|(s32[x].b>>(8-bbit));
            if (clBit==32)
                dbuf_32[x]=cl;
            else
                dbuf_16[x]=(UInt16)cl;
        }
        aOutputStream->write(dbuf,BmpByteWidth);
    }
    delete[] dbuf;
}


//////////////////////////
 struct Color24{
    UInt8  b;
    UInt8  g;
    UInt8  r;
 };

template<int rbit,int gbit,int bbit>
struct TColor16bit{
    UInt16  _b:bbit;
    UInt16  _g:gbit;
    UInt16  _r:rbit;
    inline UInt8 b8()const{ return _b<<(8-bbit); }
    inline UInt8 g8()const{ return _g<<(8-gbit); }
    inline UInt8 r8()const{ return _r<<(8-rbit); }
};
typedef TColor16bit<5,6,5> Color16_565;
typedef TColor16bit<5,5,5> Color16_555;
typedef TColor16bit<4,4,4> Color16_444;
typedef TColor16bit<6,6,6> Color16_666;
//typedef TColor16bit<6,5,5> Color16_655;

void rgbColorToColor32_line(void* pLinePixels,long biBitCount,const Color32* colorTable,Color32* dstLine,long width){
    switch (biBitCount){
        case 32:{
            const Color32* srcLine=(const Color32*)pLinePixels;
            for (long x=0;x<width;++x) dstLine[x]=srcLine[x];
            return;
        }
        case 24:{
            const Color24* srcLine=(const Color24*)pLinePixels;
            for (long x=0;x<width;++x) dstLine[x]=Color32(srcLine[x].r,srcLine[x].g,srcLine[x].b);
            return;
         }
        case 18:{
            const Color16_666* srcLine=(const Color16_666*)pLinePixels;
            for (long x=0;x<width;++x) dstLine[x]=Color32(srcLine[x].r8(),srcLine[x].g8(),srcLine[x].b8());
            return;
        }
        case 16:{
            const Color16_565* srcLine=(const Color16_565*)pLinePixels;
            for (long x=0;x<width;++x) dstLine[x]=Color32(srcLine[x].r8(),srcLine[x].g8(),srcLine[x].b8());
            return;
        }
        case 15:{
            const Color16_555* srcLine=(const Color16_555*)pLinePixels;
            for (long x=0;x<width;++x) dstLine[x]=Color32(srcLine[x].r8(),srcLine[x].g8(),srcLine[x].b8());
            return;
         }
        case 12:{
            const Color16_444* srcLine=(const Color16_444*)pLinePixels;
            for (long x=0;x<width;++x) dstLine[x]=Color32(srcLine[x].r8(),srcLine[x].g8(),srcLine[x].b8());
            return;
         }
        case  8:{
            assert(colorTable!=0);
            const UInt8* srcLine=(const UInt8*)pLinePixels;
            for (long x=0;x<width;++x) dstLine[x]=colorTable[srcLine[x]];
            return;
         }
        case  4:{
            assert(colorTable!=0);
            const UInt8* srcLine=(const UInt8*)pLinePixels;
            for (long x=0;x<(width>>1);++x){
                unsigned int tmpByte=srcLine[x];
                dstLine[0] =colorTable[tmpByte >> 4];
                dstLine[1] =colorTable[tmpByte & 0xF];
                dstLine=&dstLine[2];
            }
            if ((width&1)!=0) {
                unsigned int tmpByte=srcLine[width>>1]; // is safe
                dstLine[0] =colorTable[tmpByte >> 4];
            }
            return;
         }
        case  1:{
            assert(colorTable!=0);
            const UInt8* srcLine=(const UInt8*)pLinePixels;
            for (long x=0;x<(width>>3);++x){
                unsigned int tmpByte=srcLine[x];
                dstLine[0] =colorTable[ tmpByte >> 7];
                dstLine[1] =colorTable[(tmpByte >> 6) & 1];
                dstLine[2] =colorTable[(tmpByte >> 5) & 1];
                dstLine[3] =colorTable[(tmpByte >> 4) & 1];
                dstLine[4] =colorTable[(tmpByte >> 3) & 1];
                dstLine[5] =colorTable[(tmpByte >> 2) & 1];
                dstLine[6] =colorTable[(tmpByte >> 1) & 1];
                dstLine[7] =colorTable[tmpByte & 1];

                dstLine=&dstLine[8];
            }
            
            long borderWidth=width & 7;
            if (borderWidth>0){
                unsigned int tmpByte=srcLine[width >> 3];// is safe
                for (long x=0;x<borderWidth;++x)
                    dstLine[x]=colorTable[(tmpByte >> (7-x)) & 1];
            }
            return;
         }
    }
    assert(false);
}
