//
// Created by abc on 20-5-2.
//

#ifndef UNTITLED5_MANAGERTHREAD_H
#define UNTITLED5_MANAGERTHREAD_H

#include "ExecutorThread.h"

#define DEFAULT_EXECUTOR_THREAD_SIZE    15 //因为二叉堆的0位置是不储存数据,16会申请32个存储空间,而需要使用是17个存储空间(浪费15个存储空间),15的二叉堆是一颗完整树,不浪费存储空间

using thread_pos = unsigned int;

using thread_type = std::pair<BasicThread*, thread_pos>;
using correlate_type = std::map<std::thread::id, thread_type>::iterator;
using thread_rank = std::pair<correlate_type, int>;

using ThreadQueueUtil = UnLockQueue<std::shared_ptr<ExecutorNote>>::UnLockQueueUtil;

/**
 * 线程管理接口
 */
class ManagerInterface {
public:
    ManagerInterface() = default;
    virtual ~ManagerInterface() = default;


    //-------------ThreadCorrelate接口函数---------//
    virtual uint32_t threadSize() = 0;
    virtual uint32_t threadNoteSize(int) = 0;
    virtual thread_type stealThread() = 0;
    virtual thread_type executorThread() = 0;
    virtual thread_type executorThread(const std::thread::id&) = 0;
    virtual void onThreadReceiveNote(thread_type&) = 0;
    virtual bool onThreadReceiveTask(thread_type&, int) = 0;

    //-------------ThreadDispatcher----------------//
    virtual void onThreadEmpty(BasicThread*) = 0;
    virtual void onThreadUnEmpty(BasicThread*) = 0;

    //-------------ThreadCorrelate---------------//
    //-------------ManagerThread---------------//
    /*
     * 回收ExecutorThread线程（从ManagerThread内删除）
     */
    virtual void onRecoveryThread(BasicThread*) = 0;

    //-------------ManagerThread---------------//
    /*
     * 请求窃取note
     */
    virtual bool onRequestStealTask(BasicThread*) = 0;
    /*
     * 所有的ExecutorThread线程无法增加note
     */
    virtual void onThreadLoad(std::shared_ptr<ExecutorNote>) = 0;
    /*
     * 无法增加note后处理函数
     */
    virtual void onDispenseLoad() = 0;

    //-------------ThreadDispatcher--------------//
    //-------------ThreadCorrelate---------------//
    virtual void onStopCorrelate() = 0;
    virtual void onStopDispatcher() = 0;

    //空线程
    static thread_type empty_thread;
    //空线程id
    static std::thread::id empty_thread_id;
};

/**
 * 线程关联器
 */
class ThreadCorrelate : virtual public ManagerInterface{
public:
    ThreadCorrelate();
    ~ThreadCorrelate() override = default;

    /*
     * 最小的ExecutorThread线程内note的数量
     */
    uint32_t threadSize() override { return static_cast<uint32_t>(executor_heap.get().second); }
    /*
     * 指定ExecutorThread线程内note的数量
     */
    uint32_t threadNoteSize(int pos) override { return static_cast<uint32_t>(executor_heap[pos].second); }
    /*
     * 响应窃取的ExecutorThread线程
     */
    thread_type stealThread() override { return executor_heap.extreme().first->second; }
    thread_type executorThread() override;
    thread_type executorThread(const std::thread::id&) override;

    void onThreadReceiveNote(thread_type&) override;
    bool onThreadReceiveTask(thread_type&, int) override;
    void onRecoveryThread(BasicThread*) override;
    void onStopCorrelate() override;
    thread_type correlateThread(BasicThread*);

    static void percolate(thread_rank &rthread, int pos){ rthread.first->second.second = static_cast<thread_pos>(pos); }
    static bool compare_correlate_thread(const std::pair<correlate_type, int> &compare1, const std::pair<correlate_type, int> &compare2){
        return compare1.second >= compare2.second;
    }
protected:
    void onThreadUpdate(BasicThread*);
    void onThreadUpdate(BasicThread*, int);
private:
    //所有的ExecutorThread线程
    std::map<std::thread::id, thread_type> executor_threads;
    //根据note数量排序的ExecutorThread线程堆
    BinaryHeap<thread_rank> executor_heap;

};

class ThreadDispatcher : virtual public ManagerInterface{
public:
    ThreadDispatcher() = default;
    ~ThreadDispatcher() override = default;

    void onThreadEmpty(BasicThread*) override;
    void onThreadUnEmpty(BasicThread*) override;
    void onStopDispatcher() override;

    int sendNoteToThread(std::shared_ptr<ExecutorNote>&, thread_type);
    //分配一个任务
    void dispenseNoteToExecutorThread(std::shared_ptr<ExecutorNote>&);
    //分配n个任务
    void dispenseNoteToExecutorThread(ThreadQueueUtil, thread_type&, const std::function<void(std::shared_ptr<ExecutorNote>&)>&);
    //为任务分配线程（多次任务）
    BasicThread* dispenseTaskToExecutorThread(std::shared_ptr<LoopExecutorTask>&);
private:
    //空闲线程队列（没有note的线程）
    std::map<std::thread::id, BasicThread*> idle_thread;
};

//  分配
//  窃取
//  转移
//  异常处理
//  中断
//  添加
//  移除

class ManagerThread : public BasicThread, private ThreadDispatcher, private ThreadCorrelate{
public:
    ManagerThread();
    ~ManagerThread() override = default; //清理instruct_queue、wait_notes、wait_tasks的内存？

    virtual void onLoop(std::promise<void>&);
    std::function<void(std::promise<void>&)> onStart() override { return std::bind(&ManagerThread::onLoop, this, std::placeholders::_1); }
    ThreadType type() const override { return THREAD_TYPE_MANAGER; }
    void receiveTask(std::shared_ptr<ExecutorNote>) override;
    void receiveTask(std::shared_ptr<ExecutorTask>) override;
    void receiveInstruct(SInstruct) override;

    void onThreadWait();
    void onThreadNotify();
    void onThreadTransfer(MeetingAddressNote*, uint32_t, uint32_t);
protected:
    void handleInterrupt() override;
    void handleLoop() override;
public:
    bool onRequestStealTask(BasicThread*) override;
    void onResponseStealTask(std::shared_ptr<StealInstruct>);
    //转移任务,分发多个固定线程
    void onTransferTask(std::shared_ptr<TransferInstruct>);

    void onThreadLoad(std::shared_ptr<ExecutorNote>) override;
    void onDispenseLoad() override;
private:
    uint32_t transmitThread();
    void transmitThread(BasicThread*, uint32_t);
    void transmitRecoveryThread(BasicThread*);
    void displayThread(BasicThread*);
    void onThreadFlush();

    void dispenseTask();
    void dispenseNote();
    void receiveNotes(ThreadQueueUtil);
    void onReceiveTask(std::shared_ptr<ExecutorTask>);

    void onRecoveryThread(SInstruct);
    void onRecoveryThread(BasicThread*) override;
    void onThreadRequest(std::shared_ptr<ThreadInstruct>);
    void onThreadStart(std::shared_ptr<StartInstruct>);
    void onThreadStartComplete(std::shared_ptr<StartInstruct>);
    void onThreadStartFail(std::shared_ptr<StartInstruct>) throw(std::logic_error);
    void onThreadObtain(BasicThread*);
    void onThreadRelease(SInstruct);
    void onThreadStop(std::shared_ptr<StopInstruct>);
    void onThreadFinish();

    static void emptyReleaseInstruct(Instruct*){}
    static FlushInstruct flush_instruct;

    //当前是否正在处理窃取请求
    bool thread_steal;
    //最大ExecutorThread线程数量
    int max_thread_size;
    //线程池
    BasicExecutor *executor_pool;

    //指令队列promise和future
    std::future<void> queue_future;
    std::promise<void> queue_promise;
    //指令队列
    UnLockQueue<std::shared_ptr<Instruct>> instruct_queue;
    //note队列
    UnLockQueue<std::shared_ptr<ExecutorNote>> wait_notes;
    //task队列
    UnLockQueue<std::shared_ptr<ExecutorTask>> wait_tasks;
};

#endif //UNTITLED5_MANAGERTHREAD_H
