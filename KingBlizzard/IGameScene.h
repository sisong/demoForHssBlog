//IGameScene.h
#ifndef __IGame_Scene_H_
#define __IGame_Scene_H_

#include "HColor.h"

class IGameScene
{
public:
    virtual ~IGameScene() {}

    virtual void Scene_Inti(long ScreenWidth,long ScreenHeight)=0; //初始化场景
    virtual void Scene_Clear()=0;  //清空场景
    virtual void Scene_Update(unsigned long StepTime_ms)=0; //更新状态
    virtual void Scene_Draw(Context32* dst)=0; //绘制自己
};



#endif //__IGame_Scene_H_