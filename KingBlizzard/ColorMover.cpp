//ColorMover.cpp
#include "ColorMover.h"
#include "stdlib.h"

static unsigned char _colorRound_table[(csMaxColorRoundValue+1)*2];
const unsigned char* colorRound_table=&_colorRound_table[(csMaxColorRoundValue+1)];
class _CAuto_inti_colorRound_table
{
private:
    static inline long round_color(long x)
    {
        if (x<0) x=-x;
        while (x>csMaxColorRoundValue) x-=csMaxColorRoundValue;
        const double PI=3.1415926535897932384626433832795;
        double rd=( sin(x*(2.0*PI/csMaxColorRoundValue))+1+0.1)/(2.0+0.1);
        long ri=(long)(rd*255+0.5);
        //long ri=abs(x-csMaxColorRoundValue/2);
        if (ri<0) return 0;
        else if (ri>255) return 255;
        else return ri;
    }
public:
    _CAuto_inti_colorRound_table() {
        for (int i=0;i<(csMaxColorRoundValue+1)*2;++i)
            _colorRound_table[i]=(unsigned char)round_color(i-(csMaxColorRoundValue+1));
    }
};
static _CAuto_inti_colorRound_table _Auto_inti_colorRound_table;



void CColorMover::Inti(double kMin,double kMax,double ColorVMin,double ColorVMax)
{
    m_kR=rand()*(1.0/RAND_MAX)*(kMax-kMin)+kMin; 
    m_kG=rand()*(1.0/RAND_MAX)*(kMax-kMin)+kMin; 
    m_kB=rand()*(1.0/RAND_MAX)*(kMax-kMin)+kMin;  

    m_VR=rand()*(1.0/RAND_MAX)*(ColorVMax-ColorVMin)+ColorVMin;
    m_VG=rand()*(1.0/RAND_MAX)*(ColorVMax-ColorVMin)+ColorVMin;
    m_VB=rand()*(1.0/RAND_MAX)*(ColorVMax-ColorVMin)+ColorVMin;
    if (rand()>RAND_MAX/2) m_VR*=(-1);
    if (rand()>RAND_MAX/2) m_VG*=(-1);
    if (rand()>RAND_MAX/2) m_VB*=(-1);

    m_R0=rand()*(1.0/RAND_MAX)*csMaxColorRoundValue;
    m_G0=rand()*(1.0/RAND_MAX)*csMaxColorRoundValue;
    m_B0=rand()*(1.0/RAND_MAX)*csMaxColorRoundValue;
}

void CColorMover::Update(unsigned long StepTime_ms)
{
    m_R0+=m_VR*StepTime_ms;
    m_G0+=m_VG*StepTime_ms;
    m_B0+=m_VB*StepTime_ms;
    if (m_R0<0) m_R0+=csMaxColorRoundValue; else if (m_R0>=csMaxColorRoundValue) m_R0-=csMaxColorRoundValue;
    if (m_G0<0) m_G0+=csMaxColorRoundValue; else if (m_G0>=csMaxColorRoundValue) m_G0-=csMaxColorRoundValue;
    if (m_B0<0) m_B0+=csMaxColorRoundValue; else if (m_B0>=csMaxColorRoundValue) m_B0-=csMaxColorRoundValue;
}