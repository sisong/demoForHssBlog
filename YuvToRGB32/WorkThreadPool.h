//WorkThreadPool.h
/////////////////////////////////////////////////////////////
//工作线程池 CWorkThreadPool
//用于把一个任务拆分成多个线程任务,从而可以使用多个CPU
//HouSisong@263.net
////////////////////////////
//todo:改成任务领取模式
//todo:修改辅助线程优先级，继承自主线程
//要求：1.任务分割时分割的任务量比较接近
//      2.任务也不要太小，否则线程的开销可能会大于并行的收益
//      3.任务数最好是CPU数的倍数
//      4.主线程不能以过高优先级运行，否则其他辅助线程可能得不到时间片

#ifndef _WorkThreadPool_H_
#define _WorkThreadPool_H_

typedef void (*TThreadCallBack)(void * pData);

class CWorkThreadPool
{
public:
    static long best_work_count();  //返回最佳工作分割数,现在的实现为返回CPU个数
    static void work_execute(const TThreadCallBack work_proc,void** word_data_list,int work_count);  //并行执行工作，并等待所有工作完成    
    static void work_execute_multi(const TThreadCallBack* work_proc_list,void** word_data_list,int work_count); //同上，但不同的work调用不同的函数
    static void work_execute_single_thread(const TThreadCallBack work_proc,void** word_data_list,int work_count)  //单线程执行工作，并等待所有工作完成;用于调试等  
    {
        for (long i=0;i<work_count;++i)
        work_proc(word_data_list[i]);
    }
    static void work_execute_single_thread_multi(const TThreadCallBack* work_proc_list,void** word_data_list,int work_count)  //单线程执行工作，并等待所有工作完成;用于调试等  
    {
        for (long i=0;i<work_count;++i)
        work_proc_list[i](word_data_list[i]);
    }
};

#endif //_WorkThreadPool_H_