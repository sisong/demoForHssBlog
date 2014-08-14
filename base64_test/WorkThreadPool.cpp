/////////////////////////////////////////////////////////////
//工作线程池 TWorkThreadPool

#include <process.h>
#include <vector>
#include "windows.h"
#include "WorkThreadPool.h"

//#define _IS_SetThreadAffinity_  
//定义该标志则执行不同的线程绑定到不同的CPU，减少线程切换开销； 不鼓励


class TCriticalSection
{
private:
    RTL_CRITICAL_SECTION m_data;
public:
    TCriticalSection()  { InitializeCriticalSection(&m_data); }
    ~TCriticalSection() { DeleteCriticalSection(&m_data); }
    inline void Enter() { EnterCriticalSection(&m_data); }
    inline void Leave() { LeaveCriticalSection(&m_data); }
};

class TWorkThreadPool;

//线程状态
enum TThreadState{ thrStartup=0, thrReady,  thrBusy, thrTerminate, thrDeath };

class TWorkThread
{
public:
    volatile HANDLE             thread_handle;
    volatile enum TThreadState  state;
    volatile TThreadCallBack    func;
    volatile void *             pdata;  //work data     
     TCriticalSection*  CriticalSection;
     TCriticalSection*  CriticalSection_back;
    TWorkThreadPool*            pool;
    volatile DWORD              thread_ThreadAffinityMask;

    TWorkThread() { memset(this,0,sizeof(TWorkThread));  }
};

void do_work_end(TWorkThread* thread_data);


void __cdecl thread_dowork(TWorkThread* thread_data) //void __stdcall thread_dowork(TWorkThread* thread_data)
{
    volatile TThreadState& state=thread_data->state;
    #ifdef _IS_SetThreadAffinity_
        SetThreadAffinityMask(GetCurrentThread(),thread_data->thread_ThreadAffinityMask);
    #endif
    state = thrStartup;

    while(true)
    {
        thread_data->CriticalSection->Enter();
        thread_data->CriticalSection->Leave();
        if(state == thrTerminate)
            break;

        state = thrBusy;
        volatile TThreadCallBack& func=thread_data->func;
        if (func!=0)
            func((void *)thread_data->pdata);
        do_work_end(thread_data);
    }
    state = thrDeath;
    _endthread();
    //ExitThread(0);
}

class TWorkThreadPool
{
private:
    std::vector<TCriticalSection*>  CriticalSections;
    std::vector<TCriticalSection*>  CriticalSections_back;
    std::vector<TWorkThread>       work_threads;
    mutable long                   cpu_count;
    inline long get_cpu_count() const { 
        if (cpu_count>0) return cpu_count;

        SYSTEM_INFO SystemInfo; 
        GetSystemInfo(&SystemInfo);
        cpu_count=SystemInfo.dwNumberOfProcessors; 
        return cpu_count;
    }
    inline long passel_count() const { return (long)work_threads.size()+1; }
    void inti_threads() 
    {
        long best_count =get_cpu_count();

        long newthrcount=best_count - 1;
        work_threads.resize(newthrcount);
        CriticalSections.resize(newthrcount);
        CriticalSections_back.resize(newthrcount);
        long i;
        for( i= 0; i < newthrcount; ++i)
        {
            CriticalSections[i]=new TCriticalSection();
            CriticalSections_back[i]=new TCriticalSection();
            work_threads[i].CriticalSection=CriticalSections[i];
            work_threads[i].CriticalSection_back=CriticalSections_back[i];
            CriticalSections[i]->Enter();
            CriticalSections_back[i]->Enter();
            work_threads[i].state = thrTerminate;
            work_threads[i].pool=this;
            work_threads[i].thread_ThreadAffinityMask=1<<(i+1);
            work_threads[i].thread_handle =(HANDLE)_beginthread((void (__cdecl *)(void *))thread_dowork, 0, (void*)&work_threads[i]); 
            //CreateThread(0, 0, (LPTHREAD_START_ROUTINE)thread_dowork,(void*) &work_threads[i], 0, &thr_id);
            //todo: _beginthread 的错误处理
        }
        #ifdef _IS_SetThreadAffinity_
            SetThreadAffinityMask(GetCurrentThread(),0x01);
        #endif
        for(i = 0; i < newthrcount; ++i)
        {
            while(true) { 
                if (work_threads[i].state == thrStartup) break;
                else Sleep(0);
            }
            work_threads[i].state = thrReady;
        }
    }
    void free_threads(void)
    {
        long thr_count=(long)work_threads.size();
        long i;
        for(i = 0; i <thr_count; ++i)
        {
            while(true) {  
                if (work_threads[i].state == thrReady) break;
                else Sleep(0);
            }
            work_threads[i].state=thrTerminate;
        }
        for (i=0;i<thr_count;++i)
        {
            CriticalSections[i]->Leave();
            CriticalSections_back[i]->Leave();
        }
        for(i = 0; i <thr_count; ++i)
        {
            while(true) {  
                if (work_threads[i].state == thrDeath) break;
                else Sleep(0);
            }
        }
        work_threads.clear();
        for (i=0;i<thr_count;++i)
        {
            delete CriticalSections[i];
            delete CriticalSections_back[i];
        }
        CriticalSections.clear();
        CriticalSections_back.clear();
    }
    void passel_work(const TThreadCallBack* work_proc,int work_proc_inc,void** word_data_list,int work_count)    {
        if (work_count==1)
            (*work_proc)(word_data_list[0]);
        else
        {
            const TThreadCallBack* pthwork_proc=work_proc;
            pthwork_proc+=work_proc_inc;
   
            long i;
            long thr_count=(long)work_threads.size();
            for(i = 0; i < work_count-1; ++i)
            {
                work_threads[i].func  = *pthwork_proc;
                work_threads[i].pdata  =word_data_list[i+1];
                work_threads[i].state = thrBusy;
                pthwork_proc+=work_proc_inc;
            }
            for(i =  work_count-1; i < thr_count; ++i)
            {
                work_threads[i].func  = 0;
                work_threads[i].pdata  =0;
                work_threads[i].state = thrBusy;
            }
            for (i=0;i<thr_count;++i)
                CriticalSections[i]->Leave();

            //current thread do a work
            (*work_proc)(word_data_list[0]);

            //wait for work finish  
            for(i = 0; i <thr_count; ++i)
            {
                while(true) {  
                    if (work_threads[i].state == thrReady) break;
                    else Sleep(0);
                }
            }
            CriticalSections.swap(CriticalSections_back);
            for (i=0;i<thr_count;++i)
                CriticalSections_back[i]->Enter();
        }
    }
    void private_work_execute(TThreadCallBack* pwork_proc,int work_proc_inc,void** word_data_list,int work_count)    {        
     while (work_count>0)
        {
            long passel_work_count;
            if (work_count>=passel_count())
                passel_work_count=passel_count();
            else
                passel_work_count=work_count;

            passel_work(pwork_proc,work_proc_inc,word_data_list,passel_work_count);

            pwork_proc+=(work_proc_inc*passel_work_count);
            word_data_list=&word_data_list[passel_work_count];
            work_count-=passel_work_count;
        }
    }
public:
   explicit TWorkThreadPool():work_threads(),cpu_count(0) {   inti_threads();    }
    ~TWorkThreadPool() {  free_threads(); }
    inline long best_work_count() const { return passel_count(); }
    inline void DoWorkEnd(TWorkThread* thread_data){ 
        thread_data->func=0;
        thread_data->state = thrReady;
        std::swap(thread_data->CriticalSection,thread_data->CriticalSection_back);
    }

    inline void work_execute_multi(TThreadCallBack* pwork_proc,void** word_data_list,int work_count)    {   
        private_work_execute(pwork_proc,1,word_data_list,work_count);
    }
    inline void work_execute(TThreadCallBack work_proc,void** word_data_list,int work_count)    {   
        private_work_execute(&work_proc,0,word_data_list,work_count);
    }
};
void do_work_end(TWorkThread* thread_data)
{
    thread_data->pool->DoWorkEnd(thread_data);
}

//TWorkThreadPool end;
////////////////////////////////////////

TWorkThreadPool g_work_thread_pool;//工作线程池

long CWorkThreadPool::best_work_count() {  return g_work_thread_pool.best_work_count();  }

void CWorkThreadPool::work_execute(const TThreadCallBack work_proc,void** word_data_list,int work_count)
{
    g_work_thread_pool.work_execute(work_proc,word_data_list,work_count);
}

void CWorkThreadPool::work_execute_multi(const TThreadCallBack* work_proc_list,void** word_data_list,int work_count)
{
    g_work_thread_pool.work_execute_multi((TThreadCallBack*)work_proc_list,word_data_list,work_count);
}