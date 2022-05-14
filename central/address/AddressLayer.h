//
// Created by abc on 20-12-8.
//

#ifndef TEXTGDB_ADDRESSLAYER_H
#define TEXTGDB_ADDRESSLAYER_H

#include "util/AddressNoteUtil.h"

#define TRANSMIT_LAYER_ADDRESS_PARAMETER            1
#define TRANSMIT_LAYER_ALL_REMOTE_PARAMETER         2
#define TRANSMIT_LAYER_SINGLE_REMOTE_PARAMETER      3
#define TRANSMIT_LAYER_SINGLE_REMOTE_LEN_PARAMETER  4
#define TRANSMIT_LAYER_HOST_PARAMETER               5

#define ADDRESS_LAYER_IS_BLACK                      10
#define ADDRESS_LAYER_SET_BLACK                     11
#define ADDRESS_LAYER_CLR_BLACK                     12
#define ADDRESS_LAYER_SET_FILTER                    13
#define ADDRESS_LAYER_JOIN_TIME                     14
#define ADDRESS_LAYER_LINK_TIME                     15
#define ADDRESS_LAYER_SEQUENCE_TIME                 16
#define ADDRESS_LAYER_PASSIVE_TIME                  17
#define ADDRESS_LAYER_TIME_SIZE                     18

//延迟时间由控制处理
//AddressNoteInfo需要存储远程端的加入请求的标志及生成加入请求的标志,判断请求加入的标志和远程端的上一次标志是否一致
//判断MsgHdr（除加入）的sequence与RemoteNoteInfo的活动sequence是否一致
//MsgHdr的serial_number变量一定要是sequence,不能是长度(除Init)
//远程端向主机发送加入请求要附加密钥(主机没有AddressNoteInfo时不需要验证密钥,验证由响应的远程端处理)(有AddressNoteInfo时需要验证的密钥和请求的密钥是否一致，即同一次请求)
//主机回复远程端加入响应也要附加密钥(远程端也需要验证主机加入响应的密钥与当前请求加入的密钥是否一致
//                                    ：可能远程端非正常退出再重新加入时,主机已经删除了AddressNoteInfo,而上一次的加入请求延迟到达主机,从而使主机忽略这次加入请求,
//                                      这时远程端需要在加入请求中附加一个错误信息给主机告诉正确的密钥)

/*
 * 传输层需要存在共享类型的同步时,忽略主类型的链接处理
 * 控制层处理STATUS_EXIT状态和没有地址时的区别处理
 * 传输层在处理除加入外的Msg时,先判断sequence,后解析类型
 * 控制层需要处理port（端口号）不一致的情况及黑名单情况
 */
//以下问题
//    1：关于执行（加入、链接、同步、序号、超时）函数是由地址层构造还是控制层构造 -> 有地址层构造内存及能够提供的参数(X)
//    2：关于销毁NoteInfo后还含有该指针如何处理(X)
//    3：注意：：：主机对远程端的转移如何处理 => 先处理normal后处理transfer(X)
//    4：修改MeetingAddressManager的遍历、host_info、core_info(X)
//    5：关于MeetingAddressManager的push（主SocketThread线程）、remove（不需要处理）和match（主SocketThread线程）、ergioc（主SocketThread线程）的多线程问题
//        (1)：每个SocketThread都存储AddressNoteInfo信息数组（大小排序）,接收MsgHdr除join外都查询AddressNoteInfo信息数组,查询不到或join交给主SocketThread线程处理
//        (2)：主SocketThread减少接收AddressNote的数量,当音视频信息需要发送时,时间层执行驱动函数,向控制层发送的音视频信息,控制层再将音视频信息调用给线程层驱动函数,该函数向所有SocketThread发送任务

//所有的类型都需要匹配PasswordKey(除了Query、Error)
//    主socket   -> 主要对没有或退出状态的AddressNote结构进行加入处理(LAYER_MASTER_INIT、LAYER_MASTER_REPEAT),处理完后忽略非本socket的输入处理
//    许可socket ->  对所有的输入（除上面）都需要进行验证（除Query、Error），由控制层代理处理输入的合法性（调用onNoteVerify函数）
//控制层对某个AddressNote输出后，需要调用地址层的onOutput函数

using StatusFunc = std::function<void(AddressStatusUtil*, MsgHdr*, AddressLayer*, AddressNoteInfo*)>;

/*
 * 远程端链接信息
 */
struct AddressLinkInfo{
    uint32_t link_rtt;              //客户机与服务器的传输时间（平均？单次？）
    AddressNoteInfo *note_info;     //客户机
};

/*
 * 远程端初始化信息
 */
struct AddressInitInfo{
    uint32_t jtime;                 //加入触发时间
    uint32_t ptime;                 //被动触发时间
    uint32_t ltime;                 //链接触发时间
    uint32_t stime;                 //序号触发时间
    uint32_t time_size;             //触发次数（超时时间 = 触发时间 x 触发次数）
    uint32_t init_indicator_time;   //初始化指示时间（弃用）
    sockaddr_in init_addr;          //远程端地址
};

class AddressLayerUtil : public LayerUtil {
public:
    BasicLayer* createLayer(BasicControl*) noexcept override;
    void destroyLayer(BasicControl*, BasicLayer*) noexcept override;
};

class AddressLayer : public BasicLayer {
    friend class AddressStatusUtil;
    friend class HostStatusUtil;
    friend class RemoteStatusUtil;
    friend class JoinStatusUtil;
    friend class LinkStatusUtil;
    friend class SequenceStatusUtil;
    friend class NormalStatusUtil;
    friend class TransferStatusUtil;
    friend class PassiveStatusUtil;
    friend class ExitStatusUtil;
public:
    explicit AddressLayer(BasicControl*);
    ~AddressLayer() override;

    void initLayer() override;
    void onInput(MsgHdr*) override;
    void onOutput() override;
    bool isDrive() const override { return false; }
    void onDrive(MsgHdr*) override;
    void onParameter(MsgHdr*) override;
    void onControl(MsgHdr*) override;
    uint32_t onStartLayerLen() const override { return sizeof(AddressInitInfo); }
    LayerType onLayerType() const override { return LAYER_ADDRESS_TYPE; }
    MeetingAddressNote* onHostNote() const { return dynamic_cast<MeetingAddressNote*>(host_note); }

    void onInputLayer(MsgHdr*, AddressNoteInfo*, const StatusFunc&);
    void onOutputLayer(MsgHdr*, AddressNoteInfo*);

    virtual bool onNoteVerify(MsgHdr*, MeetingAddressNote*) = 0;
    virtual void onRunNote(MeetingAddressNote*) = 0;
    virtual void unRunNote(MeetingAddressNote*) = 0;
    virtual uint32_t onRunNoteSize() const = 0;
    virtual void onRunNoteInfo(const std::function<void(MeetingAddressNote*, uint32_t)>&) = 0;

    bool onFilterAddress(AddressNoteInfo*, FilterInfo);
    AddressNoteInfo* onMatchAddress(const sockaddr_in&);

    void onNoteInputInit(MsgHdr*, AddressNoteInfo*);
    void onNoteInputUninit(MsgHdr*, AddressNoteInfo*);
    void onNoteInputJoin(MsgHdr*, AddressNoteInfo*);
    void onNoteInputLink(MsgHdr*, AddressNoteInfo*);
    void onNoteInputSynchro(MsgHdr*, AddressNoteInfo*);
    void onNoteInputSequence(MsgHdr*, AddressNoteInfo*);
    void onNoteInputNormal(MsgHdr*, AddressNoteInfo*);
    void onNoteInputTransfer(MsgHdr*, AddressNoteInfo*);
    void onNoteInputPassive(MsgHdr*, AddressNoteInfo*);
    void onNoteInputExit(MsgHdr*, AddressNoteInfo*);
    void onNoteInvalid(AddressNoteInfo*);
    void onNoteReuse(MsgHdr*, AddressNoteInfo*);
    void onNoteUnuse(MsgHdr*, AddressNoteInfo*);

    static StatusFunc getInputFunc(void(AddressStatusUtil::*func)(MsgHdr*, AddressLayer*, AddressNoteInfo*)){
        return [=](AddressStatusUtil *status_util, MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *note_info) -> void {
            (status_util[note_info->getStatusNote()].*func)(msg, layer, note_info);
        };
    }
//    static StatusFunc getInputJoinFunc() {
//        return [=](AddressStatusUtil *status_util, MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *note_info) -> void {
//            status_util[note_info->getStatusNote()].onInputJoin(msg, layer, note_info);
//        };
//    }
//    static StatusFunc getInputLinkFunc() {
//        return [=](AddressStatusUtil *status_util, MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *note_info) -> void {
//            status_util[note_info->getStatusNote()].onInputLink(msg, layer, note_info);
//        };
//    }
//    static StatusFunc getInputSynchroFunc(){
//        return [=](AddressStatusUtil *status_util, MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *note_info) -> void {
//            status_util[note_info->getStatusNote()].onInputSynchro(msg, layer, note_info);
//        };
//    }
//    static StatusFunc getInputNormalFunc() {
//        return [=](AddressStatusUtil *status_util, MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *note_info) -> void {
//            status_util[note_info->getStatusNote()].onInputNormal(msg, layer, note_info);
//        };
//    }
//    static StatusFunc getInputPassiveFunc() {
//        return [=](AddressStatusUtil *status_util, MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *note_info) -> void {
//            status_util[note_info->getStatusNote()].onInputPassive(msg, layer, note_info);
//        };
//    }
//    static StatusFunc getInputExitFunc() {
//        return [=](AddressStatusUtil *status_util, MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *note_info) -> void {
//            status_util[note_info->getStatusNote()].onInputExit(msg, layer, note_info);
//        };
//    }
    void initMoreTimerInfo(int, uint32_t, uint32_t , const std::function<void()>&, const std::function<void()>&, const std::function<void(MsgHdr*)>&);
    void initImmediatelyTimerInfo(int, uint32_t, const std::function<void()>&, const std::function<void(MsgHdr*)>&);

    static bool isRemoteNoteTimeout(MeetingAddressNote*, uint32_t, uint32_t);
    static TransmitNoteStatus onNoteTransmitStatus(MeetingAddressNote*);
protected:
    virtual std::function<void(AddressNoteInfo*)> onNoteTermination() = 0;
    virtual void onHostNoteInit(MsgHdr*, HostNoteInfo*);
    virtual void onRemoteNoteInit(MsgHdr*, RemoteNoteInfo*);
    virtual void onHostNoteUninit(MsgHdr*, HostNoteInfo*);
    virtual void onRemoteNoteUninit(MsgHdr*, RemoteNoteInfo*);

    void onReplyError(AddressNoteInfo*, ErrorType, StatusNote);
    void onReplyError(sockaddr_in, ErrorType, StatusNote);
    void onLinkCallback(RemoteNoteInfo*);

    virtual void onNoteInputJoin0(MsgHdr*, AddressNoteInfo*);
    virtual void onNoteInputExit0(MsgHdr*, AddressNoteInfo*);
    virtual void onNoteInputLink0(MsgHdr*, AddressNoteInfo*);
    virtual void onNoteInputSynchro0(MsgHdr*, AddressNoteInfo*);
    virtual void onNoteInputSequence0(MsgHdr*, AddressNoteInfo*);
    virtual void onNoteInputNormal0(MsgHdr*, AddressNoteInfo*);
    virtual void onNoteInputPassive0(MsgHdr*, AddressNoteInfo*);

    volatile uint32_t join_time;                //加入触发时间
    volatile uint32_t time_size;                //触发时间次数
    volatile uint32_t passive_time;             //被动触发时间
    MeetingAddressManager *address_manager;     //地址管理器
private:
    static MeetingAddressNote* onNoteCreateFunc(AddressLayer*, uint32_t);
    static MeetingAddressNote* onFixedCreateFunc(AddressLayer*);
    static void onNoteRemoveFunc(AddressLayer*, MeetingAddressNote*, MeetingAddressNote*);

    bool onCreateManager();
    void onDestroyManager();

    bool onStart(MsgHdr*);
    void onStop();
    void onNoteCreate(MsgHdr*, AddressNoteInfo*, AddressStatusUtil*);
    void onNoteInit(MsgHdr*, AddressStatusUtil*);
    void onNoteUninit(MsgHdr*, AddressNoteInfo*, AddressStatusUtil*);
    void onNoteDestroy(MeetingAddressNote*);
    void onNoteStatusUpdate(MsgHdr*, AddressNoteInfo*, int);
    void onNotePush(AddressNoteInfo*) throw(std::bad_alloc);

    virtual bool isNoteRepeatJoin(MsgHdr*, AddressNoteInfo*) const = 0;
    virtual bool isNoteRepeatLink(MsgHdr*, AddressNoteInfo*) const = 0;
    virtual void onAddressTime(uint32_t, uint32_t) = 0;

    void onAddressFilter(NoteFilterInfo);
    void onAddressBlack(RemoteNoteInfo*, bool);

    void onParameterAddress(ParameterAddress*);
    void onParameterNote(ParameterRemote*);
    void onParameterNote(ParameterRemote*, RemoteNoteInfo*);
    void onParameterNote(ParameterHost*);

    uint32_t max_indicator_time;                        //最大指示时间（弃用）
    AddressStatusUtil *status_util[NOTE_STATUS_SIZE];   //远程端状态工具类（远程、本机、加入、链接、序号、正常、被动、退出）
    HostNoteInfo *host_note;                            //本机端
    AddressParameter layer_parameter;                   //地址信息参数类
};


class MasterAddressLayer : public AddressLayer {
public:
    using AddressLayer::AddressLayer;
    ~MasterAddressLayer() override = default;

    static void onNoteJoinExecutor(AddressLayer*, BasicControl*, AddressNoteInfo*);
    static void onNoteLinkExecutor(AddressLayer*, BasicControl*, AddressNoteInfo*);
    static void onNoteSequenceExecutor(AddressLayer*, BasicControl*, AddressNoteInfo*, uint32_t, uint32_t);
    static void onNoteTimeout(AddressLayer*, BasicControl*, AddressNoteInfo*);

    bool onNoteVerify(MsgHdr*, MeetingAddressNote*) override;
    void onRunNote(MeetingAddressNote*) override;
    void unRunNote(MeetingAddressNote*) override;
    uint32_t onRunNoteSize() const override;
    void onRunNoteInfo(const std::function<void(MeetingAddressNote*, uint32_t)>&) override;

    static void onNoteImmediateSynchro(MsgHdr*);
protected:
    std::function<void(AddressNoteInfo*)> onNoteTermination() override;

//    virtual void onHostNoteInit(MsgHdr*, HostNoteInfo*);      //不需要实现,由基类（AddressLayer）实现
    void onRemoteNoteInit(MsgHdr*, RemoteNoteInfo*) override;
//    virtual void onHostNoteUninit(MsgHdr*, HostNoteInfo*);    //不需要实现,由基类（AddressLayer）实现
//    virtual void onRemoteNoteUninit(MsgHdr*, RemoteNoteInfo*);//不需要实现,由基类（AddressLayer）实现

    void onNoteInputJoin0(MsgHdr*, AddressNoteInfo*) override;
    void onNoteInputExit0(MsgHdr*, AddressNoteInfo*) override;
    void onNoteInputLink0(MsgHdr*, AddressNoteInfo*) override;
    void onNoteInputSynchro0(MsgHdr*, AddressNoteInfo*) override;
    void onNoteInputSequence0(MsgHdr*, AddressNoteInfo*) override;
    void onNoteInputNormal0(MsgHdr*, AddressNoteInfo*) override;
    void onNoteInputPassive0(MsgHdr*, AddressNoteInfo*) override;
private:
    bool isNoteRepeatJoin(MsgHdr*, AddressNoteInfo*) const override { return true; }
    bool isNoteRepeatLink(MsgHdr*, AddressNoteInfo*) const override { return true; }
    void onAddressTime(uint32_t, uint32_t) override;

    volatile uint32_t link_time;        //链接触发时间
    volatile uint32_t sequence_time;    //序号触发时间
    AddressRunInfo master_run_note;     //中心化主机运行信息类
};

class ServantAddressLayer : public AddressLayer{
public:
    using AddressLayer::AddressLayer;
    ~ServantAddressLayer() override = default;

    static void onNoteJoinExecutor(AddressLayer*, BasicControl*, AddressNoteInfo*);
    static void onNoteLinkExecutor(AddressLayer*, BasicControl*, AddressNoteInfo*);
    static void onNoteTimeout(AddressLayer*, BasicControl*, AddressNoteInfo*);

    bool onNoteVerify(MsgHdr*, MeetingAddressNote*) override;
    void onRunNote(MeetingAddressNote*) override;
    void unRunNote(MeetingAddressNote*) override;
    uint32_t onRunNoteSize() const override;
    void onRunNoteInfo(const std::function<void(MeetingAddressNote*, uint32_t)>&) override;

    static void onNoteImmediateSynchro(ServantAddressLayer*, MsgHdr*);
protected:
    std::function<void(AddressNoteInfo*)> onNoteTermination() override;

//    virtual void onHostNoteInit(MsgHdr*, HostNoteInfo*);      //不需要实现,由基类（AddressLayer）实现
    void onRemoteNoteInit(MsgHdr*, RemoteNoteInfo*) override;
//    virtual void onHostNoteUninit(MsgHdr*, HostNoteInfo*);    //不需要实现,由基类（AddressLayer）实现
//    virtual void onRemoteNoteUninit(MsgHdr*, RemoteNoteInfo*);//不需要实现,由基类（AddressLayer）实现

    void onNoteInputJoin0(MsgHdr*, AddressNoteInfo*) override;
    void onNoteInputExit0(MsgHdr*, AddressNoteInfo*) override;
    void onNoteInputLink0(MsgHdr*, AddressNoteInfo*) override;
    void onNoteInputSynchro0(MsgHdr*, AddressNoteInfo*) override;
    void onNoteInputSequence0(MsgHdr*, AddressNoteInfo*) override;
    void onNoteInputNormal0(MsgHdr*, AddressNoteInfo*) override;
    void onNoteInputPassive0(MsgHdr*, AddressNoteInfo*) override;
private:
    void onNoteSequenceGap(uint32_t);

    bool isNoteRepeatJoin(MsgHdr*, AddressNoteInfo*) const override { return false; }
    bool isNoteRepeatLink(MsgHdr*, AddressNoteInfo*) const override { return false; }
    void onAddressTime(uint32_t, uint32_t) override;

    RemoteNoteInfo *core_note;      //中心化远程端的中心化主机
};

#endif //TEXTGDB_ADDRESSLAYER_H
