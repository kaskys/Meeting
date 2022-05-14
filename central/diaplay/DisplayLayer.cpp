//
// Created by abc on 21-10-7.
//
#include "../BasicControl.h"

BasicLayer* DisplayLayerUtil::createLayer(BasicControl *control) noexcept {
    return (new (std::nothrow) DisplayLayer(control));
}

void DisplayLayerUtil::destroyLayer(BasicControl*, BasicLayer *layer) noexcept {
    delete layer;
}

//--------------------------------------------------------------------------------------------------------------------//

DisplayLayerParameter DisplayLayer::parameter{};

void DisplayLayer::initLayer() {
    parameter.onClear();
}

void DisplayLayer::onInput(MsgHdr *msg) {
    switch (msg->master_type){
        case DISPLAY_LAYER_DISPLAY_UPDATE:
            if(msg->serial_number >= sizeof(DisplayNoteInfo)) { onDisplayUpdate(reinterpret_cast<DisplayNoteInfo*>(msg->buffer)); }
            break;
        case DISPLAY_LAYER_DISPLAY_MEDIA:
            if(msg->serial_number >= sizeof(DisplayMediaInfo)) { onDisplayMedia(reinterpret_cast<DisplayMediaInfo*>(msg->buffer)); }
            break;
        default:
            break;
    }
}

void DisplayLayer::onOutput() {
    //不需要处理
}

void DisplayLayer::onDrive(MsgHdr *drive_msg) {
    if(!thread_util){ return; }

    thread_util->onRunCorrelateThreadImmediate(
            [&]() -> void {
                parameter.onDisplayDrive();
                manager->onDisplayExtract(
                        [&](char *media_buffer, uint32_t media_len, const std::function<void(char*,int)> &rfunc) -> void {
                            onDisplayDrive(media_buffer, media_len, drive_msg->serial_number, rfunc);
                        });
            });
}

void DisplayLayer::onParameter(MsgHdr *msg) {
    if(!msg){ return; }

    if(msg->serial_number >= sizeof(DisplayLayerParameter)){
        parameter.onParameter(reinterpret_cast<DisplayLayerParameter*>(msg->buffer));
        msg->serial_number = sizeof(DisplayLayerParameter);
    }else{
        msg->serial_number = 0;
    }
}

void DisplayLayer::onControl(MsgHdr *msg) {
    switch (msg->master_type){
        case LAYER_CONTROL_STATUS_START:
            if(!onStartDisplayLayer(msg)) { msg->master_type = LAYER_CONTROL_STATUS_THROW; }
            break;
        case LAYER_CONTROL_STATUS_STOP:
            onStopDisplayLayer(msg);
            break;
        case DISPLAY_LAYER_CONTROL_THREAD:
            onDisplayCorrelateThread(msg);
            break;
        case DISPLAY_LAYER_CONTROL_PARAMETER:
            onDisplayParameter(msg);
            break;
        default:
            break;
    }
}

char* DisplayLayer::onCreateDisplayBuffer(int len) {
    if(len <= 0){ return nullptr; }

    return onCreateBuffer(static_cast<uint32_t>(len));
}

void DisplayLayer::onDestroyDisplayBuffer(char *buffer, int len) {
    onDestroyBuffer(sizeof(char*),
                    [&](MsgHdr *destroy_msg) -> void {
                        MsgHdrUtil<char*>::initMsgHdr(destroy_msg, static_cast<uint32_t>(len), LAYER_CONTROL_REQUEST_DESTROY_FIXED, buffer);
                    });
}

//--------------------------------------------------------------------------------------------------------------------//

bool DisplayLayer::onStartDisplayLayer(MsgHdr*) {
    try {
        manager = new DisplayManager(
                [&](int len) -> char* {
                    return onCreateDisplayBuffer(len);
             }, [&](char *buffer, int len) -> void {
                    onDestroyDisplayBuffer(buffer, len);
             });
        thread_util = new DisplayThreadUtil(this);
    }catch (std::bad_alloc &e){
        return false;
    }catch (std::runtime_error &e){
        return false;
    }
    return true;
}

void DisplayLayer::onStopDisplayLayer(MsgHdr*) {
    delete manager;
    manager = nullptr;
    parameter.onClear();
}

void DisplayLayer::onDisplayThrow() {
    BasicControl::callOutputOnStackMemory(sizeof(MsgHdr),
                                          [&](MsgHdr *throw_msg) -> void {
                                              basic_control->onInput(MsgHdrUtil<void>::initMsgHdr(throw_msg, LAYER_CONTROL_CORE_TERMINATION),
                                                                     nullptr, *this, CONTROL_INPUT_FLAG_CONTROL);
                                          });
}

void DisplayLayer::onDisplayDrive(char *media_buffer, uint32_t media_len, uint32_t sequence, const std::function<void(char*,int)> &rfunc) {
    BasicControl::callOutputOnStackMemory(sizeof(MsgHdr) + sizeof(NoteMediaInfo2),
                                          [&](MsgHdr *drive_msg) -> void {
                                              MsgHdrUtil<NoteMediaInfo2>::initMsgHdr(drive_msg, sequence, LAYER_CONTROL_DISPLAY_DRIVE_FRAME,
                                                                                     NoteMediaInfo2(static_cast<short>(media_len), media_buffer, rfunc));
                                              basic_control->onInput(drive_msg, nullptr, *this, CONTROL_INPUT_FLAG_CONTROL);
                                              manager->onNoteMedia(media_buffer, media_len, 0);
                                          });
}

void DisplayLayer::onDisplayUpdate(DisplayNoteInfo *display_info) {
    if(!thread_util || !display_info){ return; }

    thread_util->onRunCorrelateThreadImmediate(
            [&]() -> void {
                onDisplayUpdate0(display_info->note_status, MeetingAddressManager::getNoteGlobalPos(display_info->note_info), 0);
            });
}

void DisplayLayer::onDisplayUpdate0(DisplayStatus status, uint32_t pos, uint32_t addr) {
    parameter.onNoteUpdate();
    manager->onNoteUpdate(status, pos, addr);
}

void DisplayLayer::onDisplayMedia(DisplayMediaInfo *display_info) {
    if(!thread_util || !display_info){ return; }

    thread_util->onRunCorrelateThreadImmediate(
            [&]() -> void {
                bool is_throw = false;
                try {
                    display_info->media_func((void(*)(DisplayLayer*, NoteMediaInfo*, uint32_t))&DisplayLayer::onDisplayMedia0);
                }catch (std::bad_alloc &e){
                    is_throw = true;
                }

                if(display_info->release_func){ display_info->release_func(); }
                if(is_throw){ onDisplayThrow(); }
            });
}

void DisplayLayer::onDisplayMedia0(NoteMediaInfo *media_info, uint32_t media_size) throw(std::bad_alloc) {
    NoteMediaInfo *display_media_info = nullptr;

    static auto update_func = [=](DisplayManager *manager, TransmitNoteStatus *note_status){
        onDisplayUpdate0(static_cast<DisplayStatus>(note_status->note_status), note_status->note_position, note_status->note_address);
    };

    static auto media_func = [=](DisplayManager *manager, MediaData *data, short offset, short len, uint32_t pos) {
        //判断是否本机视音频资源
        if(pos <= 0) { return; }
        parameter.onDisplayMedia();
        manager->onNoteMedia(data->media_reader.readMemory<MsgHdr>()->buffer + offset, static_cast<uint32_t>(len), pos);
    };

    for(int pos = 0; pos < media_size; pos++){
        display_media_info = (media_info + pos);

        update_func(manager, &display_media_info->note_status);
        try {
            media_func(manager, display_media_info->media_data, display_media_info->media_offset,
                                          display_media_info->media_len, display_media_info->note_status.note_position);
        }catch (std::bad_alloc &e){
            std::cout << "DisplayLayer::onDisplayMedia0->" << display_media_info->note_status.note_position
                      << ":" << display_media_info->note_status.note_address << std::endl;
            throw;
        }
    }
}

void DisplayLayer::onDisplayParameter(MsgHdr *msg) {
    if(msg->serial_number < sizeof(DisplayParameter)){ return; }

    manager->onUpdateNoteParameter(*reinterpret_cast<DisplayParameter*>(msg->buffer), msg->len_source);
}

void DisplayLayer::onDisplayCorrelateThread(MsgHdr *msg) {
    auto display_thread = reinterpret_cast<DisplayWordThread*>(msg->buffer);

    thread_util->onCorrelateThread(display_thread);
    display_thread->onCorrelateDisplayUtil(thread_util);

}

//--------------------------------------------------------------------------------------------------------------------//

void DisplayThreadUtil::onRunCorrelateThreadImmediate(const std::function<void()> &func) {
    if(!isCorrelate()){ return; }

    display_thread->onRunDisplayThread(func);
}