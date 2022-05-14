//
// Created by abc on 20-5-3.
//
#include "../central/executor/ExecutorLayer.h"

//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------ExecutorProxy-----------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

void ExecutorProxy::onCreate(ThreadExecutor *pool, BasicThread *thread) const {
    pool->onCreateThread(dynamic_cast<ExecutorThread*>(thread));
}

BasicThread* ExecutorProxy::onRequest(ThreadExecutor *pool) const {
    return pool->onRequestThread(*this);
}

void ExecutorProxy::onResponse(ThreadExecutor *pool, BasicThread *thread) const {
    pool->onResponseThread(dynamic_cast<ExecutorThread*>(thread));
}

void ExecutorProxy::onRecovery(ThreadExecutor *pool, BasicThread *thread) const {
    pool->onRecoveryThread(dynamic_cast<ExecutorThread*>(thread));
}

//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------ManagerProxy------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

void ManagerProxy::onCreate(ThreadExecutor *pool, BasicThread *thread) const {
    pool->onCreateThread(dynamic_cast<ManagerThread*>(thread));
}

//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------WordProxy---------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

void WordProxy::onCreate(ThreadExecutor *pool, BasicThread *thread) const {
    pool->onCreateThread(dynamic_cast<WordThread*>(thread));
}

BasicThread* WordProxy::onRequest(ThreadExecutor *pool) const {
    return pool->onRequestThread(*this);
}

void WordProxy::onRecovery(ThreadExecutor *pool, BasicThread *thread) const {
    pool->onRecoveryThread(dynamic_cast<WordThread*>(thread));
}

//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------LoopProxy---------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

BasicThread* LoopProxy::createThread(BasicExecutor*) const throw(std::bad_alloc) {
    auto *loop_thread = dynamic_cast<WordThread*>(WordProxy::createThread(nullptr));
    try {
        loop_thread->setWord(static_cast<Word*>(new LoopWordThread(loop_thread, true)));
    }catch (std::bad_alloc &e){
        WordProxy::destroyThread(static_cast<BasicThread*>(loop_thread));
        throw;
    }
    return loop_thread;
}

void LoopProxy::destroyThread(BasicThread *thread) const {
    delete(dynamic_cast<WordThread*>(thread)->getWord());
    WordProxy::destroyThread(thread);
}

//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------SocketProxy-------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

BasicThread* SocketProxy::createThread(BasicExecutor*) const throw(std::bad_alloc) {
    auto *socket_thread = dynamic_cast<WordThread*>(WordProxy::createThread(nullptr));
    try {
        socket_thread->setWord(static_cast<Word*>(new SocketWordThread(socket_thread)));
    }catch (std::bad_alloc &e){
        WordProxy::destroyThread(static_cast<BasicThread*>(socket_thread));
        throw;
    }
    return socket_thread;

}

void SocketProxy::destroyThread(BasicThread *thread) const {
    delete(dynamic_cast<WordThread*>(thread)->getWord());
    WordProxy::destroyThread(thread);
}

//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------DisplayProxy------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

BasicThread* DisplayProxy::createThread(BasicExecutor*) const throw(std::bad_alloc) {
    auto *socket_thread = dynamic_cast<WordThread*>(WordProxy::createThread(nullptr));
    try {
        socket_thread->setWord(static_cast<Word*>(new SocketWordThread(socket_thread)));
    }catch (std::bad_alloc &e){
        WordProxy::destroyThread(static_cast<BasicThread*>(socket_thread));
        throw;
    }
    return socket_thread;

}

void DisplayProxy::destroyThread(BasicThread *thread) const {
    delete(dynamic_cast<WordThread*>(thread)->getWord());
    WordProxy::destroyThread(thread);
}


//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------ThreadExecutor----------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

thread_proxy ThreadExecutor::core_proxy = thread_proxy(ManagerProxy(), ExecutorProxy(),  LoopProxy(), SocketProxy(), DisplayProxy());  /* NOLINT */

ThreadExecutor::ThreadExecutor(BasicLayer *layer, LaunchMode launch_mode) throw(std::runtime_error)
        : BasicExecutor(), executor_launch_mode(launch_mode), idle_thread_flag(false), manager_thread(nullptr), executor_layer(layer) {
    loop_thread = word_threads.end();
    createThread();
    if(launch_mode == LAUNCH_IMMEDIATE){
        launch();
    }
}

ThreadExecutor::~ThreadExecutor() {
    termination();
}

void ThreadExecutor::createThread() {
    ErgodicProxy<thread_proxy>::ecreate(core_proxy, this, [&](const ThreadProxy &thread_proxy) -> bool { return true; });
}

void ThreadExecutor::closeThread() {
    ErgodicProxy<thread_proxy>::eclose(core_proxy, this);
}

void ThreadExecutor::startThread(std::promise<void> &start_promise, ExecutorStatus &status) throw(std::runtime_error) {
    if(manager_thread){
        startThread(static_cast<BasicThread*>(manager_thread));
        manager_thread->receiveInstruct(Instruct::makeInstruct<StartInstruct>(INSTRUCT_START, nullptr, static_cast<BasicExecutor*>(this),
                                                                              &start_promise, (status == INIT)));
    }else{
        throw std::runtime_error("not thread to start!");
    }
}

void ThreadExecutor::startThread(BasicThread *thread) {
    std::promise<void> start_promise;
    thread->startThread(start_promise);
    start_promise.get_future().get();
}

void ThreadExecutor::onStartThread(std::shared_ptr<StartInstruct> &start_instruct) {
    if(start_instruct->start_thread){
        startExecutorThread();
    }
    startWordThread(start_instruct);
}

void ThreadExecutor::onStartComplete(std::shared_ptr<StartInstruct> &start_instruct) {
    startComplete();
    start_instruct->start_promise->set_value();
}

void ThreadExecutor::startExecutorThread() {
    for(auto begin : idle_executor_threads){
        startThread(begin);
    }
}

void ThreadExecutor::startWordThread(std::shared_ptr<StartInstruct> &start_instruct) {
    start_instruct->start_size.store(static_cast<int>(word_threads.size() - 1));
    start_instruct->start_callback = ThreadExecutor::startWordThread0;

    for(auto begin = word_threads.begin(), end = word_threads.end(); begin != end; ++begin){
        if(start_instruct->start_thread) {
            startThread(begin->second);
        }

        switch (begin->second->type()) {
            case THREAD_TYPE_LOOP:
                loop_thread = begin;
                break;
            case THREAD_TYPE_SOCKET:
                //这里已经将指令的instruct_thread赋值为SocketWordThread
                start_instruct->instruct_thread = begin->second;
            default:
                begin->second->receiveInstruct(start_instruct);
                break;
        }
    }

    loop_thread->second->receiveInstruct(start_instruct);
}

void ThreadExecutor::startWordThread0(std::shared_ptr<StartInstruct> &start_instruct) {
    //这里的start_instruct->instruct_thread 是 SocketWordThread
    start_instruct->instruct_type = (start_instruct->start_fail ? INSTRUCT_START_FAIL : INSTRUCT_START_COMPLETE);
    (dynamic_cast<ThreadExecutor*>(start_instruct->executor_pool))->manager_thread->receiveInstruct(std::static_pointer_cast<Instruct>(start_instruct));
}

/**
 * 请求线程
 * @param thread_type
 * @return
 */
BasicThread* ThreadExecutor::requestThread(ThreadType thread_type) {
    BasicThread *thread = nullptr;
    try {
        thread = ErgodicProxy<thread_proxy>::getProxy(core_proxy, thread_type).onRequest(this);
    }catch (ThreadProxyException &e){
        std::cout << "requestThread->" << e.what() << std::endl;
    }
    return thread;
}

void ThreadExecutor::responseThread(BasicThread *response_thread) {
    try {
        ErgodicProxy<thread_proxy>::getProxy(core_proxy, response_thread->type()).onResponse(this, response_thread);
    }catch (ThreadProxyException &e){
        std::cout << "responseThread->" << e.what() << std::endl;
    }
}

void ThreadExecutor::recoveryThread(BasicThread *recovery_thread) {
    try {
        ErgodicProxy<thread_proxy>::getProxy(core_proxy, recovery_thread->type()).onRecovery(this, recovery_thread);
    }catch (ThreadProxyException &e){
        std::cout << "recoveryThread->" << e.what() << std::endl;
    }

}


void ThreadExecutor::submit(std::shared_ptr<ExecutorNote> note) {
    manager_thread->receiveTask(std::move(note));
}

void ThreadExecutor::submit(std::shared_ptr<ExecutorTask> task) {
    manager_thread->receiveTask(std::move(task));
}

void ThreadExecutor::onReceiveTask(std::shared_ptr<ExecutorTask> &task) {
    loop_thread->second->receiveTask(std::move(task));
}

void ThreadExecutor::onTransferAddress(MeetingAddressNote *transfer_note, uint32_t transfer_size, uint32_t transfer_id) {
    dynamic_cast<ExecutorLayer*>(executor_layer)->onAddressTransfer(transfer_note, transfer_size, transfer_id);
}

uint32_t ThreadExecutor::onTransmitThread() {
    return dynamic_cast<ExecutorLayer*>(executor_layer)->onTransmitThread();
}

void ThreadExecutor::unTransmitThread(WordThread *thread) {
    dynamic_cast<ExecutorLayer*>(executor_layer)->unTransmitThread(dynamic_cast<SocketWordThread*>(thread->getWord()));
}

void ThreadExecutor::onTransmitThread(WordThread *thread, uint32_t transmit_id){
    dynamic_cast<ExecutorLayer*>(executor_layer)->onTransmitThread(dynamic_cast<SocketWordThread*>(thread->getWord()), transmit_id);
}

void ThreadExecutor::onDisplayThread(WordThread *thread) {
    dynamic_cast<ExecutorLayer*>(executor_layer)->onDisplayThread(dynamic_cast<DisplayWordThread*>(thread->getWord()));
}

void ThreadExecutor::onSubmitNote(MeetingAddressNote *note, uint32_t transmit_id) {
    dynamic_cast<ExecutorLayer*>(executor_layer)->onSubmitNote(note, transmit_id);
}

void ThreadExecutor::onUnloadNote(MeetingAddressNote *note, uint32_t transmit_id) {
    dynamic_cast<ExecutorLayer*>(executor_layer)->onUnloadNote(note, transmit_id);
}

void ThreadExecutor::onTransferComplete(WordThread *socket_thread) {
    loop_thread->second->receiveInstruct(Instruct::makeInstruct<TransferCompleteInstruct>(socket_thread));

}

//该函数运行在Loop线程上
void ThreadExecutor::onTransmitNote(ExecutorNoteInfo *executor_note_info) {
    LoopWordThread::onTransmitThread(dynamic_cast<WordThread*>(loop_thread->second)->getWord(), executor_note_info);
}

std::future<TransmitThreadUtil*> ThreadExecutor::submitNote(MeetingAddressNote *address, const std::function<void(MeetingAddressNote*)> &note_func) {
    auto join_instruct = Instruct::makeInstruct<AddressJoinInstruct>(INSTRUCT_ADDRESS_JOIN, loop_thread->second, THREAD_TYPE_LOOP, address);
    join_instruct->join_func = [&](TransmitThreadUtil *util, uint32_t transmit_id) -> void {
        onSubmitNote(address, transmit_id);
        join_instruct->join_promise.set_value(util);
    };
    join_instruct->note_func = note_func;

    manager_thread->receiveInstruct(std::static_pointer_cast<Instruct>(join_instruct));
    return join_instruct->join_promise.get_future();
}

void ThreadExecutor::exitNote(MeetingAddressNote *address, const std::function<void(MeetingAddressNote*)> &note_func) {
    auto exit_instruct = Instruct::makeInstruct<AddressExitInstruct>(INSTRUCT_ADDRESS_EXIT, loop_thread->second, address);
    exit_instruct->exit_func = [&](uint32_t transmit_id) -> void{
        onUnloadNote(address, transmit_id);
    };
    exit_instruct->note_func = note_func;
    manager_thread->receiveInstruct(std::static_pointer_cast<Instruct>(exit_instruct));
}

void ThreadExecutor::requestLaunch(ExecutorStatus status) {
    std::promise<void> launch_promise;
    startThread(launch_promise, status);
    launch_promise.get_future().get();
}

void ThreadExecutor::requestShutDown(std::promise<void> shut_down_promise) {
    int future_pos = 0;
    std::future<void> stop_future[word_threads.size() + 1];

    for(auto begin : word_threads){
        if(begin.first != THREAD_TYPE_SOCKET){
            stop_future[future_pos++] = shutDownThread(begin.second);
        }
    }
    stop_future[future_pos++] = shutDownThread(static_cast<BasicThread*>(manager_thread));

    waitThreadShutDown(stop_future, &shut_down_promise, future_pos);
}

std::future<void> ThreadExecutor::shutDownThread(BasicThread *thread) {
    auto stop_instruct = Instruct::makeInstruct<StopInstruct>(INSTRUCT_STOP, nullptr);
    thread->receiveInstruct(stop_instruct);
    return std::dynamic_pointer_cast<StopInstruct>(stop_instruct)->stop_promise.get_future();
}

void ThreadExecutor::waitThreadShutDown(std::future<void> *stop_futures, std::promise<void> *shut_down_promise, int future_size) {
    for(int i = 0; i < future_size; i++){
        try {
            (stop_futures + i)->get();
        }catch (std::future_error &e){
            std::cout << "thread shut down completes!" << std::endl; continue;
        }
    }

    shut_down_promise->set_value();
}

void ThreadExecutor::requestTermination() {
    closeThread();
}

BasicThread* ThreadExecutor::onCreateThread(const ThreadProxy &thread_proxy) {
    BasicThread *thread = nullptr;
    try {
        thread_proxy.onCreate(this, (thread = thread_proxy.createThread(this)));
    }catch (std::bad_alloc &e){
        std::cout << "ThreadExecutor:onCreateThread->fail!" << std::endl;
    }

    return thread;
}

void ThreadExecutor::onCreateThread(ManagerThread *manager_thread) {
    /*
     * manger_thread不会等于nullptr
     * 因为std::get<0>(thread_proxy) == ManagerProxy
     * 所以最先创建ManagerThread并赋值给manager_thread
     */
    this->manager_thread = manager_thread;
}

void ThreadExecutor::onCreateThread(ExecutorThread *executor_thread) {
    if(executor_thread) {
        /*
         * manger_thread不会等于nullptr
         * 因为std::get<0>(thread_proxy) == ManagerProxy
         * 所以最先创建ManagerThread并赋值给manager_thread
         */
        executor_thread->correlateManagerThread(manager_thread);
        idle_executor_threads.push_back(executor_thread);
    }
}

void ThreadExecutor::onCreateThread(WordThread *word_thread) {
    if(word_thread) {
        /*
         * manger_thread不会等于nullptr
         * 因为std::get<0>(thread_proxy) == ManagerProxy
         * 所以最先创建ManagerThread并赋值给manager_thread
         */
        word_thread->correlateThreadManager(manager_thread);
        word_threads.insert({word_thread->type(), word_thread});
    }
}

void ThreadExecutor::onCloseThread(const ManagerProxy &manager_proxy) {
    if(manager_thread){
        manager_thread->receiveInstruct(Instruct::makeInstruct<FinishInstruct>(INSTRUCT_FINISH, nullptr));
        manager_thread->join();
        manager_proxy.destroyThread(static_cast<BasicThread*>(manager_thread));
    }
}

void ThreadExecutor::onCloseThread(const ExecutorProxy &executor_proxy) {
    std::shared_ptr<Instruct> finish_instruct = Instruct::makeInstruct<FinishInstruct>(INSTRUCT_FINISH, nullptr);

    for(auto begin : idle_executor_threads){
        begin->receiveInstruct(finish_instruct);
        dynamic_cast<ExecutorThread*>(begin)->onWaitUp();
        begin->join();
        executor_proxy.destroyThread(begin);
    }
}

void ThreadExecutor::onCloseThread(const WordProxy &word_proxy) {
    std::shared_ptr<Instruct> finish_instruct = Instruct::makeInstruct<FinishInstruct>(INSTRUCT_FINISH, nullptr);

    for(auto begin : word_threads){
        begin.second->receiveInstruct(finish_instruct);
        begin.second->join();
        word_proxy.destroyThread(begin.second);
    }

    word_threads.clear();
}

BasicThread* ThreadExecutor::onRequestThread(const ExecutorProxy &executor_proxy) {
    BasicThread *request_thread = nullptr;

    if(idle_executor_threads.empty()){
        startThread(onCreateThread(executor_proxy));
    }

    if((request_thread = idle_executor_threads.front())){
        idle_executor_threads.pop_front();
    }else{
        request_thread = onRequestThread(executor_proxy);
    }

    return request_thread;
}

BasicThread* ThreadExecutor::onRequestThread(const WordProxy &word_proxy) {
    BasicThread *request_thread = nullptr;
    if((request_thread = onCreateThread(word_proxy))) {
        startThread(request_thread);
    }
    return request_thread;
}

void ThreadExecutor::onResponseThread(ExecutorThread *response_thread) {
    response_thread->onWaitUp();
}

void ThreadExecutor::onRecoveryThread(ExecutorThread *recover_thread) {
    std::shared_ptr<ScopeInstruct> instruct = Instruct::makeInstruct<ScopeInstruct>(INSTRUCT_SCOPE, nullptr);

    instruct->pool = this;
    recover_thread->receiveInstruct(instruct);

    idle_executor_threads.push_back(recover_thread);
}

void ThreadExecutor::onRecoveryThread(WordThread *recovery_thread) {
    decltype(word_threads.end()) recovery_iterator;

    if((recovery_iterator = word_threads.find(recovery_thread->type())) != word_threads.end()) {
        if((recovery_iterator = std::find_if(recovery_iterator, word_threads.end(),
                                             [&](std::pair<ThreadType, BasicThread*> value) -> bool {
                                                return (value.second == recovery_thread);
                                             })) != word_threads.end()){
            recovery_thread->receiveInstruct(Instruct::makeInstruct<FinishInstruct>(INSTRUCT_FINISH, nullptr, ThreadExecutor::onRecoveryFunc));
            recovery_thread->detach();
            word_threads.erase(recovery_iterator);
        }
    }
}

void ThreadExecutor::onRecoveryFunc(BasicThread *recovery_thread) {
    try {
        ErgodicProxy<thread_proxy>::getProxy(core_proxy, recovery_thread->type()).destroyThread(recovery_thread);
    }catch (ThreadProxyException &e){
        std::cout << "onRecoveryFunc->" << e.what() << std::endl;
    }
}
