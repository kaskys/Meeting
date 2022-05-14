//
// Created by abc on 20-12-30.
//

#ifndef TEXTGDB_TIMEMANAGER_H
#define TEXTGDB_TIMEMANAGER_H

#include "BasicManager.h"
#include "../concurrent/door/Door.h"

/**
 * 时间管理器存储资源缓存
 */
class TimeBuffer final{
public:
    TimeBuffer() : submit_position(0), use_sequence(0), time_info(nullptr) {}
    TimeBuffer(uint32_t sequence, TimeInfo *info) : submit_position(0), use_sequence(sequence), time_info(info) {}
    ~TimeBuffer() = default;

    //获取序号
    uint32_t onSubmitSequence() const { return use_sequence.load(std::memory_order_acquire); }
    //设置序号
    void setSubmitSequence(uint32_t sequence) { use_sequence.store(sequence, std::memory_order_release); }

    //获取储存资源位置
    uint32_t onSubmitPosition() const { return submit_position.load(std::memory_order_consume); }
    //获取当前存储资源位置并设置存储资源位置为0
    uint32_t updateSubmitPosition() { return submit_position.exchange(0, std::memory_order_release); }
    //比较并设置存储资源位置（多线程竞争）
    bool setSubmitPosition(uint32_t pos) { return submit_position.compare_exchange_weak(pos, pos + 1, std::memory_order_release, std::memory_order_relaxed); }

    //获取存储资源缓存
    TimeInfo* getTimeInfo() const { return time_info; }
    //设置存储资源缓存
    void setTimeInfo(TimeInfo *info) { time_info = info; }
private:
    std::atomic<uint32_t> submit_position;  //提交资源的位置
    std::atomic<uint32_t> use_sequence;     //存储资源的序号
    TimeInfo *time_info;                    //单个序号存储提交资源缓存
};

class TimeManager : public BasicManager{
public:
    using BasicManager::BasicManager;
    ~TimeManager() override;

    void onUpdateTime(uint32_t) override;
    bool onSubmitBuffer(MediaData*, uint32_t) override;

    //减少存储资源数量（主Socket线程）
    void setNeedSubmitNumberReduce() { need_submit_number--; }
    //增加存储资源数量（主Socket线程）
    void setNeedSubmitNumberIncrease() { need_submit_number++; }
    void onReuseManager(uint32_t, uint32_t) override;
    void onUnuseManager() override;
protected:
    void onClearBufferRange() override;
    void onUpdateBufferRange(uint32_t) override;
private:
    static char* createBuffer(TimeLayer*, uint32_t);
    static void destroyBuffer(TimeLayer*, char*, uint32_t);
    static void transferTimeBuffer(uint32_t, uint32_t, uint32_t, const std::function<void(uint32_t, uint32_t)>&, const std::function<void(uint32_t, uint32_t)>&);
    static void unitTimeInfo(TimeLayer*, TimeInfo*);

    void initTimeBuffer(uint32_t);
    void initTimeBufferToCreate(TimeBuffer*, uint32_t);
    void initTimeBufferToPosition(TimeBuffer*, uint32_t);
    void onTransferTimeBufferWithDestroy(TimeBuffer*, uint32_t);
    void onNeedSubmitNumber(uint32_t number) { need_submit_number = number; }

    volatile uint32_t need_submit_number;   //需要提交的资源数量（远程端数量）

    std::atomic<uint32_t> first_sequence;   //起始提交序号
    std::atomic<uint32_t> last_sequence;    //结束提交序号（已处理的序号）

    Door buffer_door;                       //时间门（防止连续读锁而阻止写锁的获取）
    std::shared_timed_mutex buffer_mutex;   //时间读写锁

    TimeBuffer *time_buffer;                //根据延迟帧存储提交资源缓存数组
};


#endif //TEXTGDB_TIMEMANAGER_H
