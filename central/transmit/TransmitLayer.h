//
// Created by abc on 21-2-13.
//

#ifndef TEXTGDB_TRANSMITLAYER_H
#define TEXTGDB_TRANSMITLAYER_H

#include "TransmitDataUtil.h"

#define TRANSMIT_LAYER_RUNTIME_PARAMETER        1
#define TRANSMIT_LAYER_HISTORY_PARAMETER        2
#define TRANSMIT_LAYER_SPECIFIC_PARAMETER       3

#define TRANSMIT_LAYER_SET_FILTER               100
#define TRANSMIT_LAYER_CLR_FILTER               101
#define TRANSMIT_LAYER_REPEAT                   102
#define TRANSMIT_LAYER_VERIFY                   103
#define TRANSMIT_LAYER_OUTPUT_INTERRCEPT        104
#define TRANSMIT_LAYER_MASTER_INTERCEPT         105
#define TRANSMIT_LAYER_SHARED_INTERCEPT         106
#define TRANSMIT_LAYER_RESPONSE_INTERCEPT       107

//生命周期：
//        程序启动 ->  构造TransmitLayer -> 初始化
//
//        程序开始 ->  STATUS_开始 ->
//                |-> 创建socket
//                |-> 关联SocketThread
//                |-> 绑定地址
//
//        程序停止 ->  STATUS_停止 -> 复位数据
//                |-> 断开SocketThread -> 复位数据
//
//        程序销毁 ->  销毁TransmitLayer
struct ControlDriveInfo;

extern std::mutex history_lock;
extern std::atomic<ThreadParameter*> parameter_link;
extern TransmitParameter history_parameter;

//传输层状态
enum StatusTransmit{
    TRANSMIT_STATUS_INIT = 0,   //初始化状态
    TRANSMIT_STATUS_LAUNCH,     //启动状态
    TRANSMIT_STATUS_TERMINATE,  //终止状态
    TRANSMIT_STATUS_FINAL       //结束状态
};

/*
 *  传输请求工具（封装）
 */
struct TransmitRequestInfo {
    sockaddr_in request_addr;   //请求地址信息
    bool (*request_func)(TransmitLayer*, TransmitThreadUtil*);  //请求地址函数
};

/*
 * 传输编码信息（用于视音频资源）
 */
struct TransmitCodeInfo {
    bool is_ignore_note;        //是否忽略远程端（该远程端是输出的地址）
    uint32_t media_len;         //视音频长度（总）
    uint32_t media_size;        //视音频数量（总）
    uint32_t transmit_sequence; //传输序号
    MediaData *media_data;      //视音频资源
    uint32_t (*search_func)(TransmitCodeInfo*, MeetingAddressNote*);    //远程端搜索序号函数
    TransmitNoteStatus (*status_func)(MeetingAddressNote*);             //远程端状态函数
    std::function<MeetingAddressNote*(uint32_t)> note_func;             //序号搜索远程端函数
};

/*
 * 传输输出信息
 */
class TransmitOutputInfo final {
public:
    TransmitOutputInfo() : msg_len(0), output_len(0), old_hdr_len(0), old_resource_len(0), output_msg(nullptr),
                           output_note(0), thread_util(nullptr) {}
    explicit TransmitOutputInfo(int mlen, int olen, int hlen, int rlen, MsgHdr *msg, MeetingAddressNote *note, TransmitThreadUtil *util)
            : msg_len(mlen), output_len(olen), old_hdr_len(hlen), old_resource_len(rlen), output_msg(msg),
              output_note(note), thread_util(util) {}
    ~TransmitOutputInfo() = default;

    bool verifyOutput() const { return (!thread_util || (output_len <= 0) || !output_msg); }
    bool verifyNote() const { return thread_util->onSocketInputVerify(output_note);}
    bool isNoteCompile() const;
    void setRemoteNote(MeetingAddressNote *note) { output_note = note; }
    void onNoteCompileOutput(TransmitAnalysisUtil*, MsgHdr*);
    void onNoteCompileOutput();
    void onNoteCompileReset();

    void onTransmitOutput(const std::function<void(TransmitThreadUtil*, MeetingAddressNote*, MsgHdr*, int)>&);
    bool onTransmitIntercept(TransmitTypeIntercept *intercept_info, bool(*intercept_func)(TransmitTypeIntercept*, MsgHdr*, MeetingAddressNote*)){
        return intercept_func(intercept_info, output_msg, output_note);
    }
private:
    int msg_len;                        //输出信息长度（未添加共享类型）
    int output_len;                     //输出信息长度（已添加共享类型）
    int old_hdr_len;                    //输出长度信息（旧）
    int old_resource_len;               //输出资源信息（旧）
    MsgHdr *output_msg;                 //输出信息
    MeetingAddressNote *output_note;    //输出远程的
    TransmitThreadUtil *thread_util;    //输出关联传输线程
};

/*
 * 传输初始化信息
 */
struct TransmitInitInfo {
    const sockaddr_in init_address;     //初始化地址信息
};

/*
 * 传输层状态机（包含过滤信息）
 */
class TransmitStatus : public LayerFilter {
public:
    TransmitStatus() : LayerFilter(), status_(TRANSMIT_STATUS_INIT) {}
    ~TransmitStatus() override { setStatus(TRANSMIT_STATUS_FINAL); }

    void setStatus(StatusTransmit status) { status_ = status; }
    bool onStatus(StatusTransmit status) const { return (status_ == status);}
private:
    StatusTransmit status_;     //传输层状态
};

class TransmitErrorUtil final {
public:
    static void onParameterError(TransmitParameter*, int);
    static void onParameterError(TransmitParameter*, ParameterError*);
};

class TransmitLayerUtil : public LayerUtil{
public:
    BasicLayer* createLayer(BasicControl*) noexcept override;
    void destroyLayer(BasicControl*, BasicLayer*) noexcept override;
};

class TransmitLayer : public BasicLayer{
    friend class TransmitOutputInfo;
    friend class TransmitThreadUtil;
    friend class TransmitTypeIntercept;
public:
    explicit TransmitLayer(BasicControl*);
    ~TransmitLayer() override = default;

    void initLayer() override;
    void onInput(MsgHdr*) override;
    void onOutput() override;
    bool isDrive() const override { return false; }
    void onDrive(MsgHdr*) override;
    void onParameter(MsgHdr*) override;
    void onControl(MsgHdr*) override;
    uint32_t onStartLayerLen() const override { return sizeof(TransmitInitInfo); }
    LayerType onLayerType() const override { return LAYER_TRANSMIT_TYPE; }

    void onOutputLayer(TransmitOutputInfo*);
    void onInputLayer(TransmitParameter*, TransmitThreadUtil*, TransmitInputData*);

    void onNoteTermination(MeetingAddressNote*);
    sockaddr_in onNoteAddress(MeetingAddressNote *note) { return MeetingAddressManager::getSockAddrFromNote(note); }

    static int onRemoteNoteCompileLen(TransmitThreadUtil *thread_util, MeetingAddressNote *remote_note){
        TransmitCompileUtil *compile_util = thread_util->getTransmitCompileUtil(MeetingAddressManager::getNotePermitPos(remote_note));
        return (compile_util->onCompileSharedSize() + compile_util->onCompileResourceLen());
    }
    static int onNoteCompileOutput(TransmitParameter*, TransmitAnalysisUtil*, MeetingAddressNote*, MsgHdr*, MsgHdr*, int);
    static int onNoteCompileOutput(TransmitParameter*, TransmitCompileUtil*, MeetingAddressNote*, MsgHdr*, int);
    static void onNoteCompileType(TransmitThreadUtil*, MeetingAddressNote*, MsgHdr*);
private:
    static void initInterceptMsg(MsgHdr*, MsgHdr*);
    static void onError(TransmitParameter *parameter, int type) { TransmitErrorUtil::onParameterError(parameter, type); }
    static void onError(TransmitParameter *parameter, ParameterError *error) { TransmitErrorUtil::onParameterError(parameter, error); }
    static void onAnalysisInput(TransmitParameter*, TransmitThreadUtil*, TransmitInputData*, TransmitTypeIntercept*,
                                const std::function<MeetingAddressNote*(const sockaddr_in&)>&,
                                const std::function<void(TransmitInfo*)>&);
    static void onAnalysisType(TransmitParameter* ,TransmitInfo*, TransmitTypeIntercept*, short,
                               const std::function<TransmitUtil*(short)>&,
                               const std::function<TransmitType*(TransmitParameter*, TransmitInfo*, TransmitUtil*)>&,
                               const std::function<void(TransmitType*)>&) throw(AnalysisError);
    static void onCompileOutput0(TransmitParameter*, TransmitInfo*, TransmitUtil*);
    static bool onTransmitCreateBuffer(TransmitParameter*, TransmitLayer*, int, const std::function<void(InitLocator&)> &, const std::function<void(char*)> &);
    static void onTransmitDestroyBuffer(TransmitParameter*, TransmitLayer*, const char*, int);
    static void onOutputDone(TransmitParameter*, MsgHdr*);
    static void onOutputCancel(MsgHdr*);
    static BasicControl* onTransmitLayerControl(TransmitLayer *layer) { return layer->basic_control; }

    bool requestControlThread(TransmitThreadUtil *transmit_thread) const { return transmit_thread->isRunCorrelateThread(); }
    MeetingAddressNote* requestControlInput(TransmitParameter*, sockaddr_in);
    void onReceiveInput(TransmitParameter*, TransmitAnalysisUtil*, TransmitInfo*);
    void onAnalysisMater(TransmitParameter*, TransmitInfo*, TransmitUtil*, const std::function<void(TransmitMaster*)>&) throw(AnalysisError);
    TransmitShared* onAnalysisShared(TransmitParameter*, TransmitInfo*, TransmitUtil*) throw(ParameterError, AnalysisError);
    TransmitResponse* onAnalysisResponse(TransmitParameter*, TransmitInfo*, TransmitUtil*) throw(ParameterError, AnalysisError);

    bool onLaunchLayer(TransmitInitInfo*);
    void onStopLayer();

    void onTransmitRepeat(uint32_t);
    void setTransmitFilter(FilterInfo);
    void clrTransmitFilter(FilterInfo);

    void onTransmitUtilInit(MsgHdr*);
    void onTransmitUtilUnInit(MsgHdr*);
    void onTransmitUtilCorrelate(MsgHdr*);

    void onParameterRuntime(ParameterTransmit*);
    void onParameterHistory(ParameterTransmit*);
    void onParameterSpecific(ParameterTransmit*);

    volatile uint32_t transmit_repeat_number;       //输出失败时,重复输出的次数,超过次数判定失败
    TransmitThreadUtil *wait_correlate_thread;      //Manager线程创建Socket线程前,会向控制层创建TransmitThreadUtil并赋值给该变量,Socket线程启动时会关联该变量和传输层
    TransmitStatus transmit_status;                 //传输层状态（包含过滤信息）
    sockaddr_in transmit_addr;                      //传输地址（由用户选择或默认）
    TransmitTypeIntercept transmit_intercept_info;  //传输层拦截信息

};

#endif //TEXTGDB_TRANSMITLAYER_H
