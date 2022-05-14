//
// Created by abc on 20-12-8.
//

#ifndef TEXTGDB_BASICCONTROL_H
#define TEXTGDB_BASICCONTROL_H

#ifdef __cplusplus
extern "C"{
#endif
#include "ControlCoreUtil.h"
#ifdef __cplusplus
};
#endif

#include <memory>

#include "address/AddressLayer.h"
#include "executor/ExecutorLayer.h"
#include "memory/MemoryLayer.h"
#include "time/TimeLayer.h"
#include "transmit/TransmitLayer.h"
#include "display/DisplayLayer.h"
#include "user/UserLayer.h"

#define CONTROL_INPUT_FLAG_CONTROL              0x0001
#define CONTROL_INPUT_FLAG_INPUT                0x0002
#define CONTROL_INPUT_FLAG_OUTPUT               0x0004

#define CONTROL_INPUT_FLAG_UNFILL_SEQUENCE      0x0008
#define CONTROL_INPUT_FLAG_DRIVE_REPLY          0x0010
#define CONTROL_INPUT_FLAG_IMMEDIATELY_REPLY    0x0020      //默认立即输入
#define CONTROL_INPUT_FLAG_THREAD_CORRELATE     0x0040      //由关联的Socket处理
#define CONTROL_INPUT_FLAG_HOST                 0x0080      //本机处理

#define CONTROL_MSG_VERSION                     0x88

extern AddressLayerUtil    address_util;
extern DisplayLayerUtil    display_util;
extern ExecutorLayerUtil   executor_util;
extern MemoryLayerUtil     memory_util;
extern TimerLayerUtil      timer_util;
extern TransmitLayerUtil   transmit_util;
extern UserLayerUtil       user_util;

/*
 * 控制层启动模式
 */
enum ControlLaunchMode {
    CONTROL_LAUNCH_IMMEDIATE = 0,       //构造初始化并启动
    CONTROL_LAUNCH_USE_START,           //构造初始化,用户启动
    CONTROL_LAUNCH_START_INIT           //用户启动时初始化
};

/*
 * 控制层状态机
 */
enum ControlStatus {
    CONTROL_STATUS_NONE = 0,            //无效状态
    CONTROL_STATUS_INIT,                //初始化状态
    CONTROL_STATUS_LAUNCH,              //启动状态
    CONTROL_STATUS_RUNNING,             //运行状态
    CONTROL_STATUS_STOP,                //停止状态
    CONTROL_STATUS_SIZE
};

class BasicControl;

using RequestFunc = void(BasicControl::*)(MsgHdr*);

struct RequestInfo{
    RequestFunc func;
};

inline void setRequestFunc(RequestInfo **info, ControlStatus status, RequestInfo *set_info, RequestFunc func) {
    (info[status] = set_info)->func = func;
}

inline void resetRequestFunc(RequestInfo **info, ControlStatus status) {
    info[status]->func = nullptr;
}

/*
 * 信息头
 */
struct MsgHdr {
    char version;           //版本
    char master_type;       //主类型
    short shared_type;      //共享类型
    short response_type;    //响应类型
    short address_number;   //地址数量（用于中心化主机）
    uint32_t len_source;    //长度资源
    uint32_t serial_number; //序号
    PasswordKey msg_key;    //密钥
    char buffer[0];         //内存信息
};

/*
 * 远程端视音频资源信息（单个）
 */
struct NoteMediaInfo {
    short media_len;                    //视音频资源长度
    short media_offset;                 //视音频资源偏移量
    TransmitNoteStatus note_status;     //远程端状态
    MediaData *media_data;              //视音频资源
};

/*
 * 远程端视音频资源信息（单个）
 */
struct NoteMediaInfo2 {
    explicit NoteMediaInfo2(short len, char *buffer, const std::function<void(char*, int)> &func)
                                                            : media_len(len), media_buffer(buffer), release_func(func){}
    ~NoteMediaInfo2() = default;

    short media_len;                                //视音频媒体资源长度
    char *media_buffer;                             //视音频媒体资源内存
    std::function<void(char*, int)> release_func;   //视音频媒体资源释放函数
};

/*
 * 时间层提交资源信息
 */
struct TimeSubmitInfo{
    explicit TimeSubmitInfo(uint32_t tiemout, uint32_t sequence, uint32_t size, TimeInfo *info,
                            const std::function<void(TimeLayer*, TimeInfo*)> &func)
            : submit_timeout(tiemout), submit_sequence(sequence),
              submit_info_size(size), submit_info_array(info), submit_func(func) {}
    ~TimeSubmitInfo() = default;

    uint32_t submit_timeout;                                //超时时间
    uint32_t submit_sequence;                               //序号
    uint32_t submit_info_size;                              //资源数量
    TimeInfo *submit_info_array;                            //存储资源
    std::function<void(TimeLayer*, TimeInfo*)> submit_func; //资源释放函数
};

/*
 * 传输层输入工具（特化类型输入）
 */
class TransmitInputInfo final {
public:
    explicit TransmitInputInfo(BasicControl *control) : core_control(control) {}
    ~TransmitInputInfo() = default;

    template <typename MasterType> void onControlInputTransmitMaster(MasterType*);
    template <typename SharedType> void onControlInputTransmitShared(SharedType*);
    template <typename ResponseType> void onControlInputTransmitResponse(ResponseType*);
private:
    BasicControl *core_control;     //控制层
};

/*
 * 控制层传输工具
 */
class ControlTransmitUtil final {
public:
    static TransmitNoteStatus onTransmitNoteStatus(MeetingAddressNote*);
    static void onTransmitSortMedia(MediaData**, MediaData*, uint32_t, uint32_t);
    static uint32_t onTransmitSearchMedia(TransmitCodeInfo*, MeetingAddressNote*);
    static uint32_t onTransmitSearch0(MediaData*, MeetingAddressNote*, uint32_t);
    static bool onTransmitNoteCompare(MeetingAddressNote *cnote1, MeetingAddressNote *cnote2) {
        return (MeetingAddressManager::getNoteGlobalPos(cnote1) < MeetingAddressManager::getNoteGlobalPos(cnote2));
    }
};

class BasicControl {
    friend class TransmitInputInfo;
    friend struct RequestInfo;
public:
    explicit BasicControl(ControlLaunchMode);
    virtual ~BasicControl();

    void launchControl(MsgHdr*) throw(std::runtime_error);
    void stopControl(MsgHdr*);

    virtual bool onCoreControl() const = 0;
    virtual void onLayerCommunication(MsgHdr*, LayerType) = 0;
    virtual void onInput(MsgHdr*, MeetingAddressNote*, const AddressLayer&, int) = 0;
    virtual void onInput(MsgHdr*, MeetingAddressNote*, const ExecutorLayer&, int) = 0;
    virtual void onInput(MsgHdr*, MeetingAddressNote*, const MemoryLayer&, int) = 0;
    virtual void onInput(MsgHdr*, MeetingAddressNote*, const TimeLayer&, int) = 0;
    virtual void onInput(MsgHdr*, MeetingAddressNote*, const TransmitLayer&, int) = 0;
    virtual void onInput(MsgHdr*, MeetingAddressNote*, const DisplayLayer&, int) = 0;
    virtual void onInput(MsgHdr*, MeetingAddressNote*, const UserLayer&, int) = 0;
    virtual void onTransmitInput(TransmitMaster*) = 0;
    virtual void onTransmitInput(TransmitShared*) = 0;
    virtual void onTransmitInput(TransmitResponse*) = 0;
    virtual void onCommonInput(MsgHdr*, int) = 0;

    ControlStatus getControlStatus() const { return control_status.load(std::memory_order_consume); }
    void setControlStatus(ControlStatus status) { control_status.store(status, std::memory_order_release); }

    static void copyMsgHdr(MsgHdr *src, MsgHdr *dsc){
        memcpy(dsc, src, sizeof(MsgHdr));
    }

    static void copyMsgHdr(MsgHdr *msg, char *buffer, uint32_t offset, uint32_t len){
        memcpy(msg->buffer + offset, buffer, len);
    }

    static uint32_t onRequestExecutorTime(){
        return static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    }
    static void callOutputOnStackMemory(int len, const std::function<void(MsgHdr*)> &func) {
        char buffer[len]; func(reinterpret_cast<MsgHdr*>(buffer));
    }

protected:
    void onInit(MsgHdr*) throw(std::bad_alloc);
    void onLaunch(MsgHdr*);
    void onRunning(MsgHdr*);
    void onStop(MsgHdr*);
    void onLaunchComplete(std::promise<void>&);
    void launchControl0(MsgHdr*) throw(std::bad_alloc, std::runtime_error);

    virtual void requestInit() = 0;
    virtual void requestLaunch(MsgHdr*) = 0;
    virtual void requestStop(MsgHdr*) = 0;

    virtual void timeDrive() = 0;

    virtual void onTransmitMasterInput(TransmitJoin*) = 0;
    virtual void onTransmitMasterInput(TransmitLink*) = 0;
    virtual void onTransmitMasterInput(TransmitMedia*) = 0;
    virtual void onTransmitMasterInput(TransmitExit*) = 0;

    virtual void onTransmitSharedInput(TransmitSychro*) = 0;
    virtual void onTransmitSharedInput(TransmitSequence*) = 0;
    virtual void onTransmitSharedInput(TransmitTransfer*) = 0;
    virtual void onTransmitSharedInput(TransmitQuery*) = 0;
    virtual void onTransmitSharedInput(TransmitText*) = 0;
    virtual void onTransmitSharedInput(TransmitError*) = 0;

    virtual void onTransmitResponseInput(TransmitRJoin*) = 0;
    virtual void onTransmitResponseInput(TransmitRSychro*) = 0;
    virtual void onTransmitResponseInput(TransmitRTransfer*) = 0;
    virtual void onTransmitResponseInput(TransmitRStatus*) = 0;
    virtual void onTransmitResponseInput(TransmitRQuery*) = 0;

    ControlLaunchMode control_mode;     //启动模式
    TransmitInputInfo input_info;       //传输层输入工具类
private:
    //获取当前状态的下一个状态函数
    static RequestInfo* getRequestInfo(BasicControl *control, ControlStatus status) { return control->request_info[status + 1]; }

    void init();
    bool canLaunch();
    bool canStop();

    std::atomic<ControlStatus> control_status;      //控制层状态
    RequestInfo* request_info[CONTROL_STATUS_SIZE]; //控制请求函数信息
};

//--------------------------------------------------------------------------------------------------------------------//

template <typename MasterType> void TransmitInputInfo::onControlInputTransmitMaster(MasterType *transmit_master) {
    core_control->onTransmitMasterInput(transmit_master);
}

template <typename SharedType> void TransmitInputInfo::onControlInputTransmitShared(SharedType *transmit_shared) {
    core_control->onTransmitSharedInput(transmit_shared);
}

template <typename ResponseType> void TransmitInputInfo::onControlInputTransmitResponse(ResponseType *transmit_response) {
    core_control->onTransmitResponseInput(transmit_response);
}

//--------------------------------------------------------------------------------------------------------------------//

template <typename Type> class MsgHdrUtil final {
public:
    static MsgHdr* initMsgHdr(MsgHdr *msg, uint32_t size, uint32_t type, Type msg_type){
        msg->serial_number = size; msg->master_type = static_cast<char>(type);
        new (reinterpret_cast<void*>(msg->buffer)) Type(std::move(msg_type));
        return msg;
    }

    static MsgHdr* appendMsgHdr(MsgHdr *msg, uint32_t size, Type msg_type){
        new (reinterpret_cast<void*>(msg->buffer + msg->serial_number)) Type(std::move(msg_type));
        msg->serial_number += size;
        return msg;
    }
    static MsgHdr* freeMsgHdr(MsgHdr *msg, uint32_t size){
        memmove(msg->buffer, msg->buffer + size, (msg->serial_number -= size));
        return msg;
    }
};

template <typename Type> class MsgHdrUtil<const Type> final {
public:
    static MsgHdr* initMsgHdr(MsgHdr *msg, uint32_t size, uint32_t type, const Type &msg_type){
        msg->serial_number = size; msg->master_type = static_cast<char>(type);
        new (reinterpret_cast<void*>(msg->buffer)) Type(msg_type);
        return msg;
    }

    static MsgHdr* appendMsgHdr(MsgHdr *msg, uint32_t size, const Type &msg_type){
        new (reinterpret_cast<void*>(msg->buffer + msg->serial_number)) Type(msg_type);
        msg->serial_number += size;
        return msg;
    }
    static MsgHdr* freeMsgHdr(MsgHdr *msg, uint32_t size){
        memmove(msg->buffer, msg->buffer + size, (msg->serial_number -= size));
        return msg;
    }
};

template <typename Type> class MsgHdrUtil<Type*> final {
public:
    static MsgHdr* initMsgHdr(MsgHdr *msg, uint32_t size, uint32_t type, Type *msg_type){
        msg->serial_number = size; msg->master_type = static_cast<char>(type);
        memcpy(msg->buffer, reinterpret_cast<const void*>(msg_type), sizeof(Type*));
        return msg;
    }
    static MsgHdr* appendMsgHdr(MsgHdr *msg, uint32_t size, Type *msg_type){
        memcpy(msg->buffer + msg->serial_number, msg_type, sizeof(Type*));
        msg->serial_number += size;
        return msg;
    }
    static MsgHdr* freeMsgHdr(MsgHdr *msg, uint32_t size){
        memmove(msg->buffer, msg->buffer + size, (msg->serial_number -= size));
        return msg;
    }
};

template <> class MsgHdrUtil<void> final {
public:
    static MsgHdr* initMsgHdr(MsgHdr *msg, uint32_t type) {
        msg->serial_number = 0; msg->master_type = static_cast<char>(type);
        return msg;
    }
};

#endif //TEXTGDB_BASICCONTROL_H
