//
// Created by abc on 19-6-24.
//

#ifndef UNTITLED8_EXECUTORTASK_H
#define UNTITLED8_EXECUTORTASK_H

#include <future>
#include <chrono>
#include <cstring>
#include <iostream>

class BasicThread;
//class MeetingAddressNote;
//class SimpleTaskHolder;
//class BasicExecutor;
class SocketWordThread;
class LoopWordThread;
class HolderInterior;
class ReleaseTaskHolder;

//
//struct MsgHdr;
struct ExecutorNote;
struct LoopBaseNote;
struct LoopExecutorNote;
struct LoopTimeOutNote;
struct SocketExecutorNote;

extern ReleaseTaskHolder release_holder;

struct cancel_error : public std::exception{
public:
    using exception::exception;
    ~cancel_error() override = default;

    const char* what() const noexcept override { return nullptr; }
};

/**
 * 任务持有者基类,当提交任务给线程池时会返回该变量
 * 用途：1、获取任务结果,2、取消任务
 * @tparam T    结果类型
 */
template <typename T> class TaskHolder{
public:
    TaskHolder() : future_(), holder_interior(nullptr) {}
    explicit TaskHolder(HolderInterior*, std::future<T>&&);
    virtual ~TaskHolder();

//    TaskHolder(const TaskHolder &holder) = delete;
//    TaskHolder& operator=(const TaskHolder&) = delete;
    TaskHolder(TaskHolder &&holder) noexcept : future_(std::move(holder.future_)), holder_interior(holder.holder_interior) { holder.holder_interior = nullptr; }
    TaskHolder& operator=(TaskHolder &&holder) noexcept;

    T get(const std::chrono::microseconds&) throw(cancel_error, std::logic_error);
    T get() throw(cancel_error, std::logic_error) {
        return get(std::chrono::microseconds(0));
    }
    T get(const std::chrono::seconds &time_out) throw(cancel_error, std::logic_error) {
        return get(std::chrono::microseconds(std::chrono::duration_cast<std::chrono::microseconds>(time_out)));
    }
    T get(const std::chrono::milliseconds &time_out) throw(cancel_error, std::logic_error) {
        return get(std::chrono::microseconds(std::chrono::duration_cast<std::chrono::microseconds>(time_out)));
    }
    void cancel();
private:
    std::future<T> future_;
    HolderInterior *holder_interior;
};

/**
 * 持有内部基类类
 */
class HolderInterior{
public:
    HolderInterior() : task_cancel_point(false), release_flag(0) {}
    virtual ~HolderInterior() = default;

//    HolderInterior(const HolderInterior&) = delete;
//    HolderInterior& operator=(const HolderInterior&) = delete;
    HolderInterior(HolderInterior&&) = delete;
    HolderInterior& operator=(HolderInterior&&) = delete;

    virtual void executor() = 0;
    virtual void release() = 0;

    void cancel() { task_cancel_point.store(true, std::memory_order_release); }
    bool isCancel() const { return task_cancel_point.load(std::memory_order_consume); }
    HolderInterior* onIncrease() { release_flag.fetch_add(1, std::memory_order_release); return this; }
    void onReduce();
private:
    std::atomic_bool task_cancel_point; //任务取消标志
    std::atomic_uint release_flag;      //持有者释放标记
};

/**
 * 持有内部类,当任务提交给线程池时会构造该变量,为提交任务线程返回任务结果
 * @tparam T        结果类型
 * @tparam Func     任务函数
 */
template <typename T, typename Func> class TaskHolderInterior final : public HolderInterior{
    friend class TaskHolder<T>;
public:
    explicit TaskHolderInterior(const Func &f) : HolderInterior(), promise_(), func_(f) {}
    ~TaskHolderInterior() override = default;

    /**
     * 执行任务,并返回任务结果
     */
    void executor() override {
        //判断是否取消
        if(!isCancel()){
            try{
                //执行任务函数并返回结构
                promise_.set_value(func_());
            }catch (std::exception&){
                //执行任务失败,返回异常
                promise_.set_exception(std::current_exception());
            }
        }
    }

    void release() override { releaseTaskHolder(this); }

    /**
     * 根据提交任务函数生成持有者内部类
     * @param f 函数
     * @return
     */
    static TaskHolderInterior<T, Func>* generateTaskHolder(Func f){
        return (new TaskHolderInterior<T, Func>(f));
    }
    /**
     * 释放持有者内部类
     * @param interior
     */
    static void releaseTaskHolder(TaskHolderInterior<T, Func> *interior){
        delete interior;
    }
    /**
     * 根据持有者内部类构造初始化任务持有者
     * @param holder_interior
     * @return
     */
    static TaskHolder<T> initTaskHolder(TaskHolderInterior<T, Func> *holder_interior){
        return TaskHolder<T>(holder_interior->onIncrease(), holder_interior->promise_.get_future());
    }
private:
    std::promise<T> promise_;
    Func func_;                 //函数
};

/**
 * 持有者内部类（特化版）
 * @tparam void
 * @tparam Func 任务函数
 */
template <typename Func> class TaskHolderInterior<void, Func> final : public HolderInterior{
    friend class TaskHolder<void>;
public:
    TaskHolderInterior(const Func &f) : HolderInterior(), promise_(), func_(f) {}
    ~TaskHolderInterior() override = default;

    void executor() override {
        if(!isCancel()){
            try {
                func_();
                promise_.set_value();
            }catch (std::exception&) {
                promise_.set_exception(std::current_exception());
            }
        }
    }
    void release() override { releaseTaskHolder(this); }

    static TaskHolderInterior<void, Func>* generateTaskHolder(Func f) {
        return (new TaskHolderInterior<void, Func>(f));
    }
    static void releaseTaskHolder(TaskHolderInterior<void, Func> *interior){
        delete interior;
    }
    static TaskHolder<void> initTaskHolder(TaskHolderInterior<void, Func> *holder_interior){
        return TaskHolder<void>(holder_interior->onIncrease(), holder_interior->promise_.get_future());
    }
private:
    std::promise<void> promise_;
    Func func_;
};

template <> class TaskHolderInterior<void, void> final : public HolderInterior{
    friend class TaskHolder<void>;
public:
    TaskHolderInterior() : HolderInterior(), promise_() {}
    ~TaskHolderInterior() override = default;

    void executor() override { if(!isCancel()) { promise_.set_value(); } }
    void release() override { releaseTaskHolder(this); }

    static TaskHolderInterior<void, void>* generateTaskHolder() {
        return (new (std::nothrow) TaskHolderInterior<void, void>());
    }
    static void releaseTaskHolder(TaskHolderInterior<void, void> *holder_interior){
        delete holder_interior;
    }
    static TaskHolder<void> initTaskHolder(TaskHolderInterior<void, void> *holder_interior){
        return TaskHolder<void>(holder_interior->onIncrease(), holder_interior->promise_.get_future());
    }
private:
    std::promise<void> promise_;
};

/**
 * 持有者内部类（LoopExecutorTask专用版）（由TaskHolderInterior<void,void>顶替）
 */
class TaskHolderInteriorSpecial final : public HolderInterior{
    friend class TaskHolder<void>;
public:
    TaskHolderInteriorSpecial() : HolderInterior(), promise_(){}
    ~TaskHolderInteriorSpecial() override = default;

    void executor() override { if(!isCancel()){ promise_.set_value(); } }
    void release() override { releaseTaskHolder(this); }

    static TaskHolderInteriorSpecial* generateTaskHolder() {
        return (new (std::nothrow) TaskHolderInteriorSpecial());
    }
    static void releaseTaskHolder(TaskHolderInteriorSpecial *holder_interior){
        delete holder_interior;
    }
    static TaskHolder<void> initTaskHolder(TaskHolderInteriorSpecial *holder_interior){
        return TaskHolder<void>(holder_interior->onIncrease(), holder_interior->promise_.get_future());
    }
private:
    std::promise<void> promise_;
};

template <typename T> TaskHolder<T>::~TaskHolder() { if(holder_interior) { holder_interior->onReduce(); } }

template <typename T> TaskHolder<T>::TaskHolder(HolderInterior *interior, std::future<T> &&future)
        : holder_interior(interior), future_(std::move(future)) {}

template <typename T> TaskHolder<T>& TaskHolder<T>::operator=(TaskHolder<T> &&holder) noexcept {
    if(holder_interior){ holder_interior->onReduce(); }
    future_ = std::move(holder.future_); holder_interior = holder.holder_interior->onIncrease();
    holder.holder_interior = nullptr; return *this;
}

/**
 * 获取任务结果
 * @tparam T        结果类型
 * @param time_out  超时时间
 * @return          任务结构
 */
template <typename T> T TaskHolder<T>::get(const std::chrono::microseconds &time_out) throw(cancel_error, std::logic_error) {
    //判断任务是否取消
    if(holder_interior->isCancel()){
        //生成取消异常并抛出
        throw cancel_error();
    }

    //判断任务是否有效
    if(!future_.valid()){
        //生成逻辑异常并抛出
        throw std::logic_error("void is not get!");
    }

    /*
     * 如果没有设置超时时间或在超时时间有结果,获取结果并返回
     * 否则抛出超时异常
     */
    if((time_out.count() <= 0) || (future_.wait_for(time_out) == std::future_status::ready)){
        return future_.get();
    }else{
        throw std::logic_error("timeout!");
    }
}

/**
 * 取消任务
 * @tparam T 结构类型
 */
template <typename T> void TaskHolder<T>::cancel() { holder_interior->cancel(); }

/**
 * 任务持有者释放类,用于std::shared_ptr调用持有者内部的释放函数
 */
class ReleaseTaskHolder final{
public:
    void operator()(HolderInterior *interior) { interior->release(); }
};

enum TaskType{
    TASK_TYPE_LOOP,         //Loop任务
    TASK_TYPE_SOCKET,       //Socket任务
};

/**
 * 任务基类
 */
class ExecutorTask{
public:
    ExecutorTask() = default;
    virtual ~ExecutorTask() = default;

    //类大小,用于构造类
    virtual uint32_t classSize() const = 0;
    //获取任务类型
    virtual TaskType type() const = 0;
    //生成任务的执行Note
    virtual std::shared_ptr<ExecutorNote> executor() = 0;
};

/**
 * 任务执行Note基类
 */
struct ExecutorNote{
    ExecutorNote() = default;
    virtual ~ExecutorNote() = default;

    //类大小,用于构造类
    virtual uint32_t classSize() const = 0;
    //判断任务类是否取消
    virtual bool isCancel() const = 0;
    //执行Note
    virtual void executor() = 0;
    //取消执行
    virtual void cancel() = 0;
    //执行Note失败
    virtual void fail() = 0;
    //执行Note是否需要关联线程
    virtual bool needCorrelate() const = 0;
    virtual void setCorrelateThread(BasicThread*) = 0;
    //获取执行Note的关联线程
    virtual BasicThread* getCorrelateThread() const = 0;
};

#endif //UNTITLED8_EXECUTORTASK_H
