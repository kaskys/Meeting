//
// Created by abc on 21-2-24.
//

#include "TransmitLayer.h"

/**
 * 传输输入数据
 * @param fd                socket描述符
 * @param default_read_len  默认读取长度
 * @param transmit_util     SocketThread传输层工具
 */
void onTransmitInput(int fd, int default_read_len, TransmitThreadUtil *transmit_util){
    sockaddr_in input_addr_in{};
    TransmitInputData input_data{};
    char buffer[std::max(sizeof(TransmitMemoryDynamicUtil), sizeof(TransmitMemoryFixedUtil))];

    TransmitReadInfo read_info = { .fd = fd, .read_len = 0, .read_flag = 0, .read_error = 0, .addr = &input_addr_in,
                                   .input_data = reinterpret_cast<char*>(&input_data), .memory_util = buffer,
                                   .transmit_util = reinterpret_cast<char*>(transmit_util), .init_func =  onTransmitInit,
                                   .create_func = onTransmitBufferCreate, .read_func = onTransmitReadData,
                                   .transmit_func = onTransmitData, .uinit_func = nullptr};
    //读取输入数据
    read_data(&read_info, default_read_len);
}

/**
 * 输入读取数据（因为中心化主机一次读取可能多个远程端发送的数据,所以该函数对应一个远程端的输入数据）
 * @param info  读取工具
 * @param len   长度
 */
void onTransmitData(TransmitReadInfo *info, int len){
    auto store_data = reinterpret_cast<TransmitInputData*>(info->input_data);

    store_data->onTransmitInputData(reinterpret_cast<TransmitThreadUtil*>(info->transmit_util));
    store_data->initInputData(len, *info->addr, reinterpret_cast<TransmitMemoryUtil*>(info->memory_util)->onExtractMemory());
}


/**
 * 读取socket的输入数据
 * @param info          读取工具
 * @param alread_len    已读取的长度
 * @param addr_len      地址长度
 * @return              本次读取长度
 */
int onTransmitReadData(TransmitReadInfo *info, int alread_len, int addr_len){
    int read_data_len = 0;
    reinterpret_cast<TransmitMemoryUtil*>(info->memory_util)->onCopyMemory(alread_len,
                                                                           [&](char *copy_buffer) -> void {
                                                                               read_data_len = read_data0(info->fd, static_cast<size_t>(info->read_len - alread_len),
                                                                                                          info->read_flag, copy_buffer, info->addr, (socklen_t*)&addr_len);
                                                                           });

    return read_data_len;
}

int onTransmitInit(TransmitReadInfo *info){
#define TRANSMIT_INIT_RETURN_TRUE   1
#define TRANSMIT_INIT_RETURN_FALSE  0
    auto transmit_util = reinterpret_cast<TransmitThreadUtil*>(info->transmit_util);

    //判断socket输入数据长度及创建动态内存（这里是申请动态内存,但实际是动态或固定内存由控制层决定）
    if((info->read_len <= 0) || !transmit_util->onCreateDynamicBuffer(reinterpret_cast<TransmitMemoryUtil*>(info->memory_util), info->read_len)){
        transmit_util->onError(TRANSMIT_PARAMETER_UNSOCKET);
        return TRANSMIT_INIT_RETURN_FALSE;
    }else{
        info->uinit_func = onTransmitUinit;
        return TRANSMIT_INIT_RETURN_TRUE;
    }
}

void onTransmitUinit(TransmitReadInfo *info){
    reinterpret_cast<TransmitInputData*>(info->input_data)->onTransmitInputData(reinterpret_cast<TransmitThreadUtil*>(info->transmit_util));
    reinterpret_cast<TransmitMemoryUtil*>(info->memory_util)->~TransmitMemoryUtil();
}

/**
 * 创建缓存
 * @param info  读取工具
 * @return
 */
int onTransmitBufferCreate(TransmitReadInfo *info, int create_len){
    return reinterpret_cast<TransmitMemoryUtil*>(info->memory_util)->onCreateMemory(create_len);
}

/**
 * 向socket输入数据
 * @param fd        socket描述符
 * @param len       数据长度
 * @param addr      输出地址
 * @param msg_hdr   数据信息头
 * @return          已输入长度及错误代码
 */
std::pair<int,int> onTransmitOutput(int fd, int len, sockaddr_in *addr, MsgHdr *msg_hdr){
    //构造栈输入变量
    TransmitWriteInfo write_info = { .fd = fd, .write_len = len, .alsend_len = 0,
                                     .write_error = 0, .addr = addr,
                                     .write_buf = reinterpret_cast<char*>(msg_hdr) };
    //输入数据
    write_data(&write_info);
    //返回数据
    return {write_info.alsend_len, write_info.write_error};
}

//--------------------------------------------------------------------------------------------------------------------//

void TransmitInputData::onTransmitInputData(TransmitThreadUtil *transmit_util) {
    if(transmit_len > 0){  transmit_util->onSocketInput(this); }
}

//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

std::atomic<uint32_t> TransmitThreadUtil::only_id_generator{1};

TransmitThreadUtil::TransmitThreadUtil(sockaddr_in *transmit_addr) throw(std::runtime_error)
        : ThreadParameter(), TransmitCodecUtil(), thread_open_fd(0), thread_socket_recv_size(0),
          thread_socket_send_size(0), thread_correlate_id(only_id_generator.fetch_add(TRANSMIT_THREAD_ID_GENERATOR_VALUE)),
          thread_addr(), transmit_layer(nullptr), socket_thread(nullptr){
    onLaunchTransmit(transmit_addr);
}

TransmitThreadUtil::~TransmitThreadUtil() {
    onTerminateTransmit();
}

/**
 * 启动SocketThread的传输层工具（一般SocketThread都会提供地址信息（ip地址（用户选择）、端口号在绑定时由系统设置））
 * @param launch_addr   启动传输层地址信息
 * @param recv_size     接收缓存大小
 * @param send_size     发送缓存大小
 */
void TransmitThreadUtil::onLaunchTransmit(sockaddr_in *launch_addr, int recv_size, int send_size) throw(std::runtime_error){
    //启动传输前先关闭上一次的socket描述符
    onTerminateTransmit();

    //为SocketThread打开一个socket描述符
    if((thread_open_fd = open_socket()) <= 0){
        //打开失败,抛出异常
        throw std::runtime_error("open socket error!");
    }

    //判断地址信息或socket描述符绑定地址信息失败
    if(!launch_addr || bind_socket(launch_addr, thread_open_fd)){
        //绑定失败,抛出异常
        throw std::runtime_error("bind socket error!");
    }

    thread_addr = *launch_addr;

    if(recv_size > 0){
        //设置接收缓存大小
        setSocketRecvSize(recv_size);
    }

    if(send_size > 0){
        //设置发送缓存大小
        setSocketSendSize(send_size);
    }
}


/**
 * 停止SocketThread的传输层工具
 */
void TransmitThreadUtil::onTerminateTransmit() {
    //关闭socket描述符
    close_socket(thread_open_fd);
}

/**
 * 输入数据到达,读取输入数据（由SocketWordThread调用）
 */
void TransmitThreadUtil::onSocketRead() {
    onTransmitInput(thread_open_fd, thread_socket_recv_size, this);
}

/**
 * 将读取的输入数据发送给传输层
 * @param memory_util   内存工具
 * @param addr          输入地址
 * @param len           输入长度
 */
void TransmitThreadUtil::onSocketInput(TransmitInputData *data) {
    transmit_layer->onInputLayer(getThreadParameter(), this, data);
}

bool TransmitThreadUtil::onSocketInputVerify(MeetingAddressNote *note) const {
    return socket_thread->verifyPermitThread(note);
}

/**
 * 可以输出数据,向socket描述符输出数据（由SocketWordThread调用）
 */
void TransmitThreadUtil::onSocketWrite(const TransmitOutputData &write_data) {
    int output_max = write_data.output_max;
    sockaddr_in addr = transmit_layer->onNoteAddress(write_data.output_note);

    //循环发送,直到发送成功或某些错误时,返回
    for(; output_max > 0; output_max--){
        if(onSocketWrite0(onTransmitOutput(thread_open_fd, write_data.output_len, &addr, write_data.output_msg),
                           write_data.output_len, write_data.output_msg, write_data.done_func, write_data.cancel_func)){
            break;
        }
    }

    if(output_max <= 0){
        onError(TRANSMIT_PARAMETER_OUTPUT_MAX);
    }
}

/**
 * 传输层输出数据（由传输层调用）
 * @param msg_hdr       数据信息头
 * @param output_note   输出远程端
 * @param done_func     输出旺财函数
 * @param cancel_func   取消任何函数
 * @param len           输出长度
 */
void TransmitThreadUtil::onSocketOutput(MsgHdr *msg_hdr, MeetingAddressNote *output_note, void(*done_func)(TransmitParameter*, MsgHdr*),
                                                                 void(*cancel_func)(MsgHdr*), int len, int output_max) {
    socket_thread->writeData(TransmitOutputData{ .output_len = len, .output_max = output_max,
                                                 .output_msg = msg_hdr, .output_note = output_note,
                                                 .done_func = done_func, .cancel_func = cancel_func} );
}

/**
 * 传输层输出数据结束
 * @param error         已输入长度及错误信息
 * @param output_msg    需要输出长度及回调信息
 * @param done_func     输出完成函数
 * @param cancel_func   取消任务函数
 * @return              是否结束输出循环
 */
bool TransmitThreadUtil::onSocketWrite0(std::pair<int, int> error, int output_len, MsgHdr *output_msg,
                                         void(*done_func)(TransmitParameter*, MsgHdr*), void(*cancel_func)(MsgHdr*)) {

    if(!error.first){
        //输出失败,判断错误信息
        switch (error.second){
            //参数错误,取消任务,接受储存循环
            case EINVAL:
                if(cancel_func) { cancel_func(output_msg); }
                return true;
            //发送缓存小于输出数据大小,设置输出缓存大小为发送缓存大小,重新输出
            case EMSGSIZE:
                setSocketSendSize(output_len);
                break;
            //socket描述符已经链接某个远程端（udp不需要链接）,断开链接,重新输出
            case EISCONN:
                onDisconnect();
            default:
            //忽略其他错误,重新输出
                break;
        }
        return false;
    }else{
        //输出成功,判断是否输出完毕并结束循环
        if(error.first == output_len){
            //输出完成,回调函数
            if(done_func) {
                (*done_func)(dynamic_cast<TransmitParameter*>(this), output_msg);
            }
        }else {
            //输出失败（已输出数据大小小于输出数据大小）
            onError(TRANSMIT_PARAMETER_UNWRITE);
        }
        return true;
    }
}

void TransmitThreadUtil::correlateThread(SocketWordThread *socket_thread) {
    (this->socket_thread = socket_thread)->correlateTransmit(this);
}

void TransmitThreadUtil::correlateRemoteNote(MeetingAddressNote *note) {
    getTransmitCompileUtil(MeetingAddressManager::getNotePermitPos(note))->onClearRemoteCompile();
}

void TransmitThreadUtil::onRunCorrelateThread0(const std::function<void()> &func) {
    if(socket_thread){
        socket_thread->onRunSocketThread(func, SOCKET_FUNC_TYPE_DEFAULT);
    }
}

void TransmitThreadUtil::onRunCorrelateThreadImmediate(const std::function<void()> &func) {
    if(socket_thread){
        socket_thread->onRunSocketThread(func, SOCKET_FUNC_TYPE_IMMEDIATELY);
    }
}

bool TransmitThreadUtil::isRunCorrelateThread() const {
    return socket_thread->isCorrelateThread();
}

bool TransmitThreadUtil::isThreadCorrelateRemote(MeetingAddressNote *note) const {
    return socket_thread->verifyPermitNote(note);
}

TransmitThreadUtil* TransmitThreadUtil::onTransmitThreadUtil(SocketWordThread *socket_thread) {
    return socket_thread->correlateTransmit();
}

/**
 * 创建动态内存（由传输层实现）
 * @param memory_util   内存工具（动态）
 * @param len           申请长度
 * @return
 */
bool TransmitThreadUtil::onCreateDynamicBuffer(TransmitMemoryUtil *memory_util, int len) {
    return TransmitLayer::onTransmitCreateBuffer(getThreadParameter(), transmit_layer, len,
                                         [&](InitLocator &init_locator) -> void {
                                             new (dynamic_cast<TransmitMemoryDynamicUtil*>(memory_util))
                                                     TransmitMemoryDynamicUtil(len, init_locator);
                                      }, [&](char *fixed_buffer) -> void {
                                             new (dynamic_cast<TransmitMemoryFixedUtil*>(memory_util))
                                                     TransmitMemoryFixedUtil(len, fixed_buffer, this);
                                      });
}

/**
 * 创建固定内存（由传输层实现）
 * @param memory_util   内存工具（固定）
 * @param len           申请长度
 */
void TransmitThreadUtil::onCreateFixedBuffer(TransmitMemoryUtil *memory_util, int len) {
    using FixedFunc = void(*)(TransmitThreadUtil*, TransmitMemoryUtil*, char *buffer, int len);
    TransmitLayer::onTransmitCreateBuffer(getThreadParameter(), transmit_layer, len, nullptr,
                                          [&](char *fixed_buffer) -> void {
                                              dynamic_cast<TransmitMemoryFixedUtil*>(memory_util)->
                                                      setCreateFixedBuffer(len, fixed_buffer, std::bind((FixedFunc)&TransmitThreadUtil::onDestroyFixedBuffer,
                                                                                                        this, memory_util, std::placeholders::_1, std::placeholders::_2));
                                          });
}

/**
 * 释放固定内存（动态内存自动释放）
 * @param buffer    内存
 * @param len       长度
 */
void TransmitThreadUtil::onDestroyFixedBuffer(TransmitMemoryUtil*, char *buffer, int len) {
    TransmitLayer::onTransmitDestroyBuffer(getThreadParameter(), transmit_layer, buffer, len);
}

/**
 * 终结函数
 * @param note
 */
void TransmitThreadUtil::onNoteTerminationFunc(MeetingAddressNote *note) {
    transmit_layer->onNoteTermination(note);
}

/**
 * 输入或输出错误
 * @param type  错误类型
 */
void TransmitThreadUtil::onError(int type) {
    TransmitLayer::onError(getThreadParameter(), type);
}

/**
 * 设置接收缓存大小
 * @param size  大小
 */
void TransmitThreadUtil::setSocketRecvSize(int size) {
    if(size > 0){
        TransmitReadInfo info = { .fd = thread_open_fd, .read_len = size, .read_flag = 0,
                                  .read_error = 0, .addr = nullptr, .input_data = nullptr,
                                  .memory_util = nullptr, .transmit_util = nullptr, .init_func = nullptr,
                                  .create_func = nullptr, .read_func = nullptr, .transmit_func = nullptr,
                                  .uinit_func = nullptr };
        if(set_recv_size(&info)){
            thread_socket_recv_size = info.read_len;
        }
    }
}

/**
 * 设置发送缓存大小
 * @param size 大小
 */
void TransmitThreadUtil::setSocketSendSize(int size) {
    if(size > 0){
        TransmitWriteInfo info = { .fd = thread_open_fd, .write_len = size, .alsend_len = 0,
                                   .write_error = 0, .addr = nullptr, .write_buf = nullptr };
        if(set_send_size(&info)){
            thread_socket_send_size = info.write_len;
        }
    }
}



//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

ThreadParameter::ThreadParameter() : valid(true), prev(nullptr), next(nullptr), parameter() {
    if((next = parameter_link.load(std::memory_order_consume))) {
        next->prev = this;
    }
    parameter_link.store(this, std::memory_order_release);
}

ThreadParameter::~ThreadParameter() {
    valid.store(false, std::memory_order_release);
    if(prev){
        prev->next = next;
    }else{
        parameter_link.store(next, std::memory_order_release);
    }

    {
        std::unique_lock<std::mutex> ulock(history_lock, std::try_to_lock);
        history_parameter.onFuseData(this->parameter);
    }
}

//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

bool TransmitMemoryDynamicUtil::onCreateMemory(int) {
    //不需要实现
    return true;
}

void TransmitMemoryDynamicUtil::onCopyMemory(int len, const std::function<void(char*)> &callback) throw(std::logic_error) {
    if(len < memory_len){
        throw std::logic_error(nullptr);
    }

    dynamic_reader = dynamic_locator.locatorCorrelate(static_cast<uint32_t>(len));
    callback(dynamic_reader.readMemory<char*>(false));
    dynamic_locator.releaseBuffer();
}

MemoryReader TransmitMemoryDynamicUtil::onExtractMemory() {
    //这里返回拷贝构造另一个MemoryReader（不使用移动构造）,变量dynamic_reader还引用一个MemoryReader,防止因没有资源引用MemoryReader而动态内存被释放
    return dynamic_reader;
}

//--------------------------------------------------------------------------------------------------------------------//

bool TransmitMemoryFixedUtil::onCreateMemory(int create_len) {
    if(!fixed_buffer){
        transmit_util->onCreateFixedBuffer(this, (memory_len = create_len));
    }

    return static_cast<bool>(fixed_buffer);
}

void TransmitMemoryFixedUtil::onCopyMemory(int len, const std::function<void(char*)> &callback) throw(std::logic_error) {
    if(len < memory_len){
        throw std::logic_error(nullptr);
    }

    callback(fixed_buffer);
}

MemoryReader TransmitMemoryFixedUtil::onExtractMemory() {
    char *extract_buffer = fixed_buffer;
    fixed_buffer = nullptr;
    return MemoryReader(static_cast<uint32_t>(memory_len), extract_buffer, fixed_func);
}

