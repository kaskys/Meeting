//
// Created by abc on 21-1-5.
//

#ifndef TEXTGDB_TIMELAYER_H
#define TEXTGDB_TIMELAYER_H

#include "../BasicLayer.h"
#include "../../time/TimeManager.h"
#include <mutex>
#include <atomic>

#define TIME_LAYER_TIMEOUT_THRESHOLD        3       //超时门栏
#define TIME_LAYER_DELAY_TIME               4       //延迟时间
#define TIME_LAYER_BUFFER_INCREASE          5       //地址数量增加（+1）（时间管理器更改储存地址内存）
#define TIME_LAYER_BUFFER_REDUCE            6       //地址数量减少（-1）（时间管理器更改储存地址内存）
#define TIME_LAYER_TIMEOUT_FRAME            7       //超时帧数量(时间管理器更改存储提交资源)

/*
 * 时间资源提交信息
 */
class SubmitInfo final {
public:
    MediaData* getTimeBuffer() { return &media_data; }
    void setTimeBuffer(const MediaData &data) { this->media_data = data; }
private:
    MediaData media_data;   //视音频资源
};

/**
 * 时间层资源信息（包含序号及一个或多个同一序号的地址资源信息）
 */
class TimeInfo final {
public:
    TimeInfo() : total_info_size(0), submit_info_size(0), info_sequence(0) {}

    void initTimeInfo(uint32_t size, uint32_t sequence){
        this->total_info_size = size; this->info_sequence = sequence;
        memset(submit_info, 0, sizeof(SubmitInfo*) * total_info_size);
    }

    void setSubmitInfoSize(uint32_t size) { submit_info_size = size; }
    uint32_t getAllInfoSize() const { return total_info_size; }
    uint32_t getSubmitInfoSize() const { return submit_info_size; }
    uint32_t getInfoSequence() const { return info_sequence; }

    SubmitInfo* getSubmitInfo(uint32_t position) { return (submit_info + position); }
    void setSubmitInfo(SubmitInfo *info, uint32_t position) {
        memcpy(submit_info + position, info, sizeof(SubmitInfo*));
    }
private:
    uint32_t total_info_size;       //资源总数量
    uint32_t submit_info_size;      //资源提交数量
    uint32_t info_sequence;         //资源序号
    SubmitInfo submit_info[0];      //同一序号提交的资源
};

/**
 * 时间层状态信息
 */
class StatusInfo final {
    friend class TimeLayer;
public:
    StatusInfo() : need_submit(0), timeout_frame(0), delay_frame(0), last_sequence(0), timeout_threshold(0) {}
    explicit StatusInfo(uint32_t need, uint32_t tframe, uint32_t dframe, uint32_t sequence, uint32_t threshold)
            : need_submit(need), timeout_frame(tframe), delay_frame(dframe), last_sequence(sequence), timeout_threshold(threshold) {}
    ~StatusInfo() = default;

    void setNeedSubmit(uint32_t need) { need_submit = need; }
    void setTimeoutFrame(uint32_t frame) { timeout_frame = frame; }
    void setDelayFrame(uint32_t frame) { delay_frame = frame; }
    void setLastSequence(uint32_t sequence) { last_sequence = sequence; }
    void setTimeoutThreshold(uint32_t threshold) { timeout_threshold = threshold; }
private:
    uint32_t need_submit;       //需要提交的数量(时间管理层：地址数量)
    uint32_t timeout_frame;     //超时帧数量(时间管理层：存储提交资源)

    uint32_t delay_frame;       //延迟帧数量
    uint32_t last_sequence;     //最后提交的序号
    uint32_t timeout_threshold; //超时（被动处理）门栏
};

/**
 * 时间层参数
 */
class TimeLayerParameter final {
public:
    TimeLayerParameter() : store_number(0), throw_number(0) ,timeout_number(0) {}

    uint32_t getStoreNumber() const { return store_number.load(std::memory_order_consume); }
    uint32_t getThrowNumber() const { return throw_number.load(std::memory_order_consume); }
    uint32_t getTimeOutNumber() const { return timeout_number.load(std::memory_order_consume); }

    void setIncreaseStoreNumber(uint32_t increase_number) { store_number.fetch_add(increase_number, std::memory_order_release); }
    void setIncreaseThrowNumber(uint32_t increase_number) { throw_number.fetch_add(increase_number, std::memory_order_release); ; }
    void setIncreaseTimeOutNumber(uint32_t increase_number) { timeout_number.fetch_add(increase_number, std::memory_order_release); }

    void clearParameter() {
        store_number.store(0, std::memory_order_release);
        throw_number.store(0, std::memory_order_release);
        timeout_number.store(0, std::memory_order_release);
    }
private:
    std::atomic_uint store_number;      //提交存储的数量
    std::atomic_uint throw_number;      //提交失败的数量
    std::atomic_uint timeout_number;    //超时的数量
};

class TimerLayerUtil : public LayerUtil{
public:
    BasicLayer* createLayer(BasicControl*) noexcept override;
    void destroyLayer(BasicControl*, BasicLayer*) noexcept override;
};

class TimeLayer : public BasicLayer{
    friend class SubmitInfo;
public:
    explicit TimeLayer(BasicControl*);
    ~TimeLayer() override;

    void initLayer() override;
    void onInput(MsgHdr*) override;
    void onOutput() override;
    bool isDrive() const override { return true; }
    void onDrive(MsgHdr*) override;
    void onParameter(MsgHdr*) override;
    void onControl(MsgHdr*) override;
    uint32_t onStartLayerLen() const override { return 0; }
    LayerType onLayerType() const override { return LAYER_TIME_TYPE; }

    SubmitInfo* createSubmitTimeInfo(const MediaData&);
    void destroySubmitTimeInfo(SubmitInfo*);
    char* createBuffer(uint32_t);
    void destroyBuffer(const char*, uint32_t);

    void submitTimeInfo(TimeInfo*, uint32_t, const std::function<void(TimeLayer*, TimeInfo*)>&);

    static TimeInfo empty_info;             //空存储资源类（用于创建TimeInfo失败）
private:
    bool onLaunchLayer();
    void onStopLayer();

    void onInitDelayTimeInfo(uint32_t);
    void onDestroyDelayTimeInfo(TimeInfo**, uint32_t);

    void onTimeStatusStart(StatusInfo*);
    void onTimeStatusStop();

    volatile uint32_t timeout_threshold;        //MeetingAddressNote超时门栏(被动处理序号差)
    uint32_t delay_info_size;                   //存储延迟资源数量
    uint32_t delay_time_frame;                  //延迟帧数量
    uint32_t last_submit_sequence;              //最后提交的序号
    TimeLayerParameter time_parameter;          //时间层参数类

    std::mutex time_info_lock;                  //时间锁

    TimeInfo **delay_time_info;                 //存储延迟资源
    TimeManager *time_manager;                  //时间管理器
};

#endif //TEXTGDB_TIMELAYER_H
