//
// Created by abc on 20-5-10.
//

#ifndef UNTITLED5_SOCKETWORDTASK_H
#define UNTITLED5_SOCKETWORDTASK_H

#include "ExecutorTask.h"

/**
 * Socket任务
 */
class SocketExecutorTask final : public ExecutorTask{
    static ExecutorNote* createExecutorNote(SocketExecutorTask*);
    static void destroyExecutorNote(ExecutorNote*);
public:
    explicit SocketExecutorTask(SocketWordThread *word_thread) : ExecutorTask(), socket_word_thread(word_thread) {}
    ~SocketExecutorTask() override = default;

    uint32_t classSize() const override { return sizeof(SocketExecutorTask); }
    TaskType type() const override { return TASK_TYPE_SOCKET; }
    std::shared_ptr<ExecutorNote> executor() override;

    int getSocketFd() const;
    void executorSocket();
private:
    static std::allocator<SocketExecutorNote> alloc;    //Socket任务构造器
    SocketWordThread *socket_word_thread;               //Socket线程
};

/**
 * Socket任务执行Note
 */
struct SocketExecutorNote final : public ExecutorNote{
    explicit SocketExecutorNote(SocketExecutorTask *socket) : ExecutorNote(), socket_task(socket) {}
    ~SocketExecutorNote() override = default;

    uint32_t classSize() const override { return sizeof(SocketExecutorNote); }
    bool isCancel() const override { return false; }
    void executor() override { socket_task->executorSocket(); }
    void cancel() override {}
    void fail() override {}
    bool needCorrelate() const override { return false; }
    void setCorrelateThread(BasicThread*) override {}
    BasicThread* getCorrelateThread() const override { return nullptr; }
private:
    SocketExecutorTask *socket_task;
};

#endif //UNTITLED5_SOCKETWORDTASK_H
