//
// Created by abc on 20-5-6.
//

#ifndef UNTITLED5_INSTRUCTUTIL_H
#define UNTITLED5_INSTRUCTUTIL_H

#include "ExecutorInstruct.h"
#include "LoopWordInstruct.h"
#include "SocketWordInsruct.h"
#include "DisplayWordInstruct.h"

class thread_load_error : public std::exception{
public:
    using exception::exception;
    ~thread_load_error() override = default;

    const char* what() const noexcept override { return nullptr; }
};

class finish_error : public std::exception{
    using CallBackFunc = void (*)(BasicThread*);
public:
    using exception::exception;
    explicit finish_error(CallBackFunc callback) : std::exception(), finish_callback(callback) {}
    ~finish_error() override = default;

    const char* what() const noexcept override { return nullptr; }

    CallBackFunc getCallBack() const { return finish_callback; }
private:
    CallBackFunc finish_callback;
};

#endif //UNTITLED5_INSTRUCTUTIL_H
