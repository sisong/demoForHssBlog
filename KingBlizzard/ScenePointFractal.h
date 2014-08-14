//ScenePointFractal.h  π˙Õı”≥…‰
#ifndef __ScenePointFractal_H_
#define __ScenePointFractal_H_

#include <vector>
#include "IGameScene.h"
#include "HColor.h"
#include "PointMover.h"
#include "ColorMover.h"

class CScenePointFractal:public IGameScene
{
protected:
    std::vector<CPointMover> m_MoverList;
    std::vector<CColorMover> m_ColorMoverList;
    virtual void IntiMover()   {}
    virtual void ClearMover()  { m_MoverList.clear(); m_ColorMoverList.clear(); }
    virtual void UpdateMover(unsigned long StepTime_ms) 
    { 
        for (unsigned long i=0;i<m_MoverList.size();++i)  m_MoverList[i].Update(StepTime_ms);
        for (unsigned long i=0;i<m_ColorMoverList.size();++i)  m_ColorMoverList[i].Update(StepTime_ms);
    }
    virtual void DoDraw(Context32* dst) {}
public:
    void Scene_Inti(long ScreenWidth,long ScreenHeight) { Scene_Clear(); IntiMover(); }
    void Scene_Clear() { ClearMover(); }
    void Scene_Update(unsigned long StepTime_ms) { UpdateMover(StepTime_ms); }
    void Scene_Draw(Context32* dst) { DoDraw(dst); }
    ~CScenePointFractal() { ClearMover(); }
};

#endif //__ScenePointFractal_H_