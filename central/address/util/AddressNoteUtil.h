//
// Created by abc on 21-3-9.
//

#ifndef TEXTGDB_ADDRESSNOTEUTIL_H
#define TEXTGDB_ADDRESSNOTEUTIL_H

#include "AddressStatusUtil.h"

class RemoteNoteInfo;

//extern "C"{
//    int NetWordMaxSinglePing(unsigned int);
//    int NetWordMaxAveragePing(unsigned int);
//};

using LinkCallbackFunc = void (*)(AddressLayer*, AddressNoteInfo*);

#define PRIME_SIZE      8

struct PasswordKey{
    unsigned char match_area;    //匹配区
    unsigned char decode_area;   //补码区
    unsigned char decrypt_area;  //解密区
    unsigned char amount_area;   //数量区
};

class KeyUtil final {
public:
    static PasswordKey generateSourceKey();
    //编码密钥
    static PasswordKey codeKey(const PasswordKey&, uint32_t);
    //解码密钥
    static bool decodeKey(const PasswordKey&, const PasswordKey&);
private:
    //加密、解密算法
    static unsigned char keyAlgorithm(unsigned char, unsigned char);
};

/**
 * 远程端加入、退出指示器
 */
//class BasicIndicator {
//    friend class AddressLayer;
//#define NOTE_INDICATOR_JOIN_BIT         0x4000
//#define NOTE_INDICATOR_EXIT_BIT         0x8000
//#define NOTE_INDICATOR_IGNORE_BIT       0x0001
//#define NOTE_INDICATOR_BIT_SIZE         5
//#define NOTE_INDICATOR_POS_BIT          0x0000001F
//#define NOTE_INDICATOR_GROUP_BIT        0xFFFFFFE0
//public:
//    BasicIndicator(short status, uint32_t time) : note_status(status),
//                                                  indicator_sequence(0), max_time(time) {}
//    virtual ~BasicIndicator();
//
//    //指示器组与位置函数
//    static std::pair<uint32_t, uint32_t> onIndicatorGroup(uint32_t note_size) {
//        uint32_t group_value = ((note_size & NOTE_INDICATOR_GROUP_BIT) >>  NOTE_INDICATOR_BIT_SIZE),
//                 remain_value = (note_size & NOTE_INDICATOR_POS_BIT);
//        return std::pair<uint32_t, uint32_t>(group_value, remain_value);
//    }
//
//    virtual short onNoteIndicatorStatus() const { return note_status; }
//    virtual bool  isNoteIgnoreIndicator() const { return (note_status & NOTE_INDICATOR_IGNORE_BIT);}
//    virtual void  clrNoteIgnoreIndicator() { (note_status &= (~NOTE_INDICATOR_IGNORE_BIT)); }
//    virtual void  onNoteIgnoreIndicator() { (note_status |= NOTE_INDICATOR_IGNORE_BIT); }
//
//    virtual bool onNoteStatusDrive(uint32_t drive_sequence) { return (drive_sequence - indicator_sequence) >= max_time; }
//    virtual uint32_t onNoteIndicatorTime() const { return max_time; }
//
//    virtual AddressNoteInfo* onIndicatorNote() const = 0;
//    virtual std::pair<uint32_t, AddressNoteInfo*> onNoteIndicatorTimeout() = 0;
//
//    virtual std::pair<uint32_t, uint32_t> onNoteIndicatorValue() const = 0;
//    virtual void onFillIndicator(AddressNoteInfo*, uint32_t) = 0;
//    virtual void onStartIndicator(AddressNoteInfo*, uint32_t, uint32_t) = 0;
//    virtual void onStopIndicator() = 0;
//
//    virtual bool onConfirmNoteIndicator(uint32_t) = 0;
//    virtual void onNoteConfirmIndicator(AddressNoteInfo*, uint32_t) = 0;
//protected:
//    short note_status;                          //远程端状态（加入、退出）及是否被忽略
//    uint32_t indicator_sequence;                //正在指示的序号
//    uint32_t max_time;                          //超时时间
//};
//
//template <uint32_t IndicatorSize> class NoteStatusIndicator final : public BasicIndicator{
//public:
//    NoteStatusIndicator(short status, uint32_t time) : BasicIndicator(status, time), confirm_pos(0), note_size(0),
//                                                       indicator_note_info(nullptr) {}
//    virtual ~NoteStatusIndicator() = default;
//
//    virtual void clrNoteIgnoreIndicator(){
//        BasicIndicator::clrNoteIgnoreIndicator();
//        dynamic_cast<RemoteNoteInfo*>(indicator_note_info)->clrIndicatorNote();
//    }
//    virtual void onNoteIgnoreIndicator(){
//        BasicIndicator::onNoteIgnoreIndicator();
//        dynamic_cast<RemoteNoteInfo*>(indicator_note_info)->setIndicatorNote();
//    }
//
//    virtual AddressNoteInfo* onIndicatorNote() const { return indicator_note_info; };
//    virtual std::pair<uint32_t, uint32_t> onNoteIndicatorValue() const const { return std::pair<uint32_t, uint32_t>(IndicatorSize, note_size); }
//
//    /**
//     * 填充需要被指示的远程端（包含本身）
//     * @param fill_note_info        正在运行的远程端
//     * @param fill_pos              远程端位置
//     */
//    virtual void onFillIndicator(AddressNoteInfo *fill_note_info, uint32_t fill_pos){
//        memcpy(buffer + (sizeof(AddressNoteInfo*) * fill_pos), fill_note_info, sizeof(AddressNoteInfo*));
//        this->note_size = fill_pos + 1;
//    }
//
//    /**
//     * 启动远程端的指示器（加入或退出）
//     * @param indicator_note_info   指示远程端
//     * @param note_global_pos       指示远程端的全局位置（正在运行）
//     * @param start_sequence        启动序号
//     */
//    virtual void onStartIndicator(AddressNoteInfo *indicator_note_info, uint32_t note_global_pos, uint32_t start_sequence) {
//        this->indicator_sequence = start_sequence; this->indicator_note_info = indicator_note_info;
//
//        onNoteConfirmIndicator(indicator_note_info, note_global_pos);
//        dynamic_cast<RemoteNoteInfo*>(indicator_note_info)->setNoteIndicator(this);
//
//        clrNoteIgnoreIndicator();
//    }
//    /**
//     * 停止远程端的指示器
//     */
//    virtual void onStopIndicator() {
//        onNoteIgnoreIndicator();
//        dynamic_cast<RemoteNoteInfo*>(indicator_note_info)->setNoteIndicator(nullptr);
//    }
//
//    virtual bool onConfirmNoteIndicator(uint32_t note_global_pos) {
//        if(note_global_pos >= note_size){
//            return true;
//        }
//
//        return onFindIndicator0(note_global_pos,
//                                [&](uint32_t note_group, uint32_t note_pos) -> bool {
//                                    return static_cast<bool>((indicator_group[note_group] & (1 << note_pos)));
//                                });
//    }
//
//    virtual void onNoteConfirmIndicator(AddressNoteInfo *confirm_note, uint32_t note_global_pos) {
//        if(note_global_pos >= note_size){
//            return;
//        }
//
//        onFindIndicator0(note_global_pos,
//                         [&](uint32_t note_group, uint32_t note_pos) -> bool {
//                             indicator_group[note_group] |= (1 << note_pos);
//                         });
//
//        if(onMoveIndicator0(confirm_note, note_global_pos)){
//            onStopIndicator();
//        }
//    }
//
//    virtual std::pair<uint32_t, AddressNoteInfo*> onNoteIndicatorTimeout(){
//        onStopIndicator();
//        return std::pair<uint32_t, AddressNoteInfo*>(confirm_pos, reinterpret_cast<AddressNoteInfo*>(buffer));
//    };
//private:
//    bool onFindIndicator0(uint32_t note_global_pos, const std::function<bool(uint32_t, uint32_t)> &find_func){
//        auto group_value = onIndicatorGroup(note_global_pos);
//        return find_func(group_value.first, group_value.second);
//    };
//
//    bool onMoveIndicator0(AddressNoteInfo *move_note, uint32_t note_pos){
//        memmove(buffer + (sizeof(AddressNoteInfo*) * note_pos), buffer + (sizeof(AddressNoteInfo*) * (note_pos + 1)), confirm_pos - note_pos - 1);
//        memcpy(buffer + (sizeof(AddressNoteInfo*) + (--confirm_pos)), move_note, sizeof(AddressNoteInfo*));
//        return (confirm_pos <= 0);
//    }
//    uint32_t confirm_pos;                       //确定指示的位置
//    uint32_t note_size;                         //需要指示的数量
//    uint32_t indicator_group[IndicatorSize];    //确定指示组
//
//    AddressNoteInfo *indicator_note_info;       //指示的远程端
//    char buffer[0];                             //需要指示的远程端数组
//};

class AddressRunInfo final {
public:
    AddressRunInfo() = default;
    ~AddressRunInfo() = default;

    uint32_t onRunNoteSize() const { return run_note_size; }
    void onLinkRunNote(RemoteNoteInfo*);
    void unLinkRunNote(RemoteNoteInfo*);
    void onRunNoteInfo(const std::function<void(MeetingAddressNote*, uint32_t)>&);
private:
    uint32_t run_note_size;             //运行远程端数量
    RemoteNoteInfo *run_note_info;      //执行远程端的指针（链表）
};

struct NoteLinkInfo {
    NoteLinkInfo() : last_active_sequence(0) {}
    virtual ~NoteLinkInfo() = default;

    void onClear(){ last_active_sequence = 0; }
    void onNormal(uint32_t sequence) { last_active_sequence = sequence; }

    virtual void onJoin(uint32_t, uint32_t) = 0;
    virtual void onLink(uint32_t) = 0;
    virtual bool onSynchro(uint32_t) = 0;
    virtual void onSequence(uint32_t, uint32_t, uint32_t ,uint32_t) = 0;
    virtual void onExit() = 0;

    uint32_t last_active_sequence;      //最后接收数据的序号
};

struct RemoteLinkInfo : public NoteLinkInfo{
    explicit RemoteLinkInfo(uint32_t need_number) : NoteLinkInfo(), remote_join_time(0), remote_link_time(0), remote_sequence_time(0),
                                                    remote_exit_time(0), link_sequence(0), linked_number(0), link_need_number(need_number),
                                                    link_rtt_total(0) {}
    ~RemoteLinkInfo() override = default;

    void onClear(){
        NoteLinkInfo::onClear();
        remote_join_time = 0; remote_link_time = 0; remote_sequence_time = 0; remote_exit_time = 0;
        link_sequence = 0; linked_number = 0; link_rtt_total = 0;
        memset(link_rtt_array, 0, sizeof(uint32_t) * link_need_number);
    }
    void onData(ParameterRemote *parameter){
        parameter->remote_join_time = remote_join_time; parameter->remote_link_time = remote_link_time;
        parameter->remote_sequence_time = remote_sequence_time;
        parameter->remote_exit_time = remote_exit_time; parameter->link_sequence = link_sequence;
    }
    uint32_t onRtt() const {
        return (link_rtt_total / link_need_number);
    }

    void onJoin(uint32_t link_time, uint32_t timeout) override {
        remote_join_time = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    }

    void onLink(uint32_t passive_time) override {
        remote_link_time = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    }

    bool onSynchro(uint32_t rtt) override {
        if(linked_number >= link_need_number){
            int find_pos = 0;
            for(int i = 0, max_value = 0; i < link_need_number; i++){
                if(link_rtt_array[i] > max_value){
                    max_value = link_rtt_array[(find_pos = i)];
                }
            }
            if(rtt < link_rtt_array[find_pos]){
                link_rtt_array[find_pos] = rtt;
                link_rtt_total -= (link_rtt_array[find_pos] - rtt);
            }
            return true;
        }else {
            link_rtt_array[link_need_number++] = rtt;
            link_rtt_total += rtt;
            return false;
        }
    }

    void onSequence(uint32_t, uint32_t, uint32_t, uint32_t sequence) override {
        remote_sequence_time = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
        link_sequence = sequence;
    }

    void onExit() override {
        remote_exit_time = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    }

    uint32_t remote_join_time;      //远程端加入时间（最后一次）
    uint32_t remote_link_time;      //远程端链接时间（最后一次）
    uint32_t remote_sequence_time;  //发送远程序号时间（最后一次）
    uint32_t remote_exit_time;      //远程端退出时间

    uint32_t link_sequence;         //响应远程端序号
    uint32_t linked_number;         //远程端响应同步次数
    uint32_t link_need_number;      //最多需要同步次数
    uint32_t link_rtt_total;        //远程端在链接期间的总共往返时间和
    uint32_t link_rtt_array[0];     //存储往返时间的数组
};

struct HostLinkInfo : public NoteLinkInfo {
    HostLinkInfo() : NoteLinkInfo(), host_start_time(0), host_end_time(0), link_rtt(0), link_frame(0), link_delay(0),
                                                                      link_time(0), link_timeout(0), passive_timeout(0){
        host_start_time = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    }
    ~HostLinkInfo() override = default;

    void onClear() {
        NoteLinkInfo::onClear();
        host_start_time = 0; host_end_time = 0; link_rtt = 0; link_frame = 0;
        link_time = 0; link_timeout = 0; passive_timeout = 0;
    }
    void onData(ParameterHost *parameter){
        parameter->start_time = host_start_time; parameter->end_time = host_end_time;
        parameter->link_rtt = link_rtt; parameter->link_frame = link_frame;
        parameter->link_delay = link_delay;
        parameter->link_time = link_time; parameter->link_timeout = link_timeout;
        parameter->passive_timeout = passive_timeout;
    }

    void onJoin(uint32_t link_time, uint32_t timeout) override {
        this->link_time = link_time;
        this->link_timeout = timeout;
    }

    void onLink(uint32_t timeout) override {
        this->passive_timeout = timeout;
    }

    bool onSynchro(uint32_t) override {
        return true;
    }

    void onSequence(uint32_t link_rtt, uint32_t link_frame, uint32_t link_delay, uint32_t sequence) override {
        this->link_rtt = link_rtt; this->link_frame = link_frame; this->link_delay = link_delay;
    }

    void onExit() override {
        host_end_time = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    }

    uint32_t host_start_time;   //启动时间
    uint32_t host_end_time;     //结束时间

    uint32_t link_rtt;          //与主机的往返时间（远程端变量）
    uint32_t link_frame;        //帧数（主机与远程端一致）
    uint32_t link_delay;        //延迟时间（主机与远程端一致）
    uint32_t link_time;         //链接时间
    uint32_t link_timeout;      //链接超时时间
    uint32_t passive_timeout;   //被动链接超时时间
};

class AddressNoteInfo : public MeetingAddressNote{
public:
    AddressNoteInfo() : MeetingAddressNote(), note_position(0), key_amount(0), note_status(NOTE_STATUS_INIT), note_key() {}
    ~AddressNoteInfo() override = default;

    virtual StatusNote onNoteAttributeStatus() const = 0;
    virtual uint32_t getLastSequence() const = 0;
    virtual void clearLinkInfo() = 0;
    virtual void clearTimerInfo() = 0;

    virtual void onNoteJoin(uint32_t, uint32_t) = 0;
    virtual void onNoteLink(uint32_t, uint32_t) = 0;
    virtual void onNoteSequence(const SequenceData&) = 0;
    virtual void onNoteNormal(uint32_t) = 0;
    virtual void onNotePassive() = 0;
    virtual void onNoteExit() = 0;
    virtual void onNoteInvalid() = 0;

    virtual LinkData onNoteLinkData() = 0;
    virtual SequenceData onNoteSequenceData() = 0;

    virtual void callTerminationFunc() = 0;
    virtual void setTerminationFunc(std::function<void(AddressNoteInfo*)>) = 0;

    //远程端初始化NoteKey的函数
    void onNoteKey() { note_key = KeyUtil::generateSourceKey(); }
    //主机接收解码远程端输入NoteKey的函数
    void onNoteKey(const PasswordKey &key) { note_key = KeyUtil::codeKey(key, key.amount_area); }
    //主机设置NoteKey数量区的函数
    void onNoteKey(char size) { note_key.amount_area = static_cast<unsigned char>(size); }
    //验证主机与远程端的NoteKey是否一致
    bool isNoteKey(const PasswordKey &code_key, const PasswordKey &source_key) { return KeyUtil::decodeKey(code_key, source_key); }
    //远程端获取已初始化的NoteKey并是否增加数量区的函数（默认不增加）
    PasswordKey getNoteKey(char increase_value = 0) { note_key.amount_area += increase_value; return note_key; }
    void onNoteTransfer(const sockaddr_in addr) { this->man_key = addr.sin_addr.s_addr; this->man_port = addr.sin_port; }

    void setStatusNote(StatusNote status) { note_status.store(status, std::memory_order_release); }
    StatusNote getStatusNote() const { return note_status.load(std::memory_order_consume); }
protected:
    void onInit();
    void onNoteAddress(const std::function<void(uint32_t, uint32_t)> &callback) { callback(man_port, man_key); }
private:
    static std::atomic<uint32_t> note_sequence_generator;       //远程端序号生成器

    std::atomic<StatusNote> note_status;                        //远程端状态
    uint32_t note_position;                                     //远程端编号
    uint32_t key_amount;                                        //远程端密钥数量（接收）
    PasswordKey note_key;                                       //远程端密钥（实体）
};

class RemoteNoteInfo : public AddressNoteInfo, public NoteFilter, public NoteTimerInfo{
    friend class AddressRunInfo;
public:
    RemoteNoteInfo() : AddressNoteInfo(), NoteFilter(), NoteTimerInfo(0), prev_remote_note(nullptr), next_remote_note(nullptr),
                       correlate_thread(nullptr), remote_parameter(), remote_link_info(0), termination_func(nullptr) {}
    explicit RemoteNoteInfo(uint32_t link_number) : AddressNoteInfo(), NoteFilter(), NoteTimerInfo(0), prev_remote_note(nullptr),
                                                    next_remote_note(nullptr), correlate_thread(nullptr), remote_parameter(),
                                                    remote_link_info(link_number), termination_func(nullptr) {}
    ~RemoteNoteInfo() override = default;

    RemoteNoteInfo(const RemoteNoteInfo&) = delete;
    RemoteNoteInfo& operator=(const RemoteNoteInfo&) = delete;
//    RemoteNoteInfo(RemoteNoteInfo&&) = delete;
//    RemoteNoteInfo& operator=(RemoteNoteInfo&&) = delete;

    uint32_t getLinkSize() const { return remote_link_info.link_need_number; }
    uint32_t onNoteRttSize() { return remote_parameter.onRttSize(); }
    void onNoteSynchro();
    void onNoteSynchro(uint32_t, SequenceData*, AddressLayer*, LinkCallbackFunc);
    void onData(ParameterRemote*);

    StatusNote onNoteAttributeStatus() const override { return NOTE_STATUS_REMOTE; }
    uint32_t getLastSequence() const override { return remote_link_info.last_active_sequence; }
    void clearLinkInfo() override { remote_link_info.onClear(); }
    void clearTimerInfo() override { clearNoteTimerInfo(); }

    void callTerminationFunc() override { if(termination_func) { termination_func(this); }}
    void setTerminationFunc(std::function<void(AddressNoteInfo*)> func) override { termination_func = func; }

    void onNoteJoin(uint32_t, uint32_t) override;
    void onNoteLink(uint32_t, uint32_t) override;
    void onNoteSequence(const SequenceData&) override;
    void onNoteNormal(uint32_t) override;
    void onNotePassive() override;
    void onNoteExit() override;
    void onNoteInvalid() override;

    LinkData onNoteLinkData() override;
    SequenceData onNoteSequenceData() override;

    void onNoteReInit(uint16_t);
    uint32_t onNoteRtt() const { return remote_link_info.onRtt(); }
    bool isCorrelateThread();
    void correlateThreadUtil(TransmitThreadUtil *thread_util) { correlate_thread = thread_util; }
    TransmitThreadUtil* correlateThreadUtil() { return correlate_thread; }

    static void onNoteInit(RemoteNoteInfo*, uint16_t, uint32_t, uint32_t);
    static void onLinkRunNote(AddressRunInfo*, RemoteNoteInfo*);
    static void unLinkRunNote(AddressRunInfo*, RemoteNoteInfo*);
private:
    RemoteNoteInfo *prev_remote_note;                       //远程端链表（前向指针）
    RemoteNoteInfo *next_remote_note;                       //远程端链表（后向指针）

    TransmitThreadUtil *correlate_thread;                   //远程端关联的传输线程工具类
    RemoteNoteParameter remote_parameter;                   //远程端参数类
    RemoteLinkInfo remote_link_info;                        //远程端链接信息类
    std::function<void(AddressNoteInfo*)> termination_func; //远程端终止函数
};

class HostNoteInfo : public AddressNoteInfo{
public:
    HostNoteInfo() : AddressNoteInfo(), host_link_info() {}
    ~HostNoteInfo() override = default;

    HostNoteInfo(const HostNoteInfo&) = delete;
    HostNoteInfo& operator=(const HostNoteInfo&) = delete;
//    HostNoteInfo(HostNoteInfo&&) = delete;
//    HostNoteInfo& operator=(HostNoteInfo&&) = delete;

    StatusNote onNoteAttributeStatus() const override { return NOTE_STATUS_HOST; }
    uint32_t getLastSequence() const override { return host_link_info.last_active_sequence; }
    void clearLinkInfo() override { host_link_info.onClear(); }
    void clearTimerInfo() override {}

    void callTerminationFunc() override {}
    void setTerminationFunc(std::function<void(AddressNoteInfo*)>) override {}

    void onNoteSynchro(uint32_t);
    void onData(ParameterHost*);

    void onNoteJoin(uint32_t, uint32_t) override;
    void onNoteLink(uint32_t, uint32_t) override;
    void onNoteSequence(const SequenceData&) override;
    void onNoteNormal(uint32_t) override;
    void onNotePassive() override;
    void onNoteExit() override;
    void onNoteInvalid() override;

    LinkData onNoteLinkData() override;
    SequenceData onNoteSequenceData() override;

    static void onNoteInit(HostNoteInfo*, uint16_t, uint32_t);
private:
//    HostNoteParameter host_parameter;
    HostLinkInfo host_link_info;            //本机端链接信息类
};

#endif //TEXTGDB_ADDRESSNOTEUTIL_H
