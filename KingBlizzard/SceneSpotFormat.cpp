#include "SceneSpotFormat.h"
#include "math.h"


void CSceneSpotFormat::IntiData()
{
    if (rand()<(RAND_MAX/2))
    {
        const int SpotCountListCount=6;
        static const int SpotCountList[SpotCountListCount]={3,4,5,11,36,73};
        m_SpotCount=SpotCountList[rand()%SpotCountListCount];
    }
    else
       m_SpotCount=rand()%(13-3)+3;
    //m_SpotCount=3;
    static const double rev_log2_log10=1.0/log10(2.0);
    m_IteratCount=(long)( 0.5+rev_log2_log10*log10((double)m_DrawCount/m_SpotCount) )-2;
    m_dataLength=m_SpotCount*(1<<(m_IteratCount+2))+7;
    m_S0x.resize(m_dataLength);
    m_S0y.resize(m_dataLength);
    m_S1x.resize(m_dataLength);
    m_S1y.resize(m_dataLength);
}

void CSceneSpotFormat::IntiMover()
{
    m_MoverList.resize(2);
    const double MinV=0.5 /10000;
    const double MaxV=2.0 /10000;
    m_MoverList[0].Inti(-1.7, 1.7,MinV,MaxV);
    m_MoverList[1].Inti(-1.1, 1.1,MinV,MaxV);

    m_ColorMoverList.resize(1);
    m_ColorMoverList[0].Inti(2,4,20.0/1000,40.0/1000);
}

template<class T>
must_inline T sqr(T x) { return x*x; }
void CSceneSpotFormat::NextPos()
{
    const double a=m_MoverList[0].GetValue();
    const double b=m_MoverList[1].GetValue();


    const double  PI=3.1415926535897932384626433832795;
    const double rev_SpotCount=1.0/m_SpotCount;
    for (long i=0;i<=m_SpotCount+4;++i)
    {
        double seta=(1.5+(i*2-3)*rev_SpotCount)*PI;
        m_S0x[i]=cos(seta);
        m_S0y[i]=sin(seta);
    }
    double t2=a;
    double t1=1-t2+sqr(sqr(b));
    t1*=0.5;  t2*=0.5;
    for (long k=0;k<=m_IteratCount;++k)
    {
        double* srcSx; double* srcSy;
        double* dstSx; double* dstSy;
        if ((k % 2)==0)
        {
            srcSx=&m_S0x[0]; srcSy=&m_S0y[0];
            dstSx=&m_S1x[0]; dstSy=&m_S1y[0];
        }
        else
        {
            srcSx=&m_S1x[0]; srcSy=&m_S1y[0];
            dstSx=&m_S0x[0]; dstSy=&m_S0y[0];
        }

        long posCount=m_SpotCount*(1<<k)+2;
        for (long i=0;i<=posCount;++i)
        {
            dstSx[i*2]=srcSx[i+1];
            dstSy[i*2]=srcSy[i+1];
            dstSx[i*2+1]=t1*(srcSx[i+1]+srcSx[i+2])+t2*(srcSx[i]+srcSx[i+3]);
            dstSy[i*2+1]=t1*(srcSy[i+1]+srcSy[i+2])+t2*(srcSy[i]+srcSy[i+3]);
        }
    }
}


void CSceneSpotFormat::DoDraw(Context32* dst)
{
    dst->darkle(10);

    NextPos();

    const double     scrTop   =-3;
    const double     scrButtom= 3;
    const double     scrLeft  =-scrButtom*dst->width/dst->height;   //ÆÁÄ»Ó³Éä²ÎÊý
    const double     scrRight =-scrLeft;

    const CColorMover& ColorMover=m_ColorMoverList[0];

    const double vmapp_x=dst->width*(1.0/(scrRight-scrLeft));
    const double vmapp_y=dst->height*(1.0/(scrButtom-scrTop));

    const double* srcSx; const double* srcSy;
    if ((m_IteratCount % 2)==0)
    {
        srcSx=&m_S1x[0]; srcSy=&m_S1y[0];
    }
    else
    {
        srcSx=&m_S0x[0]; srcSy=&m_S0y[0];
    }

    long posCount=(m_SpotCount*(1<<(m_IteratCount+1)));
    for (long i=1;i<=posCount;++i)
    {
       long px=(long)(vmapp_x*(srcSx[i]-scrLeft));
       long py=(long)(vmapp_y*(srcSy[i]-scrTop));
       if ((px>=0)&&(py>=0)&&(px<dst->width)&&(py<dst->height))
       {
           double dx=(srcSx[i]-srcSx[i-1]);
           double dy=(srcSy[i]-srcSy[i-1]);
           dst->Pixels(px,py)=Color32(ColorMover.getR8(dx),ColorMover.getG8(dy),ColorMover.getB8(dx+dy));
       }
    }
}