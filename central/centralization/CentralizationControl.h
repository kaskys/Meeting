//
// Created by abc on 20-12-8.
//

#ifndef TEXTGDB_CENTRALIZATIONCONTROL_H
#define TEXTGDB_CENTRALIZATIONCONTROL_H

#include "../BasicControl.h"

#define CONTROL_ADDRESS_PREVENT_SECOND_TIME         5

//TimeLayer -> StatusInfo;
//AddressLayer -> std::max(AddressInitInfo(本地地址), std::function<void(MsgHdr*, MeetingAddressNote*)>)
//TransmitLayer -> TransmitInitInfo;

class LayerControlUtil;
class CentralizationControl;

using LayerFunc = void(*)(LayerControlUtil*, MsgHdr*);
using FlagsFunc = void(*)(CentralizationControl*, MsgHdr*, MeetingAddressNote*);
using DisplaySubmitFunc = void(*)(DisplayLayer*, NoteMediaInfo*, uint32_t);

/*
 * 控制器驱动信息
 */
struct ControlDriveInfo {
    ControlDriveInfo() = default;
    explicit ControlDriveInfo(int len, int flag, MsgHdr *msg, RemoteNoteInfo *info) : output_len(len), output_flag(flag),
                                                                                      output_msg(msg), note_info(info) {}
    ~ControlDriveInfo() = default;

    int output_len;             //驱动输出长度
    int output_flag;            //驱动输出标志
    MsgHdr *output_msg;         //驱动输出信息
    RemoteNoteInfo *note_info;  //驱动输出远程端
};

/*
 * 层调用工具
 */
class LayerControlUtil final {
public:
    BasicLayer* onLayer() const  { return layer; }
    void onInitFuncUtil(BasicLayer *layer) { this->layer = layer; }

    void callLayerFuncOnInput(MsgHdr *msg) { if(layer) { layer->onInput(msg); } }
    void callLayerFuncOnControl(MsgHdr *msg) { if(layer) { layer->onControl(msg); } }
private:
    BasicLayer *layer;  //基层
};

/**
 * 控制层调用函数工具
 */
class ControlFuncUtil final {
public:
    void onInitFuncUtil(CentralizationControl *control) { this->control = control; }

    void callControlFuncOnInput(MsgHdr*, MeetingAddressNote*, int);
    void callControlFuncOnControl(MsgHdr*, MeetingAddressNote*, int);
private:
    CentralizationControl *control;     //控制层
};

class CentralizationControl : public BasicControl{
    friend class CentralizetionTransmitUtil;
    friend class ControlFuncUtil;
public:
    CentralizationControl(ControlLaunchMode) throw(std::bad_alloc, std::runtime_error);
    CentralizationControl(MsgHdr*, ControlLaunchMode) throw(std::bad_alloc, std::runtime_error);
    ~CentralizationControl() override;

    void onLayerCommunication(MsgHdr *msg, LayerType type) override { layer_array[type].onLayer()->onControl(msg); }
    void onInput(MsgHdr*, MeetingAddressNote*, const AddressLayer&, int) override;
    void onInput(MsgHdr*, MeetingAddressNote*, const ExecutorLayer&, int) override;
    void onInput(MsgHdr*, MeetingAddressNote*, const MemoryLayer&, int) override;
    void onInput(MsgHdr*, MeetingAddressNote*, const TimeLayer&, int) override;
    void onInput(MsgHdr*, MeetingAddressNote*, const TransmitLayer&, int) override;
    void onInput(MsgHdr*, MeetingAddressNote*, const DisplayLayer&, int) override;
    void onInput(MsgHdr*, MeetingAddressNote*, const UserLayer&, int) override;
    void onTransmitInput(TransmitMaster*) override;
    void onTransmitInput(TransmitShared*) override;
    void onTransmitInput(TransmitResponse*) override;
    void onCommonInput(MsgHdr*, int) override;
protected:
    template <typename T> static T* getControlLayer(LayerControlUtil *layer_util, LayerType type) { return dynamic_cast<T*>(layer_util[type].onLayer()); }
    static LayerControlUtil* getControlLayerUtil(LayerControlUtil *layer_util, LayerType type) { return (layer_util + type); }
    static void callControlLayerFunc(LayerControlUtil *layer_util, LayerFunc func, MsgHdr *func_msg, short master) throw(std::logic_error) {
        func_msg->master_type = master;
        (*func)(layer_util, func_msg);

        if(func_msg->serial_number <= 0){
            throw std::logic_error(nullptr);
        }
    };
    static  void callControlLayerFuncNotMsg(LayerControlUtil *layer_util, LayerFunc func, const std::function<void(MsgHdr*)> &callback, int len) throw(std::logic_error) {
        char buffer[sizeof(MsgHdr) + len];
        MsgHdr *func_msg = reinterpret_cast<MsgHdr*>(buffer);

        callback(func_msg);
        callControlLayerFunc(layer_util, func, func_msg, func_msg->master_type);
    };
    static void callControlLayerFuncNotMsg(LayerControlUtil *layer, LayerFunc func, const std::function<void(MsgHdr*)> &callback, const std::function<void(MsgHdr*)> &funcback, int len){
        char buffer[sizeof(MsgHdr) + len];
        MsgHdr *func_msg = reinterpret_cast<MsgHdr*>(buffer);

        callback(func_msg);
        callControlLayerFunc(layer, func, func_msg, func_msg->master_type);
        funcback(func_msg);
    };

    static void onRemoteInput(TransmitType*, const std::function<void(RemoteNoteInfo*, TransmitThreadUtil*)>&);
    static void onRemoteOutput(TransmitLayer*, RemoteNoteInfo*, TransmitThreadUtil*, MsgHdr*, uint32_t, short, short, char);
    static void onRemoteOutput(TransmitLayer*, RemoteNoteInfo*, TransmitThreadUtil*, uint32_t, short, short, char,
                               const std::function<void(MsgHdr*)>&);
    void requestInit() override;
    void requestLaunch(MsgHdr*) override;
    void requestStop(MsgHdr*) override;
    void timeDrive() override;

    virtual void onDriveOutput(MsgHdr*, AddressNoteInfo*, int);
    virtual void onDriveOutput0(MsgHdr*);
    virtual void onImmediatelyOutput(MsgHdr*, AddressNoteInfo*, int);
    virtual void onThreadOutput(const ControlDriveInfo&, uint32_t);
    virtual void onCentralizationTimeDrive(MsgHdr*) = 0;

    virtual void onLaunchCentralization(MsgHdr*) = 0;
    virtual void onStopCentralization(MsgHdr*) = 0;
    virtual void onAllocDynamic(MsgHdr*) = 0;
    virtual void onDeallocDynamic(MsgHdr*) = 0;
    virtual RemoteNoteInfo* onMatchInputAddress(sockaddr_in) = 0;

    volatile uint32_t control_timer_sequence;       //控制层时间序号
    TransmitThreadUtil *main_thread_util;           //主传输线程工具类
    LayerControlUtil layer_array[LAYER_TYPE_SIZE];  //层调用工具类（执行层、内存层、地址层、时间层、传输层、显示层、用户层、配置信息）
private:
    static void onThreadOutput0(TransmitLayer*, TransmitThreadUtil*, const ControlDriveInfo&);

    void onLayerInit();
    void onFuncInit();
    void onCentralizationInit(MsgHdr *init_msg = nullptr) throw(std::bad_alloc, std::runtime_error);

    void onInitControl() throw(std::bad_alloc);
    void onLaunchControl(MsgHdr*) throw(std::runtime_error);
    void onStopControl(MsgHdr*);

    void createLayer() throw(std::bad_alloc);
    void launchLayer(BasicLayer*, MsgHdr*) throw(std::bad_alloc, std::runtime_error);
    void stopLayer(BasicLayer*, MsgHdr*);

    void destroyLayer();
    void destroyLayer(BasicLayer*);

    static void onAnalysisMsg(MsgHdr*, const std::function<void(MsgHdr*, int)>&);
    static TransmitInfo initAnalysisInfo(MsgHdr*);

    void onInput0(MsgHdr*, MeetingAddressNote*, const BasicLayer&, int);

    //----------------------------------------------------------------------------------------------------------------//
    void onNoteInit(MsgHdr*, MeetingAddressNote*);
    void onNoteUninit(MsgHdr*, MeetingAddressNote*);

    //远程的初始化失败,将该远端端设置黑名单(防止重复加入),并定时某个时间(超时复位黑名单),向该远程端发送加入失败响应(包含黑名单超时时间)
    virtual void onNotePrevent(MsgHdr*, MeetingAddressNote*) = 0;
    virtual void onNoteDisplay(MsgHdr*, MeetingAddressNote*) = 0;
    virtual void onNoteLink(MsgHdr*, MeetingAddressNote*) = 0;
    virtual void onNoteMedia(MsgHdr*, MeetingAddressNote*) = 0;
    virtual void onNotePassive(MsgHdr*, MeetingAddressNote*) = 0;
    virtual void onNoteExit(MsgHdr*, MeetingAddressNote*) = 0;

    //----------------------------------------------------------------------------------------------------------------//
    void onLayerError(MsgHdr*, MeetingAddressNote*);
    void onRequestTimer(MsgHdr*, MeetingAddressNote*);
    void onAllocFixed(MsgHdr*, MeetingAddressNote*);
    void onDeallocFixed(MsgHdr*, MeetingAddressNote*);
    void onNoteBlack(MsgHdr*, MeetingAddressNote*);
    void onNoteUpdate(MsgHdr*, MeetingAddressNote*);
    void onExecutorCreateThread(MsgHdr*, MeetingAddressNote*);
    void onExecutorDestroyThread(MsgHdr*, MeetingAddressNote*);
    void onTransmitCorrelateThread(MsgHdr*, MeetingAddressNote*);
    void onTransmitMainThread(MsgHdr*, MeetingAddressNote*);
    void onDisplayFrame(MsgHdr*, MeetingAddressNote*);
    void onControlTermination(MsgHdr*, MeetingAddressNote*);
    virtual void onSequenceGap(MsgHdr*, MeetingAddressNote*) = 0;
    virtual void onSequenceFrame(MsgHdr*) = 0;
    //----------------------------------------------------------------------------------------------------------------//
    void onGenerateMediaMsg(MsgHdr*, NoteMediaInfo2*, TransmitMemoryFixedUtil*, uint32_t);

    ControlFuncUtil func_util;                  //函数调用工具
    UnLockQueue<ControlDriveInfo> drive_queue;  //驱动队列（无锁）
};


#endif //TEXTGDB_CENTRALIZATIONCONTROL_H
