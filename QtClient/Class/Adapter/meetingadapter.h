#ifndef MEETINGADAPTER_H
#define MEETINGADAPTER_H

#include <QObject>
#include <QMetaObject>
#include <QVideoFrame>

#include "../Control/iocontrol.h"
#include "../Lib/meetingcore.h"

class MeetingAdapter;

struct IpInfo{
    static AddressInitInfo onInitInfo(const IpInfo&, uint32_t);
    static IpInfo onInitInfo(const AddressInitInfo&);

    uint32_t jtime;
    uint32_t ptime;
    uint32_t ltime;
    uint32_t tsize;
    uint32_t link_port;
    uint32_t link_addr;
    uint32_t local_port;
    uint32_t local_addr;
};

struct TimeqInfo{
    static TimeInitInfo onInitInfo(const TimeqInfo&);
    static TimeqInfo onInitInfo(const TimeInitInfo&);

    uint32_t frame;
    uint32_t delay;
    uint32_t timeout;
    uint32_t note_size;
};

struct MediaqInfo{
    static DisplayInitInfo onInitInfo(void*);
    static DisplayInitInfo onInitInfo(void*, const MediaqInfo&);
    static MediaqInfo onInitInfo(const DisplayInitInfo&);

    uint32_t channel_count;
    uint32_t byte_order;
    uint32_t sample_size;
    uint32_t sample_rate;

    uint32_t ratio;
    uint32_t resolution;
    uint32_t format;
};

void* adapter_note_init(void*, uint32_t, uint32_t, uint32_t);
void  adapter_note_update(void*, uint32_t, const char*, const char*, void*);
void  adapter_note_media(void*, void*, const char*, uint32_t);
void  adapter_note_indicator(void*, void*, const char*);

class MeetingAdapterCore {
    friend void* adapter_note_init(void*, uint32_t, uint32_t, uint32_t);
    friend void  adapter_note_update(void*, uint32_t, const char*, const char*, void*);
    friend void  adapter_note_media(void*, void*, const char*, uint32_t);
    friend void  adapter_note_indicator(void*, void*, const char*);
public:
    explicit MeetingAdapterCore() : madapter(nullptr), cholder(nullptr) {}
    ~MeetingAdapterCore() = default;

    bool isLaunch() const { return cholder->isLaunch(); }
    MeetingCoreMode onCoreMode() const { return cholder->onCoreMode(); }
    void onCoreMode(MeetingCoreMode mode) { cholder->onCoreMode(mode); }

    //输入BasicControl(Launch、Link默认同步启动,非异步)
    void onInit(MeetingAdapter *adapter, MeetingCoreHolder *holder) { madapter = adapter; cholder = holder; }
    void onLaunch(const IpInfo&, const TimeqInfo&, const MediaqInfo&) throw(std::runtime_error);
    void onLink(const IpInfo&) throw(std::runtime_error);
    void onStop();

    void agreeNoteIndicator(uint32_t);
    void completeMediaBuffer(const char*, uint32_t);

    void onInputVideoFrame(uint32_t, uchar*, QVideoFrame&);
    void onInputAudioFrame(uint32_t, char*);

    //异步启动回调函数
    void onLaunchComplete(const AddressInitInfo&, const TimeInitInfo&, const DisplayInitInfo&);

    //BasicControl输出
    void* onNoteInit(uint32_t, uint32_t, uint32_t);
    void onNoteUpdate(uint32_t, const char*, const char*, void*);
    void onNoteMedia(const char*, uint32_t, void*);
    void onNoteIndicator(void*, const char*);
    void onNoteVideoFrame(QVideoFrame&);
private:
    MeetingAdapter *madapter;
    MeetingCoreHolder *cholder;
};


class QtAdapterControl : public QObject {
    friend class MeetingAdapter;

    Q_OBJECT
public:
    QtAdapterControl() : init_len(0), madapter(nullptr) {}
    ~QtAdapterControl() = default;

    void onInit(MeetingAdapter *a, uint32_t len) { madapter = a; init_len = len; }

    //非qt线程
    void onLocalLaunch(MeetingCoreMode, const IpInfo&, const TimeqInfo&, const MediaqInfo&);
    void* onRemoteInputInit(uint32_t, uint32_t, uint32_t);
    void onRemoteInputUpdate(uint32_t, const char*, const char*, void*);
    void onRemoteInputMedia(const char*, uint32_t, void*);
    void onRemoteInputIndicator(void*, const char*);

    //qt线程
    void onRemoteOutputIndicator(uint32_t);
    void onRemoteOutputMedia(const char*, uint32_t);
Q_SIGNALS:
    void on_adapter_local_launch();
    void on_adapter_client_launch(IpInfo, TimeqInfo, MediaqInfo);
    void on_adapter_server_launch(IpInfo, TimeqInfo, MediaqInfo);
    void on_adapter_input_init(void*, uint32_t, uint32_t, uint32_t);
    void on_adapter_input_update(uint32_t, const char*, const char*, void*);
    void on_adapter_input_media(const char*,uint32_t, void*);
    void on_adapter_input_indicator(void*, const char*);
private:
    uint32_t init_len;
    MeetingAdapter *madapter;
};

class MeetingAdapter
{    
    friend class MeetingAdapterCore;
public:
    MeetingAdapter();
    ~MeetingAdapter();

    void onCorrelateControlCore(MeetingCoreHolder *holder) { core_apapter.onInit(this, holder); }
    void onInitQtAdapter(uint32_t);
    QtAdapterControl* onQtAdapter() { return &qt_adapter; }

    bool isLaunch() const { return core_apapter.isLaunch(); }
    MeetingCoreMode onCoreMode() const { return core_apapter.onCoreMode(); }
    void onCoreMode(MeetingCoreMode mode) { core_apapter.onCoreMode(mode); }

    //Qt输出
    void onMeetingLaunch(const IpInfo&, const TimeqInfo&, const MediaqInfo&);
    void onMeetingLink(const IpInfo&, const TimeqInfo&, const MediaqInfo&);
    void onMeetingStop();
    void onMeetingClose();

    void onMeetingNoteAgree(uint32_t);
    void onMeetingMediaComplete(const char*, uint32_t);

    void onUpdateVideoFrame(const QVideoFrame&);
    void onUpdateAudioInput(int, int, IOControl*);

    //Qt输入
    //异步启动返回函数（默认同步启动）
    void onMeetingInputLaunch(const IpInfo&, const TimeqInfo&, const MediaqInfo&);

    void* onNoteInit(uint32_t, uint32_t, uint32_t);
    void onNoteUpdate(uint32_t, const char*, const char*, void*);
    void onNoteMedia(const char*, uint32_t, void*);
    void onNoteIndicator(void*, const char*);
private:
    QtAdapterControl qt_adapter;
    MeetingAdapterCore core_apapter;
};

#endif // MEETINGADAPTER_H
