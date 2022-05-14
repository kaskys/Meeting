//
// Created by abc on 20-5-1.
//

#include "ManagerThread.h"

//--------------------------------------------------------------------------------------------------------------------//
//---------------------------------------------PoolScope--------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//
/**
 * 线程池内的ExecutorThread线程
 */
void PoolScope::exe() {
    //ExecutorThread线程正在阻塞
    std::unique_lock<std::mutex> wait_lock(wait_mutex);
    wait_cond.wait(wait_lock);
    //ExecutorThread线程从阻塞状态中唤醒,将阻塞状态更改为NOTIFY
    wait_status.store(STATUS_NOTIFY, std::memory_order_release);
}

void PoolScope::response(SInstruct) {}

/**
 * 处理指令
 * @return
 */
std::function<void()> PoolScope::instruct() {
    std::function<void()> instruct_func{nullptr};

    switch (scope_instruct->instruct_type) {
        case INSTRUCT_SCOPE:
            instruct_func = std::bind(&ExecutorThread::onTransfer, executor, transfer());
            break;
        case INSTRUCT_FINISH:
            instruct_func = std::bind(&ExecutorThread::onFinish, executor,
                                            std::dynamic_pointer_cast<FinishInstruct>(scope_instruct)->finish_callback);
            break;
        default:
            break;
    }

    return instruct_func;
}

/**
 * ExecutorThread线程从ManagerThread内转到线程池内
 */
void PoolScope::convert() {
    //更改挂载状态
    executor->onMount();
}

/**
 * 唤醒ExecutorThread线程
 *  ManagerThread线程或用户线程（ShutDown->Termination）  : 等待ExecutorThread线程将状态从WAIT更改为NOTIFY,才结束循环
 *  ExecutorThread线程（ManagerScope->PoopScope）       :  只有线程从阻塞唤醒才会将状态更改为NOTIFY,该状态直到转换ManagerScope前,其他状态为WAIT
 * 该函数只能被一次唤醒,唤醒之后为NOTIFY状态,在该状态下调用该函数不处理直接返回
 * 在ExecutorThread线程没有从阻塞唤醒前,该函数一直循环唤醒,直到ExecutorThread线程真正唤醒为止（exe()函数返回后）
 */
void PoolScope::notify() {
    for(;;){
        if(wait_status.load(std::memory_order_consume) == STATUS_WAIT){
            wait_cond.notify_all();
        }else{
            break;
        }
    }
}

void PoolScope::flush() {
    //不需要处理
}

/**
 * 转换scope
 * @return ManagerThread的scope
 */
Scope* PoolScope::transfer() {
    wait_status.store(STATUS_WAIT, std::memory_order_release);
    return executor->thread_scope.onScopeConvert(THREAD_SCOPE_MANAGER);
}

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------ManagerScope--------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

ManagerScope::ManagerScope() : Scope(nullptr), manager_thread(nullptr), update_instruct(INSTRUCT_UPDATE_NOTE_SIZE, nullptr) {}

ManagerScope::ManagerScope(ExecutorThread *executor_thread, BasicThread *manager) : Scope(executor_thread), manager_thread(manager),
          update_instruct(INSTRUCT_UPDATE_NOTE_SIZE, static_cast<BasicThread*>(executor_thread)) {}

/**
 * ManagerThread内的ExecutorThread线程
 */
void ManagerScope::exe() {
    std::shared_ptr<ExecutorNote> note{nullptr};

    //获取note
    if((note = executor->task_queue.pop())){
        //执行note（没有取消）
        if (!note->isCancel()) {
            note->executor();
        }else{
            note->cancel();
        }

        //减少ExecutorThread线程内的note数量
        executor->note_size.fetch_sub(DEFAULT_EXECUTOR_OPERATOR_VALUE, std::memory_order_release);
        {
            /*
             * 如果ExecutorThread线程的状态处于EXECUTOR或WAIT状态,发送更新note数量指令
             */
            ThreadStatus executor_status = executor->getStatus();
            if ((executor_status == THREAD_STATUS_EXECUTOR) || (executor_status == THREAD_STATUS_WAIT)) {
                response(std::static_pointer_cast<Instruct>(std::shared_ptr<UpdateNoteSizeInstruct>(&update_instruct,
                                                                                                    ManagerScope::emptyRelease)));
            }
        }
    }
}

/**
 * 向ManagerThread响应指令（ManagerThread内,只对ManagerThread负责）
 * @param response_instruct
 */
void ManagerScope::response(SInstruct response_instruct) {
    if(response_instruct != nullptr){
        dynamic_cast<ManagerThread*>(manager_thread)->receiveInstruct(response_instruct);
    }
}

/**
 * 处理指令
 * @return
 */
std::function<void()> ManagerScope::instruct() {
    std::function<void()> instruct_func(nullptr);

    switch (scope_instruct->instruct_type) {
        case REQUEST_STEAL:
            instruct_func = std::bind(&ExecutorThread::onSteal, executor);
            break;
        case REQUEST_INTERRUPT_POLICY:
            instruct_func = std::bind(&ExecutorThread::onPolicy, executor);
            break;
        case INSTRUCT_SCOPE:
            instruct_func = std::bind(&ExecutorThread::onTransfer, executor, transfer());
            break;
        case RESPONSE_RELEASE:
            instruct_func = std::bind(&ExecutorThread::onRecovery, executor, scope_instruct);
            break;
        case INSTRUCT_STOP:
            instruct_func = std::bind(&ExecutorThread::onStop, executor);
            break;
        default:
            break;
    }

    return instruct_func;
}

/**
 * ExecutorThread线程从线程池转到ManagerThread
 */
void ManagerScope::convert() {
    //更改状态
    executor->onExecutor();
}

void ManagerScope::notify() {
    //不需要处理
}

/**
 * 刷新task_queue（防止task_queue内没有note到有note的时间内等待轮询而无法处理指令）
 */
void ManagerScope::flush() {
    executor->receiveTask(std::shared_ptr<ExecutorNote>{nullptr});
}

/**
 * 转换scope
 * @return 线程池的scope
 */
Scope* ManagerScope::transfer() {
    return executor->thread_scope.onScopeConvert(THREAD_SCOPE_POOL);
}

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------ExecutorThread------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

ExecutorThread::ExecutorThread(BasicExecutor *thread_pool, int max_size) : max_note_size(max_size), scope(nullptr),
                                                  receive_instruct(nullptr), note_size(0), task_queue(), thread_scope(){
    initPoolScope(thread_pool);
    onTransfer(&thread_scope.pool_scope);
    //设置:防止没有note时没有被线程池回收而无限轮询
    //不设置:处于WAIT状态的ExecutorThread线程只有一个,而该线程会优先接收note且会向ManagerThread请求窃取
    //task_queue.setBlockFunc(nullptr, nullptr);
}

void ExecutorThread::onLoop(std::promise<void> &start_promise) {
    BasicThread::onLoop();
    receive_instruct = &interrupt_instruct;
    start_promise.set_value();

    for(;;){
        if(interrupt->isInterrupt()){
            try {
                handleInterrupt();
                continue;
            }catch (finish_error &e){
                break;
            }
        }

        try {
            handleLoop();
        }catch (...){
            onRelease();
        }
    }
}

/**
 * 接收单个note
 * @param note
 */
void ExecutorThread::receiveTask(std::shared_ptr<ExecutorNote> note) throw(std::bad_alloc, thread_load_error) {
    //判断当前note数量超过最大值（大等于）
    if (note_size.load(std::memory_order_consume) < max_note_size) {
        //接收(无内存,抛出bad_alloc异常)
        receiveTask0(std::move(note));
        //更新note数量
        note_size.fetch_add(DEFAULT_EXECUTOR_OPERATOR_VALUE, std::memory_order_release);
    }else{
        //抛出负载异常
        throw thread_load_error();
    }
}

/**
 * 由Loop线程接收,ExecutorThread线程不处理
 */
void ExecutorThread::receiveTask(std::shared_ptr<ExecutorTask>) {}

/**
 * 接收指令
 * @param receive_instruct
 */
void ExecutorThread::receiveInstruct(SInstruct receive_instruct) {
    if(receive_instruct != nullptr){
        //等待上次指令接收完毕
        while(interrupt->isInterrupt());

        //接收当前指令
        (*this->receive_instruct) = receive_instruct;
        //中断线程
        interrupt->interrupt();
        //刷新(防止task_queue无限轮询而无法处理指令)
        scope->flush();
    }
}

/**
 * 响应中断
 */
void ExecutorThread::handleInterrupt() {
    std::function<void()> instruct_func{nullptr};

    if((*receive_instruct) != nullptr){
        //将其他线程发送的中断（线程池或ManagerThread）移动到ExecutorThread线程内
        scope_instruct = *receive_instruct;
        //取消中断,接收中断完成
        clearInterrupt();

        //获取并调用中断函数
        if((instruct_func = scope->instruct()) != nullptr) {
            instruct_func();
        }
    }
}

/**
 * 处理Loop
 */
void ExecutorThread::handleLoop() {
    scope->exe();
}

/**
 * 取消中断
 */
void ExecutorThread::clearInterrupt() {
    //复位指令
    receive_instruct->reset();
    //取消中断
    interrupt->cancelInterrupt();
}

/**
 * 响应scope的转换
 * @param tscope    转换的scope
 */
void ExecutorThread::onTransfer(Scope *tscope) {
    (scope = tscope)->convert();
}

/**
 * 响应窃取
 */
void ExecutorThread::onSteal() {
    uint32_t steal_count = 0;
    std::shared_ptr<StealInstruct> steal_instruct = std::dynamic_pointer_cast<StealInstruct>(scope_instruct);

    /*
     * 1.ExecutorThread线程处于EXECUTOR状态
     * 2.只有没有关联该ExecutorThread线程会被窃取
     * 3.超过请求窃取数量后结束窃取
     */
    if(getStatus() == THREAD_STATUS_EXECUTOR){
        steal_instruct->steal_util = task_queue.splice(
                [&](const std::shared_ptr<ExecutorNote> &note) -> bool {
                    if(note && !note->needCorrelate()){
                        steal_count++;
                        return true;
                    }else{
                        return false;
                    }
              },[&]() -> bool{
                    return steal_count >= steal_instruct->request_steal_size;
              });
    }

    //响应窃取数量
    steal_instruct->request_steal_size = steal_count;
    //更改指令
    scope_instruct->instruct_type = RESPONSE_STEAL;
    //回复指令
    scope->response(scope_instruct);
}

/**
 * 更改中断策略
 */
void ExecutorThread::onPolicy() {
    interrupt->setPolicy(std::dynamic_pointer_cast<InterruptPolicyInstruct>(scope_instruct)->policy);
}

/**
 * 释放ExecutorThread线程
 */
void ExecutorThread::onRelease() {
    //向ManagerThread发送释放指令
    scope->response(Instruct::makeInstruct<ReleaseInstruct>(REQUEST_RELEASE, this));
}

/**
 * 处理回收ExecutorThread线程
 * @param recovery_instruct
 */
void ExecutorThread::onRecovery(SInstruct recovery_instruct) {
    BasicThread::onStop();
    /*
     * 根据中断策略处理task_queue
     */
    switch (interrupt->getPolicy()){
        //清空
        case INTERRUPT_DISCARD:
            task_queue.clear();
            break;
        //完成
        case INTERRUPT_FINISH:
            completeQueue();
            break;
        //转移
        case INTERRUPT_TRANSFER:
            transferQueue();
            break;
    }

    //回复指令
    recovery_instruct->instruct_type = INSTRUCT_RECOVERY;
    scope->response(recovery_instruct);
}

/**
 * 处理停止ExecutorThread线程
 */
void ExecutorThread::onStop(){
    std::cout << "ExecutorThread->onStop->" << std::this_thread::get_id() << std::endl;
    //更新状态
    BasicThread::onStop();
    //清空task_queue
    task_queue.clear();
}

/**
 * 处理结束ExecutorThread线程
 * @param finish_callback
 */
void ExecutorThread::onFinish(void(*finish_callback)(BasicThread*)) throw(finish_error) {
    //更新状态
    BasicThread::onFinish();
    std::cout << "ExecutorThread::onFinish->" << std::this_thread::get_id() << std::endl;
    //抛出结束异常,从而中断Loop
    throw finish_error(finish_callback);
}

/**
 * 完成所有的note
 */
void ExecutorThread::completeQueue() {
    while(!task_queue.empty()){
        scope->exe();
    }
}

/**
 * 转移所有的note
 */
void ExecutorThread::transferQueue() {
    if(!task_queue.empty()) {
        std::shared_ptr<TransferInstruct> transfer_instruct = Instruct::makeInstruct<TransferInstruct>(INSTRUCT_TRANSFER, this);
        transfer_instruct->transfer_util = task_queue.transfer();

        scope->response(transfer_instruct);
    }
}