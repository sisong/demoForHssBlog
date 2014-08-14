//PointMover.h

#ifndef __PointMover_H_
#define __PointMover_H_

#include "stdlib.h"

class CPointMover //点移动类型
{
	double	m_Value;		 //当前值
	double	m_V;             //移动速度
	double	m_MinValue;      //下界
	double	m_MaxValue;      //上界
    double  m_MinV;          //最小速度
    double  m_MaxV;          //最大速度
    inline  void  GetRandomValue() { m_Value=rand()*(1.0/RAND_MAX)*(m_MaxValue-m_MinValue)+m_MinValue; }
    inline  void  GetRandomV() { 
        int old_sign=-1; if (m_V<0) old_sign=1;
        m_V=old_sign*( rand()*(1.0/RAND_MAX)*(m_MaxV-m_MinV)+m_MinV); 
    }
public:
    inline CPointMover(){}
    inline ~CPointMover(){}
    void	Inti(double MinValue,double MaxValue,double MinV,double MaxV) { 
        m_MinValue=MinValue; m_MaxValue=MaxValue ; m_MinV=MinV; m_MaxV=MaxV;
        m_V=0;
        GetRandomValue();  GetRandomV();
    }
    inline  double GetValue() const { return m_Value; } 
    inline  double GetV() const { return m_V; } 
    inline  void  Update(unsigned long StepTime_ms) {  	
        m_Value+=m_V*StepTime_ms;
        if (m_Value<m_MinValue)
	    {
		    m_Value=m_MinValue;
		    GetRandomV();
	    }
	    else if (m_Value>m_MaxValue)
	    {
		    m_Value=m_MaxValue;
		    GetRandomV();
	    }
    }
};

#endif //__PointMover_H_
