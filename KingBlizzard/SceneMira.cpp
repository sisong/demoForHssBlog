#include "SceneMira.h"
#include "math.h"

void CSceneMira::IntiMover()
{
    m_MoverList.resize(4);
    const double MinV=0.3 /10000;
    const double MaxV=1.5 /10000;
    m_MoverList[0].Inti(-0.5, 1,MinV*0.5,MaxV*0.5);
    m_MoverList[1].Inti( 0.5, 1,MinV*1,MaxV*1);
    m_MoverList[2].Inti( 1.5, 3,MinV*3,MaxV*3);

    m_ColorMoverList.resize(1);
    m_ColorMoverList[0].Inti(3,5,10.0/1000,60.0/1000);
}

void CSceneMira::NextPos(const double x0,const double y0,double& new_x,double& new_y)
{
    const double a=m_MoverList[0].GetValue();
    const double b=m_MoverList[1].GetValue();
    const double c=m_MoverList[2].GetValue();
    double& x1=new_x;
    new_x = b * y0 + (a * x0 + (1 - a) * c / (1 + 1/(x0*x0) ));
    new_y = -x0 + (a * x1 + (1 - a) * c / (1 + 1/(x1*x1) ));
}


void CSceneMira::DoDraw(Context32* dst)
{

    //dst->fill(Color32(0,0,0));
    dst->darkle(1<<4);

    const int     sCount=200; //初始点数目
    const int     sIteratCount=m_DrawCount/(sCount);// 每个初始点迭代的次数

    const double     scrLeft  =-14;   //屏幕映射参数
    const double     scrRight = 16;
    const double     scrTop   =-17;
    const double     scrButtom= 13;

    const CColorMover& ColorMover=m_ColorMoverList[0];

    const double vmapp_x=dst->width*(1.0/(scrRight-scrLeft));
    const double vmapp_y=dst->height*(1.0/(scrButtom-scrTop));
    for (int i=0;i<(sCount);i++)
    {
        double sx0,sy0;
        sx0=rand()*(1.0/RAND_MAX )*(scrRight-scrLeft)+scrLeft;
        sy0=rand()*(1.0/RAND_MAX )*(scrButtom-scrTop)+scrTop;
        
        const long csSkipCount=23;
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
            if (fabs(sxn)+fabs(syn)>1e8) break;

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