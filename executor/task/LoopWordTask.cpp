//
// Created by abc on 20-5-10.
//
#include "LoopWordTask.h"

std::allocator<LoopExecutorNote> LoopExecutorTask::executor_alloc = std::allocator<LoopExecutorNote>();
std::allocator<LoopTimeOutNote> LoopExecutorTask::timeout_alloc = std::allocator<LoopTimeOutNote>();

/**
 * 构造Loop任务触发Note
 * @return
 */
LoopBaseNote* LoopExecutorTask::createExecutorNote() {
    LoopExecutorNote *executor_note = nullptr;
    try {
        executor_note = executor_alloc.allocate(LOOP_EXECUTOR_TASK_NOTE_SIZE);
        executor_alloc.construct(executor_note);
    } catch (std::bad_alloc &e){
        std::cout << "LoopExecutorTask::createExecutorNote bad_alloc!" << std::endl;
    }
    return executor_note;
}

/**
 * 构造Loop任务超时Note
 * @return
 */
LoopBaseNote* LoopExecutorTask::createTimeOutNote() {
    LoopTimeOutNote *timeout_note = nullptr;
    try {
        timeout_note = timeout_alloc.allocate(LOOP_EXECUTOR_TASK_NOTE_SIZE);
        timeout_alloc.construct(timeout_note);
    } catch (std::bad_alloc &e){
        std::cout << "LoopExecutorTask::createTimeOutNote bad_alloc!" << std::endl;
    }
    return static_cast<LoopBaseNote*>(timeout_note);
}

/**
 * 销毁Loop任务触发Note
 * @param executor_note
 */
void LoopExecutorTask::destroyExecutorNote(ExecutorNote *executor_note) {
    if(executor_note){
        executor_alloc.destroy(executor_note);
        executor_alloc.deallocate(dynamic_cast<LoopExecutorNote*>(executor_note), LOOP_EXECUTOR_TASK_NOTE_SIZE);
    }
}

/**
 * 销毁Loop任务超时Note
 * @param timeout_note
 */
void LoopExecutorTask::destroyTimeOutNote(ExecutorNote *timeout_note) {
    if(timeout_note){
        timeout_alloc.destroy(timeout_note);
        timeout_alloc.deallocate(dynamic_cast<LoopTimeOutNote*>(timeout_note), LOOP_EXECUTOR_TASK_NOTE_SIZE);
    }
}

//--------------------------------------------------------------------------------------------------------------------//

/**
 *  设置Loop任务时间信息
 * @param trigger_count     任务触发次数(包含超时触发)
 * @param interval_time     触发时间间隔
 * @param interval_value    触发时间间隔增加值
 */
void LoopExecutorTask::setTaskTimeInfo(uint32_t trigger_count, int64_t interval_time, int64_t interval_value) {
    time_info.interval_time = interval_time;
    time_info.interval_time_increase = interval_value;
    time_info.ntrigger_count = trigger_count;
    //根据任务需要触发的次数计算任务最大时间
    time_info.max_time = countMaxTime(trigger_count, interval_time, interval_value);
}

/**
 * 计算任务最大时间
 * @param trigger_count     任务触发
 * @param interval_time     时间间隔
 * @param interval_value    时间间隔增加值
 * @return  任务最大时间
 */
int64_t LoopExecutorTask::countMaxTime(uint32_t trigger_count, int64_t interval_time, int64_t interval_value) {
    return (interval_time * trigger_count) + (((trigger_count * (trigger_count - 1)) / 2) * interval_value);
}

/**
 * 返回触发时间
 * @param new_time  当前时间点
 * @return 触发时间
 */
int64_t LoopExecutorTask::triggerTime(const std::chrono::steady_clock::time_point &new_time) const {
#define NEXT_SIZE (time_info.ctrigger_size + 1)
    int64_t next_trigger_time = (NEXT_SIZE * time_info.interval_time) + ((((NEXT_SIZE - 1) * NEXT_SIZE) / 2) * time_info.interval_time_increase);
    return next_trigger_time - currentTime(new_time);
}

/**
 * 返回任务的生存时间
 * @param new_time  当前时间点
 * @return 生存时间
 */
int64_t LoopExecutorTask::currentTime(const std::chrono::steady_clock::time_point &new_time) const {
    return (std::chrono::duration_cast<std::chrono::microseconds>(new_time - time_info.generate_time)).count();
}

/**
 * 任务触发
 * @param new_time  当前时间点
 * @param call_back 回调函数
 * @return Loop任务的Note
 */
std::shared_ptr<LoopBaseNote> LoopExecutorTask::onTimeTrigger(const std::chrono::steady_clock::time_point &new_time,
                                                              const std::function<void(bool)> &call_back) {
    /*
     * 判断任务是否超时
     *  是：设置超时信息,回调true
     *  否：增加触发信息,回调false
     */
    if(isTimeout(new_time)){
        executor_info.is_timeout = true;
        call_back(true);
    }else{
        time_info.ctrigger_size++;
        call_back(false);
    }
    //返回Loop任务的Note
    return std::static_pointer_cast<LoopBaseNote>(executor());
}

/**
 * 返回Loop任务的Note,根据是否超时构造触发Note或超时Note
 * @return
 */
std::shared_ptr<ExecutorNote> LoopExecutorTask::executor()  {
    std::shared_ptr<ExecutorNote> loop_note{nullptr};
    if(executor_info.is_timeout){
        loop_note = std::shared_ptr<ExecutorNote>(createTimeOutNote(), LoopExecutorTask::destroyTimeOutNote);
    }else{
        loop_note = std::shared_ptr<ExecutorNote>(createExecutorNote(), LoopExecutorTask::destroyExecutorNote);
    }
    return loop_note;
}

/**
 * 执行Loop任务的触发Note
 */
void LoopExecutorTask::executorTask() {
    if(func_info.trigger_func) {
        func_info.trigger_func();
    }
    if(executor_func){
        (*executor_func)(false);
    }
}

/**
 * 执行Loop任务的超时Note
 */
void LoopExecutorTask::executorTimeOut() {
    if(func_info.timeout_func) {
        func_info.timeout_func();

        if(timeout_func){
            (*timeout_func)();
        }
    }else{
        holder->executor();
        if(executor_func){
            (*executor_func)(false);
        }
    }
}

/**
 * Loop任务执行完成函数
 */
void LoopExecutorTask::executorNoteComplete() {
    executor_info.complete_size.fetch_add(LOOP_EXECUTOR_TASK_EXECUTOR_OPERATOR_VALUE);
}

/**
 * Loop任务执行失败函数
 */
void LoopExecutorTask::executorFail() {
    executor_info.fail_size.fetch_add(LOOP_EXECUTOR_TASK_EXECUTOR_OPERATOR_VALUE);
}

/**
 * 任务关联执行线程
 * @param correlate_thread 执行线程
 */
void LoopExecutorTask::correlateThread(BasicThread *correlate_thread) {
    this->correlate_thread = correlate_thread;
}