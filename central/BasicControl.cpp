//
// Created by abc on 20-12-8.
//

#include "BasicControl.h"

//--------------------------------------------------------------------------------------------------------------------//

static RequestInfo init_info        = RequestInfo();                //初始化请求函数
static RequestInfo launch_info      = RequestInfo();                //启动请求函数
static RequestInfo stop_info        = RequestInfo();                //停止请求函数

AddressLayerUtil    address_util    = AddressLayerUtil();           //地址层工具（创建、销毁）
DisplayLayerUtil    display_util    = DisplayLayerUtil();           //显示层工具（创建、销毁）
ExecutorLayerUtil   executor_util   = ExecutorLayerUtil();          //执行层工具（创建、销毁）
MemoryLayerUtil     memory_util     = MemoryLayerUtil();            //内存层工具（创建、销毁）
TimerLayerUtil      timer_util      = TimerLayerUtil();             //时间层工具（创建、销毁）
TransmitLayerUtil   transmit_util   = TransmitLayerUtil();          //传输层工具（创建、销毁）
UserLayerUtil       user_util       = UserLayerUtil();              //用户层工具（创建、销毁）

//--------------------------------------------------------------------------------------------------------------------//

TransmitNoteStatus ControlTransmitUtil::onTransmitNoteStatus(MeetingAddressNote *transmit_note) {
    return AddressLayer::onNoteTransmitStatus(transmit_note);
}

void ControlTransmitUtil::onTransmitSortMedia(MediaData **media_array, MediaData *media_data, uint32_t pos, uint32_t size) {
    if(pos >= size){
        return;
    }

    uint32_t value = onTransmitSearch0(*media_array, media_data->media_note, pos);

    if(value >= pos){
        media_array[pos] = media_data;
    }else{
        memmove(media_array + (value + 1), media_array + value, sizeof(MediaData*) * (pos - value));
        media_array[value] = media_data;
    }
}

uint32_t ControlTransmitUtil::onTransmitSearchMedia(TransmitCodeInfo *code_info, MeetingAddressNote *search_note) {
    return onTransmitSearch0(code_info->media_data, search_note, code_info->media_size);
}

uint32_t ControlTransmitUtil::onTransmitSearch0(MediaData *media_array, MeetingAddressNote *search_note, uint32_t end) {
    int start_pos = 0, mid_pos = 0, end_pos = (static_cast<int>(end) - 1);
    MeetingAddressNote *mid_note = nullptr;

    for(;;){
        mid_pos = (start_pos + end_pos) / 2;
        mid_note = (media_array + mid_pos)->media_note;

        if(onTransmitNoteCompare(mid_note, search_note)){
            if((start_pos = mid_pos + 1) > end_pos){
                return static_cast<uint32_t>(start_pos);
            }
        }else{
            if(mid_note == search_note){
                return static_cast<uint32_t>(mid_pos);
            }

            if((end_pos = mid_pos - 1) < start_pos){
                return static_cast<uint32_t>(start_pos);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------//

BasicControl::BasicControl(ControlLaunchMode mode) : control_mode(mode), control_status(CONTROL_STATUS_NONE), input_info(this) {
    init();
}

BasicControl::~BasicControl() {}

void BasicControl::init() {
    std::uninitialized_fill(request_info, request_info + CONTROL_STATUS_SIZE, nullptr);
    setRequestFunc(request_info, CONTROL_STATUS_INIT, &init_info, (RequestFunc)&BasicControl::onInit);
}

bool BasicControl::canLaunch() {
    bool can_launch = true;

    switch (getControlStatus()){
        case CONTROL_STATUS_NONE:
            if((control_mode == CONTROL_LAUNCH_IMMEDIATE) || (control_mode == CONTROL_LAUNCH_USE_START)){
                can_launch = false;
            }
            break;
        case CONTROL_STATUS_INIT:
            if((control_mode == CONTROL_LAUNCH_IMMEDIATE) || (control_mode == CONTROL_LAUNCH_START_INIT)){
                can_launch = false;
            }
            break;
        case CONTROL_STATUS_LAUNCH:
            std::cout << "控制器正在启动！" << std::endl;
            break;
        case CONTROL_STATUS_RUNNING:
            std::cout << "控制器正在运行！" << std::endl;
            break;
        case CONTROL_STATUS_STOP:
            setControlStatus(CONTROL_STATUS_INIT);
            break;
        default:
            can_launch = false;
            break;
    }

    return can_launch;
}

bool BasicControl::canStop() {
    bool can_stop = false;

    switch (getControlStatus()){
        case CONTROL_STATUS_NONE:
        case CONTROL_STATUS_INIT:
            std::cout << "控制器未启动！" << std::endl;
            break;
        case CONTROL_STATUS_LAUNCH:
            std::cout << "控制器正在启动,请等待启动完成后再停止！" << std::endl;
            break;
        case CONTROL_STATUS_RUNNING:
            can_stop = true;
            std::cout << "控制器正在运行,可以停止！" << std::endl;
            break;
        case CONTROL_STATUS_STOP:
            std::cout << "控制器已停止！" << std::endl;
            break;
        default:
            break;
    }

    return can_stop;
}

void BasicControl::launchControl(MsgHdr *launch_msg) throw(std::runtime_error){
    if(!launch_msg || (launch_msg->serial_number <= 0)){
        throw std::runtime_error("启动控制器失败！");
    }

    if(!canLaunch()){
        throw std::runtime_error("启动控制器失败！");
    }
    try {
        launchControl0(launch_msg);
    }catch (std::runtime_error &e){
        throw;
    }catch (std::bad_alloc &e){
        throw std::runtime_error("初始化控制器失败！");
    }
}

void BasicControl::stopControl(MsgHdr *stop_msg) {
    if(!canStop()){
        return;
    }

    (this->*getRequestInfo(this, getControlStatus())->func)(stop_msg);
}

void BasicControl::launchControl0(MsgHdr *launch_msg) throw(std::bad_alloc, std::runtime_error) {
    RequestInfo *launch_info = nullptr;

    for(;;) {
        if((launch_info = getRequestInfo(this, getControlStatus()))) {
            (this->*launch_info->func)(launch_msg);
        }else{
            break;
        }
    }
}

void BasicControl::onInit(MsgHdr*) throw(std::bad_alloc) {
    try {
        requestInit();
    }catch (std::bad_alloc &e){
        std::cout << "BasicControl::onInit->throw" << std::endl; throw;
    }

    setRequestFunc(request_info, CONTROL_STATUS_LAUNCH, &launch_info, (RequestFunc)&BasicControl::onLaunch);
    setControlStatus(CONTROL_STATUS_INIT);
}

void BasicControl::onLaunch(MsgHdr *launch_msg) {
    requestLaunch(launch_msg);
    setControlStatus(CONTROL_STATUS_LAUNCH);
}

void BasicControl::onLaunchComplete(std::promise<void> &promise) {
    onRunning(nullptr);
    promise.set_value();
}

void BasicControl::onRunning(MsgHdr*) {
    setRequestFunc(request_info, CONTROL_STATUS_STOP, &stop_info, (RequestFunc)&BasicControl::onStop);
    setControlStatus(CONTROL_STATUS_RUNNING);
}

void BasicControl::onStop(MsgHdr *stop_msg) {
    requestStop(stop_msg);
    resetRequestFunc(request_info, CONTROL_STATUS_LAUNCH);
    resetRequestFunc(request_info, CONTROL_STATUS_STOP);
    setControlStatus(CONTROL_STATUS_STOP);
}