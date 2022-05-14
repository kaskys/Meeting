//
// Created by abc on 20-4-21.
//

#ifndef UNTITLED5_BASICTHREAD_H
#define UNTITLED5_BASICTHREAD_H

#include <map>
#include <thread>
#include <atomic>
#include <tuple>
#include <algorithm>
#include <sys/socket.h>
#include <zconf.h>
#include <sys/ioctl.h>

#include "../task/SocketWordTask.h"
#include "../task/LoopWordTask.h"
#include "../instruct/InstructUtil.h"

#include "../../util/heap/BinaryHeap.h"


/**
 * 线程状态
 */
enum ThreadStatus{
    THREAD_STATUS_INITIALIZE,   //初始化
    THREAD_STATUS_MOUNT,        //挂载
    THREAD_STATUS_EXECUTOR,     //执行
    THREAD_STATUS_WAIT,         //等待
    THREAD_STATUS_STOP,         //停止
    THREAD_STATUS_FINISH        //结束
};

struct InterruptFlag final{
    InterruptFlag() : interrupt_policy(INTERRUPT_TRANSFER), interrupt_flag(false) {}
    explicit InterruptFlag(InterruptPolicy policy) : interrupt_policy(policy), interrupt_flag(false) {}
    ~InterruptFlag() = default;

    /**
     * 取消中断转状态
     */
    void cancelInterrupt() {
        interrupt_flag.store(false, std::memory_order_release);
    }


    /**
     * 根据中断策略设置中断
     */
    void interrupt() {
        switch (interrupt_policy){
            //请求-获取一致性
            case INTERRUPT_DISCARD:
            case INTERRUPT_TRANSFER:
                interrupt_flag.store(true, std::memory_order_release);
                break;
            //自由性
            case INTERRUPT_FINISH:
                interrupt_flag.store(true, std::memory_order_relaxed);
                break;
            //内存一致性
            default:
                interrupt_flag.store(true);
                break;
        }
    };

    /**
     * 根据中断策略判断当前是否处在中断状态
     * @return
     */
    bool isInterrupt() const {
        bool is_interrupt;
        //同上
        switch (interrupt_policy) {
            case INTERRUPT_DISCARD:
            case INTERRUPT_TRANSFER:
                is_interrupt = interrupt_flag.load(std::memory_order_acquire);
                break;
            case INTERRUPT_FINISH:
                is_interrupt = interrupt_flag.load(std::memory_order_relaxed);
                break;
            default:
                is_interrupt = interrupt_flag.load();
                break;
        }
        return is_interrupt;
    };
    /**
     * 设置策略
     * @param policy    策略
     */
    void setPolicy(InterruptPolicy policy){ interrupt_policy = policy; }
    /**
     * 获取当前中断策略
     * @return
     */
    InterruptPolicy getPolicy() const { return interrupt_policy; }
private:
    std::atomic_bool interrupt_flag;
    InterruptPolicy interrupt_policy;
};


class BasicThread{
public:
    BasicThread() : interrupt(nullptr), start(false), m_thread(), thread_status(THREAD_STATUS_INITIALIZE) {}
    virtual ~BasicThread() = default;

    //因为声明了拷贝构造和拷贝赋值函数,编译器不会生成移动构造和移动赋值
    BasicThread(const BasicThread&) = delete;
//    BasicThread(BasicThread&&) = delete;
    BasicThread& operator=(const BasicThread&) = delete;
//    BasicThread& operator=(BasicThread&&) = delete;
    /**
     * 判断当前线程是否能join
     * @return
     */
    bool joinAble() const {
        //线程启动且原始线程能join
        return (start && m_thread.joinable());
    };
    void join() {
        if(joinAble()) { m_thread.join(); }
    }
    void detach() {
        if(joinAble()){ m_thread.detach(); }
    };
    /**
     * 获取当前线程id
     * @return
     */
    std::thread::id id() const { return start ? m_thread.get_id() : std::thread::id(0); }
    ThreadStatus getStatus() const { return thread_status; }
    void startThread(std::promise<void>&);

    virtual ThreadType type() const = 0;
    virtual std::function<void(std::promise<void>&)> onStart() = 0;
    virtual void receiveTask(std::shared_ptr<ExecutorNote>) = 0;
    virtual void receiveTask(std::shared_ptr<ExecutorTask>) = 0;
    virtual void receiveInstruct(SInstruct) = 0;

    void onLoop();
    void onExecutor() { setStatus(THREAD_STATUS_EXECUTOR); }
    void onMount() { setStatus(THREAD_STATUS_MOUNT); }
    void onWait() { setStatus(THREAD_STATUS_WAIT); }
    void onStop() { setStatus(THREAD_STATUS_STOP); }
    void onFinish() { setStatus(THREAD_STATUS_FINISH); }
protected:
    virtual void handleInterrupt() = 0;
    virtual void handleLoop() = 0;

    InterruptFlag *interrupt;
private:
    void setStatus(volatile ThreadStatus status){ thread_status = status; }

    bool start;                             //线程是否启动
    std::thread m_thread;                   //原始线程
    volatile ThreadStatus thread_status;    //线程状态
};

#endif //UNTITLED5_BASICTHREAD_H
