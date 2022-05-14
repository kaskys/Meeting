//
// Created by abc on 20-5-10.
//
#include "../word/SocketWordThread.h"

std::allocator<SocketExecutorNote> SocketExecutorTask::alloc = std::allocator<SocketExecutorNote>();

ExecutorNote* SocketExecutorTask::createExecutorNote(SocketExecutorTask *socket)  {
    SocketExecutorNote *socket_note = nullptr;
    try {
        socket_note = alloc.allocate(1);
        alloc.construct(socket_note, socket);
    } catch (std::bad_alloc &e){
        std::cout << "SocketExecutorTask::createExecutorNote bad_alloc!" << std::endl;
    }
    return socket_note;
}

void SocketExecutorTask::destroyExecutorNote(ExecutorNote *executor_note) {
    if(executor_note){
        alloc.destroy(executor_note);
        alloc.deallocate(dynamic_cast<SocketExecutorNote*>(executor_note), 1);
    }
}

/**
 * 返回Socket线程的socket描述符
 * @return 描述符
 */
int SocketExecutorTask::getSocketFd() const {
    return socket_word_thread->onSocketFd();
}

/**
 * 返回Socket线程的执行Note
 * @return
 */
std::shared_ptr<ExecutorNote> SocketExecutorTask::executor() {
    return std::shared_ptr<ExecutorNote>(createExecutorNote(this), SocketExecutorTask::destroyExecutorNote);
}

/**
 * 执行Note
 */
void SocketExecutorTask::executorSocket() {
    socket_word_thread->onInterrupt(SOCKET_READ_INTERRUPT_FLAG);
}

