//
// Created by abc on 20-12-8.
//

#include "CentralizationControl.h"

void ControlFuncUtil::callControlFuncOnInput(MsgHdr *msg, MeetingAddressNote *note_info, int flag) {
    if(!control) { return; }

    switch (flag){
        case LAYER_MASTER_JOIN:
        case LAYER_MASTER_CREATE:
            break;
        case LAYER_MASTER_LINK:
            control->onNoteLink(msg, note_info);
            break;
        case LAYER_MASTER_EXIT:
            control->onNoteExit(msg, note_info);
            break;
        case LAYER_MASTER_MEDIA:
            control->onNoteMedia(msg, note_info);
            break;
        case LAYER_MASTER_EXIT_PASSIVE:
            control->onNotePassive(msg, note_info);
            break;
        case LAYER_MASTER_INIT:
            control->onNoteInit(msg, note_info);
            break;
        case LAYER_MASTER_UNINIT:
            control->onNoteUninit(msg, note_info);
            break;
        case LAYER_MASTER_PREVENT:
            control->onNotePrevent(msg, note_info);
            break;
        case LAYER_MASTER_UPDATE:
            control->onNoteUpdate(msg, note_info);
            break;
        case LAYER_MASTER_NOTE_DISPLAY:
            control->onNoteDisplay(msg, note_info);
            break;
        default:
            break;
    }
}

void ControlFuncUtil::callControlFuncOnControl(MsgHdr *msg, MeetingAddressNote *note_info, int flag) {
    if(!control) { return; }

    switch (flag){
        case LAYER_CONTROL_ADDRESS_ERROR:
            control->onLayerError(msg, note_info);
            break;
        case LAYER_CONTROL_REQUEST_TIMER:
            control->onRequestTimer(msg, note_info);
            break;
        case LAYER_CONTROL_REQUEST_ALLOC_DYNAMIC:
            control->onAllocDynamic(msg);
            break;
        case LAYER_CONTROL_REQUEST_ALLOC_FIXED:
            control->onAllocFixed(msg, note_info);
            break;
        case LAYER_CONTROL_REQUEST_DESTROY_DYNAMIC:
            control->onDeallocDynamic(msg);
            break;
        case LAYER_CONTROL_REQUEST_DESTROY_FIXED:
            control->onDeallocFixed(msg, note_info);
            break;
        case LAYER_CONTROL_REQUEST_ADDRESS_BLACK:
            control->onNoteBlack(msg, note_info);
            break;
        case LAYER_CONTROL_EXECUTOR_CREATE_THREAD:
            control->onExecutorCreateThread(msg, note_info);
            break;
        case LAYER_CONTROL_EXECUTOR_DESTROY_THREAD:
            control->onExecutorDestroyThread(msg, note_info);
            break;
        case LAYER_CONTROL_TRANSMIT_CORRELATE_THREAD:
            control->onTransmitCorrelateThread(msg, note_info);
            break;
        case LAYER_CONTROL_TRANSMIT_MAIN_THREAD_UTIL:
            control->onTransmitMainThread(msg, note_info);
            break;
        case LAYER_CONTROL_CORE_SEQUENCE_GAP:
            control->onSequenceGap(msg, note_info);
            break;
        case LAYER_CONTROL_DISPLAY_DRIVE_FRAME:
            control->onDisplayFrame(msg, note_info);
            break;
        case LAYER_CONTROL_CORE_TERMINATION:
            control->onControlTermination(msg, note_info);
            break;
        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------------------------//

CentralizationControl::CentralizationControl(ControlLaunchMode mode) throw(std::bad_alloc, std::runtime_error)
                                            : BasicControl(mode), control_timer_sequence(0), main_thread_util(nullptr),
                                              func_util(), drive_queue(){
    onLayerInit();
    onFuncInit();
    onCentralizationInit();
}

CentralizationControl::CentralizationControl(MsgHdr *msg, ControlLaunchMode mode) throw(std::bad_alloc, std::runtime_error)
                                            : BasicControl(mode), control_timer_sequence(0), main_thread_util(nullptr),
                                              func_util(), drive_queue() {
    onLayerInit();
    onFuncInit();
    onCentralizationInit(msg);
}

CentralizationControl::~CentralizationControl() {
    //暂未实现
}

void CentralizationControl::onRemoteInput(TransmitType *transmit_type, const std::function<void(RemoteNoteInfo*, TransmitThreadUtil*)> &call_func) {
    auto note_info = dynamic_cast<RemoteNoteInfo*>(transmit_type->getTransmitNote());
    TransmitThreadUtil *thread_util = note_info ? note_info->correlateThreadUtil() : nullptr;

    if(thread_util){
        call_func(note_info, thread_util);
    }
}

void CentralizationControl::onRemoteOutput(TransmitLayer *transmit_layer, RemoteNoteInfo *note_info, TransmitThreadUtil *thread_util,
                                           uint32_t output_len, short response, short shared, char master, const std::function<void(MsgHdr*)> &call_func) {
    callOutputOnStackMemory(output_len,
                            [&](MsgHdr *output_msg) -> void {
                                call_func(output_msg);
                                onRemoteOutput(transmit_layer, note_info, thread_util, output_msg, output_len, response, shared, master);
                            });
}

void CentralizationControl::onRemoteOutput(TransmitLayer *transmit_layer, RemoteNoteInfo *note_info, TransmitThreadUtil *thread_util,
                                           MsgHdr *output_msg, uint32_t output_len, short response, short shared, char master) {
    TransmitOutputInfo output_info{ output_len, 0, 0, 0, output_msg, note_info, thread_util };
    output_info.onNoteCompileOutput(thread_util->getTransmitAnalysisUtil(), output_msg);
    output_msg->master_type = master; output_msg->shared_type = shared; output_msg->response_type = response;

    transmit_layer->onOutputLayer(&output_info);
}

void CentralizationControl::requestInit() {
    onInitControl();
}

void CentralizationControl::requestLaunch(MsgHdr *launch_msg) {
    if(!launch_msg || (launch_msg->serial_number <= 0)){
        throw std::runtime_error("启动控制器失败！");
    }
    onLaunchControl(launch_msg);
}

void CentralizationControl::requestStop(MsgHdr *stop_msg) {
    onStopControl(stop_msg);
}

void CentralizationControl::timeDrive() {
    static MsgHdr time_drive;
    BasicLayer *drive_layer = nullptr;

    time_drive.serial_number = ++control_timer_sequence;
    time_drive.master_type = LAYER_MASTER_TIME_DRIVE;

    onDriveOutput0(&time_drive);
    onCentralizationTimeDrive(&time_drive);

    for(int i = LAYER_EXECUTOR_TYPE; i < LAYER_TYPE_SIZE; i++){
        if((drive_layer = layer_array[i].onLayer())->isDrive()) {
            drive_layer->onDrive(&time_drive);
        }
    }
}

void CentralizationControl::onLayerInit() {
    for(int i = 0; i < LAYER_TYPE_SIZE; i++){
        layer_array[i] = LayerControlUtil();
    }
}

void CentralizationControl::onFuncInit() {
    func_util.onInitFuncUtil(this);
}

void CentralizationControl::onCentralizationInit(MsgHdr *init_msg) throw(std::bad_alloc, std::runtime_error) {
    if(control_mode == CONTROL_LAUNCH_START_INIT){
        return;
    }

    if(control_mode == CONTROL_LAUNCH_IMMEDIATE){
        try {
            launchControl0(init_msg);
        }catch (std::bad_alloc &e){
            std::cout << "CentralizationControl::onCentralizationInit->CONTROL_LAUNCH_IMMEDIATE:throw" << std::endl; throw;
        }catch (std::runtime_error &e){
            std::cout << "CentralizationControl::onCentralizationInit->CONTROL_LAUNCH_IMMEDIATE:throw" << std::endl; throw;
        }
    }

    if(control_mode == CONTROL_LAUNCH_USE_START){
        try {
            onInit(nullptr);
        }catch (std::bad_alloc &e){
            std::cout << "CentralizationControl::onCentralizationInit->CONTROL_LAUNCH_USE_START:throw" << std::endl; throw;
        }
    }
}

void CentralizationControl::onInitControl() throw(std::bad_alloc) {
    createLayer();
}

void CentralizationControl::onLaunchControl(MsgHdr *launch_msg) throw(std::runtime_error) {
    std::promise<void> launch_promise;
    MsgHdr *layer_msg[LAYER_TYPE_SIZE];
    auto executor_layer = dynamic_cast<ExecutorLayer*>(layer_array[LAYER_EXECUTOR_TYPE].onLayer());

    onAnalysisMsg(launch_msg,
                  [&](MsgHdr *analysis_msg, int pos) -> void {
                      if(!(layer_msg[pos] = analysis_msg)){
                          throw std::runtime_error("启动控制器失败！");
                      }
                 });

    launchLayer(executor_layer, layer_msg[LAYER_EXECUTOR_TYPE]);

    TaskHolder<void> holder = executor_layer->onTimerImmediately<void, std::function<void()>>(
            [&]() -> void {
                for(int i = LAYER_MEMORY_TYPE; i < LAYER_TYPE_SIZE; i++){
                    launchLayer(layer_array[i].onLayer(), layer_msg[i]);
                }
                onLaunchComplete(launch_promise);
            });

    onLaunchCentralization(layer_msg[LAYER_CONFIGURE_TYPE]);
    launch_promise.get_future().get();
}

void CentralizationControl::onStopControl(MsgHdr *msg) {
    MsgHdr stop_msg{};

    auto stop_complete = [&]() -> void {
        control_timer_sequence = 0;
        main_thread_util = nullptr;
    };
    onStopCentralization(msg);

    for(int i = 0; i < LAYER_TYPE_SIZE; i++){
        stopLayer(layer_array[i].onLayer(), &stop_msg);
    }
    stop_complete();
}

void CentralizationControl::onAnalysisMsg(MsgHdr *analysis_msg, const std::function<void(MsgHdr*, int)> &callback_func) {
    TransmitInfo analysis_info = initAnalysisInfo(analysis_msg);

    for(int pos = 0, remain_hdr_len = 0, remain_resource_len = 0; ;pos++){
        remain_hdr_len = analysis_info.hdr_len - analysis_info.hdr_offset;
        remain_resource_len = analysis_msg->serial_number - analysis_info.resource_offset;

        if((remain_hdr_len < sizeof(short)) || (remain_resource_len <= 0)){
            break;
        }

        if(callback_func){
            callback_func(reinterpret_cast<MsgHdr*>(analysis_msg->buffer + analysis_info.hdr_offset), pos);
        }

        analysis_info.resource_offset += *reinterpret_cast<short*>(analysis_msg->buffer + analysis_info.hdr_offset);
        analysis_info.hdr_offset += sizeof(short);
    }
}

TransmitInfo CentralizationControl::initAnalysisInfo(MsgHdr *analysis_msg) {
    TransmitInfo analysis_info = TransmitInfo();

    analysis_info.total_len = analysis_msg->serial_number;
    if(analysis_msg->serial_number > (sizeof(MsgHdr) + sizeof(short))){
        analysis_info.hdr_offset = 0;
        analysis_info.hdr_len = analysis_msg->len_source;
        analysis_info.resource_offset = analysis_msg->len_source;
        analysis_info.resources_len = analysis_info.total_len - sizeof(MsgHdr) - analysis_info.hdr_len;
    }

    return analysis_info;
}

void CentralizationControl::createLayer() throw(std::bad_alloc) {
    BasicLayer *layer = nullptr;

    auto create_fail_func = [&]() -> void {
        destroyLayer(); throw std::bad_alloc();
    };

    for(int i = 0; i < LAYER_TYPE_SIZE; i++){
        if(!(layer = BasicLayer::createLayer(this, static_cast<LayerType>(i)))){
            create_fail_func();
        }
        layer_array[i].onInitFuncUtil(layer);
    }
}

void CentralizationControl::launchLayer(BasicLayer *layer, MsgHdr *launch_msg) throw(std::bad_alloc, std::runtime_error) {
    launch_msg->master_type = LAYER_CONTROL_STATUS_START;
    layer->onControl(launch_msg);

    if(launch_msg->master_type == LAYER_CONTROL_STATUS_THROW){
        throw std::runtime_error("CentralizationControl::launchLayer!");
    }
}

void CentralizationControl::stopLayer(BasicLayer *layer, MsgHdr *stop_msg) {
    stop_msg->master_type = LAYER_CONTROL_STATUS_STOP;
    layer->onControl(stop_msg);
}

void CentralizationControl::destroyLayer() {
    destroyLayer(layer_array[LAYER_EXECUTOR_TYPE].onLayer());

    for(int i = 0; i < LAYER_TYPE_SIZE; i++){
        if((i == LAYER_CONFIGURE_TYPE) || (i == LAYER_EXECUTOR_TYPE)){
            continue;
        }
        destroyLayer(layer_array[i].onLayer());
    }
}

void CentralizationControl::destroyLayer(BasicLayer *layer) {
    if(layer){ BasicLayer::destroyLayer(this, layer); }
}

void CentralizationControl::onInput(MsgHdr *input_msg, MeetingAddressNote *note_info, const AddressLayer &layer, int flags) {
    onInput0(input_msg, note_info, layer, flags);
}

void CentralizationControl::onInput(MsgHdr *input_msg, MeetingAddressNote *note_info, const ExecutorLayer &layer, int flags) {
    onInput0(input_msg, note_info, layer, flags);
}

void CentralizationControl::onInput(MsgHdr *input_msg, MeetingAddressNote *note_info, const MemoryLayer &layer, int flags) {
    //内存层没有请求控制层,不需要处理
    onInput0(input_msg, note_info, layer, flags);
}

void CentralizationControl::onInput(MsgHdr *input_msg, MeetingAddressNote *note_info, const TimeLayer &layer, int flags) {
    onInput0(input_msg, note_info, layer, flags);
}

void CentralizationControl::onInput(MsgHdr *input_msg, MeetingAddressNote *note_info, const TransmitLayer &layer, int flags) {
    onInput0(input_msg, note_info, layer, flags);
}

void CentralizationControl::onInput(MsgHdr *input_msg, MeetingAddressNote *note_info, const DisplayLayer &layer, int flags) {
    onInput0(input_msg, note_info, layer, flags);
}

void CentralizationControl::onInput(MsgHdr *input_msg, MeetingAddressNote *note_info, const UserLayer &layer, int flags) {
    onInput0(input_msg, note_info, layer, flags);
}

void CentralizationControl::onInput0(MsgHdr *input_msg, MeetingAddressNote *note_info, const BasicLayer &layer, int flags) {
    if(!input_msg || !flags){
        return;
    }
    input_msg->shared_type = layer.onLayerType();

    if(flags | CONTROL_INPUT_FLAG_INPUT){
        func_util.callControlFuncOnInput(input_msg, note_info, input_msg->master_type);
        return;
    }

    if(flags | CONTROL_INPUT_FLAG_CONTROL){
        func_util.callControlFuncOnControl(input_msg, note_info, input_msg->master_type);
    }

    if(flags | CONTROL_INPUT_FLAG_OUTPUT){
        if(flags | CONTROL_INPUT_FLAG_DRIVE_REPLY){
            onDriveOutput(input_msg, dynamic_cast<AddressNoteInfo*>(note_info), flags);
        }
        if(flags | CONTROL_INPUT_FLAG_IMMEDIATELY_REPLY){
            onImmediatelyOutput(input_msg, dynamic_cast<AddressNoteInfo*>(note_info), flags);
        }
    }
}

void CentralizationControl::onCommonInput(MsgHdr *common_msg, int flags) {
    if(!common_msg || !flags){
        return;
    }
    if(flags | CONTROL_INPUT_FLAG_CONTROL){
        func_util.callControlFuncOnControl(common_msg, nullptr, common_msg->master_type);
    }
}

void CentralizationControl::onTransmitInput(TransmitMaster *transmit_master) {
    transmit_master->onMasterInput(&input_info);
}

void CentralizationControl::onTransmitInput(TransmitShared *transmit_shared) {
    transmit_shared->onSharedInput(&input_info);
}

void CentralizationControl::onTransmitInput(TransmitResponse *transmit_response) {
    transmit_response->onResponseInput(&input_info);
}

void CentralizationControl::onDriveOutput(MsgHdr *output_msg, AddressNoteInfo *note_info, int flag) {
    drive_queue.push(ControlDriveInfo(output_msg->len_source, flag, output_msg, dynamic_cast<RemoteNoteInfo*>(note_info)));
}

void CentralizationControl::onDriveOutput0(MsgHdr *drive_msg) {
    while(!drive_queue.empty()){
        onThreadOutput(drive_queue.pop(), drive_msg->serial_number);
    }
}

void CentralizationControl::onImmediatelyOutput(MsgHdr *output_msg, AddressNoteInfo *note_info, int flag) {
    onThreadOutput(ControlDriveInfo(output_msg->len_source, flag, output_msg,
                                                     dynamic_cast<RemoteNoteInfo*>(note_info)), control_timer_sequence);
}

void CentralizationControl::onThreadOutput(const ControlDriveInfo &drive_info, uint32_t sequence) {
    TransmitThreadUtil *output_thread_util = nullptr;

    if(!(drive_info.output_flag | CONTROL_INPUT_FLAG_UNFILL_SEQUENCE)){
        drive_info.output_msg->serial_number = sequence;
    }

    if(drive_info.output_flag | CONTROL_INPUT_FLAG_THREAD_CORRELATE){
        output_thread_util = drive_info.note_info->correlateThreadUtil();
    }else{
        output_thread_util = main_thread_util;
    }

    if(drive_info.output_flag | CONTROL_INPUT_FLAG_HOST){
        onCentralizationTimeDrive(drive_info.output_msg);
    }else {
        onThreadOutput0(getControlLayer<TransmitLayer>(layer_array, LAYER_TRANSMIT_TYPE), output_thread_util, drive_info);
    }
}

void CentralizationControl::onThreadOutput0(TransmitLayer *transmit_layer, TransmitThreadUtil *thread_util, const ControlDriveInfo &drive_info) {
    char master_type = 0;
    short shared_type = 0;
    if(drive_info.output_msg->shared_type || drive_info.output_msg->response_type){
        master_type = drive_info.output_msg->master_type;
        drive_info.output_msg->master_type = 0;
    }
    if(drive_info.output_msg->response_type){
        shared_type = drive_info.output_msg->shared_type;
        drive_info.output_msg->shared_type = 0;
    }

    onRemoteOutput(transmit_layer, drive_info.note_info, thread_util, drive_info.output_msg,
                                             static_cast<uint32_t>(drive_info.output_len), 0, shared_type, master_type);
}

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 远程端加入（第一次加入或多次加入）,需要初始化数据（线程池层、时间层、显示层）（地址层除外,该函数由地址层调用）
 * @param note_msg
 */
void CentralizationControl::onNoteInit(MsgHdr *note_msg, MeetingAddressNote *note_info) {
    auto init_throw_func = [&]() -> void {
        note_msg->master_type = LAYER_MASTER_PREVENT;
        note_msg->serial_number = sizeof(AddressNoteInfo*);
        func_util.callControlFuncOnControl(note_msg, note_info, note_msg->master_type);
    };

    //该函数运行在主SocketThread线程
    auto init_executor_func = [&](TransmitThreadUtil *init_thread_util) -> void {
        callControlLayerFunc(getControlLayerUtil(layer_array, LAYER_TIME_TYPE), (LayerFunc)&LayerControlUtil::callLayerFuncOnControl, note_msg, TIME_LAYER_BUFFER_INCREASE);
        /*
         * 向显示层输入更新显示框（note）
         *  1：构造显示框（失败返回）
         *  2：显示ip地址、序号（主机排序）
         *  3：显示正在初始化
         */
        MsgHdrUtil<DisplayNoteInfo>::initMsgHdr(note_msg, sizeof(DisplayNoteInfo), DISPLAY_LAYER_DISPLAY_UPDATE, DisplayNoteInfo(DISPLAY_NOTE_STATUS_INIT, note_info));
        callControlLayerFunc(getControlLayerUtil(layer_array, LAYER_DISPLAY_TYPE), (LayerFunc)&LayerControlUtil::callLayerFuncOnInput, note_msg, note_msg->master_type);
        //复用远程端
        getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onNoteReuse(note_msg, dynamic_cast<AddressNoteInfo*>(note_info));
    };

    auto call_note_func = [&](MeetingAddressNote *note_info) -> void {
        getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onRunNote(note_info);
    };

    if(!note_info){
        note_msg->serial_number = 0;
        return;
    }

    /*
     * 向线程池层发送加入请求（onInput函数）,流程如下（忽略std::logic_error异常）
     *  1：关联socket线程
     *      （1）.关联失败调用init_throw_func函数（主socket线程）
     *      （2）.关联成功,由关联socket线程接着执行
     *  2：调用init_executor_func
     *      （1）.执行成功（无需处理后续）
     *      （2）.执行失败（std::logic_error异常）,解除关联,调用init_throw_func函数（关联socket线程）
     */
    MsgHdrUtil<ExecutorInitInfo>::initMsgHdr(note_msg, sizeof(ExecutorInitInfo), EXECUTOR_LAYER_ADDRESS_JOIN,
                                      ExecutorInitInfo(note_info, call_note_func, init_executor_func, init_throw_func));
    callControlLayerFunc(getControlLayerUtil(layer_array, LAYER_EXECUTOR_TYPE), (LayerFunc)&LayerControlUtil::callLayerFuncOnInput, note_msg, EXECUTOR_LAYER_ADDRESS_JOIN);
}

/**
 * 远程端因退出而注销
 * @param note_msg
 * @param note_info
 */
void CentralizationControl::onNoteUninit(MsgHdr *note_msg, MeetingAddressNote *note_info) {
    //该函数运行在关联的SocketThread线程上
    if(!note_info){
        note_msg->serial_number = 0;
        return;
    }

    auto call_note_func = [&](MeetingAddressNote *note_info) -> void {
        getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->unRunNote(note_info);
    };

    main_thread_util->onRunCorrelateThreadImmediate([&]() -> void {
        //重置远程端
        getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onNoteUnuse(note_msg, dynamic_cast<AddressNoteInfo*>(note_info));

        /*
         * 向显示层输入更新显示框（note）
         * 1：隐藏显示框
         * 2：删除显示框
         */
        callControlLayerFuncNotMsg(getControlLayerUtil(layer_array, LAYER_DISPLAY_TYPE), (LayerFunc)&LayerControlUtil::callLayerFuncOnInput,
                                   [&](MsgHdr *display_msg) -> void {
                                       MsgHdrUtil<DisplayNoteInfo>::initMsgHdr(display_msg, sizeof(DisplayNoteInfo), DISPLAY_LAYER_DISPLAY_UPDATE,
                                                                               DisplayNoteInfo(DISPLAY_NOTE_STATUS_UNINIT, note_info));
                                   }, sizeof(DisplayNoteInfo));
        //更新时间层
        callControlLayerFunc(getControlLayerUtil(layer_array, LAYER_TIME_TYPE), (LayerFunc)&LayerControlUtil::callLayerFuncOnControl, note_msg, TIME_LAYER_BUFFER_REDUCE);

        //退出执行层
        callControlLayerFuncNotMsg(getControlLayerUtil(layer_array, LAYER_EXECUTOR_TYPE),  (LayerFunc)&LayerControlUtil::callLayerFuncOnInput,
                                   [&](MsgHdr *executor_msg) -> void {
                                       MsgHdrUtil<ExecutorThreadInfo>::initMsgHdr(executor_msg, sizeof(ExecutorThreadInfo), EXECUTOR_LAYER_ADDRESS_EXIT,
                                                                                  ExecutorThreadInfo(note_info, call_note_func));
                                   }, sizeof(ExecutorThreadInfo));

    });
}

/**
 * 远程端状态发生变化,需要更改显示层的显示框
 * @param note_msg
 */
void CentralizationControl::onNoteUpdate(MsgHdr *note_msg, MeetingAddressNote *note_info) {
    if(!note_info){
        note_msg->serial_number = 0;
        return;
    }

    /*
     * 向显示层输入更新显示框
     *  1：根据note状态更新状态（正在加入、正在链接、正在同步、正常（取消显示）、正在断线、正在退出）
     */
    callControlLayerFuncNotMsg(getControlLayerUtil(layer_array, LAYER_DISPLAY_TYPE), (LayerFunc)&LayerControlUtil::callLayerFuncOnInput,
                               [&](MsgHdr *display_msg) -> void {
                                   MsgHdrUtil<DisplayNoteInfo>::initMsgHdr(display_msg, sizeof(DisplayNoteInfo), DISPLAY_LAYER_DISPLAY_UPDATE,
                                                                           DisplayNoteInfo(DisplayLayer::onConvertStatus(note_msg->response_type), note_info));
                               }, sizeof(DisplayNoteInfo));
}

//--------------------------------------------------------------------------------------------------------------------//

void CentralizationControl::onLayerError(MsgHdr*, MeetingAddressNote*) {
    //暂不实现
}

void CentralizationControl::onRequestTimer(MsgHdr *timer_msg, MeetingAddressNote*) {
    if(timer_msg->serial_number < sizeof(NoteTimerInfo)){
        return;
    }
    NoteTimerInfo timer_info = std::move(*reinterpret_cast<NoteTimerInfo*>(timer_msg->buffer));

    if(timer_msg->response_type == LAYER_CONTROL_TIMER_IMMEDIATELY){
        timer_msg->master_type = EXECUTOR_LAYER_APPLICATION_TIMER_IMMEDIATELY;
    }else if(timer_msg->response_type == LAYER_CONTROL_TIMER_MORE){
        timer_msg->master_type = EXECUTOR_LAYER_APPLICATION_TIMER_MORE;
    }

    getControlLayer<ExecutorLayer>(layer_array, LAYER_EXECUTOR_TYPE)->onInput(
            MsgHdrUtil<ExecutorTimerData>::initMsgHdr(timer_msg, sizeof(ExecutorTimerData), static_cast<uint32_t>(timer_msg->master_type),
                                                      timer_info.getTimerData()));

    if(timer_msg->serial_number >= sizeof(TaskHolder<void>)) {
        timer_info.setTimerHolder(std::move(*reinterpret_cast<TaskHolder<void>*>(timer_msg->buffer)));
        MsgHdrUtil<NoteTimerInfo>::initMsgHdr(timer_msg, sizeof(NoteTimerInfo), LAYER_CONTROL_REQUEST_TIMER, std::move(timer_info));
    }
}

void CentralizationControl::onAllocFixed(MsgHdr *alloc_msg, MeetingAddressNote*) {
    alloc_msg->master_type = MEMORY_LAYER_FIXED_MEMORY;
    getControlLayer<MemoryLayer>(layer_array, LAYER_MEMORY_TYPE)->onInput(alloc_msg);
}

void CentralizationControl::onDeallocFixed(MsgHdr *dealloc_msg, MeetingAddressNote*) {
    dealloc_msg->master_type = MEMORY_LAYER_RELEASE_MEMORY;
    getControlLayer<MemoryLayer>(layer_array, LAYER_MEMORY_TYPE)->onInput(dealloc_msg);
}

/**
 * 是否过滤远程端（查看远程端是否黑名单）
 *  没有匹配到（该地址没有加入远程端树）,将该地址加入到本地远程端树中,返回非黑名单
 *  配置到,返回是否黑名单
 * @param note_msg
 */
void CentralizationControl::onNoteBlack(MsgHdr *note_msg, MeetingAddressNote*) {
    if(note_msg->serial_number < std::max(sizeof(TransmitRequestInfo), sizeof(MeetingAddressNote*))){
        note_msg->serial_number = 0;
        return;
    }

    AddressNoteInfo *note_info = nullptr;
    TransmitRequestInfo *request_info = nullptr;

    if(!(request_info = reinterpret_cast<TransmitRequestInfo*>(note_msg->buffer))){
        note_msg->serial_number = 0;
        return;
    }

    if(!(note_info = getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onMatchAddress(request_info->request_addr))
       && request_info->request_func(getControlLayer<TransmitLayer>(layer_array, LAYER_TRANSMIT_TYPE), main_thread_util)){
        note_msg->shared_type = NOTE_STATUS_REMOTE;
        callControlLayerFunc(getControlLayerUtil(layer_array, LAYER_ADDRESS_TYPE), (LayerFunc)&LayerControlUtil::callLayerFuncOnInput, note_msg, LAYER_MASTER_CREATE);
    }

    if(note_info){
        note_msg->master_type = ADDRESS_LAYER_IS_BLACK;
        getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onControl(
                MsgHdrUtil<AddressNoteInfo*>::initMsgHdr(note_msg, sizeof(AddressNoteInfo*), ADDRESS_LAYER_IS_BLACK, note_info));
    }


    if(note_msg->serial_number > 0){
        note_msg->shared_type = NOTE_STATUS_REMOTE;
        callControlLayerFunc(getControlLayerUtil(layer_array, LAYER_ADDRESS_TYPE), (LayerFunc)&LayerControlUtil::callLayerFuncOnInput, note_msg, LAYER_MASTER_INIT);
    }
}

void CentralizationControl::onExecutorCreateThread(MsgHdr *thread_msg, MeetingAddressNote*) {
    auto transmit_layer = getControlLayer<TransmitLayer>(layer_array, LAYER_TRANSMIT_TYPE);

    transmit_layer->onControl(thread_msg);

    if(thread_msg->serial_number <= 0){
        thread_msg->serial_number = sizeof(TransmitThreadUtil);
        onAllocFixed(thread_msg, nullptr);
    }

    if(thread_msg->serial_number > 0){
        thread_msg->master_type = LAYER_CONTROL_TRANSMIT_INIT_THREAD;
        transmit_layer->onControl(thread_msg);
    }
}

void CentralizationControl::onExecutorDestroyThread(MsgHdr *thread_msg, MeetingAddressNote*) {
    uint32_t transmit_id = 0;
    thread_msg->master_type = LAYER_CONTROL_TRANSMIT_UNINIT_THREAD;
    getControlLayer<TransmitLayer>(layer_array, LAYER_TRANSMIT_TYPE)->onControl(thread_msg);

    if((transmit_id = thread_msg->serial_number) > 0){
        thread_msg->serial_number = sizeof(TransmitThreadUtil);
        onDeallocFixed(thread_msg, nullptr);
    }

    MsgHdrUtil<uint32_t>::initMsgHdr(thread_msg, static_cast<uint32_t>(thread_msg->master_type), sizeof(uint32_t), transmit_id);
}

void CentralizationControl::onTransmitCorrelateThread(MsgHdr *correlate_msg, MeetingAddressNote*) {
    getControlLayer<TransmitLayer>(layer_array, LAYER_TRANSMIT_TYPE)->onControl(correlate_msg);
}

void CentralizationControl::onTransmitMainThread(MsgHdr *correlate_msg, MeetingAddressNote*) {
    if(!main_thread_util) {
        main_thread_util = reinterpret_cast<TransmitThreadUtil*>(correlate_msg->buffer);
    }
}

void CentralizationControl::onDisplayFrame(MsgHdr *frame_msg, MeetingAddressNote*) {
    TransmitMemoryFixedUtil fixed_memory_util = TransmitMemoryFixedUtil(0, nullptr, main_thread_util);

    callOutputOnStackMemory(sizeof(MsgHdr) + sizeof(MediaData),
                            [&](MsgHdr *buffer) -> void {
                                onGenerateMediaMsg(buffer, reinterpret_cast<NoteMediaInfo2*>(frame_msg->buffer),
                                                                          &fixed_memory_util, frame_msg->serial_number);
                            });
}

void CentralizationControl::onGenerateMediaMsg(MsgHdr *media_msg, NoteMediaInfo2 *media_info, TransmitMemoryFixedUtil *fixed_memory_util, uint32_t sequence) {
    MediaData media_data{};

    fixed_memory_util->setCreateFixedBuffer(media_info->media_len, media_info->media_buffer, media_info->release_func);

    media_data.media_reader = fixed_memory_util->onExtractMemory();
    media_data.media_note = getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onHostNote();
    media_data.media_size = 1;
    media_data.media_len_offset = 0;
    media_data.media_buffer_offset = sizeof(short);
    media_data.analysis_media_func = [&](MediaData *data, NoteMediaInfo *info, int) -> void {
        info->media_len = media_info->media_len;
        info->media_offset = 0;
        info->media_data = data;
    };

    media_msg->len_source = static_cast<uint32_t>(media_info->media_len);
    onSequenceFrame(MsgHdrUtil<MediaData>::initMsgHdr(media_msg, sequence, LAYER_MASTER_MEDIA, media_data));
}

void CentralizationControl::onControlTermination(MsgHdr *termination_msg, MeetingAddressNote*) {
    getControlLayer<UserLayer>(layer_array, LAYER_USER_TYPE)->onInput(MsgHdrUtil<void>::initMsgHdr(termination_msg, USER_CONTROL_TERMINATION));
}