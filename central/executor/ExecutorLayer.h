//
// Created by abc on 20-12-23.
//

#ifndef TEXTGDB_EXECUTORLAYER_H
#define TEXTGDB_EXECUTORLAYER_H

#include "../BasicLayer.h"
#include "../../util/heap/BinaryHeap.h"
#include "ExecutorParameter.h"

#define EXECUTOR_LAYER_APPLICATION_TIMER_MORE           3
#define EXECUTOR_LAYER_APPLICATION_TIMER_IMMEDIATELY    4
#define EXECUTOR_LAYER_ADDRESS_JOIN                     5
#define EXECUTOR_LAYER_ADDRESS_EXIT                     6
#define EXECUTOR_LAYER_CONFIRM_TRANSFER                 7
#define EXECUTOR_LAYER_IS_TRANSFER                      8
#define EXECUTOR_LAYER_START_THREAD                     9
#define EXECUTOR_LAYER_STOP_THREAD                      10
#define EXECUTOR_LAYER_CORRELATE_NOTE                   11

#define EXECUTOR_LAYER_TRANSFER_TIME_SECONDS            8
#define EXECUTOR_LAYER_TRANSFER_HEAD_SIZE               8

class TransmitThreadUtil;
class DisplayWordThread;

/**
 * 管理SocketThread接收被转移note的类
 *  接收被转移note的数量
 *  被转移定时器
 *  存储被转移note的数组
 *  关联的SocketThread线程
 */
class ExecutorTransferInfo final{
#define EXECUTOR_TRANSFER_INFO_NOTE_TRANSFER    0x80000000      //标记note是否被转移状态
public:
    explicit ExecutorTransferInfo(WordThread*);
    ~ExecutorTransferInfo();

    void onInitTransfer(MeetingAddressNote*);
    void onUnitTransfer(MeetingAddressNote*);
    void onStartTransfer(TaskHolder<void>&&);
    void onStopTransfer();
    void onNoteTransfer(MeetingAddressNote*);
    bool onNoteConfirm(MeetingAddressNote*);
    int  onTimeoutTransfer();
    void onTimeoutNote(const std::function<bool(MeetingAddressNote*)>&);
    WordThread* getTransferThread() const { return transfer_thread; };
private:
    /**
     * 获取一个没有储存被转移note数组的序号
     * @param transfer_array    序号数组
     * @return
     */
    static int getTransferNotePos(int *transfer_array){
        int note_pos = transfer_array[EXECUTOR_LAYER_TRANSFER_HEAD_SIZE];
        transfer_array[EXECUTOR_LAYER_TRANSFER_HEAD_SIZE] = transfer_array[note_pos];
        return note_pos ;
    }
    /**
     * 释放一个储存被转移note数组的序号
     * @param transfer_array    序号数组
     * @param transfer_pos      序号
     */
    static void setTransferNotePos(int *transfer_array, int transfer_pos){
        transfer_array[transfer_pos] = transfer_array[EXECUTOR_LAYER_TRANSFER_HEAD_SIZE];
        transfer_array[EXECUTOR_LAYER_TRANSFER_HEAD_SIZE] = transfer_pos;
    }

    std::atomic<int> need_transfer_size;                                        //正在被转移note的数量（SocketThread线程接收note的数量）
    TaskHolder<void> timeout_holder;                                            //定制器控制器
    int note_pos_heap[EXECUTOR_LAYER_TRANSFER_HEAD_SIZE + 1];                   //存储被转移note的序号及标记的数组
    MeetingAddressNote *transfer_note_heap[EXECUTOR_LAYER_TRANSFER_HEAD_SIZE];  //储存被转移note的数组
    WordThread *transfer_thread;                                                //关联的SocketThread线程
};

/*
 * 线程执行信息
 */
class ExecutorThreadInfo {
public:
    ExecutorThreadInfo() : note_info(nullptr), note_func(nullptr) {}
    explicit ExecutorThreadInfo(MeetingAddressNote *note, std::function<void(MeetingAddressNote*)> func)
                                                                        : note_info(note), note_func(std::move(func)) {}
    virtual ~ExecutorThreadInfo() = default;

    MeetingAddressNote* getNoteInfo() const { return note_info; }
    std::function<void(MeetingAddressNote*)> getNoteFunc() const { return note_func; }
private:
    MeetingAddressNote *note_info;                          //远程端
    std::function<void(MeetingAddressNote*)> note_func;     //远程端回调函数（运行在LoopThread线程）
};

/*
 * 远程端note加入的执行层数据
 */
class ExecutorInitInfo final : public ExecutorThreadInfo{
public:
    ExecutorInitInfo() : ExecutorThreadInfo(), init_func(nullptr), throw_func(nullptr) {}
    explicit ExecutorInitInfo(MeetingAddressNote *note, std::function<void(MeetingAddressNote*)> nfunc,
                              std::function<void(TransmitThreadUtil*)> ifunc, std::function<void()> tfunc)
            : ExecutorThreadInfo(note, std::move(nfunc)), init_func(std::move(ifunc)), throw_func(std::move(tfunc)) {}
    ~ExecutorInitInfo() override = default;

    void callInitFunc(TransmitThreadUtil *util) { init_func(util); }
    void callThrowFunc() { throw_func(); }
private:
    std::function<void(TransmitThreadUtil*)> init_func;     //加入成功函数
    std::function<void()> throw_func;                       //加入失败函数
};


class ExecutorNoteMediaInfo : public ExecutorNoteInfo {
public:
    explicit ExecutorNoteMediaInfo(uint32_t nsize, uint32_t isize, uint32_t sequence, uint32_t timeout)
                                  : ExecutorNoteInfo(), note_size(nsize), info_size(isize), transmit_sequence(sequence),
                                    timeout_threshold(timeout), submit_func(nullptr), transmit_func(nullptr), executor_func(nullptr),
                                    time_array(nullptr), note_array(nullptr) {
        initArray(note_size, info_size, this);
    }
    ~ExecutorNoteMediaInfo() override = default;

    uint32_t getNoteInfoSize() const override { return note_size; }
    uint32_t getTimeInfoSize() const override { return info_size; }
    MeetingAddressNote* getNoteInfoOnPos(uint32_t) override;
    TimeInfo* getTimeInfoOnPos(uint32_t) override;
    void callExecutorFunc(const ExecutorFunc&) override;
    void callNoteCompleteFunc() override;
    void callNoteTransmitFunc(TransmitThreadUtil*, MeetingAddressNote*, MsgHdr*, uint32_t) override;

    void* operator new(size_t size, size_t len, const std::nothrow_t&) noexcept {
        return malloc((sizeof(ExecutorNoteMediaInfo) * size) + len);
    }
    void operator delete(void *buffer) noexcept {
        if(buffer) { free(buffer); }
    }

    ExecutorNoteMediaInfo* setSubmitFunc(std::function<void(TimeInfo*)> func) {
        submit_func = std::move(func); return this;
    }
    ExecutorNoteMediaInfo* setTransmitFunc(std::function<void(TransmitThreadUtil*, MeetingAddressNote*, MsgHdr*, uint32_t, uint32_t, uint32_t)> func) {
        transmit_func = std::move(func); return this;
    }
    ExecutorNoteMediaInfo* setExecutorFunc(std::function<void(ExecutorNoteInfo*, const ExecutorFunc&)> func) {
        executor_func = std::move(func); return this;
    }
    ExecutorNoteMediaInfo* initNoteInfo(const std::function<void(const std::function<void(MeetingAddressNote*,uint32_t)>&)>&, TimeInfo*);
private:
    static void initArray(uint32_t nsize, uint32_t isize, ExecutorNoteMediaInfo *info){
        info->note_array = reinterpret_cast<MeetingAddressNote*>(info++);
        info->time_array = reinterpret_cast<TimeInfo*>(info->note_array + (sizeof(MeetingAddressNote*) * nsize));
    }
    uint32_t note_size;                             //远程端数量
    uint32_t info_size;                             //视音频资源数量
    uint32_t transmit_sequence;                     //传输序号
    uint32_t timeout_threshold;                     //超时门槛
    std::function<void(TimeInfo*)> submit_func;     //提交函数（时间层销毁资源）
    std::function<void(TransmitThreadUtil*, MeetingAddressNote*, MsgHdr*, uint32_t, uint32_t, uint32_t)> transmit_func; //传输函数（输出数据）
    std::function<void(ExecutorNoteInfo*, const ExecutorFunc&)> executor_func;  //执行函数（运行在SocketThread）
    TimeInfo *time_array;                           //存储视音频
    MeetingAddressNote *note_array;                 //存储远程端
};

class ExecutorLayerUtil : public LayerUtil{
public:
    BasicLayer* createLayer(BasicControl*) noexcept override;
    void destroyLayer(BasicControl*, BasicLayer*) noexcept override;
};

//一对一（一个SocketThread对应一个ExecutorLayer）X
//一对多（一个ExecutorLayer对用多个SocketThread）以下结构
class ExecutorLayer : public BasicLayer, public ThreadExecutorParameter, public TaskExecutorParameter{
    friend class ThreadExecutor;
public:
    ExecutorLayer(BasicControl*);
    ~ExecutorLayer() override;

    void initLayer() override;
    void onInput(MsgHdr*) override;
    void onOutput() override;
    bool isDrive() const override { return false; }
    void onDrive(MsgHdr*) override;
    void onParameter(MsgHdr*) override;
    void onControl(MsgHdr*) override;
    uint32_t onStartLayerLen() const override { return 0; }
    LayerType onLayerType() const override { return LAYER_EXECUTOR_TYPE; }

    template <typename T, typename Func> TaskHolder<T> onTimerImmediately(Func &&func) throw(std::logic_error) {
        return (dynamic_cast<ExecutorServer*>(executor_pool))->submit<T>(std::forward<Func>(func), &executor_func, &cancel_func,
                                                                         [&]() -> void{
                                                                            onRequestTask(ParameterRequestImmediately);
                                                                      }, [&]() -> void{
                                                                            onRequestTask(ParameterRequestFail);
                                                                      });
    };
    template <typename T, typename Func> TaskHolder<T> onTimerDelay(Func &&func, const std::chrono::seconds &time) throw(std::logic_error){
        return (dynamic_cast<ExecutorServer*>(executor_pool))->submit<T>(std::forward<Func>(func), time, &executor_func, &cancel_func,
                                                                         [&]() -> void{
                                                                            onRequestTask(ParameterRequestImmediately);
                                                                      }, [&]() -> void{
                                                                            onRequestTask(ParameterRequestFail);
                                                                      });
    };
    template <typename T, typename Func> TaskHolder<T> onTimerDelay(Func &&func, const std::chrono::milliseconds &time) throw(std::logic_error){
        return (dynamic_cast<ExecutorServer*>(executor_pool))->submit<T>(std::forward<Func>(func), time, &executor_func, &cancel_func,
                                                                         [&]() -> void{
                                                                             onRequestTask(ParameterRequestImmediately);
                                                                         }, [&]() -> void{
                                                                             onRequestTask(ParameterRequestFail);
                                                                         });
    };
    template <typename T, typename Func> TaskHolder<T> onTimerDelay(Func &&func, const std::chrono::microseconds &time) throw(std::logic_error){
        return (dynamic_cast<ExecutorServer*>(executor_pool))->submit<T>(std::forward<Func>(func), time, &executor_func, &cancel_func,
                                                                         [&]() -> void{
                                                                             onRequestTask(ParameterRequestImmediately);
                                                                         }, [&]() -> void{
                                                                             onRequestTask(ParameterRequestFail);
                                                                         });
    };
private:
    static LoopExecutorTask initExecutorTask(ExecutorTimerData*) throw (std::logic_error);
    static bool TransferCompare(const MeetingAddressNote*, const MeetingAddressNote*);
    static void TransferSort(MeetingAddressNote*, int);
    static int TransferSearch(int, int, const std::function<int(int)>&);

    bool launchExecutorLayer();
    void stopExecutorLayer();

    void isTransferAddress(uint32_t, MeetingAddressNote*, const std::function<void(ExecutorTransferInfo*,int)>&);
    bool onAddressInput(MeetingAddressNote*, const std::function<void(MeetingAddressNote*, ExecutorServer*)>&);
    void isTransferThread(MsgHdr*);
    bool isTransferThread0(ExecutorTransferData*);

    void onMoreTimer(MsgHdr*);
    void onMoreTimer0(ExecutorTimerData*, const std::function<void(TaskHolder<void>*)>&);
    void onImmediatelyTimer(MsgHdr*);
    void onImmediatelyTimer0(ExecutorTimerData*, const std::function<void(TaskHolder<void>*)>&);

    void onAddressJoin(MsgHdr*);
    TransmitThreadUtil* onAddressJoin0(ExecutorServer*, ExecutorInitInfo*);
    void onAddressExit(MsgHdr*);
    void onAddressExit0(ExecutorServer*, ExecutorThreadInfo*);
    bool onSocketThreadStart(ExecutorTransferData*);
    void onSocketThreadStop(ExecutorTransferData*);
    void onCorrelateNote(ExecutorNoteInfo*);

    uint32_t onTransmitThread();
    void onTransmitThread(SocketWordThread*, uint32_t);
    void unTransmitThread(SocketWordThread*);
    void onDisplayThread(DisplayWordThread*);

    void onSubmitNote(MeetingAddressNote*, uint32_t);
    void onUnloadNote(MeetingAddressNote*, uint32_t);

    void onAddressTransfer(MeetingAddressNote*, uint32_t, uint32_t);
    void onAddressTransfer0(uint32_t, MeetingAddressNote*, ExecutorTransferInfo*);
    void onConfirmTransfer(MsgHdr*);
    void onTransferTimeout(ExecutorTransferInfo*);
    void onTransferTimeout(ExecutorTransferInfo*, const ExecutorTransferData&);
    void onAddressTimeout(int, ExecutorTransferInfo*);

    BasicExecutor *executor_pool;                                       //线程池管理工具
    std::map<uint32_t, ExecutorTransferInfo*> transfer_info_map;        //存储转移信息的map
};


#endif //TEXTGDB_EXECUTORLAYER_H
