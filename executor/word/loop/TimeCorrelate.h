//
// Created by abc on 20-4-17.
//

#ifndef UNTITLED5_TIMECORRELATE_H
#define UNTITLED5_TIMECORRELATE_H

#include "TimeInterval.h"

#define TIME_INTERVAL_TYPE_MICROSECOND  1
#define TIME_INTERVAL_TYPE_MILLISECOND  2
#define TIME_INTERVAL_TYPE_SECOND       4

//#define correlate_time_point (loop_thread->currentTimePoint())
#define correlate_time_point (LoopWordThread::select_time_point)

/**
 * 任务触发异常：说明任务已经处于触发状态,需要执行任务的触发动作
 */
class task_trigger_error : public std::exception {
public:
    using exception::exception;
    ~task_trigger_error() override = default;

    const char* what() const noexcept override { return nullptr; }
};

/**
 * 时间关联器基类
 *  管理存储任务的数据结构
 *  关联LoopThread线程
 */
class CorrelateBase{
    friend class TimeInterval;
public:
    explicit CorrelateBase(LoopWordThread *lthread) : loop_thread(lthread), bucket_link() {};
    virtual ~CorrelateBase() = default;

    CorrelateBase(const CorrelateBase&) = default;
    CorrelateBase(CorrelateBase &&base) noexcept : loop_thread(base.loop_thread), bucket_link(std::move(base.bucket_link)) {
        base.loop_thread = nullptr;
    }
    CorrelateBase& operator=(const CorrelateBase&) = default;
    CorrelateBase& operator=(CorrelateBase &&base) noexcept {
        loop_thread = base.loop_thread; bucket_link = std::move(base.bucket_link);
        base.loop_thread = nullptr; return *this;
    }

    /*
     * 任务触发时间是否在时间间隔里
     */
    bool onContainStatic(const double &contain_time, uint32_t contain_start, uint32_t contain_end) {
        return (!contain_start) ? ((contain_time > contain_start) && (contain_time < contain_end))
                                : ((contain_time >= contain_start) && (contain_time < contain_end));
    }

    virtual bool onContain(TaskInfo*, uint32_t, uint32_t) throw(std::logic_error) = 0;

    /*
     * 获取所有任务里的最小触发时间
     */
    virtual int64_t minTime() throw(task_trigger_error) = 0;
    /*
     * 任务的最小触发时间
     */
    virtual int64_t minTime(TaskInfo*) throw(task_trigger_error) = 0;
    /*
     * 插入任务
     */
    virtual void insertTimeTask(std::shared_ptr<LoopExecutorTask>) throw(std::bad_alloc) = 0;
    /*
     * 更新任务时间
     */
    virtual void updateTimeTask() = 0;
    /*
     * 直接触发任务,非更新数据结构
     */
    virtual void timeTaskUpdate(TaskInfo*) = 0;

    virtual void clearAll() = 0;
    virtual void ergodicTimeInfo() = 0;
protected:
    /*
     * 时间间隔里的任务最小触发时间
     */
    virtual TaskInfo* onMinTime() = 0;
    virtual void onInsert(TaskInfo*) = 0;

    virtual void onUpdate() = 0;
    /*
     * 任务向上个时间间隔递进
     */
    virtual void onSkipLevel(TaskInfo*) = 0;
    /*
     * 任务触发完成,复位原始时间间隔
     */
    virtual void onResetLevel(TaskInfo*) = 0;

    TaskInfo* pushTask(std::shared_ptr<LoopExecutorTask>&&) throw(std::bad_alloc);

    void onTimeComplete(TaskInfo*);
    void onTimeTrigger(TaskInfo*);
    bool onTimeTrigger0(std::shared_ptr<LoopExecutorTask>&);
    void onClear(TaskInfo*);

    /*
     * 根据任务信息获取task
     */
    std::shared_ptr<LoopExecutorTask> getTaskTime(TaskInfo *info) { return info->time_task; }
    /*
     * 根据位置信息获取task
     */
    std::shared_ptr<LoopExecutorTask> getTaskTime(std::pair<int, int> pos) { return bucket_link.get(pos); }
    std::shared_ptr<LoopExecutorTask> getTaskTime(int bpos, int tpos) { return bucket_link.get(bpos, tpos); }

    /*
     * 格式化指定任务的触发时间
     *  获取微秒级触发时间
     *  将微秒时间转为对应时间级并更改时间类型
     */
    template <int den> double initContainTime(TaskInfo *info, const interval_time_point &new_time){
        return std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1, den>>>(
                                           std::chrono::microseconds(getTaskTime(info)->triggerTime(new_time))).count();
    }

    /*
     * 格式化指定任务的触发时间
     *  获取微秒级的触发时间
     *  更改触发时间类型
     */
    double initContainTime(TaskInfo*);

    static void increaseErgodic(int start, int end, const std::function<bool(int)>&, const std::function<void()>&);
    static void reduceErgodic(int start, int end, const std::function<bool(int)>&, const std::function<void()>&);
private:
    /*
     * 根据任务信息获取触发时间
     */
    int64_t triggerTime(TaskInfo *info, const interval_time_point &new_time) {
        return getTaskTime(info)->triggerTime(new_time);
    }
//    SectionLink section_link;       //二叉堆信息链表
protected:
    LoopWordThread *loop_thread;    //关联线程
    BucketLink bucket_link;         //桶链表
};

/**
 * 微妙级时间间隔
 * [0-----500)[500-----1000)
 */
class MicrosecondSection : public CorrelateBase {
#define CORRELATE_MICROSECOND_INTERVAL_SIZE     2
public:
    explicit MicrosecondSection(LoopWordThread*);
    ~MicrosecondSection() override = default;

    MicrosecondSection(const MicrosecondSection &microsecond) : CorrelateBase(microsecond) { initInterval(); }
    MicrosecondSection(MicrosecondSection &&microsecond) noexcept : CorrelateBase(std::move(microsecond)) { initInterval(); }
    MicrosecondSection& operator=(const MicrosecondSection&) = default;
    MicrosecondSection& operator=(MicrosecondSection&&) noexcept = default;

    bool onContain(TaskInfo*, uint32_t, uint32_t) throw(std::logic_error) override;
    void ergodicTimeInfo() override;
protected:
    void clearInterval();

    TaskInfo* onMinTime() override;
    void onInsert(TaskInfo*) override;
    void onUpdate() override;
    void onSkipLevel(TaskInfo*) override;
    void onResetLevel(TaskInfo*) override;

    virtual void onResetMicrosecond(TaskInfo*) = 0;
private:
    void initInterval();
    TimeInterval microsecond_interval[CORRELATE_MICROSECOND_INTERVAL_SIZE]; //时间间隔数组
};

/**
 * 毫秒级时间间隔
 * [1-----200)[200-----500)[500-----1000)
 */
class MillisecondSection : public MicrosecondSection {
#define CORRELATE_MILLISECOND_INTERVAL_SIZE     3
#define CORRELATE_MILLISECOND_DEN_VALUE         1000
public:
    explicit MillisecondSection(LoopWordThread*);
    ~MillisecondSection() override = default;

    MillisecondSection(const MillisecondSection &millsecond) : MicrosecondSection(millsecond) { initInterval(); }
    MillisecondSection(MillisecondSection &&millsecond) noexcept : MicrosecondSection(std::move(millsecond)) { initInterval(); }
    MillisecondSection& operator=(const MillisecondSection&) = default;
    MillisecondSection& operator=(MillisecondSection &&) noexcept = default;

    bool onContain(TaskInfo*, uint32_t, uint32_t) throw(std::logic_error) override;
    void ergodicTimeInfo() override;

//    virtual bool onContain(SectionInfo*, const interval_time_point&, uint32_t, uint32_t) throw(std::logic_error);
protected:
    void clearInterval();

    TaskInfo* onMinTime() override;
    void onInsert(TaskInfo*) override;
    void onUpdate() override;
    void onSkipLevel(TaskInfo*) override;
    void onResetLevel(TaskInfo*) override;

    void onResetMicrosecond(TaskInfo*) override;

    virtual void onResetMillisecond(TaskInfo*) = 0;
private:
    void initInterval();
    static int millisecond_init_value[CORRELATE_MILLISECOND_INTERVAL_SIZE + 1]; //微秒时间间隔数值
    TimeInterval millisecond_interval[CORRELATE_MILLISECOND_INTERVAL_SIZE];     //微秒时间间隔数组
};

/**
 * 秒级时间间隔
 * [1-----5)[5----15)[15-----30)[30----60)[60-----无限大(-1)
 */
class SecondSection : public MillisecondSection {
#define CORRELATE_SECOND_INTERVAL_SIZE          5
#define CORRELATE_SECOND_DEN_VALUE              (1000 * 1000)
public:
    explicit SecondSection(LoopWordThread*);
    ~SecondSection() override = default;

    SecondSection(const SecondSection &second) : MillisecondSection(second) { initInterval(); }
    SecondSection(SecondSection &&second) noexcept : MillisecondSection(std::move(second)) { initInterval(); }
    SecondSection& operator=(const SecondSection&) = default;
    SecondSection& operator=(SecondSection&&) noexcept = default;

    bool onContain(TaskInfo*, uint32_t, uint32_t) throw(std::logic_error) override;
    void ergodicTimeInfo() override;
protected:
    void clearInterval();

    TaskInfo* onMinTime() override;
    void onInsert(TaskInfo*) override;
    void onUpdate() override;
    void onSkipLevel(TaskInfo*) override;
    void onResetLevel(TaskInfo*) override;
    void onResetMillisecond(TaskInfo*) override;
private:
    void initInterval();
    static int second_init_value[CORRELATE_SECOND_INTERVAL_SIZE + 1];   //秒时间间隔数值
    TimeInterval second_interval[CORRELATE_SECOND_INTERVAL_SIZE];       //秒时间间隔数组
};

/**
 * 时间关联器
 */
class TimeCorrelate : public SecondSection{
public:
    using SecondSection::SecondSection;
    ~TimeCorrelate() override = default;

    TimeCorrelate(const TimeCorrelate&) = default;
    TimeCorrelate(TimeCorrelate&&) noexcept = default;
    TimeCorrelate& operator=(const TimeCorrelate&) = default;
    TimeCorrelate& operator=(TimeCorrelate&&) noexcept = default;

    int64_t minTime() throw(task_trigger_error) override;
    int64_t minTime(TaskInfo*) throw(task_trigger_error) override;

    void insertTimeTask(std::shared_ptr<LoopExecutorTask>) throw(std::bad_alloc) override;
    void updateTimeTask() override;
    void timeTaskUpdate(TaskInfo*) override;
    void clearAll() override;
private:
    void onResetLevel(TaskInfo*) override;
};

#endif //UNTITLED5_TIMECORRELATE_H
