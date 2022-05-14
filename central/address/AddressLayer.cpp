//
// Created by abc on 20-12-8.
//

#include "../BasicControl.h"

//--------------------------------------------------------------------------------------------------------------------//

static HostStatusUtil      host_util{NOTE_STATUS_HOST};
static RemoteStatusUtil    remote_util{NOTE_STATUS_REMOTE};
static JoinStatusUtil      join_util{NOTE_STATUS_JOIN};
static LinkStatusUtil      link_util{NOTE_STATUS_LINK};
static SequenceStatusUtil  sequence_util{NOTE_STATUS_SEQUENCE};
static NormalStatusUtil    normal_util{NOTE_STATUS_NORMAL};
static PassiveStatusUtil   passive_util{NOTE_STATUS_EXIT_PASSIVE};
static ExitStatusUtil      exit_util{NOTE_STATUS_EXIT};

//--------------------------------------------------------------------------------------------------------------------//

BasicLayer* AddressLayerUtil::createLayer(BasicControl *control) noexcept {
    BasicLayer *layer = control->onCoreControl() ? reinterpret_cast<BasicLayer*>(new (std::nothrow) MasterAddressLayer(control))
                                                 : reinterpret_cast<BasicLayer*>(new (std::nothrow) ServantAddressLayer(control));
    return layer;
}

void AddressLayerUtil::destroyLayer(BasicControl*, BasicLayer *layer) noexcept {
    delete layer;
}

//--------------------------------------------------------------------------------------------------------------------//

AddressLayer::AddressLayer(BasicControl *control) : BasicLayer(control), join_time(0), time_size(0), passive_time(0),
                                                    address_manager(nullptr), max_indicator_time(0), host_note(nullptr), layer_parameter(){
    initLayer();
}

AddressLayer::~AddressLayer() {
    onDestroyManager();
}

void AddressLayer::initLayer() {
    status_util[NOTE_STATUS_HOST]           = &host_util;
    status_util[NOTE_STATUS_REMOTE]         = &remote_util;
    status_util[NOTE_STATUS_JOIN]           = &join_util;
    status_util[NOTE_STATUS_LINK]           = &link_util;
    status_util[NOTE_STATUS_SEQUENCE]       = &sequence_util;
    status_util[NOTE_STATUS_NORMAL]         = &normal_util;
    status_util[NOTE_STATUS_EXIT_PASSIVE]   = &passive_util;
    status_util[NOTE_STATUS_EXIT]           = &exit_util;
    layer_parameter.onClear();
}

bool AddressLayer::onCreateManager() {
    address_manager = new (std::nothrow) MeetingAddressManager(onFixedCreateFunc(this), onFixedCreateFunc(this),
                                                               [&](MeetingAddressNote *note, MeetingAddressNote *fixed) -> void {
                                                                   onNoteRemoveFunc(this, note, fixed);
                                                               });
    return static_cast<bool>(address_manager);
}

void AddressLayer::onDestroyManager() {
    delete address_manager; address_manager = nullptr;
}

bool AddressLayer::onStart(MsgHdr *start_msg) {
    auto start_info = reinterpret_cast<AddressInitInfo*>(start_msg->buffer);

    if(!start_info
       || (start_info->init_addr.sin_addr.s_addr == INADDR_ANY)
       || (start_info->init_addr.sin_addr.s_addr == INADDR_NONE)
       || !onCreateManager()){
        return false;
    }

    join_time = start_info->jtime; passive_time = start_info->ptime; time_size = start_info->time_size;
    max_indicator_time = start_info->init_indicator_time;
    onAddressTime(ADDRESS_LAYER_LINK_TIME, start_info->ltime);
    onAddressTime(ADDRESS_LAYER_SEQUENCE_TIME, start_info->stime);

    start_info->init_addr.sin_port = 0;
    start_msg->master_type = LAYER_MASTER_CREATE;
    start_msg->shared_type = NOTE_STATUS_HOST;
    onInput(start_msg);

    if((start_msg->serial_number < sizeof(HostNoteInfo*)) || !(host_note = reinterpret_cast<HostNoteInfo*>(start_msg->buffer))){
        onDestroyManager();
        return false;
    }else{
        using VerifyFunc = std::function<void(MsgHdr*, MeetingAddressNote*)>;
        MsgHdrUtil<VerifyFunc>::initMsgHdr(start_msg, start_msg->serial_number, TRANSMIT_LAYER_VERIFY,
                                              std::bind(&AddressLayer::onNoteVerify, this, std::placeholders::_1, std::placeholders::_2));
        basic_control->onLayerCommunication(start_msg, LAYER_TRANSMIT_TYPE);
        return true;
    }
}

void AddressLayer::onStop() {
    join_time = 0; time_size = 0; passive_time = 0;
    max_indicator_time = 0;
    host_note = nullptr;
    layer_parameter.onClear();
    onDestroyManager();
}

MeetingAddressNote* AddressLayer::onNoteCreateFunc(AddressLayer *layer, uint32_t create_size) {
    return reinterpret_cast<MeetingAddressNote*>(layer->onCreateBuffer(create_size));
}

MeetingAddressNote* AddressLayer::onFixedCreateFunc(AddressLayer *layer) {
    return reinterpret_cast<MeetingAddressNote*>(layer->onCreateBuffer(sizeof(MeetingAddressNote)));
}

void AddressLayer::onNoteRemoveFunc(AddressLayer *layer, MeetingAddressNote *note_info, MeetingAddressNote *fixed_info) {
    if(note_info){ layer->onNoteDestroy(note_info); }
    if(fixed_info){ layer->onNoteDestroy(fixed_info); }
}

/**
 * 输入创建远程或本机节点
 * input_msg的变量定义
 *  serial_number => sizeof(sockaddr_in)
 *  shared_type   => StatusNote(REMOTE、HOST)
 *  response_type => LAYER_MASTER_INIT表示远程端的端口号（port）
 *  buffer        => sockaddr_in
 * @param input_msg
 */
void AddressLayer::onInput(MsgHdr *input_msg) {
    static auto error_func = [=](uint32_t &number) -> void {
        number = 0;
    };

    switch(input_msg->master_type){
        case LAYER_MASTER_CREATE:     //主socket线程处理
            AddressStatusUtil::onNoteCreate(this, input_msg, status_util[input_msg->shared_type]);
            break;
        case LAYER_MASTER_INIT:       //主socket线程处理
            AddressStatusUtil::onNoteInit(this, input_msg, status_util[input_msg->shared_type]);
            break;
        default:
            error_func(input_msg->serial_number);
            break;
    }
}

void AddressLayer::onInputLayer(MsgHdr *input_msg, AddressNoteInfo *note_info, const StatusFunc &input_func) {
    if(input_func){
        input_func(status_util[note_info->getStatusNote()], input_msg, this, note_info);
    }else{
        onReplyError(note_info, NOTE_ERROR_TYPE_UNSUPPORT, note_info->onNoteAttributeStatus());
    }
}

void AddressLayer::onNoteCreate(MsgHdr *msg, AddressNoteInfo *note_info, AddressStatusUtil *init_util){
    auto create_func = [&](const sockaddr_in &addr, uint32_t link_size) -> void {
        if(!(note_info = dynamic_cast<AddressNoteInfo*>(
                onNoteCreateFunc(this, sizeof(AddressNoteInfo) + (init_util->onUtilLinkInfo() * sizeof(uint32_t) * link_size))))){
            msg->serial_number = 0;
        }else{
            try {
                onNotePush(note_info);
                init_util->onInputCreate(this, note_info, addr.sin_port, addr.sin_addr.s_addr, link_size);
                MsgHdrUtil<AddressNoteInfo*>::initMsgHdr(msg, sizeof(AddressNoteInfo*), static_cast<uint32_t>(msg->master_type), note_info);
            }catch (std::bad_alloc &e){
                onNoteDestroy(dynamic_cast<MeetingAddressNote*>(note_info));
                msg->serial_number = 0;
            }
        }
    };

    if(init_util && (msg->serial_number >= sizeof(sockaddr_in))){
        create_func(*reinterpret_cast<sockaddr_in*>(msg->buffer), time_size);
    }else{
        msg->serial_number = 0;
    }
}

void AddressLayer::onNoteInit(MsgHdr *msg, AddressStatusUtil *init_util) {
    init_util->onInputInit(msg, this, reinterpret_cast<AddressNoteInfo*>(msg->buffer), static_cast<uint16_t>(msg->response_type));
}

void AddressLayer::onHostNoteInit(MsgHdr *msg, HostNoteInfo *note_info) {
    onNoteInputInit(msg, host_note = note_info);
}

void AddressLayer::onRemoteNoteInit(MsgHdr *msg, RemoteNoteInfo *note_info) {
    onNoteInputInit(msg, note_info);
}

void AddressLayer::onNoteUninit(MsgHdr *msg, AddressNoteInfo *note_info, AddressStatusUtil*) {
    if(!note_info){
        msg->serial_number = 0;
    }else{
        status_util[note_info->onNoteAttributeStatus()]->onInputUninit(msg, this, note_info);
    }
}

void AddressLayer::onHostNoteUninit(MsgHdr*, HostNoteInfo *note_info) {
    host_note = nullptr;
    onNoteInputUninit(nullptr, note_info);
}

void AddressLayer::onRemoteNoteUninit(MsgHdr*, RemoteNoteInfo *note_info) {
    onNoteInputUninit(nullptr, note_info);
}

void AddressLayer::onNoteDestroy(MeetingAddressNote *note_info) {
    uint32_t destroy_size = note_info->man_b ? sizeof(MeetingAddressNote)
                                             : ((dynamic_cast<AddressNoteInfo*>(note_info)->onNoteAttributeStatus() == NOTE_STATUS_HOST)
                                                ? sizeof(HostNoteInfo)
                                                : sizeof(RemoteNoteInfo) + (sizeof(uint32_t) * dynamic_cast<RemoteNoteInfo*>(note_info)->getLinkSize()));

    onDestroyBuffer(sizeof(char*),
                    [&](MsgHdr *destroy_msg) -> void {
                        MsgHdrUtil<char*>::initMsgHdr(destroy_msg, destroy_size, LAYER_CONTROL_REQUEST_DESTROY_FIXED,
                                                      reinterpret_cast<char*>(note_info));
                    });
}

void AddressLayer::onNotePush(AddressNoteInfo *note_info) throw(std::bad_alloc) {
    address_manager->pushNote(dynamic_cast<MeetingAddressNote*>(note_info),
                              [&]() -> MeetingAddressNote* {
                                  return onFixedCreateFunc(this);
                              });
}

/**
 * 主socket线程
 * @param input_msg
 * @param note_info
 */
void AddressLayer::onNoteReuse(MsgHdr*, AddressNoteInfo *note_info) {
    note_info->setTerminationFunc(onNoteTermination());
    address_manager->reuseNote(dynamic_cast<MeetingAddressNote*>(note_info));
}

void AddressLayer::onNoteUnuse(MsgHdr*, AddressNoteInfo *note_info) {
    note_info->setTerminationFunc(nullptr);
    address_manager->removeNote(dynamic_cast<MeetingAddressNote*>(note_info));
}

void AddressLayer::onNoteInputJoin(MsgHdr *input_msg, AddressNoteInfo *note_info) {
    onInputLayer(input_msg, note_info, getInputFunc(&AddressStatusUtil::onInputJoin));
}

void AddressLayer::onNoteInputLink(MsgHdr *input_msg, AddressNoteInfo *note_info) {
    onInputLayer(input_msg, note_info, getInputFunc(&AddressStatusUtil::onInputLink));
}

void AddressLayer::onNoteInputSynchro(MsgHdr *input_msg, AddressNoteInfo *note_info) {
    onInputLayer(input_msg, note_info, getInputFunc(&AddressStatusUtil::onInputSynchro));
}

void AddressLayer::onNoteInputSequence(MsgHdr *input_msg, AddressNoteInfo *note_info) {
    onInputLayer(input_msg, note_info, getInputFunc(&AddressStatusUtil::onInputNormal));
}

void AddressLayer::onNoteInputNormal(MsgHdr*, AddressNoteInfo*) {
    //不需要处理
}

void AddressLayer::onNoteInputTransfer(MsgHdr *input_msg, AddressNoteInfo *note_info) {
    if(input_msg->serial_number >= sizeof(sockaddr_in)){
        note_info->onNoteTransfer(*reinterpret_cast<sockaddr_in*>(input_msg->buffer));
    }
}

/**
 * 驱动线程             ==>    判断被动状态 -> 提交SocketThread线程
 * SocketThread线程    ==>                                        ->   调用onNoteInputPassive函数  ->  正常数据输入   （1）
 * SocketThread线程    ==>                                        ->   正常数据输入  -> 调用onNoteInputPassive函数    （2）
 * SocketThread线程    ==>                                        ->   退出请求输入  -> 调用onNoteInputPassive函数    （3）
 * 对于（1）情况可直接转正常（Normal）状态处理
 * 对于（2）情况需要判断被动（Passive）状态及正常（Normal）状态的序号 :
 *  1: passive_sequence > normal_sequence => 变更被动（Passive）状态处理
 *  2: passive_sequence < normal_sequence => 无需处理
 *  3: 退出状态
 * 对于（3）情况NoteInfo处于退出状态(但不删除)
 * @param passive_msg
 * @param note_info
 */
void AddressLayer::onNoteInputPassive(MsgHdr *input_msg, AddressNoteInfo *note_info) {
    if((note_info->getStatusNote() != NOTE_STATUS_EXIT) && (input_msg->serial_number > note_info->getLastSequence())){
        onInputLayer(input_msg, note_info, getInputFunc(&AddressStatusUtil::onInputPassive));
    }
}

void AddressLayer::onNoteInputExit(MsgHdr *input_msg, AddressNoteInfo *note_info) {
    onInputLayer(input_msg, note_info, getInputFunc(&AddressStatusUtil::onInputExit));
}

/**
 * 通知控制层远程端初始化
 * @param init_msg  该msg是控制层的栈变量,非输入的变量msg
 * @param note_info 远程端
 */
void AddressLayer::onNoteInputInit(MsgHdr *init_msg, AddressNoteInfo *note_info) {
    init_msg->master_type = LAYER_MASTER_INIT;
    basic_control->onInput(init_msg, note_info, *this, CONTROL_INPUT_FLAG_INPUT);
}

/**
 * 通知控制层远程端因退出（主动、被动）而析初始化
 * @param uinit_msg 该msg是输入的变量,但退出时master类型（唯一）,将EXIT修改为UNINIT不会影响后续运行
 * @param note_info 远程端
 */
void AddressLayer::onNoteInputUninit(MsgHdr *uinit_msg, AddressNoteInfo *note_info){
    uinit_msg->master_type = LAYER_MASTER_UNINIT;
    basic_control->onInput(uinit_msg, note_info, *this, CONTROL_INPUT_FLAG_INPUT);
}

void AddressLayer::onNoteInputJoin0(MsgHdr *msg, AddressNoteInfo *note_info) {
    onNoteStatusUpdate(msg, note_info, LAYER_CONTROL_NOTE_JOIN);
}

void AddressLayer::onNoteInputLink0(MsgHdr *msg, AddressNoteInfo *note_info) {
    onNoteStatusUpdate(msg, note_info, LAYER_CONTROL_NOTE_LINK);
}

void AddressLayer::onNoteInputSynchro0(MsgHdr*, AddressNoteInfo*) {
    //不需要处理
}

void AddressLayer::onNoteInputSequence0(MsgHdr *msg, AddressNoteInfo *note_info) {
    onNoteStatusUpdate(msg, note_info, LAYER_CONTROL_NOTE_SEQUENCE);
}

void AddressLayer::onNoteInputNormal0(MsgHdr *msg, AddressNoteInfo *note_info) {
    onNoteStatusUpdate(msg, note_info, LAYER_CONTROL_NOTE_NORMAL);
}

void AddressLayer::onNoteInputPassive0(MsgHdr *msg, AddressNoteInfo *note_info) {
    onNoteStatusUpdate(msg, note_info, LAYER_CONTROL_NOTE_PASSIVE);
}

void AddressLayer::onNoteInputExit0(MsgHdr *msg, AddressNoteInfo *note_info) {
    onNoteStatusUpdate(msg, note_info, LAYER_CONTROL_NOTE_EXIT);
}

/**
 * 通知控制层更改远程端在显示状态
 * @param note_info     远程端
 * @param status_type   显示状态
 */
void AddressLayer::onNoteStatusUpdate(MsgHdr*, AddressNoteInfo *note_info, int status_type) {
    /*
     * 参数MsgHdr是输入的，这里修改会影响后续运行,所有需要构造栈变量
     */
    BasicControl::callOutputOnStackMemory(sizeof(MsgHdr),
                                          [&](MsgHdr *update_msg) -> void {
                                              update_msg->master_type = LAYER_MASTER_UPDATE;
                                              update_msg->response_type = static_cast<short>(status_type);
                                              basic_control->onInput(update_msg, note_info, *this, CONTROL_INPUT_FLAG_INPUT);
                                          });
}

void AddressLayer::onNoteInvalid(AddressNoteInfo *note_info) {
    layer_parameter.onInvalid();
    note_info->onNoteInvalid();
}

void AddressLayer::onReplyError(AddressNoteInfo *info, ErrorType error_type, StatusNote error_status) {
    onReplyError(address_manager->getSockAddrFromNote(dynamic_cast<MeetingAddressNote*>(info)), error_type, error_status);
}

void AddressLayer::onReplyError(sockaddr_in addr, ErrorType error_type, StatusNote error_status) {
    BasicControl::callOutputOnStackMemory(sizeof(MsgHdr) + sizeof(ErrorInfo),
                                          [&](MsgHdr *error_msg) -> void {
                                              ErrorInfo error_info{addr, error_type, error_status};
                                              error_msg = MsgHdrUtil<ErrorInfo>::initMsgHdr(error_msg, sizeof(ErrorInfo), LAYER_CONTROL_ADDRESS_ERROR, error_info);
                                              basic_control->onInput(error_msg, nullptr, *this, CONTROL_INPUT_FLAG_CONTROL);
                                          });
}

void AddressLayer::onOutput() {} //不用实现

void AddressLayer::onOutputLayer(MsgHdr *output_msg, AddressNoteInfo *note_info) {
    onNoteInputNormal0(output_msg, note_info);
}

void AddressLayer::onDrive(MsgHdr*) {}

void AddressLayer::onParameter(MsgHdr *msg) {
    uint32_t parameter_len = msg->serial_number;

    if(parameter_len < std::max(sizeof(ParameterRemote), sizeof(ParameterAddress))){
        msg->serial_number = 0;
    }else{
        switch (msg->shared_type){
            case TRANSMIT_LAYER_ADDRESS_PARAMETER:
                if(parameter_len >= sizeof(ParameterAddress)) {
                    onParameterAddress(reinterpret_cast<ParameterAddress*>(msg->buffer));
                }else{
                    msg->serial_number = 0;
                }
                break;
            case TRANSMIT_LAYER_ALL_REMOTE_PARAMETER:
                if(parameter_len >= sizeof(ParameterRemote*)){
                    onParameterNote(reinterpret_cast<ParameterRemote*>(msg->buffer));
                }else{
                    msg->serial_number = 0;
                }
                break;
            case TRANSMIT_LAYER_SINGLE_REMOTE_LEN_PARAMETER:
                if(parameter_len >= sizeof(AddressNoteInfo*) && (reinterpret_cast<AddressNoteInfo*>(msg->buffer)->onNoteAttributeStatus() != NOTE_STATUS_HOST)) {
                    MsgHdrUtil<uint32_t>::initMsgHdr(msg, sizeof(uint32_t), static_cast<uint32_t>(msg->master_type),
                                                       reinterpret_cast<RemoteNoteInfo*>(msg->buffer)->onNoteRttSize());
                }else{
                    msg->serial_number = 0;
                }
                break;
            case TRANSMIT_LAYER_SINGLE_REMOTE_PARAMETER:
                if(parameter_len >= sizeof(ParameterRemote)){
                    onParameterNote(reinterpret_cast<ParameterRemote*>(msg->buffer),
                                    reinterpret_cast<RemoteNoteInfo*>(reinterpret_cast<ParameterRemote*>(msg->buffer)->note_rtt_value));
                }else{
                    msg->serial_number = 0;
                }
                break;
            case TRANSMIT_LAYER_HOST_PARAMETER:
                if(parameter_len >= sizeof(ParameterHost)) {
                    onParameterNote(reinterpret_cast<ParameterHost*>(msg->buffer));
                }else{
                    msg->serial_number = 0;
                }
            default:
                break;
        }
    }
}

void AddressLayer::onControl(MsgHdr *msg) {
    auto control_len = static_cast<uint32_t>(msg->serial_number), control_type = static_cast<uint32_t>(msg->master_type);
    AddressNoteInfo *note_info = nullptr;

    switch (control_type){
        case LAYER_CONTROL_STATUS_STOP:
            onStop();
            break;
        case LAYER_CONTROL_STATUS_START:
            if(control_len < sizeof(AddressInitInfo) || !onStart(msg)){
                msg->master_type = LAYER_CONTROL_STATUS_THROW;
            }
            break;
        case LAYER_CONTROL_NOTE_TERMINATION:
            if((control_len >= sizeof(AddressNoteInfo*))){ reinterpret_cast<AddressNoteInfo*>(msg->buffer)->callTerminationFunc(); }
            break;
        case ADDRESS_LAYER_IS_BLACK:
            if((control_len < sizeof(AddressNoteInfo*)) || !(note_info = reinterpret_cast<AddressNoteInfo*>(msg->buffer))
                                                        || (note_info->onNoteAttributeStatus() == NOTE_STATUS_HOST)
                                                        || dynamic_cast<RemoteNoteInfo*>(note_info)->getBlackNote()){
                msg->serial_number = 0;
            }
            break;
        case ADDRESS_LAYER_SET_BLACK:
            if((control_len >= sizeof(AddressNoteInfo)) && (note_info = reinterpret_cast<AddressNoteInfo*>(msg->buffer))
                                                        && (note_info->onNoteAttributeStatus() != NOTE_STATUS_HOST)) {
                onAddressBlack(dynamic_cast<RemoteNoteInfo*>(note_info), true);
            }
            break;
        case ADDRESS_LAYER_CLR_BLACK:
            if((control_len >= sizeof(AddressNoteInfo)) && (note_info = reinterpret_cast<AddressNoteInfo*>(msg->buffer))
                                                                  && (note_info->onNoteAttributeStatus() != NOTE_STATUS_HOST)) {
                onAddressBlack(dynamic_cast<RemoteNoteInfo*>(note_info), false);
            }
            break;
        case ADDRESS_LAYER_SET_FILTER:
            if(control_len >= sizeof(NoteFilterInfo)) { onAddressFilter(*reinterpret_cast<NoteFilterInfo*>(msg->buffer)); }
            break;
        case ADDRESS_LAYER_JOIN_TIME:
        case ADDRESS_LAYER_LINK_TIME:
        case ADDRESS_LAYER_SEQUENCE_TIME:
        case ADDRESS_LAYER_PASSIVE_TIME:
            if(control_len >= sizeof(uint32_t)) { onAddressTime(control_type , *reinterpret_cast<uint32_t*>(msg->buffer)); }
            break;
        case ADDRESS_LAYER_TIME_SIZE:
            if(control_len >= sizeof(uint32_t)) { time_size = *reinterpret_cast<uint32_t*>(msg->buffer); }
            break;
        default:
            break;
    }
}

bool AddressLayer::onFilterAddress(AddressNoteInfo *note_info, FilterInfo filter) {
    auto remote_info = dynamic_cast<RemoteNoteInfo*>(note_info);
    return ((note_info->onNoteAttributeStatus() == NOTE_STATUS_HOST) || remote_info->onFilterMaster(filter.master_type)
            || remote_info->onFilterShared(filter.shared_type) || remote_info->onFilterResponse(filter.response_type));
}

AddressNoteInfo* AddressLayer::onMatchAddress(const sockaddr_in &addr) {
    return dynamic_cast<AddressNoteInfo*>(address_manager->matchNote(addr));
}

void AddressLayer::onAddressFilter(NoteFilterInfo info) {
    dynamic_cast<RemoteNoteInfo*>(info.note_info)->setFilter(info.filter_info);
}

void AddressLayer::onAddressBlack(RemoteNoteInfo *note_info, bool is_black) {
    is_black ? note_info->setBlackNote() : note_info->clrBlackNote();
}

void AddressLayer::onParameterAddress(ParameterAddress *address) {
    layer_parameter.onData(address);
}

void AddressLayer::onParameterNote(ParameterRemote *note_parameter) {
    if(note_parameter->value_size <= 0){
        return;
    }
    uint32_t pos = 0, size = note_parameter->value_size;
    address_manager->ergodicNote(MEETING_ADDRESS_ERGODIC_RUNNING,
                                 [&](MeetingAddressNote *note_info) -> void {
                                     if(note_info){
                                         if(dynamic_cast<AddressNoteInfo*>(note_info)->getStatusNote() == NOTE_STATUS_HOST){
                                             return;
                                         }
                                         if(pos >= size){
                                             return;
                                         }
                                         onParameterNote(note_parameter + pos, dynamic_cast<RemoteNoteInfo*>(note_info));
                                         pos++;
                                     }
                                });
}

void AddressLayer::onParameterNote(ParameterRemote *note_parameter, RemoteNoteInfo *note_info) {
    note_info->onData(note_parameter);
}

void AddressLayer::onParameterNote(ParameterHost *note_parameter) {
    if(host_note){ host_note->onData(note_parameter); }
}

void AddressLayer::initMoreTimerInfo(int type, uint32_t executor_time, uint32_t executor_amount, const std::function<void()> &executor_func,
                                     const std::function<void()> &timeout_func, const std::function<void(MsgHdr*)> &callback) {
    BasicControl::callOutputOnStackMemory(sizeof(MsgHdr) + sizeof(NoteTimerInfo),
                                          [&](MsgHdr *timer_msg) -> void {
                                              MsgHdrUtil<NoteTimerInfo>::initMsgHdr(timer_msg, sizeof(NoteTimerInfo), LAYER_CONTROL_REQUEST_TIMER,
                                                      NoteTimerInfo::initNoteTimerInfo(type, executor_amount, executor_time, 0, executor_func, timeout_func));
                                              timer_msg->response_type = LAYER_CONTROL_TIMER_MORE;
                                              basic_control->onInput(timer_msg, nullptr, *this, CONTROL_INPUT_FLAG_CONTROL);
                                              callback(timer_msg);
                                         });
}

void AddressLayer::initImmediatelyTimerInfo(int type, uint32_t timeout_time, const std::function<void()> &timeout_func,
                                            const std::function<void(MsgHdr*)> &callback) {
    BasicControl::callOutputOnStackMemory(sizeof(MsgHdr) + sizeof(NoteTimerInfo),
                                          [&](MsgHdr *timer_msg) -> void {
                                              MsgHdrUtil<NoteTimerInfo>::initMsgHdr(timer_msg, sizeof(NoteTimerInfo), LAYER_CONTROL_REQUEST_TIMER,
                                                                                    NoteTimerInfo::initNoteTimerInfo(type, 1, timeout_time, 0, nullptr, timeout_func));
                                              timer_msg->response_type = LAYER_CONTROL_TIMER_IMMEDIATELY;
                                              basic_control->onInput(timer_msg, nullptr, *this, CONTROL_INPUT_FLAG_CONTROL);
                                              callback(timer_msg);
                                         });
}

bool AddressLayer::isRemoteNoteTimeout(MeetingAddressNote *note_info, uint32_t timeout_threshold, uint32_t current_sequence) {
    return ((current_sequence - dynamic_cast<RemoteNoteInfo*>(note_info)->getLastSequence()) >= timeout_threshold);
}

TransmitNoteStatus AddressLayer::onNoteTransmitStatus(MeetingAddressNote *note) {
    StatusNote status = dynamic_cast<RemoteNoteInfo*>(note)->getStatusNote();
    TransmitNoteStatus note_stats = TransmitNoteStatus((status == NOTE_STATUS_HOST) ? NOTE_STATUS_NORMAL : static_cast<uint32_t>(status), 0, 0);

    MeetingAddressManager::onNoteStatusInfo(note,
                                            [&](uint32_t position, uint32_t address) -> void {
                                                note_stats.note_position = position; note_stats.note_address = address;
                                            });

    return note_stats;
}

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 初始化远程端的节点信息（主Socket线程）
 *  端口号（port）
 *  清除链接和参数信息
 *  设置验证信息（NoteKey）
 *  通知控制层远程端初始化
 * @param msg           信息
 * @param note_info     远程端节点
 */
void MasterAddressLayer::onRemoteNoteInit(MsgHdr *msg, RemoteNoteInfo *note_info) {
    //设置验证信息
    note_info->onNoteKey(msg->msg_key);
    //设置远程端信息（端口号、链接、参数）及通知控制层
    AddressLayer::onRemoteNoteInit(msg, note_info);
}

/**
 * 处理远程端的加入（关联Socket线程）
 *   设置链接信息（加入时间点）
 *   初始化加入超时器
 *      触发：发送加入响应
 *      超时：退出处理
 *   通知控制层远程端加入
 * @param msg           信息
 * @param note_info     远程端节点
 */
void MasterAddressLayer::onNoteInputJoin0(MsgHdr *msg, AddressNoteInfo *note_info){
    //设置链接信息
    note_info->onNoteJoin(link_time, link_time * time_size);

    //初始化超时器
    initMoreTimerInfo(NOTE_TIMER_TYPE_JOIN, join_time, join_time * time_size,
                      [&]() -> void { //触发任务
                          onNoteJoinExecutor(this, basic_control, note_info);
                   }, [&]() -> void {  //超时任务
                          onNoteTimeout(this, basic_control, note_info);
                   }, [&](MsgHdr *timer_msg) -> void {
                         if(timer_msg && (timer_msg->serial_number >= sizeof(NoteTimerInfo))){
                             dynamic_cast<RemoteNoteInfo*>(note_info)->setTimerInfo(reinterpret_cast<NoteTimerInfo*>(timer_msg->buffer));
                         }
                   });
    //通知控制层
    AddressLayer::onNoteInputJoin0(msg, note_info);
}

/**
 * 处理远程端的退出（关联Socket线程）
 *  通知控制层远程端退出
 *  设置链接信息（退出时间点）
 *  更改退出状态,调用退出函数
 * @param msg           信息
 * @param note_info     远程端节点
 */
void MasterAddressLayer::onNoteInputExit0(MsgHdr *msg, AddressNoteInfo *note_info){
    //通知控制层
    AddressLayer::onNoteInputExit0(msg, note_info);
    //设置链接信息
    note_info->onNoteExit();
    //调用退出函数
    onInputLayer(msg, note_info, getInputFunc(&AddressStatusUtil::onInputExit));


}

/**
 * 处理远程端的链接（关联Socket线程）
 *  设置链接信息
 *  初始化链接超时器
 *  通知控制层远程端链接
 * @param msg           信息
 * @param note_info     远程端节点
 */
void MasterAddressLayer::onNoteInputLink0(MsgHdr *msg, AddressNoteInfo *note_info){
    //设置链接信息
    note_info->onNoteLink(passive_time, msg->serial_number);

    //初始化超时器
    initMoreTimerInfo(NOTE_TIMER_TYPE_LINK, link_time, link_time * time_size,
                      [&]() -> void { //触发任务
                          onNoteLinkExecutor(this, basic_control, note_info);
                   }, [&]() -> void { //超时任务
                          onNoteTimeout(this, basic_control, note_info);
                   }, [&](MsgHdr *timer_msg) -> void {
                          if(timer_msg && (timer_msg->serial_number >= sizeof(NoteTimerInfo))){
                              dynamic_cast<RemoteNoteInfo*>(note_info)->setTimerInfo(reinterpret_cast<NoteTimerInfo*>(timer_msg->buffer));
                          }
                   });
    //通知控制层
    AddressLayer::onNoteInputLink0(msg, note_info);
}

/**
 * 处理远端端的同步（关联Socket线程）
 *  计算RTT（控制层）
 *  通知控制层远程端同步（不处理）
 * @param msg           信息
 * @param note_info     远程端节点
 */
void MasterAddressLayer::onNoteInputSynchro0(MsgHdr *msg, AddressNoteInfo *note_info) {
    //设置RTT信息
    dynamic_cast<RemoteNoteInfo*>(note_info)->onNoteSynchro(msg->serial_number, reinterpret_cast<SequenceData*>(msg->buffer),
                                                            this, (LinkCallbackFunc)&MasterAddressLayer::onLinkCallback);
    //通知控制层（不处理）
    AddressLayer::onNoteInputSynchro0(msg, note_info);
}

void AddressLayer::onLinkCallback(RemoteNoteInfo *note_info) {
    if(!note_info->getIndicatorNote()){
        BasicControl::callOutputOnStackMemory(sizeof(MsgHdr) + sizeof(SequenceData),
                                              [&](MsgHdr *msg) -> void {
                                                  MsgHdrUtil<void>::initMsgHdr(msg, LAYER_MASTER_LINK);
                                                  basic_control->onInput(msg, note_info, *this, CONTROL_INPUT_FLAG_INPUT);
                                                  dynamic_cast<LinkStatusUtil*>(status_util[note_info->getStatusNote()])->onInputSequence(
                                                                  msg, this, dynamic_cast<AddressNoteInfo*>(note_info));
                                              });
    }
}

void MasterAddressLayer::onNoteImmediateSynchro(MsgHdr *immediate_msg) {
    reinterpret_cast<SynchroData*>(immediate_msg->buffer)->synchro_recv_time = BasicControl::onRequestExecutorTime();
}

std::function<void(AddressNoteInfo*)> MasterAddressLayer::onNoteTermination() {
    return [&](AddressNoteInfo *note_info) -> void {
        onNoteTimeout(this, basic_control, note_info);
    };
}

/**
 * 处理远程端的序号（关联Socket线程）
 *  设置链接信息（RTT、帧、延迟、序号）（由主机同步完成时提供）
 *  初始化序号超时器
 *  通知控制层远程端序号（同步完成）
 * @param msg           信息
 * @param note_info     远程端节点
 */
void MasterAddressLayer::onNoteInputSequence0(MsgHdr *msg, AddressNoteInfo *note_info){
    //获取链接信息（由地址层判断远程端同步时调用该函数及提供链接信息(帧、延迟)）
    auto sequence_data = reinterpret_cast<SequenceData*>(msg->buffer);
    //设置链接信息
    note_info->onNoteSequence(SequenceData{dynamic_cast<RemoteNoteInfo*>(note_info)->onNoteSequenceData().link_rtt,
                                                              sequence_data->link_frame, sequence_data->link_delay,
                                                              sequence_data->serial_number});
    //初始化超时将
    initMoreTimerInfo(NOTE_TIMER_TYPE_SEQUENCE, sequence_time, sequence_time * time_size,
                      [&]() -> void {
                          onNoteSequenceExecutor(this, basic_control, note_info, sequence_data->link_frame, sequence_data->link_delay);
                   }, [&]() -> void {
                          onNoteTimeout(this, basic_control, note_info);
                   }, [&](MsgHdr *timer_msg) -> void {
                          if(timer_msg && (timer_msg->serial_number >= sizeof(NoteTimerInfo))){
                              dynamic_cast<RemoteNoteInfo*>(note_info)->setTimerInfo(reinterpret_cast<NoteTimerInfo*>(timer_msg->buffer));
                          }
                   });
    //通知控制层
    AddressLayer::onNoteInputSequence0(msg, note_info);
}

/**
 * 处理远程端的被动（Loop线程 -> 关联Socket线程）
 *  设置链接信息
 *  初始化超时器
 *  通知控制层远程端被动处理
 * @param msg           信息
 * @param note_info     远端端节点
 */
void MasterAddressLayer::onNoteInputPassive0(MsgHdr *msg, AddressNoteInfo *note_info){
    //设置链接信息
    note_info->onNotePassive();

    //初始化超时器
    initImmediatelyTimerInfo(NOTE_TIMER_TYPE_PASSIVE, passive_time,
                             [&]() -> void {
                                 onNoteTimeout(this, basic_control, note_info);
                          }, [&](MsgHdr *timer_msg) -> void {
                                 if(timer_msg && (timer_msg->serial_number >= sizeof(NoteTimerInfo))){
                                     dynamic_cast<RemoteNoteInfo*>(note_info)->setTimerInfo(reinterpret_cast<NoteTimerInfo*>(timer_msg->buffer));
                                 }
                          });
    //通知通知层
    AddressLayer::onNoteInputPassive0(msg, note_info);
}

/**
 * 处理远程端的正常（关联Socket线程）
 *  设置链接信息
 *  通知控制层远程端正常
 * @param msg           信息
 * @param note_info     远程端节点
 */
void MasterAddressLayer::onNoteInputNormal0(MsgHdr *msg, AddressNoteInfo *note_info){
    //设置链接信息
    note_info->onNoteNormal(msg->serial_number);

    //通知控制层
    AddressLayer::onNoteInputNormal0(msg, note_info);
}


/**
 * 验证远程端节点的输入信息（NoteKey及Sequence）
 * @param msg           信息
 * @param note_info     远程端节点
 * @return              是否验证通过
 */
bool MasterAddressLayer::onNoteVerify(MsgHdr *msg, MeetingAddressNote *info) {
    auto note_info = dynamic_cast<RemoteNoteInfo*>(info);

    /*
     * 判断远程端是否关联当前SocketThread线程
     */
    if(!note_info->isCorrelateThread()){
        return false;
    }

    /*
     * 判断远程端（加入设置NoteKey、退出复位NoteKey）的NoteKey与输入的NoteKey是否一致（解码（不同链接、用于加入）、数量（同一链接、用于加入））及输入信息序号是否过时
     */
    if((note_info->isNoteKey(note_info->getNoteKey(0), msg->msg_key)) && (msg->serial_number >= note_info->getLastSequence())){
        //设置NoteKey的加入请求数量（用于加入请求、其他请求该数量不会增加）
        note_info->onNoteKey(msg->msg_key.amount_area);
        return true;
    }else{
        return false;
    }
}

void MasterAddressLayer::onRunNote(MeetingAddressNote *note) {
    RemoteNoteInfo::onLinkRunNote(&master_run_note, dynamic_cast<RemoteNoteInfo*>(note));
}

void MasterAddressLayer::unRunNote(MeetingAddressNote *note) {
    RemoteNoteInfo::unLinkRunNote(&master_run_note, dynamic_cast<RemoteNoteInfo*>(note));
}

uint32_t MasterAddressLayer::onRunNoteSize() const {
    return master_run_note.onRunNoteSize();
}

void MasterAddressLayer::onRunNoteInfo(const std::function<void(MeetingAddressNote*,uint32_t)> &func) {
    master_run_note.onRunNoteInfo(func);
}

/**
 * 处理地址层的加入、链接、序号及被动的超时时间
 * @param type      类型
 * @param value     时间
 */
void MasterAddressLayer::onAddressTime(uint32_t type, uint32_t value) {
    switch (type){
        //加入触发时间
        case ADDRESS_LAYER_JOIN_TIME:
            join_time = value;
            break;
        //链接触发时间
        case ADDRESS_LAYER_LINK_TIME:
            link_time = value;
            break;
        //序号触发时间
        case ADDRESS_LAYER_SEQUENCE_TIME:
            sequence_time = value;
            break;
        //被动超时时间
        case ADDRESS_LAYER_PASSIVE_TIME:
            passive_time = value;
            break;
        default:
            break;
    }
}

/**
 * 响应远程端加入的触发任务（向远端端发送加入响应）（Executor线程）
 * @param layer         地址层指针
 * @param note_info     远端端节点
 */
void MasterAddressLayer::onNoteJoinExecutor(AddressLayer *layer, BasicControl *control, AddressNoteInfo *note_info){
    auto *master_layer = dynamic_cast<MasterAddressLayer*>(layer);
    //获取链接信息（链接触发时间、超时时间、被动链接时间）
    LinkData link_data{master_layer->link_time, master_layer->link_time * master_layer->time_size, master_layer->passive_time};

    /*
     * 通知控制层由关联的Socket线程执行输入数据,数据结构如下:
     *  |-MsgHdr-(buffer)->|----LinkData----|
     */
    BasicControl::callOutputOnStackMemory(sizeof(MsgHdr) + sizeof(LinkData),
                                          [&](MsgHdr *timer_msg) -> void {
                                              //初始化MsgHdr
                                              MsgHdrUtil<LinkData>::initMsgHdr(timer_msg, sizeof(LinkData), LAYER_MASTER_JOIN, link_data);
                                              //设置加入响应
                                              timer_msg->response_type = LAYER_RESPONSE_JOIN;
                                              //设置输出长度
                                              timer_msg->len_source = sizeof(MsgHdr) + sizeof(LinkData);
                                              //设置NoteKey（验证信息）
                                              timer_msg->msg_key = note_info->getNoteKey();
                                              //通知控制层输入（由关联的Socket线程输入）
                                              control->onInput(timer_msg, note_info, *layer, CONTROL_INPUT_FLAG_OUTPUT | CONTROL_INPUT_FLAG_THREAD_CORRELATE);
                                          });
}

/**
 * 响应远程端链接（同步）的触发任务（向远程端发送同步响应）（Executor线程）
 * @param layer         地址层指针
 * @param note_info     远程端节点
 */
void MasterAddressLayer::onNoteLinkExecutor(AddressLayer *layer, BasicControl *control, AddressNoteInfo *note_info){
    /*
     * 通知控制层由关联的Socket线程执行输入数据,数据结构如下:
     *  |-MsgHdr-(buffer)->|----SynchroData（同步时间）----|
     */
    BasicControl::callOutputOnStackMemory(sizeof(MsgHdr) + sizeof(SynchroData),
                                          [&](MsgHdr *timer_msg) -> void {
                                              //初始化MsgHdr
                                              MsgHdrUtil<void>::initMsgHdr(timer_msg, LAYER_MASTER_LINK);
                                              //设置同步
                                              timer_msg->shared_type = LAYER_SHARED_SYNCHRO;
                                              //设置输出长度
                                              timer_msg->len_source = sizeof(MsgHdr) + sizeof(SynchroData);
                                              //设置NoteKey
                                              timer_msg->msg_key = note_info->getNoteKey();
                                              //设置同步信息
                                              dynamic_cast<RemoteNoteInfo*>(note_info)->onNoteSynchro();
                                              //通知控制层输入数据（由关联的Socket线程输入）（填充同步时间）
                                              control->onInput(timer_msg, note_info, *layer, CONTROL_INPUT_FLAG_OUTPUT | CONTROL_INPUT_FLAG_THREAD_CORRELATE);
                                          });
}

/**
 * 响应远程端序号的触发任务（向远程端发送序号）（Executor线程）
 * @param layer         地址层指针
 * @param note_info     远程端节点
 * @param link_frame    帧
 * @param link_delay    延迟
 */
void MasterAddressLayer::onNoteSequenceExecutor(AddressLayer *layer, BasicControl *control, AddressNoteInfo *note_info, uint32_t link_frame, uint32_t link_delay){
    //获取序号数据（RTT、帧、延迟）
    SequenceData sequence_data{note_info->onNoteSequenceData().link_rtt, link_frame, link_delay, 0};

    /*
     * 通知控制层在驱动任务时由关联的Socket线程执行输入数据,数据结构如下:
     *  |-MsgHdr-(buffer)->|----SequenceData----|
     */
    BasicControl::callOutputOnStackMemory(sizeof(MsgHdr) + sizeof(SequenceData),
                                          [&](MsgHdr *timer_msg) -> void {
                                              //初始化MsgHdr、设置序号信息
                                              MsgHdrUtil<SequenceData>::initMsgHdr(timer_msg, sizeof(MsgHdr) + sizeof(SequenceData), LAYER_MASTER_LINK, sequence_data);
                                              //设置序号
                                              timer_msg->shared_type = LAYER_SHARED_SEQUENCE;
                                              //设置输出长度
                                              timer_msg->len_source = sizeof(MsgHdr) + sizeof(SequenceData);
                                              //设置NoteKey
                                              timer_msg->msg_key = note_info->getNoteKey();
                                              //通知控制层输入数据（有关联的Socket线程输入）（驱动函数输入）（填充序号）
                                              control->onInput(timer_msg,  note_info, *layer, CONTROL_INPUT_FLAG_OUTPUT
                                                                                            | CONTROL_INPUT_FLAG_DRIVE_REPLY
                                                                                            | CONTROL_INPUT_FLAG_THREAD_CORRELATE);
                                          });
}

/**
 * 远程端的加入、链接、序号及被动的超时触发任务（Executor线程）
 * @param layer         地址层指针
 * @param note_info     远程端信息
 */
void MasterAddressLayer::onNoteTimeout(AddressLayer *layer, BasicControl *control, AddressNoteInfo *note_info){
    //通知控制层处理远程端的退出（由关联的Socket线程处理）
    BasicControl::callOutputOnStackMemory(sizeof(MsgHdr),
                                          [&](MsgHdr *timer_msg) -> void {
                                              //初始化MsgHdr
                                              MsgHdrUtil<void>::initMsgHdr(timer_msg, LAYER_MASTER_EXIT);
                                              timer_msg->msg_key = note_info->getNoteKey();
                                              control->onInput(timer_msg, note_info, *layer, CONTROL_INPUT_FLAG_INPUT
                                                                                           | CONTROL_INPUT_FLAG_THREAD_CORRELATE);
                                          });
}


//--------------------------------------------------------------------------------------------------------------------//

/**
 * 初始化主机的节点信息（Socket线程）
 *  清除链接和参数信息
 *  初始化验证信息（NoteKey）
 *  通知控制层远程端初始化
 * @param msg           信息
 * @param note_info     主机节点
 */
void ServantAddressLayer::onRemoteNoteInit(MsgHdr *msg, RemoteNoteInfo *note_info) {
    //初始化NoteKey（主机接收解码并存储NoteKey）
    note_info->onNoteKey();
    //设置主机节点
    core_note = note_info;
    //设置链接信息（清除）
    AddressLayer::onRemoteNoteInit(msg, core_note);
}

/**
 * 处理加入主机（Socket线程）
 *  初始化加入超时器
 *  通知控制层申请加入主机
 * @param msg
 */
void ServantAddressLayer::onNoteInputJoin0(MsgHdr *msg, AddressNoteInfo*) {
    //初始化加入超时器
    initMoreTimerInfo(NOTE_TIMER_TYPE_JOIN, join_time, join_time * time_size,
                      [&]() -> void { //触发任务
                          onNoteJoinExecutor(this, basic_control, core_note);
                   }, [&]() -> void { //超时任务
                          onNoteTimeout(this, basic_control, core_note);
                   }, [&](MsgHdr *timer_msg) -> void {
                          if(timer_msg && (timer_msg->serial_number >= sizeof(NoteTimerInfo))){
                              core_note->setTimerInfo(reinterpret_cast<NoteTimerInfo*>(timer_msg->buffer));
                          }
                   });
    //通知控制层
    AddressLayer::onNoteInputJoin0(msg, core_note);
}

/**
 * 处理主机的退出（Socket线程）
 *  通知控制层主机退出（主动退出）
 *      主机异常退出（被动退出） -> 主机被动处理 -> 重新链接处理 -> 链接超时 -> 主机退出处理
 *  设置链接信息
 *  调用退出函数
 * @param msg           信息
 * @param note_info     主机节点
 */
void ServantAddressLayer::onNoteInputExit0(MsgHdr *msg, AddressNoteInfo*){
    //通知控制层
    AddressLayer::onNoteInputExit0(msg, core_note);
    //设置链接信息
    core_note->onNoteExit();
    //调用退出函数
    onInputLayer(msg, core_note, getInputFunc(&AddressStatusUtil::onInputExit));
}

/**
 * 处理主机的响应加入、链接主机（Socket线程）
 *  设置链接信息（链接、超时、被动链接时间）
 *  初始化链接超时器
 *  通知控制层远程端链接
 * @param msg           信息
 * @param note_info     主机节点
 */
void ServantAddressLayer::onNoteInputLink0(MsgHdr *msg, AddressNoteInfo*) {
    //获取主机发送的链接信息（主机响应加入）
    auto link_data = reinterpret_cast<LinkData*>(msg->buffer);
    //设置链接信息（链接触发、超时时间）
    core_note->onNoteJoin(link_data->link_time, link_data->link_timeout);
    //设置被动链接时间
    passive_time = link_data->link_passive;

    //初始化链接超时器
    initMoreTimerInfo(NOTE_TIMER_TYPE_LINK, link_data->link_time, link_data->link_timeout,
                      [&]() -> void {
                          onNoteLinkExecutor(this, basic_control, core_note);
                   }, [&]() -> void {
                          onNoteTimeout(this, basic_control, core_note);
                   }, [&](MsgHdr *timer_msg) -> void {
                          if(timer_msg && (timer_msg->serial_number >= sizeof(NoteTimerInfo))){
                              core_note->setTimerInfo(reinterpret_cast<NoteTimerInfo*>(timer_msg->buffer));
                          }
                   });
    //通知控制层
    AddressLayer::onNoteInputLink0(msg, core_note);
}

/**
 * 处理主机的响应链接、同步（Socket线程）
 *  设置链接信息
 *  初始化被动链接超时器
 *  通知控制层主机同步（不处理）
 * @param msg           信息
 * @param note_info     主机节点
 */
void ServantAddressLayer::onNoteInputSynchro0(MsgHdr *msg, AddressNoteInfo*) {
    //设置链接信息
    core_note->onNoteLink(passive_time, msg->serial_number);

    //初始化被动链接超时器
    initImmediatelyTimerInfo(NOTE_TIMER_TYPE_SEQUENCE, passive_time,
                             [&]() -> void {
                                 onNoteTimeout(this, basic_control, core_note);
                          }, [&](MsgHdr *timer_msg) -> void {
                                 if(timer_msg && (timer_msg->serial_number >= sizeof(NoteTimerInfo))){
                                     core_note->setTimerInfo(reinterpret_cast<NoteTimerInfo*>(timer_msg->buffer));
                                 }
                          });
    //通知控制层（不处理）
    AddressLayer::onNoteInputSynchro0(msg, core_note);
}

/**
 * 处理响应主机发送的同步
 * @param msg           信息
 */
void ServantAddressLayer::onNoteImmediateSynchro(ServantAddressLayer *layer, MsgHdr *immdeiate_msg) {
    //设置同步响应
    immdeiate_msg->response_type = LAYER_RESPONSE_SYNCHRO;
    //拷贝主机节点
    MsgHdrUtil<AddressNoteInfo*>::initMsgHdr(immdeiate_msg, sizeof(AddressNoteInfo*),
                                             static_cast<uint32_t>(immdeiate_msg->master_type), layer->core_note);
}

/**
 * 处理主机的序号（Socket线程）
 *  获取主机序号信息（RTT、序号、帧、延迟）
 *  调用正常函数
 *  通知控制层主机同步（不处理）
 * @param msg           信息
 * @param note_info     主机端节点
 */
void ServantAddressLayer::onNoteInputSequence0(MsgHdr *msg, AddressNoteInfo*){
    //设置接收主机与客户端的序号差值
    onNoteSequenceGap(reinterpret_cast<SequenceData*>(msg->buffer)->serial_number);
    //获取并设置链接信息（RTT、序号、帧、延迟）
    core_note->onNoteSequence(*reinterpret_cast<SequenceData*>(msg->buffer));
    //调用正常函数
    onInputLayer(msg, core_note, getInputFunc(&AddressStatusUtil::onInputNormal));
    //通知控制层（不处理）
    AddressLayer::onNoteInputSynchro0(msg, core_note);
}

void ServantAddressLayer::onNoteSequenceGap(uint32_t core_sequence) {
    BasicControl::callOutputOnStackMemory(sizeof(MsgHdr) + sizeof(uint32_t),
                                          [&](MsgHdr *msg) -> void {
                                              MsgHdrUtil<uint32_t>::initMsgHdr(msg, sizeof(uint32_t), LAYER_CONTROL_CORE_SEQUENCE_GAP, core_sequence);
                                              basic_control->onInput(msg, nullptr, *this, CONTROL_INPUT_FLAG_CONTROL);
                                          });
}

/**
 * 处理主机的正常（Socket线程）
 *  设置链接信息
 *  通知控制层主机正常通信
 * @param msg           信息
 * @param note_info     主机节点
 */
void ServantAddressLayer::onNoteInputNormal0(MsgHdr *msg, AddressNoteInfo*) {
    //设置链接信息
    core_note->onNoteNormal(msg->serial_number);
    //通知控制层
    AddressLayer::onNoteInputNormal0(msg, core_note);
}

/**
 * 处理主机的被动退出（Loop线程 -> Socket线程）
 *  重新加入处理
 *  通知控制层主机被动退出
 * @param msg           信息
 * @param note_info     主机节点
 */
void ServantAddressLayer::onNoteInputPassive0(MsgHdr *msg, AddressNoteInfo*){
    //获取主机发送的链接信息
    LinkData link_data = core_note->onNoteLinkData();
    //设置链接信息（链接）
    core_note->onNoteJoin(link_data.link_time, link_data.link_timeout);

    //调用来链接函数（远程端的加入函数实际是链接函数）
    onInputLayer(msg, core_note, getInputFunc(&AddressStatusUtil::onInputJoin));
    //通知控制层
    AddressLayer::onNoteInputPassive0(msg, core_note);
}

/**
 * 验证主机的输入信息（NoteKey及Sequence）
 * @param msg           信息
 * @param note_info     主机节点
 * @return              是否验证通过
 */
bool ServantAddressLayer::onNoteVerify(MsgHdr *msg, MeetingAddressNote *info) {
    auto note_info = dynamic_cast<RemoteNoteInfo*>(info);

    /*
     * 判断远程端是否关联当前SocketThread线程
     */
    if(!note_info->isCorrelateThread()){
        return false;
    }

    /*
     * 判断远程端（加入设置NoteKey、退出复位NoteKey）的NoteKey与输入的NoteKey是否一致（解码（不同链接、用于加入）、数量（同一链接、用于加入））及输入信息序号是否过时
     */
    if((note_info->isNoteKey(msg->msg_key, note_info->getNoteKey(0))) && (msg->serial_number > note_info->getLastSequence())){
        //设置接收主机的序号（用于发送响应链接、同步）（加入、正常除外）
        onNoteInputNormal0(msg, note_info);
        return true;
    }else{
        return false;
    }

}

void ServantAddressLayer::onRunNote(MeetingAddressNote *) {}

void ServantAddressLayer::unRunNote(MeetingAddressNote *) {}

uint32_t ServantAddressLayer::onRunNoteSize() const {
    return 0;
}

void ServantAddressLayer::onRunNoteInfo(const std::function<void(MeetingAddressNote *, uint32_t)> &) {}

std::function<void(AddressNoteInfo*)> ServantAddressLayer::onNoteTermination() {
    return [=](AddressNoteInfo *note_info) -> void {
        onNoteTimeout(this, basic_control, note_info);
    };
}

/**
 * 处理地址层的超时时间
 * @param type      类型
 * @param value     时间
 */
void ServantAddressLayer::onAddressTime(uint32_t type, uint32_t value) {
    //加入超时时间
    if(type == ADDRESS_LAYER_JOIN_TIME){ join_time = value; }
}

/**
 * 响应主机加入的触发任务（向主机发送加入请求）（Executor线程）
 * @param layer         地址层
 * @param note_info     主机节点
 */
void ServantAddressLayer::onNoteJoinExecutor(AddressLayer *layer, BasicControl *control, AddressNoteInfo *note_info){
#define SERVANT_PASSWORD_KEY_JOIN_VALUE     1

    /*
     * 通知控制层输出数据,数据结构如下：
     *      |--MsgHdr---|
     * =>
     *          version         =>  控制层填充
     *          master_type     =>  MASTER_JOIN
     *          shared_type     =>  0
     *          response_type   =>  0
     *          address_number  =>  0
     *          serial_number   =>  0
     *          msg_key         =>  地址层初始化并填充
     */
    BasicControl::callOutputOnStackMemory(sizeof(MsgHdr),
                                          [&](MsgHdr *timer_msg) -> void {
                                              //初始化MsgHdr
                                              MsgHdrUtil<void>::initMsgHdr(timer_msg, LAYER_MASTER_JOIN);
                                              //设置输出长度
                                              timer_msg->len_source = sizeof(MsgHdr);
                                              //设置NoteKey
                                              timer_msg->msg_key = note_info->getNoteKey(SERVANT_PASSWORD_KEY_JOIN_VALUE);
                                              //通知控制层输出数据（由关联的Socket线程输出）
                                              control->onInput(timer_msg, note_info, *layer, CONTROL_INPUT_FLAG_OUTPUT
                                                                                           | CONTROL_INPUT_FLAG_THREAD_CORRELATE);
                                          });
}

/**
 * 响应主机链接的触发任务（向主机发送链接请求）（Executor线程）
 * @param layer         地址层
 * @param note_info     主机节点
 */
void ServantAddressLayer::onNoteLinkExecutor(AddressLayer *layer, BasicControl *control, AddressNoteInfo *note_info){
    /*
     * 通知控制层输出数据,结构如下：
     *      |--MsgHdr---|
     * =>
     *          version         =>  控制层填充
     *          master_type     =>  MASTER_LINK
     *          shared_type     =>  0
     *          response_type   =>  0
     *          address_number  =>  0
     *          serial_number   =>  接收主机最后有效输入的序号
     *          msg_key         =>  地址层填充
     */
    BasicControl::callOutputOnStackMemory(sizeof(MsgHdr),
                                          [&](MsgHdr *timer_msg) -> void {
                                              //初始化MsgHdr
                                              MsgHdrUtil<void>::initMsgHdr(timer_msg, LAYER_MASTER_LINK);
                                              //填充序号
                                              timer_msg->serial_number = note_info->getLastSequence();
                                              //设置输出长度
                                              timer_msg->len_source = sizeof(MsgHdr);
                                              //设置NoteKey
                                              timer_msg->msg_key = note_info->getNoteKey();
                                              //通知控制层输出数据（由关联的Socket线程输出）
                                              control->onInput(timer_msg, note_info, *layer, CONTROL_INPUT_FLAG_OUTPUT
                                                                                           | CONTROL_INPUT_FLAG_UNFILL_SEQUENCE
                                                                                           | CONTROL_INPUT_FLAG_THREAD_CORRELATE);
                                          });
}

/**
 * 主机的加入、链接及被动的超时触发任务（Executor线程）
 * @param layer         地址层
 * @param note_info     主机节点
 */
void ServantAddressLayer::onNoteTimeout(AddressLayer *layer, BasicControl *control, AddressNoteInfo *note_info){
    //通知控制层处理初级的退出（由关联的Socket线程处理）
    BasicControl::callOutputOnStackMemory(sizeof(MsgHdr),
                                          [&](MsgHdr *timer_msg) -> void {
                                              //初始化MsgHdr
                                              MsgHdrUtil<void>::initMsgHdr(timer_msg, LAYER_MASTER_EXIT);
                                              timer_msg->serial_number = note_info->getLastSequence();
                                              //设置输出长度
                                              timer_msg->len_source = sizeof(MsgHdr);
                                              //设置NoteKey
                                              timer_msg->msg_key = note_info->getNoteKey();
                                              //通知控制层处理主机的超时（由关联的Socket线程处理）
                                              control->onInput(timer_msg, note_info, *layer, CONTROL_INPUT_FLAG_OUTPUT
                                                                                           | CONTROL_INPUT_FLAG_UNFILL_SEQUENCE
                                                                                           | CONTROL_INPUT_FLAG_THREAD_CORRELATE);
                                          });
}