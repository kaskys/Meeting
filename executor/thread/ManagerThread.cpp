// Created by abc on 20-5-2.
//

#include "../ThreadExecutor.h"

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------ThreadInterface-----------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

thread_type ManagerInterface::empty_thread = std::pair<BasicThread*, int>(nullptr, 0);  /* NOLINT */
std::thread::id ManagerInterface::empty_thread_id = std::thread::id(0); /* NOLINT */

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------ThreadCorrelate-----------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

ThreadCorrelate::ThreadCorrelate() : executor_heap(compare_correlate_thread) {
    executor_heap.setPercolateFunc(ThreadCorrelate::percolate);
}

/**
 * 获取最小note的ExecutorThread线程
 * @return 线程
 */
thread_type ThreadCorrelate::executorThread() {
    if(!executor_heap.empty()){
        auto min_thread = executor_heap.get();
        if(min_thread.first != executor_threads.end()){
            return min_thread.first->second;
        }else{
            executor_heap.remove();
            return executorThread();
        }
    }else{
        return empty_thread;
    }
}

/**
 * 获取指定（id）的ExecutorThread线程
 * @param thread_id 线程id
 * @return 线程
 */
thread_type ThreadCorrelate::executorThread(const std::thread::id &thread_id) {
    auto executor_thread = executor_threads.end();
    if((thread_id != empty_thread_id) && (executor_thread = executor_threads.find(thread_id)) != executor_threads.end()){
        return executor_thread->second;
    }else{
        return empty_thread;
    }
}

/**
 * ExecutorThread线程增加一个note
 * @param receive_thread 增加的线程
 */
void ThreadCorrelate::onThreadReceiveNote(thread_type &receive_thread) {
#define THREAD_RECEIVE_NOTE_DEFAULT_VALUE   1
    thread_rank &rthread = executor_heap[receive_thread.second];

    if((rthread.first != executor_threads.end())){
        rthread.second += THREAD_RECEIVE_NOTE_DEFAULT_VALUE;
        executor_heap.reduce(receive_thread.second);
        onThreadUnEmpty(receive_thread.first);
        std::cout << "ThreadCorrelate::onThreadReceiveNote->" << receive_thread.first->id() << "," << rthread.second << std::endl;
    }
}

/**
 * ExecutorThread线程增加一个task
 * @param receive_thread    增加的线程
 * @param receive_size      task内note的数量
 * @return  ture：成功,false:失败
 */
bool ThreadCorrelate::onThreadReceiveTask(thread_type &receive_thread, int receive_size) {
    thread_rank &rthread = executor_heap[receive_thread.second];

    if((rthread.first != executor_threads.end()) &&
               ((rthread.second + receive_size) <= dynamic_cast<ExecutorThread*>(receive_thread.first)->maxNoteSize())){
        std::cout << "onThreadReceiveTask->" << receive_size << "," << rthread.second << ","
                  << dynamic_cast<ExecutorThread*>(receive_thread.first)->maxNoteSize() << ","
                  << receive_thread.first->id() << std::endl;
        rthread.second += receive_size;
        executor_heap.reduce(receive_thread.second);
        onThreadUnEmpty(receive_thread.first);
        return true;
    }else{
        return false;
    }
}

/**
 * 将ExecutorThread线程从ThreadCorrelate中移除
 * @param recovery_thread 移除线程
 */
void ThreadCorrelate::onRecoveryThread(BasicThread *recovery_thread) {
    auto ethread = executor_threads.find(recovery_thread->id());
    if(ethread != executor_threads.end()){
        thread_rank &rthread = executor_heap[ethread->second.second];

        rthread.second = -1;
        executor_heap.increase(ethread->second.second);

        executor_heap.remove();
        executor_threads.erase(ethread);
    }
}

/**
 * ExecutorThread线程减少一个note
 * @param update_thread 线程
 */
void ThreadCorrelate::onThreadUpdate(BasicThread *update_thread) {
    auto ethread = executor_threads.find(update_thread->id());
    if(ethread != executor_threads.end()){
        thread_rank &rthread = executor_heap[ethread->second.second];

        rthread.second--;
        executor_heap.increase(ethread->second.second);

        std::cout << "ThreadCorrelate::onThreadUpdate->" << update_thread->id() << "," << rthread.second << std::endl;
        if(rthread.second <= 0){
            onThreadEmpty(update_thread);
        }
    }
}


/**
 * ExecutorThread线程增加或减少note
 * @param update_thread     线程
 * @param update_size       数量（>0:增加,<0:减少）
 */
void ThreadCorrelate::onThreadUpdate(BasicThread *update_thread, int update_size) {
    auto ethread = executor_threads.find(update_thread->id());
    if(ethread != executor_threads.end()){
        thread_rank &rthread = executor_heap[ethread->second.second];

        rthread.second += update_size;
        if(update_size >= 0) {
            executor_heap.reduce(ethread->second.second);
        }else{
            executor_heap.increase(ethread->second.second);
        }

        if(rthread.second <= 0){
            onThreadEmpty(update_thread);
        }
    }
}

/**
 * 关联ExecutorThread线程（插入线程）
 * @param correlate_thread 线程
 * @return 关联是否成功
 */
thread_type ThreadCorrelate::correlateThread(BasicThread *correlate_thread){
    thread_type ethread{correlate_thread, 0};
    std::pair<correlate_type, bool> insert_thread{executor_threads.end(), false};

    if(correlate_thread && (insert_thread = executor_threads.insert({correlate_thread->id(), ethread})).second){
        try {
            executor_heap.insert(thread_rank(insert_thread.first, 0));
        }catch (std::bad_alloc &e){
            executor_threads.erase(insert_thread.first);
            ethread = empty_thread;
        }
    }else{
        ethread = empty_thread;
    }
    std::cout << "ThreadCorrelate::correlateThread->" << (ethread.first ? ethread.first->id() : empty_thread_id) << std::endl;
    return ethread;
}

/**
 * 处理停止指令（删除所有的ExecutorThread线程）
 */
void ThreadCorrelate::onStopCorrelate() {
    for(;;){
        if(executor_heap.empty()){
            break;
        }
        onRecoveryThread(executor_heap.get().first->second.first);
    }
    executor_threads.clear();
}

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------ThreadDispatcher----------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * ExecutorThread线程内没有note
 *  存在其他空线程且插入成功,回收该ExecutorThread线程到线程池
 *  不存在其他空线程或插入失败,更改等待状态并请求窃取note
 * @param executor_thread 线程
 */
void ThreadDispatcher::onThreadEmpty(BasicThread *executor_thread) {
    if(!idle_thread.empty() || !idle_thread.insert({executor_thread->id(), executor_thread}).second){
        onRecoveryThread(executor_thread);
    }else{
        executor_thread->onWait();
        if(!onRequestStealTask(executor_thread)){
            onRecoveryThread(executor_thread);
            //窃取失败,回收线程
            idle_thread.erase(executor_thread->id());
        }
    }
}

/**
 * ExecutorThread线程内有note,更改执行状态,从空闲队列删除
 * @param executor_thread 线程
 */
void ThreadDispatcher::onThreadUnEmpty(BasicThread *executor_thread) {
    auto ethread = idle_thread.find(executor_thread->id());
    if(ethread != idle_thread.end()){
        executor_thread->onExecutor();
        idle_thread.erase(ethread);
    }
}

/**
 * 将note添加到ExecutorThread线程
 * @param note
 * @param executor_thread   指定的添加线程
 * @return 无法添加且需要执行的note
 */
int ThreadDispatcher::sendNoteToThread(std::shared_ptr<ExecutorNote> &note, thread_type executor_thread) {
    int send_res = 0;
    BasicThread *correlate_thread = std::dynamic_pointer_cast<LoopBaseNote>(note)->getCorrelateThread();

    /*
     * 无线程或添加失败执行的lambda函数
     */
    auto exception_func = [&]() -> void {
        executor_thread.first = nullptr;
        //判断无法添加时是否需要执行
        if(std::dynamic_pointer_cast<LoopBaseNote>(note)->isExecutor()){
            //返回note等待队列
            onThreadLoad(note);
            //当前note有关联的ExecutorThread线程（一般情况不会执行,除非ExecutorThread线程无内存增加note(即bad_alloc),不处理也不响应）
            //没有关联的note响应调用函数
            send_res = (correlate_thread ? 0 : 1);
        }else{
            //调用note的失败函数
            note->fail();
        }
    };

    /*
     * 没有指定线程,获取最小note的ExecutorThread线程
     */
    if(!executor_thread.first){
        executor_thread = executorThread();
    }

    if(executor_thread.first){
        try {
            executor_thread.first->receiveTask(note);
            if(!correlate_thread) {
                //添加成功且没有关联的note（有关联的note在task已经处理（onThreadReceiveTask））,增加ExecutorThread线程内note的数量
                onThreadReceiveNote(executor_thread);
            }
        }catch (std::bad_alloc &e){
            exception_func();
        }catch (thread_load_error &e){
            exception_func();
        }
    }else{
        exception_func();
    }
    
    return send_res;
}

/**
 * 分配一个note到ExecutorThread线程
 * @param note
 */
void ThreadDispatcher::dispenseNoteToExecutorThread(std::shared_ptr<ExecutorNote> &note) {
    int dispense_res;

    /*
     * 分配到note关联ExecutorThread线程或分配到最小note的ExecutorThread线程
     */
    if(note->getCorrelateThread()){
        dispense_res = sendNoteToThread(note, executorThread(note->getCorrelateThread()->id()));
    }else{
        dispense_res = sendNoteToThread(note, empty_thread);
    }

    if(dispense_res) {
        //分配失败
        onDispenseLoad();
    }
}

/**
 * 分配多个note到ExecutorThread线程
 * @param notes
 * @param executor_thread 指定线程or空线程
 */
void ThreadDispatcher::dispenseNoteToExecutorThread(ThreadQueueUtil note_util, thread_type &executor_thread,
                                                    const std::function<void(std::shared_ptr<ExecutorNote>&)> &call_back) {
    int dispense_res = 0;
    UnLockQueue<std::shared_ptr<ExecutorNote>> queue_(note_util);

    for(std::shared_ptr<ExecutorNote> note;; ){
        if(queue_.empty() || ((note = queue_.pop()) != nullptr)){
            break;
        }

        /*
         * 这里处理转移的note,需要回调note给调用者用于删除关联ExecutorThread线程
         */
        if(call_back != nullptr){
            call_back(note);
        }

        //同上
        if(executor_thread.first){
            dispense_res += sendNoteToThread(note, executor_thread);
        }else{
            dispense_res += sendNoteToThread(note, executorThread(note->getCorrelateThread() ?
                                                                  note->getCorrelateThread()->id() : empty_thread_id));
        }
    }

    if(dispense_res) {
        onDispenseLoad();
    }
}

/**
 * 分配一个task到ExecutorThread线程
 * @param task
 * @return
 */
BasicThread* ThreadDispatcher::dispenseTaskToExecutorThread(std::shared_ptr<LoopExecutorTask> &task) {
    thread_type correlate_thread = empty_thread;

    /*
     * 如果该task没有note,则不能分配到ExecutorThread线程
     */
    if(task->getNeedTriggerSize() > 0){
        //循环处理,直到分配成功
        for(;;){
            //获取到最小note的ExecutorThread线程且该ExecutorThread线程添加task成功
            if((correlate_thread = executorThread()).first && onThreadReceiveTask(correlate_thread, task->getNeedTriggerSize())){
                break;
            }
            //分配失败
            onDispenseLoad();
        }
    }
    
    return correlate_thread.first;
}

/**
 * 处理停止指令
 */
void ThreadDispatcher::onStopDispatcher() {
    idle_thread.clear();
}

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------ManagerThread-------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//
FlushInstruct ManagerThread::flush_instruct = FlushInstruct();  /* NOLINT */

ManagerThread::ManagerThread() : BasicThread(), ThreadDispatcher(), ThreadCorrelate(), ManagerInterface(),
                                 thread_steal(false), max_thread_size(0), executor_pool(nullptr), queue_future(), queue_promise(),
                                 instruct_queue( std::bind(&ManagerThread::onThreadNotify, this), std::bind(&ManagerThread::onThreadWait, this)),
                                 wait_notes(), wait_tasks() {
    queue_future = queue_promise.get_future();
    max_thread_size = DEFAULT_EXECUTOR_THREAD_SIZE;
}

void ManagerThread::onLoop(std::promise<void> &start_promise) {
    BasicThread::onLoop();
    start_promise.set_value();

    for(;;){
        if(interrupt->isInterrupt()){
            handleInterrupt();
            break;
        }
        handleLoop();
        dispenseTask();
        dispenseNote();
    }
}

/**
 * 接收单个note（非ManagerThread）
 *  用户线程提交的note
 *  Loop线程task生成的note
 * @param note
 */
void ManagerThread::receiveTask(std::shared_ptr<ExecutorNote> note){
    if(getStatus() == THREAD_STATUS_EXECUTOR){ wait_notes.push(note); }
}

/**
 * 接收多个note（所有线程）
 *  窃取note后无法添加的note
 * @param receive_notes
 */
void ManagerThread::receiveNotes(ThreadQueueUtil receive_notes) {
    if(getStatus() == THREAD_STATUS_EXECUTOR){
        wait_notes.merge(receive_notes);
        onThreadFlush();
    }else{
        UnLockQueue<std::shared_ptr<ExecutorNote>> cqueue(receive_notes);
        cqueue.clear();
    }
}

/**
 * 接收单个task（非ManagerThread）
 *  用户线程无法调用
 *  由时间管理层或其他层（？）添加的task
 * @param receive_task
 */
void ManagerThread::receiveTask(std::shared_ptr<ExecutorTask> receive_task) {
    if(getStatus() == THREAD_STATUS_EXECUTOR){
        wait_tasks.push(receive_task);
        onThreadFlush();
    }
}

/**
 * 响应处理接收的task(将task从其他线程转到ManagerThread线程处理)
 * @param receive_task
 */
void ManagerThread::onReceiveTask(std::shared_ptr<ExecutorTask> receive_task) {
    //转换类型
    std::shared_ptr<LoopExecutorTask> loop_task = std::dynamic_pointer_cast<LoopExecutorTask>(receive_task);
    //将task关联到ExecutorThread线程
    loop_task->correlateThread(dispenseTaskToExecutorThread(loop_task));
    ((dynamic_cast<ThreadExecutor*>(executor_pool)))->onReceiveTask(receive_task);
}

/**
 * 接收指令（非ManagerThread）
 * @param instruct
 */
void ManagerThread::receiveInstruct(SInstruct instruct) {
    while(interrupt->isInterrupt()){ }
    instruct_queue.push(instruct);
}

/**
 * 响应中断指令
 */
void ManagerThread::handleInterrupt() {
    //清空指令队列
    instruct_queue.clear();
}

/**
 * 处理指令
 */
void ManagerThread::handleLoop() {
    std::shared_ptr<Instruct> instruct = instruct_queue.pop();
    if(instruct != nullptr){
        switch (instruct->instruct_type){
            case REQUEST_THREAD:
                onThreadRequest(std::dynamic_pointer_cast<ThreadInstruct>(instruct));
                break;
            case INSTRUCT_START:
                onThreadStart(std::dynamic_pointer_cast<StartInstruct>(instruct));
                break;
            case INSTRUCT_START_COMPLETE:
                onThreadStartComplete(std::dynamic_pointer_cast<StartInstruct>(instruct));
                break;
            case INSTRUCT_START_FAIL:
                onThreadStartFail(std::dynamic_pointer_cast<StartInstruct>(instruct));
                break;
            case INSTRUCT_UPDATE_NOTE_SIZE:
                onThreadUpdate(instruct->instruct_thread);
                break;
            case RESPONSE_STEAL:
                onResponseStealTask(std::dynamic_pointer_cast<StealInstruct>(instruct));
                break;
            case INSTRUCT_ADDRESS_JOIN:
            case INSTRUCT_ADDRESS_EXIT:
                instruct->instruct_thread->receiveInstruct(instruct);
                break;
            case REQUEST_RELEASE:
                onThreadRelease(instruct);
                break;
            case INSTRUCT_TRANSFER:
                onTransferTask(std::dynamic_pointer_cast<TransferInstruct>(instruct));
                break;
            case INSTRUCT_RECOVERY:
                onRecoveryThread(instruct);
                break;
            case INSTRUCT_STOP:
                onThreadStop(std::dynamic_pointer_cast<StopInstruct>(instruct));
                break;
            case INSTRUCT_FINISH:
                onThreadFinish();
                break;
            case INSTRUCT_FLUSH:
                break;
            default:
                break;
        }
    }
}

/**
 * 将SocketWordThread提交控制层为其关联传输层相关的数据
 * @param transmit_thread 线程
 */
uint32_t ManagerThread::transmitThread() {
    return dynamic_cast<ThreadExecutor*>(executor_pool)->onTransmitThread();
}

void ManagerThread::transmitThread(BasicThread *transmit_thread, uint32_t transmit_id) {
    dynamic_cast<ThreadExecutor*>(executor_pool)->onTransmitThread(dynamic_cast<WordThread*>(transmit_thread), transmit_id);
}

void ManagerThread::transmitRecoveryThread(BasicThread *transmit_thread) {
    dynamic_cast<ThreadExecutor*>(executor_pool)->unTransmitThread(dynamic_cast<WordThread*>(transmit_thread));
}

void ManagerThread::displayThread(BasicThread *display_thread) {
    dynamic_cast<ThreadExecutor*>(executor_pool)->onDisplayThread(dynamic_cast<WordThread*>(display_thread));
}

/**
 * 处理task（将task从其他线程转到ManagerThread线程）
 */
void ManagerThread::dispenseTask() {
    for(;;){
        if(wait_tasks.empty()){
            break;
        }
        onReceiveTask(wait_tasks.pop());
    }
}

/**
 * 处理note（将note从其他线程转到ManagerThread线程）
 */
void ManagerThread::dispenseNote() {
    for(std::shared_ptr<ExecutorNote> note;;){
        if(wait_notes.empty()){
            break;
        }
        note = wait_notes.pop();
        dispenseNoteToExecutorThread(note);
    }
}

/**
 * 没有指令时阻塞线程
 */
void ManagerThread::onThreadWait() {
    queue_future.get();
    queue_future = (queue_promise = std::promise<void>()).get_future();
}

/**
 * 唤醒线程
 */
void ManagerThread::onThreadNotify() {
    try {
        queue_promise.set_value();
    }catch (std::future_error &e){
        std::cout << "onThreadNotify->" << e.what() << std::endl;
    }
}

/**
 * 地址转移
 * @param address
 */
void ManagerThread::onThreadTransfer(MeetingAddressNote *transfer_note, uint32_t transfer_size, uint32_t transfer_id) {
    dynamic_cast<ThreadExecutor*>(executor_pool)->onTransferAddress(transfer_note, transfer_size, transfer_id);
}

/**
 * 插入空指令(用于接收note或task时ManagerThread线程没有指令时处于阻塞状态)`
 */
void ManagerThread::onThreadFlush() {
    if(instruct_queue.empty()) {
        receiveInstruct(std::shared_ptr<Instruct>(&flush_instruct, ManagerThread::emptyReleaseInstruct));
    }
}

/**
 * 回收ExecutorThread线程
 * @param thread 线程
 */
void ManagerThread::onRecoveryThread(BasicThread *thread) {
    if(thread != nullptr){
        ThreadCorrelate::onRecoveryThread(thread);

        //向ExecutorThread线程发送STOP指令
        thread->receiveInstruct(Instruct::makeInstruct<StopInstruct>(INSTRUCT_STOP, nullptr));
        //将ExecutorThread线程回收到线程池
        executor_pool->recoveryThread(thread);
    }
}

/**
 * 回收ManagerThread线程
 * @param instruct  指令
 */
void ManagerThread::onRecoveryThread(SInstruct instruct) {
    BasicThread *recovery_thread = instruct->instruct_thread;

    if(recovery_thread->type() == THREAD_TYPE_SOCKET){
        transmitRecoveryThread(recovery_thread);
    }

    executor_pool->recoveryThread(instruct->instruct_thread);
}

/**
 * 请求窃取note
 * @param request_thread    请求线程
 * @return 窃取note是否成功
 */
bool ManagerThread::onRequestStealTask(BasicThread *request_thread) {
    int note_size = 0;
    BasicThread *steal_thread = nullptr;
    thread_type ethread = executorThread(request_thread->id());

    /*
     * 1.当前没有等待添加的note
     * 2.请求的ExecutorThread线程没有处于等待状态
     *      请求失败：ExecutorThread线程处于等待状态->回收该线程
     * 3.当前没有处理窃取请求
     * 4.没有可以响应窃取的ExecutorThread线程
     * 5.响应窃取的ExecutorThread线程的note数量没有达到阀值（最大note数量的一半）
     *      请求成功：窃取的note数量为响应窃取ExecutorThread线程当前note的一半（实际响应的note数量不一定等于请求窃取的数量（关联ExecutorThread线程的note不能被窃取））
     *              当前无视ExecutorThread线程下一次的请求窃取,直到响应窃取完成
     */
    if(wait_notes.empty() && (ethread != empty_thread) && (request_thread->getStatus() == THREAD_STATUS_WAIT)){
        if((steal_thread = stealThread().first)){
            if(!thread_steal && (steal_thread != request_thread) && ((note_size = threadNoteSize(executorThread(steal_thread->id()).second)) >= DEFAULT_EXECUTOR_STEAL_SIZE)){
                thread_steal = true;
                //计算请求窃取数量
                note_size = ((note_size /= 2) >= DEFAULT_EXECUTOR_STEAL_SIZE ? DEFAULT_EXECUTOR_STEAL_SIZE : note_size);

                std::cout << "onRequestStealTask->(request_thread:" << request_thread->id() << ", steal_thread:" << steal_thread->id() << "," << note_size << ")" << std::endl;
                //发送窃取指令
                steal_thread->receiveInstruct(Instruct::makeInstruct<StealInstruct>(REQUEST_STEAL, steal_thread, note_size , request_thread->id()));
            }
        }else{
            return false;
        }
    }

    return true;
}

/**
 * 响应窃取note
 * @param steal_instruct
 */
void ManagerThread::onResponseStealTask(std::shared_ptr<StealInstruct> steal_instruct) {
    auto ethread = executorThread(steal_instruct->request_steal_id);

    //响应窃取完成,能够处理下一次的请求
    thread_steal = false;
    //已经窃取到note
    if(!steal_instruct->steal_util.isEmpty()){
        //更新响应窃取ExecutorThread线程的note的数量
        onThreadUpdate(steal_instruct->instruct_thread, 0 - steal_instruct->request_steal_size);

        //1.请求ExecutorThread线程没有被回收
        //2.请求ExecutorThread线程处于等待状态（没有note）或ExecutorThread线程的note数量小于数大note数量的一半
        if(ethread.first && ((ethread.first->getStatus() == THREAD_STATUS_WAIT) || (threadNoteSize(ethread.second) < DEFAULT_EXECUTOR_STEAL_SIZE))){
            //将窃取的note分配到请求的ExecutorThread线程
            dispenseNoteToExecutorThread(steal_instruct->steal_util, ethread, nullptr);
        }else{
            //将窃取的note回收到等待添加的note,等待下一轮分配
            receiveNotes(steal_instruct->steal_util);
        }
    }
}

/**
 * 转换回收ExecutorThread线程内的note
 * @param transfer_instruct
 */
void ManagerThread::onTransferTask(std::shared_ptr<TransferInstruct> transfer_instruct) {
    if(!transfer_instruct->transfer_util.isEmpty()){
        dispenseNoteToExecutorThread(transfer_instruct->transfer_util, empty_thread,
                                     [&](std::shared_ptr<ExecutorNote> &enote) -> void {
                                        if(enote->getCorrelateThread() == transfer_instruct->instruct_thread){
                                            std::static_pointer_cast<LoopExecutorNote>(enote)->setCorrelateThread(nullptr);
                                        }
                                     });
    }
}

/**
 * 所有的ExecutorThread线程无法添加note
 * @param note
 */
void ManagerThread::onThreadLoad(std::shared_ptr<ExecutorNote> note) {
    //下一轮会分配线程
    wait_notes.push(note);
    //插入空指令（防止当前没有指令时处于阻塞状态而无法处理note）
    onThreadFlush();
}

/**
 * 处理无法添加note
 */
void ManagerThread::onDispenseLoad() {
    BasicThread *executor_thread = nullptr;

    //当前ManagerThread管理的ExecutorThread线程还没有达到上限且从线程池中获取到ExecutorThread线程
    if((threadSize() < max_thread_size) && (executor_thread = executor_pool->requestThread(THREAD_TYPE_EXECUTOR))) {
        //处理获取线程
        onThreadObtain(executor_thread);

        if(!(correlateThread(executor_thread)).first){
            //关联线程失败,回收线程
            executor_pool->recoveryThread(executor_thread);
        }
    }
}

/**
 * 响应线程请求
 *  Loop线程请求Socket线程
 * @param thread_instruct   指令
 */
void ManagerThread::onThreadRequest(std::shared_ptr<ThreadInstruct> thread_instruct) {
//    BasicThread *request_thread = thread_instruct->instruct_thread;
//    //从线程池中获取线程
//    thread_instruct->instruct_thread = executor_pool->requestThread(thread_instruct->thread_type);
//    //更改响应指令
//    thread_instruct->instruct_type = RESPONSE_THREAD;
//
//    //响应指令（无论是否获取成功）
//    request_thread->receiveInstruct(std::static_pointer_cast<Instruct>(thread_instruct));

    //不使用指令响应Thread（无法分辨LoopThread线程是那种类型请求线程）
    //使用promise响应Thread
    uint32_t socket_id = 0;
    BasicThread *thread = nullptr;

    if((thread_instruct->thread_type != THREAD_TYPE_SOCKET) || (socket_id = transmitThread())){
        thread = executor_pool->requestThread(thread_instruct->thread_type);
        transmitThread(thread, socket_id);
    }

    thread_instruct->thread_promise.set_value(thread);
}

/**
 * 启动线程（ManagerThread线程处理）
 *  用户线程 -> 线程池 -> ManagerThread
 * @param start_instruct    指令
 */
void ManagerThread::onThreadStart(std::shared_ptr<StartInstruct> start_instruct) {
    uint32_t socket_id = 0;
    //更改状态
    onExecutor();
    std::cout << "ManagerThread::onThreadStart->" << std::endl;
    /*
     * 向控制层请求创建socket线程
     */
    if(!(socket_id = transmitThread())){
        //请求失败，启动失败
        onThreadStartFail(start_instruct);
    }else {
        start_instruct->socket_id = socket_id;
        //请求成功，调用线程池启动函数
        dynamic_cast<ThreadExecutor*>((executor_pool = start_instruct->executor_pool))->onStartThread(start_instruct);
    }
}

/**
 * 完成启动线程（ManagerThread线程处理）
 *  其他WordThread -> ManagerThread
 * @param start_instruct    指令
 */
void ManagerThread::onThreadStartComplete(std::shared_ptr<StartInstruct> start_instruct) {
    //获取ExecutorThread线程
    BasicThread *executor_thread = executor_pool->requestThread(THREAD_TYPE_EXECUTOR);
    //处理获取线程
    onThreadObtain(executor_thread);
    //处理socket线程与传输层的关联
    transmitThread(start_instruct->instruct_thread, start_instruct->socket_id);
    //处理display线程与显示层的关联
    displayThread(start_instruct->display_thread);
    //关联线程
    if(!correlateThread(executor_thread).first){
        start_instruct->start_fail = true;
        onThreadStartFail(start_instruct);
    }else {
        //调用线程池完成启动函数
        (dynamic_cast<ThreadExecutor *>(executor_pool))->onStartComplete(start_instruct);
    }
}

void ManagerThread::onThreadStartFail(std::shared_ptr<StartInstruct>) throw(std::logic_error){
    throw std::logic_error("onThreadStartFail!");
}

/**
 * 处理从线程池获取ExecutorThread线程
 * @param thread    线程
 */
void ManagerThread::onThreadObtain(BasicThread *thread) {
    //生成ExecutorThread线程的scope指令(将ExecutorThread线程从线程池转到ManagerThread)
    std::shared_ptr<ScopeInstruct> obtain_instruct = Instruct::makeInstruct<ScopeInstruct>(INSTRUCT_SCOPE, nullptr);
    obtain_instruct->manager = this;
    //发送指令
    thread->receiveInstruct(obtain_instruct);

    //向线程池响应获取ExecutorThread线程（中断ExecutorThread线程的阻塞状态）
    executor_pool->responseThread(thread);
}

/**
 * 处理释放ExecutorThread线程
 * @param release_instruct
 */
void ManagerThread::onThreadRelease(SInstruct release_instruct) {
    BasicThread *executor_thread = nullptr;
    if((executor_thread = release_instruct->instruct_thread)){
        //更改ExecutorThread的note数量为最大值（无法添加note）
        onThreadUpdate(executor_thread, DEFAULT_EXECUTOR_NOTE_SIZE);

        release_instruct->instruct_type = RESPONSE_RELEASE;
        //响应释放指令
        executor_thread->receiveInstruct(release_instruct);
    }
}

/**
 * 处理停止指令
 * @param stop_instruct
 */
void ManagerThread::onThreadStop(std::shared_ptr<StopInstruct> stop_instruct) {
    //更改状态
    onStop();
    std::cout <<  "ManagerThread::onThreadStop->" << std::endl;
    //ThreadCorrelate处理停止指令
    onStopCorrelate();
    //ThreadDispatcher处理停止指令
    onStopDispatcher();
    //清空等待添加的note
    wait_notes.clear();
    //清空等待添加的task
    wait_tasks.clear();
    //清空指令
    instruct_queue.clear();
    //响应完成指令
    stop_instruct->stop_promise.set_value();
    std::cout <<  "ManagerThread::onThreadStop->set_Value" << std::endl;
}

/**
 * 处理结束指令
 */
void ManagerThread::onThreadFinish() {
    //中断
    interrupt->interrupt();
    std::cout << "ManagerThread::onThreadFinish->" << std::endl;
    //更改状态
    onFinish();
}