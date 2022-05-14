//
// Created by abc on 20-5-3.
//
#include "ExecutorServer.h"

//TaskHolder<void> ExecutorServer::submit(const LoopExecutorTask &loop_task, ExecutorFunc *efunc_, CalcelFunc *cfunc_, TimeoutFunc *tfunc_) throw(std::logic_error) {
//    TaskHolderInteriorSpecial *holder_interior = TaskHolderInteriorSpecial::generateTaskHolder();
//
//    auto submit_func = [&](LoopExecutorTask *executor_task) -> void{
//                            executor_task->correlateHolder(holder_interior->onIncrease());
//                            executor_task->setOnCallBackFunc(efunc_, cfunc_, tfunc_);
//                            ThreadExecutor::submit(std::static_pointer_cast<ExecutorTask>(
//                                    std::shared_ptr<LoopExecutorTask>(executor_task, ExecutorServer::releaseExecutorTask)));
//                        };
//    createSubmitData(loop_task, submit_func);
//
//    return TaskHolderInteriorSpecial::initTaskHolder(holder_interior);
//}

void ExecutorServer::submitImmediately(HolderInterior *holder_interior, ExecutorFunc *efunc_, CalcelFunc *cfunc_) throw(std::logic_error) {
    auto submit_func = [&](LoopImmediatelyNote *immediately_note) -> void {
                            immediately_note->correlateHolder(holder_interior->onIncrease());
                            immediately_note->setOnCallBackFunc(efunc_, cfunc_);
                            ThreadExecutor::submit(std::shared_ptr<ExecutorNote>(immediately_note, ExecutorServer::releaseExecutorNote));
                       };
    createSubmitData(LoopImmediatelyNote(), submit_func);
}

void ExecutorServer::submitDelay(HolderInterior *holder_interior, const std::chrono::microseconds& time, ExecutorFunc *efunc_, CalcelFunc *cfunc_) throw(std::logic_error) {
    auto submit_func = [&](LoopExecutorTask *loop_task) -> void {
                            loop_task->correlateHolder(holder_interior->onIncrease());
                            loop_task->setOnCallBackFunc(efunc_, cfunc_, nullptr);
                            ThreadExecutor::submit(std::static_pointer_cast<ExecutorTask>(
                                    std::shared_ptr<LoopExecutorTask>(loop_task, ExecutorServer::releaseExecutorTask)));
                       };
    createSubmitData(LoopExecutorTask(std::chrono::duration_cast<std::chrono::duration<uint64_t, std::micro>>(time).count()), submit_func);
}

void ExecutorServer::submitInside(const LoopExecutorTask &loop_task, HolderInterior *holder_interior, ExecutorFunc *efunc, CalcelFunc *cfunc, TimeoutFunc *tfunc) throw(std::logic_error) {
    auto submit_func = [&](LoopExecutorTask *executor_task) -> void {
        executor_task->correlateHolder(holder_interior->onIncrease());
        executor_task->setOnCallBackFunc(efunc, cfunc, tfunc);
        ThreadExecutor::submit(std::static_pointer_cast<ExecutorTask>(
                std::shared_ptr<LoopExecutorTask>(executor_task, ExecutorServer::releaseExecutorTask)));
    };
    createSubmitData(loop_task, submit_func);
}
