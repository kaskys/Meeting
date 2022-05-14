//
// Created by abc on 21-10-9.
//

#ifndef TEXTGDB_DISPLAYWORDTHREAD_H
#define TEXTGDB_DISPLAYWORDTHREAD_H

#include "../thread/WordThread.h"

#define DISPLAY_THREAD_INTERRUPT_VALUE     1

class DisplayThreadUtil;
class DisplayWordThread;

using DisplayMemberFunc = void (DisplayWordThread::*)();

class DisplayWordThread final : public Word {
public:
    DisplayWordThread(WordThread*);
    ~DisplayWordThread() override = default;

    ThreadType type() const override { return THREAD_TYPE_DISPLAY; }
    void exe() override;
    void interrupt() override;
    void onPush(std::shared_ptr<ExecutorTask>) override;
    void onExecutor(std::shared_ptr<ExecutorNote>) override;
    void onInterrupt(SInstruct) throw(finish_error) override;
    void onLoop() override;

    virtual void onCorrelateDisplayUtil(DisplayThreadUtil*);
    virtual DisplayThreadUtil* onCorrelateDisplayUtil() const;

    virtual void onRunDisplayThread(const std::function<void()>&);
private:
    void onFuncWait();
    void onFuncNotify();

    void onStartDisplay(std::shared_ptr<StartInstruct>);
    void onStopDisplay(std::shared_ptr<StopInstruct>);
    void onFinishDisplay(std::shared_ptr<FinishInstruct>);

    void onRunDisplayFunc(const std::function<void()>&);
    void onWaitDisplayFunc(const std::function<void()>&);
    void onDisplayCorrelateUtil(std::shared_ptr<CorrelateDisplayInstruct>);

    static DisplayMemberFunc wait_func;
    static DisplayMemberFunc nofify_func;

    std::atomic<uint32_t> interrupt_amount;
    DisplayThreadUtil *thread_util;
    std::mutex queue_mutex;
    std::condition_variable queue_condition;
    UnLockQueue<std::function<void()>> display_func;
};


#endif //TEXTGDB_DISPLAYWORDTHREAD_H
