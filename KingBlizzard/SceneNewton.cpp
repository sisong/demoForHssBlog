#include "SceneNewton.h"
#include "MMTime.h"
#include "math.h"

void CSceneNewton::Inti()
{
    m_UpdateTime=0;
    Scene_Update(0);
    m_BufBckIsOk=false;
}
    
void CSceneNewton::Scene_Clear()
{
}

must_inline double sqr(const double x) { return x*x; }
    
    inline double intpow(double x,int N)
    {
        switch (N)
        {
        case 0: { return 1; }break;
        case 1: { return x; }break;
        case 2: { return x*x; }break;
        case 3: { return x*x*x;  }break;
        case 4: { return sqr(x*x);   }break;
        case 5: { return sqr(x*x)*x; }break;
        case 6: { return sqr(x*x*x); }break;
        default: 
            {
                long half=N>>1;
                double xh=sqr(intpow(x,half));
                if ((N&1)==0)
                    return xh;
                else
                    return xh*x;
            }
        }
    }

void CSceneNewton::Scene_Update(unsigned long StepTime_ms)
{
    const int ExtractNumberCountListCount=11;
    static const int ExtractNumberCountList[ExtractNumberCountListCount]={3,3,3,3,4,4,5,5,6,7,8};
    m_ExtractNumber=ExtractNumberCountList[rand()%ExtractNumberCountListCount];
    m_IsExtract3Ex= (m_ExtractNumber==3) && (rand()>(RAND_MAX/2));

    m_ColorK1=0; m_ColorK2=0; m_ColorK3=0; 
    while ((m_ColorK1+m_ColorK2+m_ColorK3)<0.8)
    {
        m_ColorK1=(rand()*(1.0/RAND_MAX)*rand()*(1.0/RAND_MAX)*rand()*(1.0/RAND_MAX));
        m_ColorK2=(rand()*(1.0/RAND_MAX));
        m_ColorK3=(rand()*(1.0/RAND_MAX));
    }
    if (rand()<(RAND_MAX/2)) m_ColorK1*=-1;
    if (rand()<(RAND_MAX/2)) m_ColorK2*=-1;
    if (rand()<(RAND_MAX/2)) m_ColorK3*=-1;
  
    double r=1.0/(1<<(m_ExtractNumber-3));
    r=pow(r,0.095);
    m_ColorK1*=r;
    m_ColorK2*=r;
    m_ColorK3*=r;
    m_ColorMover.Inti(50*r,90*r,200000.0/1000,400000.0/1000);

    m_isTanRev=(rand()>(RAND_MAX/4));
    if (m_isTanRev)
    {
        if (m_ExtractNumber==3)
            m_iteratInc=1+(rand()%6);
        else
            m_iteratInc=1+(rand()%4);
    }
    else
      m_iteratInc=1+(rand()%3);
}


    must_inline void getNextPos_3Ex(const double x0,const double y0,double& out_x,double& out_y)
    {
        double x2=x0*x0; double y2=y0*y0; 
        double r = (1.0/6)/sqr(x2+y2+1e-300);
        double a=x2 - y2;
        double b=x0*y0*2;
        out_x = -y0 + (a-b )*r;
        out_y =  x0 - (a+b )*r;
    }
   
const double eValue=0.01;
must_inline double mLog(double x)
{
    if (x<eValue)
    {
        x=pow(x*(1.0/eValue),0.3)*eValue; 
    }
    return log(x);
}

inline void getExtractByNewton_3Ex(double x0,double y0,long iteratInc,double& dL1,double& dL2,double& dL3)
{    
    x0*=0.75; y0*=0.75;
    //Z^3-1=0
    double x1=x0,y1=y0;
    for (long i=0;i<iteratInc;++i)
    {
        x0=x1; y0=y1;
        getNextPos_3Ex(x0,y0,x1,y1);
    }

    dL1=mLog(abs(x1-x0)*abs(y1-y0))*0.6;
    dL2=mLog(sqr(x1-x0)+sqr(y1-y0))*0.6;

    getNextPos_3Ex(x1,y1,x0,y0);
    dL3=mLog(abs(x1-x0)+abs(y1-y0))*2.0;
}


    must_inline void sqr(const double x,const double y,double& out_x,double& out_y) 
    {
        out_x=x*x-y*y;
        out_y=2*x*y;
    }
    must_inline void mul(const double x0,const double y0,const double x1,const double y1,double& out_x,double& out_y) 
    {
        out_x=x0*x1-y0*y1;
        out_y=x0*y1+x1*y0;
    }

    inline void pow(const double x,const double y,long N,double& out_x,double& out_y)
    {
        switch (N)
        {
        case 0: { out_x=1; out_y=0; }break;
        case 1: { out_x=x; out_y=y; }break;
        case 2: { sqr(x,y,out_x,out_y); }break;
        case 3: { double x1,y1;  sqr(x,y,x1,y1);  mul(x,y,x1,y1,out_x,out_y);  }break;
        case 4: { double x1,y1;  sqr(x,y,x1,y1);  sqr(x1,y1,out_x,out_y);   }break;
        case 5: { double x1,y1,x2,y2;  sqr(x,y,x1,y1);  sqr(x1,y1,x2,y2);   mul(x,y,x2,y2,out_x,out_y); }break;
        case 6: { double x1,y1,x2,y2;  sqr(x,y,x1,y1);  sqr(x1,y1,x2,y2);   mul(x1,y1,x2,y2,out_x,out_y); }break;
        default: 
            {
                long half=N>>1;
                double xh,yh;
                pow(x,y,half,xh,yh);
                if ((N&1)==0)
                    sqr(xh,yh,out_x,out_y);
                else
                {
                    double xsqr,ysqr;
                    sqr(xh,yh,xsqr,ysqr);
                    mul(x,y,xsqr,ysqr,out_x,out_y);
                }
            }
        }
    }
    must_inline void div(const double x0,const double y0,const double x1,const double y1,double& out_x,double& out_y) 
    {
        double r= 1/(x1*x1 + y1*y1+1e-300);
        out_x = ( x0*x1 + y0*y1)*r;
        out_y = ( y0*x1 - x0*y1)*r;
    }



    const double PI=3.1415926535897932384626433832795;
    inline  double asin2(double x,double y,double r)
    {
        double seta=asin(y/r);
        if (x>=0)
            return seta;
        else if (y>=0)
            return PI-seta;
        else
            return -PI-seta;
    }
    must_inline void getNextPos(const double x0,const double y0,long N,bool isTanRev,double& out_x,double& out_y)
    {
        //Z^N-1=0
        double seta;
        if (isTanRev) seta = atan2(x0, y0); else seta = atan2(y0, x0);
        double r = sqrt(x0*x0+y0*y0);
        r=r*(N-1)/N;
        double sl=1.0/(N*intpow(r,(N-1)));
        out_x = (r * cos(seta) +sl* cos((1 - N) * seta));
        out_y = (r * sin(seta) +sl* sin((1 - N) * seta));

        /*
        //实际牛顿迭代方程
        double xndel,yndel;
        pow(x0,y0,N-1,xndel,yndel);
        double xn,yn;
        mul(x0,y0,xndel,yndel,xn,yn);
        double x,y;
        div(xn-1,yn,N*xndel,N*yndel,x,y);
        out_x=x0-x;
        out_y=y0-y;
        */
    }
void getExtractByNewton(double x0,double y0,long N,long iteratInc,bool isTanRev,double& dL1,double& dL2,double& dL3)
{    
    //Z^N-1=0
    double x1=x0,y1=y0;
    for (long i=0;i<iteratInc;++i)
    {
        x0=x1; y0=y1;
        getNextPos(x0,y0,N,isTanRev,x1,y1);
    }

    dL1=mLog(abs(x1-x0)*abs(y1-y0))*0.6;
    dL2=mLog(sqr(x1-x0)+sqr(y1-y0))*0.6;

    getNextPos(x1,y1,N,isTanRev,x0,y0);
    dL3=mLog(abs(x1-x0)+abs(y1-y0))*2.0;
}

Color32 CSceneNewton::getColor(const double dL1,const double dL2,const double dL3)
{
    double kR= dL1*m_ColorK1+dL2*m_ColorK2-dL3*m_ColorK3;
    double kG= dL1*m_ColorK1-dL2*m_ColorK2+dL3*m_ColorK3;
    double kB=-dL1*m_ColorK1+dL2*m_ColorK2+dL3*m_ColorK3;
    return Color32(m_ColorMover.getR8(kR),m_ColorMover.getG8(kG),m_ColorMover.getB8(kB));
}

void CSceneNewton::DoDraw(Context32* out_dst)
{
    if (!m_BufBckIsOk)
    {
        Context32 dst;
        m_BufBck.Inti(out_dst->width,out_dst->height);
        m_BufBck.lock_Data(dst);

        const double rTop    =-1.37;
        const double rBottom =-rTop+0.000001;
        const double rLeft   =-(rBottom*dst.width)/dst.height;
        const double rRight  =-rLeft+0.000002;

        const double xJScale= ((rRight-rLeft)/dst.width);
        const double yJScale= ((rBottom-rTop)/dst.height);
        
        Color32* dstLine=dst.pdata;
        for (long y=0;y<dst.height;++y)
        {
            double ry0=y*yJScale+rTop;
            for (long x=0;x<dst.width;++x)
            {
                double rx0=x*xJScale+rLeft;

                double dL1,dL2,dL3;
                if (m_IsExtract3Ex)
                  getExtractByNewton_3Ex(rx0,ry0,m_iteratInc,dL1,dL2,dL3);
                else
                  getExtractByNewton(rx0,ry0,m_ExtractNumber,m_iteratInc,m_isTanRev,dL1,dL2,dL3);
                dstLine[x]=getColor(dL1,dL2,dL3);
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