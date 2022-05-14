//
// Created by abc on 20-4-12.
//
#include "LoopWordThread.h"

std::chrono::steady_clock::time_point LoopWordThread::select_time_point = std::chrono::steady_clock::now();

LoopWordThread::LoopWordThread(WordThread *wthread, bool can_transfer) throw(std::runtime_error)
        : Word(wthread), is_transfer_thread(can_transfer), select_size(0), interrupt_size(0), sequence_time_task(nullptr),
          socket_transfer_info(nullptr), socket_time_tasks(), insert_task_queue(), socket_join_heap(), time_correlate(this) {
    //初始化LoopThread线程
    if(initLoopWordThread()){
        //设置用于中断的管道
        FD_SET(task_interrupt[1], &wset);
        //设置SocketThread线程二叉堆的排序函数
        socket_join_heap.setPercolateFunc((void(*)(SocketWordInfo*,int))&SocketWordInfo::updateSocketWordInfo);
    }else{
        //关闭LoopThread线程
        closeLoopWordThread();
        //抛出异常
        throw std::runtime_error("LoopWordThread::LoopWordThread(WordThread*) structure fail！");
    }
}

LoopWordThread::~LoopWordThread() {
    closeLoopWordThread();
}

/**
 * 初始化LoopThread函数
 *  打开用于中断的管道（socketpair）
 * @return
 */
bool LoopWordThread::initLoopWordThread() {
    return (!socketpair(AF_UNIX, SOCK_STREAM, 0, task_interrupt) && word_thread);
}

/**
 * 关闭LoopThead函数
 *  关闭用于中断的管道（socketpair）
 */
void LoopWordThread::closeLoopWordThread() {
    close(task_interrupt[0]);
    close(task_interrupt[1]);
}

/**
 * LoopThread线程运行函数
 * 有四种情况唤醒正在阻塞等待的LoopThread线程
 *  1：任务超时
 *  2：处理指令（中断）
 *  3：接收任务（中断）
 *  3：处理socket消息（需要校正序号）
 */
void LoopWordThread::exe() {
    //获取当前中断点
    onUpdateTimePoint();
    //获取中断数量
    updateInterrupt();
    //更新任务时间
    updateTime();
    //处理唤醒阻塞的描述符
    if(select_size > 0){
        //中断处理（处理任务）
        updateInsert();
        //校正序号
        correctionSequence();
        //SocketThread线程处理
        updateSocket();
    }

    //阻塞线程
    onSelect();
}

/**
 * 中断LoopThread线程（处理指令或接收任务）
 */
void LoopWordThread::interrupt() {
    //调用中断
    interruptLoop();
}

/**
 * 接收单个任务
 * @param push_task 任务
 */
void LoopWordThread::onPush(std::shared_ptr<ExecutorTask> push_task) {
    if(push_task != nullptr) {
        try {
            //插入队列
            insert_task_queue.push(std::move(std::dynamic_pointer_cast<LoopExecutorTask>(push_task)));
            //中断
            interruptLoop();
        }catch (std::bad_alloc &e){
            std::cout << "LoopWordThread::onPush() bad_alloc!" << std::endl;
        }
    }
}

/**
 * 接收单个note
 * @param executor_note 执行单元
 */
void LoopWordThread::onExecutor(std::shared_ptr<ExecutorNote> executor_note) {
    //交给MangerThread线程处理（由其调度ExecutorThread线程执行）
    threadManager()->receiveTask(std::move(executor_note));
}

/**
 * LoopThread线程处理指令
 * @param instruct
 */
void LoopWordThread::onInterrupt(SInstruct instruct) throw(finish_error){
    switch (instruct->instruct_type){
        //LoopThread线程接收远程端的加入指令
        case INSTRUCT_ADDRESS_JOIN:
            onJoin(std::dynamic_pointer_cast<AddressJoinInstruct>(instruct));
            break;
        //LoopThread线程接收远程端退出的指令
        case INSTRUCT_ADDRESS_EXIT:
            onExit(std::dynamic_pointer_cast<AddressExitInstruct>(instruct));
            break;
        case INSTRUCT_ADDRESS_COMPLETE:
            onTransferComplete(std::dynamic_pointer_cast<TransferCompleteInstruct>(instruct));
            break;
        //ManagerThread线程响应LoopThread线程的请求SocketThread线程的指令
//        case RESPONSE_THREAD:
//            onObtain(std::dynamic_pointer_cast<AddressJoinInstruct>(instruct));
//            break;
        case EXCEPTION_SOCKET_RELEASE:
            onExceptionSocket(std::dynamic_pointer_cast<ExceptionSocketInstruct>(instruct));
            break;
        //释放SocketThread线程的指令
        case REQUEST_SOCKET_RELEASE:
            onReleaseSocket(std::dynamic_pointer_cast<ReleaseSocketInstruct>(instruct));
            break;
        //SocketThread线程完成释放的指令
        case RESPONSE_SOCKET_RELEASE:
            onRecoverySocket(std::dynamic_pointer_cast<ReleaseSocketInstruct>(instruct));
            break;
        //LoopThread线程启动指令
        case INSTRUCT_START:
            onStart(std::dynamic_pointer_cast<StartInstruct>(instruct));
            break;
        //LoopThread线程停止指令
        case INSTRUCT_STOP:
            onStop(std::dynamic_pointer_cast<StopInstruct>(instruct));
            break;
        //LoopThread线程结束指令
        case INSTRUCT_FINISH:
            onFinish(std::dynamic_pointer_cast<FinishInstruct>(instruct));
            break;
        default:
            break;
    }
}

void LoopWordThread::onLoop() {
    //不处理
}

void LoopWordThread::onTransmitThread(Word *thread, ExecutorNoteInfo *transmit_note_info) {
    uint32_t socket_pos = 0;
    auto *loop_thread = dynamic_cast<LoopWordThread*>(thread);
    BasicThread *sockets[loop_thread->socket_time_tasks.size()];

    for(auto socket_thread : loop_thread->socket_time_tasks){
        sockets[socket_pos++]= socket_thread.first;
    }

    transmit_note_info->callExecutorFunc([&](MsgHdr *msg) -> void {
        auto note_instruct = Instruct::makeInstruct<TransmitNoteInstruct>(INSTRUCT_SOCKET_TRANSMIT_NOTE, nullptr, socket_pos);
        note_instruct->note_msg = msg;
        note_instruct->transmit_func = [&](TransmitThreadUtil *thread_util, MeetingAddressNote *note, MsgHdr *msg, uint32_t note_size) -> void {
            transmit_note_info->callNoteTransmitFunc(thread_util, note, msg, note_size);
        };
        note_instruct->release_func = [&]() -> void {
            transmit_note_info->callNoteCompleteFunc();
        };

        for(; socket_pos <= 0; socket_pos--){
            (note_instruct->instruct_thread = sockets[socket_pos])->receiveInstruct(note_instruct);
        }
    });
}

/**
 * 处理新增的任务
 */
void LoopWordThread::updateInsert() {
    //判断是否调用中断
    if(FD_ISSET(task_interrupt[1], &rset)) {
        //判断是否有新增的任务
        while (!insert_task_queue.empty()) {
            //接收任务
            insertLoopTask(insert_task_queue.pop());
        }
    }
}

/**
 * 处理socket消息
 *  需要先校正sequence序号
 */
void LoopWordThread::updateSocket() {
    SocketWordThread *socket_thread = nullptr;
    //判断是否socket消息
    for(auto socket_task : socket_time_tasks){
        if((socket_thread = (dynamic_cast<SocketWordThread*>(originalWordThread(socket_task.first))))
           && FD_ISSET(socket_thread->getSocketExecutorTask()->getSocketFd(), &rset)){
            //生成SocketThread线程的消息note并执行
            socket_task.first->receiveTask(socket_thread->getSocketExecutorTask()->executor());
        }
    }
}

/**
 * 获取处理中断后的其他线程调用中断的数量
 *  防止忽略在处理完中断指令后与阻塞前的中断指令调用的中断调用
 */
void LoopWordThread::updateInterrupt() {
    if(ioctl(task_interrupt[1], FIONREAD, &interrupt_size) < 0){
        interrupt_size = 0;
    }
}

/**
 * 更新LoopThread线程所有任务的时间
 */
void LoopWordThread::updateTime() {
    time_correlate.updateTimeTask();
}

/**
 * 校正序号（序号任务）
 *  time ---- stime ----------- ntime
 *       |t1|       |    t2    |
 *  -> 从处理中断后(select_time_point,用于更新序号任务)和处理socket消息前(sequence_time_point)所消耗时间（t1）会影响序号(sequence)的精度,
 *     所有需要将该时间差再次更新序号任务
 *  -> 其他任务的时间不需要高精度,该时间差会包含再下一次的更新的时间内
 *     下一次更新时间时,序号任务只会更新t2的时间,而其他任务更新t1+t2的时间
 */
void LoopWordThread::correctionSequence() {
    if(sequence_time_task) {
        onUpdateTimePoint();
        time_correlate.timeTaskUpdate(sequence_time_task);
    }
}

/**
 * 选择阻塞
 *  -> push的中断信号可能会丢失(该任务要等到下一轮才会处理)
 *  LoopWordThread :      updateInsert   ------------------------------------> onSelect
 *  其他线程        :                        push(task) -->  interruptLoop（因为interrupt_size存在,不会忽略该中断请求）
 */
void LoopWordThread::onSelect() {
    //获取LoopThread线程内的任务最小触发时间
    bool skip_select = false;
    timeval min_time{0, 0};

    try {
        min_time = minLoopTime();
    } catch (task_trigger_error &e){
        //最小任务触发时间小于用于处理中断的时间,在select()前最小任务应该在触发状态,所以直接跳过阻塞等待,而执行下一轮
        skip_select = true;
    }
    //复位中断
    resetInterrupt();
    //复位文件描述符
    resetFdSet();

    //阻塞线程
    if(!skip_select) {
        select_size = select(FD_SETSIZE + 1, &rset, nullptr, nullptr, (min_time.tv_usec <= 0) ? nullptr : &min_time);
    }else{
        //用于更新序号任务时间
        select_size = 1;
    }
}

/**
 * 获取LoopThread线程内的任务最小触发时间
 * @return  触发时间
 */
timeval LoopWordThread::minLoopTime() throw(task_trigger_error){
//    无异常函数过程,无法判断最小任务时间是否处于触发状态
//    if(task_time <= 0){
//        //没有其他任务,选择序号任务
//        min_time = sequence_time;
//    }else if(sequence_time <= 0){
//        //没有序号任务,选择其他任务
//        min_time = task_time;
//    }else{
//        //都有,选择的最小触发时间
//        min_time = std::min<uint64_t>(task_time, sequence_time);
//    }
//    return {0, min_time};
//          ||
//          ||
//          \/
//    有异常函数过程,如果最小任务时间触发时间小于当前时间,则minTime()和minTime(sequence_time_task)会抛出task_trigger_error,
//    该函数会重新抛出
//    int64_t min_time = 0, task_time = time_correlate.minTime(),
//            sequence_time = sequence_time_task ? time_correlate.minTime(sequence_time_task) : 0;
//    min_time = std::min<int64_t>(task_time, sequence_time);
//    return {0, min_time};
//          ||
//          ||简化过程
//          \/
    //从序号任务获取最小触发时间(精确) 和 其他任务获取最小触发时间(忽略时间差) 的最小值
    return {0, std::min<int64_t>(time_correlate.minTime(), sequence_time_task ? time_correlate.minTime(sequence_time_task) : INT64_MAX)};
}


/**
 * 中断LoopThread线程(其他线程调用)
 */
void LoopWordThread::interruptLoop() {
    int res = 0;
    int buf[1] = {1};
    do{
        res = static_cast<int>(write(task_interrupt[0], buf, 1));
    }while((res == -1) && (errno == EINTR));
}

/**
 * 复位LoopThread线程中断(LoopWordThread调用该函数)
 */
void LoopWordThread::resetInterrupt() {
    int res;

    char buf[interrupt_size];
    do {
        res = static_cast<int>(read(task_interrupt[1], buf, static_cast<size_t>(interrupt_size)));
    }while((res == -1) && (errno == EINTR));
}

/**
 * 向LoopThread线程插入任务
 * @param task
 */
void LoopWordThread::insertLoopTask(std::shared_ptr<LoopExecutorTask> task) {
    try {
        //关联当前时间点
        task->setTaskGenerateTime(select_time_point);
        //插入到时间关联器
        time_correlate.insertTimeTask(std::move(task));
    }catch (std::bad_alloc &e){
        std::cout << "LoopWordThread::insertLoopTask() bad_alloc!" << std::endl;
    }
}

/**
 * 向ManagerThread线程请求SocketThread线程,该操作为原子性,不存在LoopThread线程同时获取两次线程(即以下情况)
 *  LoopThread          --> 请求线程(指令1)   --> 请求线程(指令2)
 *  ManagerThread                                               --> 指令1(线程1)  --> 指令2(线程2)
 *
 *  1.由于SocketThread线程无法接收远程端（满载）,需要新的SocketThread线程来接收远程端）（由新的远程端加入而调用该函数）
 *  2.SocketThread线程释放时,会转移远程端而现有的SocketThread无法接收
 */
void LoopWordThread::obtainSocketThread() {
    std::future<BasicThread*> obtain_future;
    //创建指令
    SInstruct instruct = Instruct::makeInstruct<ThreadInstruct>(REQUEST_THREAD, word_thread, THREAD_TYPE_SOCKET);
    obtain_future = std::dynamic_pointer_cast<ThreadInstruct>(instruct)->thread_promise.get_future();
    //发送指令
    threadManager()->receiveInstruct(instruct);

    onObtain(obtain_future.get());
}

/**
 * 接收获取的SocketThread线程
 * @param socket_thread 线程
 * @return
 */
bool LoopWordThread::insertSocketTask(WordThread *socket_thread){
    //获取SocketThread线程的描述符
    int socket_fd = (dynamic_cast<SocketWordThread*>(originalWordThread(socket_thread)))->getSocketExecutorTask()->getSocketFd();
    std::pair<std::map<WordThread*, SocketWordInfo>::iterator, bool> insert_pair = {socket_time_tasks.end(), false};

    //SocketThread线程是否打开socket 且 是否已经存在该线程 且 是否接收成功
    if((socket_fd >= 0) && !FD_ISSET(socket_fd, &wset)
       && (insert_pair = socket_time_tasks.insert({socket_thread, SocketWordInfo(socket_time_tasks.end())})).second){
        insert_pair.first->second.socket_iterator = insert_pair.first;
        //将SocketThread线程添加到二叉堆
        try {
            socket_join_heap.insert(&insert_pair.first->second);
        }catch (std::bad_alloc &e){
            insert_pair.second = false;
            socket_time_tasks.erase(insert_pair.first);
        }

        //插入map和二叉堆成功
        if(insert_pair.second) {
            //设置阻塞描述符
            FD_SET(socket_fd, &wset);
            //关联LoopThread线程
            (dynamic_cast<SocketWordThread *>(originalWordThread(socket_thread)))->correlateLoopWordThread(word_thread);
        }
    }

    return insert_pair.second;
}

/**
 * 移除SocketThread线程
 * @param socket_thread 线程
 */
void LoopWordThread::removeSocketTask(WordThread *socket_thread) {
    //获取SocketThread线程的描述符
    int socket_fd = (dynamic_cast<SocketWordThread*>(originalWordThread(socket_thread)))->getSocketExecutorTask()->getSocketFd();
    auto erase_task = socket_time_tasks.end();

    //SocketThread线程是否打开socket 且 是否存在该线程
    if((socket_fd >= 0) && FD_ISSET(socket_fd, &wset) &&
            ((erase_task = socket_time_tasks.find(socket_thread)) != socket_time_tasks.end())){
        erase_task->second.heap_rank = -1;
        socket_join_heap.increase(erase_task->second.heap_pos);

        //移除SocketThread线程二叉堆
        socket_join_heap.remove();
        //移除SocketThread线程任务
        socket_time_tasks.erase(erase_task);
        //复位阻塞描述符
        FD_CLR(socket_fd, &wset);
    }
}

/**
 * 处理远程端的加入
 * @param instruct 指令
 */
void LoopWordThread::onJoin(std::shared_ptr<AddressJoinInstruct> instruct) {
    BasicThread *thread = nullptr;
    SocketWordThread *socket_thread = nullptr;

    if(instruct->join_note) {
        /*
         * 循环加入SocketThread线程,直到加入成功为止
         *  失败：创建SocketThread线程
         */
        for (;;) {
            if((thread = onJoinAddress(instruct->join_note))){
                break;
            }
            obtainSocketThread();
        }

        //加入成功
        socket_thread = dynamic_cast<SocketWordThread*>(dynamic_cast<WordThread*>(thread)->getWord());
        //调用回调函数
        instruct->note_func(instruct->join_note);
        //将线程池的加入函数在SocketThread线程上运行
        socket_thread->onRunSocketThread(std::bind(instruct->join_func, socket_thread->correlateTransmit(), socket_thread->correlateTransmitId()), SOCKET_FUNC_TYPE_IMMEDIATELY);
    }
}

/**
 * 处理远程端的退出
 * @param instruct 指令
 */
void LoopWordThread::onExit(std::shared_ptr<AddressExitInstruct> instruct) {
    auto socket_thread = dynamic_cast<SocketWordThread*>(dynamic_cast<WordThread*>(instruct->instruct_thread)->getWord());
    auto exit_task = socket_time_tasks.find(dynamic_cast<WordThread*>(instruct->instruct_thread));

    if(exit_task != socket_time_tasks.end()){
        //释放许可
        socket_thread->releasePermit(instruct->exit_note);
        //运行线程池的退出函数在SocketThread线程上运行
        socket_thread->onRunSocketThread(std::bind(instruct->exit_func, socket_thread->correlateTransmitId()), SOCKET_FUNC_TYPE_IMMEDIATELY);
        //更新SocketThread线程二叉堆
        exit_task->second.heap_rank--;
        socket_join_heap.reduce(exit_task->second.heap_pos);

        if(!is_transfer_thread) {
            //没有存在转移或转移已经完成,请求转移
            onTransfer();
        }

        //调用回调函数
        instruct->note_func(instruct->exit_note);
    }
}

/**
 * 转移SocketThread线程关联的远程端note（运行在LoopThread线程上）
 *  获取关联远程端note最小的SocketThread线程释放,其他SocketThread接收被转移
 */
void LoopWordThread::onTransfer() {
    //获取最小关联远程端note的线程关联的note数量
    int thread_size = socket_join_heap.get()->heap_rank,
    //获取最小关联远程端的线程序号
        thread_pos = socket_join_heap.get()->heap_pos;

    //如果最小远程端的线程的远程端数量大于SOCKET_PERMIT_DEFAULT_SIZE的一半(4),则不需要转移
    if(thread_size > (SOCKET_PERMIT_DEFAULT_SIZE >> 1)){
        return;
    }

    //遍历所有的SocketThread线程（除被转移的SocketThread线程）
    for(int i = 1, receive_size = 0; i <= socket_time_tasks.size(); i++){
        //当前线程是被转移的线程,跳过
        if(i == thread_pos){
            continue;
        }

        //增加接收转移线程的数量
        receive_size++;

        /*
         * 其他SocketThread线程是否能接收所有被转移的远程端note
         *  能   :  确定转移,发送SocketThread线程释放请求
         *  不能 ：  不需要转移,因为释放SocketThread线程之后又需要创建（耗时且无用）
         */
        if((thread_size -= (SOCKET_PERMIT_DEFAULT_SIZE - socket_join_heap[i]->heap_rank)) <= 0){
            //设置转移标志
            is_transfer_thread = true;
            //响应SocketThread线程的释放
            response(Instruct::makeInstruct<ReleaseSocketInstruct>(REQUEST_SOCKET_RELEASE, socket_join_heap.get()->socket_iterator->first, receive_size));
            break;
        }
    }

}

/**
 * SocketThread线程完成接收被转移的note（运行在LoopThread线程上）
 * @param instruct
 */
void LoopWordThread::onTransferComplete(std::shared_ptr<TransferCompleteInstruct> instruct) {
    //确定完成被转移
    if(socket_transfer_info->onSocketTransfer(instruct->instruct_thread)){
        instruct->instruct_type = RESPONSE_SOCKET_TRANSFER;
        //发送SocketThread线程
        instruct->instruct_thread->receiveInstruct(instruct);
    }

    //是否所用线程都完成
    if(socket_transfer_info->isCompleteTransfer()){
        //复位转移标志
        is_transfer_thread = false;
        //释放接收被转移线程信息类
        socket_transfer_info = onDestroyThreadTransferInfo(socket_transfer_info);
    }
}

/**
 * 处理获取SocketThread线程（运行在LoopThread线程上）
 *  逻辑循环（获取->回收->获取）： 当所有SocketThread线程因满载而不能接收note时,会向线程池获取SocketThread线程,
 *      获取的SocketThread线程关联LoopThread线程失败时,会回收获取的SocketThread线程,从而重新因不能接收note而获取SocketThread线程。
 *  循环停止（运行一定时间内）  ： 因为不能接收的note会被LoopThread线程关联的SocketThread线程因消耗内部的note而接收外部的note,
 *      当所有不能因满载而不能接收的note被SocketThread全部接收完毕时,会停止循环。
 *  处理方法：由时间去解决
 * @param instruct SocketThread线程
 */
void LoopWordThread::onObtain(BasicThread *thread) {
    //创建指令
    SInstruct instruct = Instruct::makeInstruct<StartInstruct>(INSTRUCT_START, nullptr, nullptr, nullptr, false);

    if(insertSocketTask(dynamic_cast<WordThread*>(thread))){
        //关联成功,启动线程
        thread->receiveInstruct(instruct);
    }else{
        //关联失败,回收线程
        instruct->instruct_type = INSTRUCT_RECOVERY;
        threadManager()->receiveInstruct(instruct);
    }

//    if(instruct->instruct_thread){
//        //获取线程,复位获取线程变量
//        is_obtain_thread = false;
//
//        if(insertSocketTask(dynamic_cast<WordThread*>(instruct->instruct_thread))){
//            //关联成功,启动线程
//            instruct->instruct_thread->receiveInstruct(Instruct::makeInstruct<StartInstruct>(INSTRUCT_START, nullptr, nullptr, nullptr, false));
//            onJoin(instruct);
//        }else{
//            //关联失败,回收Socket线程
//            instruct->instruct_type = INSTRUCT_RECOVERY;
//            threadManager()->receiveInstruct(instruct);
//        }
//    }
}

/**
 * 处理单个远程端加入（运行在LoopThread线程上）
 * @param note 远程端
 * @return     加入的SocketThread线程
 */
BasicThread* LoopWordThread::onJoinAddress(MeetingAddressNote *note) {
    //获取远程端实例最小的SocketThread线程
    SocketWordInfo *socket_word_info = socket_join_heap.get();
    WordThread *join_thread = socket_word_info->socket_iterator->first;

    //是否能够在SocketThread线程内获取许可
    if((dynamic_cast<SocketWordThread*>(originalWordThread(join_thread))->acquirePermit(note))){
        socket_word_info->heap_rank++;
        socket_join_heap.reduce(socket_word_info->heap_pos);
    }else{
        join_thread = nullptr;
    }

    return join_thread;
}

/**
 * SocketThread线程异常释放（运行在LoopThread线程上）
 * @param exception_instruct
 */
void LoopWordThread::onExceptionSocket(std::shared_ptr<ExceptionSocketInstruct> exception_instruct) {
    onReleaseSocket0(std::static_pointer_cast<Instruct>(exception_instruct), true);
    exception_instruct->exception_promise.set_value();

}

/**
 * 处理SocketThread线程的释放（运行在LoopThread线程上）
 * @param release_instruct 指令
 */
void LoopWordThread::onReleaseSocket(std::shared_ptr<ReleaseSocketInstruct> release_instruct) {
    if(onReleaseSocket0(std::static_pointer_cast<Instruct>(release_instruct), false)){
        //响应SocketThread线程的释放
        response(release_instruct);
    }
}

/**
 * SocketThread线程释放的实现函数
 * @param release_instruct  指令
 * @param force             是否强制释放
 *                              是：LoopThread线程接收到释放指令,会强制释放所有的SocketThread函数
 *                              否：SocketThread线程因某些原因而发送释放指令,LoopThread线程可不处理释放
 */
bool LoopWordThread::onReleaseSocket0(std::shared_ptr<Instruct> release_instruct, bool force) {
    auto word_thread = dynamic_cast<WordThread*>(release_instruct->instruct_thread);
    auto release_iterator = socket_time_tasks.find(word_thread);

    if(release_iterator != socket_time_tasks.end()){
        //获取SocketThread线程的远程端数量
        int socket_thread_rank = release_iterator->second.heap_rank;

        //设置满载
        release_iterator->second.heap_rank = SOCKET_PERMIT_DEFAULT_SIZE;
        socket_join_heap.reduce(release_iterator->second.heap_pos);

        /*
         * 判断是否能够接收释放SocketThread线程内的远程端
         *  1.非强制
         *  2.LoopTread线程有其他的SocketThread线程（SocketThread线程数量 > 1）
         *  3.其他的SocketThread线程能够接收远程端
         */
        if(!force && (socket_time_tasks.size() <= LOOP_CORRELATE_SOCKET_INIT_VALUE)
           && dynamic_cast<SocketWordThread*>(originalWordThread(socket_join_heap.get()->socket_iterator->first))->verifyFullLoad()){
            //不能释放SocketThread线程,复位远程源信息
            release_iterator->second.heap_rank = socket_thread_rank;
            socket_join_heap.increase(release_iterator->second.heap_pos);
            return false;
        }else{
            std::cout << "LoopWordThread::onReleaseSocket0->" << std::endl;
            return true;
        }
    }
    return false;
}

/**
 * 回收SocketThread线程（LoopThread线程已经响应SocketThread的释放）
 * @param release_instruct 指令
 */
void LoopWordThread::onRecoverySocket(std::shared_ptr<ReleaseSocketInstruct> release_instruct) {

    //移除SocketThread线程
    removeSocketTask(dynamic_cast<WordThread*>(release_instruct->instruct_thread));

    //等待SocketThread线程停止
    onStopSocket(LOOP_CORRELATE_SOCKET_INIT_VALUE,
                 [&](BasicThread*) -> BasicThread* {
                    return release_instruct->instruct_thread;
                 }).get();

    //向ManagerThread线程回收SocketThread线程
    release_instruct->instruct_type = INSTRUCT_RECOVERY;
    threadManager()->receiveInstruct(std::static_pointer_cast<Instruct>(release_instruct));

    //需要转移远程端,重新获取SocketThread线程
    if(!release_instruct->release_notes.empty()){
        auto receive_func = [&](int thread_size) -> void {
            int receive_total_size = 0;
            BasicThread *receive_thread = nullptr;
            SInstruct receive_instruct_array[thread_size];
            std::shared_ptr<TransferSocketInstruct> transfer_socket_instruct;
            std::fill(receive_instruct_array, receive_instruct_array + thread_size, nullptr);

            for(auto begin = release_instruct->release_notes.begin(), end = release_instruct->release_notes.end(); begin != end;){
                if(!(receive_thread = onJoinAddress(*begin))){
                    obtainSocketThread();
                    continue;
                }

                if(!receive_instruct_array[receive_total_size] || (receive_instruct_array[receive_total_size]->instruct_thread != receive_thread)){
                    receive_total_size++;
                    try {
                        receive_instruct_array[receive_total_size] = Instruct::makeInstruct<TransferSocketInstruct>(REQUEST_SOCKET_TRANSFER, receive_thread);
                        transfer_socket_instruct = std::dynamic_pointer_cast<TransferSocketInstruct>(receive_instruct_array[receive_total_size]);
                    }catch (std::bad_alloc &e){
                        if(receive_total_size <= 0){
                            is_transfer_thread = true;
                        }
                        break;
                    }
                }

                transfer_socket_instruct->transfer_note[transfer_socket_instruct->transfer_size++] = *begin;
                ++begin;
            }

            if(receive_total_size <= 0){
                return;
            }

            try {
                socket_transfer_info = onCreateThreadTransferInfo(receive_total_size);

                for(int i = 0; i < receive_total_size; i++){
                    receive_instruct_array[i]->instruct_thread->receiveInstruct(receive_instruct_array[i]);
                    socket_transfer_info->initSocketTransfer(receive_instruct_array[i]->instruct_thread);
                }
            }catch (std::bad_alloc &e){
                //如果没内存,会忽略接收转移的SocketThread线程的转移状态
                is_transfer_thread = true;
            }
        };

        receive_func(release_instruct->transfer_thread_size ? release_instruct->transfer_thread_size
                                                            : std::max<int>(socket_time_tasks.size(), release_instruct->release_notes.size()));
    }
}

/**
 * 处理LoopSocket线程的启动
 * @param start_instruct 指令
 */
void LoopWordThread::onStart(std::shared_ptr<StartInstruct> start_instruct) {
    //更改状态
    Word::onExecutor();
    std::cout << "LoopWordThread::onStart->" << std::endl;
    if(start_instruct->instruct_thread){
        //关联SocketThread线程（启动线程池会产生一条SocketThread线程）
        if(!insertSocketTask(dynamic_cast<WordThread*>(start_instruct->instruct_thread))){
            start_instruct->start_fail = true;
        }
    }

    Word::onStartComplete(start_instruct);
}

/**
 * 处理LoopSocket的停止
 * @param stop_instruct 停止
 */
void LoopWordThread::onStop(std::shared_ptr<StopInstruct> stop_instruct) {
    //更新状态
    Word::onStop();
    std::cout << "LoopWordThread::onStop->" << std::endl;

    //停止所有的SocketThread线程
    std::future<void> stop_future = onStopSocket(static_cast<int>(socket_time_tasks.size()),
                                                 [&](BasicThread *thread) -> BasicThread* {
                                                     return thread;
                                                 });

    sequence_time_task = nullptr;
    //清空SocketThread线程
    socket_time_tasks.clear();
    //清空时间关联器（任务）
    time_correlate.clearAll();

    //等待所有的SocketThread线程完成kingship
    stop_future.get();

    //设置停止成功
    stop_instruct->stop_promise.set_value();
    std::cout << "LoopWordThread::onStop->set_value" << std::endl;
}
/**
 * 停止SocketThread线程
 * @param socket_thread
 *          有：指定的SocketThread线程（指定的SocketThread线程已经作出释放的决定并回收相关的内存,只需发送停止指令并等待SocketThread线程停止完毕）
 *          无：所有的SocketThread线程（响应由线程池发送的停止指令,停止所有的SocketThread线程,不需要回收内存,只需发送停止指令并等待SocketThread线程停止完毕）
 * @return
 */
std::future<void> LoopWordThread::onStopSocket(int size, const std::function<BasicThread*(BasicThread*)> &stop_func) {
    std::shared_ptr<StopSocketInstruct> socket_stop_instruct = Instruct::makeInstruct<StopSocketInstruct>(INSTRUCT_STOP, nullptr, size);

    //向指定或所有的SocketThread线程发送停止指令
    for(auto begin = socket_time_tasks.begin(); size > 0 ; size--, ++begin){
        if((socket_stop_instruct->instruct_thread = stop_func(begin->first))){
            response(std::static_pointer_cast<Instruct>(socket_stop_instruct));
        }
    }

    return socket_stop_instruct->stop_promise.get_future();
}

/**
 * 停止LoopThread线程
 */
void LoopWordThread::onFinish(std::shared_ptr<FinishInstruct> finish_instruct) throw(finish_error){
    std::cout << "LoopWordThread::onFinish->" << std::endl;
    Word::onFinish(finish_instruct->finish_callback);
}
