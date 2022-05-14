//
// Created by abc on 20-4-21.
//
#include "BasicThread.h"

static thread_local InterruptFlag interrupt_flag;   //中断标志

/**
 * 启动线程
 * @param start_promise
 */
void BasicThread::startThread(std::promise<void> &start_promise) {
    if(!start){
        //更改为已启动线程
        start = true;
        //调用派生类的onStart函数
        m_thread = std::thread(onStart(), std::ref(start_promise));
    }
}

void BasicThread::onLoop() {
    interrupt = &interrupt_flag;
    onExecutor();
}


