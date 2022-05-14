//
// Created by abc on 20-12-23.
//
#include "../BasicControl.h"

ExecutorTransferInfo::ExecutorTransferInfo(WordThread *thread) : need_transfer_size(0), timeout_holder(), transfer_thread(thread){
    //初始化储存数组
    memset(transfer_note_heap, 0, sizeof(MeetingAddressNote*) * EXECUTOR_LAYER_TRANSFER_HEAD_SIZE);

    //设置序号的关联性
    for(int i = 1; i <= EXECUTOR_LAYER_TRANSFER_HEAD_SIZE; i++){
        note_pos_heap[i] = (i - 1);
    }
    note_pos_heap[0] = 0;
}

ExecutorTransferInfo::~ExecutorTransferInfo() {
    //取消定时器
    timeout_holder.cancel();
}

/**
 * 初始化SocketThread线程的note（运行在SocketThread线程上）
 *   加入： 获取序号,存储note,note关联序号
 *   转移： 获取序号,存储note,note关联序号,设置转移标志,增加数量
 * @param note
 */
void ExecutorTransferInfo::onInitTransfer(MeetingAddressNote *note) {
    //获取存储note的序号
    int note_pos = getTransferNotePos(note_pos_heap);

    //存储note
    transfer_note_heap[note_pos] = note;
    //note关联序号
    MeetingAddressManager::setNoteTransferPos(note, static_cast<uint32_t>(note_pos));
}

/**
 * 析够SocketThread线程的note（运行在SocketThread线程上）
 *   退出：回收被存储的note,取消标志,回收序号
 * @param note
 */
void ExecutorTransferInfo::onUnitTransfer(MeetingAddressNote *note) {
    //获取note关联的嘻哈
    int note_pos = MeetingAddressManager::getNoteTransferPos(note);

    //取消转移标志
    note_pos_heap[note_pos] &= (~EXECUTOR_TRANSFER_INFO_NOTE_TRANSFER);
    //释放存储的note
    transfer_note_heap[note_pos] = nullptr;
    //回收序号
    setTransferNotePos(note_pos_heap, note_pos);
}

/**
 * 启动转移器（SocketThread线程接收被转移note）（运行在SocketThread线程上）
 * @param start_holder  定时器控制器
 */
void ExecutorTransferInfo::onStartTransfer(TaskHolder<void> &&start_holder) {
    //取消上一次的定时器
    timeout_holder.cancel();
    //设置定时器控制器
    timeout_holder = std::move(start_holder);
}

/**
 * 停止转移器（SocketThread线程完成或超时）
 */
void ExecutorTransferInfo::onStopTransfer() {
    //取消定时器
    timeout_holder.cancel();
}

/**
 * 接收被转移的note（单个）（运行在SocketThread线程上）
 * @param note
 */
void ExecutorTransferInfo::onNoteTransfer(MeetingAddressNote *note) {
    //获取note关联的序号
    int note_pos = MeetingAddressManager::getNoteTransferPos(note);

    //增加被转移数量
    need_transfer_size++;
    //设置转移标志
    note_pos_heap[note_pos] |= EXECUTOR_TRANSFER_INFO_NOTE_TRANSFER;
}

/**
 * 被转移的note回复确定转移
 * @param confirm_note  确定的note
 * @return
 */
bool ExecutorTransferInfo::onNoteConfirm(MeetingAddressNote *confirm_note) {
    int remain_size = 0, transfer_size = 0, note_pos = MeetingAddressManager::getNoteTransferPos(confirm_note);

    //该note没有被标记转移,返回
    if(!(note_pos_heap[note_pos] &= EXECUTOR_TRANSFER_INFO_NOTE_TRANSFER)){
        return false;
    }

    /*
     * 1.获取当前需要被转移的数量,数量为0（定时器已超时）,返回
     * 2.更改被转移的数量为(当前 - 1)
     * 3.比较并设置
     *  成功:取消转移标记,返回当前数量是否为最后一个
     *  失败:重复步骤1
     */
    for(;;){
        if((remain_size = need_transfer_size.load(std::memory_order_consume)) <= 0){
            return false;
        }
        transfer_size = remain_size - 1;

        if(need_transfer_size.compare_exchange_weak(remain_size, transfer_size, std::memory_order_release, std::memory_order_relaxed)){
            note_pos_heap[note_pos] &= (~EXECUTOR_TRANSFER_INFO_NOTE_TRANSFER);
            return (transfer_size <= 0);
        }
    }
}

/**
 * 转移定时器超时
 * @return
 */
int ExecutorTransferInfo::onTimeoutTransfer() {
    int remain_size = 0, timeout_size = 0;

    /*
     * 1:获取当前需要被转移的数量,数量为0（被转移note已全部确定,停止定时器前,定制器触发）,返回
     * 2:比较并设置数量为0
     *  成功：返回当前需要被转移的数量
     *  失败：重复步骤1
     */
    for(;;){
        if((timeout_size = remain_size = need_transfer_size.load(std::memory_order_consume) <= 0)){
            break;
        }

        if(need_transfer_size.compare_exchange_weak(remain_size, 0, std::memory_order_release, std::memory_order_relaxed)){
            break;
        }
    }

    return timeout_size;
}

/**
 * 超时的被转移的note
 * @param callback  回调函数
 */
void ExecutorTransferInfo::onTimeoutNote(const std::function<bool(MeetingAddressNote *)> &callback) {
    MeetingAddressNote *timeout_note = nullptr;

    //遍历全部note
    for(int i = 0; i < EXECUTOR_LAYER_TRANSFER_HEAD_SIZE; i++){
        //序号被标记转移且序号有存储note
        if((note_pos_heap[i] & EXECUTOR_TRANSFER_INFO_NOTE_TRANSFER) && (timeout_note = transfer_note_heap[i])){
            //回调超时的note并根据返回值是否结束循环
            if(!callback(timeout_note)){
                break;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------//

MeetingAddressNote* ExecutorNoteMediaInfo::getNoteInfoOnPos(uint32_t pos)  {
    return (note_array + pos);
}

TimeInfo* ExecutorNoteMediaInfo::getTimeInfoOnPos(uint32_t pos)  {
    return (time_array + pos);
}

void ExecutorNoteMediaInfo::callExecutorFunc(const ExecutorFunc &func)  {
    executor_func(this, func);
}

void ExecutorNoteMediaInfo::callNoteCompleteFunc()  {
    for(uint32_t i = 0; i < info_size; i++) { submit_func((time_array + i)); }
}

void ExecutorNoteMediaInfo::callNoteTransmitFunc(TransmitThreadUtil *thread_util, MeetingAddressNote *note, MsgHdr *msg, uint32_t note_size) {
    transmit_func(thread_util, note, msg, note_size, transmit_sequence, timeout_threshold);
}

ExecutorNoteMediaInfo* ExecutorNoteMediaInfo::initNoteInfo(const std::function<void(const std::function<void(MeetingAddressNote*, uint32_t)>&)> &note_func, TimeInfo *tarray) {
    for(int i = 0; i < info_size; i++){
        memcpy(time_array + i, tarray + i, sizeof(TimeInfo*));
    }

    note_func([&](MeetingAddressNote *note_info, uint32_t pos) -> void {
                memcpy(note_array + pos, note_info, sizeof(MeetingAddressNote*));
            });

    return this;
}

//--------------------------------------------------------------------------------------------------------------------//

BasicLayer* ExecutorLayerUtil::createLayer(BasicControl *control) noexcept {
    return (new (std::nothrow) ExecutorLayer(control));
}

void ExecutorLayerUtil::destroyLayer(BasicControl *, BasicLayer *layer) noexcept {
    delete layer;
}

//--------------------------------------------------------------------------------------------------------------------//

ExecutorLayer::ExecutorLayer(BasicControl *control) : BasicLayer(control), ThreadExecutorParameter(), TaskExecutorParameter(){
    initLayer();
}

ExecutorLayer::~ExecutorLayer() {
    initLayer();
}

void ExecutorLayer::initLayer() {
    for(auto iterator : transfer_info_map){
        iterator.second->~ExecutorTransferInfo();
        onDestroyBuffer(sizeof(ExecutorTransferInfo),
                        [&](MsgHdr *msg) -> void {
                            MsgHdrUtil<ExecutorTransferInfo*>::initMsgHdr(msg, sizeof(ExecutorTransferInfo),
                                                                          LAYER_CONTROL_REQUEST_DESTROY_FIXED, iterator.second);
                        });
    }
    transfer_info_map.clear();
}

void ExecutorLayer::onInput(MsgHdr *msg) {
    if(msg){
        switch (msg->master_type){
            case EXECUTOR_LAYER_APPLICATION_TIMER_IMMEDIATELY:
                onImmediatelyTimer(msg);
                break;
            case EXECUTOR_LAYER_APPLICATION_TIMER_MORE:
                onMoreTimer(msg);
                break;
            case EXECUTOR_LAYER_IS_TRANSFER:
                isTransferThread(msg);
                break;
            case EXECUTOR_LAYER_ADDRESS_JOIN:
                onAddressJoin(msg);
                break;
            case EXECUTOR_LAYER_ADDRESS_EXIT:
                onAddressExit(msg);
                break;
            case EXECUTOR_LAYER_CONFIRM_TRANSFER:
                onConfirmTransfer(msg);
                break;
            case EXECUTOR_LAYER_START_THREAD:
                if(!onSocketThreadStart(reinterpret_cast<ExecutorTransferData*>(msg->buffer))){ msg->serial_number = 0; }
                break;
            case EXECUTOR_LAYER_STOP_THREAD:
                onSocketThreadStop(reinterpret_cast<ExecutorTransferData*>(msg->buffer));
                break;
            case EXECUTOR_LAYER_CORRELATE_NOTE:
                onCorrelateNote(reinterpret_cast<ExecutorNoteInfo*>(msg->buffer));
                break;
            default:
                break;
        }
    }
}

void ExecutorLayer::onOutput() {}

void ExecutorLayer::onDrive(MsgHdr*) {}

void ExecutorLayer::onParameter(MsgHdr *msg) {
    ExecutorParameter *parameter = nullptr;

    if((msg->serial_number < sizeof(ExecutorParameter)) || !(parameter = reinterpret_cast<ExecutorParameter*>(msg->buffer))){
        msg->serial_number = 0;
    }else{
        onThreadParameter(parameter);
        onTaskParameter(parameter);
        msg->serial_number = sizeof(ExecutorParameter);
    }
}

/**
 * 控制输入
 * @param msg
 */
void ExecutorLayer::onControl(MsgHdr *control_msg) {
    switch (control_msg->master_type) {
        case LAYER_CONTROL_STATUS_STOP:
            stopExecutorLayer();
            break;
        case LAYER_CONTROL_STATUS_START:
            if (!launchExecutorLayer()) { control_msg->master_type = LAYER_CONTROL_STATUS_THROW; }
            break;
        default:
            break;
    }
}

/**
 * 启动执行层
 * @return  是否启动成功
 */
bool ExecutorLayer::launchExecutorLayer() {
    return static_cast<bool>((executor_pool = new (std::nothrow) ExecutorServer(this, LAUNCH_IMMEDIATE)));
}

/**
 * 停止执行层
 */
void ExecutorLayer::stopExecutorLayer() {
    delete executor_pool;
    executor_pool = nullptr;
    transfer_info_map.clear();
}

/**
 * 多次执行的定时器
 * @param timer_msg
 */
void ExecutorLayer::onMoreTimer(MsgHdr *timer_msg) {
    if(timer_msg->serial_number < std::max(sizeof(TaskHolder<void>), sizeof(LoopExecutorTask))){
        timer_msg->serial_number = 0;
    }else{
        onMoreTimer0(reinterpret_cast<ExecutorTimerData*>(timer_msg->buffer),
                     [&](TaskHolder<void> *holder) -> void {
                         if(holder){
                             MsgHdrUtil<TaskHolder<void>>::initMsgHdr(timer_msg, sizeof(TaskHolder<void>),
                                                                      static_cast<uint32_t>(timer_msg->master_type), std::move(*holder));
                         }else{
                             timer_msg->serial_number = 0;
                         }
                     });
    }
}

/**
 * 多次执行的定时器实现函数
 * @param timer_data    定时信息
 * @param callback      回调函数
 */
void ExecutorLayer::onMoreTimer0(ExecutorTimerData *timer_data, const std::function<void(TaskHolder<void>*)> &callback) {
    if(!timer_data || !executor_pool){
        //没有定时信息或没有启动执行层,返回空
        callback(nullptr);
    }else{
        try {
            //创建多次执行的定时层并返回控制器
            TaskHolder<void> holder = dynamic_cast<ExecutorServer*>(executor_pool)->submit(initExecutorTask(timer_data),
                                                                                           &executor_func, &cancel_func, &timeout_func);
            //回调控制器
            callback(&holder);
            //创建成功参数
            onRequestTask(ParameterRequestMore);
        }catch (...){
            //创建失败,回调空
            callback(nullptr);
            //创建失败参数
            onRequestTask(ParameterRequestFail);
        }
    }
}

/**
 * 根据定时信息初始化Loop任务信息类
 * @param timer_data    定时信息（多次）
 * @return
 */
LoopExecutorTask ExecutorLayer::initExecutorTask(ExecutorTimerData *timer_data) throw (std::logic_error){
    //触发次数小于等于1、触发时间小于0、没有触发函数、没有超时函数
    //初始化失败,抛出logic_error
    if((timer_data->executor_amount <= 1) || (timer_data->executor_microsecond <= 0) || !timer_data->executor_func || !timer_data->timeout_func){
        throw std::logic_error(nullptr);
    }

    //初始化Loop任务信息类
    LoopExecutorTask executor_task{};

    //设置定时时间信息
    executor_task.setTaskTimeInfo(static_cast<uint32_t>(timer_data->executor_amount), timer_data->executor_microsecond,
                                                                             timer_data->executor_increase_microsecond);
    //设置定时触发函数
    executor_task.setTriggerFunc(timer_data->executor_func);
    //设置定时超时函数
    executor_task.setTimeoutFunc(timer_data->timeout_func);

    //返回Loop任务信息类
    return executor_task;
}

/**
 * 单次执行的定时器（立即或延迟）
 * @param timer_msg
 */
void ExecutorLayer::onImmediatelyTimer(MsgHdr *timer_msg) {
    if(timer_msg->serial_number < std::max(sizeof(ExecutorTimerData), sizeof(TaskHolder<void>))){
        timer_msg->serial_number = 0;
    }else{
        onImmediatelyTimer0(reinterpret_cast<ExecutorTimerData*>(timer_msg->buffer),
                           [&](TaskHolder<void> *holder) -> void {
                               if(holder){
                                   MsgHdrUtil<TaskHolder<void>>::initMsgHdr(timer_msg, sizeof(TaskHolder<void>),
                                                                            static_cast<uint32_t>(timer_msg->master_type), std::move(*holder));
                               }else{
                                   timer_msg->serial_number = 0;
                               }
                           });
    }
}

/**
 * 单次执行的定时器（立即或延迟）实现类
 * @param timer_data    定时信息
 * @param callback      回调函数
 */
void ExecutorLayer::onImmediatelyTimer0(ExecutorTimerData *timer_data, const std::function<void(TaskHolder<void>*)> &callback) {
    int64_t delay_time = 0;

    if(!executor_pool || !timer_data || !timer_data->timeout_func){
        callback(nullptr);
    }else{
        /*
         * 计算延迟时间（0：立即）
         * LoopWordTask::countMaxTime()函数
         */
        delay_time = (timer_data->executor_microsecond * timer_data->executor_amount) +
                (((timer_data->executor_amount * (timer_data->executor_amount - 1)) / 2) * timer_data->executor_increase_microsecond);

        try {
            //创建单次执行的定时层并返回控制器
            TaskHolder<void> holder = dynamic_cast<ExecutorServer*>(executor_pool)->submit<void>(std::move(timer_data->timeout_func),
                                                                                                 std::chrono::microseconds(delay_time),
                                                                                                 &executor_func, &cancel_func);
            //回调控制器
            callback(&holder);
            //创建成功参数
            onRequestTask(ParameterRequestImmediately);
        }catch (...){
            //创建失败,回调空
            callback(nullptr);
            //创建失败参数
            onRequestTask(ParameterRequestFail);
        }
    }
}

void ExecutorLayer::isTransferThread(MsgHdr *thread_msg) {
    if((thread_msg->serial_number < sizeof(ExecutorTransferData*)) || !isTransferThread0(reinterpret_cast<ExecutorTransferData*>(thread_msg->buffer))){
        thread_msg->serial_number = 0;
    }
}

bool ExecutorLayer::isTransferThread0(ExecutorTransferData *transfer_data) {
    auto transfer_iterator = transfer_info_map.find(transfer_data->transfer_thread_id);
    return ((transfer_iterator == transfer_info_map.end()) || transfer_iterator->second->onTimeoutTransfer());
}

/**
 * 处理远端端的加入关联SocketThread线程（主Socket线程）
 * @param msg
 */
void ExecutorLayer::onAddressJoin(MsgHdr *msg) {
    auto init_info = reinterpret_cast<ExecutorInitInfo*>(msg->buffer);
    TransmitThreadUtil *join_thread_util = nullptr;

    if(!executor_pool || !(join_thread_util = onAddressJoin0(dynamic_cast<ExecutorServer*>(executor_pool), init_info))) {
        //没有启动执行层或加入失败,执行失败函数
        init_info->callThrowFunc();
        onInputAddress(ParameterJoinFail);
    }else{
        try {
            //加入成功,执行成功函数
            init_info->callInitFunc(join_thread_util);
            onInputAddress(ParameterJoinSuccess);
        }catch (std::logic_error &e){
            //成功函数抛出异常,退出该远程端note
            onAddressExit0(dynamic_cast<ExecutorServer*>(executor_pool), init_info);
            //执行失败函数
            init_info->callThrowFunc();
            onInputAddress(ParameterJoinFail);
        }

    }
}

/**
 * 远程端note加入关联SocketThread线程的实现类
 * @param server        线程池
 * @param note          远程端
 * @return
 */
TransmitThreadUtil* ExecutorLayer::onAddressJoin0(ExecutorServer *server, ExecutorInitInfo *init_info) {
    //提价线程层加入远程端
    return server->submitNote(init_info->getNoteInfo(), init_info->getNoteFunc()).get();
}

/**
 * 处理远程端的退出取消关联SocketThread线程（关联Socket线程或其他线程）
 * @param msg
 */
void ExecutorLayer::onAddressExit(MsgHdr *msg) {
    if((msg->serial_number < sizeof(MeetingAddressNote*)) || !executor_pool){
        msg->serial_number = 0;
    }else {
        onAddressExit0(dynamic_cast<ExecutorServer*>(executor_pool), reinterpret_cast<ExecutorThreadInfo*>(msg->buffer));
    }
}

/**
 * 远程端note退出取消关联SocketThread线程的实现类
 * @param server    线程池
 * @param note      远程端
 */
void ExecutorLayer::onAddressExit0(ExecutorServer *server, ExecutorThreadInfo *unit_info) {
    //提交线程池退出远程端
    server->exitNote(unit_info->getNoteInfo(), unit_info->getNoteFunc());
    //设置退出参数
    onInputAddress(ParameterExit);
}

//启动的SocketThread线程
bool ExecutorLayer::onSocketThreadStart(ExecutorTransferData *transfer_data) {
    ExecutorTransferInfo *transfer_info = nullptr;
    onSocketThreadStop(transfer_data);

    if(!(transfer_info = reinterpret_cast<ExecutorTransferInfo*>(onCreateBuffer(sizeof(ExecutorTransferInfo))))){
        return false;
    }

    if(!transfer_info_map.insert({transfer_data->transfer_thread_id, transfer_info}).second){
        onDestroyBuffer(sizeof(ExecutorTransferInfo),
                        [&](MsgHdr *msg) -> void {
                            MsgHdrUtil<ExecutorTransferInfo*>::initMsgHdr(msg, sizeof(ExecutorTransferInfo), LAYER_CONTROL_REQUEST_DESTROY_FIXED, transfer_info);
                        });
        return false;
    }else{
        new (reinterpret_cast<void*>(transfer_info)) ExecutorTransferInfo(dynamic_cast<WordThread*>(transfer_data->transfer_thread));
        return true;
    }
}

//停止的SocketThread线程
void ExecutorLayer::onSocketThreadStop(ExecutorTransferData *transfer_data) {
    auto transmit_info_iterator = transfer_info_map.find(transfer_data->transfer_thread_id);

    if(transmit_info_iterator != transfer_info_map.end()){
        transmit_info_iterator->second->~ExecutorTransferInfo();
        onDestroyBuffer(sizeof(ExecutorTransferInfo),
                        [&](MsgHdr *msg) -> void {
                            MsgHdrUtil<ExecutorTransferInfo*>::initMsgHdr(msg, sizeof(ExecutorTransferInfo), LAYER_CONTROL_REQUEST_DESTROY_FIXED, transmit_info_iterator->second);
        });
        transfer_info_map.erase(transmit_info_iterator);
    }
}

void ExecutorLayer::onCorrelateNote(ExecutorNoteInfo *note_info) {
    dynamic_cast<ThreadExecutor*>(basic_control)->onTransmitNote(note_info);
}

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 线程池创建传输线程（SocketThread线程）前调用该函数（运行在ManagerThread线程上）
 * 创建SocketThread线程的传输工具类及管理被转移信息类
 * @return  传输工具类的标记id（SocketThread线程）
 */
uint32_t ExecutorLayer::onTransmitThread() {
    MsgHdr *transmit_msg = nullptr;
    BasicControl::callOutputOnStackMemory(sizeof(MsgHdr) + std::max(sizeof(uint32_t), sizeof(SocketWordThread*)),
                                          [&](MsgHdr *msg) -> void {
                                              transmit_msg = MsgHdrUtil<void>::initMsgHdr(msg, LAYER_CONTROL_EXECUTOR_CREATE_THREAD);
                                          });

    //发送控制层创建传输工具类
    basic_control->onInput(transmit_msg, nullptr, *this, CONTROL_INPUT_FLAG_CONTROL);

    //创建成功,执行层根据传输工具类的标记id（SocketThread线程）添加管理被转移信息类
    if(transmit_msg->serial_number > 0){
        MsgHdrUtil<void>::initMsgHdr(transmit_msg, EXECUTOR_LAYER_START_THREAD);
        onInput(transmit_msg);
    }

    //创建传输工具类或添加管理被转移信息类失败
    if(transmit_msg->serial_number <= 0){
        //删除执行层的传输工具类的标记id（SocketThread线程）的管理被转移信息类
        MsgHdrUtil<void>::initMsgHdr(transmit_msg, EXECUTOR_LAYER_STOP_THREAD);
        onInput(transmit_msg);
    }

    //返回函数是否执行成功
    return (transmit_msg->serial_number > 0) ? *reinterpret_cast<uint32_t*>(transmit_msg->buffer) : 0;
}

/**
 * 线程池创建SocketThread线程成功或失败后调用该函数（运行在ManagerThread线程上）
 *  成功：关联SocketThread线程与传输工具类
 *  失败：删除执行层的传输工具类的标记id（SocketThread线程）的管理被转移信息类
 * @param socket_thread
 * @param transfer_info_id
 */
void ExecutorLayer::onTransmitThread(SocketWordThread *socket_thread, uint32_t transfer_info_id) {
    BasicControl::callOutputOnStackMemory(sizeof(MsgHdr) + sizeof(SocketWordThread*),
                                          [&](MsgHdr *transmit_msg) -> void {
                                              /*
                                               * 判断创建线程是否成功
                                               */
                                              if(socket_thread){
                                                  //发送控制层关联SocketThread线程与传输工具类
                                                  MsgHdrUtil<SocketWordThread*>::initMsgHdr(transmit_msg, sizeof(SocketWordThread*),
                                                                                            LAYER_CONTROL_TRANSMIT_CORRELATE_THREAD, socket_thread);
                                                  basic_control->onInput(transmit_msg, nullptr, *this, CONTROL_INPUT_FLAG_CONTROL);
                                              }else{
                                                  //删除执行层的传输工具类的标记id（SocketThread线程）的管理被转移信息类
                                                  MsgHdrUtil<uint32_t>::initMsgHdr(transmit_msg, sizeof(uint32_t), EXECUTOR_LAYER_STOP_THREAD, transfer_info_id);
                                                  onInput(transmit_msg);
                                              }
                                          });
}

/**
 * 释放SocketThread线程后调用该函数（运行在ManagerThread线程上）
 * 取消关联并释放传输工具类
 * 删除执行层的传输工具类的标记id（SocketThread线程）的管理被转移信息类
 * @param socket_thread
 */
void ExecutorLayer::unTransmitThread(SocketWordThread *socket_thread) {
    BasicControl::callOutputOnStackMemory(sizeof(MsgHdr) + std::max(sizeof(uint32_t), sizeof(SocketWordThread*)),
                                          [&](MsgHdr *transmit_msg) -> void {
                                              MsgHdrUtil<SocketWordThread*>::initMsgHdr(transmit_msg, sizeof(SocketWordThread*),
                                                                                        LAYER_CONTROL_EXECUTOR_DESTROY_THREAD, socket_thread);
                                              //发送控制层取消关联并释放传输工具类
                                              basic_control->onInput(transmit_msg ,nullptr, *this, CONTROL_INPUT_FLAG_CONTROL);
                                              //删除执行层的传输工具类的标记id（SocketThread线程）的管理被转移信息类
                                              onInput(MsgHdrUtil<void>::initMsgHdr(transmit_msg, EXECUTOR_LAYER_STOP_THREAD));
                                          });
}

void ExecutorLayer::onDisplayThread(DisplayWordThread *display_thread) {
    BasicControl::callOutputOnStackMemory(sizeof(MsgHdr) + sizeof(DisplayWordThread*),
                                          [&](MsgHdr *msg) -> void {
                                              MsgHdrUtil<DisplayWordThread*>::initMsgHdr(msg, sizeof(DisplayWordThread*),
                                                                                         DISPLAY_LAYER_CONTROL_THREAD, display_thread);
                                              basic_control->onLayerCommunication(msg, LAYER_DISPLAY_TYPE);
                                          });
}

/**
 * 将SocketThread线程关联的note存储被转移信息类中（note的加入）（在运行在关联的SocketThread线程上）
 * @param note
 */
void ExecutorLayer::onSubmitNote(MeetingAddressNote *note, uint32_t transfer_id) {
    auto transfer_iterator = transfer_info_map.find(transfer_id);

    if((transfer_iterator != transfer_info_map.end())){
        transfer_iterator->second->onInitTransfer(note);
    }
}

/**
 * 将SocketThread线程g关联的note移除被转移信息类中（note的退出）（运行在关联的SocketThread线程上）
 * @param note
 */
void ExecutorLayer::onUnloadNote(MeetingAddressNote *note, uint32_t transfer_id) {
    auto transfer_iterator = transfer_info_map.find(transfer_id);

    if(transfer_iterator != transfer_info_map.end()){
        transfer_iterator->second->onUnitTransfer(note);
    }
}

/**
 * 管理被转移信息类接收被转移note（运行在关联的SocketThread线程上）
 * @param transfer_size
 * @param transfer_id
 */
void ExecutorLayer::onAddressTransfer(MeetingAddressNote *transfer_note, uint32_t transfer_size, uint32_t transfer_id) {
    decltype(transfer_info_map.end()) transfer_iterator;

    //被转移数量小于等于0、当前SocketThread线程没有添加管理被转移信息类
    if((transfer_size <= 0) || ((transfer_iterator = transfer_info_map.find(transfer_id)) == transfer_info_map.end())){
        return;
    }

    onAddressTransfer0(transfer_size, transfer_note, transfer_iterator->second);
}

/**
 * 管理被转移信息类接收被转移note的实现类
 * @param transfer_size     被转移的数量
 * @param transfer_note     被转移的note
 * @param transfer_info     管理被转移信息类
 */
void ExecutorLayer::onAddressTransfer0(uint32_t transfer_size, MeetingAddressNote *transfer_note, ExecutorTransferInfo *transfer_info) {
//    using transfer_timeout_func = void(*)(ExecutorLayer*, ExecutorTransferInfo*);
    MeetingAddressNote *note = nullptr;

    //遍历所有被转移的note
    for(int i = 0; i < transfer_size; i++){
        note = (transfer_note + i);
        //将被转移的note管理被转移信息类中（转移）
        //与onSubmitNote函数相同
        transfer_info->onInitTransfer(note);
        //添加被转移信息
        transfer_info->onNoteTransfer(note);
    }
    //设置被转移数量参数
    onTransferAddress(transfer_size);

    try {
        //创建被转移定时器并启动转移器
        transfer_info->onStartTransfer(dynamic_cast<ExecutorServer*>(executor_pool)->submit<void>(
                [=]() -> void {
                    onTransferTimeout(transfer_info);
                },
                std::chrono::seconds(EXECUTOR_LAYER_TRANSFER_TIME_SECONDS)));
        //启动成功,设置参数
        onRequestTask(ParameterRequestImmediately);
    }catch (std::logic_error &e){
        //创建失败,设置参数
        onRequestTask(ParameterRequestFail);
        //遍历所有被转移的note
        for(int i = 0; i < transfer_size; i++){
            //确定被转移信息
            transfer_info->onNoteConfirm(transfer_note + i);
        }
        //停止转移器
        transfer_info->onStopTransfer();
        //发送线程池SocketThread线程接收转移完成
        dynamic_cast<ThreadExecutor*>(executor_pool)->onTransferComplete(transfer_info->getTransferThread());
    }
}

/**
 * 远程端note响应被转移函数（运行在关联的SocketThread线程上）
 * @param confirm_msg
 */
void ExecutorLayer::onConfirmTransfer(MsgHdr *confirm_msg) {
    ExecutorTransferData *transfer_data = nullptr;
    decltype(transfer_info_map.end()) transfer_iterator;

    if((confirm_msg->serial_number < sizeof(MeetingAddressNote*)) || !(transfer_data = reinterpret_cast<ExecutorTransferData*>(confirm_msg->buffer))){
        return;
    }

    //没有远程端、关联的SocketThread线程没有添加管理被转移信息类
    if(!transfer_data->transfer_note || (transfer_iterator = transfer_info_map.find(transfer_data->transfer_thread_id)) == transfer_info_map.end()){
        return;
    }

    /*
     * 远程端note确定被转移信息
     *  true  : 全部被转移的note已确定（关联的SocketThread线程）
     *  false : 还有没有确定的被转移的note
     */
    if(transfer_iterator->second->onNoteConfirm(transfer_data->transfer_note)){
        //设置完成转移参数
        onTransferResponse();
        //停止转移器
        transfer_iterator->second->onStopTransfer();
        //发送线程池SocketThread线程接收转移完成
        dynamic_cast<ThreadExecutor*>(executor_pool)->onTransferComplete(transfer_iterator->second->getTransferThread());
    }

}

/**
 * 被转移的note信息超时（规定时间内没有全部确定）（ExecutorThread线程）
 * @param transfer_info     管理被转移信息类
 */
void ExecutorLayer::onTransferTimeout(ExecutorTransferInfo *transfer_info) {
    //获取超时的被转移note的数量
    int timeout_size = transfer_info->onTimeoutTransfer();

    /*
     * 判断是否有超时没有确定被转移note的数量
     *  可能全部确定之后,停止转移器之前触发该函数
     */
    if(timeout_size > 0){
        //发送线程池SocketThread线程接收转移完成
        dynamic_cast<ThreadExecutor*>(executor_pool)->onTransferComplete(transfer_info->getTransferThread());
        onAddressTimeout(timeout_size, transfer_info);
    }
}

/**
 * 被转移的note信息超时实现类
 * @param timeout_size      超时数量
 * @param transfer_info     管理被转移信息类
 */
void ExecutorLayer::onAddressTimeout(int timeout_size, ExecutorTransferInfo *transfer_info) {
    //确定的超时note的数量
    int address_size = 0;
    //超时note的数组
    MeetingAddressNote *timeout_note[timeout_size];

    //调用管理被转移信息类接收超时的note
    transfer_info->onTimeoutNote(
            [&](MeetingAddressNote *note) -> bool {
                timeout_note[address_size++] = note;
                return (address_size >= timeout_size);
            });

    /*
     * 判断是否有确定的超时note的数量
     *  可能远程端note已退出并复位转移标志,但管理被转移信息类的转移数量没有减少,导致数量与实际的note不符
     */
    if(address_size > 0){
        BasicControl::callOutputOnStackMemory(sizeof(MsgHdr),
                                              [&](MsgHdr *timeout_msg) -> void {
                                                  //初始化信息
                                                  MsgHdrUtil<void>::initMsgHdr(timeout_msg, LAYER_MASTER_EXIT_PASSIVE);
                                                  //设置超时数量
                                                  timeout_msg->address_number = static_cast<short>(address_size);

                                                  //发送控制层
                                                  basic_control->onInput(timeout_msg, *timeout_note, *this, CONTROL_INPUT_FLAG_INPUT);
                                              });
    }
}