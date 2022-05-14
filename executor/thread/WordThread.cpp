//
// Created by abc on 20-4-21.
//
#include "WordThread.h"

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------Word----------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * 获取WordThread线程状态
 * @return
 */
ThreadStatus Word::onStatus() {
    return word_thread->getStatus();
}

void Word::onStartComplete(std::shared_ptr<StartInstruct> &start_instruct) {
    if((start_instruct->start_size.fetch_sub(COMPLETE_VALUE) == COMPLETE_VALUE) && start_instruct->start_callback) {
        start_instruct->start_callback(start_instruct);
    }
}

void Word::onStopComplete(std::shared_ptr<StopSocketInstruct> &stop_instruct) {
    if((stop_instruct->stop_size.fetch_sub(COMPLETE_VALUE) == COMPLETE_VALUE)){
        stop_instruct->stop_promise.set_value();
    }
}

/**
 * 更改EXECUTOR状态
 */
void Word::onExecutor()  {
    word_thread->onExecutor();
}

/**
 * 更改STOP状态
 */
void Word::onStop() {
    word_thread->onStop();
}

/**
 * 处理结束WordThread线程
 * @param call_back     回调函数
 */
void Word::onFinish(void(*call_back)(BasicThread*)) throw(finish_error) {
    //更改状态
    word_thread->onFinish();
    //将回调函数赋值给结束异常并抛出
    throw finish_error(call_back);
}

/**
 * 响应指令
 * @param instruct  指令
 */
void Word::response(SInstruct instruct) {
    //由WordThread发送响应指令
    word_thread->sendInstruct(std::move(instruct));
}

/**
 * 从WordThread线程内获取Word
 * @param word_thread   线程
 * @return
 */
Word* Word::originalWordThread(const WordThread *word_thread) const {
    return word_thread->word;
}

/**
 * 获取ManagerThread
 * @return  线程
 */
ManagerThread* Word::threadManager() const {
    return word_thread->thread_manager;
}

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------WordThread----------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

static thread_local SInstruct thread_instruct;

/**
 * Loop函数
 * @param start_promise
 */
void WordThread::onLoop(std::promise<void> &start_promise) {
    void (*finish_callback)(BasicThread*) = nullptr;

    //调用基类的onLoop函数
    BasicThread::onLoop();
    //获取线程变量
    local_instruct = &thread_instruct;
    //调用Word的onLoop函数
    word->onLoop();
    //响应启动完成
    start_promise.set_value();

    for(;;){
        if(interrupt->isInterrupt()){
            try {
                //处理中断
                handleInterrupt();
                continue;
            }catch (finish_error &e){
                //结束异常,处理WordThread线程结束,结束当前Loop
                //获取finish回调函数
                finish_callback = e.getCallBack();
                break;
            }
        }
        //处理任务
        handleLoop();
    }

    if(finish_callback){
        //调用finish回调函数
        (*finish_callback)(this);
    }
}

/**
 * 接收单个note
 *  Loop线程不会处理
 *  Socket线程、Display线程或其他WordThread线程处理
 * @param note
 */
void WordThread::receiveTask(std::shared_ptr<ExecutorNote> note){
    if(getStatus() == THREAD_STATUS_EXECUTOR) {
        //由Word处理
        word->onExecutor(std::move(note));
    }
}

/**
 * 接收单个task
 *  Loop线程处理
 *  Socket线程、Display线程或其他WordThread线程不会处理
 * @param task
 */
void WordThread::receiveTask(std::shared_ptr<ExecutorTask> task) {
    if(getStatus() == THREAD_STATUS_EXECUTOR) {
        //由Word处理
        word->onPush(std::move(task));
    }
}

/**
 * 接收指令
 * @param receive_instruct
 */
void WordThread::receiveInstruct(SInstruct receive_instruct) {
    if(receive_instruct != nullptr){
        //等待上次指令接收完毕
        while(interrupt->isInterrupt());
        //接收当前指令
        (*local_instruct) = receive_instruct;
        //中断线程
        interrupt->interrupt();
        //中断word
        word->interrupt();
    }
}

/**
 * 发送指令
 * @param instruct
 */
void WordThread::sendInstruct(SInstruct instruct) {
    //向指令内的线程发送指令
    instruct->instruct_thread->receiveInstruct(instruct);
}

/**
 * 处理中断
 */
void WordThread::handleInterrupt() throw(finish_error) {
    //将其他线程的中断指令移动到WordThread线程内
    SInstruct interrupt_instruct = *local_instruct;
    //取消中断
    interrupt->cancelInterrupt();
    //由Word处理
    word->onInterrupt(interrupt_instruct);
}

void WordThread::handleLoop() {
    word->exe();
}