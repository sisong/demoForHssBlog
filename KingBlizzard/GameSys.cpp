//GameSys.h

#include "GameSys.h"
#include "MMTime.h"

CGameSys::CGameSys()
:m_DDrawImprot(0),m_SceneRuningIndex(-1) 
{}

CGameSys::~CGameSys() 
{ 
    Game_Shutdown();
}

void CGameSys::AddGameScene(IGameScene* GameScene)
{
    if (GameScene!=0)
        m_SceneList.push_back(GameScene);
}

bool CGameSys::Game_Init(long ScreenWidth,long ScreenHeight,long ScreenColorBit,void * main_window_handle,bool isRunAsDebug)
{
	srand(mmGetTime());
    Game_Shutdown();
    m_ScreenWidth=ScreenWidth;
    m_ScreenHeight=ScreenHeight;

    m_DDrawImprot=new CDDrawImprot();
    if (!m_DDrawImprot->IntiScreen(ScreenWidth,ScreenHeight,ScreenColorBit,main_window_handle))
    {
        Game_Shutdown();
        return false;
    }
    else
    {
        m_DDrawImprot->IsShowFPS(isRunAsDebug);
        return true;
    }

}

void CGameSys::Game_Shutdown()
{
    if (m_DDrawImprot!=0)
    {
        delete m_DDrawImprot;
        m_DDrawImprot=0;
    }
    for (unsigned long i=0;i<m_SceneList.size();++i)
    {
        if (m_SceneList[i]!=0)
            delete m_SceneList[i];
        m_SceneList[i]=0;
    }
    m_SceneList.clear();
} 

void CGameSys::Game_Loop()
{
    long SceneCount=m_SceneList.size();
    if (SceneCount<=0) return;
    
    const long Min_SceneRunSumTime= 90*1000; 
    const long MAX_SceneRunSumTime=150*1000;  
    bool is_runnew=false;
    if ( (m_SceneRuningIndex<0)||(m_SceneRunSumTime<=0) )
    {
        if ((m_SceneRuningIndex>=0)&&(SceneCount!=1))
            m_SceneList[m_SceneRuningIndex]->Scene_Clear();
        void* old_SceneRuning=0;
        if (m_SceneRuningIndex>=0) old_SceneRuning=m_SceneList[m_SceneRuningIndex];
        if (SceneCount>1)
        {   //随机下一个不同场景
            while (true)
            {
                ++m_SceneRuningIndex;
                if (m_SceneRuningIndex>=SceneCount)
                    m_SceneRuningIndex-=SceneCount;
                long swapIndex=m_SceneRuningIndex+rand()%(SceneCount-m_SceneRuningIndex);
                std::swap(m_SceneList[m_SceneRuningIndex],m_SceneList[swapIndex]);
                if (old_SceneRuning!=m_SceneList[m_SceneRuningIndex]) break;           
            }
        }
        else
            m_SceneRuningIndex=0;

        is_runnew=old_SceneRuning!=m_SceneList[m_SceneRuningIndex];
    }

    if (is_runnew)
    {
        m_UpdateTime=mmGetTime();
        m_SceneRunSumTime=(unsigned long)((rand()*(1.0/RAND_MAX) * (MAX_SceneRunSumTime-Min_SceneRunSumTime))+Min_SceneRunSumTime);
        m_SceneList[m_SceneRuningIndex]->Scene_Inti(m_ScreenWidth,m_ScreenHeight);
    }
    else //if (!is_runnew)
    {
        unsigned long nowTime=mmGetTime();
        unsigned long stepTime=nowTime-m_UpdateTime;
        if (stepTime<=0) return;
        m_SceneRunSumTime-=stepTime;
        m_UpdateTime=nowTime;
        m_SceneList[m_SceneRuningIndex]->Scene_Update(stepTime);
    }

    Context32 dst;
    if (m_DDrawImprot->DrawBegin(dst))
    {
        m_SceneList[m_SceneRuningIndex]->Scene_Draw(&dst);
        m_DDrawImprot->DrawEnd(dst);
    }
    else
        mmSleep(10);
}
