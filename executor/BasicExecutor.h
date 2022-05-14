//
// Created by abc on 20-5-3.
//

#ifndef UNTITLED5_BASICEXECUTOR_H
#define UNTITLED5_BASICEXECUTOR_H

#include "thread/WordTelationUtil.h"

enum ExecutorStatus{
    INIT,           //初始化
    LAUNCH,         //启动
    RUNNING,        //运行
    STOP,           //停止
    TERMINATE       //结束
};

enum LaunchMode{
    LAUNCH_IMMEDIATE,   //构造类时启动(实现)
    LAUNCH_USE_START,   //用户启动(没有实现)
    LAUNCH_USE_SUMBIT   //提交任务时启动(没有实现)
};

class BasicExecutor{
public:
    BasicExecutor() : executor_status(INIT) { }
    virtual ~BasicExecutor() = default;

    BasicExecutor(const BasicExecutor&) = delete;
    BasicExecutor(BasicExecutor&&) = delete;
    BasicExecutor& operator=(const BasicExecutor&) = delete;
    BasicExecutor& operator=(BasicExecutor&&) = delete;

    void launch();
    bool isLaunch() const { return (executor_status.load(std::memory_order_consume) == LAUNCH);}
    void shutDown();
    bool isShutDown() const { return (executor_status.load(std::memory_order_consume) == STOP); }

    virtual BasicThread* requestThread(const ThreadType) = 0;
    virtual void responseThread(BasicThread*) = 0;
    virtual void recoveryThread(BasicThread*) = 0;
protected:
    bool canLaunch(ExecutorStatus status) const { return ((status == INIT) || (status == STOP)); }
    bool canShutDown(ExecutorStatus status) const { return (status == RUNNING); }
    bool canSubmit() const { return (executor_status.load(std::memory_order_consume) == RUNNING); }
    bool canTermination(ExecutorStatus status) const { return ((status == INIT) || (status == STOP)); }
    void shutDown0();
    void termination();
    void termination0(ExecutorStatus);
    void startComplete() { setExecutorStatus(RUNNING);  }

    virtual void requestLaunch(ExecutorStatus) = 0;
    virtual void requestShutDown(std::promise<void>) = 0;
    virtual void requestTermination() = 0;
private:
    ExecutorStatus getExecutorStatus() { return executor_status.load(std::memory_order_consume); }
    void setExecutorStatus(ExecutorStatus status) { executor_status.store(status, std::memory_order_release); }

    std::atomic<ExecutorStatus> executor_status;
    std::future<void> shutdown_callback;
};

#endif //UNTITLED5_BASICEXECUTOR_H
