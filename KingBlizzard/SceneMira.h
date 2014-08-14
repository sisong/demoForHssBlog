//SceneMira.h  Mira”≥…‰
#ifndef __SceneMira_H_
#define __SceneMira_H_

#include "ScenePointFractal.h"

class CSceneMira:public CScenePointFractal
{
protected:
    long    m_DrawCount;
    //std::vector<CStar> m_StarList;
    void NextPos(const double x0,const double y0,double& new_x,double& new_y);
protected:
    void Scene_Inti(long ScreenWidth,long ScreenHeight) { CScenePointFractal::Scene_Inti(ScreenWidth,ScreenHeight); m_DrawCount=50000; }
    void IntiMover();
    void DoDraw(Context32* dst);
public:
};

#endif //__SceneMira_H_