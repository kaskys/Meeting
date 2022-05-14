//
// Created by abc on 19-12-6.
//

#ifndef UNTITLED8_DOOR_H
#define UNTITLED8_DOOR_H

#include <iostream>
#include <atomic>
#include <future>

/**
 * 需要管理者？？？
 * 只能由同一个线程id进行开门关门？？？
 */
class Door final{
    friend class UniqueDoor;
public:
    explicit Door(bool status = true);
    ~Door() { open(); };
    /**
     * 开门
     */
    void open() {
        if(is_close()) { //判断是否已经开启
            open0(); //调用开门实现函数
        }
    }
    /**
     * 关门
     */
    void close();
    /**
     * 过门
     */
    void pass();
private:
    bool is_open() const { return door_status.load(std::memory_order_consume); }
    bool is_close() const { return !is_open(); }
    void open0();

    std::atomic_bool door_status;
    std::atomic_int wait_size;
    std::promise<void> door_promise;
    std::shared_future<void> door_future;
};

class UniqueDoor{
public:
    explicit UniqueDoor(Door *door);
    ~UniqueDoor();
private:
    Door *door_;
};

#endif //UNTITLED8_DOOR_H
