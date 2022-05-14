//
// Created by abc on 20-5-10.
//

#ifndef UNTITLED5_LOOPWORDTASK_H
#define UNTITLED5_LOOPWORDTASK_H

#include "ExecutorTask.h"

#define LOOP_EXECUTOR_TASK_EXECUTOR_OPERATOR_VALUE  1
#define LOOP_EXECUTOR_TASK_NOTE_SIZE                1

/**
 *
 * LoopExecutorTask的生命周期只有两个阶段
 * 阶段一:构造初始化(挂载LoopWordThread之前阶段),只有TimeInfo和FuncInfo信息有效,能够调用拷贝/移动构造函数、赋值函数
 * 阶段二:已经申请内存(挂载LoopWordThread之后、销毁之前),全部数据有效,不能调用拷贝/移动构造函数、赋值函数
 * 销毁:LoopWordThread卸载任务及LoopExecutorNote和LoopTimeOutNote销毁之后
 */
class LoopExecutorTask final : public ExecutorTask{
    friend class ExecutorNote;
    static LoopBaseNote* createExecutorNote();
    static LoopBaseNote* createTimeOutNote();
    static void destroyExecutorNote(ExecutorNote*);
    static void destroyTimeOutNote(ExecutorNote*);

    /*
     * Loop任务的时间信息（毫秒级）
     */
    struct TimeInfo{
        uint32_t ntrigger_count;                                //任务触发数量（包含超时触发）
        uint32_t ctrigger_size;                                 //已经触发的次数
        int64_t max_time;                                       //任务的最大时间
        int64_t interval_time;                                  //触发时间的间隔时间
        int64_t interval_time_increase;                         //间隔时间的增长值(暂时支持递增(整数),不支持递减(负数))

        std::chrono::steady_clock::time_point generate_time;    //生成时间点
    };

    /*
     * Loop任务的函数信息
     */
    struct FuncInfo{
        std::function<void()> trigger_func; //触发函数
        std::function<void()> timeout_func; //超时函数
    };

    /*
     * Loop任务的执行信息
     */
    struct ExecutorInfo{
        ExecutorInfo() : is_timeout(false), is_cancel(false), fail_size(0), complete_size(0){}
        ~ExecutorInfo() = default;

        ExecutorInfo(const ExecutorInfo &info) noexcept : is_timeout(info.is_timeout), is_cancel(info.is_cancel),
                                            fail_size(info.fail_size.load()), complete_size(info.complete_size.load()){}
        ExecutorInfo(ExecutorInfo &&info) noexcept : is_timeout(info.is_timeout), is_cancel(info.is_cancel),
                                            fail_size(info.fail_size.load()), complete_size(info.complete_size.load()){}

        ExecutorInfo& operator=(const ExecutorInfo &info) noexcept {
            is_timeout = info.is_timeout; is_cancel = info.is_cancel;
            fail_size.store(info.fail_size.load()); complete_size.store(info.complete_size.load());
            return *this;
        }
        ExecutorInfo& operator=(ExecutorInfo &&info) noexcept {
            is_timeout = info.is_timeout; is_cancel = info.is_cancel;
            fail_size.store(info.fail_size.load()); complete_size.store(info.complete_size.load());
            info.is_timeout = false; info.is_cancel = false; info.fail_size.store(0); info.complete_size.store(0);
            return *this;
        }

        bool is_timeout;                //是否超时
        bool is_cancel;                 //是否取消
        std::atomic_uint fail_size;     //失败次数
        std::atomic_uint complete_size; //完成次数
    };
public:
    LoopExecutorTask() : ExecutorTask(), holder(nullptr), executor_func(nullptr), cancel_func(nullptr),
      timeout_func(nullptr), correlate_thread(nullptr), loop_thread(nullptr), time_info(), func_info(), executor_info(){}
    explicit LoopExecutorTask(uint64_t interval_time) : ExecutorTask(), holder(nullptr), executor_func(nullptr),
                                                        cancel_func(nullptr), timeout_func(nullptr), correlate_thread(nullptr),
                                                        loop_thread(nullptr), time_info(), func_info(), executor_info(){
        setTaskTimeInfo(1, interval_time, 0);
    }
    ~LoopExecutorTask() override {
        if(holder) { holder->onReduce(); }
        if(isCancel() && cancel_func) { (*cancel_func)(false); }
    }
    LoopExecutorTask(const LoopExecutorTask &task) noexcept : time_info(task.time_info), func_info(task.func_info), executor_info(),
                                                     holder(nullptr), executor_func(nullptr), cancel_func(nullptr),
                                                     timeout_func(nullptr), correlate_thread(nullptr), loop_thread(nullptr) {}
    LoopExecutorTask(LoopExecutorTask &&task) noexcept : time_info(task.time_info), func_info(std::move(task.func_info)),
                                                executor_info(), holder(nullptr), executor_func(nullptr), cancel_func(nullptr), timeout_func(nullptr),
                                                correlate_thread(nullptr), loop_thread(nullptr) {}
    LoopExecutorTask& operator=(const LoopExecutorTask &task) noexcept {
        time_info = task.time_info; func_info = task.func_info; return *this;
    }
    LoopExecutorTask& operator=(LoopExecutorTask &&task) noexcept {
        time_info = task.time_info; func_info = std::move(task.func_info); return *this;
    }

    //返回LoopExecutorTask占有的字节大小,用于构造该类
    uint32_t classSize() const override { return sizeof(LoopExecutorTask); }
    //返回Loop任务类型
    TaskType type() const override { return TASK_TYPE_LOOP; }
    std::shared_ptr<ExecutorNote> executor() override;

    void setTaskTimeInfo(uint32_t, int64_t, int64_t);
    //设置任务提交给LoopThread的时间点
    void setTaskGenerateTime(const std::chrono::steady_clock::time_point &generate_time) { time_info.generate_time = generate_time; }
    //设置触发函数
    void setTriggerFunc(const std::function<void()> &func) { func_info.trigger_func = func; }
    //设置超时函数
    void setTimeoutFunc(const std::function<void()> &func) { func_info.timeout_func = func; }

    void setOnCallBackFunc(std::function<void(bool)> *efunc_, std::function<void(bool)> *cfunc_, std::function<void()> *tfunc_) {
        executor_func = efunc_; cancel_func = cfunc_; timeout_func = tfunc_;
    }

    //返回该任务触发的次数（包含超时函数）
    int getNeedTriggerSize() const { return time_info.ntrigger_count; }

    //返回任务是否取消
    bool isCancel() const { return (holder ? holder->isCancel() : false); }
    //返回任务是否超时
    bool isTimeout(const std::chrono::steady_clock::time_point &new_time) const { return ((time_info.max_time <= 0) ? false : (currentTime(new_time) >= time_info.max_time)); }
    int64_t triggerTime(const std::chrono::steady_clock::time_point&) const;
    int64_t currentTime(const std::chrono::steady_clock::time_point&) const;
    std::shared_ptr<LoopBaseNote> onTimeTrigger(const std::chrono::steady_clock::time_point&, const std::function<void(bool)>&);

    void executorTask();
    void executorTimeOut();
    void executorFail();
    void executorNoteComplete();
    //关联任务持有者
    void correlateHolder(HolderInterior *holder_interior) { holder = holder_interior; }
    void correlateThread(BasicThread*);
    //返回任务关联的线程
    BasicThread* correlateThread() const { return correlate_thread; }
private:
    static int64_t countMaxTime(uint32_t, int64_t, int64_t);
    static std::allocator<LoopExecutorNote> executor_alloc; //触发任务Note构造器
    static std::allocator<LoopTimeOutNote>  timeout_alloc;  //超时任务Note构造器

    std::function<void(bool)> *executor_func;
    std::function<void(bool)> *cancel_func;
    std::function<void()>     *timeout_func;

    /*
     * 任务持有者
     *   是否需要任务持有者？（Loop任务根据运行触发次数后自动销毁,是否需要取消？）
     *   需要：因为需要取消或删除定时器（如：加入定时器收到确定或正常数据后取消定时器）
     */
    HolderInterior *holder;

    BasicThread *correlate_thread;                              //Loop任务关联的Executor线程
    LoopWordThread *loop_thread;                                //Loop任务关联的Loop线程
    TimeInfo time_info;                                         //时间信息
    FuncInfo func_info;                                         //执行、超时函数
    ExecutorInfo executor_info;                                 //执行信息
};

/**
 * Loop任务执行Note基类
 */
struct LoopBaseNote : public ExecutorNote {
    LoopBaseNote() : loop_task(nullptr) {}
    ~LoopBaseNote() override { if(loop_task != nullptr) { loop_task->executorNoteComplete(); }}

    LoopBaseNote(const LoopBaseNote &note) noexcept = default;
    LoopBaseNote(LoopBaseNote &&note) noexcept : loop_task(std::move(note.loop_task)) {}

    LoopBaseNote& operator=(const LoopBaseNote &note) noexcept = default;
    LoopBaseNote& operator=(LoopBaseNote &&note) noexcept {
        loop_task = std::move(note.loop_task); return *this;
    }

    //无法添加队列时是否执行？
    virtual bool isExecutor() const  = 0;
    //不实现
    void cancel() override {}
    //关联Loop任务
    virtual void correlateLoopTask(const std::shared_ptr<LoopExecutorTask> &loop) { loop_task = loop; }
protected:
    std::shared_ptr<LoopExecutorTask> loop_task;    //指向Loop任务的指针
};

/**
 * Loop任务触发Note类
 */
struct LoopExecutorNote final: public LoopBaseNote{
    uint32_t classSize() const override  { return sizeof(LoopExecutorNote); }
    //返回任务是否取消
    bool isCancel() const override { return loop_task->isCancel(); }
    bool isExecutor() const override { return false; }
    //执行触发Note
    void executor() override { loop_task->executorTask(); }
    //触发Note执行失败
    void fail() override { loop_task->executorFail(); }
    //返回是否需要关联Executor线程
    bool needCorrelate() const override { return true; }
    void setCorrelateThread(BasicThread *thread) override { loop_task->correlateThread(thread); }
    //返回关联的Executor线程
    BasicThread* getCorrelateThread() const override { return loop_task->correlateThread(); }
};

/**
 * Loop任务超时Note类
 */
struct LoopTimeOutNote final : public LoopBaseNote{
    uint32_t classSize() const override { return sizeof(LoopTimeOutNote); }
    bool isCancel() const override { return loop_task->isCancel(); }
    bool isExecutor() const override { return true; }
    void executor() override { loop_task->executorTimeOut(); }
    void fail() override { }
    bool needCorrelate() const override { return true; }
    void setCorrelateThread(BasicThread *thread) override { loop_task->correlateThread(thread); }
    BasicThread* getCorrelateThread() const override { return loop_task->correlateThread(); }
};

/**
 * Loop任务即时Note类（只执行一次）
 */
struct LoopImmediatelyNote : public LoopBaseNote{
    LoopImmediatelyNote() : LoopBaseNote(), executor_func(nullptr), cancel_func(nullptr), holder(nullptr) {}
    ~LoopImmediatelyNote() override { if(holder){ holder->onReduce(); } }

    //已经关联了HolderInterior不会调用拷贝、移动构造函数和拷贝、移动赋值函数(初始化会调用)
    LoopImmediatelyNote(const LoopImmediatelyNote &note) noexcept : LoopBaseNote(note), executor_func(note.executor_func),
                                                                       cancel_func(note.cancel_func), holder(nullptr) {}
    LoopImmediatelyNote(LoopImmediatelyNote &&note) noexcept : LoopBaseNote(std::move(note)), executor_func(note.executor_func),
                                                        cancel_func(note.cancel_func), holder(nullptr) {
        note.executor_func = nullptr; note.cancel_func = nullptr;
    }

    LoopImmediatelyNote& operator=(const LoopImmediatelyNote &note) noexcept {
        if(holder) { return *this; }
        LoopBaseNote::operator=(note); executor_func = note.executor_func; cancel_func = note.executor_func;
        return *this;
    }
    LoopImmediatelyNote& operator=(LoopImmediatelyNote &&note) noexcept {
        if(holder) { return *this; }
        LoopBaseNote::operator=(std::move(note)); executor_func = note.executor_func; cancel_func = note.executor_func;
        note.executor_func = nullptr; note.cancel_func = nullptr;
        return *this;
    }

    uint32_t classSize() const override { return sizeof(LoopImmediatelyNote); }
    bool isCancel() const override { return holder->isCancel(); }
    bool isExecutor() const override { return true; }
    void executor() override { holder->executor(); if(executor_func){ (*executor_func)(true); } }
    void cancel() override { if(cancel_func) { (*cancel_func)(true); } }
    void fail() override { }
    bool needCorrelate() const override { return false; }
    void setCorrelateThread(BasicThread *thread) override {}
    BasicThread* getCorrelateThread() const override { return nullptr; }

    void correlateHolder(HolderInterior *holder_interior) { holder = holder_interior; }
    void setOnCallBackFunc(std::function<void(bool)> *efunc, std::function<void(bool)> *cfunc) { executor_func = efunc; cancel_func = cfunc;}
private:
    std::function<void(bool)> *executor_func;
    std::function<void(bool)> *cancel_func;
    //任务持有者（可能与LoopBaseNote的loop_task有逻辑冲突,但只取LoopBaseNote的loop_task的时间信息）
    HolderInterior *holder;
};

#endif //UNTITLED5_LOOPWORDTASK_H
