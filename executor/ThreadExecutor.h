//
// Created by abc on 20-5-3.
//

#ifndef UNTITLED5_THREADEXECUTOR_H
#define UNTITLED5_THREADEXECUTOR_H

#include "BasicExecutor.h"

class BasicLayer;
class ThreadExecutor;

struct ThreadProxy{
    ThreadProxy() = default;
    virtual ~ThreadProxy() = default;

    virtual BasicThread* createThread(BasicExecutor*) const throw(std::bad_alloc) = 0;
    virtual void destroyThread(BasicThread*) const  = 0;
    virtual void onCreate(ThreadExecutor*, BasicThread*) const = 0;
    virtual ThreadType typeProxy() const = 0;
    virtual bool isProxy(const ThreadType &) const = 0;
    virtual BasicThread* onRequest(ThreadExecutor*) const = 0;
    virtual void onResponse(ThreadExecutor*, BasicThread*) const = 0;
    virtual void onRecovery(ThreadExecutor*, BasicThread*) const = 0;
};

struct ExecutorProxy final : public ThreadProxy{
    using ThreadProxy::ThreadProxy;
    ~ExecutorProxy() override = default;

    BasicThread* createThread(BasicExecutor *pool) const throw(std::bad_alloc) override { return new ExecutorThread(pool);}
    void destroyThread(BasicThread *thread) const override { delete(thread); }
    void onCreate(ThreadExecutor*, BasicThread*) const override;
    ThreadType typeProxy() const override { return THREAD_TYPE_EXECUTOR; }
    bool isProxy(const ThreadType &type) const override { return (type == typeProxy()); }
    BasicThread* onRequest(ThreadExecutor*) const override;
    void onResponse(ThreadExecutor*, BasicThread*) const override;
    void onRecovery(ThreadExecutor *pool, BasicThread *thread) const override;
};

struct ManagerProxy final : public ThreadProxy{
    using ThreadProxy::ThreadProxy;
    ~ManagerProxy() override = default;

    BasicThread* createThread(BasicExecutor*) const throw(std::bad_alloc) override { return new ManagerThread(); }
    void destroyThread(BasicThread *thread) const override { delete(thread); }
    void onCreate(ThreadExecutor*, BasicThread*) const override;
    ThreadType typeProxy() const override { return THREAD_TYPE_MANAGER; }
    bool isProxy(const ThreadType &type) const override { return (type == typeProxy()); }
    BasicThread* onRequest(ThreadExecutor*) const override {  return nullptr; }
    void onResponse(ThreadExecutor*, BasicThread*) const override {} //不需要处理
    void onRecovery(ThreadExecutor*, BasicThread*) const override {} //不需要处理
};

struct WordProxy : public ThreadProxy{
    using ThreadProxy::ThreadProxy;
    ~WordProxy() override = default;

    BasicThread* createThread(BasicExecutor*) const throw(std::bad_alloc) override { return new WordThread(); }
    void destroyThread(BasicThread *thread) const override { delete(thread); }
    void onCreate(ThreadExecutor*, BasicThread*) const override;
    BasicThread* onRequest(ThreadExecutor*) const override;
    void onResponse(ThreadExecutor*, BasicThread*) const override {} //不需要处理
    void onRecovery(ThreadExecutor *pool, BasicThread *thread) const override;
};

struct LoopProxy final : public WordProxy{
    using WordProxy::WordProxy;
    ~LoopProxy() override = default;

    BasicThread* createThread(BasicExecutor*) const throw(std::bad_alloc) override;
    void destroyThread(BasicThread*) const override;
    ThreadType typeProxy() const override { return THREAD_TYPE_LOOP; }
    bool isProxy(const ThreadType &type) const override { return (type == typeProxy()); }
};

struct SocketProxy final : public WordProxy{
    using WordProxy::WordProxy;
    ~SocketProxy() override = default;

    BasicThread* createThread(BasicExecutor*) const throw(std::bad_alloc) override;
    void destroyThread(BasicThread*) const override;
    ThreadType typeProxy() const override { return THREAD_TYPE_SOCKET; }
    bool isProxy(const ThreadType &type) const override { return (type == typeProxy()); }
};

struct DisplayProxy final : public WordProxy{
    using WordProxy::WordProxy;
    ~DisplayProxy() override = default;

    BasicThread* createThread(BasicExecutor*) const throw(std::bad_alloc) override;
    void destroyThread(BasicThread*) const override;
    ThreadType typeProxy() const override { return THREAD_TYPE_DISPLAY; }
    bool isProxy(const ThreadType &type) const override { return (type == typeProxy()); }
};

using thread_proxy = std::tuple<ManagerProxy, ExecutorProxy, LoopProxy, SocketProxy, DisplayProxy>;

class ThreadExecutor : public BasicExecutor{
friend struct ExecutorProxy;
friend struct ManagerProxy;
friend struct WordProxy;
    class ThreadProxyException final : public std::exception {
    public:
        ThreadProxyException(ThreadType src_type, ThreadType dsc_type) : exception(), exception_value("ThreadProxyException: type mismatch(") {
            exception_value.append(std::to_string(src_type)).append(":").append(std::to_string(dsc_type)).append(")");
        }
        ~ThreadProxyException() override = default;
        ThreadProxyException(const ThreadProxyException&) = default;
        ThreadProxyException(ThreadProxyException&&) = default;
        ThreadProxyException& operator=(const ThreadProxyException&) = default;
        ThreadProxyException& operator=(ThreadProxyException&&) = default;

        const char* what() const noexcept override { return exception_value.c_str(); }
    private:
        std::string exception_value;
    };

    template <typename Tuple, const size_t n = std::tuple_size<Tuple>::value> struct ErgodicProxy{
        static void ecreate(const Tuple &t, ThreadExecutor *pool, const std::function<bool(const ThreadProxy&)> &is_create){
            const ThreadProxy &proxy = std::get<n - 1>(t);
            ErgodicProxy<Tuple, n - 1>::ecreate(t, pool, is_create);

            if((is_create == nullptr) || (is_create(proxy))) {
                pool->onCreateThread(proxy);
            }
        }

        static void eclose(const Tuple &t, ThreadExecutor *pool){
            ErgodicProxy<Tuple, n - 1>::eclose(t, pool);
            pool->onCloseThread(std::get<n - 1>(t));
        }

        static const ThreadProxy& getProxy(const Tuple &t, const ThreadType &type) throw(ThreadProxyException) {
            const ThreadProxy &proxy = std::get<n - 1>(t);
            if(proxy.isProxy(type)){
                return proxy;
            }else{
                return ErgodicProxy<Tuple, n - 1>::getProxy(t, type);
            }
        }
    };

    template <typename Tuple> struct ErgodicProxy<Tuple, 1>{
        static void ecreate(const Tuple &t, ThreadExecutor *pool, const std::function<bool(const ThreadProxy&)> &is_create){
            const ThreadProxy &proxy = std::get<0>(t);
            if((is_create == nullptr) || (is_create(proxy))) {
                pool->onCreateThread(proxy);
            }
        }
        static void eclose(const Tuple &t, ThreadExecutor *pool){
            pool->onCloseThread(std::get<0>(t));
        }
        static const ThreadProxy& getProxy(const Tuple &t, const ThreadType &type) throw(ThreadProxyException) {
            const ThreadProxy &proxy = std::get<0>(t);
            if(proxy.isProxy(type)){
                return proxy;
            }else{
                throw ThreadProxyException(type, proxy.typeProxy());
            }
        }
    };

public:
    ThreadExecutor(BasicLayer*, LaunchMode launch_mode) throw(std::runtime_error);
    ~ThreadExecutor() override;

    BasicThread* requestThread(ThreadType) override;
    void responseThread(BasicThread*) override;
    void recoveryThread(BasicThread*) override;

    void onStartThread(std::shared_ptr<StartInstruct>&);
    void onStartComplete(std::shared_ptr<StartInstruct>&);
    void onReceiveTask(std::shared_ptr<ExecutorTask>&);
    void onTransferAddress(MeetingAddressNote*, uint32_t, uint32_t);
    uint32_t onTransmitThread();
    void unTransmitThread(WordThread*);
    void onTransmitThread(WordThread*, uint32_t);
    void onDisplayThread(WordThread*);
    void onSubmitNote(MeetingAddressNote*, uint32_t);
    void onUnloadNote(MeetingAddressNote*, uint32_t);
    void onTransferComplete(WordThread*);
    void onTransmitNote(ExecutorNoteInfo*);

    virtual std::future<TransmitThreadUtil*> submitNote(MeetingAddressNote*, const std::function<void(MeetingAddressNote*)>&);
    virtual void exitNote(MeetingAddressNote*, const std::function<void(MeetingAddressNote*)>&);
protected:
    void submit(std::shared_ptr<ExecutorNote>);
    void submit(std::shared_ptr<ExecutorTask>);
    void requestLaunch(ExecutorStatus) override;
    void requestShutDown(std::promise<void>) override;
    void requestTermination() override;
private:
    void startThread(std::promise<void>&, ExecutorStatus&) throw(std::runtime_error);

    void createThread();
    void closeThread();
    void startThread(BasicThread*);
    void startExecutorThread();
    void startWordThread(std::shared_ptr<StartInstruct>&);
    static void startWordThread0(std::shared_ptr<StartInstruct>&);

    BasicThread* onCreateThread(const ThreadProxy&);
    void onCreateThread(ManagerThread*);
    void onCreateThread(ExecutorThread*);
    void onCreateThread(WordThread*);
    void onCloseThread(const ManagerProxy&);
    void onCloseThread(const ExecutorProxy&);
    void onCloseThread(const WordProxy&);

    BasicThread* onRequestThread(const ExecutorProxy&);
    BasicThread* onRequestThread(const WordProxy&);
    void onResponseThread(ExecutorThread*);
    void onRecoveryThread(ExecutorThread*);
    void onRecoveryThread(WordThread*);

    std::future<void> shutDownThread(BasicThread*);
    void waitThreadShutDown(std::future<void>*, std::promise<void>*, int);

    static void onRecoveryFunc(BasicThread*);
    static thread_proxy core_proxy;

    LaunchMode executor_launch_mode;

    std::atomic_bool idle_thread_flag;
    //不使用锁,只有ManagerThread对该对象进行操作
    std::list<BasicThread*> idle_executor_threads;
    std::multimap<ThreadType, BasicThread*> word_threads;
    std::multimap<ThreadType, BasicThread*>::iterator loop_thread;

    ManagerThread *manager_thread;
    BasicLayer *executor_layer;
};


#endif //UNTITLED5_THREADEXECUTOR_H
