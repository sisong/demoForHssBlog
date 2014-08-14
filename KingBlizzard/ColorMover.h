//ColorMover.h
#ifndef __ColorMover_H_
#define __ColorMover_H_

#include "HColor.h"
#include "math.h"

enum { csMaxColorRoundValue=512};

//颜色查表
extern const unsigned char* colorRound_table;

class CColorMover
{
private:
    double m_kR;  //颜色变化强度
    double m_kG;
    double m_kB;
    double m_R0;  //初始颜色
    double m_G0;
    double m_B0;
    double m_VR;  //颜色变化速度
    double m_VG;
    double m_VB;
    static must_inline unsigned long getColor(const double Color0,const double k,const double Gene)
    {
        long rd=(long)(Color0+k*Gene);
        rd=rd%csMaxColorRoundValue;
        return colorRound_table[rd];
    }
public:
    void Inti(double kMin,double kMax,double ColorVMin,double ColorVMax);
    void Update(unsigned long StepTime_ms);

    must_inline unsigned long getR8(const double GeneR) const { return getColor(m_R0,m_kR,GeneR); }
    must_inline unsigned long getG8(const double GeneG) const { return getColor(m_G0,m_kG,GeneG); }
    must_inline unsigned long getB8(const double GeneB) const { return getColor(m_B0,m_kB,GeneB); }
    must_inline unsigned long getColor32() const { return Color32(getR8(0),getG8(0),getB8(0)).xrgb; }
};



#endif //__ColorMover_H_