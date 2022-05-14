//
// Created by abc on 20-12-25.
//

#ifndef TEXTGDB_EXECUTORPARAMETER_H
#define TEXTGDB_EXECUTORPARAMETER_H

#include <cstdint>
#include <functional>
#include <atomic>

struct ExecutorParameter{
    struct ThreadParameter{
        uint32_t join_thread_number;
        uint32_t exit_thread_number;
        uint32_t join_thread_success;
        uint32_t join_thread_fail;
        uint32_t join_thread_runtime;

        uint32_t transfer_thread_number;
        uint32_t transfer_thread_amount;
        uint32_t transfer_thread_response;
    };

    struct TaskParameter{
        uint32_t request_task_number;
        uint32_t request_task_success;
        uint32_t request_task_fail;
        uint32_t request_task_immediately;
        uint32_t request_task_more;

        uint32_t executor_task_immediately;
        uint32_t cancel_task_immediately;
        uint32_t executor_task_more;
        uint32_t cancel_task_more;
        uint32_t timeout_task_more;
    };

    TaskParameter   task_parameter;
    ThreadParameter thread_parameter;
};

enum ParameterStatus{
    ParameterJoinSuccess,
    ParameterJoinFail,
    ParameterExit,
    ParameterRequestImmediately,
    ParameterRequestMore,
    ParameterRequestFail
};

class ThExecutorParameter {
public:
    ThExecutorParameter() = default;
    virtual ~ThExecutorParameter() = default;

    virtual void onInputAddress(ParameterStatus) = 0;
    virtual void onTransferAddress(uint32_t) = 0;
    virtual void onTransferResponse() = 0;
    virtual void onThreadParameter(ExecutorParameter*) = 0;
};

class ThreadExecutorParameter : public ThExecutorParameter{
public:
    ThreadExecutorParameter();
    ~ThreadExecutorParameter() override = default;

    void onInputAddress(ParameterStatus) override;
    void onTransferAddress(uint32_t) override;
    void onTransferResponse() override;
    void onThreadParameter(ExecutorParameter*) override;
protected:
    uint32_t join_thread_success;                   //由主Socket线程修改该变量（单）
    uint32_t join_thread_fail;                      //由主Socket线程修改该变量（单）

    uint32_t transfer_thread_number;                //由关联的Socket线程修改该变量,但由Loop线程控制一次只能由一个Socket线程执行修改（单）
    uint32_t transfer_thread_amount;                //同上

    std::atomic<uint32_t> exit_thread_number;       //由关联的Socket线程修改该变量（多）
    std::atomic<uint32_t> transfer_thread_response; //由转移的关联的Socket线程修改该变量（多）
};

class TaExecutorParameter {
public:
    TaExecutorParameter() = default;
    virtual ~TaExecutorParameter() = default;

    virtual void onRequestTask(ParameterStatus) = 0;
    virtual void onExecutorTask(bool) = 0;
    virtual void onCancelTask(bool) = 0;
    virtual void onTimeOutTask() = 0;
    virtual void onTaskParameter(ExecutorParameter*) = 0;
};

class TaskExecutorParameter : public TaExecutorParameter{
public:
    TaskExecutorParameter();
    ~TaskExecutorParameter() override = default;

    void onRequestTask(ParameterStatus) override;
    void onExecutorTask(bool) override;
    void onCancelTask(bool) override;
    void onTimeOutTask() override;
    void onTaskParameter(ExecutorParameter*) override;
protected:
    std::atomic<uint32_t> request_task_immediately;         //由所有的Socket线程或其他线程调用修改变量（多）
    std::atomic<uint32_t> request_task_more;                //同上
    std::atomic<uint32_t> request_task_fail;                //同上

    std::atomic<uint32_t> executor_task_immediately;        //由执行Executor线程修改该变量（多）
    std::atomic<uint32_t> executor_task_more;               //同上
    std::atomic<uint32_t> cancel_task_immediately;          //同上
    std::atomic<uint32_t> cancel_task_more;                 //同上
    std::atomic<uint32_t> timeout_task_more;                //同上
protected:
    std::function<void()>     timeout_func;
    std::function<void(bool)> executor_func;
    std::function<void(bool)> cancel_func;
};

#endif //TEXTGDB_EXECUTORPARAMETER_H
