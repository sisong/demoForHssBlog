#include "SceneKing.h"
#include "math.h"

void CSceneKing::IntiMover()
{
    m_MoverList.resize(8);
    const double MinV=10.0 /100000;
    const double MaxV=20.0 /100000;
    m_MoverList[0].Inti(-1.5,-0.5,MinV,MaxV);
    m_MoverList[1].Inti(-1.5,-0.5,MinV,MaxV);
    m_MoverList[2].Inti(-3.5,   4,MinV,MaxV);
    m_MoverList[3].Inti(-3.5,   4,MinV,MaxV);
    m_MoverList[4].Inti(  -1,   1,MinV,MaxV);
    m_MoverList[5].Inti(  -1,   1,MinV,MaxV);
    m_MoverList[6].Inti( 0.8, 1.2,MinV/5,MaxV/5);
    m_MoverList[7].Inti( 0.8, 1.2,MinV/5,MaxV/5);

    m_ColorMoverList.resize(1);
    m_ColorMoverList[0].Inti(20,50,10.0/1000,80.0/1000);
}

void CSceneKing::NextPos(const double x0,const double y0,double& new_x,double& new_y)
{
    const double a =m_MoverList[0].GetValue();
    const double al=m_MoverList[1].GetValue();
    const double b =m_MoverList[2].GetValue();
    const double bl=m_MoverList[3].GetValue();
    const double c =m_MoverList[4].GetValue();
    const double d =m_MoverList[5].GetValue();
    const double e =m_MoverList[6].GetValue();
    const double f =m_MoverList[7].GetValue();
    new_x=c*sin(b*x0)+e*sin(bl*y0);
    new_y=f*sin(a*x0)+d*sin(al*y0);
/*
使用的方程:
x_(n+1)=c*sin(b*x_n)+sin(b*y_n),
y_(n+1)=sin(a*x_n)+d*sin(a*y_n),

x:=0.1;y:=0.1;
a:=-0.9666918; {-0.97}
b:=2.679879; {2.8}
c:=0.565145; {0.45,0.76}
d:=0.744728; {0.71}
{-1.86＜x＜1.86}; {-1.51＜y＜1.51};

//================  
x_(n+1)=sin(a*y_n)-z*cos(b*x_n),
y_(n+1)=z*sin(c*x_n)-cos(d*y_n),
z_(n+1)=e*sin(x_n).

a=2.24,b=0.43,c=-0.65,d=-2.43，0.5<e<1.0

*/
}


void CSceneKing::DoDraw(Context32* dst)
{

    //dst->fill(Color32(0,0,0));
    dst->darkle(1<<6);

    const int     sCountCr=300;   //圆上 初始点数目 
    const int     sCountLine=300; //直线上 初始点数目
    const int     sIteratCount=m_DrawCount/(sCountCr+sCountLine);// 每个初始点迭代的次数

    const double     scrLeft  =-2.2;   //屏幕映射参数
    const double     scrRight = 2.2;
    const double     scrTop   =-2.2;
    const double     scrButtom= 2.2;

    const CColorMover& ColorMover=m_ColorMoverList[0];

    const double vmapp_x=dst->width*(1.0/(scrRight-scrLeft));
    const double vmapp_y=dst->height*(1.0/(scrButtom-scrTop));
    for (int i=0;i<(sCountCr+sCountLine);i++)
    {
        double sx0,sy0;
        /*if (i<sCountCr)
        {
            const double  PI=3.1415926535897932384626433832795;
            double seta=i*(PI*2/sCountCr);
            sx0=sin(seta);
            sy0=cos(seta);
        }
        else
        {
            sx0=(i-sCountCr)*(2.0/sCountLine)+0.00-1;
            sy0=(i-sCountCr)*(2.0/sCountLine)+0.05-1;
        }*/
        sx0=rand()*(1.0/RAND_MAX )*(scrRight-scrLeft)+scrLeft;
        sy0=rand()*(1.0/RAND_MAX )*(scrButtom-scrTop)+scrTop;
        
        const long csSkipCount=15;
        for (int skip=0;skip<csSkipCount;skip++)
        {
            double sxn;
            double syn;
            NextPos(sx0,sy0,sxn,syn);
            sx0=sxn;
            sy0=syn;
        }
        for (int j=0;j<sIteratCount;j++)
        {
            double sxn;
            double syn;
            NextPos(sx0,sy0,sxn,syn);

            long px=(long)(vmapp_x*(sxn-scrLeft));
            long py=(long)(vmapp_y*(syn-scrTop));

            if ((px>=0)&&(py>=0)&&(px<dst->width)&&(py<dst->height))
            {
                double dx=(sxn-sx0);
                double dy=(syn-sy0);
                dst->Pixels(px,py)=Color32(ColorMover.getR8(dx),ColorMover.getG8(dy),ColorMover.getB8(dx+dy));
            }
            sx0=sxn;
            sy0=syn;
        }
    }


}