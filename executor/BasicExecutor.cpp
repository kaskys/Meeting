//
// Created by abc on 20-5-3.
//
#include "BasicExecutor.h"

void BasicExecutor::launch() {
    ExecutorStatus status_ = getExecutorStatus();

    if(canLaunch(status_)){
        setExecutorStatus(LAUNCH);
        requestLaunch(status_);
    }
}

void BasicExecutor::shutDown() {
    ExecutorStatus status_ = getExecutorStatus();

    if(canShutDown(status_)) {
        shutDown0();
    }
}

void BasicExecutor::shutDown0() {
    std::promise<void> shut_down_promise;
    shutdown_callback = shut_down_promise.get_future();

    requestShutDown(std::move(shut_down_promise));
    setExecutorStatus(STOP);
}

void BasicExecutor::termination() {
    termination0(getExecutorStatus());
}

void BasicExecutor::termination0(ExecutorStatus status) {
    if(canTermination(status)){
        shutdown_callback.get();

        setExecutorStatus(TERMINATE);
        requestTermination();
    }else if(canShutDown(status)){
        shutDown0();
        termination();
    }else{
        for(;;){
            if((status = getExecutorStatus()) != LAUNCH){
                termination0(status);
                break;
            }
        }
    }
}