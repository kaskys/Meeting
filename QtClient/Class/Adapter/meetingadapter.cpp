#include "meetingadapter.h"

void* adapter_note_init(void *adapter, uint32_t pos, uint32_t port, uint32_t addr)
{
    if(adapter){
        return reinterpret_cast<MeetingAdapterCore*>(adapter)->onNoteInit(pos, port, addr);
    }else{
        return nullptr;
    }
}

void adapter_note_update(void *adapter, uint32_t pos, const char *status, const char *addr, void *holder)
{
    if(adapter){
        reinterpret_cast<MeetingAdapterCore*>(adapter)->onNoteUpdate(pos, status, addr, holder);
    }
}

void adapter_note_media(void *adapter, void *holder, const char *media_buffer, uint32_t media_len)
{
    if(adapter){
        reinterpret_cast<MeetingAdapterCore*>(adapter)->onNoteMedia(media_buffer, media_len, holder);
    }
}

void adapter_note_indicator(void *adapter, void *holder, const char *addr)
{
    if(adapter){
        reinterpret_cast<MeetingAdapterCore*>(adapter)->onNoteIndicator(holder, addr);
    }
}

//----------------------------------------------------------------------------------------------------------------------------//

AddressInitInfo IpInfo::onInitInfo(const IpInfo &ip_info, uint32_t frame)
{
    AddressInitInfo init_info;
    init_info.jtime = ip_info.jtime;
    init_info.ptime = ip_info.ptime;
    init_info.ltime = ip_info.jtime;
    init_info.stime = (1000 / frame) + ((1000 % frame) ? 1 : 0);
    init_info.time_size = ip_info.tsize;
    init_info.init_indicator_time = 0;
    sockaddr_in ain;
    ain.sin_port = ip_info.local_port;
    ain.sin_addr.s_addr = ip_info.local_addr;
    init_info.init_addr = ain;
//    init_info.link_addr = sockaddr_in{.sin_port = ip_info.link_port, .sin_addr.s_addr = ip_info.link_addr };
    return init_info;
}

IpInfo IpInfo::onInitInfo(const AddressInitInfo &init_info)
{
    IpInfo ip_info;
    ip_info.jtime = init_info.jtime;
    ip_info.ptime = init_info.ptime;
    ip_info.ltime = init_info.ltime;
    ip_info.tsize = init_info.time_size;
//    ip_info.link_port = init_info.link_addr.sin_port;
//    ip_info.link_addr = init_info.link_addr.sin_addr.s_addr;
    ip_info.local_port = init_info.init_addr.sin_port;;
    ip_info.local_addr = init_info.init_addr.sin_addr.s_addr;
    return ip_info;
}

TimeInitInfo TimeqInfo::onInitInfo(const TimeqInfo &time_info)
{
    TimeInitInfo init_info;
    init_info.timeout_frame = time_info.frame;
    init_info.delay_frame = time_info.delay;
    init_info.timeout_threshold = time_info.timeout;
    return init_info;
}

TimeqInfo TimeqInfo::onInitInfo(const TimeInitInfo &init_info)
{
    TimeqInfo time_info;
    time_info.frame = init_info.timeout_frame;
    time_info.delay = init_info.delay_frame;
    time_info.timeout =  init_info.timeout_threshold;
    time_info.note_size = init_info.need_submit;
    return time_info;
}

DisplayInitInfo MediaqInfo::onInitInfo(void *adapter)
{
    DisplayInitInfo init_info;

    init_info.audio_channel_count = 0;
    init_info.audio_byte_order = 0;
    init_info.audio_sample_size = 0;
    init_info.audio_sample_rate = 0;

    init_info.video_ratio = 0;
    init_info.video_resolution = 0;
    init_info.video_format = 0;

    init_info.adapter = adapter;
    init_info.init_func = adapter_note_init;
    init_info.note_func = adapter_note_update;
    init_info.media_func = adapter_note_media;
    init_info.indicator_func = adapter_note_indicator;

    return init_info;
}

DisplayInitInfo MediaqInfo::onInitInfo(void *adapter, const MediaqInfo &media_info)
{
    DisplayInitInfo init_info;

    init_info.audio_channel_count = media_info.channel_count;
    init_info.audio_byte_order = media_info.byte_order;
    init_info.audio_sample_size = media_info.sample_size;
    init_info.audio_sample_rate = media_info.sample_rate;

    init_info.video_ratio = media_info.ratio;
    init_info.video_resolution = media_info.resolution;
    init_info.video_format = media_info.format;

    init_info.adapter = adapter;
    init_info.init_func = adapter_note_init;
    init_info.note_func = adapter_note_update;
    init_info.media_func = adapter_note_media;
    init_info.indicator_func = adapter_note_indicator;

    return init_info;
}

MediaqInfo MediaqInfo::onInitInfo(const DisplayInitInfo &init_info)
{
    MediaqInfo media_info;

    media_info.channel_count = init_info.audio_channel_count;
    media_info.byte_order = init_info.audio_byte_order;
    media_info.sample_size = init_info.audio_sample_size;
    media_info.sample_rate = init_info.audio_sample_rate;

    media_info.ratio = init_info.video_ratio;
    media_info.resolution = init_info.video_resolution;
    media_info.format = init_info.video_format;

    return media_info;
}

void MeetingAdapterCore::onLaunch(const IpInfo &ip_info, const TimeqInfo &time_info, const MediaqInfo &media_info) throw(std::runtime_error)
{
    AddressInitInfo ip_init = IpInfo::onInitInfo(ip_info, time_info.frame);
    TimeInitInfo time_init = TimeqInfo::onInitInfo(time_info);
    DisplayInitInfo display_init = MediaqInfo::onInitInfo(reinterpret_cast<void*>(this), media_info);

    cholder->launchControl(cholder->onInitLaunch(ip_init, time_init, display_init));
}

void MeetingAdapterCore::onLink(const IpInfo &ip_info) throw(std::runtime_error)
{
    AddressInitInfo ip_init = IpInfo::onInitInfo(ip_info, 1);
    DisplayInitInfo display_init = MediaqInfo::onInitInfo(reinterpret_cast<void*>(this));

    cholder->launchControl(cholder->onInitLaunch(ip_init, TimeInitInfo{}, display_init));
}

void MeetingAdapterCore::onStop()
{
    cholder->stopControl(nullptr);
}

void MeetingAdapterCore::agreeNoteIndicator(uint32_t pos)
{
    cholder->agreeNoteIndicator(pos);
}

void MeetingAdapterCore::completeMediaBuffer(const char *buffer, uint32_t len)
{
    cholder->completeMediaBuffer(buffer, len);
}

void MeetingAdapterCore::onInputVideoFrame(uint32_t len, uchar *buffer, QVideoFrame &frame)
{
    cholder->inputVideoFrame(len, buffer, [=]() mutable -> void { onNoteVideoFrame(frame); });
}

void MeetingAdapterCore::onInputAudioFrame(uint32_t len, char *buffer)
{
    cholder->inputAudioFrame(len, buffer);
}

void MeetingAdapterCore::onLaunchComplete(const AddressInitInfo &ainfo, const TimeInitInfo &tinfo, const DisplayInitInfo &dinfo)
{
    madapter->onMeetingInputLaunch(IpInfo::onInitInfo(ainfo), TimeqInfo::onInitInfo(tinfo), MediaqInfo::onInitInfo(dinfo));
}

void* MeetingAdapterCore::onNoteInit(uint32_t pos, uint32_t ip_port, uint32_t ip_addr)
{
    return madapter->onNoteInit(pos, ip_port, ip_addr);
}

void MeetingAdapterCore::onNoteUpdate(uint32_t pos, const char *status, const char *addr, void *holder)
{
    madapter->onNoteUpdate(pos, status, addr, holder);
}

void MeetingAdapterCore::onNoteMedia(const char *media_buffer, uint32_t media_len, void *holder)
{
    madapter->onNoteMedia(media_buffer, media_len, holder);
}

void MeetingAdapterCore::onNoteIndicator(void *holder, const char *addr)
{
    madapter->onNoteIndicator(holder, addr);
}

void MeetingAdapterCore::onNoteVideoFrame(QVideoFrame &frame)
{
    if(frame.isValid() && frame.isMapped()){
        frame.unmap();
    }
}

//-----------------------------------------------------------------------------------------------//

void QtAdapterControl::onLocalLaunch(MeetingCoreMode mode, const IpInfo &ip_info, const TimeqInfo &time_info, const MediaqInfo &media_info)
{
    if(mode == FAILI_MODE){
emit    on_adapter_local_launch();
    }else if(mode == CLIENT_MODE){
emit    on_adapter_client_launch(ip_info, time_info, media_info);
    }else if(mode == SERVER_MODE){
emit    on_adapter_server_launch(ip_info, time_info, media_info);
    }
}

void* QtAdapterControl::onRemoteInputInit(uint32_t pos, uint32_t port, uint32_t addr)
{
    char *note = new (std::nothrow) char[init_len];
    if(note){
emit    on_adapter_input_init(reinterpret_cast<void*>(note), pos, port, addr);
    }
    return note;
}

void QtAdapterControl::onRemoteInputUpdate(uint32_t pos, const char *status, const char *addr, void *holder)
{
emit on_adapter_input_update(pos, status, addr, holder);
}

void QtAdapterControl::onRemoteInputMedia(const char *buffer, uint32_t len, void *holder)
{
emit on_adapter_input_media(buffer, len, holder);
}

void QtAdapterControl::onRemoteInputIndicator(void *holder, const char *addr)
{
emit on_adapter_input_indicator(holder, addr);
}

void QtAdapterControl::onRemoteOutputIndicator(uint32_t pos)
{
    madapter->onMeetingNoteAgree(pos);
}

void QtAdapterControl::onRemoteOutputMedia(const char *buffer, uint32_t len)
{
    madapter->onMeetingMediaComplete(buffer, len);
}


//-----------------------------------------------------------------------------------------------//

MeetingAdapter::MeetingAdapter() : qt_adapter(), core_apapter()
{

}

MeetingAdapter::~MeetingAdapter()
{

}

void MeetingAdapter::onInitQtAdapter(uint32_t init_len)
{
    qt_adapter.onInit(this, init_len);
}

void MeetingAdapter::onMeetingLaunch(const IpInfo &ip_info, const TimeqInfo &time_info, const MediaqInfo &media_info)
{
    try{
        core_apapter.onLaunch(ip_info, time_info, media_info);
    }catch(std::runtime_error &e){
        qt_adapter.onLocalLaunch(FAILI_MODE, ip_info, time_info, media_info);
    }
}

void MeetingAdapter::onMeetingLink(const IpInfo &ip_info, const TimeqInfo&, const MediaqInfo&)
{
    try{
        core_apapter.onLink(ip_info);
    }catch(std::runtime_error &e){
        qt_adapter.onLocalLaunch(FAILI_MODE, ip_info, TimeqInfo{}, MediaqInfo{});
    }
}

void MeetingAdapter::onMeetingStop()
{
    core_apapter.onStop();
}

void MeetingAdapter::onMeetingClose()
{
    core_apapter.onStop();
}

void MeetingAdapter::onMeetingNoteAgree(uint32_t pos)
{
    core_apapter.agreeNoteIndicator(pos);
}

void MeetingAdapter::onMeetingMediaComplete(const char *buffer, uint32_t len)
{
    core_apapter.completeMediaBuffer(buffer, len);
}

void MeetingAdapter::onUpdateVideoFrame(const QVideoFrame &frame)
{
    QVideoFrame copy_frame = frame;
    if(copy_frame.map(QAbstractVideoBuffer::ReadOnly)){
        core_apapter.onInputVideoFrame(copy_frame.bytesPerLine(copy_frame.height()), copy_frame.bits(), copy_frame);
    }
}

void MeetingAdapter::onUpdateAudioInput(int pos, int len, IOControl *io_control)
{
    if(len <= 0) { return; }
    char buffer[len];
    if(io_control->copyData(pos, buffer, len)){
        core_apapter.onInputAudioFrame(static_cast<uint32_t>(len), buffer);
    }
}

void MeetingAdapter::onMeetingInputLaunch(const IpInfo &ip_info, const TimeqInfo &time_info, const MediaqInfo &media_info)
{
    //由Link.add或port的有无判断SERVER和CLIENT模式
    if(ip_info.local_addr > 0){
        qt_adapter.onLocalLaunch(core_apapter.onCoreMode(), ip_info, time_info, media_info);
    }else{
        qt_adapter.onLocalLaunch(FAILI_MODE, ip_info, time_info, media_info);
    }
}

void* MeetingAdapter::onNoteInit(uint32_t pos, uint32_t port, uint32_t addr)
{
    return qt_adapter.onRemoteInputInit(pos, port, addr);
}

void MeetingAdapter::onNoteUpdate(uint32_t pos, const char *status, const char *addr, void *holder)
{
    qt_adapter.onRemoteInputUpdate(pos, status, addr, holder);
}

void MeetingAdapter::onNoteMedia(const char *buffer, uint32_t len, void *holder)
{
    qt_adapter.onRemoteInputMedia(buffer, len, holder);
}

void MeetingAdapter::onNoteIndicator(void *holder, const char *addr)
{
    qt_adapter.onRemoteInputIndicator(holder, addr);
}
