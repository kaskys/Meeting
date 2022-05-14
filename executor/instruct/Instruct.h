//
// Created by abc on 20-5-9.
//

#ifndef UNTITLED5_INSTRUCT_H
#define UNTITLED5_INSTRUCT_H

#include <future>
#include "../../util/queue/UnLockQueue.h"

enum InstructType{
    REQUEST_THREAD,             //线程
    REQUEST_STEAL,              //窃取
    REQUEST_INTERRUPT_POLICY,   //更改中断策略
    REQUEST_RELEASE,

    RESPONSE_THREAD,            //线程
    RESPONSE_STEAL,             //窃取
    RESPONSE_RELEASE,

    INSTRUCT_START,
    INSTRUCT_START_COMPLETE,
    INSTRUCT_START_FAIL,
    INSTRUCT_STOP,
    INSTRUCT_UPDATE_NOTE_SIZE,
    INSTRUCT_TRANSFER,
    INSTRUCT_RECOVERY,
    INSTRUCT_SCOPE,
    INSTRUCT_FINISH,
    INSTRUCT_FLUSH,

    //LoopWordThread
    INSTRUCT_ADDRESS_JOIN,
    INSTRUCT_ADDRESS_EXIT,
    INSTRUCT_ADDRESS_DONE,
    INSTRUCT_ADDRESS_COMPLETE,
    EXCEPTION_SOCKET_RELEASE,
    REQUEST_SOCKET_RELEASE,
    RESPONSE_SOCKET_RELEASE,
    REQUEST_SOCKET_TRANSFER,
    RESPONSE_SOCKET_TRANSFER,
    INSTRUCT_SOCKET_TRANSMIT_NOTE,

    //DisplayWordThread
    INSTRUCT_DISPLAY_CORRELATE
};

enum ThreadType{
    THREAD_TYPE_EXECUTOR = 0,
    THREAD_TYPE_MANAGER,
    THREAD_TYPE_LOOP,
    THREAD_TYPE_SOCKET,
    THREAD_TYPE_DISPLAY
};

enum InterruptPolicy{
    INTERRUPT_DISCARD,      //丢弃
    INTERRUPT_FINISH,       //结束
    INTERRUPT_TRANSFER,     //转移
};

struct MsgHdr;
class TransmitThreadUtil;
class MeetingAddressNote;
class BasicThread;
class BasicExecutor;

struct Instruct{
    explicit Instruct(InstructType type, BasicThread *thread) : instruct_type(type), instruct_thread(thread) {}
    virtual ~Instruct() = default;

    //该函数没有被调用？
    virtual uint32_t classSize() const = 0;

    template <class V, typename... T> static void initInstruct(V *cp, T&&... args) {
        new (reinterpret_cast<void*>(cp)) V(std::forward<T>(args)...);
    }
    template <class V, typename... T> static std::shared_ptr<V> makeInstruct(T&&... args){
        V *cp = nullptr;
        if(typeid(V).before(typeid(Instruct))){
            try {
                cp = reinterpret_cast<V*>(malloc(sizeof(V)));
                Instruct::initInstruct(cp, std::forward<T>(args)...);
            }catch (std::bad_alloc &e){
                std::cout << "makeInstruct:" << typeid(V).name() << "," << sizeof(V) << " -> fail!" << std::endl;
            }
        }
        return std::shared_ptr<V>(cp, releaseInstruct);
    };
    static void releaseInstruct(Instruct *);

    InstructType instruct_type;
    BasicThread *instruct_thread;
};

using SInstruct = std::shared_ptr<Instruct>;

struct ThreadInstruct : public Instruct{
    explicit ThreadInstruct(InstructType instruct_type, BasicThread *instruct_thread, ThreadType type)
                                      : Instruct(instruct_type, instruct_thread), thread_type(type), thread_promise() {}
    ~ThreadInstruct() override = default;

    uint32_t classSize() const override { return sizeof(ThreadInstruct); }

    ThreadType thread_type;
    std::promise<BasicThread*> thread_promise;
};

struct StartInstruct : public Instruct {
    explicit StartInstruct(InstructType instruct_type, BasicThread *instruct_thread, BasicExecutor *pool, std::promise<void> *promise, bool start)
            : Instruct(instruct_type, instruct_thread), start_thread(start), start_fail(false), socket_id(0), start_size(0), executor_pool(pool),
              display_thread(nullptr), start_promise(promise), start_callback(nullptr){}
    ~StartInstruct() override = default;

    uint32_t classSize() const override { return sizeof(StartInstruct); }

    bool start_thread;
    bool start_fail;
    uint32_t socket_id;
    std::atomic_int start_size;
    BasicExecutor *executor_pool;
    BasicThread *display_thread;
    std::promise<void> *start_promise;
    void (*start_callback)(std::shared_ptr<StartInstruct>&);
};

struct StopInstruct : public Instruct{
    using Instruct::Instruct;
    ~StopInstruct() override = default;

    uint32_t classSize() const override { return sizeof(StopInstruct); }

    std::promise<void> stop_promise;
};

struct FinishInstruct : public Instruct{
    using Instruct::Instruct;
    explicit FinishInstruct(InstructType instruct_type, BasicThread *instruct_thread, void(*callback)(BasicThread*))
            : Instruct(instruct_type, instruct_thread), finish_callback(callback) {}
    ~FinishInstruct() override = default;

    uint32_t classSize() const override { return sizeof(FinishInstruct); }

    //线程异步退出时需要设置该回调函数
    //并将该函数赋值给finish_error,从而使线程调用该函数,释放线程内存
    void (*finish_callback)(BasicThread*);
};

struct FlushInstruct : public Instruct{
    FlushInstruct() : Instruct(INSTRUCT_FLUSH, nullptr) {}
    ~FlushInstruct() override = default;

    uint32_t classSize() const override { return sizeof(FlushInstruct); }
};

#endif //UNTITLED5_INSTRUCT_H
