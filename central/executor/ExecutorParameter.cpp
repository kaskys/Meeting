//
// Created by abc on 20-12-25.
//

#include "ExecutorParameter.h"

ThreadExecutorParameter::ThreadExecutorParameter() : join_thread_success(0), join_thread_fail(0), transfer_thread_number(0),
                                                     transfer_thread_amount(0), exit_thread_number(0), transfer_thread_response(0) {}

void ThreadExecutorParameter::onInputAddress(ParameterStatus parameter_status) {
    switch (parameter_status){
        case ParameterJoinSuccess:
            join_thread_success++;
            break;
        case ParameterJoinFail:
            join_thread_fail++;
            break;
        case ParameterExit:
            exit_thread_number++;
            break;
        default:
            break;
    }
}

void ThreadExecutorParameter::onTransferAddress(uint32_t number) {
    transfer_thread_number += number;
    transfer_thread_amount++;
}

void ThreadExecutorParameter::onTransferResponse() {
    transfer_thread_response++;
}

void ThreadExecutorParameter::onThreadParameter(ExecutorParameter *parameter) {
    uint32_t exit_number = exit_thread_number.load(std::memory_order_consume);

    parameter->thread_parameter.join_thread_success = join_thread_success;
    parameter->thread_parameter.join_thread_fail = join_thread_fail;
    parameter->thread_parameter.join_thread_number =  join_thread_success + join_thread_fail;
    parameter->thread_parameter.join_thread_runtime =  join_thread_success - exit_number;
    parameter->thread_parameter.exit_thread_number = exit_number;
}

TaskExecutorParameter::TaskExecutorParameter() : request_task_immediately(0), request_task_more(0),
                                                 request_task_fail(0), executor_task_immediately(0), executor_task_more(0),
                                                 cancel_task_immediately(0), cancel_task_more(0), timeout_task_more(0) {
    executor_func = std::bind(&TaskExecutorParameter::onExecutorTask, this, std::placeholders::_1);
    cancel_func = std::bind(&TaskExecutorParameter::onCancelTask, this, std::placeholders::_1);
    timeout_func = std::bind(&TaskExecutorParameter::onTimeOutTask, this);
}

void TaskExecutorParameter::onRequestTask(ParameterStatus parameter_status) {
    switch (parameter_status){
        case ParameterRequestImmediately:
            request_task_immediately++;
            break;
        case ParameterRequestMore:
            request_task_more++;
            break;
        case ParameterRequestFail:
            request_task_fail++;
            break;
        default:
            break;
    }
}

void TaskExecutorParameter::onExecutorTask(bool executor_) {
    executor_ ? executor_task_immediately++ : executor_task_more++;
}

void TaskExecutorParameter::onCancelTask(bool cancel_) {
    cancel_ ? cancel_task_immediately++ :  cancel_task_more++;
}

void TaskExecutorParameter::onTimeOutTask() {
    timeout_task_more++;
}

void TaskExecutorParameter::onTaskParameter(ExecutorParameter *parameter) {
    uint32_t task_immediately = request_task_immediately.load(std::memory_order_consume);
    uint32_t task_more = request_task_more.load(std::memory_order_consume);
    uint32_t task_fail = request_task_fail.load(std::memory_order_consume);

    parameter->task_parameter.request_task_success = task_immediately + task_more;
    parameter->task_parameter.request_task_immediately = task_immediately;
    parameter->task_parameter.request_task_more = task_more;
    parameter->task_parameter.request_task_fail = task_fail;
    parameter->task_parameter.request_task_number = task_immediately + task_more + task_fail;

    parameter->task_parameter.executor_task_immediately = executor_task_immediately.load(std::memory_order_consume);
    parameter->task_parameter.executor_task_more = executor_task_more.load(std::memory_order_consume);
    parameter->task_parameter.cancel_task_immediately = cancel_task_immediately.load(std::memory_order_consume);
    parameter->task_parameter.cancel_task_more = cancel_task_more.load(std::memory_order_consume);
    parameter->task_parameter.timeout_task_more = timeout_task_more.load(std::memory_order_consume);
}