//
// Created by abc on 20-12-8.
//

#include "MasterCentralization.h"

//--------------------------------------------------------------------------------------------------------------------//
//中心化主机序号信息（默认）
const SequenceData master_default_sequence{0, 30, 0, 0};
//--------------------------------------------------------------------------------------------------------------------//
SequenceData MasterCentralization::default_sequence_data{};
//--------------------------------------------------------------------------------------------------------------------//

void MasterCentralization::onTransmitMasterInput(TransmitJoin *transmit_join) {
    main_thread_util->onRunCorrelateThreadImmediate([&]() -> void {
        callControlLayerFuncNotMsg(getControlLayerUtil(layer_array, LAYER_ADDRESS_TYPE), (LayerFunc)&LayerControlUtil::callLayerFuncOnInput,
                                   [&](MsgHdr *msg) -> void {
                                       copyMsgHdr(transmit_join->getTransmitMsg(), msg);
                                       msg->master_type = LAYER_MASTER_INIT;
                                       msg->shared_type = NOTE_STATUS_REMOTE;
                                   }, std::max(sizeof(DisplayNoteInfo), sizeof(ExecutorInitInfo)));
    });

    onRemoteInput(dynamic_cast<TransmitType*>(transmit_join),
                  [&](RemoteNoteInfo *note_info, TransmitThreadUtil *thread_util) -> void {
                      thread_util->onRunCorrelateThreadImmediate(
                              [&]() -> void {
                                  getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onNoteInputJoin(
                                                                            transmit_join->getTransmitMsg(), note_info);
                              });
                  });
}

void MasterCentralization::onTransmitMasterInput(TransmitLink *transmit_link) {
    onRemoteInput(dynamic_cast<TransmitType*>(transmit_link),
                  [&](RemoteNoteInfo *note_info, TransmitThreadUtil *thread_util) -> void {
                      thread_util->onRunCorrelateThreadImmediate(
                              [&]() -> void {
                                  getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onNoteInputLink(
                                                                             transmit_link->getTransmitMsg(), note_info);
                              });
                  });
}

void MasterCentralization::onTransmitMasterInput(TransmitMedia *transmit_media) {
    onRemoteInput(dynamic_cast<TransmitType*>(transmit_media),
                  [&](RemoteNoteInfo *note_info, TransmitThreadUtil *thread_util) -> void {
                      thread_util->onRunCorrelateThreadImmediate(
                              [&]() -> void {
                                  getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onNoteInputNormal(
                                                                           transmit_media->getTransmitMsg(), note_info);
                                  callControlLayerFuncNotMsg(getControlLayerUtil(layer_array, LAYER_TIME_TYPE), (LayerFunc)&LayerControlUtil::callLayerFuncOnInput,
                                                     [&](MsgHdr *time_msg) -> void {
                                                         MsgHdrUtil<MediaData>::initMsgHdr(time_msg, sizeof(MediaData), LAYER_MASTER_MEDIA, transmit_media->getMediaData());
                                                     }, sizeof(MediaData));
                              });
                  });
}

void MasterCentralization::onTransmitMasterInput(TransmitExit *transmit_exit) {
    onRemoteInput(dynamic_cast<TransmitType*>(transmit_exit),
                  [&](RemoteNoteInfo *note_info, TransmitThreadUtil *thread_util) -> void {
                      getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onNoteInputExit(transmit_exit->getTransmitMsg(), note_info);
                  });
}

//--------------------------------------------------------------------------------------------------------------------//

void MasterCentralization::onTransmitSharedInput(TransmitSychro *transmit_synchro) {
    onRemoteInput(dynamic_cast<TransmitType*>(transmit_synchro),
                  [&](RemoteNoteInfo *note_info, TransmitThreadUtil *thread_util) -> void {
                      getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onNoteInputSynchro(transmit_synchro->getTransmitMsg(), note_info);
                  });
}

void MasterCentralization::onTransmitSharedInput(TransmitSequence *transmit_sequence) {
    onRemoteInput(dynamic_cast<TransmitType*>(transmit_sequence),
                  [&](RemoteNoteInfo *note_info, TransmitThreadUtil *thread_util) -> void {
                      getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onNoteInputSequence(transmit_sequence->getTransmitMsg(), note_info);
                  });
}

void MasterCentralization::onTransmitSharedInput(TransmitTransfer *transmit_transfer) {
    onRemoteInput(dynamic_cast<TransmitType*>(transmit_transfer),
                  [&](RemoteNoteInfo *note_info, TransmitThreadUtil *thread_util) -> void {
                      getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onNoteInputTransfer(transmit_transfer->getTransmitMsg(), note_info);
                  });
}

void MasterCentralization::onTransmitSharedInput(TransmitQuery *transmit_query) {
    uint32_t query_code = transmit_query->getTransmitQuery(), response_len = 0;
    InfoData response_data = InfoData(0, nullptr);

    switch (query_code){
        //暂时不实现
        default:
            break;
    }
    if((response_len = static_cast<uint32_t>(response_data.info_len)) <= 0){
        return;
    }

    onRemoteInput(dynamic_cast<TransmitType*>(transmit_query),
                  [&](RemoteNoteInfo *note_info, TransmitThreadUtil *thread_util) -> void {
                      onRemoteOutput(getControlLayer<TransmitLayer>(layer_array, LAYER_TRANSMIT_TYPE), note_info, thread_util,
                                     sizeof(MsgHdr) + sizeof(short) + response_len, 0, LAYER_SHARED_QUERY, 0,
                                     [&](MsgHdr *output_msg) -> void {
                                         MsgHdrUtil<InfoData>::initMsgHdr(output_msg, 0, 0, response_data);
                                         output_msg->response_type = LAYER_RESPONSE_QUERY;
                                     });
                  });
}

void MasterCentralization::onTransmitSharedInput(TransmitText*) {
    //暂不实现
}

void MasterCentralization::onTransmitSharedInput(TransmitError*) {
    //暂不实现
}

//--------------------------------------------------------------------------------------------------------------------//

void MasterCentralization::onTransmitResponseInput(TransmitRJoin*) {
    //不需要处理
}

void MasterCentralization::onTransmitResponseInput(TransmitRSychro*) {
    //不需要处理
}

void MasterCentralization::onTransmitResponseInput(TransmitRTransfer*) {
    //不需要处理
}

void MasterCentralization::onTransmitResponseInput(TransmitRStatus*) {
    //不需要处理
}

void MasterCentralization::onTransmitResponseInput(TransmitRQuery*) {
    //不需要处理
}

//--------------------------------------------------------------------------------------------------------------------//

/**
 * （输入拦截）
 * @param synchro_msg
 * @return
 */
bool MasterCentralization::onTransmitSynchroIntercept(MsgHdr *synchro_msg) {
    MasterAddressLayer::onNoteImmediateSynchro(synchro_msg);
    return true;
}

/**
 * （输出拦截）
 * @param sequence_msg
 * @return
 */
bool MasterCentralization::onTransmitOutputIntercept(MsgHdr *sequence_msg, MeetingAddressNote *output_note) {
    if(sequence_msg->shared_type |= LAYER_SHARED_SEQUENCE) {
        reinterpret_cast<SequenceData*>(sequence_msg->buffer)->serial_number = control_timer_sequence;
    }else if(sequence_msg->shared_type |= LAYER_SHARED_SYNCHRO){
        reinterpret_cast<SynchroData*>(sequence_msg->buffer)->synchro_send_time = onRequestExecutorTime();
    }
    sequence_msg->msg_key = dynamic_cast<RemoteNoteInfo*>(output_note)->getNoteKey();
    return true;
}

void MasterCentralization::onNotePrevent(MsgHdr*, MeetingAddressNote *info) {
    auto note_info = dynamic_cast<RemoteNoteInfo*>(info);
    auto transmit_layer = getControlLayer<TransmitLayer>(layer_array, LAYER_TRANSMIT_TYPE);
    auto executor_layer = getControlLayer<ExecutorLayer>(layer_array, LAYER_EXECUTOR_TYPE);

    //设置黑名单
    note_info->setBlackNote();

    //初始化定时器（超时取消黑名单）
    TaskHolder<void> holder = executor_layer->onTimerDelay<void, std::function<void()>>(
            [&]() -> void {
                note_info->clrBlackNote();
            }, std::chrono::seconds(CONTROL_ADDRESS_PREVENT_SECOND_TIME));

    onRemoteOutput(transmit_layer, note_info, main_thread_util, sizeof(MsgHdr) + sizeof(short) + sizeof(uint32_t), 0, 0, 0,
                   [&](MsgHdr *output_msg) -> void {
                       MsgHdrUtil<uint32_t>::initMsgHdr(output_msg, 0, 0, LAYER_SHARED_ERROR_CODE_PREVENT);
                       output_msg->shared_type = LAYER_SHARED_ERROR;
                   });

}

void MasterCentralization::onNoteDisplay(MsgHdr *display_msg, MeetingAddressNote*) {
    auto submit_media_info = reinterpret_cast<TimeSubmitInfo*>(display_msg->buffer);

    callControlLayerFuncNotMsg(getControlLayerUtil(layer_array, LAYER_DISPLAY_TYPE), (LayerFunc)&LayerControlUtil::callLayerFuncOnInput,
                               [&](MsgHdr *msg) -> void {
                                   MsgHdrUtil<DisplayMediaInfo>::initMsgHdr(msg, sizeof(DisplayMediaInfo), DISPLAY_LAYER_DISPLAY_MEDIA,
                                              DisplayMediaInfo(nullptr, std::bind(&MasterCentralization::onDisplayAnalysis0, this,
                                                                                  submit_media_info, std::placeholders::_1)));
                               }, sizeof(DisplayMediaInfo));
}

void MasterCentralization::onNoteLink(MsgHdr *link_msg, MeetingAddressNote*) {
    MsgHdrUtil<SequenceData>::initMsgHdr(link_msg, sizeof(SequenceData), static_cast<uint32_t>(link_msg->master_type),
                             SequenceData{0, default_sequence_data.link_frame, default_sequence_data.link_delay, 0});
}

/**
 * 该函数由时间层调用（LoopThread线程）
 * @param media_msg
 */
void MasterCentralization::onNoteMedia(MsgHdr *media_msg, MeetingAddressNote*) {
    auto submit_media_info = reinterpret_cast<TimeSubmitInfo*>(media_msg->buffer);
    uint32_t info_size = submit_media_info->submit_info_size;
    uint32_t note_size = getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onRunNoteSize();

    ExecutorNoteMediaInfo *note_info = initExecutorNoteInfo(note_size, info_size,
                                                            submit_media_info->submit_sequence, submit_media_info->submit_timeout,
                                                            this, submit_media_info,
                                                            [&](const std::function<void(MeetingAddressNote*, uint32_t)> &func) -> void {
                                                                getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onRunNoteInfo(func);
                                                            });

    callControlLayerFuncNotMsg(getControlLayerUtil(layer_array, LAYER_EXECUTOR_TYPE), (LayerFunc)&LayerControlUtil::callLayerFuncOnInput,
                               [&](MsgHdr *msg) -> void {
                                   MsgHdrUtil<ExecutorNoteMediaInfo*>::initMsgHdr(msg, sizeof(ExecutorNoteMediaInfo*),
                                                                                  EXECUTOR_LAYER_CORRELATE_NOTE, note_info);
                               }, sizeof(ExecutorNoteMediaInfo*));
}

/**
 * 该函数有控制调用（关联的SocketThread线程）
 * @param passive_msg
 * @param note_info
 */
void MasterCentralization::onNotePassive(MsgHdr*, MeetingAddressNote *note_info) {
    getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onNoteInputPassive(nullptr, dynamic_cast<AddressNoteInfo*>(note_info));
}

/**
 * 该函数由地址层调用（ExecutorThread线程）,远程端因被动超时而导致退出（被动退出）
 * @param exit_msg  退出msg
 * @param info      远程端
 */
void MasterCentralization::onNoteExit(MsgHdr *exit_msg, MeetingAddressNote *info) {
    auto note_info = dynamic_cast<RemoteNoteInfo*>(info);
    auto thread_util = note_info->correlateThreadUtil();

    if(thread_util){
        exit_msg->serial_number = static_cast<uint32_t>(control_timer_sequence);
        thread_util->onRunCorrelateThreadImmediate([&]() -> void {
            getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onNoteInputExit(exit_msg, note_info);
        });
    }
}

void MasterCentralization::onSequenceGap(MsgHdr*, MeetingAddressNote*) {
    //不需要处理
}

void MasterCentralization::onSequenceFrame(MsgHdr *frame_msg) {
    getControlLayer<TimeLayer>(layer_array, LAYER_TIME_TYPE)->onInput(frame_msg);
}

void MasterCentralization::onCentralizationTimeDrive(MsgHdr*) {}

/**
 * 该函数运行在关联的SocketThread线程上
 * @param note
 */
void MasterCentralization::onNoteTransmit0(TransmitThreadUtil *thread_util,  MeetingAddressNote *note, MsgHdr *transmit_msg,
                                           uint32_t note_size, uint32_t transmit_sequence, uint32_t timeout_threshold) {
    int create_len = 0;
    auto address_layer = getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE);
    auto transmit_layer = getControlLayer<TransmitLayer>(layer_array, LAYER_TRANSMIT_TYPE);

    for(int i = 0, note_len = 0; i < note_size; i++){
        note_len = transmit_msg->serial_number + TransmitLayer::onRemoteNoteCompileLen(thread_util, note + i);
        create_len = std::max(note_len, create_len);
    }

    {
        MemoryReader memory_reader;
        TransmitMemoryFixedUtil fixed_memory_util = TransmitMemoryFixedUtil(0, nullptr, thread_util);

        if(!fixed_memory_util.onCreateMemory(create_len)){
            return;
        }
        ReaderIterator<MsgHdr> media_msg = ReaderIterator<MsgHdr>((memory_reader = fixed_memory_util.onExtractMemory()).readMemory<MsgHdr>());

        TransmitOutputInfo output_info = TransmitOutputInfo(create_len, 0, 0, 0, media_msg.operator->(), nullptr, thread_util);
        output_info.onNoteCompileOutput(thread_util->getTransmitAnalysisUtil(), transmit_msg);

        //这里只需要设置地址数量（序号由各调用函数设置、视音频资源由解析函数设置）
        media_msg->address_number = transmit_msg->address_number;

        for(int i = 0; i < note_size; i++){
            output_info.setRemoteNote(note + i);
            transmit_layer->onOutputLayer(&output_info);
        }

        media_msg->master_type = LAYER_MASTER_EXIT_PASSIVE;
        for(int i = 0; i < note_size; i++){
            if(address_layer->isRemoteNoteTimeout(note + i, timeout_threshold, transmit_sequence)){
                onInput(media_msg.operator->(), note + i, *address_layer, CONTROL_INPUT_FLAG_INPUT);
            }
        }
    }
}

/**
 * 该函数将media资源的解析放在ExecutorThread线程上（运行在Loop线程）
 * @param executor_note_info
 * @param callback LoopThread线程回调函数
 */
void MasterCentralization::onNoteExecutor0(ExecutorNoteInfo *executor_note_info, const MediaAnalysisFunc &callback) {
    auto func = [&]() -> void {
        MasterCentralization::onMediaAnalysis0(executor_note_info, callback);
    };

    callControlLayerFuncNotMsg(getControlLayerUtil(layer_array, LAYER_EXECUTOR_TYPE), (LayerFunc)&LayerControlUtil::callLayerFuncOnInput,
                               [&](MsgHdr *time_msg) -> void {
                                   MsgHdrUtil<ExecutorTimerData>::initMsgHdr(time_msg, sizeof(ExecutorTimerData), LAYER_CONTROL_TIMER_IMMEDIATELY,
                                                                             ExecutorTimerData(0, 0, 0, func, nullptr));
                               }, sizeof(ExecutorTimerData));
}

/**
 *  该函数运行在ExecutorThread线程上
 */
void MasterCentralization::onMediaAnalysis0(ExecutorNoteInfo *executor_note_info, const MediaAnalysisFunc &callback){
#define MASTER_ANALYSIS_SINGLE_VALUE    1
    char buffer[sizeof(MsgHdr) + sizeof(TransmitCodeInfo)];
    auto media_msg = reinterpret_cast<MsgHdr*>(buffer);

    auto media_func = [&](uint32_t time_pos, uint32_t submit_pos) -> MediaData* {
        return executor_note_info->getTimeInfoOnPos(time_pos)->getSubmitInfo(submit_pos)->getTimeBuffer();
    };

    TimeInfo *time_info = nullptr;
    MediaData *media_data = nullptr;
    NoteMediaInfo note_media_info = {.media_len = 0, .media_offset = 0, .note_status = TransmitNoteStatus(0, 0, 0), .media_data = nullptr};

    for(uint32_t time_pos = 0, time_end = executor_note_info->getTimeInfoSize(), msg_len = 0, media_len = 0; time_pos < time_end; time_pos++, media_len = 0){
        time_info = executor_note_info->getTimeInfoOnPos(time_pos);
        MediaData *media_array[time_info->getSubmitInfoSize()];

        for(uint32_t submit_pos = 0, submit_end = time_info->getSubmitInfoSize(); submit_pos < submit_end; submit_pos++){
            ControlTransmitUtil::onTransmitSortMedia(media_array, media_data = media_func(time_pos, submit_pos), submit_pos, time_info->getSubmitInfoSize());
            media_data->analysis_media_func(media_data, &note_media_info, MASTER_ANALYSIS_SINGLE_VALUE);
            media_len += note_media_info.media_len;
        }

        msg_len = onCalculateMediaMsgLen(media_len, executor_note_info->getNoteInfoSize());
        media_msg->address_number = static_cast<short>(executor_note_info->getNoteInfoSize());

        MsgHdrUtil<TransmitCodeInfo>::initMsgHdr(media_msg, msg_len, LAYER_MASTER_MEDIA,
                                     TransmitCodeInfo{.is_ignore_note = true, .media_len = media_len, .media_size = time_info->getSubmitInfoSize(),
                                                     .transmit_sequence = time_info->getInfoSequence(), .media_data = media_array[0],
                                                     .search_func = ControlTransmitUtil::onTransmitSearchMedia,
                                                     .status_func = ControlTransmitUtil::onTransmitNoteStatus,
                                                     .note_func = std::bind(&ExecutorNoteInfo::getNoteInfoOnPos, executor_note_info, std::placeholders::_1)});

        callback(media_msg);
    }
}

void MasterCentralization::onDisplayAnalysis0(TimeSubmitInfo *submit_info, DisplaySubmitFunc display_func) {
#define MASTER_NOTE_ANALYSIS_VALUE  1

    uint32_t media_size = 0;
    TimeInfo *time_info = nullptr;
    MediaData *media_data = nullptr;
    auto display_layer = getControlLayer<DisplayLayer>(layer_array, LAYER_DISPLAY_TYPE);

    for(int i = 0; i < submit_info->submit_info_size; i++){
        media_size = std::max(media_size, (submit_info->submit_info_array + i)->getSubmitInfoSize());
    }

    NoteMediaInfo note_media_array[media_size];
    for(int i = 0, submit_size = 0; i < submit_info->submit_info_size; i++){
        submit_size = (time_info = submit_info->submit_info_array + i)->getSubmitInfoSize();

        for(int pos = 0; pos < submit_size; pos++){
            media_data = time_info->getSubmitInfo(static_cast<uint32_t>(pos))->getTimeBuffer();
            media_data->analysis_media_func(media_data, note_media_array + pos, MASTER_NOTE_ANALYSIS_VALUE);
        }

        display_func(display_layer, note_media_array, static_cast<uint32_t>(submit_size));
    }
}

RemoteNoteInfo* MasterCentralization::onMatchInputAddress(sockaddr_in match_addr) {
    MsgHdr *match_msg = nullptr;
    auto match_note = getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onMatchAddress(match_addr);

    if(!match_note){
        callControlLayerFuncNotMsg(getControlLayerUtil(layer_array, LAYER_ADDRESS_TYPE), (LayerFunc)&LayerControlUtil::callLayerFuncOnInput,
                                    [&](MsgHdr *msg) -> void {
                                        MsgHdrUtil<sockaddr_in>::initMsgHdr(msg, sizeof(sockaddr_in), LAYER_MASTER_CREATE, match_addr);
                                 }, [&](MsgHdr *result_msg) -> void {
                                        if(result_msg->serial_number >= sizeof(MeetingAddressNote*)){
                                            match_note = reinterpret_cast<AddressNoteInfo*>(match_msg->buffer);
                                        }
                                 }, std::max(sizeof(sockaddr_in), sizeof(MeetingAddressNote*)));
    }

    return dynamic_cast<RemoteNoteInfo*>(match_note);
}

void MasterCentralization::onLaunchCentralization(MsgHdr *launch_msg) {
    using intercept_func = std::function<bool(MsgHdr*, MeetingAddressNote*)>;

    auto func = [=](MsgHdr *msg, MeetingAddressNote *note_info) -> bool {
        return MasterCentralization::onTransmitOutputIntercept(msg, note_info);
    };

    if(launch_msg->serial_number < sizeof(SequenceData)){
        default_sequence_data = master_default_sequence;
    }else{
        default_sequence_data = *reinterpret_cast<SequenceData*>(launch_msg->buffer);
    }

    //初始化拦截
    callControlLayerFuncNotMsg(getControlLayerUtil(layer_array, LAYER_TRANSMIT_TYPE), (LayerFunc)&LayerControlUtil::callLayerFuncOnControl,
                               [&](MsgHdr *msg) -> void {
                                   MsgHdrUtil<void*>::initMsgHdr(msg, sizeof(InterceptFunc), TRANSMIT_LAYER_SHARED_INTERCEPT,
                                                                 reinterpret_cast<void*>((InterceptFunc)&MasterCentralization::onTransmitSynchroIntercept));
                               }, sizeof(MsgHdr) + sizeof(InterceptFunc));

    callControlLayerFuncNotMsg(getControlLayerUtil(layer_array, LAYER_TRANSMIT_TYPE), (LayerFunc)&LayerControlUtil::callLayerFuncOnControl,
                               [&](MsgHdr *msg) -> void {
                                   MsgHdrUtil<intercept_func>::initMsgHdr(msg, sizeof(intercept_func), TRANSMIT_LAYER_OUTPUT_INTERRCEPT, func);
                               }, sizeof(MsgHdr) + sizeof(intercept_func));
}

void MasterCentralization::onStopCentralization(MsgHdr*) {
    default_sequence_data = SequenceData();
}

void MasterCentralization::onAllocDynamic(MsgHdr *alloc_msg) {
    uint32_t alloc_len = alloc_msg->serial_number;

    if((alloc_msg->shared_type != LAYER_TRANSMIT_TYPE) || (alloc_msg->serial_number <= 0)){
        alloc_msg->serial_number = 0;
    }else{
        alloc_msg->master_type = MEMORY_LAYER_VARIETY_MEMORY;
        getControlLayer<MemoryLayer>(layer_array, LAYER_MEMORY_TYPE)->onInput(alloc_msg);
        alloc_msg->master_type = LAYER_CONTROL_REQUEST_ALLOC_DYNAMIC;
    }


    if((alloc_msg->serial_number <= 0) && (alloc_msg->response_type == MEMORY_LAYER_BAN_VARIETY_MEMORY) && (alloc_len > 0)){
        onInput(MsgHdrUtil<char*>::initMsgHdr(alloc_msg, alloc_len, MEMORY_LAYER_FIXED_MEMORY, nullptr), nullptr,
                         *getControlLayer<TransmitLayer>(layer_array, LAYER_TRANSMIT_TYPE) , CONTROL_INPUT_FLAG_CONTROL);
        alloc_msg->master_type = LAYER_CONTROL_REQUEST_ALLOC_FIXED;
    }
}

/**
 * 销毁动态内存
 * @param dealloc_msg
 */
void MasterCentralization::onDeallocDynamic(MsgHdr *dealloc_msg) {
    /*
     * 实际上不需要处理,该函数被调用并传递动态内存,需要销毁动态内存
     */
    if((dealloc_msg->shared_type == LAYER_TRANSMIT_TYPE) && (dealloc_msg->serial_number >= sizeof(MemoryReader))){
        /*
         * 将MemoryReader从参数中移动赋值给栈变量
         * 栈变量在析构函数中会释放内存
         */
        MemoryReader memory_reader = std::move(*reinterpret_cast<MemoryReader*>(dealloc_msg->buffer));
    }
}