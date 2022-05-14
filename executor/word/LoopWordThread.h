//
// Created by abc on 20-4-12.
//

#ifndef UNTITLED5_LOOPWORDTHREADPLUS_H
#define UNTITLED5_LOOPWORDTHREADPLUS_H

#include "SocketWordThread.h"
#include "loop/TimeCorrelate.h"

#define LOOP_CORRELATE_SOCKET_INIT_VALUE    1

class TimeInfo;
using ExecutorFunc = std::function<void(MsgHdr*)>;

class ExecutorNoteInfo {
public:
    ExecutorNoteInfo() = default;
    virtual ~ExecutorNoteInfo() = default;

    virtual uint32_t getNoteInfoSize() const = 0;
    virtual MeetingAddressNote* getNoteInfoOnPos(uint32_t) = 0;
    virtual uint32_t getTimeInfoSize() const = 0;
    virtual TimeInfo* getTimeInfoOnPos(uint32_t) = 0;
    virtual void callNoteCompleteFunc() = 0;
    virtual void callExecutorFunc(const std::function<void(MsgHdr*)>&) = 0;
    virtual void callNoteTransmitFunc(TransmitThreadUtil*, MeetingAddressNote*, MsgHdr*, uint32_t) = 0;
};

/**
 * 接收转移线程基类
 */
class ThreadTransferBasic {
public:
    ThreadTransferBasic() = default;
    virtual ~ThreadTransferBasic() = default;

    virtual void initSocketTransfer(BasicThread*) = 0;
    virtual bool onSocketTransfer(BasicThread*) = 0;
    virtual bool isCompleteTransfer() const = 0;
};

/**
 * 接收被转移note的线程信息类（单次->从开始转移到转移完成）
 * @tparam TransferSize     接收线程的数量
 */
class ThreadTransferInfo final : public ThreadTransferBasic{
public:
    explicit ThreadTransferInfo(uint32_t size) : ThreadTransferBasic(), need_transfer(0), transfer_size(size) {}
    ~ThreadTransferInfo() override = default;

    //初始化接收转移线程（储存接收转移的线程）
    void initSocketTransfer(BasicThread *socket_Thread) override { if(socket_Thread) { transfer_thread[need_transfer++] = socket_Thread; } }
    //是否所有接收转移线程完成确定
    bool isCompleteTransfer() const override { return (need_transfer <= 0); }
    //接收转移线程全部确定被转移的note
    bool onSocketTransfer(BasicThread *thread) override {
        int transfer_pos = 0;
        SocketWordThread *socket_thread = nullptr;
        //线程为空
        if(!thread || !(socket_thread = dynamic_cast<SocketWordThread*>(dynamic_cast<WordThread*>(thread)->getWord()))){
            return false;
        }

        //获取存储线程的序号
        transfer_pos = socket_thread->getTransferPos();
        //序号不在存储范围
        if((transfer_pos < 0) || (transfer_pos >= transfer_size)){
            return false;
        }
        /*
         * 不需要处理同步关系（单线程处理）（LoopThread线程运行）
         *  单线程执行
         *  多线程也不需要处理同步,SocketThread线程不会竞争同一个变量（除need_transfer）
         */
        need_transfer--;
        //释放被存储的线程
        transfer_thread[transfer_pos] = nullptr;
        return true;
    }
private:
    uint32_t need_transfer;                         //接收转移线程的数量
    uint32_t transfer_size;                         //数组数量
    BasicThread *transfer_thread[0];                //存储接收转移线程的数组
};

class LoopWordThread final : public Word{
    friend class CorrelateBase;
    friend class MicrosecondSection;
    friend class MillisecondSection;
    friend class SecondSection;
    friend class TimeCorrelate;
public:
    /**
     * 存储在LoopThread线程内的SocketThread线程的远程端数量信息
     */
    struct SocketWordInfo{
        typedef std::map<WordThread*, SocketWordInfo>::iterator     MapIterator;

        explicit SocketWordInfo(MapIterator iterator) : heap_rank(0), heap_pos(0), socket_iterator(iterator){}
        ~SocketWordInfo() = default;

        /*
         * 更新SocketWordInfo在二叉堆内的节点
         */
        static void updateSocketWordInfo(SocketWordInfo &info, int pos) { info.heap_pos = static_cast<uint32_t>(pos); }
        static void updateSocketWordInfo(SocketWordInfo *info, int pos) { info->heap_pos = static_cast<uint32_t>(pos); }

        int heap_rank;                  //远程端数量
        uint32_t heap_pos;              //堆节点信息
        MapIterator socket_iterator;    //map信息
    } ;

public:
    explicit LoopWordThread(WordThread*, bool can_transfer) throw(std::runtime_error);
    ~LoopWordThread() override;

    ThreadType type() const override { return THREAD_TYPE_LOOP; }
    void exe() override;
    void interrupt() override;
    void onPush(std::shared_ptr<ExecutorTask>) override;
    void onExecutor(std::shared_ptr<ExecutorNote>) override;
    void onInterrupt(SInstruct) throw(finish_error) override;
    void onLoop() override;

    static void onTransmitThread(Word*, ExecutorNoteInfo*);
private:
    static ThreadTransferBasic* onCreateThreadTransferInfo(uint32_t transfer_size){
        void *buffer = malloc(sizeof(ThreadTransferInfo) + (sizeof(BasicThread*) * transfer_size));
        return new (buffer) ThreadTransferInfo(transfer_size);
    }
    static ThreadTransferBasic* onDestroyThreadTransferInfo(ThreadTransferBasic *transfer_info){
        (dynamic_cast<ThreadTransferInfo*>(transfer_info))->~ThreadTransferInfo();
        free(static_cast<void*>(transfer_info)); return nullptr;
    }

    bool initLoopWordThread();
    void closeLoopWordThread();

    void onRequestTransfer();
    void onResponseTransfer();

    void updateInsert();
    void updateSocket();
    void updateInterrupt();
    void updateTime();
    void correctionSequence();
    void onSelect();

    timeval minLoopTime() throw(task_trigger_error);
    void interruptLoop();
    void resetInterrupt();
    /*
     * 复位等待消息的描述符
     */
    void resetFdSet() { memcpy(&rset, &wset, sizeof(fd_set)); }

    void obtainSocketThread();
    void insertLoopTask(std::shared_ptr<LoopExecutorTask>);
    bool insertSocketTask(WordThread*);
    void removeSocketTask(WordThread*);

    void onJoin(std::shared_ptr<AddressJoinInstruct>);
    void onExit(std::shared_ptr<AddressExitInstruct>);
    void onObtain(BasicThread*);
    void onTransfer();
    void onTransferComplete(std::shared_ptr<TransferCompleteInstruct>);
    BasicThread* onJoinAddress(MeetingAddressNote*);
    void onExceptionSocket(std::shared_ptr<ExceptionSocketInstruct>);
    void onReleaseSocket(std::shared_ptr<ReleaseSocketInstruct>);
    bool onReleaseSocket0(std::shared_ptr<Instruct>, bool);
    void onRecoverySocket(std::shared_ptr<ReleaseSocketInstruct>);
    int  onRecoverySocket0();
    void onStart(std::shared_ptr<StartInstruct>);
    void onStop(std::shared_ptr<StopInstruct>);
    void onFinish(std::shared_ptr<FinishInstruct>) throw(finish_error);
    std::future<void> onStopSocket(int, const std::function<BasicThread*(BasicThread*)>&);

    static std::chrono::steady_clock::time_point select_time_point;         //1：唤醒阻塞的时间点,2:更新序号的时间点
    //更新时间点
    static void onUpdateTimePoint() { select_time_point = std::chrono::steady_clock::now(); }

    bool is_transfer_thread;                                                        //是否正在转移远程端
    int select_size;                                                                //唤醒阻塞LoopThread线程的描述符数量
    int task_interrupt[2];                                                          //管道描述符,处理中断（0：其他线程中断调用,1：LoopThread线程接收中断调用）
    int interrupt_size;                                                             //忽略的中断数量（在LoopTread线程处理中断后和复位中断前的时间内其他线程调用中断的数量）
    fd_set wset, rset;                                                              //1：阻塞等待的描述符,2：唤醒阻塞的描述符
    TaskInfo *sequence_time_task;                                                   //序号更新任务
    ThreadTransferBasic *socket_transfer_info;                                      //接收转移的线程信息

    std::map<WordThread*, SocketWordInfo> socket_time_tasks;                        //SocketThread线程的任务
    UnLockQueue<std::shared_ptr<LoopExecutorTask>> insert_task_queue;               //添加任务队列
    BinaryHeap<SocketWordInfo*> socket_join_heap;                                   //根据远程端的数量排序的SocketThread线程的二叉堆
    TimeCorrelate time_correlate;                                                   //时间关联器
};

/*
 * 比较两个SocketWordInfo的远程端数量
 */
inline bool operator<(const LoopWordThread::SocketWordInfo &w1, const LoopWordThread::SocketWordInfo &w2) {
    return (w1.heap_rank < w2.heap_rank);
}
inline bool operator>(const LoopWordThread::SocketWordInfo &w1, const LoopWordThread::SocketWordInfo &w2) {
    return (w1.heap_rank > w2.heap_rank);
}
inline bool operator==(const LoopWordThread::SocketWordInfo &w1, const LoopWordThread::SocketWordInfo &w2) {
    return (w1.heap_rank == w2.heap_rank);
}
inline bool operator!=(const LoopWordThread::SocketWordInfo &w1, const LoopWordThread::SocketWordInfo &w2) {
    return !(w1 == w2);
}

#endif //UNTITLED5_LOOPWORDTHREADPLUS_H
