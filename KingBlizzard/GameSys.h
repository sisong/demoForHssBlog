//GameSys.h
#ifndef __Game_Sys_H_
#define __Game_Sys_H_

#include "DDrawImport.h"
#include "IGameScene.h"
#include <vector>

class CGameSys
{
    CDDrawImprot*               m_DDrawImprot;
    std::vector<IGameScene*>    m_SceneList;
    unsigned long               m_UpdateTime;
    long                        m_SceneRunSumTime;
    int                         m_SceneRuningIndex;
    long                        m_ScreenWidth;
    long                        m_ScreenHeight;
public:
    CGameSys();
    ~CGameSys() ;
    void AddGameScene(IGameScene* GameScene);
    bool Game_Init(long ScreenWidth,long ScreenHeight,long ScreenColorBit,void * main_window_handle,bool isRunAsDebug);
    void Game_Shutdown();
    void Game_Loop();
};

#endif //__Game_Sys_H_