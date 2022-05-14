//
// Created by abc on 21-2-2.
//

#ifndef TEXTGDB_ADDRESSUTIL_H
#define TEXTGDB_ADDRESSUTIL_H

#include "../AddressParameter.h"
#include "AddressTimerUtil.h"

//重构MeetingAddressNote（需要包含的信息）
//    1：过滤信息
//    2：关联的SocketThread信息（或线程提供的信息）
//    3：状态信息
//    4：统计信息
//    5：遍历函数
//    6：连接信息
//    7：转移信息
//    8：线程信息

class AddressLayer;
class AddressNoteInfo;

/*
 * 远程端状态机
 */
enum StatusNote{
    NOTE_STATUS_INIT = 0,       //初始化阶段
    NOTE_STATUS_HOST,           //主机节点(由本地控制创建)
    NOTE_STATUS_REMOTE,         //远程端节点(由输入控制创建)
    NOTE_STATUS_JOIN,           //加入阶段(由输入控制变更)
    NOTE_STATUS_LINK,           //链接阶段(由输入控制变更)
    NOTE_STATUS_SEQUENCE,       //序号阶段（由输入和本机共同控制变更->确定RTT,链接完成）
    NOTE_STATUS_NORMAL,         //正常阶段(由输入控制变更)
    NOTE_STATUS_EXIT_PASSIVE,   //被动退出阶段(由本地控制变更)
    NOTE_STATUS_EXIT,           //退出节点(由本地或输入控制变更)
    NOTE_STATUS_SIZE
};

/*
 * 远程端错误信息
 */
enum ErrorType{
    NOTE_ERROR_TYPE_NONE = 0,       //无类型错误
    NOTE_ERROR_TYPE_JOINING,        //加入类型错误
    NOTE_ERROR_TYPE_LINKING,        //链接类型错误
    NOTE_ERROR_TYPE_RUNNING,        //运行类型错误
    NOTE_ERROR_TYPE_INIT,           //初始化类型错误
    NOTE_ERROR_TYPE_MEMORY,         //内存类型错误
    NOTE_ERROR_TYPE_INVALID_JOIN,   //无效加入类型错误
    NOTE_ERROR_TYPE_UNSUPPORT       //无效类型错误
};

/**
 * 远程端过滤信息（黑名单）
 */
class NoteFilter : public LayerFilter {
public:
    NoteFilter() : LayerFilter(), black(false), indicator(false){}
    ~NoteFilter() override = default;

    void setBlackNote() { black.store(true, std::memory_order_release); }
    void clrBlackNote() { black.store(false, std::memory_order_release); }
    bool getBlackNote() const { return black.load(std::memory_order_consume); }

    void setIndicatorNote() { indicator.store(true, std::memory_order_release); }
    void clrIndicatorNote() { indicator.store(false, std::memory_order_release); }
    bool getIndicatorNote() const { return indicator.load(std::memory_order_consume); }
private:
    std::atomic_bool black;     //是否黑名单
    std::atomic_bool indicator; //指示器（是否链接：默认链接）
};

/**
 * 远程端设置够滤信息
 */
struct NoteFilterInfo{
    FilterInfo filter_info;     //过滤信息
    AddressNoteInfo *note_info; //被设置的远程端
};

/**
 * 远程端错误信息
 */
class ErrorInfo {
public:
    ErrorInfo(sockaddr_in addr, ErrorType type, StatusNote status) : error_type(type), error_status(status), error_addr(addr) {}
private:
    ErrorType  error_type;      //错误信息
    StatusNote error_status;    //错误状态
    sockaddr_in error_addr;     //地址信息
};

class AddressStatusUtil{
public:
    explicit AddressStatusUtil(StatusNote status_) noexcept : status(status_){}
    virtual ~AddressStatusUtil() = default;

    static void onNoteCreate(AddressLayer*, MsgHdr*, AddressStatusUtil*);
    static void onNoteInit(AddressLayer*, MsgHdr*, AddressStatusUtil*);
    static void onNoteUninit(AddressLayer*, MsgHdr*, AddressNoteInfo*, AddressStatusUtil*);
    static bool onNoteRepeat(AddressLayer*, MsgHdr*, AddressNoteInfo*, AddressStatusUtil*);

    virtual void onInputJoin(MsgHdr*, AddressLayer*, AddressNoteInfo*) = 0;
    virtual void onInputLink(MsgHdr*, AddressLayer*, AddressNoteInfo*) = 0;
    virtual void onInputSynchro(MsgHdr*, AddressLayer*, AddressNoteInfo*) = 0;
    virtual void onInputNormal(MsgHdr*, AddressLayer*, AddressNoteInfo*) = 0;
    virtual void onInputPassive(MsgHdr*, AddressLayer*, AddressNoteInfo*) = 0;
    virtual void onInputExit(MsgHdr*, AddressLayer*, AddressNoteInfo*) = 0;
    virtual void onInputUnSupport(AddressLayer*, AddressNoteInfo*, StatusNote) = 0;

    //默认不支持（HostStatusUtil、RemoteStatusUtil例外）
    virtual void onInputCreate(AddressLayer *layer, AddressNoteInfo*, uint16_t, uint32_t, uint32_t) { onInputUnSupport(layer, nullptr, NOTE_STATUS_INIT);}
    //默认不支持（HostStatusUtil、RemoteStatusUtil例外）
    virtual void onInputInit(MsgHdr*, AddressLayer *layer, AddressNoteInfo*, uint16_t) { onInputUnSupport(layer, nullptr, NOTE_STATUS_INIT); }
    //默认不支持（HostStatusUtil、RemoteStatusUtil例外）
    virtual void onInputUninit(MsgHdr*, AddressLayer *layer, AddressNoteInfo*) { onInputUnSupport(layer, nullptr, NOTE_STATUS_INIT); }
    //默认忽略处理（HostStatusUtil、RemoteStatusUtil、ExitStatusUtil例外）
    virtual bool onInputRepeat(MsgHdr*, AddressLayer*, AddressNoteInfo*) { return false; }
    virtual bool onUtilLinkInfo() { return false; }

    StatusNote onUtilStatus() const { return status; }
protected:
    void onStatus(AddressNoteInfo*, StatusNote);

    StatusNote status;      //远程端状态（当前）
};

class HostStatusUtil : public AddressStatusUtil {
public:
    using AddressStatusUtil::AddressStatusUtil;
    ~HostStatusUtil() override = default;

    void onInputJoin(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputLink(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputSynchro(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputNormal(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputPassive(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputExit(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputUnSupport(AddressLayer*, AddressNoteInfo*, StatusNote) override;

    void onInputCreate(AddressLayer*, AddressNoteInfo*, uint16_t, uint32_t, uint32_t) override;
    void onInputInit(MsgHdr*, AddressLayer*, AddressNoteInfo*, uint16_t) override;
    void onInputUninit(MsgHdr*, AddressLayer *layer, AddressNoteInfo*) override;
    bool onInputRepeat(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
};

class RemoteStatusUtil : public AddressStatusUtil{
public:
    using AddressStatusUtil::AddressStatusUtil;
    ~RemoteStatusUtil() override = default;

    void onInputJoin(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputLink(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputSynchro(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputNormal(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputPassive(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputExit(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputUnSupport(AddressLayer*, AddressNoteInfo*, StatusNote) override;

    void onInputCreate(AddressLayer*, AddressNoteInfo*, uint16_t, uint32_t, uint32_t) override;
    void onInputInit(MsgHdr*, AddressLayer*, AddressNoteInfo*, uint16_t) override;
    void onInputUninit(MsgHdr*, AddressLayer *layer, AddressNoteInfo*) override;
    bool onInputRepeat(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    bool onUtilLinkInfo() override { return true; }
};

class JoinStatusUtil : public AddressStatusUtil{
public:
    using AddressStatusUtil::AddressStatusUtil;
    ~JoinStatusUtil() override = default;

    void onInputJoin(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputLink(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputSynchro(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputNormal(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputPassive(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputExit(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputUnSupport(AddressLayer*, AddressNoteInfo*, StatusNote) override;
};

class LinkStatusUtil : public AddressStatusUtil{
public:
    using AddressStatusUtil::AddressStatusUtil;
    ~LinkStatusUtil() override = default;

    void onInputJoin(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputLink(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputSynchro(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputNormal(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputPassive(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputExit(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputUnSupport(AddressLayer*, AddressNoteInfo*, StatusNote) override;

    void onInputSequence(MsgHdr*, AddressLayer*, AddressNoteInfo*);
};

class SequenceStatusUtil : public AddressStatusUtil{
public:
    using AddressStatusUtil::AddressStatusUtil;
    ~SequenceStatusUtil() override = default;

    void onInputJoin(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputLink(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputSynchro(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputNormal(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputPassive(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputExit(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputUnSupport(AddressLayer*, AddressNoteInfo*, StatusNote) override;
};

class NormalStatusUtil : public AddressStatusUtil{
public:
    using AddressStatusUtil::AddressStatusUtil;
    ~NormalStatusUtil() override = default;

    void onInputJoin(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputLink(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputSynchro(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputNormal(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputPassive(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputExit(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputUnSupport(AddressLayer*, AddressNoteInfo*, StatusNote) override;
};

class PassiveStatusUtil : public AddressStatusUtil{
public:
    using AddressStatusUtil::AddressStatusUtil;
    ~PassiveStatusUtil() override = default;

    void onInputJoin(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputLink(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputSynchro(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputNormal(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputPassive(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputExit(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputUnSupport(AddressLayer*, AddressNoteInfo*, StatusNote) override;
};

class ExitStatusUtil : public AddressStatusUtil{
public:
    using AddressStatusUtil::AddressStatusUtil;
    ~ExitStatusUtil() override = default;

    void onInputJoin(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputLink(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputSynchro(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputNormal(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputPassive(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputExit(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
    void onInputUnSupport(AddressLayer*, AddressNoteInfo*, StatusNote) override;
    bool onInputRepeat(MsgHdr*, AddressLayer*, AddressNoteInfo*) override;
};

#endif //TEXTGDB_ADDRESSUTIL_H
