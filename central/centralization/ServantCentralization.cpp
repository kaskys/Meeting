//
// Created by abc on 20-12-8.
//

#include "ServantCentralization.h"

//--------------------------------------------------------------------------------------------------------------------//

void ServantCentralization::onLaunchCentralization(MsgHdr*) {
    using intercept_func = std::function<bool(MsgHdr*, MeetingAddressNote*)>;
    auto func = [=](MsgHdr *msg, MeetingAddressNote *note_info) -> bool {
        return ServantCentralization::onTransmitOutputIntercept(msg, note_info);
    };

    //设置拦截函数（输出）
    callControlLayerFuncNotMsg(getControlLayerUtil(layer_array, LAYER_TRANSMIT_TYPE),  (LayerFunc)&LayerControlUtil::callLayerFuncOnControl,
                               [&](MsgHdr *msg) -> void {
                                   MsgHdrUtil<intercept_func>::initMsgHdr(msg, sizeof(intercept_func), TRANSMIT_LAYER_OUTPUT_INTERRCEPT, func);
                               }, sizeof(intercept_func));

    //设置拦截函数（输入）
    callControlLayerFuncNotMsg(getControlLayerUtil(layer_array, LAYER_TRANSMIT_TYPE),  (LayerFunc)&LayerControlUtil::callLayerFuncOnControl,
                               [&](MsgHdr *msg) -> void {
                                   MsgHdrUtil<void*>::initMsgHdr(msg, sizeof(InterceptFunc), TRANSMIT_LAYER_SHARED_INTERCEPT,
                                                                         reinterpret_cast<void*>((InterceptFunc)&ServantCentralization::onTransmitSynchroIntercept));
                               }, sizeof(InterceptFunc));
}

void ServantCentralization::onStopCentralization(MsgHdr*) {
    synchro_util.onClearSynchro();
}

void ServantCentralization::onTransmitMasterInput(TransmitJoin *transmit_join) {
    //需要处理,因为中心化远程端不能接收其他地址的加入请求,但会处理用户层的加入主机请求
    if(!main_thread_util){
        return;
    }

    auto join_func = [&](RemoteNoteInfo *note_info) -> void {
        getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onNoteInputJoin(transmit_join->getTransmitMsg(), note_info);
    };

    callControlLayerFuncNotMsg(getControlLayerUtil(layer_array, LAYER_ADDRESS_TYPE), (LayerFunc)&LayerControlUtil::callLayerFuncOnInput,
                               [&](MsgHdr *msg) -> void {
                                   msg->shared_type = NOTE_STATUS_REMOTE;
                                   MsgHdrUtil<std::function<void(RemoteNoteInfo*)>>::initMsgHdr(msg, sizeof(std::function<void(RemoteNoteInfo*)>), LAYER_MASTER_INIT, join_func);
                               }, sizeof(std::function<void(RemoteNoteInfo*)>));
}

void ServantCentralization::onTransmitMasterInput(TransmitLink*) {
    //不需要处理,因为中心化远程端不能接收其他地址或中心化主机的链接请求
}

void ServantCentralization::onTransmitMasterInput(TransmitMedia *transmit_media) {
    if(!transmit_media){
        return;
    }

    onRemoteInput(dynamic_cast<TransmitType*>(transmit_media),
                  [&](RemoteNoteInfo *note_info, TransmitThreadUtil *thread_util) -> void {
                      getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onNoteInputNormal(transmit_media->getTransmitMsg(), note_info);

                      callControlLayerFuncNotMsg(getControlLayerUtil(layer_array, LAYER_TIME_TYPE), (LayerFunc)&LayerControlUtil::callLayerFuncOnInput,
                                                 [&](MsgHdr *msg) -> void {
                                                     MsgHdrUtil<MediaData>::initMsgHdr(msg, sizeof(MediaData), LAYER_MASTER_MEDIA, transmit_media->getMediaData());
                                                 }, sizeof(MediaData));
                  });
}

void ServantCentralization::onTransmitMasterInput(TransmitExit *transmit_exit) {
    if(!transmit_exit){
        return;
    }

    getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onNoteInputExit(
                     transmit_exit->getTransmitMsg(), dynamic_cast<AddressNoteInfo*>(transmit_exit->getTransmitNote()));
}

//--------------------------------------------------------------------------------------------------------------------//

//输出拦截
bool ServantCentralization::onTransmitOutputIntercept(MsgHdr *output_msg, MeetingAddressNote *output_note) {
    output_msg->msg_key = dynamic_cast<RemoteNoteInfo*>(output_note)->getNoteKey();
    output_msg->serial_number = synchro_util.onServantSequenceGap(output_msg->serial_number);
    return synchro_util.isServantSynchro();
}

//该函数作为拦截函数需要传给传输层(输入拦截)
bool ServantCentralization::onTransmitSynchroIntercept(MsgHdr *synchro_msg) {
    callOutputOnStackMemory(sizeof(MsgHdr) + std::max(sizeof(SynchroData), sizeof(AddressNoteInfo*)),
                            [&](MsgHdr *func_msg) -> void {
                                AddressNoteInfo *note_info = nullptr;

                                memcpy(func_msg, synchro_msg, sizeof(MsgHdr));
                                ServantAddressLayer::onNoteImmediateSynchro(getControlLayer<ServantAddressLayer>(layer_array, LAYER_ADDRESS_TYPE), func_msg);

                                note_info = reinterpret_cast<AddressNoteInfo*>(func_msg->buffer);
                                MsgHdrUtil<SynchroData>::initMsgHdr(func_msg, synchro_msg->serial_number, static_cast<uint32_t>(synchro_msg->master_type),
                                                                  *reinterpret_cast<SynchroData*>(synchro_msg->buffer));

                                onInput(func_msg, note_info, *getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE),
                                        CONTROL_INPUT_FLAG_OUTPUT | CONTROL_INPUT_FLAG_IMMEDIATELY_REPLY | CONTROL_INPUT_FLAG_THREAD_CORRELATE);
                            });
    return false;
}

void ServantCentralization::onDisplayAnalysis0(TimeSubmitInfo *submit_info, DisplaySubmitFunc display_func) {
#define SERVANT_NOTE_ANALYSIS_VALUE     1

    MediaData *media_data = nullptr;
    auto display_layer = getControlLayer<DisplayLayer>(layer_array, LAYER_DISPLAY_TYPE);

    for(int i = 0, media_size = 0; i < submit_info->submit_info_size; i++){
        media_data = (submit_info->submit_info_array + i)->getSubmitInfo(SERVANT_NOTE_ANALYSIS_VALUE)->getTimeBuffer();
        NoteMediaInfo note_media_array[(media_size = static_cast<int>(media_data->media_size))];

        media_data->analysis_media_func(media_data, note_media_array, media_size);
        display_func(display_layer, note_media_array, static_cast<uint32_t>(media_size));
    }
}

void ServantCentralization::onDisplayRelease0(TimeSubmitInfo *submit_info) {
    auto time_layer = getControlLayer<TimeLayer>(layer_array, LAYER_TIME_TYPE);

    for(int i = 0; i < submit_info->submit_info_size; i++){
        submit_info->submit_func(time_layer, submit_info->submit_info_array + i);
    }
}

void ServantCentralization::onNotePrevent(MsgHdr*, MeetingAddressNote *note_info) {
    /*
     * 向显示层输入
     *  1:显示加入主机失败（显示主机ip地址）
     *  2:确定键（确定返回主页面）
     */
    callControlLayerFuncNotMsg(getControlLayerUtil(layer_array, LAYER_DISPLAY_TYPE), (LayerFunc)&LayerControlUtil::callLayerFuncOnInput,
                               [&](MsgHdr *display_msg) -> void {
                                   MsgHdrUtil<DisplayNoteInfo>::initMsgHdr(display_msg, sizeof(DisplayNoteInfo),  DISPLAY_LAYER_DISPLAY_UPDATE,
                                                                           DisplayNoteInfo(DISPLAY_NOTE_STATUS_PREVENT, dynamic_cast<AddressNoteInfo*>(note_info)));
                               }, sizeof(DisplayNoteInfo));
}

void ServantCentralization::onNoteDisplay(MsgHdr *display_msg, MeetingAddressNote *note_info) {
    auto submit_media_info = reinterpret_cast<TimeSubmitInfo*>(display_msg->buffer);
    auto address_layer = getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE);

    callControlLayerFuncNotMsg(getControlLayerUtil(layer_array, LAYER_DISPLAY_TYPE), (LayerFunc)&LayerControlUtil::callLayerFuncOnInput,
                               [&](MsgHdr *msg) -> void {
                                   MsgHdrUtil<DisplayMediaInfo>::initMsgHdr(msg, sizeof(DisplayMediaInfo), DISPLAY_LAYER_DISPLAY_MEDIA,
                                                                DisplayMediaInfo(std::bind(&ServantCentralization::onDisplayRelease0, this, submit_media_info),
                                                                                 std::bind(&ServantCentralization::onDisplayAnalysis0, this, submit_media_info, std::placeholders::_1)));
                               }, sizeof(DisplayMediaInfo));


    if(address_layer->isRemoteNoteTimeout(note_info, submit_media_info->submit_timeout, submit_media_info->submit_sequence)){
        display_msg->master_type = LAYER_MASTER_EXIT_PASSIVE;
        display_msg->serial_number = submit_media_info->submit_sequence;

        onInput(display_msg, note_info, *address_layer, CONTROL_INPUT_FLAG_INPUT);
    }
}

void ServantCentralization::onNoteLink(MsgHdr *link_msg, MeetingAddressNote*) {
    MsgHdrUtil<SequenceData>::initMsgHdr(link_msg, sizeof(SequenceData), static_cast<uint32_t>(link_msg->master_type),
                                         SequenceData{0, 0, 0, 0});
}

void ServantCentralization::onNoteMedia(MsgHdr*, MeetingAddressNote*) {
    //不需要处理
}

void ServantCentralization::onNotePassive(MsgHdr*, MeetingAddressNote *note_info) {
    getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onNoteInputPassive(nullptr, dynamic_cast<AddressNoteInfo*>(note_info));
}

/**
 * 由执行函数调用（ExecutorThread线程）
 * @param exit_msg
 */
void ServantCentralization::onNoteExit(MsgHdr *exit_msg, MeetingAddressNote *info) {
    auto note_info = dynamic_cast<RemoteNoteInfo*>(info);
    auto thread_util = note_info->correlateThreadUtil();

    if(thread_util){
        exit_msg->serial_number = static_cast<uint32_t>(control_timer_sequence);
        thread_util->onRunCorrelateThreadImmediate([&]() -> void {
            getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onNoteInputExit(exit_msg, note_info);
        });
    }
}

void ServantCentralization::onSequenceGap(MsgHdr *msg, MeetingAddressNote*) {
    onDriveOutput(msg, nullptr, CONTROL_INPUT_FLAG_HOST);
}

void ServantCentralization::onSequenceFrame(MsgHdr *frame_msg) {
    main_thread_util->onRunCorrelateThreadImmediate([&]() -> void {
        onSequenceFrame0(reinterpret_cast<MediaData*>(frame_msg->buffer), frame_msg->len_source, frame_msg->serial_number);
    });
}

void ServantCentralization::onSequenceFrame0(MediaData *media_data, uint32_t media_len, uint32_t sequence) {
    uint32_t create_len = sizeof(MsgHdr) + sizeof(short) + sizeof(TransmitNoteStatus) + media_len;
    create_len += TransmitLayer::onRemoteNoteCompileLen(main_thread_util, media_data->media_note);

    {
        MemoryReader memory_reader;
        TransmitMemoryFixedUtil fixed_memory_util = TransmitMemoryFixedUtil(0, nullptr, main_thread_util);
        if(!fixed_memory_util.onCreateMemory(static_cast<int>(create_len))){
            return;
        }

        ReaderIterator<MsgHdr> media_msg = fixed_memory_util.onExtractMemory().readMemory<MsgHdr>();
        TransmitOutputInfo output_info = TransmitOutputInfo(create_len, 0 ,0 ,0, media_msg.operator->(), media_data->media_note, main_thread_util);
        media_msg->address_number = 1;

        callOutputOnStackMemory(sizeof(MsgHdr) + sizeof(TransmitCodeInfo),
                                [&](MsgHdr *compile_msg) -> void {
                                    MsgHdrUtil<TransmitCodeInfo>::initMsgHdr(compile_msg, sequence,
                                                                             LAYER_MASTER_MEDIA, initTransmitCodeInfo(media_data, media_len + sizeof(TransmitStatus), sequence));
                                    compile_msg->serial_number = 1;
                                    output_info.onNoteCompileOutput(main_thread_util->getTransmitAnalysisUtil(), compile_msg);
                                });
        getControlLayer<TransmitLayer>(layer_array, LAYER_TRANSMIT_TYPE)->onOutputLayer(&output_info);
    }
}

TransmitCodeInfo ServantCentralization::initTransmitCodeInfo(MediaData *media_data, uint32_t media_len, uint32_t sequence) {
    TransmitCodeInfo code_info{};
    code_info.is_ignore_note = false;
    code_info.media_len = media_len;
    code_info.media_size = 1;
    code_info.transmit_sequence = sequence;
    code_info.media_data = media_data;
    code_info.search_func = ControlTransmitUtil::onTransmitSearchMedia;
    code_info.status_func = ControlTransmitUtil::onTransmitNoteStatus;
    code_info.note_func = [&](uint32_t) -> MeetingAddressNote* { return getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onHostNote(); };
    return code_info;
}

void ServantCentralization::onCentralizationTimeDrive(MsgHdr *drive_msg) {
    switch (drive_msg->master_type){
        case LAYER_MASTER_TIME_DRIVE:
            break;
        case LAYER_CONTROL_CORE_SEQUENCE_GAP:
            synchro_util.onServantSynchro(drive_msg);
            break;
        default:
            break;
    }
}

RemoteNoteInfo* ServantCentralization::onMatchInputAddress(sockaddr_in match_addr) {
    return dynamic_cast<RemoteNoteInfo*>(getControlLayer<AddressLayer>(layer_array, LAYER_ADDRESS_TYPE)->onMatchAddress(match_addr));
}

void ServantCentralization::onAllocDynamic(MsgHdr *alloc_msg) {
    alloc_msg->serial_number = 0;
}

void ServantCentralization::onDeallocDynamic(MsgHdr *dealloc_msg) {
    dealloc_msg->serial_number = 0;
}