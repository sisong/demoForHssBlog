//SceneNewton.h  牛顿法解方程的迭代混沌
#ifndef __SceneNewton_H_
#define __SceneNewton_H_

#include "IGameScene.h"
#include "HColor.h"
#include "ColorMover.h"
#include <vector>
#include "Surface.h"


class CSceneNewton:public IGameScene
{
protected:
    CColorMover     m_ColorMover;
    double          m_ColorK1;
    double          m_ColorK2;
    double          m_ColorK3;
    long            m_UpdateTime;
    bool            m_IsExtract3Ex;
    long            m_ExtractNumber;
    bool            m_isTanRev;
    long            m_iteratInc;
    CSurface        m_BufBck;
    bool            m_BufBckIsOk;

    must_inline Color32 getColor(const double dL1,const double dL2,const double dL3);
    void DoDraw(Context32* out_dst);
    void Inti();
public:
    void Scene_Inti(long ScreenWidth,long ScreenHeight) { Scene_Clear(); Inti(); }
    void Scene_Clear();
    void Scene_Update(unsigned long StepTime_ms);
    void Scene_Draw(Context32* dst) { DoDraw(dst); }
    ~CSceneNewton(){}
};



#endif //__SceneNewton_H_