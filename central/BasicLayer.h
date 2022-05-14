//
// Created by abc on 20-12-8.
//

#ifndef TEXTGDB_BASICLAYER_H
#define TEXTGDB_BASICLAYER_H

#include <atomic>
#include <cstring>
#include <cstdint>
#include <stdexcept>
#include <functional>
#include <netinet/in.h>
#include <chrono>
#include "../executor/ExecutorServer.h"
#include "../space/manager/MemoryReader.h"
#include "../meetingaddress/MeetingAddressManager.h"

#define LAYER_BIT_SIZE                          8
#define LAYER_MATER_TYPE_SIZE                   255

#define LAYER_MASTER_JOIN                       1
#define LAYER_MASTER_LINK                       2
#define LAYER_MASTER_MEDIA                      3
#define LAYER_MASTER_EXIT                       4
#define LAYER_MASTER_EXIT_PASSIVE               5
#define LAYER_MASTER_INIT                       6
#define LAYER_MASTER_UNINIT                     7
#define LAYER_MASTER_CREATE                     8
#define LAYER_MASTER_TIME_DRIVE                 9
#define LAYER_MASTER_PREVENT                    10
#define LAYER_MASTER_UPDATE                     11
#define LAYER_MASTER_NOTE_DISPLAY               12

#define LAYER_SHARED_SYNCHRO                    0x01
#define LAYER_SHARED_SEQUENCE                   0x02
#define LAYER_SHARED_TRANSFER                   0x04
#define LAYER_SHARED_QUERY                      0x08
#define LAYER_SHARED_TEXT                       0x10
#define LAYER_SHARED_ERROR                      0x20

#define LAYER_RESPONSE_JOIN                     0x01
#define LAYER_RESPONSE_SYNCHRO                  0x02
#define LAYER_RESPONSE_TRANSFER                 0x04
#define LAYER_RESPONSE_QUERY                    0x08
#define LAYER_RESPONSE_STATUS                   0x10

#define LAYER_CONTROL_ADDRESS_ERROR             1
#define LAYER_CONTROL_REQUEST_TIMER             2
#define LAYER_CONTROL_REQUEST_ALLOC_DYNAMIC     3
#define LAYER_CONTROL_REQUEST_ALLOC_FIXED       4
#define LAYER_CONTROL_REQUEST_DESTROY_DYNAMIC   5
#define LAYER_CONTROL_REQUEST_DESTROY_FIXED     6
#define LAYER_CONTROL_REQUEST_ADDRESS_BLACK     7
#define LAYER_CONTROL_TRANSMIT_INIT_THREAD      8
#define LAYER_CONTROL_TRANSMIT_UNINIT_THREAD    9
#define LAYER_CONTROL_EXECUTOR_CREATE_THREAD    10
#define LAYER_CONTROL_EXECUTOR_DESTROY_THREAD   11
#define LAYER_CONTROL_TRANSMIT_CORRELATE_THREAD 12
#define LAYER_CONTROL_TRANSMIT_MAIN_THREAD_UTIL 13
#define LAYER_CONTROL_CORE_SEQUENCE_GAP         14
#define LAYER_CONTROL_DISPLAY_DRIVE_FRAME       15
#define LAYER_CONTROL_CORE_TERMINATION          16

#define LAYER_CONTROL_FLAG_INPUT_SIZE           11
#define LAYER_CONTROL_FLAG_CONTROL_SIZE         11

#define LAYER_CONTROL_TIMER_MORE                6
#define LAYER_CONTROL_TIMER_IMMEDIATELY         7
#define LAYER_CONTROL_NOTE_JOIN                 8
#define LAYER_CONTROL_NOTE_LINK                 9
#define LAYER_CONTROL_NOTE_SEQUENCE             10
#define LAYER_CONTROL_NOTE_NORMAL               11
#define LAYER_CONTROL_NOTE_PASSIVE              12
#define LAYER_CONTROL_NOTE_EXIT                 13

#define LAYER_CONTROL_STATUS_STOP               0
#define LAYER_CONTROL_STATUS_START              1
#define LAYER_CONTROL_STATUS_THROW              2
#define LAYER_CONTROL_NOTE_TERMINATION          3

//error
#define LAYER_SHARED_ERROR_CODE_PREVENT         1

struct MsgHdr;
class BasicControl;
class BasicLayer;
class AddressNoteInfo;
class NoteMediaInfo;
class TimeInfo;
class TransmitThreadUtil;

/*
 * 远程端状态（弃用）
 */
enum RemoteStatus{
    REMOTE_STATUS_JOIN,     //加入
    REMOTE_STATUS_NORMAL,   //正常
    REMOTE_STATUS_EXIT      //退出
};

/*
 * 层类型
 */
enum LayerType{
    LAYER_EXECUTOR_TYPE = 0,    //执行层
    LAYER_MEMORY_TYPE,          //内存层
    LAYER_ADDRESS_TYPE,         //地址层
    LAYER_TIME_TYPE,            //时间层
    LAYER_TRANSMIT_TYPE,        //传输层
    LAYER_DISPLAY_TYPE,         //显示层
    LAYER_USER_TYPE,            //用户层
    LAYER_CONFIGURE_TYPE,       //配置
    LAYER_TYPE_SIZE             //
};

/*
 * 过滤信息
 */
struct FilterInfo{
    short master_type;      //主类型
    short shared_type;      //共享类型
    short response_type;    //响应类型
};

/*
 * 过滤工具
 */
struct LayerFilter {
public:
    LayerFilter() = default;
    virtual ~LayerFilter() = default;
//    LayerFilter(const LayerFilter&) = delete;
//    LayerFilter& operator=(const LayerFilter&) = delete;
    LayerFilter(LayerFilter&&) = delete;
    LayerFilter& operator=(LayerFilter&&) = delete;

    virtual void clear();
    virtual void setFilter(const FilterInfo&);
    virtual void clrFilter(const FilterInfo&);

    virtual bool onFilterMaster(short);
    virtual bool onFilterShared(short);
    virtual bool onFilterResponse(short);
protected:
    void setMasterFilter(short);
    void setSharedFilter(short);
    void setResponseFilter(short);

    void clrMasterFilter(short);
    void clrSharedFilter(short);
    void clrResponseFilter(short);

    std::atomic_short shared_;      //过滤主类型
    std::atomic_short response_;    //过滤响应类型
    std::atomic_bool master_[LAYER_MATER_TYPE_SIZE];    //过滤共享类型
};

/*
 * 链接数据
 */
struct LinkData {
    uint32_t link_time;     //链接触发时间（远程端）、同步触发时间（主机）
    uint32_t link_timeout;  //链接超时时间（远程端）、同步超时时间（主机）
    uint32_t link_passive;  //被动链接时间（远程端）
};

/*
 * 序号数据
 */
struct SequenceData{
    uint32_t link_rtt;      //客户机与服务器传输时间
    uint32_t link_frame;    //显示帧数
    uint32_t link_delay;    //延迟时间
    uint32_t serial_number; //序号
};

/*
 * 同步数据
 */
struct SynchroData{
    uint32_t synchro_send_time;     //发送时间
    uint32_t synchro_recv_time;     //接收时间
};

/*
 * 信息数据
 */
struct InfoData{
    InfoData() = default;
    explicit InfoData(int len, int offset) : info_len(len), info_offset(offset) {}
    explicit InfoData(int len, char *buffer = nullptr) : info_len(len), info_buffer(buffer) {}
    ~InfoData() = default;

    InfoData(const InfoData &data) noexcept : InfoData() {
        memcpy(this, &data, sizeof(InfoData));
    }
    InfoData& operator=(const InfoData &data) noexcept {
        memcpy(this, &data, sizeof(InfoData));
    }

    int info_len;           //数据长度
    union {
        int info_offset;    //偏移量
        char *info_buffer;  //指向数据内存
    };
};

/*
 * 存储多个视音频资源
 */
struct MediaData{
    short media_size;                   //视音频资源数量（默认为1,中心化非核心为接收的数量）
    short media_len_offset;             //视音频资源的长度偏移量（中心化非核心才有效）
    short media_buffer_offset;          //视音频资源的内存偏移量（中心化非核心才有效）
    MeetingAddressNote *media_note;     //本机的MeetingAddressNote
    MemoryReader media_reader;          //视音频资源读取器（指向被FixedMemory封装的NoteMediaInfo2的视音频资源内存或被DynamicMemory封装的内存）
    std::function<void(MediaData*, NoteMediaInfo*, int)> analysis_media_func;   //资源解析函数（将MediaData（多个资源）解析为NoteMediaInfo（单个资源））
};

/*
 * 资源数据（弃用）
 */
struct ResourceData{
    short resource_len;             //资源长度
    short resource_offset;          //资源偏移量
    MemoryReader resource_reader;   //资源读取器
};

/*
 * 远程端转移数据
 */
struct ExecutorTransferData{
    uint32_t transfer_thread_id;            //转移线程id（传输线程类）
    union {
        BasicThread *transfer_thread;       //转移线程
        MeetingAddressNote *transfer_note;  //转移远程端
    };
};

/*
 * 初始化执行定时器数据
 */
struct ExecutorTimerData{
    ExecutorTimerData() : executor_amount(0), executor_microsecond(0), executor_increase_microsecond(0),
                          executor_func(nullptr), timeout_func(nullptr) {}
    explicit ExecutorTimerData(uint64_t amount, uint64_t microsecond, uint64_t increase_microsecond, std::function<void()> efunc, std::function<void()> tfunc)
            : executor_amount(amount),executor_microsecond(microsecond), executor_increase_microsecond(increase_microsecond),
              executor_func(std::move(efunc)), timeout_func(std::move(tfunc)) {}
    ~ExecutorTimerData() = default;

    ExecutorTimerData(const ExecutorTimerData&) = default;
    ExecutorTimerData(ExecutorTimerData&&) = default;
    ExecutorTimerData& operator=(const ExecutorTimerData&) = default;
    ExecutorTimerData& operator=(ExecutorTimerData&&) = default;

    /*
     * 触发次数
     *  <= 1 为 立即或延迟任务（视触发时间而定）
     *  >  1 为 多次任务
     */
    uint64_t executor_amount;
    uint64_t executor_microsecond;          //触发时间间隔
    uint64_t executor_increase_microsecond; //触发时间间隔增长值
    std::function<void()> executor_func;    //触发函数
    std::function<void()> timeout_func;     //超时函数
};

/*
 * 传输远程状态
 */
struct TransmitNoteStatus{
    TransmitNoteStatus() : note_status(0), note_position(0), note_address(0) {}
    explicit TransmitNoteStatus(uint32_t status, uint32_t position, uint32_t addr)
            : note_status(status), note_position(position), note_address(addr) {}
    ~TransmitNoteStatus() = default;

    uint32_t note_status;       //远程端状态
    uint32_t note_position;     //远程端序号
    uint32_t note_address;      //远程端地址
};

class LayerUtil {
public:
    LayerUtil() = default;
    virtual ~LayerUtil() = default;

    virtual BasicLayer* createLayer(BasicControl*) noexcept = 0;
    virtual void destroyLayer(BasicControl*, BasicLayer*) noexcept = 0;
};

class BasicLayer {
public:
    explicit BasicLayer(BasicControl*);
    virtual ~BasicLayer() = default;

    virtual void initLayer() = 0;
    virtual void onInput(MsgHdr*) = 0;
    virtual void onOutput() = 0;
    virtual bool isDrive() const = 0;
    virtual void onDrive(MsgHdr*) = 0;
    virtual void onParameter(MsgHdr*) = 0;
    virtual void onControl(MsgHdr*) = 0;
    virtual uint32_t onStartLayerLen() const = 0;
    virtual LayerType onLayerType() const = 0;

    static BasicLayer* createLayer(BasicControl*, LayerType);
    static void destroyLayer(BasicControl*, BasicLayer*);
protected:
    virtual char* onCreateBuffer(uint32_t);
    virtual void  onCreateBuffer(const std::function<void(MsgHdr*)>&, const std::function<void(MsgHdr*)>&);
    virtual void  onDestroyBuffer(uint32_t, const std::function<void(MsgHdr*)>&);

    BasicControl *basic_control;                    //控制层
private:
    static LayerUtil* layer_util[LAYER_TYPE_SIZE];  //层工具集合（地址、显示、执行、内存、时间、传输、用户）
};

#endif //TEXTGDB_BASICLAYER_H
