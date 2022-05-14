//
// Created by abc on 20-4-21.
//
#include "../../central/transmit/TransmitDataUtil.h"

SocketFuncMode::SocketFuncMode(SocketWordThread *thread) : executor_func_size(SOCKET_EXECUTOR_FUNC_ACTUAL_SIZE), socket_thread(thread),
                                                           socket_mode_block(){
    std::fill(std::begin(executor_socket_func), std::end(executor_socket_func), nullptr);
}

/**
 * 设置默认型函数（添加）（运行在其他线程上）
 * @param func  函数
 */
void SocketFuncMode::setSocketExecutorFunc(const std::function<void()> &func) {
    setSocketFunc0(func, SOCKET_DEFAULT_FUNC_INTERRUPT_FLAG);
}


/**
 * 设置立即型函数（添加）（运行在其他线程上）
 * @param func  函数
 */
void SocketFuncMode::setSocketImmediatelyFunc(const std::function<void()> &func) {
    setSocketFunc0(func, SOCKET_IMMEDIATELY_FUNC_INTERRUPT_FLAG);
}

/**
 * 存储函数并通知SocketThread线程
 * @param socket_func   执行函数
 * @param func_type     函数类型
 */
void SocketFuncMode::setSocketFunc0(const std::function<void()> &socket_func, int func_type) {
    int func_pos = 0;

    /*
     * 获取存储函数的序号（减少存储函数数量）
     *  <= 0 : 其他线程添加默认型函数满载或添加立即型函数
     *  >  0 : 获取成功,设置函数
     */
    for(;;){
        if((func_pos = onSocketFuncPos([&](int pos) -> int { return pos - 1; })) <= 0){
            try {
                //阻塞等待
                socket_mode_block.onModeLock();
            }catch (std::runtime_error &e){
                /*
                 * 该异常是因为添加函数线程被SocketThread线程onSocketDone函数忽略,阻塞到SocketThread销毁
                 *  SocketFuncMode类的销毁导致onModeLock会抛出异常
                 */
                return;
            }
        }else {
            //设置立即型函数
            executor_socket_func[func_pos - 1] = socket_func;
            break;
        }
    }

    //如果存储函数已满载,则将中断类型设置为立即型
    if((func_pos - 1) <= 0){
        func_type = SOCKET_IMMEDIATELY_FUNC_INTERRUPT_FLAG;
    }

    //按中断类型触发SocketThread线程函数中断
    onSocketInterrupt(func_type);
}

/**
 * SocketThread线程中断函数（运行在其他线程上）
 */
void SocketFuncMode::onSocketInterrupt(int func_type) {
    //中断SocketThread线程
    socket_thread->onInterrupt(func_type);
}

/**
 * 设置函数满载阻塞添加函数
 * @return 存储函数的数量
 */
int SocketFuncMode::onSocketFuncFull() {
    //设置函数满载并返回存储函数的序号
    int func_size = onSocketFuncPos([&](int) -> int { return 0; });
    //转换存储函数数量并返回
    return (SOCKET_EXECUTOR_FUNC_ACTUAL_SIZE - ((func_size < 0) ? 0 : func_size));
}

/**
 * 设置函数数量的函数（运行在其他线程上）
 * @param size_func 存储函数数量的回调函数
 * @return
 */
int SocketFuncMode::onSocketFuncPos(const std::function<int(int)> &size_func) {
    int func_pos = 0, back_pos = 0;

    for(;;){
        //获取当前存储函数的数量
        back_pos = func_pos = executor_func_size.load(std::memory_order_acquire);

        //存储函数已满载,返回
        if(func_pos <= 0){
            return func_pos;
        }

        //比较并设置存储函数的数量
        if(executor_func_size.compare_exchange_weak(func_pos, size_func(func_pos), std::memory_order_release, std::memory_order_relaxed)){
            //返回获取的存储函数的数量
            return back_pos;
        }
    }
}

/**
 * 执行函数（运行在SocketThread线程上）
 */
void SocketFuncMode::onExecutorSocketFunc(int func_size) {
    //判断是否存储有函数
    if(func_size <= 0){
        return;
    }
    //调用实现函数
    onExecutorSocketFunc0(func_size);
}

/**
 * 执行函数实现函数（运行在执行的SocketThread线程上）
 * @param size  需要执行的函数数量（包含是否通知阻塞线程的标志）
 */
void SocketFuncMode::onExecutorSocketFunc0(int size) {
    //设置唤醒标志
    auto is_notify = static_cast<bool>(size & SOCKET_FUNC_MODE_EXECUTOR_NOTIFY_FLAG);
    //设置函数数量
    auto func_size  = static_cast<uint32_t>(size & (~SOCKET_FUNC_MODE_EXECUTOR_NOTIFY_FLAG));
    //存储执行函数数组
    std::function<void()> executor_func[func_size];

    //拷贝默认型函数和立即型函数,拷贝过程如下：
    //|_________________|____func____|
    //                     /
    //                    /
    //                   /
    //                  /
    //                 /
    //                /
    //               /
    //              /
    //             /
    //            /
    //           /
    //          /
    //|__func____|___________________|
    std::copy(std::move_iterator<std::function<void()>*>(executor_func),
              std::move_iterator<std::function<void()>*>(executor_func + func_size),
              executor_socket_func + (SOCKET_EXECUTOR_FUNC_ACTUAL_SIZE - func_size));
//    memcpy(executor_func, executor_socket_func + (SOCKET_EXECUTOR_FUNC_ACTUAL_SIZE - func_size), sizeof(std::function<void()>) * func_size);
//    //初始化函数数组
//    memset(executor_socket_func + (SOCKET_EXECUTOR_FUNC_ACTUAL_SIZE - func_size), 0, sizeof(std::function<void()>) * func_size);

    //设置存储函数数量
    executor_func_size.store(SOCKET_EXECUTOR_FUNC_ACTUAL_SIZE, std::memory_order_release);
    if(is_notify) {
        //唤醒阻塞等待满载函数线程
        socket_mode_block.onModeNotify();
    }

    //SocketThread线程执行函数
    {
        std::function<void()> callfunc = nullptr;

        //遍历默认型函数和立即型函数,直到没有默认型函数为止
        for(int i = 0; i < func_size; i++){
            if(!(callfunc = executor_func[i])){
                break;
            }
            callfunc();
        }
    }
}

/**
 * SocketThread线程释放,执行完所有需要执行的函数
 */
void SocketFuncMode::onSocketDone() {
    //设置函数满载阻塞其他线程添加函数
    int func_size = onSocketFuncFull();
    //执行函数
    onExecutorSocketFunc0(func_size);

    for(; socket_mode_block.onModeLockSize() > 0;) {
        //执行正在阻塞添加线程的函数（不需要唤醒,因为上面已调用唤醒函数）
        onExecutorSocketFunc0(SOCKET_EXECUTOR_FUNC_ACTUAL_SIZE | SOCKET_FUNC_MODE_EXECUTOR_NOTIFY_FLAG);
    }
}

//--------------------------------------------------------------------------------------------------------------------//

std::allocator<SocketPermit> SocketPermitFactory::alloc_ = std::allocator<SocketPermit>();

/**
 * 创建Socket许可
 * @param permit 被创建许可模板
 * @return
 */
SocketPermit* SocketPermitFactory::createPermit(const PermitSocket &permit) {
    SocketPermit *create_permit = nullptr;
    try {
        create_permit = alloc_.allocate(SOCKET_PERMIT_FACTORY_CREATE_DEFAULT_SIZE);
    }catch (std::bad_alloc &e){
        std::cout << "SocketPermitFactory::createPermit->fail!" << std::endl;
    }

    if(create_permit){
        alloc_.construct(create_permit, dynamic_cast<const SocketPermit&>(permit));
    }
    return create_permit;
}

/**
 * 销毁Socket许可
 * @param permit 许可
 */
void SocketPermitFactory::destroyPermit(PermitSocket *permit) {
    SocketPermit *socket_permit = nullptr;
    if((socket_permit = dynamic_cast<SocketPermit*>(permit))) {
        alloc_.destroy(socket_permit);
        alloc_.deallocate(socket_permit, SOCKET_PERMIT_FACTORY_CREATE_DEFAULT_SIZE);
    }
}

//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

SocketWordThread::SocketWordThread(WordThread *thread) : Word(thread), is_socket_transfer(false), transfer_pos(-1), interrupt_gemini(0), loop_thread(nullptr),
                                                         thread_util(nullptr), release_func(nullptr), socket_status(SOCKET_STATUS_INITIALIZE),
                                                         socket_mutex(), socket_condition(), socket_func_mode(this), socket_task(this),
                                                         socket_semaphore(SOCKET_PERMIT_DEFAULT_SIZE, SocketPermitFactory()){
    /*
     * 设置许可释放函数
     */
    release_func = std::bind(&SocketWordThread::permitRelease, this, std::placeholders::_1);
    /*
     * 设置许可回调函数
     */
    socket_semaphore.setCallback(std::bind(&SocketWordThread::onNoneAddress, this));

    for(int i = SOCKET_PERMIT_DEFAULT_SIZE; i >= 0; i--){
        unpermit_pos[i] = (i + 1);
    }
    unpermit_pos[SOCKET_PERMIT_DEFAULT_SIZE] = -1;
}

SocketWordThread::~SocketWordThread() {
    SocketPermit *socket_permit = nullptr;

    for(int i = 0; i < SOCKET_PERMIT_DEFAULT_SIZE; i++){
        if(!(socket_permit = socket_permits[i])){
            continue;
        }

        socket_permit->masterRelease();
        socket_permit->servantRelease();
    }
}

void SocketWordThread::exe() {
    /*
     * 判断是否需要执行任务但被忽略了中断（不需要处理read函数,因为当socket有数据没有被读取时,LoopThread线程会多次中断SocketThread线程,直到读取为止）
     */
    if(!(interrupt_gemini & (SOCKET_READ_INTERRUPT_FLAG | SOCKET_IMMEDIATELY_FUNC_INTERRUPT_FLAG))){
        //阻塞等待有远程端的消息或需要执行函数
        waitRead();
    }

    //检查状态
    if(isSocketStatus(SOCKET_STATUS_RUNNING) || isSocketStatus(SOCKET_STATUS_EXCEPTION)){
        /* 问：当SocketThread线程正在执行任务时
         * SocketFuncMode满载或有立即执行任务,调用中断后,当前函数还未或正在执行thread_util->onSocketRead()函数,中断会被忽略
         * 解：socket输入与执行函数的中断标志是独立的,互不影响,同一中断标志在不同Thread线程不会存在竞争条件(不同线程会按执行链执行流程,如1、2)
         *  1.socket输入中断流程（由LoopThread线程设置中断,SocketThread线程处理中断）,如下
         *    (1)：LoopThread线程响应socket输入,设置read中断标志,中断SocketThread线程（SocketThread线程执行函数,会忽略中断,但不会忽略中断标志）
         *    (2)：SocketThread线程处理socket输入,复位read中断标志,读取数据
         *    (3)：等待下一次socket输入（步骤1）
         *  2.执行函数中断流程（由非本SocketThread线程设置中断,本SocketThread线程处理中断）,如下
         *    (1)：非本SocketThread线程输入函数（立即型或默认型）,设置func中断标志（立即型或默认型）,唤醒通知上一次因满载而阻塞的函数（步骤3）,中断SocketThread线程（SocketThread线程执行socket输入,
         *         会忽略中断,但不会忽略中断标志(忽略默认型中断)）
         *    (2)：Socket响应func中断,执行处理函数流程（立即型标志或默认型函数数量超过某个值 -> 设置其他线程不能输入函数（*阻塞等待） -> 拷贝函数 -> 设置函数输入（复位）-> 唤醒通知（步骤3） -> 执行输入函数）
         *    (3)：非本SocketThread输入函数,函数已满载*,阻塞等待输入（步骤1）
         */
        try {
            if(interrupt_gemini & (SOCKET_IMMEDIATELY_FUNC_INTERRUPT_FLAG | SOCKET_DEFAULT_FUNC_INTERRUPT_FLAG)) {
                if((interrupt_gemini | SOCKET_IMMEDIATELY_FUNC_INTERRUPT_FLAG) || socket_func_mode.isSocketFuncFull()){
                    onResetInterruptFlag(interrupt_gemini, (SOCKET_IMMEDIATELY_FUNC_INTERRUPT_FLAG | SOCKET_DEFAULT_FUNC_INTERRUPT_FLAG),
                                         [&]() -> void {
                                             socket_func_mode.onExecutorSocketFunc(socket_func_mode.onSocketFuncFull());
                                         });
                }
            }
            if(interrupt_gemini & SOCKET_READ_INTERRUPT_FLAG) {
                onResetInterruptFlag(interrupt_gemini, SOCKET_READ_INTERRUPT_FLAG,
                                     [&]() -> void {
                                         thread_util->onSocketRead();
                                     });
            }
        }catch(...){
            onException();
        }
    }
}

/**
 * 异常处理
 */
void SocketWordThread::onException() {
    //构造异常指令
    auto exception_instruct = Instruct::makeInstruct<ExceptionSocketInstruct>(word_thread);
    //将异常指令发送LoopThread线程
    loop_thread->receiveInstruct(exception_instruct);
    //设置异常状态
    onSocketStatus(SOCKET_STATUS_EXCEPTION);

    //等待LoopThread线程完成异常指令
    exception_instruct->exception_promise.get_future().get();

    /*
     * 判断SocketThread线程是否正在接收转移
     *  是 : 等待转移完成,再处理异常函数
     *  否 : 调用异常处理函数
     */
    if(!is_socket_transfer) {
        //异常处理函数
        onTransferSocketDone();
    }
}

/**
 * 异常处理函数实现类型（运行在SocketThread线程上）
 */
void SocketWordThread::onException0() {
    //向自己发送释放指令
    onInterrupt(Instruct::makeInstruct<ReleaseSocketInstruct>(REQUEST_SOCKET_RELEASE, word_thread));
}

/**
 * 中断（处理指令）
 */
void SocketWordThread::interrupt() {
    notifyRead();
}

/**
 * 中断并设置标记
 * @param flag
 */
void SocketWordThread::onInterrupt(int flag) {
    interrupt_gemini |= flag;
    interrupt();
}

void SocketWordThread::onPush(std::shared_ptr<ExecutorTask>) {
    //不需要处理
}

/**
 * 将数据输出（Executor线程或Loop线程）、数据输入（主socket线程）或驱动输入（Loop线程）转移到关联的socket线程
 *  主socket线程    -> 驱动输入（Loop线程）、数据输出（Executor线程或Loop线程）
 *  其他socket线程  -> 数据输入（主socket线程）、数据输出（Executor线程或Loop线程）
 * 成功：该Socket线程
 */
void SocketWordThread::onFuncExecutorSocket(const std::function<void()> &func, SocketFuncType func_type) {
    switch (func_type){
        case SOCKET_FUNC_TYPE_IMMEDIATELY:
            socket_func_mode.setSocketImmediatelyFunc(func);
            break;
        case SOCKET_FUNC_TYPE_DEFAULT:
        default:
            socket_func_mode.setSocketExecutorFunc(func);
            break;
    }
}


/**
 * 处理指令
 * @param instruct
 */
void SocketWordThread::onInterrupt(SInstruct instruct) throw(finish_error) {
    switch (instruct->instruct_type){
        //SocketThread线程没有远程端指令
        case INSTRUCT_ADDRESS_DONE:
            onAddressDone(std::dynamic_pointer_cast<ReleaseSocketInstruct>(instruct));
            break;
        //SocketThread线程释放指令
        case REQUEST_SOCKET_RELEASE:
            onReleaseSocket(std::dynamic_pointer_cast<ReleaseSocketInstruct>(instruct));
            break;
        //SocketThread线程启动指令
        case INSTRUCT_START:
            onStartSocket(std::dynamic_pointer_cast<StartInstruct>(instruct));
            break;
        //SocketThread线程停止指令
        case INSTRUCT_STOP:
            onStopSocket(std::dynamic_pointer_cast<StopSocketInstruct>(instruct));
            break;
        //SocketThread线程结束指令
        case INSTRUCT_FINISH:
            onFinishSocket(std::dynamic_pointer_cast<FinishInstruct>(instruct));
            break;
        case REQUEST_SOCKET_TRANSFER:
            onTransferSocket(std::dynamic_pointer_cast<TransferSocketInstruct>(instruct));
            break;
        case RESPONSE_SOCKET_TRANSFER:
            onTransferSocketDone();
        case INSTRUCT_SOCKET_TRANSMIT_NOTE:
            onTransmitNote(std::dynamic_pointer_cast<TransmitNoteInstruct>(instruct));
        default:
            break;
    }
}

/**
 * 处理接收远程端消除（LoopSocket线程）（socket有可读消息 -> LoopThread线程触发select -> 生成消息note -> 处理消息note）
 * @param note 消息note
 */
void SocketWordThread::onExecutor(std::shared_ptr<ExecutorNote> note) {
    if(note) { note->executor(); }
}

void SocketWordThread::onLoop() {}

/**
 * 验证远程端子许可（远程端是否在SocketThread线程）(写许可)
 * @param note  远程端
 * @return
 */
bool SocketWordThread::verifyPermitNote(MeetingAddressNote *note) const {
    uint32_t permit_pos = thread_util->onNotePermitFunc(note);
    return (note && (permit_pos < SOCKET_PERMIT_DEFAULT_SIZE) && socket_permits[permit_pos] && (note == socket_permits[permit_pos]->getMeetingAddressNote()));
}

/**
 * 验证远程端子许可（读许可）
 * @param note  远程端
 * @return
 */
bool SocketWordThread::verifyPermitThread(MeetingAddressNote *note) const {
    uint32_t permit_pos = thread_util->onNotePermitFunc(note);
    return (note && (permit_pos < SOCKET_PERMIT_DEFAULT_SIZE) && socket_permits[permit_pos] && (socket_permits[permit_pos]->getSocketWordThread() == this));
}

/**
 * 根据远程端释放Socket许可（该函数统一由LoopThread线程执行）
 * @param note 远程端note
 */
void SocketWordThread::releasePermit(MeetingAddressNote *note) {
    uint32_t permit_pos = thread_util->onNotePermitFunc(note);
    SocketPermit *note_permit = nullptr;

    if((permit_pos < SOCKET_PERMIT_DEFAULT_SIZE) && (note_permit = socket_permits[permit_pos]) && (note_permit->getMeetingAddressNote() == note)){
        //释放SocketThread线程的子许可
        note_permit->masterRelease();
        //释放远程端的子许可
        note_permit->servantRelease();

        //删除Socket许可
        socket_permits[permit_pos] = nullptr;
        setSocketPermitPos(unpermit_pos, permit_pos);
    }
}

/**
 * 远程端请求许可（该函数统一由LoopThread线程执行）
 * @param note 远程端note
 * @return
 */
bool SocketWordThread::acquirePermit(MeetingAddressNote *note) {
    uint32_t permit_pos = 0;
    SocketPermit *spermit = dynamic_cast<SocketPermit*>(socket_semaphore.acquire(SocketPermit(this, note, release_func)));
    //没有其他线程请求许可且SocketThread线程还有许可
    if(spermit){
        //获取存储许可序号
        permit_pos = getSocketPermitPos(unpermit_pos);

        //插入Socket许可
        socket_permits[permit_pos] = spermit;
        thread_util->onNotePermitFunc(note, permit_pos);
        thread_util->correlateRemoteNote(note);
        return true;
    }else{
        return false;
    }
}

int SocketWordThread::onSocketFd() const {
    return (thread_util ? TransmitThreadUtil::getTransmitSocketFd(thread_util) : -1);
}

uint32_t SocketWordThread::correlateTransmitId() const {
    return thread_util->getThreadCorrelateId();
}

/**
 * 发送消息
 * @param permit_socket 许可
 * @param buf
 */
void SocketWordThread::writeData(const TransmitOutputData &output_data) {
    onRunSocketThread(
            [&]() -> void {
                if(verifyPermitNote(output_data.output_note)){ thread_util->onSocketWrite(output_data); }
            }
    );
}

/**
 * 阻塞等待远程端消息
 */
void SocketWordThread::waitRead() {
    std::unique_lock<std::mutex> unique_lock(socket_mutex);
    socket_condition.wait(unique_lock);
}

/**
 * 唤醒阻塞的SocketThread线程,接收远程端消息
 */
void SocketWordThread::notifyRead() {
    socket_condition.notify_all();
}

/**
 * 处理SocketThread线程没有远程端（当释放所有的许可时回调该函数）
 * 该SocketWordThread没有MeetingAddressNote(LoopWordThread线程),回调该函数
 */
void SocketWordThread::onNoneAddress() {
    if(onStatus() == THREAD_STATUS_EXECUTOR) {
        //将线程权转到SocketWordThread线程
        word_thread->receiveInstruct(Instruct::makeInstruct<ReleaseSocketInstruct>(INSTRUCT_ADDRESS_DONE, word_thread));
    }
}

void SocketWordThread::onTransmitNote(std::shared_ptr<TransmitNoteInstruct> note_instruct) {
#define SOCKET_THREAD_TRANSMIT_NOTE_VALUE   1

    uint32_t note_pos = 0;
    SocketPermit *socket_permit = nullptr;
    MeetingAddressNote *socket_note_array[SOCKET_PERMIT_DEFAULT_SIZE];

    for(int i = 0; i < SOCKET_PERMIT_DEFAULT_SIZE; i++){
        if(!(socket_permit = socket_permits[i])){
            continue;
        }

        if(socket_permit->checkPermit()){
            socket_note_array[note_pos++] = socket_permit->getMeetingAddressNote();
        }
    }
    note_instruct->transmit_func(thread_util, *socket_note_array, note_instruct->note_msg, note_pos);

    if(note_instruct->socket_size.fetch_sub(SOCKET_THREAD_TRANSMIT_NOTE_VALUE, std::memory_order_acq_rel) <= SOCKET_THREAD_TRANSMIT_NOTE_VALUE){
        note_instruct->release_func();
    }
}

/**
 * SocketThread线程处理没有远程端,发送INSTRUCT_ADDRESS_DONE指令给LoopThread线程请求释放该线程
 * LoopWordThread是否处理INSTRUCT_ADDRESS_DONE指令
 *  是 => 没有或转移（发生异常）远程端,释放SocketThread线程
 *  否 => 其他SocketThread无法接收远程端（一或多个）,SocketThread线程继续运行
 * @param instruct
 */
void SocketWordThread::onAddressDone(std::shared_ptr<ReleaseSocketInstruct> instruct) {
    instruct->instruct_type = REQUEST_SOCKET_RELEASE;
    instruct->instruct_thread = word_thread;

    //请求LoopWordThread释放该线程
    loop_thread->receiveInstruct(instruct);
}

/**
 * 释放SocketThread线程函数
 * @param instruct
 */
void SocketWordThread::onReleaseSocket(std::shared_ptr<ReleaseSocketInstruct> instruct) {
    onReleaseSocket0(instruct);
}

/**
 * 释放函数实现
 *  在转移过程中远程端消息如何处理和如何向远程端发送消息？
 *  以下几种情况需要释放SocketThread线程（转移远程端）
 *      1：SocketThread线程内的远程端全部退出,需要释放SocketThread线程（无需转移远程端）
 *      2：SocketThread线程因异常退出（需要转移远程端）
 *      3：SocketThread线程正常运行,但LoopThread线程协调将该线程内的远程端转移到其他SocketThread线程内（需要转移远程端）
 *  对于2、3的情况如何处理远程端的消息和向远程端发送消息
 *      将远程端先转移到新的SocketThread线程,旧SocketThread线程向远程端发送转移消息
 *      SocketThread线程不接收和不发送远程端的消息,暂时由旧SocketThread线程接收和发送消息
 *      由远程端发送确定消息才接收和发送消息由新的SocketThread线程处理
 *      但旧的SocketThread线程内所有的远程端确定转移消息才真正释放旧的SocketThread线程
 * @param instruct
 */
//在转移过程中远程端消息如何处理和如何向远程端发送消息 --> (按新地址发送消息、抛弃旧地址的消息)
void SocketWordThread::onReleaseSocket0(std::shared_ptr<ReleaseSocketInstruct> instruct) {
    /*
     * 将SocketThread线程远程端移动到指令内,等待被其他SocketThread接收
     */
    SocketPermit *socket_permit = nullptr;
    //设置Socket线程停止状态（不能接收执行在Socket线程的func,即onFuncExecutorSocket）
    onSocketStatus(SOCKET_STATUS_STOP);

    for(int i = 0; i < SOCKET_PERMIT_DEFAULT_SIZE; i++){
        if(!(socket_permit = socket_permits[i])){
            continue;
        }

        if(socket_permit->checkPermit()){
            instruct->release_notes.push_back(socket_permit->getMeetingAddressNote());
        }

        socket_permit->masterRelease();
        socket_permit->servantRelease();
    }

    socket_func_mode.onSocketDone();

    instruct->instruct_type = RESPONSE_SOCKET_RELEASE;
    loop_thread->receiveInstruct(instruct);
}

/**
 * SocketThread线程接收转移函数（运行在SocketThread线程上）
 * @param instruct
 */
void SocketWordThread::onTransferSocket(std::shared_ptr<TransferSocketInstruct> instruct) {
    //设置线程转移
    is_socket_transfer = true;
    //向执行层发送接收的远程端note
    threadManager()->onThreadTransfer(*instruct->transfer_note, instruct->transfer_size,  thread_util->getThreadCorrelateId());
}

/**
 * SocketThread线程接收转移完成函数（运行在SocketThread线程上）
 */
void SocketWordThread::onTransferSocketDone() {
    //复位线程转移
    is_socket_transfer = false;
    //判断SocketThread线程是否异常状态
    if(isSocketStatus(SOCKET_STATUS_EXCEPTION)) {
        //调用异常处理函数
        onException0();
    }
}

/**
 * 启动SocketThread线程指令
 * @param start_instruct 指令
 */
void SocketWordThread::onStartSocket(std::shared_ptr<StartInstruct> start_instruct) {
    //更改状态
    Word::onExecutor();
    onSocketStatus(SOCKET_STATUS_RUNNING);
    //只有由ThreadExecutor（ManagerThread线程）发送的指令才会设置并调用回调函数,而LoopThread线程没有设置回调函数所以不会调用
    Word::onStartComplete(start_instruct);
    std::cout << "SocketWordThread::onStartSocket->" << std::endl;
}

/**
 * 停止SocketThread线程指令
 * @param stop_instruct 指令
 */
void SocketWordThread::onStopSocket(std::shared_ptr<StopSocketInstruct> stop_instruct) {
    SocketPermit *socket_permit = nullptr;
    //更改状态
    Word::onStop();
    onSocketStatus(SOCKET_STATUS_STOP);

    //释放SocketThread线程的所有许可
    for(int pos = 0; pos < SOCKET_PERMIT_DEFAULT_SIZE; pos++){
        if(!(socket_permit = socket_permits[pos])){
            continue;
        }
        thread_util->onNoteTerminationFunc(socket_permit->getMeetingAddressNote());

        socket_permit->masterRelease();
        socket_permit->servantRelease();
    }

    Word::onStopComplete(stop_instruct);
    std::cout <<  "SocketWordThread::onStopSocket" << std::endl;
}

/**
 * 结束SocketThread线程指令
 * @param finish_instruct 指令
 */
void SocketWordThread::onFinishSocket(std::shared_ptr<FinishInstruct> finish_instruct) throw(finish_error){
    std::cout << "SocketWordThread::onFinishSocket->" << std::endl;
    //更改状态
    Word::onFinish(finish_instruct->finish_callback);
}