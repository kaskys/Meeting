//
// Created by abc on 20-5-1.
//

#ifndef UNTITLED5_EXECUTORTHREAD_H
#define UNTITLED5_EXECUTORTHREAD_H

#include "BasicThread.h"

#define DEFAULT_EXECUTOR_NOTE_SIZE          16
#define DEFAULT_EXECUTOR_STEAL_SIZE         8
#define DEFAULT_EXECUTOR_OPERATOR_VALUE     1

class ExecutorThread;

struct Scope{
    explicit Scope(ExecutorThread *executor_thread) : executor(executor_thread) {}
    virtual ~Scope() = default;

    Scope(const Scope&) = default;
    Scope(Scope&&) = default;
    Scope& operator=(const Scope&) = default;
    Scope& operator=(Scope&&) = default;

    virtual void exe() = 0;
    virtual void response(SInstruct) = 0;
    virtual void convert() = 0;
    virtual void notify() = 0;
    virtual void flush() = 0;
    virtual Scope* transfer() = 0;
    virtual std::function<void()> instruct() = 0;
protected:
    ExecutorThread *executor;
};

struct PoolScope final : public Scope{
#define STATUS_WAIT     true
#define STATUS_NOTIFY   false
    PoolScope() : Scope(nullptr), wait_status(STATUS_WAIT), wait_mutex(), wait_cond(), executor_pool(nullptr) {}
    explicit PoolScope(ExecutorThread *executor, BasicExecutor *pool)
                          : Scope(executor), wait_status(STATUS_WAIT), wait_mutex(), wait_cond(), executor_pool(pool) {}
    ~PoolScope() override = default;

    /*
     * this和pscope假设为一个线程持有
     * 拷贝和移动只对executor_pool进行初始化或赋值,其他wait_status、wait_mutex、wait_cond为默认状态
     */
    PoolScope(const PoolScope &pscope) noexcept : Scope(pscope), wait_status(STATUS_WAIT), wait_mutex(), wait_cond(),
                                                                                  executor_pool(pscope.executor_pool) {}
    PoolScope(PoolScope &&pscope) noexcept : Scope(std::move(pscope)), wait_status(STATUS_WAIT), wait_mutex(), wait_cond(),
                                    executor_pool(pscope.executor_pool) {
        pscope.executor_pool = nullptr;
    }

    PoolScope& operator=(const PoolScope &pscope) noexcept {
        Scope::operator=(pscope); executor_pool = pscope.executor_pool;
        return *this;
    }
    PoolScope& operator=(PoolScope &&pscope) noexcept {
        Scope::operator=(std::move(pscope)); executor_pool = pscope.executor_pool;
        pscope.executor_pool = nullptr; return *this;
    }

    void exe() override;
    void response(SInstruct) override;
    void convert() override;
    void notify() override;
    void flush() override;
    Scope* transfer() override;
    std::function<void()> instruct() override;
private:
    std::atomic_bool wait_status;
    std::mutex wait_mutex;              //阻塞锁
    std::condition_variable wait_cond;
    BasicExecutor *executor_pool;       //线程池

};

struct ManagerScope final : public Scope {
    ManagerScope();
    explicit ManagerScope(ExecutorThread*, BasicThread*);
    ~ManagerScope() override = default;

    ManagerScope(const ManagerScope &scope) noexcept = default;
    ManagerScope(ManagerScope &&scope) noexcept : Scope(std::move(scope)), manager_thread(scope.manager_thread),
                                                                    update_instruct(scope.update_instruct) {
        scope.manager_thread = nullptr;
        update_instruct = UpdateNoteSizeInstruct(INSTRUCT_UPDATE_NOTE_SIZE, nullptr);
    }

    ManagerScope& operator=(const ManagerScope &scope) noexcept = default;
    ManagerScope& operator=(ManagerScope &&scope) noexcept {
        Scope::operator=(std::move(scope)); manager_thread = scope.manager_thread; update_instruct = scope.update_instruct;
        scope.manager_thread = nullptr; update_instruct = UpdateNoteSizeInstruct(INSTRUCT_UPDATE_NOTE_SIZE, nullptr);
        return *this;
    }

    void exe() override;
    void response(SInstruct) override;
    void convert() override;
    void notify() override;
    void flush() override;
    Scope* transfer() override;
    std::function<void()> instruct() override;

    static void emptyRelease(UpdateNoteSizeInstruct*){}
private:
    BasicThread *manager_thread;            //ManagerThread
    UpdateNoteSizeInstruct update_instruct; //更新note数量的指令
};

struct ThreadScope final {
#define THREAD_SCOPE_POOL       true
#define THREAD_SCOPE_MANAGER    false
friend class ExecutorThread;
    ThreadScope() : scope_status(THREAD_SCOPE_POOL), pool_scope() {}
    ~ThreadScope() {
        if(scope_status == THREAD_SCOPE_POOL){
            pool_scope.~PoolScope();
        }else{
            manager_scope.~ManagerScope();
        }
    }

    Scope* onScopeConvert(bool convert_status) {
        Scope *scope = nullptr;
        if(convert_status == THREAD_SCOPE_POOL){
            if(scope_status == THREAD_SCOPE_POOL){
                scope = &pool_scope;
            }else{
                manager_scope.~ManagerScope();
                scope = new (reinterpret_cast<void*>(&pool_scope)) PoolScope();
                scope_status = THREAD_SCOPE_POOL;
            }
        }else{
            if(scope == THREAD_SCOPE_MANAGER){
                scope = &manager_scope;
            }else{
                pool_scope.~PoolScope();
                scope = new (reinterpret_cast<void*>(&manager_scope)) ManagerScope();
                scope_status = THREAD_SCOPE_MANAGER;
            }
        }
        return scope;
    };

    bool scope_status;
    union {
        PoolScope    pool_scope;
        ManagerScope manager_scope;
    };
};

thread_local static std::shared_ptr<Instruct> interrupt_instruct{nullptr};
thread_local static std::shared_ptr<Instruct> scope_instruct{nullptr};

class ExecutorThread final : public BasicThread{
    friend struct PoolScope;
    friend struct ManagerScope;
public:
    ExecutorThread(BasicExecutor*, int max_size = DEFAULT_EXECUTOR_NOTE_SIZE);
    ~ExecutorThread() override = default;

    virtual void onLoop(std::promise<void>&);
    ThreadType type() const override { return THREAD_TYPE_EXECUTOR; }
    std::function<void(std::promise<void>&)> onStart() override { return std::bind(&ExecutorThread::onLoop, this, std::placeholders::_1); }
    void receiveTask(std::shared_ptr<ExecutorNote>) throw(std::bad_alloc, thread_load_error) override;
    void receiveTask(std::shared_ptr<ExecutorTask>) override;
    void receiveInstruct(SInstruct) override;

    int maxNoteSize() const { return max_note_size; }
    void onWaitUp() { scope->notify(); }
    void correlateManagerThread(BasicThread *manager_thread) { initManagerScope(manager_thread); }
protected:
    void handleInterrupt() override;
    void handleLoop() override;
private:
    void initPoolScope(BasicExecutor *pool) { thread_scope.pool_scope = PoolScope(this, pool); }
    void initManagerScope(BasicThread *manager_thread) { thread_scope.manager_scope = ManagerScope(this, manager_thread); }
    void clearInterrupt();

    void onTransfer(Scope*);
    void onSteal();
    void onPolicy();
    void onRelease();
    void onRecovery(SInstruct);
    void onStop();
    void onFinish(void(*)(BasicThread*)) throw(finish_error);

    void receiveTask0(std::shared_ptr<ExecutorNote> &&note) { task_queue.push(std::move(note)); }
    void transferQueue();
    void completeQueue();

    int max_note_size;              //最大note数量
    Scope *scope;                   //thread_scope的指针
    SInstruct *receive_instruct;    //接收指令
    std::atomic_int note_size;      //当前note数量
    UnLockQueue<std::shared_ptr<ExecutorNote>> task_queue;  //note队列
    ThreadScope thread_scope;       //ExecutorThread线程的范围
};

#endif //UNTITLED5_EXECUTORTHREAD_H
