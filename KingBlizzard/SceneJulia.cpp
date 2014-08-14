#include "SceneJulia.h"
#include "MMTime.h"
#include "math.h"

must_inline bool M_getIsOut4(const double x,const double y)
{
    return (x*x+y*y)>=4;
}
bool M_getIsInRegion(const double x0,const double y0,const long MaxTestIteratCount)
{
    if (M_getIsOut4(x0,y0)) return false;
    double x=x0; double y=y0;
    for (long i=0;i<MaxTestIteratCount;++i)
    {
        //复函数迭代  f(C)=C^2+C0 
        double new_x=x*x-y*y+x0;
        double new_y=2*x*y+y0;
        if (M_getIsOut4(new_x,new_y)) return false;
        x=new_x; y=new_y;
    }
    return true;
}


must_inline double sqr(const double x) { return x*x; }

bool J_getIsInRegion(const double Mx0,const double My0,const double Jx0,const double Jy0,const long TestIteratCount,double& out_new_DJx,double& out_new_DJy,long& out_IteratCount)
{
    double x=Jx0; double y=Jy0;
    if (M_getIsOut4(Jx0,Jy0))
    {
        out_IteratCount=0;
        return false;
    }
    const long csTestDXYCount=20;
    long i;
    for (i=0;i<csTestDXYCount;++i)
    {
        //复函数迭代  f(C)=C^2+MC0; 
        double new_x=x*x-y*y+Mx0;
        double new_y=2*x*y+My0;
        if (M_getIsOut4(new_x,new_y)) 
        {
            out_IteratCount=i+1;
            return false;
        }
        x=new_x; y=new_y;
    }
    double new_dx,new_dy;
    {
        i=csTestDXYCount;
        double new_x=x*x-y*y+Mx0;
        double new_y=2*x*y+My0;
        if (M_getIsOut4(new_x,new_y)) 
        {
            out_IteratCount=i+1;
            return false;
        }
        new_dx=new_x-x;
        new_dy=new_y-y;
    }
    for (i=csTestDXYCount+1;i<TestIteratCount+1;++i)
    {
        //复函数迭代  f(C)=C^2+MC0; 
        double new_x=x*x-y*y+Mx0;
        double new_y=2*x*y+My0;
        if (M_getIsOut4(new_x,new_y)) 
        {
            out_IteratCount=i+1;
            return false;
        }
        x=new_x; y=new_y;
    }
    out_new_DJx=(abs(new_dx)+abs(new_dy));
    out_new_DJy=sqrt(abs(sqr(new_dx)+sqr(new_dy)));
    return true;
}

const long MWidth   =512;
const long MHeight  =MWidth+1;
const double rLeft   =-2.02;
const double rRight  = 2.02;
const double rTop    =-2.02;
const double rBottom = 2.02;
const double xMScale= ((rRight-rLeft)/MWidth);
const double yMScale= ((rBottom-rTop)/MHeight);

must_inline long MIndex(long x,long y)
{
    return y*MWidth+x;
}

void CSceneJulia::Inti()
{
    m_BufBckIsOk=false;
    m_UpdateTime=0;

    //得到M集合的迭代逸出数据

    std::vector<char> _MDataTmp(MWidth*MHeight);
    char* MDataTmp=&_MDataTmp[0];
    const long TestIteratCount=80;
    for (long y=0;y<=MHeight/2;++y)
    {
        double ry0=y*yMScale+rTop;
        for (long x=0;x<MWidth;++x)
        {
            double rx0=x*xMScale+rLeft;
            bool isInRegion=M_getIsInRegion(rx0,ry0,TestIteratCount);
            MDataTmp[MIndex(x,y)]=(char)isInRegion;            
            MDataTmp[MIndex(x,MHeight-1-y)]=(char)isInRegion;
        }
    }

    _m_MData.resize(MWidth*MHeight,0);
    m_MData=&_m_MData[0];
    for (long y=1;y<=MHeight/2;++y)
    {
        for (long x=1;x<MWidth-1;++x)
        {
            long sum=  MDataTmp[MIndex(x-1,y-1)]+MDataTmp[MIndex(x,y-1)]+MDataTmp[MIndex(x+1,y-1)]
                      +MDataTmp[MIndex(x-1,y  )]+MDataTmp[MIndex(x,y  )]+MDataTmp[MIndex(x+1,y  )]
                      +MDataTmp[MIndex(x-1,y+1)]+MDataTmp[MIndex(x,y+1)]+MDataTmp[MIndex(x+1,y+1)] ;

            bool isInRegion=((1<<sum)&((1<<9)-2))!=0;
            m_MData[MIndex(x,y)]=(char)isInRegion;
            m_MData[MIndex(x,MHeight-1-y)]=(char)isInRegion;
        }
    }


    Scene_Update(0);
}
    
void CSceneJulia::Scene_Clear()
{
    _m_MData.clear();
    m_MData=0;
}
    
void CSceneJulia::Scene_Update(unsigned long StepTime_ms)
{
    //m_ColorMoverIn.Update(StepTime_ms);
    //m_ColorMoverOut.Update(StepTime_ms);
    m_ColorMoverIn.Inti(300,450,200000.0/1000,400000.0/1000);
    m_ColorMoverOut.Inti(250,350,200000.0/1000,400000.0/1000);

    while (true)
    {
        double Mx0Rnd=((rand()*RAND_MAX) + rand())*(1.0/RAND_MAX/RAND_MAX);
        double My0Rnd=((rand()*RAND_MAX) + rand())*(1.0/RAND_MAX/RAND_MAX);
        long iMx0=(long)(Mx0Rnd*(MWidth-0.0001));
        long iMy0=(long)(My0Rnd*(MHeight-0.0001));
        if (m_MData[MIndex(iMx0,iMy0)]) 
        {
            m_Mx0=Mx0Rnd*(rRight-rLeft)+rLeft;
            m_My0=My0Rnd*(rBottom-rTop)+rTop;
            break;
        }
    }
}

static void Debug_DrawM(Context32* dst,char* MData)
{
    Color32* dstLine=dst->pdata;
    for (long y=0;y<dst->height;++y)
    {
        long my=y%MHeight;
        for (long x=0;x<dst->width;++x)
        {
            long mx=x%MWidth;
            long c=MData[MIndex(mx,my)]*255;
            dstLine[x]=Color32(c,c,c);
        }
        ((unsigned char*&)dstLine)+=dst->byte_width;
    }
}

inline Color32 getColor(const CColorMover& ColorMoverIn,const CColorMover& ColorMoverOut,double Mx0,double My0,double rx0,double ry0)
{
     double dx,dy;
     long iteratCount;
     if (J_getIsInRegion(Mx0,My0,rx0,ry0,1000,dx,dy,iteratCount))
         return Color32(ColorMoverIn.getR8(dx),ColorMoverIn.getG8(dy),ColorMoverIn.getB8(dx+dy));
     else
     {
         if (iteratCount<40)
             return Color32(0);
         else
         {
             double d=sqrt(sqrt((double)iteratCount));
             return Color32(ColorMoverOut.getR8(d),ColorMoverOut.getG8(d+63),ColorMoverOut.getB8(d+127));
         }
     }
}
void CSceneJulia::DoDraw(Context32* out_dst)
{
    //Debug_DrawM(dst,m_MData);  return;

    if (!m_BufBckIsOk)
    {
        Context32 dst;
        m_BufBck.Inti(out_dst->width,out_dst->height);
        m_BufBck.lock_Data(dst);

        const double zZoomX=0.75;
        const double zZoomY=0.6;
        const double xJScale= ((rRight-rLeft)*zZoomX/dst.width);
        const double yJScale= ((rBottom-rTop)*zZoomY/dst.height);
        
        const CColorMover& ColorMoverIn=m_ColorMoverIn;
        const CColorMover& ColorMoverOut=m_ColorMoverOut;
        Color32* dstLine=dst.pdata;
        for (long y=0;y<dst.height;++y)
        {
            double ry0=y*yJScale+rTop*zZoomY;
            for (long x=0;x<dst.width;++x)
            {
                double rx0=x*xJScale+rLeft*zZoomX;

                dstLine[x]=getColor(ColorMoverIn,ColorMoverOut,m_Mx0,m_My0,rx0,ry0);
            }
            ((unsigned char*&)dstLine)+=dst.byte_width;
        }

        m_BufBck.unlock_Data(dst);
        m_BufBckIsOk=true;

        return; //留到下一帧更新
    }


    const unsigned long StepTime=3500;    

    unsigned long nowTime=mmGetTime();
    if (nowTime>=(m_UpdateTime+StepTime)) 
    {
        Context32 bck;
        m_BufBck.lock_Data(bck);
        out_dst->copy(bck);
        m_BufBck.unlock_Data(bck);

        m_UpdateTime=nowTime;
        m_BufBckIsOk=false;
    }
}