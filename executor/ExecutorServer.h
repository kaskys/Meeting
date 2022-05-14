//
// Created by abc on 20-5-3.
//

#ifndef UNTITLED5_EXECUTORSERVER_H
#define UNTITLED5_EXECUTORSERVER_H

#include <type_traits>
#include "ThreadExecutor.h"

class ExecutorServer : public ThreadExecutor{
public:
    using ThrowFunc = std::function<void()>;
    using SubmitFunc = std::function<void()>;
    using CalcelFunc = std::function<void(bool)>;
    using TimeoutFunc = std::function<void()>;
    using ExecutorFunc = std::function<void(bool)>;
private:
    template <typename T, typename Func> class SubmitUtil final {
    public:
        //移动构造
        explicit SubmitUtil(Func func, const std::function<void(T*)> &call) : func_(std::move(func)), callback(call) {}
        ~SubmitUtil() = default;

        T* onSubmit(const SubmitFunc &submit_func, const ThrowFunc &throw_func) throw(std::logic_error){
            T *holder_interior = nullptr;

            try {
                holder_interior = T::generateTaskHolder(func_);
            } catch (std::bad_alloc &e){
                throw std::logic_error("ExecutorServer not Memory!");
            }

            try {
                callback(holder_interior);
                submit_func();
            } catch (std::logic_error &e){
                T::releaseTaskHolder(holder_interior);
                throw_func();
                throw;
            }

            return holder_interior;
        }
    private:
        typename std::remove_reference<Func>::type func_;
        std::function<void(T*)> callback;
    };

    template <typename T, typename Func> class SubmitUtil<T, const Func> final {
    public:
        //拷贝构造
        explicit SubmitUtil(const Func func, const std::function<void(T*)> &call) : func_(func), callback(call) {}
        ~SubmitUtil() = default;

        T* onSubmit(const SubmitFunc &submit_func, const ThrowFunc &throw_func) throw(std::logic_error){
            T *holder_interior = nullptr;

            try {
                holder_interior = T::generateTaskHolder(func_);
            } catch (std::bad_alloc &e){
                throw std::logic_error("ExecutorServer not Memory!");
            }

            try {
                callback(holder_interior);
                submit_func();
            } catch (std::logic_error &e){
                T::releaseTaskHolder(holder_interior);
                throw_func();
                throw;
            }

            return holder_interior;
        }
    private:
        typename std::remove_reference<Func>::type func_;
        std::function<void(T*)> callback;
    };

    template <typename T> class SubmitUtil<T, void> final{
    public:
        explicit SubmitUtil(const std::function<void(T*)> &func) : callback(func) {}
        ~SubmitUtil() = default;

        T* onSubmit(const SubmitFunc &submit_func, const ThrowFunc &throw_func) throw(std::logic_error){
            T *holder_interior = nullptr;

            try {
                holder_interior = T::generateTaskHolder();
            } catch (std::bad_alloc &e){
                throw std::logic_error("ExecutorServer not Memory!");
            }

            try {
                callback(holder_interior);
                submit_func();
            } catch (std::logic_error &e){
                T::releaseTaskHolder(holder_interior);
                throw_func();
                throw;
            }

            return holder_interior;
        }
    private:
        std::function<void(T*)> callback;
    };

public:
    explicit ExecutorServer(BasicLayer *layer, LaunchMode launch_mode = LAUNCH_IMMEDIATE) : ThreadExecutor(layer, launch_mode) {}
    ~ExecutorServer() override { shutDown(); }

    TaskHolder<void> submit(const LoopExecutorTask &task, ExecutorFunc *efunc_ = nullptr, CalcelFunc *cfunc_ = nullptr, TimeoutFunc *tfunc_ = nullptr) throw(std::logic_error) {
        using InsideFunc = void(*)(ExecutorServer*, const LoopExecutorTask&, HolderInterior*, ExecutorFunc*, CalcelFunc*, TimeoutFunc*);
        SubmitUtil<TaskHolderInterior<void, void>, void> submit_util{
                std::bind((InsideFunc)&ExecutorServer::submitInside, this, task, std::placeholders::_1, efunc_, cfunc_, tfunc_)};
        TaskHolderInterior<void, void> *holder_interior = submit0(submit_util,
                                                                  [&]() -> void {},
                                                                  [&]() -> void {});
        return TaskHolderInterior<void, void>::initTaskHolder(holder_interior);
    }

    template <typename T, typename Func> TaskHolder<T> submit(Func &&func_, ExecutorFunc *efunc_ = nullptr, CalcelFunc *cfunc_ = nullptr,
                                                              const SubmitFunc &submit_func = nullptr, const ThrowFunc &throw_func = nullptr) throw(std::logic_error) {
        using ImmediatelyFunc = void(*)(ExecutorServer*, HolderInterior*, ExecutorFunc*, CalcelFunc*);
        SubmitUtil<TaskHolderInterior<T, Func>, Func> submit_util{std::forward<Func>(func_),
                                                                  std::bind((ImmediatelyFunc)&ExecutorServer::submitImmediately, this, std::placeholders::_1, efunc_, cfunc_)};
        TaskHolderInterior<T, Func> *holder_interior = submit0(submit_util,
                                                               submit_func ? submit_func : [&]() -> void {},
                                                               throw_func ? throw_func : [&]() -> void {});
//        TaskHolderInterior<T, Func> *holder_interior = submit0(std::forward<Func>(func_),
//                                                               [&](TaskHolderInterior<T, Func> *interior) -> void {
//                                                                   submitImmediately(static_cast<HolderInterior*>(interior), efunc_, cfunc_);
//                                                               },
//                                                               submit_func ? submit_func : [&]() -> void {},
//                                                               throw_func ? throw_func : [&]() -> void {});
        return TaskHolderInterior<T, Func>::initTaskHolder(holder_interior);
    }

    template <typename T, typename Func> TaskHolder<T> submit(Func &&func_, const std::chrono::microseconds &time,
                                                              ExecutorFunc *efunc_ = nullptr, CalcelFunc *cfunc_ = nullptr,
                                                              const SubmitFunc &submit_func = nullptr, const ThrowFunc &throw_func = nullptr) throw(std::logic_error){
        using DelayFunc = void(*)(ExecutorServer*, HolderInterior*, const std::chrono::microseconds&, ExecutorFunc*, CalcelFunc*);
        SubmitUtil<TaskHolderInterior<T, Func>, Func> submit_util{std::forward<Func>(func_),
                                                                  std::bind((DelayFunc)&ExecutorServer::submitDelay, this, std::placeholders::_1, time, efunc_, cfunc_)};
        TaskHolderInterior<T, Func> *holder_interior = submit0(submit_util,
                                                               submit_func ? submit_func : [&]() -> void {},
                                                               throw_func ? throw_func : [&]() -> void {});
//        TaskHolderInterior<T, Func> *holder_interior = submit0(std::forward<Func>(func_),
//                                                               [&](TaskHolderInterior<T, Func> *interior) -> void {
//                                                                   submitDelay(static_cast<HolderInterior*>(interior), time, efunc_, cfunc_);
//                                                               },
//                                                               submit_func ? submit_func : [&]() -> void {},
//                                                               throw_func ? throw_func : [&]() -> void {});
        return TaskHolderInterior<T, Func>::initTaskHolder(holder_interior);
    };

    template <typename T, typename Func> TaskHolder<T> submit(Func &&func_, const std::chrono::milliseconds &time, ExecutorFunc *efunc_ = nullptr, CalcelFunc *cfunc_ = nullptr,
                                                              const SubmitFunc &submit_func = nullptr, const ThrowFunc &throw_func = nullptr) throw(std::logic_error){
        return submit<T, Func>(std::forward<Func>(func_), std::chrono::microseconds(std::chrono::duration_cast<std::chrono::microseconds>(time)), efunc_, cfunc_, submit_func, throw_func);
    };

    template <typename T, typename Func> TaskHolder<T> submit(Func &&func_, const std::chrono::seconds &time, ExecutorFunc *efunc_ = nullptr, CalcelFunc *cfunc_ = nullptr,
                                                              const SubmitFunc &submit_func = nullptr, const ThrowFunc &throw_func = nullptr) throw(std::logic_error){
        return submit<T, Func>(std::forward<Func>(func_), std::chrono::microseconds(std::chrono::duration_cast<std::chrono::microseconds>(time)), efunc_, cfunc_, submit_func, throw_func);
    };
private:
    void submitImmediately(HolderInterior*, ExecutorFunc*, CalcelFunc*) throw(std::logic_error);
    void submitDelay(HolderInterior*, const std::chrono::microseconds&, ExecutorFunc*, CalcelFunc*) throw(std::logic_error);
    void submitInside(const LoopExecutorTask &, HolderInterior*, ExecutorFunc*, CalcelFunc*, TimeoutFunc*) throw(std::logic_error);

    template <typename T, typename Func> T* submit0(SubmitUtil<T, Func> &submit_util, const SubmitFunc &submit_func, const ThrowFunc &throw_func) throw(std::logic_error) {
        return submit_util.onSubmit(submit_func, throw_func);
    };

//    template <typename T, typename Func> TaskHolderInterior<T, Func>* submit0(Func &&func_, const std::function<void(TaskHolderInterior<T, Func>*)> &callback,
//                                                                              const SubmitFunc &submit_func, const ThrowFunc &throw_func) throw(std::logic_error) {
//        TaskHolderInterior<T, Func> *holder_interior = nullptr;
//
//        try {
//            holder_interior = TaskHolderInterior<T, Func>::generateTaskHolder(std::forward<Func>(func_));
//        } catch (std::bad_alloc &e){
//            throw std::logic_error("ExecutorServer not Memory!");
//        }
//
//        try {
//            callback(holder_interior);
//            submit_func();
//        } catch (std::logic_error &e){
//            TaskHolderInterior<T, Func>::releaseTaskHolder(holder_interior);
//            throw_func();
//            throw;
//        }
//
//        return holder_interior;
//    };

    template <typename T, typename CallBack> void createSubmitData(T &&submit_data, const CallBack &call_back) throw(std::logic_error){
        if(canSubmit()) {
            using DataType = typename std::remove_const<typename std::remove_reference<T>::type>::type;
            DataType *executor_data = new (std::nothrow) DataType(std::forward<T>(submit_data));
            if(executor_data){
                call_back(executor_data);
            }else{
                throw std::logic_error("ExecutorServer not Memory!");
            }
        }else{
            throw std::logic_error("ExecutorServer not Running!");
        }
    }
    static void releaseExecutorNote(ExecutorNote *executor_note){
        delete(executor_note);
    }
    static void releaseExecutorTask(ExecutorTask *executor_task){
        delete(executor_task);
    }
};

#endif //UNTITLED5_EXECUTORSERVER_H
