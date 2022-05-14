//
// Created by abc on 21-10-9.
//

#include "DisplayWordThread.h"

DisplayMemberFunc DisplayWordThread::wait_func = &DisplayWordThread::onFuncWait;
DisplayMemberFunc DisplayWordThread::nofify_func = &DisplayWordThread::onFuncNotify;

DisplayWordThread::DisplayWordThread(WordThread *thread) : Word(thread), interrupt_amount(0), thread_util(nullptr),
                                                           queue_mutex(), queue_condition(),
                                                           display_func(std::bind(nofify_func, this), std::bind(wait_func, this)) {}

void DisplayWordThread::exe() {
    std::function<void()> run_display_func = nullptr;

    for(;;){
        if(interrupt_amount.load(std::memory_order_consume) > 0){
            interrupt_amount.fetch_sub(DISPLAY_THREAD_INTERRUPT_VALUE, std::memory_order_acq_rel);
            break;
        }

        if((run_display_func = display_func.pop())){
            onRunDisplayFunc(run_display_func);
        }
    }
}

void DisplayWordThread::interrupt() {
    onFuncNotify();
}

void DisplayWordThread::onPush(std::shared_ptr<ExecutorTask>) {
    //不需要处理
}

void DisplayWordThread::onExecutor(std::shared_ptr<ExecutorNote>) {
    //不需要处理
}

void DisplayWordThread::onInterrupt(SInstruct instruct) throw(finish_error) {
    switch (instruct->instruct_type) {
        case INSTRUCT_START:
            onStartDisplay(std::dynamic_pointer_cast<StartInstruct>(instruct));
            break;
        case INSTRUCT_STOP:
            onStopDisplay(std::dynamic_pointer_cast<StopInstruct>(instruct));
            break;
        case INSTRUCT_FINISH:
            onFinishDisplay(std::dynamic_pointer_cast<FinishInstruct>(instruct));
            break;
        case INSTRUCT_DISPLAY_CORRELATE:
            onDisplayCorrelateUtil(std::dynamic_pointer_cast<CorrelateDisplayInstruct>(instruct));
            break;
        default:
            break;
    }
}

void DisplayWordThread::onLoop() {
    //不需要处理
}

//--------------------------------------------------------------------------------------------------------------------//

void DisplayWordThread::onFuncWait() {
    std::unique_lock<std::mutex> unique_lock(queue_mutex);
    queue_condition.wait(unique_lock);
}

void DisplayWordThread::onFuncNotify() {
    static std::function<void()> empty_func{nullptr};

    interrupt_amount.fetch_add(DISPLAY_THREAD_INTERRUPT_VALUE, std::memory_order_acq_rel);
    display_func.push(empty_func);
    queue_condition.notify_all();
}

void DisplayWordThread::onStartDisplay(std::shared_ptr<StartInstruct> start_instruct) {
    Word::onExecutor();
    start_instruct->display_thread = word_thread;
    Word::onStartComplete(start_instruct);
    std::cout << "DisplayWordThread::onStartDisplay->" << std::endl;
}

void DisplayWordThread::onStopDisplay(std::shared_ptr<StopInstruct> stop_instruct) {
    Word::onStop();
    display_func.clear();
    std::cout << "DisplayWordThread::onStopDisplay->" << std::endl;
    stop_instruct->stop_promise.set_value();
}

void DisplayWordThread::onFinishDisplay(std::shared_ptr<FinishInstruct> finish_instruct) {
    std::cout << "DisplayWordThread::onFinishDisplay->" << std::endl;
    Word::onFinish(finish_instruct->finish_callback);
}

void DisplayWordThread::onDisplayCorrelateUtil(std::shared_ptr<CorrelateDisplayInstruct> correlate_instruct) {
    thread_util = correlate_instruct->thread_util;
    correlate_instruct->correlate_promise.set_value();
}

void DisplayWordThread::onCorrelateDisplayUtil(DisplayThreadUtil *util) {
    auto correlate_instruct = Instruct::makeInstruct<CorrelateDisplayInstruct>(word_thread, util);
    std::future<void> correlate_future = correlate_instruct->correlate_promise.get_future();

    word_thread->receiveInstruct(correlate_instruct);
    correlate_future.get();
}

DisplayThreadUtil* DisplayWordThread::onCorrelateDisplayUtil() const { return thread_util; }

void DisplayWordThread::onRunDisplayThread(const std::function<void()> &func) {
    if(!func || (word_thread->getStatus() != THREAD_STATUS_EXECUTOR)){
        return;
    }

    if(std::this_thread::get_id() == word_thread->id()){
        onRunDisplayFunc(func);
    }else{
        onWaitDisplayFunc(func);
    }
}

void DisplayWordThread::onWaitDisplayFunc(const std::function<void()> &func) {
    display_func.push(func);
}

void DisplayWordThread::onRunDisplayFunc(const std::function<void()> &func) {
    try {
        func();
    }catch (...){
        std::cout << "DisplayWordThread::onRunDisplayFunc->" << std::endl;
    }
}