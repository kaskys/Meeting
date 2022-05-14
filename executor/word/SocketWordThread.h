//
// Created by abc on 20-4-21.
//

#ifndef UNTITLED5_SOCKETWORDTHREAD_H
#define UNTITLED5_SOCKETWORDTHREAD_H

#include <mutex>
#include <map>

#include "../thread/WordThread.h"
#include "../../concurrent/semaphore/Semaphore.h"

#define SOCKET_PERMIT_DEFAULT_SIZE              8
//ExecutorThread线程的最大值是15个,所以阻塞线程不会超过存储函数的数量
#define SOCKET_EXECUTOR_FUNC_DEFAULT_SIZE       (SOCKET_PERMIT_DEFAULT_SIZE * 2)
//存储函数的实际大小（ExecutorThread线程的最大值（16） + LoopThread线程 + 主SocketThread线程 + ManagerThread线程 + 用户线程）
#define SOCKET_EXECUTOR_FUNC_ACTUAL_SIZE        (SOCKET_EXECUTOR_FUNC_DEFAULT_SIZE + 4)
#define SOCKET_EXECUTOR_FUNC_MIN_SIZE           (SOCKET_EXECUTOR_FUNC_DEFAULT_SIZE >> 2)
#define SOCKET_READ_INTERRUPT_FLAG              0x00000001
#define SOCKET_DEFAULT_FUNC_INTERRUPT_FLAG      0x00000002
#define SOCKET_IMMEDIATELY_FUNC_INTERRUPT_FLAG  0x00000004

enum SocketStatus{
    SOCKET_STATUS_INITIALIZE,   //线程正在初始化
    SOCKET_STATUS_RUNNING,      //线程正在运行
    SOCKET_STATUS_EXCEPTION,    //线程正在异常
    SOCKET_STATUS_STOP          //线程正在停止
};

//SocketThread线程函数执行类型
enum SocketFuncType{
    SOCKET_FUNC_TYPE_DEFAULT,       //默认（等待满载）
    SOCKET_FUNC_TYPE_IMMEDIATELY    //立即
};

class SocketWordThread;
class SocketPermitFactory;
struct TransmitOutputData;
using PermitSocket = Permit<SocketWordThread, MeetingAddressNote>;
using SocketExceptionFunc = void(*)(SocketWordThread*);

/**
 * SocketThread线程执行函数模式
 */
class SocketFuncMode final {
#define SOCKET_FUNC_MODE_EXECUTOR_NOTIFY_FLAG       0x80000000
    friend class SocketWordThread;
    //函数阻塞类
    class ModeBlock {
    public:
        ModeBlock() : block_flag(false), block_size(0), block_promise(), block_future() {
            block_future = block_promise.get_future();
        }
        ~ModeBlock() = default;

        /**
         * 函数等待阻塞（阻塞线程添加函数）
         */
        void onModeLock() throw(std::runtime_error) {
            //设置阻塞future
            std::shared_future<void> lock_future = block_future;

            //判断是否正在初始化阻塞标志
            if(block_flag.load(std::memory_order_consume)){
                //正在初始化,直接返回
                return;
            }

            //增加等待阻塞数量
            block_size.fetch_add(1, std::memory_order_release);
            try {
                //阻塞等待
                lock_future.get();
            }catch (std::future_error &e){
                //这里是因为block_promise的销毁而导致的异常（正在阻塞等待的线程因SocketThread的销毁而block_promise被销毁）
                throw std::runtime_error(nullptr);
            }
            //唤醒阻塞,减少等待阻塞数量
            block_size.fetch_sub(1, std::memory_order_release);
        }

        /**
         * 函数阻塞唤醒（唤醒正在阻塞的线程）
         */
        void onModeNotify(){
            //设置唤醒阻塞
            block_promise.set_value();

            //设置初始化标志
            block_flag.store(true, std::memory_order_release);

            //初始化promise和future
            block_future = (block_promise = std::promise<void>()).get_future();

            //复位初始化标志
            block_flag.store(false, std::memory_order_release);
        }
        /**
         * 正在阻塞线程的数量
         * @return
         */
        uint32_t onModeLockSize() const { return block_size.load(std::memory_order_consume); }
    private:
        std::atomic<bool> block_flag;           //初始化promise和future的标志（用于防止未完全初始化而泄漏）
        std::atomic<uint32_t> block_size;       //正在阻塞的线程数量
        std::promise<void> block_promise;       //函数唤醒
        std::shared_future<void> block_future;  //函数阻塞
    };
public:
    explicit SocketFuncMode(SocketWordThread*);
    ~SocketFuncMode() = default;

    bool isSocketFuncFull(int min_size = SOCKET_EXECUTOR_FUNC_MIN_SIZE) const { return (executor_func_size.load(std::memory_order_consume) >= min_size); }
    int  onSocketFuncFull();
    void setSocketExecutorFunc(const std::function<void()>&);
    void setSocketImmediatelyFunc(const std::function<void()>&);
    void onExecutorSocketFunc(int);
private:
    void setSocketFunc0(const std::function<void()>&, int);
    int  onSocketFuncPos(const std::function<int(int)>&);
    void onExecutorSocketFunc0(int);
    void onSocketInterrupt(int);
    void onSocketDone();

    std::atomic<int> executor_func_size;        //执行函数的数量

    //存储函数的数组（ExecutorThread线程的最大值 + LoopThread线程 + 主SocketThread线程 + ManagerThread线程 + 用户线程）
    std::function<void()> executor_socket_func[SOCKET_EXECUTOR_FUNC_ACTUAL_SIZE];
    SocketWordThread *socket_thread;                                                    //关联的SocketThread线程
    ModeBlock socket_mode_block;
};

/**
 * SocketThread线程许可,用于接收及发送远程端的消息
 *  许可(permit)分为两个子许可(1.SocketThread线程：接收,2.远程端note：发送)
 *  只有许可的两个子许可都释放才释放许可（释放SocketThread线程子许可：无法发送消息,释放远程端note子许可：无法接收消息）
 */
class SocketPermit final : public PermitSocket {
public:
    using PermitSocket::PermitSocket;
    ~SocketPermit() override = default;

    SocketPermit(const SocketPermit &socket_permit) noexcept : Permit(socket_permit) {}
    SocketPermit(SocketPermit &&socket_permit) noexcept : Permit(std::move(socket_permit)) {}

    SocketPermit& operator=(const SocketPermit &socket_permit) noexcept {
        Permit::operator=(socket_permit); return *this;
    }
    SocketPermit& operator=(SocketPermit &&socket_permit) noexcept {
        Permit::operator=(std::move(socket_permit)); return *this;
    }

    /*
     * 获取SocketThread线性的子许可
     */
    SocketWordThread* getSocketWordThread() const { return master; }
    /*
     * 获取远程端的子许可
     */
    MeetingAddressNote* getMeetingAddressNote() const { return servant; }
};

/**
 * Socket许可创建工厂
 */
struct SocketPermitFactory final{
#define SOCKET_PERMIT_FACTORY_CREATE_DEFAULT_SIZE   1
    static SocketPermit* createPermit(const PermitSocket&);
    static void destroyPermit(PermitSocket*);
    static std::allocator<SocketPermit> alloc_;
};

class SocketWordThread final : public Word {
    friend class SocketPermitFactory;
    friend class SocketExecutorTask;
    friend class TransmitThreadUtil;
public:
    SocketWordThread(WordThread*);
    ~SocketWordThread() override;

    ThreadType type() const override { return THREAD_TYPE_SOCKET; }
    void exe() override;
    void interrupt() override;
    void onPush(std::shared_ptr<ExecutorTask>) override;
    void onInterrupt(SInstruct) throw(finish_error) override;
    void onExecutor(std::shared_ptr<ExecutorNote>) override;
    void onLoop() override;
    virtual void onException();

    std::shared_ptr<SocketExecutorTask> getSocketExecutorTask() { return std::shared_ptr<SocketExecutorTask>(&socket_task, SocketWordThread::emptyReleaseSocketTask); }

    void onRunSocketThread(const std::function<void()> &func, SocketFuncType type = SOCKET_FUNC_TYPE_DEFAULT){
        if(isSocketStatus(SOCKET_STATUS_STOP)){ return; }
        isCorrelateThread() ? func() : onFuncExecutorSocket(func, type);
    }

    /**
     * 判断SocketThread线程连接的远程端是否超出最大连接数
     * @return
     */
    bool verifyFullLoad() const { return (socket_semaphore.emptySemaphore()); }
    bool verifyPermitNote(MeetingAddressNote*) const;
    bool verifyPermitThread(MeetingAddressNote*) const;
    void onInterrupt(int);

    void releasePermit(MeetingAddressNote*);
    bool acquirePermit(MeetingAddressNote*);
    bool isCorrelateThread() const { return (std::this_thread::get_id() == word_thread->id()); }
    void setTransferPos(int pos) { transfer_pos = pos; }
    int  getTransferPos() const { return transfer_pos; }
    int  onSocketFd() const;
    /**
     * 关联LoopThread线程
     * @param lthread 线程
     */
    void correlateLoopWordThread(WordThread *lthread) { loop_thread = lthread; }
    void correlateTransmit(TransmitThreadUtil *tutil) { thread_util = tutil; }
    TransmitThreadUtil* correlateTransmit() const { return thread_util; }
    uint32_t correlateTransmitId() const;

    static void emptyReleaseSocketTask(SocketExecutorTask*){}
private:
    static uint32_t getSocketPermitPos(int *pos_array){
        uint32_t permit_pos = static_cast<uint32_t>(pos_array[0]);
        pos_array[0] = pos_array[permit_pos];

        return (permit_pos - 1);
    }
    static void setSocketPermitPos(int *pos_array, uint32_t pos){
        pos_array[pos + 1] = pos_array[0];
        pos_array[0] = (pos + 1);
    }
    static void onResetInterruptFlag(volatile int &interrupt_flag, int reset_flag, const std::function<void()> &callback){
        interrupt_flag &= ~reset_flag;
        callback();
    }
    void permitRelease(PermitSocket *permit){ socket_semaphore.release(permit); }

    /**
     * 判断当前状态
     * @param status    判断状态
     * @return          是否判断状态
     */
    bool isSocketStatus(SocketStatus status) const { return (socket_status.load(std::memory_order_consume) == status); }
    void onSocketStatus(SocketStatus status) { socket_status.store(status, std::memory_order_release); }

    void onException0();
    void waitRead();
    void notifyRead();
    void writeData(const TransmitOutputData&);
    void onFuncExecutorSocket(const std::function<void()>&, SocketFuncType);

    void onNoneAddress();
    void onAddressDone(std::shared_ptr<ReleaseSocketInstruct>);
    void onReleaseSocket(std::shared_ptr<ReleaseSocketInstruct>);
    void onReleaseSocket0(std::shared_ptr<ReleaseSocketInstruct>);
    void onTransmitNote(std::shared_ptr<TransmitNoteInstruct>);

    void onStartSocket(std::shared_ptr<StartInstruct>);
    void onStopSocket(std::shared_ptr<StopSocketInstruct>);
    void onTransferSocket(std::shared_ptr<TransferSocketInstruct>);
    void onTransferSocketDone();
    void onFinishSocket(std::shared_ptr<FinishInstruct>) throw(finish_error);

    bool is_socket_transfer;
    int transfer_pos;
    int unpermit_pos[SOCKET_PERMIT_DEFAULT_SIZE + 1];   //没有许可的序号（与socket_permit的关系 -> unpermit_pos - 1 = socket_permits）
    volatile int interrupt_gemini;                      //输入或执行函数阻塞标志
    WordThread *loop_thread;                            //LoopThread线程
    TransmitThreadUtil *thread_util;                    //关联传输层的工具类

    std::function<void(PermitSocket*)> release_func;    //许可释放函数
    std::atomic<SocketStatus> socket_status;            //SocketThread线程状态
    std::mutex socket_mutex;                            //SocketThread线程锁（用于等待接收远程端消息）
    std::condition_variable socket_condition;           //同上

    SocketFuncMode     socket_func_mode;                //运行在SocketThread线程的函数
    SocketExecutorTask socket_task;                     //SocketThread线程任务

    SocketPermit *socket_permits[SOCKET_PERMIT_DEFAULT_SIZE];       //许可数组（SocketThread线程的远程端连接许可集合）
    Semaphore<PermitSocket, SocketPermitFactory> socket_semaphore;  //许可剩余量（远程端连接需要获取）
};

#endif //UNTITLED5_SOCKETWORDTHREAD_H
